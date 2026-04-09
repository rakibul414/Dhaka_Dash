#include "include/GlutCompat.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

namespace cfg {
constexpr int windowWidth = 1280;
constexpr int windowHeight = 720;
constexpr float laneX[3] = {-3.0f, 0.0f, 3.0f};
constexpr float roadWidth = 11.0f;
constexpr float playerY = 0.85f;
constexpr float playerZ = 6.0f;
constexpr float playerWidth = 1.8f;
constexpr float playerHeight = 1.6f;
constexpr float playerDepth = 2.8f;
constexpr float obstacleSpawnZ = -130.0f;
constexpr float obstacleRemoveZ = 16.0f;
constexpr float obstacleBaseSpeed = 30.0f;
constexpr float laneSwitchSpeed = 10.0f;
}

struct AABB {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
};

struct Obstacle {
    float x, z;
    float w, h, d;
    float speed;
    float r, g, b;
    int type;
};

class GameSingle {
public:
    void reset() {
        laneIndex = 1;
        xPos = cfg::laneX[1];
        targetX = xPos;
        score = 0.0f;
        gameOver = false;
        announced = false;
        obstacles.clear();
        spawnTimer = 0.0f;
        nextSpawn = 0.9f;
    }

    void update(float dt) {
        if (gameOver) {
            return;
        }

        const float step = cfg::laneSwitchSpeed * dt;
        const float diff = targetX - xPos;
        if (diff > step) xPos += step;
        else if (diff < -step) xPos -= step;
        else xPos = targetX;

        spawnTimer += dt;
        if (spawnTimer >= nextSpawn) {
            spawnTimer = 0.0f;
            spawnObstacle();
            std::uniform_real_distribution<float> interval(0.55f, 1.2f);
            nextSpawn = interval(rng);
        }

        for (auto& o : obstacles) o.z += o.speed * dt;
        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(), [](const Obstacle& o){ return o.z > cfg::obstacleRemoveZ; }), obstacles.end());

        score += dt * 10.0f;

        if (checkCollision()) {
            gameOver = true;
            if (!announced) {
                std::cout << "Game Over! Final Score: " << static_cast<int>(score) << "\n";
                announced = true;
            }
        }
    }

    void left() { if (!gameOver && laneIndex > 0) targetX = cfg::laneX[--laneIndex]; }
    void right() { if (!gameOver && laneIndex < 2) targetX = cfg::laneX[++laneIndex]; }

    float playerX() const { return xPos; }
    float getScore() const { return score; }
    bool isGameOver() const { return gameOver; }
    const std::vector<Obstacle>& getObstacles() const { return obstacles; }

private:
    void spawnObstacle() {
        std::uniform_int_distribution<int> laneDist(0,2), typeDist(0,1);
        std::uniform_real_distribution<float> c(0.2f, 0.95f);
        Obstacle o;
        o.type = typeDist(rng);
        o.x = cfg::laneX[laneDist(rng)];
        o.z = cfg::obstacleSpawnZ;
        o.speed = cfg::obstacleBaseSpeed + std::min(score * 0.05f, 14.0f);
        if (o.type == 0) { o.w = 1.7f; o.h = 1.4f; o.d = 2.7f; }
        else { o.w = 2.2f; o.h = 2.3f; o.d = 4.2f; }
        o.r = c(rng); o.g = c(rng); o.b = c(rng);
        obstacles.push_back(o);
    }

    bool checkCollision() const {
        AABB p{ xPos - cfg::playerWidth * 0.5f, 0.0f, cfg::playerZ - cfg::playerDepth * 0.5f,
                xPos + cfg::playerWidth * 0.5f, cfg::playerHeight, cfg::playerZ + cfg::playerDepth * 0.5f };

        for (const auto& o : obstacles) {
            AABB b{ o.x - o.w * 0.5f, 0.0f, o.z - o.d * 0.5f,
                    o.x + o.w * 0.5f, o.h, o.z + o.d * 0.5f };
            const bool overlap = (p.minX <= b.maxX && p.maxX >= b.minX) &&
                                 (p.minY <= b.maxY && p.maxY >= b.minY) &&
                                 (p.minZ <= b.maxZ && p.maxZ >= b.minZ);
            if (overlap) return true;
        }
        return false;
    }

    int laneIndex = 1;
    float xPos = 0.0f;
    float targetX = 0.0f;
    float score = 0.0f;
    bool gameOver = false;
    bool announced = false;
    float spawnTimer = 0.0f;
    float nextSpawn = 1.0f;
    std::vector<Obstacle> obstacles;
    std::mt19937 rng{std::random_device{}()};
};

GameSingle game;
int lastTimeMs = 0;

void drawCube(float w, float h, float d) {
    glPushMatrix();
    glScalef(w, h, d);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawWheel(float radius, float width) {
    GLUquadric* q = gluNewQuadric();
    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    gluCylinder(q, radius, radius, width, 18, 1);
    gluDisk(q, 0.0f, radius, 18, 1);
    glTranslatef(0.0f, 0.0f, width);
    gluDisk(q, 0.0f, radius, 18, 1);
    glPopMatrix();
    gluDeleteQuadric(q);
}

void drawRoad() {
    glColor3f(0.14f, 0.14f, 0.14f);
    glPushMatrix();
    glTranslatef(0.0f, -0.02f, -60.0f);
    drawCube(cfg::roadWidth, 0.04f, 160.0f);
    glPopMatrix();

    glColor3f(0.86f, 0.86f, 0.3f);
    for (int i = 0; i < 2; ++i) {
        float x = (cfg::laneX[i] + cfg::laneX[i + 1]) * 0.5f;
        for (int k = 0; k < 26; ++k) {
            glPushMatrix();
            glTranslatef(x, 0.01f, -5.0f - k * 6.0f);
            drawCube(0.08f, 0.02f, 2.5f);
            glPopMatrix();
        }
    }
}

void drawPlayer() {
    glPushMatrix();
    glTranslatef(game.playerX(), cfg::playerY, cfg::playerZ);

    glColor3f(0.12f, 0.75f, 0.45f);
    glPushMatrix(); glTranslatef(0.0f, 0.55f, 0.0f); drawCube(1.6f, 0.7f, 2.1f); glPopMatrix();

    glColor3f(0.9f, 0.2f, 0.2f);
    glPushMatrix(); glTranslatef(0.0f, 1.25f, -0.1f); drawCube(1.8f, 0.45f, 1.4f); glPopMatrix();

    glColor3f(0.18f, 0.18f, 0.18f);
    glPushMatrix(); glTranslatef(-0.72f, 0.25f, 0.8f); drawWheel(0.28f, 0.15f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.57f, 0.25f, 0.8f); drawWheel(0.28f, 0.15f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.08f, 0.23f, -0.95f); drawWheel(0.23f, 0.12f); glPopMatrix();

    glPopMatrix();
}

void drawObstacles() {
    for (const auto& o : game.getObstacles()) {
        glPushMatrix();
        glTranslatef(o.x, o.h * 0.5f, o.z);
        glColor3f(o.r, o.g, o.b);
        drawCube(o.w, o.h, o.d);
        if (o.type == 1) {
            glColor3f(0.1f, 0.1f, 0.15f);
            glPushMatrix();
            glTranslatef(0.0f, 0.2f, o.d * 0.42f);
            drawCube(o.w * 0.72f, o.h * 0.32f, 0.08f);
            glPopMatrix();
        }
        glPopMatrix();
    }
}

void drawText(float x, float y, const char* t) {
    glRasterPos2f(x, y);
    while (*t) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *t++);
}

void display() {
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 6.3, 16.0, 0.0, 1.2, -20.0, 0.0, 1.0, 0.0);

    drawRoad();
    drawPlayer();
    drawObstacles();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, glutGet(GLUT_WINDOW_WIDTH), 0.0, glutGet(GLUT_WINDOW_HEIGHT));
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Score: %d", static_cast<int>(game.getScore()));
    glColor3f(0.95f, 0.95f, 0.95f);
    drawText(20.0f, glutGet(GLUT_WINDOW_HEIGHT) - 35.0f, buf);

    if (game.isGameOver()) {
        glColor3f(1.0f, 0.3f, 0.3f);
        drawText(glutGet(GLUT_WINDOW_WIDTH) * 0.5f - 120.0f, glutGet(GLUT_WINDOW_HEIGHT) * 0.5f, "GAME OVER - Press R to Restart");
    }

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, static_cast<double>(w) / h, 0.1, 300.0);
    glMatrixMode(GL_MODELVIEW);
}

void idle() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = static_cast<float>(now - lastTimeMs) / 1000.0f;
    lastTimeMs = now;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.05f) dt = 0.05f;
    game.update(dt);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int) {
    if (key == 'a' || key == 'A') game.left();
    else if (key == 'd' || key == 'D') game.right();
    else if (key == 'r' || key == 'R') game.reset();
    else if (key == 27) std::exit(0);
}

void special(int key, int, int) {
    if (key == GLUT_KEY_LEFT) game.left();
    else if (key == GLUT_KEY_RIGHT) game.right();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(cfg::windowWidth, cfg::windowHeight);
    glutCreateWindow("Dhaka Dash: Rickshaw Run (Single File)");

    glEnable(GL_DEPTH_TEST);

    game.reset();
    lastTimeMs = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);

    glutMainLoop();
    return 0;
}
