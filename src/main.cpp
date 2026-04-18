#include <cstdlib>
#include <cstdio>

#include "GlutCompat.h"

#include "Config.h"
#include "Game.h"
#include "Renderer.h"

namespace {
Game game;
int lastTimeMs = 0;
int selectedMode = static_cast<int>(cfg::defaultDifficultyPreset);

enum AppState {
    APP_MENU = 0,
    APP_PLAYING,
    APP_SHOWCASE
};

AppState appState = APP_MENU;
bool isPaused = false;

void goBackToMenu() {
    appState = APP_MENU;
    isPaused = false;
}

void startRickshawShowcase() {
    appState = APP_SHOWCASE;
    isPaused = false;
}

void startGameFromMenu() {
    game.setDifficultyPreset(selectedMode);
    game.reset();
    lastTimeMs = glutGet(GLUT_ELAPSED_TIME);
    isPaused = false;
    appState = APP_PLAYING;
}

void onDisplay() {
    if (appState == APP_MENU) {
        renderer::drawOpeningScreen(selectedMode, game.highScore());
        return;
    }

    if (appState == APP_SHOWCASE) {
        renderer::drawRickshawShowcase();
        return;
    }

    renderer::drawWorld(game.player(), game.obstacleManager(), game.score(), game.highScore(), game.isGameOver(), isPaused);
}

void onIdle() {
    if (appState == APP_MENU) {
        glutPostRedisplay();
        return;
    }

    if (appState == APP_SHOWCASE) {
        glutPostRedisplay();
        return;
    }

    const int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt = static_cast<float>(nowMs - lastTimeMs) / 1000.0f;
    lastTimeMs = nowMs;

    if (dt < 0.0f) {
        dt = 0.0f;
    }
    if (dt > 0.05f) {
        dt = 0.05f;
    }

    if (!isPaused) {
        game.update(dt);
    }
    glutPostRedisplay();
}

void onReshape(int w, int h) {
    renderer::setup3D(w, h);
}

void onKeyboard(unsigned char key, int, int) {
    if (appState == APP_MENU) {
        if (key == '1') {
            selectedMode = cfg::PRESET_CASUAL;
        } else if (key == '2') {
            selectedMode = cfg::PRESET_COMPETITIVE;
        } else if (key == '3') {
            selectedMode = cfg::PRESET_INSANE;
        } else if (key == '4' || key == 's' || key == 'S') {
            startRickshawShowcase();
        } else if (key == 13) {
            startGameFromMenu();
        } else if (key == 27) {
            std::exit(0);
        }
        return;
    }

    if (appState == APP_SHOWCASE) {
        if (key == 'm' || key == 'M' || key == 'b' || key == 'B' || key == 8 || key == 27) {
            goBackToMenu();
        }
        return;
    }

    if (key == 'a' || key == 'A') {
        game.moveLeft();
    } else if (key == 'd' || key == 'D') {
        game.moveRight();
    } else if (key == ' ') {
        if (!game.isGameOver()) {
            isPaused = !isPaused;
        }
    } else if (key == 'r' || key == 'R') {
        game.reset();
        isPaused = false;
    } else if (key == 'm' || key == 'M' || key == 'b' || key == 'B' || key == 8) {
        goBackToMenu();
    } else if (key == 27) {
        std::exit(0);
    }
}

void onSpecial(int key, int, int) {
    if (appState == APP_MENU) {
        if (key == GLUT_KEY_UP || key == GLUT_KEY_LEFT) {
            selectedMode = (selectedMode + 2) % 3;
        } else if (key == GLUT_KEY_DOWN || key == GLUT_KEY_RIGHT) {
            selectedMode = (selectedMode + 1) % 3;
        }
        return;
    }

    if (appState == APP_SHOWCASE) {
        if (key == GLUT_KEY_LEFT || key == GLUT_KEY_RIGHT || key == GLUT_KEY_UP || key == GLUT_KEY_DOWN) {
            goBackToMenu();
        }
        return;
    }

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

    game.setDifficultyPreset(selectedMode);
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
