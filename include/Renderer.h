#pragma once

#include "ObstacleManager.h"

class Game;
class Player;

namespace renderer {
void setup3D(int width, int height);
void drawWorld(const Player& player, const ObstacleManager& obstacleManager, float score, bool gameOver);
void drawText2D(float x, float y, const char* text);
}  // namespace renderer
