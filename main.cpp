#include <GL/glut.h>
#include <iostream>
#include <string>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Paddle Constants
const float PADDLE_WIDTH = 100.0f;
const float PADDLE_HEIGHT = 15.0f;
const float PADDLE_Y = 30.0f;
const float PADDLE_SPEED = 8.0f;

// Paddle Structure
struct Paddle
{
   float x;
   float y;
   float width;
   float height;
};

// Global paddle object
Paddle paddle;

// Keyboard state
bool keyLeft = false;
bool keyRight = false;

// Utility: Draw filled rectangle
void drawRect(float x, float y, float w, float h,
              float r, float g, float b, bool filled = true)
{
   glColor3f(r, g, b);
   if (filled)
      glBegin(GL_QUADS);
   else
      glBegin(GL_LINE_LOOP);

   glVertex2f(x, y);
   glVertex2f(x + w, y);
   glVertex2f(x + w, y + h);
   glVertex2f(x, y + h);
   glEnd();
}

// Draw Gradient Background
void drawBackground()
{
   glBegin(GL_QUADS);
   glColor3f(0.0f, 0.0f, 0.15f);
   glVertex2f(0, 0);
   glVertex2f(WINDOW_WIDTH, 0);
   glColor3f(0.0f, 0.0f, 0.35f);
   glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
   glVertex2f(0, WINDOW_HEIGHT);
   glEnd();
}

// Draw Paddle with highlight effect
void drawPaddle()
{
   // Main body - blue color
   drawRect(paddle.x, paddle.y, paddle.width, paddle.height,
            0.3f, 0.6f, 1.0f);

   // Top highlight (lighter strip)
   drawRect(paddle.x + 2, paddle.y + paddle.height - 4,
            paddle.width - 4, 3,
            0.7f, 0.9f, 1.0f);

   // Border - white outline
   drawRect(paddle.x, paddle.y, paddle.width, paddle.height,
            1.0f, 1.0f, 1.0f, false);
}

// Draw text helper
void drawText(float x, float y, const std::string &text,
              float r = 1.0f, float g = 1.0f, float b = 1.0f)
{
   glColor3f(r, g, b);
   glRasterPos2f(x, y);
   for (char c : text)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

// Display Callback
void display()
{
   glClear(GL_COLOR_BUFFER_BIT);
   glLoadIdentity();

   drawBackground();
   drawPaddle();

   drawText(220, 100, "LEFT/RIGHT Arrow or Mouse to move paddle",
            0.7f, 0.7f, 0.7f);

   glutSwapBuffers();
}

// Update - paddle movement logic
void update(float dt)
{
   if (keyLeft)
   {
      paddle.x -= PADDLE_SPEED;
      if (paddle.x < 0)
         paddle.x = 0;
   }
   if (keyRight)
   {
      paddle.x += PADDLE_SPEED;
      if (paddle.x + paddle.width > WINDOW_WIDTH)
         paddle.x = WINDOW_WIDTH - paddle.width;
   }
}

// Timer - 60 FPS game loop
void timer(int value)
{
   update(1.0f / 60.0f);
   glutPostRedisplay();
   glutTimerFunc(16, timer, 0);
}

void reshape(int w, int h)
{
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

// Keyboard press
void keyboard(unsigned char key, int x, int y)
{
   if (key == 27)
      exit(0);
}

// Special keys (arrow keys)
void specialKeys(int key, int x, int y)
{
   if (key == GLUT_KEY_LEFT)
      keyLeft = true;
   if (key == GLUT_KEY_RIGHT)
      keyRight = true;
}

void specialKeysUp(int key, int x, int y)
{
   if (key == GLUT_KEY_LEFT)
      keyLeft = false;
   if (key == GLUT_KEY_RIGHT)
      keyRight = false;
}

// Mouse passive motion (no button pressed)
void mouseMotion(int x, int y)
{
   float mx = (float)x;
   paddle.x = mx - paddle.width / 2.0f;
   if (paddle.x < 0)
      paddle.x = 0;
   if (paddle.x + paddle.width > WINDOW_WIDTH)
      paddle.x = WINDOW_WIDTH - paddle.width;
}

// Init function
void init()
{
   paddle.x = WINDOW_WIDTH / 2.0f - PADDLE_WIDTH / 2.0f;
   paddle.y = PADDLE_Y;
   paddle.width = PADDLE_WIDTH;
   paddle.height = PADDLE_HEIGHT;
}

int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
   glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
   glutInitWindowPosition(100, 50);
   glutCreateWindow("DX Ball - CSE 426");

   glClearColor(0.0f, 0.0f, 0.1f, 1.0f);

   init();

   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutSpecialFunc(specialKeys);
   glutSpecialUpFunc(specialKeysUp);
   glutPassiveMotionFunc(mouseMotion);
   glutMotionFunc(mouseMotion);
   glutTimerFunc(16, timer, 0);

   glutMainLoop();
   return 0;
}