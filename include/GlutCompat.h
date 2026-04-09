#pragma once

#if __has_include(<GL/freeglut.h>)
#include <GL/freeglut.h>
#elif __has_include(<GL/glut.h>)
#include <GL/glut.h>
#else
#error "GLUT headers not found. Install freeglut and ensure headers are in MinGW include path."
#endif
