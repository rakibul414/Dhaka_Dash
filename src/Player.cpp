#include "Player.h"

#include "Config.h"

void Player::reset() {
    laneIndex_ = 1;
    xPos_ = cfg::laneX[laneIndex_];
    targetX_ = xPos_;
}

void Player::moveLeft() {
    if (laneIndex_ > 0) {
        --laneIndex_;
        targetX_ = cfg::laneX[laneIndex_];
    }
}

void Player::moveRight() {
    if (laneIndex_ < 2) {
        ++laneIndex_;
        targetX_ = cfg::laneX[laneIndex_];
    }
}

void Player::update(float dt) {
    const float diff = targetX_ - xPos_;
    const float step = cfg::laneSwitchSpeed * dt;
    if (diff > step) {
        xPos_ += step;
    } else if (diff < -step) {
        xPos_ -= step;
    } else {
        xPos_ = targetX_;
    }
}

AABB Player::bounds() const {
    const float mercyWidth = cfg::playerWidth * 0.8f;
    const float mercyDepth = cfg::playerDepth * 0.82f;

    AABB box;
    box.min = {xPos_ - mercyWidth * 0.5f, 0.0f, cfg::playerZ - mercyDepth * 0.5f};
    box.max = {xPos_ + mercyWidth * 0.5f, cfg::playerHeight, cfg::playerZ + mercyDepth * 0.5f};
    return box;
}
