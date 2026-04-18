#include <GL/glut.h>
#include <iostream>

// Window dimensions
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;

// Display callback - screen draw kore
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // Simple text to verify window works
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(300, 300);
    std::string msg = "DX Ball - Window OK!";
    for (char c : msg)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

    glutSwapBuffers();
}

// Reshape callback - window resize handle
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 2D orthographic projection set
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Keyboard callback - ESC to exit
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0); // ESC
}

// MAIN
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("DX Ball - CSE 426");

    // Background color: dark blue
    glClearColor(0.0f, 0.0f, 0.1f, 1.0f);

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    std::cout << "Window created successfully!" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    glutMainLoop();
    return 0;
}
