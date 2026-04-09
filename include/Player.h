#pragma once

#include "Types.h"

class Player {
public:
    void reset();
    void update(float dt);

    void moveLeft();
    void moveRight();

    float x() const { return xPos_; }
    int lane() const { return laneIndex_; }
    AABB bounds() const;

private:
    int laneIndex_ = 1;
    float xPos_ = 0.0f;
    float targetX_ = 0.0f;
};
