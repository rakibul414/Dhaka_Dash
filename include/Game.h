#pragma once

#include "ObstacleManager.h"
#include "Player.h"

class Game {
public:
    void reset();
    void update(float dt);

    void moveLeft();
    void moveRight();

    const Player& player() const { return player_; }
    const ObstacleManager& obstacleManager() const { return obstacleManager_; }

    bool isGameOver() const { return gameOver_; }
    float score() const { return score_; }

private:
    bool collidesWithAnyObstacle() const;

    Player player_;
    ObstacleManager obstacleManager_;
    bool gameOver_ = false;
    bool announcedGameOver_ = false;
    float score_ = 0.0f;
};
