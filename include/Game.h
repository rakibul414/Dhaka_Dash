#pragma once

#include "ObstacleManager.h"
#include "Player.h"

class Game {
public:
    void setDifficultyPreset(int preset);
    void reset();
    void update(float dt);

    void moveLeft();
    void moveRight();

    const Player& player() const { return player_; }
    const ObstacleManager& obstacleManager() const { return obstacleManager_; }

    bool isGameOver() const { return gameOver_; }
    float score() const { return score_; }
    int highScore() const { return highScore_; }

private:
    int loadHighScoreFromLocalDb() const;
    void saveHighScoreToLocalDb(int value) const;
    void updateHighScoreIfNeeded();
    bool collidesWithAnyObstacle() const;

    Player player_;
    ObstacleManager obstacleManager_;
    bool gameOver_ = false;
    bool announcedGameOver_ = false;
    float score_ = 0.0f;
    float elapsedSeconds_ = 0.0f;
    int highScore_ = 0;
    bool highScoreLoaded_ = false;
};
