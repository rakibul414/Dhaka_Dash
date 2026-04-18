#pragma once

#include <array>
#include <random>

#include "Config.h"
#include "Types.h"

enum ObstaclePattern {
    PATTERN_SINGLE = 0,
    PATTERN_DOUBLE_GAP,
    PATTERN_TRIPLE_FAKE,
    PATTERN_STAGGER,
    PATTERN_CONVOY
};

enum VehicleType {
    VEHICLE_BUS = 0,
    VEHICLE_CNG,
    VEHICLE_TRUCK,
    VEHICLE_RICKSHAW
};

struct Obstacle {
    bool active = false;
    bool closeCallChecked = false;
    int lane = 1;
    float x;
    float z;
    float width;
    float height;
    float depth;
    float speed;
    int type = 0;
    int pattern = PATTERN_SINGLE;
    int vehicleType = VEHICLE_CNG;
    bool isWide = false;
    Color color;
};

class ObstacleManager {
public:
    static constexpr int kMaxObstacles = 10;

    ObstacleManager();

    void setDifficultyPreset(cfg::DifficultyPreset preset) { difficultyPreset_ = preset; }
    void reset();
    void update(float dt, float elapsedSeconds, int playerLane, float playerZ);
    bool collidesWith(const AABB& playerBounds) const;
    void checkCloseCalls(const AABB& playerBounds);

    const std::array<Obstacle, kMaxObstacles>& obstacles() const { return pool_; }
    int activeCount() const { return activeCount_; }

private:
    bool isSafeToSpawn(int lane, float spawnZ);
    int chooseSafeLane(float spawnZ, int playerLane);
    int choosePattern(float elapsedSeconds);
    void spawnPattern(int pattern, float elapsedSeconds, int playerLane, float playerZ);

    int findInactiveSlot();
    void activateVehicle(int slot, int lane, float spawnZ, int vehicleType, float elapsedSeconds, int pattern, float zOffset = 0.0f);
    int laneMaskWithinBand(float minZ, float maxZ) const;
    int laneMaskForBox(float minX, float maxX) const;
    int laneFromX(float x) const;
    void enforceEscapeLaneNearPlayer(float playerZ);
    float nextSpawnInterval(float elapsedSeconds);
    float tierSpeedMultiplier(float elapsedSeconds) const;
    int maxActiveForTier(float elapsedSeconds) const;
    int chooseVehicleType();

    bool pendingSpawnIsWide_ = false;
    bool pendingTripleFake_ = false;
    float pendingSpawnWidth_ = 1.8f;
    float pendingEscapeWindowSec_ = 0.0f;

    std::array<Obstacle, kMaxObstacles> pool_{};
    std::mt19937 rng_;
    int activeCount_ = 0;

    // Last three spawn lanes for variety control.
    std::array<int, 3> recentLanes_{{-1, -1, -1}};
    int recentLaneCursor_ = 0;
    int consecutiveCenterSpawns_ = 0;

    float elapsedTime_ = 0.0f;
    float spawnTimer_ = 0.0f;
    float nextSpawnIn_ = 1.0f;
    float lastTripleFakeTime_ = -1000.0f;
    cfg::DifficultyPreset difficultyPreset_ = cfg::defaultDifficultyPreset;
};
