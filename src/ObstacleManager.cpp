#include "ObstacleManager.h"

#include <algorithm>
#include <array>
#include <random>

#include "Config.h"

ObstacleManager::ObstacleManager() : rng_(std::random_device{}()) {
    reset();
}

void ObstacleManager::reset() {
    obstacles_.clear();
    spawnTimer_ = 0.0f;
    nextSpawnIn_ = 0.9f;
}

void ObstacleManager::spawn(float scoreSeconds) {
    std::uniform_int_distribution<int> laneDist(0, 2);
    std::uniform_int_distribution<int> typeDist(0, 1);
    std::uniform_real_distribution<float> colorDist(0.25f, 0.95f);

    Obstacle o;
    o.type = typeDist(rng_);
    o.x = cfg::laneX[laneDist(rng_)];
    o.z = cfg::obstacleSpawnZ;

    if (o.type == 0) {
        o.width = 1.7f;
        o.height = 1.4f;
        o.depth = 2.7f;
    } else {
        o.width = 2.2f;
        o.height = 2.3f;
        o.depth = 4.2f;
    }

    const float difficultyBoost = std::min(scoreSeconds * 0.06f, 14.0f);
    o.speed = cfg::obstacleBaseSpeed + difficultyBoost;

    o.color = {colorDist(rng_), colorDist(rng_), colorDist(rng_)};
    obstacles_.push_back(o);
}

void ObstacleManager::update(float dt, float scoreSeconds) {
    spawnTimer_ += dt;
    if (spawnTimer_ >= nextSpawnIn_) {
        spawnTimer_ = 0.0f;
        spawn(scoreSeconds);

        const float spawnTightness = std::min(scoreSeconds * 0.01f, 0.45f);
        std::uniform_real_distribution<float> intervalDist(
            cfg::minSpawnInterval,
            cfg::maxSpawnInterval - spawnTightness);
        nextSpawnIn_ = intervalDist(rng_);
    }

    for (auto& o : obstacles_) {
        o.z += o.speed * dt;
    }

    obstacles_.erase(
        std::remove_if(obstacles_.begin(), obstacles_.end(), [](const Obstacle& o) {
            return o.z > cfg::obstacleRemoveZ;
        }),
        obstacles_.end());
}
