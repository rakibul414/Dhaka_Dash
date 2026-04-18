#include "Game.h"

#include <fstream>
#include <iostream>

#include "Config.h"

namespace {
constexpr const char* kHighScoreDbPath = "dhaka_dash_high_score.db";
}

int Game::loadHighScoreFromLocalDb() const {
    std::ifstream db(kHighScoreDbPath);
    int value = 0;
    if (!(db >> value)) {
        return 0;
    }
    return (value < 0) ? 0 : value;
}

void Game::saveHighScoreToLocalDb(int value) const {
    std::ofstream db(kHighScoreDbPath, std::ios::trunc);
    if (!db.is_open()) {
        return;
    }
    db << value << '\n';
}

void Game::updateHighScoreIfNeeded() {
    const int finalScore = static_cast<int>(score_);
    if (finalScore > highScore_) {
        highScore_ = finalScore;
        saveHighScoreToLocalDb(highScore_);
        std::cout << "New High Score: " << highScore_ << "\n";
    }
}

void Game::setDifficultyPreset(int preset) {
    if (preset < cfg::PRESET_CASUAL || preset > cfg::PRESET_INSANE) {
        obstacleManager_.setDifficultyPreset(cfg::defaultDifficultyPreset);
        return;
    }
    obstacleManager_.setDifficultyPreset(static_cast<cfg::DifficultyPreset>(preset));
}

void Game::reset() {
    if (!highScoreLoaded_) {
        highScore_ = loadHighScoreFromLocalDb();
        highScoreLoaded_ = true;
    }

    player_.reset();
    obstacleManager_.reset();
    gameOver_ = false;
    announcedGameOver_ = false;
    score_ = 0.0f;
    elapsedSeconds_ = 0.0f;
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
    return obstacleManager_.collidesWith(player_.bounds());
}

void Game::update(float dt) {
    if (gameOver_) {
        return;
    }

    player_.update(dt);
    elapsedSeconds_ += dt;
    obstacleManager_.update(dt, elapsedSeconds_, player_.lane(), cfg::playerZ);
    obstacleManager_.checkCloseCalls(player_.bounds());
    score_ += dt * 10.0f;

    if (collidesWithAnyObstacle()) {
        gameOver_ = true;
        updateHighScoreIfNeeded();
        if (!announcedGameOver_) {
            std::cout << "Game Over! Final Score: " << static_cast<int>(score_) << "\n";
            announcedGameOver_ = true;
        }
    }
}
