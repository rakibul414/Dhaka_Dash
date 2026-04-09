#include <cstdlib>

#include "GlutCompat.h"

#include "Config.h"
#include "Game.h"
#include "Renderer.h"

namespace {
Game game;
int lastTimeMs = 0;

void onDisplay() {
    renderer::drawWorld(game.player(), game.obstacleManager(), game.score(), game.isGameOver());
}

void onIdle() {
    const int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt = static_cast<float>(nowMs - lastTimeMs) / 1000.0f;
    lastTimeMs = nowMs;

    if (dt < 0.0f) {
        dt = 0.0f;
    }
    if (dt > 0.05f) {
        dt = 0.05f;
    }

    game.update(dt);
    glutPostRedisplay();
}

void onReshape(int w, int h) {
    renderer::setup3D(w, h);
}

void onKeyboard(unsigned char key, int, int) {
    if (key == 'a' || key == 'A') {
        game.moveLeft();
    } else if (key == 'd' || key == 'D') {
        game.moveRight();
    } else if (key == 'r' || key == 'R') {
        game.reset();
    } else if (key == 27) {
        std::exit(0);
    }
}

void onSpecial(int key, int, int) {
    if (key == GLUT_KEY_LEFT) {
        game.moveLeft();
    } else if (key == GLUT_KEY_RIGHT) {
        game.moveRight();
    }
}
}  // namespace

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(cfg::windowWidth, cfg::windowHeight);
    glutCreateWindow("Dhaka Dash: Rickshaw Run");

    renderer::setup3D(cfg::windowWidth, cfg::windowHeight);

    glEnable(GL_DEPTH_TEST);

    game.reset();
    lastTimeMs = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(onDisplay);
    glutReshapeFunc(onReshape);
    glutIdleFunc(onIdle);
    glutKeyboardFunc(onKeyboard);
    glutSpecialFunc(onSpecial);

    glutMainLoop();
    return 0;
}
