#include "Renderer.h"

#include <cmath>
#include <cstdio>

#include "GlutCompat.h"

#include "Config.h"
#include "Player.h"

namespace {
void drawCube(float w, float h, float d) {
    glPushMatrix();
    glScalef(w, h, d);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawWheel(float radius, float width) {
    GLUquadric* quad = gluNewQuadric();

    glPushMatrix();
    // Rotate so the cylinder axis aligns with the wheel axle (X-axis).
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    gluCylinder(quad, radius, radius, width, 20, 1);

    glPushMatrix();
    gluDisk(quad, 0.0f, radius, 20, 1);
    glTranslatef(0.0f, 0.0f, width);
    gluDisk(quad, 0.0f, radius, 20, 1);
    glPopMatrix();

    glPopMatrix();
    gluDeleteQuadric(quad);
}

void drawRoad() {
    // Base road plane stretched along Z to simulate depth.
    glColor3f(0.14f, 0.14f, 0.14f);
    glPushMatrix();
    glTranslatef(0.0f, -0.02f, -60.0f);
    drawCube(cfg::roadWidth, 0.04f, 160.0f);
    glPopMatrix();

    glColor3f(0.86f, 0.86f, 0.3f);
    for (int i = 0; i < 2; ++i) {
        const float x = (cfg::laneX[i] + cfg::laneX[i + 1]) * 0.5f;
        for (int k = 0; k < 26; ++k) {
            const float z = -5.0f - k * 6.0f;
            glPushMatrix();
            // Translate each dashed segment to lane divider positions.
            glTranslatef(x, 0.01f, z);
            drawCube(cfg::laneMarkerWidth, 0.02f, 2.5f);
            glPopMatrix();
        }
    }
}

void drawSidewalks() {
    glColor3f(0.26f, 0.26f, 0.28f);

    glPushMatrix();
    glTranslatef(-7.0f, 0.03f, -60.0f);
    drawCube(2.4f, 0.06f, 160.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(7.0f, 0.03f, -60.0f);
    drawCube(2.4f, 0.06f, 160.0f);
    glPopMatrix();
}

void drawTree() {
    glColor3f(0.35f, 0.2f, 0.08f);
    glPushMatrix();
    // Thin trunk raised from the pavement plane.
    glTranslatef(0.0f, 0.8f, 0.0f);
    drawCube(0.35f, 1.6f, 0.35f);
    glPopMatrix();

    glColor3f(0.08f, 0.55f, 0.18f);
    glPushMatrix();
    // Foliage clusters stacked upward for a fuller tree silhouette.
    glTranslatef(0.0f, 2.05f, 0.0f);
    drawCube(1.3f, 0.85f, 1.3f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 2.7f, 0.0f);
    drawCube(1.0f, 0.65f, 1.0f);
    glPopMatrix();
}

void drawBuilding(int variant) {
    const float heights[3] = {4.2f, 5.3f, 6.5f};
    const float widths[3] = {2.6f, 3.0f, 2.3f};
    const float depths[3] = {2.1f, 2.6f, 2.0f};

    const int id = variant % 3;
    const float h = heights[id];
    const float w = widths[id];
    const float d = depths[id];

    glColor3f(0.17f + 0.07f * id, 0.2f + 0.05f * id, 0.24f + 0.04f * id);
    glPushMatrix();
    // Main tower block anchored on ground, height controls skyline variation.
    glTranslatef(0.0f, h * 0.5f, 0.0f);
    drawCube(w, h, d);
    glPopMatrix();

    glColor3f(0.85f, 0.85f, 0.65f);
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 2; ++col) {
            glPushMatrix();
            // Window cubes translated over facade for depth cues at speed.
            glTranslatef(-0.5f + col * 1.0f, 1.0f + row * 1.25f, d * 0.51f);
            drawCube(0.28f, 0.38f, 0.03f);
            glPopMatrix();
        }
    }
}

void drawLampPost() {
    glColor3f(0.46f, 0.46f, 0.5f);
    glPushMatrix();
    glTranslatef(0.0f, 1.5f, 0.0f);
    drawCube(0.18f, 3.0f, 0.18f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.36f, 2.9f, 0.0f);
    drawCube(0.72f, 0.12f, 0.12f);
    glPopMatrix();

    glColor3f(1.0f, 0.92f, 0.48f);
    glPushMatrix();
    glTranslatef(0.66f, 2.75f, 0.0f);
    drawCube(0.2f, 0.2f, 0.2f);
    glPopMatrix();
}

void drawRoadsideScenery(float score) {
    const float spacing = 9.0f;
    const float scroll = std::fmod(score * 0.7f, spacing);

    for (int i = 0; i < 24; ++i) {
        const float z = -130.0f + i * spacing + scroll;
        const int pattern = i % 6;

        // Left side sequence.
        glPushMatrix();
        glTranslatef(-9.7f, 0.0f, z);
        if (pattern == 0 || pattern == 3) {
            drawBuilding(i);
        } else if (pattern == 1 || pattern == 4) {
            drawTree();
        } else {
            drawLampPost();
        }
        glPopMatrix();

        // Right side sequence with a phase offset to avoid mirrored repetition.
        glPushMatrix();
        glTranslatef(9.7f, 0.0f, z - 4.5f);
        if (pattern == 2 || pattern == 5) {
            drawBuilding(i + 1);
        } else if (pattern == 0 || pattern == 4) {
            drawTree();
        } else {
            drawLampPost();
        }
        glPopMatrix();
    }
}

void drawRickshaw(const Player& player) {
    const float x = player.x();

    glPushMatrix();
    // Move the whole rickshaw model to current lane X and fixed gameplay Z.
    glTranslatef(x, cfg::playerY, cfg::playerZ);

    glColor3f(0.12f, 0.75f, 0.45f);
    glPushMatrix();
    // Body base scaled from unit cube into a cart-like chassis.
    glTranslatef(0.0f, 0.55f, 0.0f);
    drawCube(1.6f, 0.7f, 2.1f);
    glPopMatrix();

    glColor3f(0.9f, 0.2f, 0.2f);
    glPushMatrix();
    // Canopy frame elevated and scaled to sit above the body.
    glTranslatef(0.0f, 1.25f, -0.1f);
    drawCube(1.8f, 0.45f, 1.4f);
    glPopMatrix();

    glColor3f(0.18f, 0.18f, 0.18f);
    glPushMatrix();
    glTranslatef(-0.72f, 0.25f, 0.8f);
    drawWheel(0.28f, 0.15f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.57f, 0.25f, 0.8f);
    drawWheel(0.28f, 0.15f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.08f, 0.23f, -0.95f);
    drawWheel(0.23f, 0.12f);
    glPopMatrix();

    glPopMatrix();
}

void drawObstacle(const Obstacle& o) {
    glPushMatrix();
    // Place each obstacle in one of the lane centers and animated Z position.
    glTranslatef(o.x, o.height * 0.5f, o.z);

    glColor3f(o.color.r, o.color.g, o.color.b);
    drawCube(o.width, o.height, o.depth);

    if (o.type == 1) {
        glColor3f(0.1f, 0.1f, 0.15f);
        glPushMatrix();
        // Slightly offset a thin scaled cube to fake a windshield panel.
        glTranslatef(0.0f, 0.2f, o.depth * 0.42f);
        drawCube(o.width * 0.72f, o.height * 0.32f, 0.08f);
        glPopMatrix();
    }

    glPopMatrix();
}

void begin2D(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(width), 0.0, static_cast<double>(height));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
}

void end2D() {
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    // Ensure 3D rendering in the next frame starts from model-view space.
    glMatrixMode(GL_MODELVIEW);
}
}  // namespace

namespace renderer {
void setup3D(int width, int height) {
    if (height == 0) {
        height = 1;
    }

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, static_cast<double>(width) / static_cast<double>(height), 0.1, 300.0);

    glMatrixMode(GL_MODELVIEW);
}

void drawText2D(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
        ++text;
    }
}

void drawWorld(const Player& player, const ObstacleManager& obstacleManager, float score, bool gameOver) {
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 6.3, 16.0,
              0.0, 1.2, -20.0,
              0.0, 1.0, 0.0);

    drawRoad();
    drawSidewalks();
    drawRoadsideScenery(score);
    drawRickshaw(player);

    for (const auto& o : obstacleManager.obstacles()) {
        drawObstacle(o);
    }

    begin2D(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

    char scoreText[64];
    std::snprintf(scoreText, sizeof(scoreText), "Score: %d", static_cast<int>(score));
    glColor3f(0.95f, 0.95f, 0.95f);
    drawText2D(20.0f, glutGet(GLUT_WINDOW_HEIGHT) - 35.0f, scoreText);

    if (gameOver) {
        glColor3f(1.0f, 0.3f, 0.3f);
        drawText2D(glutGet(GLUT_WINDOW_WIDTH) * 0.5f - 120.0f,
                   glutGet(GLUT_WINDOW_HEIGHT) * 0.5f,
                   "GAME OVER - Press R to Restart");
    }

    end2D();

    glutSwapBuffers();
}
}  // namespace renderer
