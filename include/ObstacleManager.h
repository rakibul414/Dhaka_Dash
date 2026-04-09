#pragma once

#include <random>
#include <vector>

#include "Types.h"

struct Obstacle {
    float x;
    float z;
    float width;
    float height;
    float depth;
    float speed;
    int type;
    Color color;
};

class ObstacleManager {
public:
    ObstacleManager();

    void reset();
    void update(float dt, float scoreSeconds);
    const std::vector<Obstacle>& obstacles() const { return obstacles_; }

private:
    void spawn(float scoreSeconds);

    std::vector<Obstacle> obstacles_;
    std::mt19937 rng_;
    float spawnTimer_ = 0.0f;
    float nextSpawnIn_ = 1.0f;
};
