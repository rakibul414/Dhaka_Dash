#include "ObstacleManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <random>

#include "Config.h"

ObstacleManager::ObstacleManager() : rng_(std::random_device{}()) {
    reset();
}

void ObstacleManager::reset() {
    for (auto& o : pool_) {
        o = Obstacle{};
    }
    activeCount_ = 0;
    recentLanes_ = {{-1, -1, -1}};
    recentLaneCursor_ = 0;
    consecutiveCenterSpawns_ = 0;
    elapsedTime_ = 0.0f;
    spawnTimer_ = 0.0f;
    nextSpawnIn_ = 3.6f;
    lastTripleFakeTime_ = -1000.0f;
}

int ObstacleManager::findInactiveSlot() {
    for (int i = 0; i < kMaxObstacles; ++i) {
        if (!pool_[i].active) {
            return i;
        }
    }
    return -1;
}

int ObstacleManager::laneFromX(float x) const {
    float bestDist = std::abs(x - cfg::laneX[0]);
    int best = 0;
    for (int i = 1; i < 3; ++i) {
        const float d = std::abs(x - cfg::laneX[i]);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

int ObstacleManager::laneMaskForBox(float minX, float maxX) const {
    static constexpr float laneMin[3] = {-6.0f, -2.0f, 2.0f};
    static constexpr float laneMax[3] = {-2.0f, 2.0f, 6.0f};

    int mask = 0;
    for (int lane = 0; lane < 3; ++lane) {
        if (maxX >= laneMin[lane] && minX <= laneMax[lane]) {
            mask |= (1 << lane);
        }
    }
    return mask;
}

int ObstacleManager::laneMaskWithinBand(float minZ, float maxZ) const {
    int mask = 0;
    for (const auto& o : pool_) {
        if (!o.active) {
            continue;
        }

        const float halfW = o.width * (o.isWide ? 0.65f : 0.5f);
        const float halfD = o.depth * 0.5f;
        const float oMinZ = o.z - halfD;
        const float oMaxZ = o.z + halfD;
        if (oMaxZ < minZ || oMinZ > maxZ) {
            continue;
        }

        mask |= laneMaskForBox(o.x - halfW, o.x + halfW);
    }
    return mask;
}

void ObstacleManager::enforceEscapeLaneNearPlayer(float playerZ) {
    const float bandMin = playerZ - 0.95f;
    const float bandMax = playerZ + 0.95f;
    const int mask = laneMaskWithinBand(bandMin, bandMax) & 0b111;
    if (mask != 0b111) {
        return;
    }

    int bestIdx = -1;
    float bestDist = 1e9f;

    for (int i = 0; i < kMaxObstacles; ++i) {
        const auto& o = pool_[i];
        if (!o.active) {
            continue;
        }

        const float halfD = o.depth * 0.5f;
        const float oMinZ = o.z - halfD;
        const float oMaxZ = o.z + halfD;
        if (oMaxZ < bandMin || oMinZ > bandMax) {
            continue;
        }

        const float d = std::abs(o.z - playerZ);
        if (d < bestDist) {
            bestDist = d;
            bestIdx = i;
        }
    }

    if (bestIdx >= 0) {
        pool_[bestIdx].active = false;
        pool_[bestIdx].closeCallChecked = true;
        activeCount_ = std::max(0, activeCount_ - 1);
        std::cout << "Safety override - opened escape lane\n";
    }
}

bool ObstacleManager::isSafeToSpawn(int lane, float spawnZ) {
    const int occupiedMask = laneMaskWithinBand(spawnZ - 8.0f, spawnZ + 8.0f);

    const float cx = cfg::laneX[lane];
    const float halfW = pendingSpawnWidth_ * (pendingSpawnIsWide_ ? 0.65f : 0.5f);
    const int spawnMask = laneMaskForBox(cx - halfW, cx + halfW);

    const int combinedMask = occupiedMask | spawnMask;
    const bool blocksAllLanes = (combinedMask & 0b111) == 0b111;
    if (!blocksAllLanes) {
        return true;
    }

    // Triple fake can bypass strict check only with guaranteed escape window.
    if (pendingTripleFake_ && pendingEscapeWindowSec_ >= 0.8f) {
        return true;
    }
    return false;
}

int ObstacleManager::chooseSafeLane(float spawnZ, int playerLane) {
    std::array<float, 3> weights{{0.325f, 0.35f, 0.325f}};

    if (consecutiveCenterSpawns_ >= 2) {
        weights[1] = 0.0f;
    }

    for (int lane = 0; lane < 3; ++lane) {
        if (!isSafeToSpawn(lane, spawnZ)) {
            weights[lane] = 0.0f;
            continue;
        }

        if (lane == playerLane) {
            weights[lane] *= 0.55f;
        }

        for (int recentLane : recentLanes_) {
            if (recentLane == lane) {
                weights[lane] *= 0.58f;
                break;
            }
        }
    }

    float total = weights[0] + weights[1] + weights[2];
    if (total <= 0.0001f) {
        for (int lane = 0; lane < 3; ++lane) {
            if (isSafeToSpawn(lane, spawnZ)) {
                return lane;
            }
        }
        return 1;
    }

    std::uniform_real_distribution<float> pick(0.0f, total);
    float r = pick(rng_);
    for (int lane = 0; lane < 3; ++lane) {
        r -= weights[lane];
        if (r <= 0.0f) {
            return lane;
        }
    }
    return 2;
}

float ObstacleManager::tierSpeedMultiplier(float elapsedSeconds) const {
    if (difficultyPreset_ == cfg::PRESET_CASUAL) {
        // Easy: starts slow and ramps gently.
        if (elapsedSeconds < 30.0f) return 0.78f;
        if (elapsedSeconds < 90.0f) return 0.95f;
        if (elapsedSeconds < 180.0f) return 1.12f;
        return 1.28f + std::floor(std::max(0.0f, elapsedSeconds - 180.0f) / 45.0f) * 0.06f;
    }

    if (difficultyPreset_ == cfg::PRESET_INSANE) {
        // Insane: very fast from the beginning.
        if (elapsedSeconds < 30.0f) return 1.8f;
        if (elapsedSeconds < 90.0f) return 2.0f;
        if (elapsedSeconds < 180.0f) return 2.25f;
        return 2.5f + std::floor(std::max(0.0f, elapsedSeconds - 180.0f) / 30.0f) * 0.14f;
    }

    // Competitive (hard): gradual but challenging ramp.
    if (elapsedSeconds < 30.0f) return 1.0f;
    if (elapsedSeconds < 90.0f) return 1.25f;
    if (elapsedSeconds < 180.0f) return 1.55f;
    return 1.9f + std::floor(std::max(0.0f, elapsedSeconds - 180.0f) / 40.0f) * 0.1f;
}

int ObstacleManager::maxActiveForTier(float elapsedSeconds) const {
    if (difficultyPreset_ == cfg::PRESET_CASUAL) {
        if (elapsedSeconds < 30.0f) return 2;
        if (elapsedSeconds < 90.0f) return 3;
        if (elapsedSeconds < 180.0f) return 4;
        return 5;
    }

    if (difficultyPreset_ == cfg::PRESET_INSANE) {
        if (elapsedSeconds < 30.0f) return 5;
        if (elapsedSeconds < 90.0f) return 6;
        if (elapsedSeconds < 180.0f) return 7;
        return 8;
    }

    // Competitive
    if (elapsedSeconds < 30.0f) return 3;
    if (elapsedSeconds < 90.0f) return 4;
    if (elapsedSeconds < 180.0f) return 5;
    return 6;
}

float ObstacleManager::nextSpawnInterval(float elapsedSeconds) {
    float minGap = 3.5f;
    float maxGap = 5.0f;

    if (difficultyPreset_ == cfg::PRESET_CASUAL) {
        // Easy: fewer cars, more breathing room.
        if (elapsedSeconds < 30.0f) {
            minGap = 4.2f;
            maxGap = 6.0f;
        } else if (elapsedSeconds < 90.0f) {
            minGap = 3.5f;
            maxGap = 5.0f;
        } else if (elapsedSeconds < 180.0f) {
            minGap = 2.8f;
            maxGap = 4.0f;
        } else {
            minGap = 2.2f;
            maxGap = 3.2f;
        }
    } else if (difficultyPreset_ == cfg::PRESET_INSANE) {
        // Insane: high density from the start.
        if (elapsedSeconds < 30.0f) {
            minGap = 1.4f;
            maxGap = 2.2f;
        } else if (elapsedSeconds < 90.0f) {
            minGap = 1.2f;
            maxGap = 1.8f;
        } else if (elapsedSeconds < 180.0f) {
            minGap = 1.0f;
            maxGap = 1.5f;
        } else {
            minGap = 0.85f;
            maxGap = 1.3f;
            const float inCycle = std::fmod(std::max(0.0f, elapsedSeconds - 180.0f), 30.0f);
            if (inCycle < 4.0f) {
                return 0.65f;
            }
        }
    } else {
        // Competitive (hard): moderate density, gradually harder.
        if (elapsedSeconds < 30.0f) {
            minGap = 3.4f;
            maxGap = 4.8f;
        } else if (elapsedSeconds < 90.0f) {
            minGap = 2.8f;
            maxGap = 4.0f;
        } else if (elapsedSeconds < 180.0f) {
            minGap = 2.1f;
            maxGap = 3.2f;
        } else {
            minGap = 1.6f;
            maxGap = 2.6f;
        }
    }

    std::uniform_real_distribution<float> gapDist(minGap, maxGap);
    return gapDist(rng_);
}

int ObstacleManager::chooseVehicleType() {
    // BUS 25%, CNG 30%, TRUCK 20%, RICKSHAW 25%
    std::uniform_real_distribution<float> roll(0.0f, 1.0f);
    const float r = roll(rng_);
    if (r < 0.25f) {
        return VEHICLE_BUS;
    }
    if (r < 0.55f) {
        return VEHICLE_CNG;
    }
    if (r < 0.75f) {
        return VEHICLE_TRUCK;
    }
    return VEHICLE_RICKSHAW;
}

void ObstacleManager::activateVehicle(int slot,
                                      int lane,
                                      float spawnZ,
                                      int vehicleType,
                                      float elapsedSeconds,
                                      int pattern,
                                      float zOffset) {
    if (slot < 0 || slot >= kMaxObstacles) {
        return;
    }

    Obstacle& o = pool_[slot];
    o = Obstacle{};
    o.active = true;
    o.lane = lane;
    o.x = cfg::laneX[lane];
    o.z = spawnZ + zOffset;
    o.pattern = pattern;
    o.vehicleType = vehicleType;
    o.closeCallChecked = false;

    switch (vehicleType) {
        case VEHICLE_BUS:
            o.type = 2;
            o.width = 2.45f;
            o.height = 2.55f;
            o.depth = 5.2f;
            o.color = {0.92f, 0.22f, 0.16f};
            o.isWide = true;
            break;
        case VEHICLE_CNG:
            o.type = 1;
            o.width = 1.55f;
            o.height = 1.45f;
            o.depth = 2.7f;
            o.color = {0.18f, 0.72f, 0.28f};
            o.isWide = false;
            break;
        case VEHICLE_TRUCK:
            o.type = 2;
            o.width = 2.35f;
            o.height = 2.85f;
            o.depth = 4.8f;
            o.color = {0.25f, 0.3f, 0.45f};
            o.isWide = true;
            break;
        default:
            o.type = 1;
            o.width = 1.7f;
            o.height = 1.65f;
            o.depth = 3.1f;
            o.color = {0.75f, 0.8f, 0.2f};
            o.isWide = false;
            break;
    }

    float vehicleMultiplier = 1.0f;
    if (vehicleType == VEHICLE_BUS) {
        vehicleMultiplier = 0.82f;
    } else if (vehicleType == VEHICLE_CNG) {
        vehicleMultiplier = 1.08f;
    } else if (vehicleType == VEHICLE_TRUCK) {
        vehicleMultiplier = 0.95f;
    } else {
        std::uniform_real_distribution<float> rickshawSpeed(0.85f, 1.2f);
        vehicleMultiplier = rickshawSpeed(rng_);
    }

    o.speed = cfg::obstacleBaseSpeed * tierSpeedMultiplier(elapsedSeconds) * vehicleMultiplier;

    if (pattern == PATTERN_TRIPLE_FAKE) {
        // Keep triple-fake speed low enough to preserve ~0.8s escape timing.
        o.speed = std::min(o.speed, 3.0f);
    }

    recentLanes_[recentLaneCursor_] = lane;
    recentLaneCursor_ = (recentLaneCursor_ + 1) % static_cast<int>(recentLanes_.size());
    if (lane == 1) {
        ++consecutiveCenterSpawns_;
    } else {
        consecutiveCenterSpawns_ = 0;
    }

    ++activeCount_;
}

int ObstacleManager::choosePattern(float elapsedSeconds) {
    std::uniform_real_distribution<float> roll(0.0f, 1.0f);
    const float r = roll(rng_);

    if (difficultyPreset_ == cfg::PRESET_CASUAL) {
        // Easy: fewer complex patterns.
        if (elapsedSeconds < 30.0f) {
            return (r < 0.92f) ? PATTERN_SINGLE : PATTERN_DOUBLE_GAP;
        }
        if (elapsedSeconds < 90.0f) {
            if (r < 0.80f) return PATTERN_SINGLE;
            if (r < 0.98f) return PATTERN_DOUBLE_GAP;
            return PATTERN_CONVOY;
        }
        if (elapsedSeconds < 180.0f) {
            if (r < 0.62f) return PATTERN_SINGLE;
            if (r < 0.92f) return PATTERN_DOUBLE_GAP;
            if (r < 0.98f) return PATTERN_CONVOY;
            return PATTERN_STAGGER;
        }

        if (r < 0.48f) return PATTERN_SINGLE;
        if (r < 0.84f) return PATTERN_DOUBLE_GAP;
        if (r < 0.95f) return PATTERN_CONVOY;
        return PATTERN_STAGGER;
    }

    if (difficultyPreset_ == cfg::PRESET_COMPETITIVE) {
        // Hard: mostly manageable, rare 3-lane style pressure with one escape.
        if (elapsedSeconds < 30.0f) {
            return (r < 0.85f) ? PATTERN_SINGLE : PATTERN_DOUBLE_GAP;
        }
        if (elapsedSeconds < 90.0f) {
            if (r < 0.65f) return PATTERN_SINGLE;
            if (r < 0.95f) return PATTERN_DOUBLE_GAP;
            return PATTERN_STAGGER;
        }
        if (elapsedSeconds < 180.0f) {
            if (r < 0.45f) return PATTERN_SINGLE;
            if (r < 0.90f) return PATTERN_DOUBLE_GAP;
            if (r < 0.97f) return PATTERN_STAGGER;
            return PATTERN_CONVOY;
        }

        if (r < 0.30f) return PATTERN_SINGLE;
        if (r < 0.82f) return PATTERN_DOUBLE_GAP;
        if (r < 0.92f) return PATTERN_STAGGER;
        if (r < 0.98f) return PATTERN_CONVOY;
        // Very low ratio for triple fake in hard mode.
        if (elapsedSeconds - lastTripleFakeTime_ >= 35.0f) {
            return PATTERN_TRIPLE_FAKE;
        }
        return PATTERN_DOUBLE_GAP;
    }

    // Insane: heavy pressure from start, still escape-safe due to validator.
    if (elapsedSeconds < 30.0f) {
        if (r < 0.22f) return PATTERN_SINGLE;
        if (r < 0.77f) return PATTERN_DOUBLE_GAP;
        if (r < 0.95f) return PATTERN_STAGGER;
        return PATTERN_CONVOY;
    }
    if (elapsedSeconds < 90.0f) {
        if (r < 0.16f) return PATTERN_SINGLE;
        if (r < 0.69f) return PATTERN_DOUBLE_GAP;
        if (r < 0.90f) return PATTERN_STAGGER;
        return PATTERN_CONVOY;
    }
    if (elapsedSeconds < 180.0f) {
        if (r < 0.10f) return PATTERN_SINGLE;
        if (r < 0.58f) return PATTERN_DOUBLE_GAP;
        if (r < 0.84f) return PATTERN_STAGGER;
        if (r < 0.97f) return PATTERN_CONVOY;
    } else {
        if (r < 0.08f) return PATTERN_SINGLE;
        if (r < 0.50f) return PATTERN_DOUBLE_GAP;
        if (r < 0.79f) return PATTERN_STAGGER;
        if (r < 0.95f) return PATTERN_CONVOY;
    }

    // Max one triple-fake per 30 seconds.
    if (elapsedSeconds - lastTripleFakeTime_ >= 30.0f) {
        return PATTERN_TRIPLE_FAKE;
    }
    return PATTERN_DOUBLE_GAP;
}

void ObstacleManager::spawnPattern(int pattern, float elapsedSeconds, int playerLane, float playerZ) {
    const float spawnZ = cfg::obstacleSpawnZ;

    auto nearPlayerLaneMask = [&]() {
        return laneMaskWithinBand(playerZ - 1.0f, playerZ + 1.0f);
    };

    if (pattern == PATTERN_SINGLE) {
        pendingTripleFake_ = false;
        const int lane = chooseSafeLane(spawnZ, playerLane);
        const int slot = findInactiveSlot();
        if (slot >= 0) {
            activateVehicle(slot, lane, spawnZ, chooseVehicleType(), elapsedSeconds, PATTERN_SINGLE);
        }
        return;
    }

    if (pattern == PATTERN_DOUBLE_GAP) {
        if ((nearPlayerLaneMask() & 0b111) == 0b111) {
            std::cout << "Spawn blocked - pattern adjusted for fairness\n";
            return;
        }

        const int openLane = chooseSafeLane(spawnZ, playerLane);
        std::array<int, 2> blocked{};
        int idx = 0;
        for (int lane = 0; lane < 3; ++lane) {
            if (lane != openLane) blocked[idx++] = lane;
        }

        for (int lane : blocked) {
            const int vehicleType = chooseVehicleType();
            pendingSpawnIsWide_ = (vehicleType == VEHICLE_BUS || vehicleType == VEHICLE_TRUCK);
            pendingSpawnWidth_ = pendingSpawnIsWide_ ? 2.45f : 1.8f;
            pendingTripleFake_ = false;
            if (!isSafeToSpawn(lane, spawnZ)) {
                std::cout << "Spawn blocked - pattern adjusted for fairness\n";
                continue;
            }
            const int slot = findInactiveSlot();
            if (slot < 0) break;
            activateVehicle(slot, lane, spawnZ, vehicleType, elapsedSeconds, PATTERN_DOUBLE_GAP);
        }
        return;
    }

    if (pattern == PATTERN_TRIPLE_FAKE) {
        const float escapeWindow = 2.5f / std::max(0.001f, std::min(cfg::obstacleBaseSpeed * tierSpeedMultiplier(elapsedSeconds), 3.0f));
        pendingTripleFake_ = true;
        pendingEscapeWindowSec_ = escapeWindow;

        std::cout << "TRIPLE INCOMING - find the gap!\n";

        for (int lane = 0; lane < 3; ++lane) {
            const int slot = findInactiveSlot();
            if (slot < 0) break;

            const int type = chooseVehicleType();
            pendingSpawnIsWide_ = (type == VEHICLE_BUS || type == VEHICLE_TRUCK);
            pendingSpawnWidth_ = pendingSpawnIsWide_ ? 2.45f : 1.8f;

            // Center lane gets +2.5Z offset to create the narrow fake-gap timing.
            const float zOffset = (lane == 1) ? 2.5f : 0.0f;
            if (!isSafeToSpawn(lane, spawnZ + zOffset)) {
                std::cout << "Spawn blocked - pattern adjusted for fairness\n";
                continue;
            }
            activateVehicle(slot, lane, spawnZ, type, elapsedSeconds, PATTERN_TRIPLE_FAKE, zOffset);
        }
        lastTripleFakeTime_ = elapsedSeconds;
        pendingTripleFake_ = false;
        return;
    }

    if (pattern == PATTERN_STAGGER) {
        if ((nearPlayerLaneMask() & 0b111) == 0b111) {
            std::cout << "Spawn blocked - pattern adjusted for fairness\n";
            return;
        }

        std::array<int, 3> order{{0, 1, 2}};
        std::shuffle(order.begin(), order.end(), rng_);

        for (int i = 0; i < 3; ++i) {
            const int lane = order[i];
            const float zOffset = i * -3.4f;
            const int type = chooseVehicleType();
            pendingSpawnIsWide_ = (type == VEHICLE_BUS || type == VEHICLE_TRUCK);
            pendingSpawnWidth_ = pendingSpawnIsWide_ ? 2.45f : 1.8f;
            pendingTripleFake_ = false;

            if (!isSafeToSpawn(lane, spawnZ + zOffset)) {
                std::cout << "Spawn blocked - pattern adjusted for fairness\n";
                continue;
            }
            const int slot = findInactiveSlot();
            if (slot < 0) break;
            activateVehicle(slot, lane, spawnZ, type, elapsedSeconds, PATTERN_STAGGER, zOffset);
        }
        return;
    }

    // PATTERN_CONVOY: 2-3 same-lane vehicles in sequence.
    const int lane = chooseSafeLane(spawnZ, playerLane);
    std::uniform_int_distribution<int> countDist(2, 3);
    const int count = countDist(rng_);
    const int type = chooseVehicleType();
    for (int i = 0; i < count; ++i) {
        const int slot = findInactiveSlot();
        if (slot < 0) break;
        const float zOffset = -i * 5.5f;
        pendingSpawnIsWide_ = (type == VEHICLE_BUS || type == VEHICLE_TRUCK);
        pendingSpawnWidth_ = pendingSpawnIsWide_ ? 2.45f : 1.8f;
        pendingTripleFake_ = false;
        if (!isSafeToSpawn(lane, spawnZ + zOffset)) {
            std::cout << "Spawn blocked - pattern adjusted for fairness\n";
            continue;
        }
        activateVehicle(slot, lane, spawnZ, type, elapsedSeconds, PATTERN_CONVOY, zOffset);
    }
}

void ObstacleManager::update(float dt, float elapsedSeconds, int playerLane, float playerZ) {
    elapsedTime_ = elapsedSeconds;

    for (auto& o : pool_) {
        if (!o.active) {
            continue;
        }
        o.z += o.speed * dt;

        if (o.z > playerZ + 2.0f) {
            o.active = false;
            o.closeCallChecked = true;
            activeCount_ = std::max(0, activeCount_ - 1);
            // Recycling obstacle nudges spawner to re-evaluate quickly.
            spawnTimer_ = std::max(spawnTimer_, nextSpawnIn_);
        }
    }

    spawnTimer_ += dt;

    const int maxActive = maxActiveForTier(elapsedSeconds);
    if (spawnTimer_ >= nextSpawnIn_ && activeCount_ < maxActive) {
        spawnTimer_ = 0.0f;
        const int pattern = choosePattern(elapsedSeconds);
        spawnPattern(pattern, elapsedSeconds, playerLane, playerZ);
        nextSpawnIn_ = nextSpawnInterval(elapsedSeconds);
    }

    // Hard fairness rule: never allow all three lanes blocked near player.
    enforceEscapeLaneNearPlayer(playerZ);
}

bool ObstacleManager::collidesWith(const AABB& playerBounds) const {
    for (const auto& o : pool_) {
        if (!o.active) {
            continue;
        }

        const float hw = o.width * 0.5f;
        const float hd = o.depth * 0.5f;
        AABB b;
        b.min = {o.x - hw, 0.0f, o.z - hd};
        b.max = {o.x + hw, o.height, o.z + hd};
        if (overlaps(playerBounds, b)) {
            return true;
        }
    }
    return false;
}

void ObstacleManager::checkCloseCalls(const AABB& playerBounds) {
    for (auto& o : pool_) {
        if (!o.active || o.closeCallChecked) {
            continue;
        }

        const float hw = o.width * 0.5f;
        const float hd = o.depth * 0.5f;
        const float minX = o.x - hw;
        const float maxX = o.x + hw;
        const float minZ = o.z - hd;
        const float maxZ = o.z + hd;

        // Trigger close-call only once obstacle has reached/passed player zone.
        if (maxZ < playerBounds.max.z) {
            continue;
        }

        const float xGap = std::max(0.0f, std::max(playerBounds.min.x - maxX, minX - playerBounds.max.x));
        const float zGap = std::max(0.0f, std::max(playerBounds.min.z - maxZ, minZ - playerBounds.max.z));

        if (xGap <= 0.3f && zGap <= 0.3f) {
            const bool isOverlapX = !(playerBounds.max.x < minX || playerBounds.min.x > maxX);
            const bool isOverlapZ = !(playerBounds.max.z < minZ || playerBounds.min.z > maxZ);
            if (!(isOverlapX && isOverlapZ)) {
                std::cout << "CLOSE!\n";
            }
            o.closeCallChecked = true;
        }
    }
}
