#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <algorithm>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float PADDLE_WIDTH = 100.0f;
const float PADDLE_HEIGHT = 15.0f;
const float PADDLE_Y = 30.0f;
const float PADDLE_SPEED = 8.0f;
const float BALL_RADIUS = 10.0f;
const float BALL_SPEED_INIT = 4.5f;
const int BRICK_ROWS = 6;
const int BRICK_COLS = 10;
const float BRICK_WIDTH = 70.0f;
const float BRICK_HEIGHT = 25.0f;
const float BRICK_PADDING = 5.0f;
const float BRICK_START_X = 25.0f;
const float BRICK_START_Y = 430.0f;

struct Paddle
{
   float x, y, width, height;
};
struct Ball
{
   float x, y, dx, dy, speed;
   bool active;
};

struct Brick
{
   float x, y;
   bool active;
   int health;    // 1 or 2 hits
   float r, g, b; // color
};

Paddle paddle;
Ball ball;
std::vector<Brick> bricks;
bool ballOnPaddle = true;
bool keyLeft = false;
bool keyRight = false;
int score = 0;

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

void drawCircle(float cx, float cy, float radius,
                float r, float g, float b)
{
   glColor3f(r, g, b);
   glBegin(GL_POLYGON);
   for (int i = 0; i < 36; i++)
   {
      float a = 2.0f * 3.14159265f * i / 36;
      glVertex2f(cx + radius * cosf(a), cy + radius * sinf(a));
   }
   glEnd();
}

std::string toStr(int v)
{
   std::ostringstream o;
   o << v;
   return o.str();
}

void drawText(float x, float y, const std::string &text,
              float r = 1, float g = 1, float b = 1,
              void *font = GLUT_BITMAP_HELVETICA_18)
{
   glColor3f(r, g, b);
   glRasterPos2f(x, y);
   for (char c : text)
      glutBitmapCharacter(font, c);
}

void getBrickColor(int row, float &r, float &g, float &b)
{
   switch (row)
   {
   case 0:
      r = 1.0f;
      g = 0.2f;
      b = 0.2f;
      break; // Red
   case 1:
      r = 1.0f;
      g = 0.5f;
      b = 0.0f;
      break; // Orange
   case 2:
      r = 1.0f;
      g = 1.0f;
      b = 0.0f;
      break; // Yellow
   case 3:
      r = 0.0f;
      g = 0.8f;
      b = 0.0f;
      break; // Green
   case 4:
      r = 0.2f;
      g = 0.5f;
      b = 1.0f;
      break; // Blue
   case 5:
      r = 0.6f;
      g = 0.0f;
      b = 0.8f;
      break; // Purple
   default:
      r = 1;
      g = 1;
      b = 1;
      break;
   }
}

void initBricks()
{
   bricks.clear();
   for (int row = 0; row < BRICK_ROWS; row++)
   {
      for (int col = 0; col < BRICK_COLS; col++)
      {
         Brick b;
         b.x = BRICK_START_X + col * (BRICK_WIDTH + BRICK_PADDING);
         b.y = BRICK_START_Y - row * (BRICK_HEIGHT + BRICK_PADDING);
         b.active = true;
         b.health = (row < 2) ? 2 : 1;
         getBrickColor(row, b.r, b.g, b.b);
         bricks.push_back(b);
      }
   }
}

// Draw Bricks
void drawBricks()
{
   for (auto &b : bricks)
   {
      if (!b.active)
         continue;
      drawRect(b.x, b.y, BRICK_WIDTH, BRICK_HEIGHT, b.r, b.g, b.b);
      float hr = std::min(b.r + 0.3f, 1.0f);
      float hg = std::min(b.g + 0.3f, 1.0f);
      float hbb = std::min(b.b + 0.3f, 1.0f);
      drawRect(b.x + 2, b.y + BRICK_HEIGHT - 5,
               BRICK_WIDTH - 4, 4, hr, hg, hbb);

      drawRect(b.x, b.y, BRICK_WIDTH, BRICK_HEIGHT,
               0.0f, 0.0f, 0.0f, false);

      if (b.health == 2)
      {
         drawText(b.x + BRICK_WIDTH / 2 - 4, b.y + 6,
                  "2", 1, 1, 1, GLUT_BITMAP_HELVETICA_12);
      }
   }
}

bool checkBrickCollision(Brick &brick)
{
   if (!brick.active)
      return false;

   float bLeft = ball.x - BALL_RADIUS;
   float bRight = ball.x + BALL_RADIUS;
   float bBottom = ball.y - BALL_RADIUS;
   float bTop = ball.y + BALL_RADIUS;
   float brLeft = brick.x;
   float brRight = brick.x + BRICK_WIDTH;
   float brBottom = brick.y;
   float brTop = brick.y + BRICK_HEIGHT;
   if (bRight < brLeft || bLeft > brRight)
      return false;
   if (bTop < brBottom || bBottom > brTop)
      return false;
   float overlapL = bRight - brLeft;
   float overlapR = brRight - bLeft;
   float overlapB = bTop - brBottom;
   float overlapT = brTop - bBottom;
   float minX = std::min(overlapL, overlapR);
   float minY = std::min(overlapB, overlapT);

   if (minX < minY)
      ball.dx = -ball.dx;
   else
      ball.dy = -ball.dy;

   return true;
}

void updateBall()
{
   if (ballOnPaddle)
   {
      ball.x = paddle.x + paddle.width / 2.0f;
      ball.y = PADDLE_Y + PADDLE_HEIGHT + BALL_RADIUS + 1.0f;
      return;
   }

   ball.x += ball.dx;
   ball.y += ball.dy;

   if (ball.x - BALL_RADIUS < 0)
   {
      ball.x = BALL_RADIUS;
      ball.dx = fabsf(ball.dx);
   }
   if (ball.x + BALL_RADIUS > WINDOW_WIDTH)
   {
      ball.x = WINDOW_WIDTH - BALL_RADIUS;
      ball.dx = -fabsf(ball.dx);
   }
   if (ball.y + BALL_RADIUS > WINDOW_HEIGHT)
   {
      ball.y = WINDOW_HEIGHT - BALL_RADIUS;
      ball.dy = -fabsf(ball.dy);
   }

   if (ball.y - BALL_RADIUS <= PADDLE_Y + PADDLE_HEIGHT &&
       ball.y - BALL_RADIUS >= PADDLE_Y - 5.0f &&
       ball.x >= paddle.x && ball.x <= paddle.x + paddle.width &&
       ball.dy < 0)
   {

      ball.dy = fabsf(ball.dy);
      float hitPos = (ball.x - paddle.x) / paddle.width;
      float angle = (hitPos - 0.5f) * 2.0f;
      ball.dx = ball.speed * angle * 0.8f;
      float mag = sqrtf(ball.dx * ball.dx + ball.dy * ball.dy);
      if (mag > 0)
      {
         ball.dx = (ball.dx / mag) * ball.speed;
         ball.dy = (ball.dy / mag) * ball.speed;
      }
      if (fabsf(ball.dy) < 1.5f)
         ball.dy = (ball.dy < 0) ? -1.5f : 1.5f;
   }

   for (auto &brick : bricks)
   {
      if (!brick.active)
         continue;
      if (checkBrickCollision(brick))
      {
         brick.health--;
         if (brick.health <= 0)
         {
            brick.active = false;
            score += 10;
         }
         else
         {
            // Damaged - darken color
            brick.r *= 0.65f;
            brick.g *= 0.65f;
            brick.b *= 0.65f;
            score += 5;
         }
         break;
      }
   }

   if (ball.y - BALL_RADIUS < 0)
   {
      std::cout << "Ball lost! Score: " << score << std::endl;
      ballOnPaddle = true;
      ball.speed = BALL_SPEED_INIT;
   }
}

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

void drawPaddle()
{
   drawRect(paddle.x, paddle.y, paddle.width, paddle.height,
            0.3f, 0.6f, 1.0f);
   drawRect(paddle.x + 2, paddle.y + paddle.height - 4,
            paddle.width - 4, 3, 0.7f, 0.9f, 1.0f);
   drawRect(paddle.x, paddle.y, paddle.width, paddle.height,
            1.0f, 1.0f, 1.0f, false);
}

void drawBall()
{
   drawCircle(ball.x + 2, ball.y - 2, BALL_RADIUS, 0, 0, 0);
   drawCircle(ball.x, ball.y, BALL_RADIUS, 1, 1, 1);
   drawCircle(ball.x - 3, ball.y + 3, BALL_RADIUS * 0.3f,
              0.9f, 0.9f, 1.0f);
}

void display()
{
   glClear(GL_COLOR_BUFFER_BIT);
   glLoadIdentity();

   drawBackground();
   drawBricks();
   drawPaddle();
   drawBall();

   drawText(10, WINDOW_HEIGHT - 25, "Score: " + toStr(score),
            1.0f, 1.0f, 0.0f);

   if (ballOnPaddle)
      drawText(255, 200, "Press SPACE to launch!",
               1.0f, 1.0f, 0.0f);

   glutSwapBuffers();
}

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
   updateBall();
}

void timer(int v)
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

void keyboard(unsigned char key, int x, int y)
{
   if (key == 27)
      exit(0);
   if (key == ' ' && ballOnPaddle)
   {
      ballOnPaddle = false;
      ball.dy = ball.speed * 0.707f;
      ball.dx = ball.speed * 0.707f;
   }
}

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
void mouseMotion(int x, int y)
{
   paddle.x = (float)x - paddle.width / 2.0f;
   if (paddle.x < 0)
      paddle.x = 0;
   if (paddle.x + paddle.width > WINDOW_WIDTH)
      paddle.x = WINDOW_WIDTH - paddle.width;
}
void mouseClick(int button, int state, int x, int y)
{
   if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && ballOnPaddle)
   {
      ballOnPaddle = false;
      ball.dy = ball.speed * 0.707f;
      ball.dx = ball.speed * 0.707f;
   }
}

void init()
{
   paddle.x = WINDOW_WIDTH / 2.0f - PADDLE_WIDTH / 2.0f;
   paddle.y = PADDLE_Y;
   paddle.width = PADDLE_WIDTH;
   paddle.height = PADDLE_HEIGHT;
   ball.speed = BALL_SPEED_INIT;
   ball.active = true;
   ballOnPaddle = true;
   ball.x = paddle.x + paddle.width / 2.0f;
   ball.y = PADDLE_Y + PADDLE_HEIGHT + BALL_RADIUS + 1.0f;
   ball.dx = ball.dy = 0;

   initBricks();
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
   glutMouseFunc(mouseClick);
   glutTimerFunc(16, timer, 0);

   glutMainLoop();
   return 0;
}