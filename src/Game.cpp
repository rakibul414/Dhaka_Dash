#include "Game.h"

#include <iostream>

#include "Config.h"

void Game::reset() {
    player_.reset();
    obstacleManager_.reset();
    gameOver_ = false;
    announcedGameOver_ = false;
    score_ = 0.0f;
}

void Game::moveLeft() {
    if (!gameOver_) {
        player_.moveLeft();
    }
}

void Game::moveRight() {
    if (!gameOver_) {
        player_.moveRight();
    }
}

bool Game::collidesWithAnyObstacle() const {
    const AABB p = player_.bounds();

    for (const auto& o : obstacleManager_.obstacles()) {
        AABB b;
        b.min = {o.x - o.width * 0.5f, 0.0f, o.z - o.depth * 0.5f};
        b.max = {o.x + o.width * 0.5f, o.height, o.z + o.depth * 0.5f};

        if (overlaps(p, b)) {
            return true;
        }
    }
    return false;
}

void Game::update(float dt) {
    if (gameOver_) {
        return;
    }

    player_.update(dt);
    obstacleManager_.update(dt, score_);
    score_ += dt * 10.0f;

    if (collidesWithAnyObstacle()) {
        gameOver_ = true;
        if (!announcedGameOver_) {
            std::cout << "Game Over! Final Score: " << static_cast<int>(score_) << "\n";
            announcedGameOver_ = true;
        }
    }
}
