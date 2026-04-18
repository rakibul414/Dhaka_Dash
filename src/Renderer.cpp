#include "Renderer.h"

#include <cmath>
#include <cstdio>

#include "GlutCompat.h"

#include "Config.h"
#include "Player.h"

namespace {
constexpr float kRoadNearZ = 18.0f;
constexpr float kRoadFarZ = -140.0f;
constexpr float kRoadHalfWidth = 6.0f;
constexpr bool kEnableDoubleYellow = true;
constexpr bool kNightMode = true;

float gSceneScrollOffset = 0.0f;

float wrapPositive(float value, float period) {
    float wrapped = std::fmod(value, period);
    if (wrapped < 0.0f) {
        wrapped += period;
    }
    return wrapped;
}

void drawCube(float w, float h, float d) {
    glPushMatrix();
    glScalef(w, h, d);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawCylinder(float radius, float height, int slices = 16) {
    GLUquadric* quad = gluNewQuadric();
    glPushMatrix();
    // Rotate cylinder so height grows upward along world Y.
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quad, radius, radius, height, slices, 1);
    gluDisk(quad, 0.0f, radius, slices, 1);
    // Move to far cap before drawing top disk.
    glTranslatef(0.0f, 0.0f, height);
    gluDisk(quad, 0.0f, radius, slices, 1);
    glPopMatrix();
    gluDeleteQuadric(quad);
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

void drawText3D(float x, float y, float z, const char* text) {
    glRasterPos3f(x, y, z);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
        ++text;
    }
}

void drawRoad(float scrollOffset) {
    const float dashCycle = 4.4f;
    const float dashPhase = wrapPositive(scrollOffset, dashCycle);

    // Main asphalt with depth shading from far to near.
    for (float z = kRoadFarZ; z < kRoadNearZ; z += 1.8f) {
        const float t0 = (z - kRoadFarZ) / (kRoadNearZ - kRoadFarZ);
        const float t1 = (z + 1.8f - kRoadFarZ) / (kRoadNearZ - kRoadFarZ);
        const float shade0 = (t0 < 0.35f) ? 0.16f : ((t0 < 0.7f) ? 0.19f : 0.22f);
        const float shade1 = (t1 < 0.35f) ? 0.16f : ((t1 < 0.7f) ? 0.19f : 0.22f);

        glBegin(GL_QUADS);
        glColor3f(shade0, shade0, shade0 + 0.01f);
        glVertex3f(-kRoadHalfWidth, 0.0f, z);
        glVertex3f(kRoadHalfWidth, 0.0f, z);
        glColor3f(shade1, shade1, shade1 + 0.01f);
        glVertex3f(kRoadHalfWidth, 0.0f, z + 1.8f);
        glVertex3f(-kRoadHalfWidth, 0.0f, z + 1.8f);
        glEnd();
    }

    // Asphalt grain illusion using subtle alternating strips.
    for (float z = kRoadFarZ; z < kRoadNearZ; z += 0.5f) {
        const bool evenBand = (static_cast<int>((z - kRoadFarZ) * 2.0f) % 2) == 0;
        const float grain = evenBand ? 0.18f : 0.16f;
        glColor4f(grain, grain, grain, 1.0f);
        glBegin(GL_QUADS);
        glVertex3f(-kRoadHalfWidth, 0.001f, z);
        glVertex3f(kRoadHalfWidth, 0.001f, z);
        glVertex3f(kRoadHalfWidth, 0.001f, z + 0.25f);
        glVertex3f(-kRoadHalfWidth, 0.001f, z + 0.25f);
        glEnd();
    }

    // Solid road edge lines at X = +/-5.9.
    glColor3f(0.92f, 0.92f, 0.90f);
    glBegin(GL_QUADS);
    glVertex3f(-5.97f, 0.01f, kRoadFarZ);
    glVertex3f(-5.83f, 0.01f, kRoadFarZ);
    glVertex3f(-5.83f, 0.01f, kRoadNearZ);
    glVertex3f(-5.97f, 0.01f, kRoadNearZ);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(5.83f, 0.01f, kRoadFarZ);
    glVertex3f(5.97f, 0.01f, kRoadFarZ);
    glVertex3f(5.97f, 0.01f, kRoadNearZ);
    glVertex3f(5.83f, 0.01f, kRoadNearZ);
    glEnd();

    // Two animated dashed lane dividers at X = -2 and X = +2.
    glColor3f(0.95f, 0.95f, 0.92f);
    for (float x : {-2.0f, 2.0f}) {
        for (float z = kRoadFarZ - dashCycle; z < kRoadNearZ + dashCycle; z += dashCycle) {
            const float z0 = z + dashPhase;
            const float z1 = z0 + 2.2f;
            glBegin(GL_QUADS);
            glVertex3f(x - 0.05f, 0.012f, z0);
            glVertex3f(x + 0.05f, 0.012f, z0);
            glVertex3f(x + 0.05f, 0.012f, z1);
            glVertex3f(x - 0.05f, 0.012f, z1);
            glEnd();
        }
    }

    // Faint tire marks and occasional pothole patches.
    glColor3f(0.20f, 0.20f, 0.22f);
    for (float x : {-0.25f, 0.25f}) {
        glBegin(GL_QUADS);
        glVertex3f(x - 0.015f, 0.002f, kRoadFarZ);
        glVertex3f(x + 0.015f, 0.002f, kRoadFarZ);
        glVertex3f(x + 0.015f, 0.002f, kRoadNearZ);
        glVertex3f(x - 0.015f, 0.002f, kRoadNearZ);
        glEnd();
    }

    for (int i = 0; i < 7; ++i) {
        const float z = -8.0f - i * 16.0f + wrapPositive(scrollOffset, 16.0f);
        glColor3f(0.15f, 0.15f, 0.16f);
        glBegin(GL_QUADS);
        glVertex3f(-1.6f, 0.003f, z);
        glVertex3f(-1.2f, 0.003f, z);
        glVertex3f(-1.2f, 0.003f, z + 0.3f);
        glVertex3f(-1.6f, 0.003f, z + 0.3f);
        glEnd();
    }

    if (kEnableDoubleYellow) {
        glColor3f(0.95f, 0.82f, 0.14f);
        for (float x : {-0.10f, 0.10f}) {
            glBegin(GL_QUADS);
            glVertex3f(x - 0.02f, 0.013f, kRoadFarZ);
            glVertex3f(x + 0.02f, 0.013f, kRoadFarZ);
            glVertex3f(x + 0.02f, 0.013f, kRoadNearZ);
            glVertex3f(x - 0.02f, 0.013f, kRoadNearZ);
            glEnd();
        }
    }
}

void drawRoadside() {
    const float repeatSpan = 96.0f;
    const float phase = wrapPositive(gSceneScrollOffset, repeatSpan);

    // Left footpath strip.
    glColor3f(0.55f, 0.56f, 0.58f);
    glBegin(GL_QUADS);
    glVertex3f(-8.0f, 0.02f, kRoadFarZ);
    glVertex3f(-6.0f, 0.02f, kRoadFarZ);
    glVertex3f(-6.0f, 0.02f, kRoadNearZ);
    glVertex3f(-8.0f, 0.02f, kRoadNearZ);
    glEnd();

    // Right footpath strip.
    glBegin(GL_QUADS);
    glVertex3f(6.0f, 0.02f, kRoadFarZ);
    glVertex3f(8.0f, 0.02f, kRoadFarZ);
    glVertex3f(8.0f, 0.02f, kRoadNearZ);
    glVertex3f(6.0f, 0.02f, kRoadNearZ);
    glEnd();

    // Left parking zone painted yellow boxes.
    glColor3f(0.9f, 0.78f, 0.2f);
    for (int i = 0; i < 10; ++i) {
        const float z = -10.0f - i * 10.0f + phase;
        glPushMatrix();
        // Place each painted bay marker in parking strip.
        glTranslatef(-7.0f, 0.03f, z);
        drawCube(1.6f, 0.02f, 2.4f);
        glPopMatrix();
    }

    // Left vendor stalls at intervals.
    for (int i = 0; i < 8; ++i) {
        const float z = -14.0f - i * 12.0f + phase;
        const int c = i % 3;
        const Color colors[3] = {{0.75f, 0.2f, 0.2f}, {0.2f, 0.35f, 0.8f}, {0.18f, 0.65f, 0.35f}};

        glColor3f(colors[c].r, colors[c].g, colors[c].b);
        glPushMatrix();
        // Move stall body to left roadside market strip.
        glTranslatef(-10.5f, 1.1f, z);
        drawCube(2.4f, 2.2f, 2.0f);
        glPopMatrix();

        glColor3f(colors[c].r * 0.9f, colors[c].g * 0.9f, colors[c].b * 0.9f);
        glPushMatrix();
        // Lift roof slab above stall body.
        glTranslatef(-10.5f, 2.35f, z);
        drawCube(2.7f, 0.15f, 2.3f);
        glPopMatrix();
    }

    // Trees on left side with slight scale variation.
    for (int i = 0; i < 14; ++i) {
        const float z = -8.0f - i * 8.0f + phase;
        const float s = 0.9f + (i % 3) * 0.12f;

        glColor3f(0.38f, 0.23f, 0.1f);
        glPushMatrix();
        // Position tree trunk in roadside belt.
        glTranslatef(-12.8f, 0.0f, z);
        drawCylinder(0.14f * s, 1.6f * s, 12);
        glPopMatrix();

        glColor3f(0.1f, 0.56f, 0.2f);
        glPushMatrix();
        // Raise foliage cone above trunk top.
        glTranslatef(-12.8f, 2.1f * s, z);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidCone(0.8f * s, 1.8f * s, 14, 4);
        glPopMatrix();
    }

    // Utility poles and crossbars on left side.
    for (int i = 0; i < 9; ++i) {
        const float z = -10.0f - i * 12.0f + phase;

        glColor3f(0.2f, 0.2f, 0.22f);
        glPushMatrix();
        // Place pole near footpath outer boundary.
        glTranslatef(-14.1f, 0.0f, z);
        drawCylinder(0.1f, 4.0f, 10);
        glPopMatrix();

        glPushMatrix();
        // Lift crossbar near pole top.
        glTranslatef(-13.65f, 3.85f, z);
        drawCube(0.9f, 0.08f, 0.08f);
        glPopMatrix();
    }

    // Utility wires connecting poles.
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_LINES);
    for (int i = 0; i < 8; ++i) {
        const float z0 = -10.0f - i * 12.0f + phase;
        const float z1 = z0 - 12.0f;
        glVertex3f(-13.2f, 3.86f, z0);
        glVertex3f(-13.2f, 3.86f, z1);
    }
    glEnd();

    // Left small buildings (3-8 units).
    const char* leftBuildingNames[4] = {
        "Bangla Bank",
        "Sonar Mall",
        "Shapla Plaza",
        "City Bazar"
    };
    for (int i = 0; i < 12; ++i) {
        const float z = -6.0f - i * 8.0f + phase;
        const float h = 3.0f + (i % 6) * 0.9f;
        const Color c[3] = {{0.92f, 0.92f, 0.88f}, {0.95f, 0.9f, 0.74f}, {0.95f, 0.95f, 0.82f}};
        const Color bc = c[i % 3];

        glColor3f(bc.r, bc.g, bc.b);
        glPushMatrix();
        // Move building mass behind the left market strip.
        glTranslatef(-17.5f, h * 0.5f, z);
        drawCube(4.0f, h, 3.8f);
        glPopMatrix();

        glColor3f(0.35f, 0.35f, 0.38f);
        for (int w = 0; w < 3; ++w) {
            glPushMatrix();
            // Place inset window slabs on front facade.
            glTranslatef(-18.6f + w * 1.1f, 1.2f + (w % 2) * 1.3f, z + 1.92f);
            drawCube(0.5f, 0.55f, 0.06f);
            glPopMatrix();
        }

        if (i % 3 == 0) {
            glDisable(GL_LIGHTING);
            glColor3f(0.96f, 0.94f, 0.78f);
            drawText3D(-19.25f, h + 0.35f, z + 1.96f, leftBuildingNames[(i / 3) % 4]);
            glEnable(GL_LIGHTING);
        }
    }

    // Rickshaw repair shop sign suggestion.
    glColor3f(0.95f, 0.2f, 0.25f);
    glPushMatrix();
    // Raise colorful signboard above left footpath.
    glTranslatef(-10.8f, 2.9f, -6.0f + phase);
    drawCube(2.2f, 0.4f, 0.12f);
    glPopMatrix();

    // Right drain/gutter strip.
    glColor3f(0.08f, 0.08f, 0.1f);
    glBegin(GL_QUADS);
    glVertex3f(6.15f, -0.01f, kRoadFarZ);
    glVertex3f(6.5f, -0.01f, kRoadFarZ);
    glVertex3f(6.5f, -0.01f, kRoadNearZ);
    glVertex3f(6.15f, -0.01f, kRoadNearZ);
    glEnd();

    // Right bus stop structure with bench.
    glColor3f(0.5f, 0.55f, 0.6f);
    for (int i = 0; i < 4; ++i) {
        const float z = -16.0f - i * 28.0f + phase;

        glPushMatrix();
        // Move first support pole to stop position.
        glTranslatef(10.2f, 0.0f, z);
        drawCylinder(0.09f, 2.4f, 10);
        glPopMatrix();

        glPushMatrix();
        // Move second support pole to stop position.
        glTranslatef(11.6f, 0.0f, z);
        drawCylinder(0.09f, 2.4f, 10);
        glPopMatrix();

        glPushMatrix();
        // Lift stop roof slab over poles.
        glTranslatef(10.9f, 2.45f, z);
        drawCube(2.0f, 0.12f, 1.2f);
        glPopMatrix();

        glPushMatrix();
        // Place bench under bus stop roof.
        glTranslatef(10.9f, 0.55f, z);
        drawCube(1.4f, 0.16f, 0.5f);
        glPopMatrix();
    }

    // Right taller buildings (5-12 units).
    const char* rightBuildingNames[4] = {
        "Janata Bank",
        "Dhaka Shopping Mall",
        "Metro Tower",
        "Nagar Center"
    };
    for (int i = 0; i < 12; ++i) {
        const float z = -10.0f - i * 8.0f + phase;
        const float h = 5.0f + (i % 7) * 1.0f;

        glColor3f(0.85f, 0.85f, 0.8f);
        glPushMatrix();
        // Position dense city block on right skyline.
        glTranslatef(16.0f, h * 0.5f, z);
        drawCube(4.6f, h, 4.0f);
        glPopMatrix();

        // Window rows for the right-side towers.
        glColor3f(0.64f, 0.74f, 0.84f);
        for (int row = 0; row < 4; ++row) {
            const float wy = 1.0f + row * 0.95f;
            if (wy > h - 0.8f) {
                break;
            }

            for (int col = 0; col < 3; ++col) {
                glPushMatrix();
                glTranslatef(14.9f + col * 1.1f, wy, z + 2.02f);
                drawCube(0.58f, 0.52f, 0.06f);
                glPopMatrix();
            }
        }

        if (i % 3 == 0) {
            glDisable(GL_LIGHTING);
            glColor3f(0.92f, 0.95f, 1.0f);
            drawText3D(14.35f, h + 0.45f, z + 2.04f, rightBuildingNames[(i / 3) % 4]);
            glEnable(GL_LIGHTING);
        }
    }

    // Billboard on right with two poles.
    glColor3f(0.35f, 0.36f, 0.4f);
    glPushMatrix();
    // Place first billboard support pole.
    glTranslatef(13.8f, 0.0f, -22.0f + phase);
    drawCylinder(0.12f, 4.1f, 12);
    glPopMatrix();

    glPushMatrix();
    // Place second billboard support pole.
    glTranslatef(16.4f, 0.0f, -22.0f + phase);
    drawCylinder(0.12f, 4.1f, 12);
    glPopMatrix();

    glColor3f(0.9f, 0.32f, 0.2f);
    glPushMatrix();
    // Lift billboard panel above support poles.
    glTranslatef(15.1f, 4.65f, -22.0f + phase);
    drawCube(3.1f, 1.5f, 0.15f);
    glPopMatrix();

    // Parked vehicle suggestions in right pull-off area.
    for (int i = 0; i < 8; ++i) {
        const float z = -6.0f - i * 11.0f + phase;
        glColor3f(0.2f + 0.08f * (i % 3), 0.28f + 0.07f * (i % 2), 0.62f);
        glPushMatrix();
        // Move parked vehicle body to pull-off strip.
        glTranslatef(8.9f, 0.45f, z);
        drawCube(1.9f, 0.9f, 3.2f);
        glPopMatrix();
    }
}

void drawSky() {
    // Fog setup for atmospheric haze.
    const GLfloat fogColor[4] = {0.45f, 0.35f, 0.25f, 1.0f};
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogf(GL_FOG_START, 30.0f);
    glFogf(GL_FOG_END, 80.0f);

    // Gradient sky backdrop.
    glDisable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glBegin(GL_QUADS);
    glColor3f(0.45f, 0.35f, 0.25f);
    glVertex3f(-70.0f, -2.0f, kRoadFarZ - 1.0f);
    glVertex3f(70.0f, -2.0f, kRoadFarZ - 1.0f);
    glColor3f(0.05f, 0.08f, 0.25f);
    glVertex3f(70.0f, 34.0f, kRoadFarZ - 1.0f);
    glVertex3f(-70.0f, 34.0f, kRoadFarZ - 1.0f);
    glEnd();
    glEnable(GL_DEPTH_TEST);

    // Sun or moon depending on time-of-day flag.
    glColor3f(kNightMode ? 0.88f : 1.0f, kNightMode ? 0.9f : 0.9f, kNightMode ? 0.98f : 0.55f);
    glPushMatrix();
    // Move celestial sphere to upper-right sky area.
    glTranslatef(24.0f, 21.0f, -85.0f);
    glutSolidSphere(1.6f, 20, 20);
    glPopMatrix();

    // Cloud clusters from overlapping spheres.
    glColor3f(0.88f, 0.9f, 0.95f);
    const float cloudX[4] = {-24.0f, -7.0f, 12.0f, 28.0f};
    for (int i = 0; i < 4; ++i) {
        const float z = -72.0f - i * 8.0f;
        const float y = 16.0f + (i % 2) * 1.4f;

        glPushMatrix();
        // Position base cloud sphere in distant sky.
        glTranslatef(cloudX[i], y, z);
        glutSolidSphere(1.25f, 14, 10);
        glPopMatrix();

        glPushMatrix();
        // Offset second sphere for cloud cluster shape.
        glTranslatef(cloudX[i] + 1.1f, y + 0.2f, z + 0.4f);
        glutSolidSphere(1.0f, 14, 10);
        glPopMatrix();

        glPushMatrix();
        // Offset third sphere for cloud cluster shape.
        glTranslatef(cloudX[i] - 1.0f, y - 0.1f, z + 0.3f);
        glutSolidSphere(0.9f, 14, 10);
        glPopMatrix();
    }

    // Distant skyline silhouette near road horizon.
    glColor3f(0.16f, 0.16f, 0.2f);
    for (int i = 0; i < 18; ++i) {
        const float x = -35.0f + i * 4.0f;
        const float h = 2.0f + (i % 6) * 1.1f;
        glPushMatrix();
        // Move skyline cuboid along horizon line.
        glTranslatef(x, h * 0.5f, kRoadFarZ + 1.5f);
        drawCube(3.4f, h, 1.2f);
        glPopMatrix();
    }

    if (kNightMode) {
        // Night stars as point cluster.
        glDisable(GL_FOG);
        glPointSize(2.0f);
        glColor3f(0.95f, 0.95f, 1.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < 80; ++i) {
            const float x = -42.0f + std::fmod(i * 13.0f, 84.0f);
            const float y = 12.0f + std::fmod(i * 7.0f, 16.0f);
            const float z = -90.0f - std::fmod(i * 5.0f, 18.0f);
            glVertex3f(x, y, z);
        }
        glEnd();
        glEnable(GL_FOG);
    }
}

void drawFullScene(float scrollOffset) {
    gSceneScrollOffset = scrollOffset;
    glEnable(GL_COLOR_MATERIAL);
    drawSky();
    drawRoad(scrollOffset);
    drawRoadside();
    glDisable(GL_FOG);
}

void drawRickshawCanopyBands() {
    // Build a front semicircle feel using stacked arc segments made of tiny cubes.
    const float radii[5] = {0.78f, 0.66f, 0.54f, 0.44f, 0.34f};
    const Color bands[5] = {
        {0.1f, 0.2f, 0.8f},
        {1.0f, 0.85f, 0.0f},
        {0.9f, 0.1f, 0.1f},
        {0.95f, 0.92f, 0.84f},
        {0.1f, 0.2f, 0.8f}
    };

    for (int b = 0; b < 5; ++b) {
        glColor3f(bands[b].r, bands[b].g, bands[b].b);
        for (int i = 0; i <= 18; ++i) {
            const float a = static_cast<float>(i) * 3.14159f / 18.0f;
            const float x = std::cos(a) * radii[b] * 0.9f;
            const float y = std::sin(a) * radii[b] * 0.55f;

            glPushMatrix();
            // Position decorative canopy arc tile on the front face.
            glTranslatef(x, 1.34f + y, 0.50f);
            drawCube(0.07f, 0.05f, 0.03f);
            glPopMatrix();
        }
    }
}

void drawRickshawArtPanel() {
    // Base poster plate.
    glColor3f(0.2f, 0.35f, 0.82f);
    glPushMatrix();
    // Move painted panel to seat front face.
    glTranslatef(0.0f, 0.45f, 0.24f);
    drawCube(1.0f, 0.44f, 0.03f);
    glPopMatrix();

    // Stylized flower motifs.
    const Color petals[3] = {{0.95f, 0.2f, 0.3f}, {0.95f, 0.82f, 0.12f}, {0.2f, 0.8f, 0.3f}};
    for (int i = 0; i < 3; ++i) {
        glColor3f(petals[i].r, petals[i].g, petals[i].b);
        glPushMatrix();
        // Place petal motif blocks across panel width.
        glTranslatef(-0.28f + i * 0.28f, 0.48f, 0.258f);
        drawCube(0.14f, 0.1f, 0.01f);
        glPopMatrix();
    }

    // Scenic strip.
    glColor3f(0.14f, 0.56f, 0.84f);
    glPushMatrix();
    // Add lower landscape-like strip on art panel.
    glTranslatef(0.0f, 0.36f, 0.258f);
    drawCube(0.92f, 0.08f, 0.01f);
    glPopMatrix();
}

void drawRickshawWheelUnit(float wheelAngle) {
    // Proportioned wheel: smaller diameter so it reads like a rickshaw wheel.
    glColor3f(0.08f, 0.08f, 0.08f);
    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glutSolidTorus(0.045f, 0.16f, 16, 24);
    glPopMatrix();

    glPushMatrix();
    glRotatef(wheelAngle, 1.0f, 0.0f, 0.0f);

    glColor3f(0.55f, 0.55f, 0.58f);
    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(-0.06f, 0.0f, 0.0f);
    glutSolidCylinder(0.10f, 0.12f, 14, 2);
    glPopMatrix();

    glColor3f(0.38f, 0.38f, 0.40f);
    for (int i = 0; i < 10; ++i) {
        glPushMatrix();
        glRotatef(i * 36.0f, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, 0.08f, 0.0f);
        drawCube(0.015f, 0.13f, 0.015f);
        glPopMatrix();
    }

    glColor3f(0.95f, 0.82f, 0.08f);
    glutSolidSphere(0.05f, 12, 10);

    glPopMatrix();
}

void drawRickshaw(float wheelAngle) {
    // Familiar older-style rickshaw body.
    glColor3f(0.114f, 0.721f, 0.290f);
    glPushMatrix();
    glTranslatef(0.0f, 0.64f, -0.2f);
    drawCube(1.4f, 0.76f, 1.38f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.92f, -0.2f);
    drawCube(1.24f, 0.24f, 1.2f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.1f, -0.2f);
    drawCube(1.02f, 0.15f, 1.04f);
    glPopMatrix();

    glColor3f(0.94f, 0.84f, 0.14f);
    glPushMatrix();
    glTranslatef(0.0f, 0.82f, -0.2f);
    drawCube(1.42f, 0.08f, 1.4f);
    glPopMatrix();

    glColor3f(0.1f, 0.2f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0f, 1.34f, -0.18f);
    drawCube(1.7f, 0.09f, 1.7f);
    glPopMatrix();

    glColor3f(1.0f, 0.85f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 1.40f, -0.18f);
    drawCube(1.56f, 0.05f, 1.56f);
    glPopMatrix();

    glColor3f(0.9f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 1.45f, -0.18f);
    drawCube(1.42f, 0.04f, 1.42f);
    glPopMatrix();

    glColor3f(0.95f, 0.92f, 0.84f);
    glPushMatrix();
    glTranslatef(0.0f, 1.48f, -0.18f);
    drawCube(1.28f, 0.03f, 1.28f);
    glPopMatrix();

    glColor3f(0.1f, 0.2f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0f, 1.51f, -0.18f);
    drawCube(1.04f, 0.03f, 1.04f);
    glPopMatrix();

    glPushMatrix();
    drawRickshawCanopyBands();
    glPopMatrix();

    const Color fringe[4] = {
        {0.9f, 0.15f, 0.15f},
        {0.2f, 0.75f, 0.25f},
        {0.96f, 0.84f, 0.12f},
        {0.95f, 0.95f, 0.95f}
    };
    for (int i = 0; i < 12; ++i) {
        const int c = i % 4;
        glColor3f(fringe[c].r, fringe[c].g, fringe[c].b);
        glPushMatrix();
        glTranslatef(-0.66f + i * 0.12f, 1.27f, 0.54f);
        drawCube(0.06f, 0.09f, 0.02f);
        glPopMatrix();
    }

    glColor3f(0.08f, 0.58f, 0.24f);
    glPushMatrix();
    glTranslatef(0.0f, 0.63f, 0.46f);
    drawCube(0.78f, 0.52f, 0.68f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.88f, 0.41f);
    drawCube(0.58f, 0.24f, 0.52f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.02f, 0.36f);
    drawCube(0.42f, 0.12f, 0.36f);
    glPopMatrix();

    glColor3f(0.7f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(0.0f, 0.58f, -0.46f);
    drawCube(0.96f, 0.16f, 0.5f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.88f, -0.72f);
    drawCube(0.94f, 0.56f, 0.09f);
    glPopMatrix();

    for (float x : {-0.54f, 0.54f}) {
        glPushMatrix();
        glTranslatef(x, 0.72f, -0.45f);
        drawCube(0.08f, 0.18f, 0.46f);
        glPopMatrix();
    }

    glPushMatrix();
    drawRickshawArtPanel();
    glPopMatrix();

    glColor3f(0.22f, 0.22f, 0.24f);
    glPushMatrix();
    glTranslatef(0.0f, 0.26f, 0.42f);
    drawCube(1.02f, 0.04f, 0.04f);
    glPopMatrix();

    glColor3f(0.2f, 0.16f, 0.1f);
    glPushMatrix();
    glTranslatef(0.0f, 0.54f, 0.12f);
    drawCube(0.46f, 0.07f, 0.34f);
    glPopMatrix();

    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix();
    glTranslatef(-0.43f, 0.28f, 0.1f);
    drawCube(0.05f, 0.05f, 1.7f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.43f, 0.28f, 0.1f);
    drawCube(0.05f, 0.05f, 1.7f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.24f, -0.12f);
    drawCube(0.86f, 0.05f, 0.08f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.21f, 0.44f, -0.02f);
    glRotatef(-24.0f, 0.0f, 0.0f, 1.0f);
    drawCube(0.05f, 0.42f, 0.05f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.21f, 0.44f, -0.02f);
    glRotatef(24.0f, 0.0f, 0.0f, 1.0f);
    drawCube(0.05f, 0.42f, 0.05f);
    glPopMatrix();

    for (float x : {-0.6f, 0.6f}) {
        for (float z : {-0.72f, 0.22f}) {
            glPushMatrix();
            glTranslatef(x, 0.9f, z);
            drawCube(0.05f, 0.9f, 0.05f);
            glPopMatrix();
        }
    }

    const Color dots[3] = {{0.9f, 0.1f, 0.1f}, {0.95f, 0.8f, 0.12f}, {0.2f, 0.75f, 0.25f}};
    for (int i = 0; i < 6; ++i) {
        const int c = i % 3;
        glColor3f(dots[c].r, dots[c].g, dots[c].b);
        glPushMatrix();
        glTranslatef(-0.48f + (i % 3) * 0.48f, 0.32f + (i / 3) * 0.28f, -0.08f);
        glutSolidSphere(0.025f, 8, 8);
        glPopMatrix();
    }

    glColor3f(0.3f, 0.3f, 0.3f);
    glPushMatrix();
    glTranslatef(0.0f, 1.02f, 0.58f);
    drawCube(0.06f, 0.58f, 0.06f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 1.26f, 0.58f);
    drawCube(0.66f, 0.05f, 0.05f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.56f, 0.58f);
    drawCube(0.05f, 0.62f, 0.05f);
    glPopMatrix();

    glColor3f(0.09f, 0.09f, 0.09f);
    glPushMatrix();
    glTranslatef(-0.36f, 1.26f, 0.58f);
    drawCube(0.08f, 0.06f, 0.06f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.36f, 1.26f, 0.58f);
    drawCube(0.08f, 0.06f, 0.06f);
    glPopMatrix();

    glColor3f(0.82f, 0.82f, 0.86f);
    glPushMatrix();
    glTranslatef(0.17f, 1.30f, 0.58f);
    glutSolidSphere(0.04f, 12, 8);
    glPopMatrix();

    glColor3f(0.97f, 0.85f, 0.24f);
    glPushMatrix();
    glTranslatef(0.0f, 0.9f, 0.78f);
    glutSolidSphere(0.08f, 12, 10);
    glPopMatrix();

    glColor3f(0.22f, 0.22f, 0.24f);
    glPushMatrix();
    glTranslatef(0.0f, 0.28f, 0.0f);
    glutSolidTorus(0.012f, 0.12f, 12, 24);
    glPopMatrix();

    for (float ang : {wheelAngle * 0.8f, wheelAngle * 0.8f + 180.0f}) {
        glPushMatrix();
        glTranslatef(0.0f, 0.28f, 0.0f);
        glRotatef(ang, 0.0f, 0.0f, 1.0f);
        glTranslatef(0.1f, 0.0f, 0.0f);
        drawCube(0.2f, 0.02f, 0.02f);
        glTranslatef(0.11f, 0.0f, 0.0f);
        drawCube(0.08f, 0.03f, 0.05f);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(0.46f, 0.28f, -0.4f);
    glutSolidTorus(0.008f, 0.05f, 10, 20);
    glPopMatrix();

    glColor3f(0.75f, 0.75f, 0.75f);
    for (int i = 0; i < 12; ++i) {
        glPushMatrix();
        glTranslatef(0.22f + i * 0.02f, 0.28f, -0.06f - i * 0.028f);
        drawCube(0.014f, 0.01f, 0.018f);
        glPopMatrix();
    }

    glColor3f(0.18f, 0.18f, 0.2f);
    glPushMatrix();
    glTranslatef(0.44f, 0.34f, -0.37f);
    drawCube(0.08f, 0.22f, 0.42f);
    glPopMatrix();

    glColor3f(0.12f, 0.45f, 0.86f);
    for (float x : {-0.55f, 0.55f}) {
        glPushMatrix();
        glTranslatef(x, 0.42f, -0.4f);
        drawCube(0.36f, 0.05f, 0.22f);
        glPopMatrix();

        for (int i = 0; i < 4; ++i) {
            glColor3f(0.95f, 0.95f, 0.95f);
            glPushMatrix();
            glTranslatef(x - 0.12f + i * 0.08f, 0.36f, -0.29f);
            glutSolidSphere(0.015f, 8, 8);
            glPopMatrix();
        }

        glColor3f(0.95f, 0.2f, 0.25f);
        glPushMatrix();
        glTranslatef(x, 0.42f, -0.29f);
        drawCube(0.06f, 0.06f, 0.01f);
        glPopMatrix();
    }

    glPushMatrix();
    glTranslatef(-0.55f, 0.24f, -0.4f);
    drawRickshawWheelUnit(wheelAngle);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.55f, 0.24f, -0.4f);
    drawRickshawWheelUnit(wheelAngle);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.2f, 0.55f);
    drawRickshawWheelUnit(0.0f);
    glPopMatrix();
}

void drawCarWheelUnit(float wheelAngle) {
    glColor3f(0.06f, 0.06f, 0.06f);
    glPushMatrix();
    // Rotate tyre around local X to animate rolling motion.
    glRotatef(wheelAngle, 1.0f, 0.0f, 0.0f);
    glutSolidTorus(0.045f, 0.16f, 14, 22);
    glPopMatrix();

    glColor3f(0.58f, 0.6f, 0.62f);
    glPushMatrix();
    // Align hub cylinder with wheel axle direction.
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glutSolidCylinder(0.05f, 0.12f, 12, 2);
    glPopMatrix();

    glColor3f(0.78f, 0.78f, 0.8f);
    for (int i = 0; i < 5; ++i) {
        const float ang = i * 72.0f;
        glPushMatrix();
        // Rotate spoke placement around wheel center.
        glRotatef(wheelAngle + ang, 1.0f, 0.0f, 0.0f);
        // Move spoke segment outward from hub center.
        glTranslatef(0.0f, 0.08f, 0.0f);
        drawCube(0.018f, 0.13f, 0.018f);
        glPopMatrix();
    }
}

void drawCar(float wheelAngle, float r, float g, float b) {
    // Lower chassis body.
    glColor3f(r, g, b);
    glPushMatrix();
    // Lift chassis cuboid to sit above road plane.
    glTranslatef(0.0f, 0.36f, 0.0f);
    drawCube(1.34f, 0.36f, 2.25f);
    glPopMatrix();

    // Upper cabin.
    glPushMatrix();
    // Move narrower cabin towards center-rear of chassis.
    glTranslatef(0.0f, 0.62f, -0.22f);
    drawCube(1.02f, 0.3f, 1.2f);
    glPopMatrix();

    // Front windshield.
    glColor3f(r * 0.75f + 0.2f, g * 0.75f + 0.2f, b * 0.75f + 0.2f);
    glPushMatrix();
    // Move windshield slab to front cabin edge.
    glTranslatef(0.0f, 0.67f, 0.3f);
    // Tilt windshield backward by about 25 degrees.
    glRotatef(25.0f, 1.0f, 0.0f, 0.0f);
    drawCube(0.94f, 0.18f, 0.04f);
    glPopMatrix();

    // Rear windshield.
    glPushMatrix();
    // Move rear windshield slab to back cabin edge.
    glTranslatef(0.0f, 0.67f, -0.74f);
    // Tilt rear windshield in mirrored direction.
    glRotatef(-25.0f, 1.0f, 0.0f, 0.0f);
    drawCube(0.94f, 0.18f, 0.04f);
    glPopMatrix();

    // Roof panel.
    glColor3f(r, g, b);
    glPushMatrix();
    // Lift roof slab to connect windshield tops.
    glTranslatef(0.0f, 0.79f, -0.22f);
    drawCube(0.9f, 0.06f, 0.76f);
    glPopMatrix();

    // Side windows (two per side).
    glColor3f(0.75f, 0.82f, 0.88f);
    for (float x : {-0.52f, 0.52f}) {
        for (float z : {-0.45f, -0.02f}) {
            glPushMatrix();
            // Move side window slab to door frame location.
            glTranslatef(x, 0.64f, z);
            drawCube(0.04f, 0.2f, 0.34f);
            glPopMatrix();
        }
    }

    // Sloped hood layers.
    glColor3f(r * 0.95f, g * 0.95f, b * 0.95f);
    glPushMatrix();
    // Place first hood layer at front upper chassis.
    glTranslatef(0.0f, 0.56f, 0.78f);
    drawCube(1.0f, 0.08f, 0.38f);
    glPopMatrix();

    glPushMatrix();
    // Place second hood layer slightly higher and shorter.
    glTranslatef(0.0f, 0.6f, 0.92f);
    drawCube(0.92f, 0.06f, 0.24f);
    glPopMatrix();

    // Sloped trunk layers.
    glPushMatrix();
    // Place first trunk layer at rear upper chassis.
    glTranslatef(0.0f, 0.56f, -0.9f);
    drawCube(1.0f, 0.08f, 0.34f);
    glPopMatrix();

    glPushMatrix();
    // Place second trunk layer slightly higher and shorter.
    glTranslatef(0.0f, 0.6f, -1.02f);
    drawCube(0.9f, 0.06f, 0.22f);
    glPopMatrix();

    // Front and rear bumpers.
    glColor3f(0.2f, 0.2f, 0.22f);
    glPushMatrix();
    // Move front bumper to nose edge.
    glTranslatef(0.0f, 0.26f, 1.14f);
    drawCube(1.28f, 0.12f, 0.08f);
    glPopMatrix();

    glPushMatrix();
    // Move rear bumper to tail edge.
    glTranslatef(0.0f, 0.26f, -1.14f);
    drawCube(1.28f, 0.12f, 0.08f);
    glPopMatrix();

    // Headlights and tail lights.
    glColor3f(0.98f, 0.95f, 0.72f);
    for (float x : {-0.47f, 0.47f}) {
        glPushMatrix();
        // Move headlight sphere to front corner.
        glTranslatef(x, 0.4f, 1.08f);
        glutSolidSphere(0.06f, 10, 8);
        glPopMatrix();
    }

    glColor3f(0.9f, 0.12f, 0.12f);
    for (float x : {-0.47f, 0.47f}) {
        glPushMatrix();
        // Move tail light sphere to rear corner.
        glTranslatef(x, 0.4f, -1.08f);
        glutSolidSphere(0.055f, 10, 8);
        glPopMatrix();
    }

    // Front grille with vertical slats.
    glColor3f(0.16f, 0.16f, 0.17f);
    for (int i = 0; i < 5; ++i) {
        glPushMatrix();
        // Place grille slat across front center section.
        glTranslatef(-0.24f + i * 0.12f, 0.34f, 1.1f);
        drawCube(0.03f, 0.18f, 0.04f);
        glPopMatrix();
    }

    // Wheel arches.
    glColor3f(r * 0.85f, g * 0.85f, b * 0.85f);
    const float wx[2] = {-0.55f, 0.55f};
    const float wz[2] = {0.9f, -0.9f};
    for (float x : wx) {
        for (float z : wz) {
            glPushMatrix();
            // Move wheel arch piece above wheel center.
            glTranslatef(x, 0.48f, z);
            drawCube(0.38f, 0.08f, 0.24f);
            glPopMatrix();
        }
    }

    // Side mirrors.
    glColor3f(r, g, b);
    for (float x : {-0.58f, 0.58f}) {
        glPushMatrix();
        // Move side mirror to front door area.
        glTranslatef(x, 0.62f, 0.18f);
        drawCube(0.08f, 0.05f, 0.1f);
        glPopMatrix();
    }

    // Door seams as thin dark grooves.
    glColor3f(0.12f, 0.12f, 0.14f);
    for (float x : {-0.53f, 0.53f}) {
        glPushMatrix();
        // Place front-rear door seam indicator line.
        glTranslatef(x, 0.45f, -0.08f);
        drawCube(0.02f, 0.28f, 0.02f);
        glPopMatrix();
    }

    // Exhaust pipe.
    glColor3f(0.18f, 0.18f, 0.2f);
    glPushMatrix();
    // Move exhaust tip to rear-right lower corner.
    glTranslatef(0.44f, 0.17f, -1.15f);
    // Align exhaust cylinder with rear direction axis.
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCylinder(0.035f, 0.12f, 10, 2);
    glPopMatrix();

    // Wheels with required positions.
    for (float x : {-0.55f, 0.55f}) {
        glPushMatrix();
        // Move front wheel to requested coordinate.
        glTranslatef(x, 0.18f, 0.9f);
        drawCarWheelUnit(wheelAngle);
        glPopMatrix();

        glPushMatrix();
        // Move rear wheel to requested coordinate.
        glTranslatef(x, 0.18f, -0.9f);
        drawCarWheelUnit(wheelAngle);
        glPopMatrix();
    }
}

void drawBus(float wheelAngle, float r, float g, float b) {
    // Main bus body shell.
    glColor3f(r, g, b);
    glPushMatrix();
    // Lift body block above road to bus center height.
    glTranslatef(0.0f, 0.76f, 0.0f);
    drawCube(1.52f, 1.3f, 3.2f);
    glPopMatrix();

    // Roof cap.
    glColor3f(r * 0.85f, g * 0.85f, b * 0.85f);
    glPushMatrix();
    // Move roof slab to top of bus body.
    glTranslatef(0.0f, 1.45f, 0.0f);
    drawCube(1.5f, 0.09f, 3.1f);
    glPopMatrix();

    // Window band.
    glColor3f(0.72f, 0.82f, 0.9f);
    for (int i = 0; i < 5; ++i) {
        const float z = 1.1f - i * 0.55f;
        glPushMatrix();
        // Move left window panel to side wall.
        glTranslatef(-0.77f, 1.0f, z);
        drawCube(0.04f, 0.34f, 0.42f);
        glPopMatrix();

        glPushMatrix();
        // Move right window panel to side wall.
        glTranslatef(0.77f, 1.0f, z);
        drawCube(0.04f, 0.34f, 0.42f);
        glPopMatrix();
    }

    // Windshields.
    glColor3f(0.78f, 0.86f, 0.92f);
    glPushMatrix();
    // Place front windshield panel.
    glTranslatef(0.0f, 1.0f, 1.63f);
    drawCube(1.2f, 0.5f, 0.04f);
    glPopMatrix();

    glPushMatrix();
    // Place rear windshield panel.
    glTranslatef(0.0f, 1.0f, -1.63f);
    drawCube(1.2f, 0.5f, 0.04f);
    glPopMatrix();

    // Bumpers.
    glColor3f(0.2f, 0.2f, 0.22f);
    glPushMatrix();
    // Move front bumper to front lower edge.
    glTranslatef(0.0f, 0.22f, 1.67f);
    drawCube(1.46f, 0.13f, 0.08f);
    glPopMatrix();

    glPushMatrix();
    // Move rear bumper to rear lower edge.
    glTranslatef(0.0f, 0.22f, -1.67f);
    drawCube(1.46f, 0.13f, 0.08f);
    glPopMatrix();

    // Head/tail lights.
    glColor3f(0.98f, 0.93f, 0.68f);
    for (float x : {-0.56f, 0.56f}) {
        glPushMatrix();
        // Move bus headlight to front corner.
        glTranslatef(x, 0.5f, 1.64f);
        glutSolidSphere(0.06f, 10, 8);
        glPopMatrix();
    }

    glColor3f(0.9f, 0.14f, 0.14f);
    for (float x : {-0.56f, 0.56f}) {
        glPushMatrix();
        // Move bus tail light to rear corner.
        glTranslatef(x, 0.5f, -1.64f);
        glutSolidSphere(0.055f, 10, 8);
        glPopMatrix();
    }

    // Wheels (2 front + 2 rear style).
    for (float x : {-0.66f, 0.66f}) {
        glPushMatrix();
        // Move front bus wheel near front axle.
        glTranslatef(x, 0.2f, 1.0f);
        drawCarWheelUnit(wheelAngle);
        glPopMatrix();

        glPushMatrix();
        // Move rear bus wheel near rear axle.
        glTranslatef(x, 0.2f, -1.0f);
        drawCarWheelUnit(wheelAngle);
        glPopMatrix();
    }
}

void drawRickshaw(const Player& player) {
    const float x = player.x();

    glPushMatrix();
    // Move the whole rickshaw model to current lane X and fixed gameplay Z.
    glTranslatef(x, cfg::playerY, cfg::playerZ);

    // Rotate wheels based on scene movement to fake forward rolling.
    const float wheelAngle = std::fmod(gSceneScrollOffset * 60.0f, 360.0f);
    drawRickshaw(wheelAngle);

    glPopMatrix();
}

void drawObstacle(const Obstacle& o) {
    glPushMatrix();
    // Place each obstacle in one of the lane centers and animated Z position.
    glTranslatef(o.x, o.height * 0.5f, o.z);

    if (o.type == 1) {
        const float wheelAngle = std::fmod((gSceneScrollOffset + o.z) * 45.0f, 360.0f);
        glPushMatrix();
        // Scale unit sedan model to obstacle bounding dimensions.
        glScalef(o.width / 1.34f, o.height / 0.92f, o.depth / 2.25f);
        drawCar(wheelAngle, o.color.r, o.color.g, o.color.b);
        glPopMatrix();
    } else if (o.type == 2) {
        const float wheelAngle = std::fmod((gSceneScrollOffset + o.z) * 38.0f, 360.0f);
        glPushMatrix();
        // Scale bus model to obstacle dimensions.
        glScalef(o.width / 1.52f, o.height / 1.54f, o.depth / 3.2f);
        drawBus(wheelAngle, o.color.r * 0.9f, o.color.g * 0.9f, o.color.b * 0.9f);
        glPopMatrix();
    } else {
        // Stylized road barricade (rare) instead of plain box.
        glColor3f(o.color.r, o.color.g, o.color.b);
        glPushMatrix();
        // Place lower barricade base block.
        glTranslatef(0.0f, -o.height * 0.12f, 0.0f);
        drawCube(o.width, o.height * 0.6f, o.depth);
        glPopMatrix();

        glPushMatrix();
        // Place upper taper block for barricade profile.
        glTranslatef(0.0f, o.height * 0.18f, 0.0f);
        drawCube(o.width * 0.82f, o.height * 0.42f, o.depth * 0.86f);
        glPopMatrix();

        glColor3f(0.96f, 0.88f, 0.16f);
        for (int i = 0; i < 3; ++i) {
            glPushMatrix();
            // Add yellow hazard stripe slabs on front face.
            glTranslatef(0.0f, -0.05f + i * 0.16f, o.depth * 0.43f);
            drawCube(o.width * 0.7f, 0.06f, 0.07f);
            glPopMatrix();
        }
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

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
}

void end2D() {
    glEnable(GL_LIGHTING);
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
    gluPerspective(52.0, static_cast<double>(width) / static_cast<double>(height), 0.3, 250.0);

    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    const GLfloat ambient[] = {0.28f, 0.28f, 0.3f, 1.0f};
    const GLfloat diffuse[] = {0.95f, 0.95f, 0.9f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
}

void drawText2D(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
        ++text;
    }
}

void drawOpeningScreen(int selectedMode, int highScore) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glClearColor(0.03f, 0.05f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(cfg::windowWidth), 0.0, static_cast<double>(cfg::windowHeight));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor3f(0.04f, 0.07f, 0.18f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(static_cast<float>(cfg::windowWidth), 0.0f);
    glColor3f(0.12f, 0.09f, 0.2f);
    glVertex2f(static_cast<float>(cfg::windowWidth), static_cast<float>(cfg::windowHeight));
    glVertex2f(0.0f, static_cast<float>(cfg::windowHeight));
    glEnd();

    glColor3f(0.95f, 0.92f, 0.75f);
    drawText2D(430.0f, 600.0f, "DHAKA DASH: RICKSHAW RUN");

    glColor3f(0.78f, 0.85f, 0.95f);
    drawText2D(500.0f, 555.0f, "Choose Difficulty Mode");

    const char* modes[3] = {
        "1. CASUAL",
        "2. COMPETITIVE",
        "3. INSANE"
    };

    for (int i = 0; i < 3; ++i) {
        if (i == selectedMode) {
            glColor3f(0.96f, 0.82f, 0.2f);
        } else {
            glColor3f(0.86f, 0.9f, 0.94f);
        }
        drawText2D(545.0f, 475.0f - i * 60.0f, modes[i]);
    }

    glColor3f(0.78f, 0.9f, 0.78f);
    drawText2D(375.0f, 235.0f, "Press 1/2/3 to select, 4/S for showcase, ENTER to start");

    glColor3f(0.75f, 0.75f, 0.82f);
    drawText2D(340.0f, 190.0f, "In-game: A/D or Arrow keys, R restart, M/B back to menu, ESC exit");

    char bestScoreText[64];
    std::snprintf(bestScoreText, sizeof(bestScoreText), "Highest Score: %d", highScore);
    glColor3f(0.96f, 0.86f, 0.32f);
    drawText2D(530.0f, 145.0f, bestScoreText);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

void drawWorld(const Player& player, const ObstacleManager& obstacleManager, float score, int highScore, bool gameOver, bool paused) {
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float playerX = player.x();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(52.0, static_cast<double>(glutGet(GLUT_WINDOW_WIDTH)) / static_cast<double>(std::max(1, glutGet(GLUT_WINDOW_HEIGHT))), 0.3, 250.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // Keep the camera farther behind and a bit higher so the player rickshaw
    // remains inside frame even during lane changes.
    gluLookAt(playerX * 0.55f, 6.1f, 15.8f,
              playerX * 0.35f, 1.0f, -20.0f,
              0.0f, 1.0f, 0.0f);

    const GLfloat lightPos[] = {6.0f, 11.0f, 14.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    drawFullScene(score * 0.7f);
    drawRickshaw(player);

    for (const auto& o : obstacleManager.obstacles()) {
        if (!o.active) {
            continue;
        }
        drawObstacle(o);
    }

    begin2D(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

    char scoreText[64];
    char highScoreText[64];
    std::snprintf(scoreText, sizeof(scoreText), "Score: %d", static_cast<int>(score));
    std::snprintf(highScoreText, sizeof(highScoreText), "Highest: %d", highScore);
    glColor3f(0.95f, 0.95f, 0.95f);
    drawText2D(20.0f, glutGet(GLUT_WINDOW_HEIGHT) - 35.0f, scoreText);
    glColor3f(0.96f, 0.84f, 0.3f);
    drawText2D(20.0f, glutGet(GLUT_WINDOW_HEIGHT) - 62.0f, highScoreText);

    if (gameOver) {
        glColor3f(1.0f, 0.3f, 0.3f);
        drawText2D(glutGet(GLUT_WINDOW_WIDTH) * 0.5f - 120.0f,
                   glutGet(GLUT_WINDOW_HEIGHT) * 0.5f,
                   "GAME OVER - Press R to Restart");
    }

    if (paused && !gameOver) {
        glColor3f(1.0f, 0.92f, 0.2f);
        drawText2D(glutGet(GLUT_WINDOW_WIDTH) * 0.5f - 70.0f,
                   glutGet(GLUT_WINDOW_HEIGHT) * 0.5f + 24.0f,
                   "PAUSED");
        glColor3f(0.88f, 0.9f, 0.95f);
        drawText2D(glutGet(GLUT_WINDOW_WIDTH) * 0.5f - 168.0f,
                   glutGet(GLUT_WINDOW_HEIGHT) * 0.5f - 6.0f,
                   "Press SPACE again to resume");
    }

    end2D();

    glutSwapBuffers();
}

void drawRickshawShowcase() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glClearColor(0.97f, 0.95f, 0.90f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(0.97f, 0.95f, 0.90f);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    glColor3f(0.82f, 0.85f, 0.88f);
    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -0.32f);
    glVertex2f(1.0f, -0.32f);
    glVertex2f(1.0f, -0.38f);
    glVertex2f(-1.0f, -0.38f);
    glEnd();

    // Reuse the stylized 2D rickshaw look from the standalone sample.
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

    glColor3f(0.75f, 0.10f, 0.10f);
    glBegin(GL_QUADS);
    glVertex2f(-0.34f, 0.08f);
    glVertex2f(0.32f, 0.08f);
    glVertex2f(0.32f, 0.32f);
    glVertex2f(-0.34f, 0.32f);
    glEnd();

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

    glColor3f(0.10f, 0.10f, 0.10f);
    glBegin(GL_LINES);
    glVertex2f(0.18f, 0.03f);
    glVertex2f(0.18f, -0.13f);
    glVertex2f(0.10f, -0.05f);
    glVertex2f(0.26f, -0.05f);
    glEnd();

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
    }

    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 48; ++i) {
        const float angle = 2.0f * 3.1415926f * static_cast<float>(i) / 48.0f;
        glVertex2f(-0.36f + std::cos(angle) * 0.16f, -0.18f + std::sin(angle) * 0.16f);
    }
    glEnd();

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 48; ++i) {
        const float angle = 2.0f * 3.1415926f * static_cast<float>(i) / 48.0f;
        glVertex2f(0.42f + std::cos(angle) * 0.16f, -0.18f + std::sin(angle) * 0.16f);
    }
    glEnd();

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 48; ++i) {
        const float angle = 2.0f * 3.1415926f * static_cast<float>(i) / 48.0f;
        glVertex2f(0.00f + std::cos(angle) * 0.16f, -0.18f + std::sin(angle) * 0.16f);
    }
    glEnd();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glutSwapBuffers();
}
}  // namespace renderer
