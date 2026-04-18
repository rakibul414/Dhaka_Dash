#include "Game.h"

#include <fstream>
#include <iostream>

#include "Config.h"

namespace {
const char* getHighScoreDbPath(int difficulty) {
    static const char* paths[3] = {
        "dhaka_dash_high_score_casual.db",
        "dhaka_dash_high_score_competitive.db",
        "dhaka_dash_high_score_insane.db"
    };
    if (difficulty < 0 || difficulty > 2) difficulty = 0;
    return paths[difficulty];
}
}

int Game::loadHighScoreFromLocalDb(int difficulty) const {
    const char* dbPath = getHighScoreDbPath(difficulty);
    std::ifstream db(dbPath);
    int value = 0;
    if (!(db >> value)) {
        return 0;
    }
    return (value < 0) ? 0 : value;
}

void Game::saveHighScoreToLocalDb(int difficulty, int value) const {
    const char* dbPath = getHighScoreDbPath(difficulty);
    std::ofstream db(dbPath, std::ios::trunc);
    if (!db.is_open()) {
        return;
    }
    db << value << '\n';
}

void Game::updateHighScoreIfNeeded() {
    const int finalScore = static_cast<int>(score_);
    if (finalScore > highScore_) {
        highScore_ = finalScore;
        highScores_[currentDifficulty_] = highScore_;
        saveHighScoreToLocalDb(currentDifficulty_, highScore_);
        const char* modeNames[] = {"CASUAL", "COMPETITIVE", "INSANE"};
        std::cout << "New " << modeNames[currentDifficulty_] << " High Score: " << highScore_ << "\n";
    }
}

void Game::setDifficultyPreset(int preset) {
    if (preset < cfg::PRESET_CASUAL || preset > cfg::PRESET_INSANE) {
        preset = cfg::defaultDifficultyPreset;
    }
    currentDifficulty_ = preset;
    obstacleManager_.setDifficultyPreset(static_cast<cfg::DifficultyPreset>(preset));
}

void Game::reset() {
    if (!highScoresLoaded_) {
        for (int i = 0; i < 3; ++i) {
            highScores_[i] = loadHighScoreFromLocalDb(i);
        }
        highScoresLoaded_ = true;
    }
    highScore_ = highScores_[currentDifficulty_];

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
