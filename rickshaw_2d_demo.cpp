#include <GL/glut.h>

#include <cmath>

namespace {
void drawCircleWithSpokes(float cx, float cy, float radius, int segments = 48) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        const float angle = 2.0f * 3.1415926f * static_cast<float>(i) / static_cast<float>(segments);
        glVertex2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);
    }
    glEnd();

    glBegin(GL_LINES);
    for (int i = 0; i < 10; ++i) {
        const float angle = 2.0f * 3.1415926f * static_cast<float>(i) / 10.0f;
        glVertex2f(cx, cy);
        glVertex2f(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);
    }
    glEnd();
}

void drawWheel(float cx, float cy, float radius) {
    glColor3f(0.05f, 0.05f, 0.05f);
    drawCircleWithSpokes(cx, cy, radius);

    glColor3f(0.75f, 0.75f, 0.78f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 32; ++i) {
        const float angle = 2.0f * 3.1415926f * static_cast<float>(i) / 32.0f;
        glVertex2f(cx + std::cos(angle) * (radius * 0.78f), cy + std::sin(angle) * (radius * 0.78f));
    }
    glEnd();
}

void drawRickshaw() {
    // Chassis and frame.
    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(-0.55f, -0.05f);
    glVertex2f(0.60f, -0.05f);
    glVertex2f(0.60f, 0.00f);
    glVertex2f(-0.55f, 0.00f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(-0.42f, 0.00f);
    glVertex2f(-0.36f, 0.00f);
    glVertex2f(-0.28f, 0.48f);
    glVertex2f(-0.34f, 0.48f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(0.42f, 0.00f);
    glVertex2f(0.48f, 0.00f);
    glVertex2f(0.38f, 0.48f);
    glVertex2f(0.32f, 0.48f);
    glEnd();

    // Passenger seat.
    glColor3f(0.75f, 0.10f, 0.10f);
    glBegin(GL_QUADS);
    glVertex2f(-0.34f, 0.08f);
    glVertex2f(0.32f, 0.08f);
    glVertex2f(0.32f, 0.32f);
    glVertex2f(-0.34f, 0.32f);
    glEnd();

    // Hood / Poka layers.
    glColor3f(0.10f, 0.30f, 0.85f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.42f, 0.55f);
    glVertex2f(-0.30f, 0.72f);
    glVertex2f(-0.12f, 0.82f);
    glVertex2f(0.10f, 0.82f);
    glVertex2f(0.28f, 0.72f);
    glVertex2f(0.40f, 0.55f);
    glVertex2f(0.40f, 0.48f);
    glVertex2f(-0.42f, 0.48f);
    glEnd();

    glColor3f(0.95f, 0.80f, 0.10f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.36f, 0.60f);
    glVertex2f(-0.25f, 0.74f);
    glVertex2f(-0.10f, 0.80f);
    glVertex2f(0.08f, 0.80f);
    glVertex2f(0.24f, 0.74f);
    glVertex2f(0.35f, 0.60f);
    glVertex2f(0.35f, 0.54f);
    glVertex2f(-0.36f, 0.54f);
    glEnd();

    glColor3f(0.85f, 0.10f, 0.10f);
    glBegin(GL_POLYGON);
    glVertex2f(-0.30f, 0.65f);
    glVertex2f(-0.20f, 0.77f);
    glVertex2f(-0.08f, 0.80f);
    glVertex2f(0.06f, 0.80f);
    glVertex2f(0.20f, 0.77f);
    glVertex2f(0.30f, 0.65f);
    glVertex2f(0.30f, 0.61f);
    glVertex2f(-0.30f, 0.61f);
    glEnd();

    // Simple canopy support.
    glColor3f(0.08f, 0.08f, 0.08f);
    glBegin(GL_QUADS);
    glVertex2f(-0.43f, 0.48f);
    glVertex2f(-0.40f, 0.48f);
    glVertex2f(-0.36f, 0.08f);
    glVertex2f(-0.39f, 0.08f);
    glEnd();

    glBegin(GL_QUADS);
    glVertex2f(0.39f, 0.08f);
    glVertex2f(0.42f, 0.08f);
    glVertex2f(0.45f, 0.48f);
    glVertex2f(0.42f, 0.48f);
    glEnd();

    // Artistic floral details.
    const GLfloat motifColors[3][3] = {
        {0.95f, 0.20f, 0.30f},
        {0.20f, 0.75f, 0.35f},
        {0.95f, 0.85f, 0.15f}
    };
    for (int i = 0; i < 6; ++i) {
        const float x = -0.24f + static_cast<float>(i) * 0.10f;
        glColor3f(motifColors[i % 3][0], motifColors[i % 3][1], motifColors[i % 3][2]);
        glBegin(GL_TRIANGLES);
        glVertex2f(x, 0.37f);
        glVertex2f(x - 0.02f, 0.33f);
        glVertex2f(x + 0.02f, 0.33f);
        glEnd();

        glBegin(GL_POLYGON);
        glVertex2f(x - 0.015f, 0.305f);
        glVertex2f(x + 0.015f, 0.305f);
        glVertex2f(x + 0.02f, 0.335f);
        glVertex2f(x, 0.355f);
        glVertex2f(x - 0.02f, 0.335f);
        glEnd();
    }

    // Handlebar and pedals.
    glColor3f(0.10f, 0.10f, 0.10f);
    glBegin(GL_QUADS);
    glVertex2f(0.38f, 0.22f);
    glVertex2f(0.42f, 0.22f);
    glVertex2f(0.56f, 0.42f);
    glVertex2f(0.52f, 0.42f);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(0.18f, 0.03f);
    glVertex2f(0.18f, -0.13f);
    glVertex2f(0.10f, -0.05f);
    glVertex2f(0.26f, -0.05f);
    glEnd();

    // Wheels: two at the back, one at the front.
    drawWheel(-0.36f, -0.18f, 0.16f);
    drawWheel(0.42f, -0.18f, 0.16f);
    drawWheel(0.00f, -0.18f, 0.16f);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Background.
    glColor3f(0.97f, 0.95f, 0.90f);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Ground line.
    glColor3f(0.75f, 0.75f, 0.78f);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -0.34f);
    glVertex2f(1.0f, -0.34f);
    glVertex2f(1.0f, -0.38f);
    glVertex2f(-1.0f, -0.38f);
    glEnd();

    drawRickshaw();

    glutSwapBuffers();
}

void reshape(int width, int height) {
    if (height == 0) {
        height = 1;
    }

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void init() {
    glClearColor(0.97f, 0.95f, 0.90f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
}  // namespace

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(900, 700);
    glutCreateWindow("Stylized Bangladeshi Cycle Rickshaw");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMainLoop();
    return 0;
}