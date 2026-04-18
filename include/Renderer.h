#pragma once

#include "ObstacleManager.h"

class Game;
class Player;

namespace renderer {
void setup3D(int width, int height);
void drawWorld(const Player& player, const ObstacleManager& obstacleManager, float score, int highScore, bool gameOver, bool paused);
void drawText2D(float x, float y, const char* text);
void drawOpeningScreen(int selectedMode, int highScore);
void drawRickshawShowcase();
}  // namespace renderer
