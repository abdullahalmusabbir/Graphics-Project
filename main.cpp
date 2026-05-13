#include <GL/glut.h>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float PADDLE_WIDTH_DEFAULT = 100.0f;
const float PADDLE_HEIGHT = 15.0f;
const float PADDLE_Y = 30.0f;
const float PADDLE_SPEED = 8.0f;
const float BALL_RADIUS = 10.0f;
const float BALL_SPEED_INIT = 4.5f;
const float BALL_SPEED_INC = 0.0004f;
const int BRICK_ROWS = 6;
const int BRICK_COLS = 10;
const float BRICK_WIDTH = 70.0f;
const float BRICK_HEIGHT = 25.0f;
const float BRICK_PADDING = 5.0f;
const float BRICK_START_X = 25.0f;
const float BRICK_START_Y = 430.0f;

// Perk constants
const float PERK_WIDTH = 22.0f;
const float PERK_HEIGHT = 22.0f;
const float PERK_SPEED = 2.5f;
const float WIDER_PADDLE_DURATION = 10.0f;

// Enums
enum GameState
{
   MENU,
   PLAYING,
   PAUSED,
   GAME_OVER,
   WIN
};
enum PerkType
{
   PERK_NONE,
   PERK_EXTRA_LIFE,
   PERK_FASTER_BALL,
   PERK_WIDER_PADDLE
};

// Structures
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
   int health;
   float r, g, b;
   PerkType perk; // which perk this brick holds
};

// NEW: Perk drop structure
struct PerkDrop
{
   float x, y;
   float dy; // falling speed (negative = down)
   PerkType type;
   bool active;
   float r, g, b; // display color
};

GameState gameState = MENU;
Paddle paddle;
Ball ball;
std::vector<Brick> bricks;
std::vector<PerkDrop> perks; // active falling perks
bool ballOnPaddle = true;
bool keyLeft = false, keyRight = false;
int score = 0, lives = 3;
float gameTime = 0.0f;
int selectedMenu = 0;
bool showHelp = false;
float paddleWidth = PADDLE_WIDTH_DEFAULT;
bool widerPaddleActive = false;
float widerPaddleTimer = 0.0f;

// All utility functions
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
void drawCircle(float cx, float cy, float rad, float r, float g, float b)
{
   glColor3f(r, g, b);
   glBegin(GL_POLYGON);
   for (int i = 0; i < 36; i++)
   {
      float a = 2 * 3.14159f * i / 36;
      glVertex2f(cx + rad * cosf(a), cy + rad * sinf(a));
   }
   glEnd();
}
std::string toStr(int v)
{
   std::ostringstream o;
   o << v;
   return o.str();
}
std::string toStr(float v, int p = 1)
{
   std::ostringstream o;
   o.precision(p);
   o << std::fixed << v;
   return o.str();
}
void drawText(float x, float y, const std::string &s,
              float r = 1, float g = 1, float b = 1,
              void *font = GLUT_BITMAP_HELVETICA_18)
{
   glColor3f(r, g, b);
   glRasterPos2f(x, y);
   for (char c : s)
      glutBitmapCharacter(font, c);
}
void drawTextLarge(float x, float y, const std::string &s,
                   float r = 1, float g = 1, float b = 1)
{
   glColor3f(r, g, b);
   glRasterPos2f(x, y);
   for (char c : s)
      glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
}

// Perk Helper Functions
PerkType randomPerk()
{
   // 40% chance no perk, 20% each for 3 perks
   int r = rand() % 5;
   if (r == 0)
      return PERK_EXTRA_LIFE;
   if (r == 1)
      return PERK_FASTER_BALL;
   if (r == 2)
      return PERK_WIDER_PADDLE;
   return PERK_NONE;
}

void getPerkColor(PerkType type, float &r, float &g, float &b)
{
   switch (type)
   {
   case PERK_EXTRA_LIFE:
      r = 0.0f;
      g = 1.0f;
      b = 0.2f;
      break; // Green
   case PERK_FASTER_BALL:
      r = 1.0f;
      g = 0.1f;
      b = 0.1f;
      break; // Red
   case PERK_WIDER_PADDLE:
      r = 0.1f;
      g = 0.5f;
      b = 1.0f;
      break; // Blue
   default:
      r = g = b = 1;
      break;
   }
}

std::string getPerkLabel(PerkType type)
{
   switch (type)
   {
   case PERK_EXTRA_LIFE:
      return "L";
   case PERK_FASTER_BALL:
      return "F";
   case PERK_WIDER_PADDLE:
      return "W";
   default:
      return "?";
   }
}

// Spawn a perk drop at given position
void spawnPerk(float x, float y, PerkType type)
{
   if (type == PERK_NONE)
      return;
   PerkDrop p;
   p.x = x - PERK_WIDTH / 2.0f;
   p.y = y;
   p.dy = -PERK_SPEED; // fall downward
   p.type = type;
   p.active = true;
   getPerkColor(type, p.r, p.g, p.b);
   perks.push_back(p);
}

// Apply perk effect to game
void applyPerk(PerkType type)
{
   switch (type)
   {
   case PERK_EXTRA_LIFE:
      lives++;
      break;
   case PERK_FASTER_BALL:
      ball.speed += 1.5f;
      { // Normalize velocity to new speed
         float mag = sqrtf(ball.dx * ball.dx + ball.dy * ball.dy);
         if (mag > 0)
         {
            ball.dx = (ball.dx / mag) * ball.speed;
            ball.dy = (ball.dy / mag) * ball.speed;
         }
      }
      break;
   case PERK_WIDER_PADDLE:
      widerPaddleActive = true;
      widerPaddleTimer = WIDER_PADDLE_DURATION;
      paddle.width = PADDLE_WIDTH_DEFAULT * 1.7f;
      break;
   default:
      break;
   }
}

// Update all falling perks
void updatePerks(float dt)
{
   // Wider paddle timer countdown
   if (widerPaddleActive)
   {
      widerPaddleTimer -= dt;
      if (widerPaddleTimer <= 0)
      {
         widerPaddleActive = false;
         paddle.width = PADDLE_WIDTH_DEFAULT;
      }
   }

   for (auto &p : perks)
   {
      if (!p.active)
         continue;

      p.y += p.dy;

      // Check paddle collision
      bool caught = (p.y <= PADDLE_Y + PADDLE_HEIGHT) &&
                    (p.y >= PADDLE_Y - PERK_HEIGHT) &&
                    (p.x + PERK_WIDTH >= paddle.x) &&
                    (p.x <= paddle.x + paddle.width);

      if (caught)
      {
         p.active = false;
         applyPerk(p.type);
      }

      // Fell off screen
      if (p.y < -PERK_HEIGHT)
      {
         p.active = false;
      }
   }
}

// Draw falling perk drops
void drawPerks()
{
   for (auto &p : perks)
   {
      if (!p.active)
         continue;

      // Perk box
      drawRect(p.x, p.y, PERK_WIDTH, PERK_HEIGHT,
               p.r, p.g, p.b);
      // White border
      drawRect(p.x, p.y, PERK_WIDTH, PERK_HEIGHT,
               1, 1, 1, false);
      // Label
      drawText(p.x + 6, p.y + 5, getPerkLabel(p.type),
               1, 1, 1, GLUT_BITMAP_HELVETICA_12);
   }
}

// Brick init with perk assignment
void getBrickColor(int row, float &r, float &g, float &b)
{
   switch (row)
   {
   case 0:
      r = 1;
      g = 0.2f;
      b = 0.2f;
      break;
   case 1:
      r = 1;
      g = 0.5f;
      b = 0;
      break;
   case 2:
      r = 1;
      g = 1;
      b = 0;
      break;
   case 3:
      r = 0;
      g = 0.8f;
      b = 0;
      break;
   case 4:
      r = 0.2f;
      g = 0.5f;
      b = 1;
      break;
   case 5:
      r = 0.6f;
      g = 0;
      b = 0.8f;
      break;
   default:
      r = g = b = 1;
   }
}

void initBricks()
{
   bricks.clear();
   for (int row = 0; row < BRICK_ROWS; row++)
      for (int col = 0; col < BRICK_COLS; col++)
      {
         Brick b;
         b.x = BRICK_START_X + col * (BRICK_WIDTH + BRICK_PADDING);
         b.y = BRICK_START_Y - row * (BRICK_HEIGHT + BRICK_PADDING);
         b.active = true;
         b.health = (row < 2) ? 2 : 1;
         getBrickColor(row, b.r, b.g, b.b);
         b.perk = randomPerk(); // Assign random perk
         bricks.push_back(b);
      }
}

// Draw bricks with perk indicator dots
void drawBricks()
{
   for (auto &b : bricks)
   {
      if (!b.active)
         continue;
      drawRect(b.x, b.y, BRICK_WIDTH, BRICK_HEIGHT, b.r, b.g, b.b);
      drawRect(b.x + 2, b.y + BRICK_HEIGHT - 5, BRICK_WIDTH - 4, 4,
               std::min(b.r + 0.3f, 1.0f), std::min(b.g + 0.3f, 1.0f),
               std::min(b.b + 0.3f, 1.0f));
      drawRect(b.x, b.y, BRICK_WIDTH, BRICK_HEIGHT, 0, 0, 0, false);

      // Show perk indicator on brick
      if (b.perk != PERK_NONE)
      {
         float pr, pg, pb;
         getPerkColor(b.perk, pr, pg, pb);
         drawCircle(b.x + BRICK_WIDTH / 2,
                    b.y + BRICK_HEIGHT / 2,
                    4, pr, pg, pb);
      }
   }
}

void initBall()
{
   ball.speed = BALL_SPEED_INIT;
   ball.active = true;
   ballOnPaddle = true;
   ball.x = paddle.x + paddle.width / 2;
   ball.y = PADDLE_Y + PADDLE_HEIGHT + BALL_RADIUS + 1;
   ball.dx = ball.dy = 0;
}

void initGame()
{
   lives = 3;
   score = 0;
   gameTime = 0;
   paddleWidth = PADDLE_WIDTH_DEFAULT;
   widerPaddleActive = false;
   widerPaddleTimer = 0;
   perks.clear();
   gameState = PLAYING;
   paddle.x = WINDOW_WIDTH / 2 - PADDLE_WIDTH_DEFAULT / 2;
   paddle.y = PADDLE_Y;
   paddle.width = PADDLE_WIDTH_DEFAULT;
   paddle.height = PADDLE_HEIGHT;
   initBricks();
   initBall();
}

bool checkBrickCollision(Brick &brick)
{
   if (!brick.active)
      return false;
   float bL = ball.x - BALL_RADIUS, bR = ball.x + BALL_RADIUS;
   float bB = ball.y - BALL_RADIUS, bT = ball.y + BALL_RADIUS;
   float brL = brick.x, brR = brick.x + BRICK_WIDTH;
   float brB = brick.y, brT = brick.y + BRICK_HEIGHT;
   if (bR < brL || bL > brR || bT < brB || bB > brT)
      return false;
   float oL = bR - brL, oR = brR - bL, oB = bT - brB, oT = brT - bB;
   if (std::min(oL, oR) < std::min(oB, oT))
      ball.dx = -ball.dx;
   else
      ball.dy = -ball.dy;
   return true;
}

void drawBackground()
{
   glBegin(GL_QUADS);
   glColor3f(0, 0, 0.15f);
   glVertex2f(0, 0);
   glVertex2f(WINDOW_WIDTH, 0);
   glColor3f(0, 0, 0.35f);
   glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
   glVertex2f(0, WINDOW_HEIGHT);
   glEnd();
}
void drawPaddle()
{
   drawRect(paddle.x, paddle.y, paddle.width, paddle.height, 0.3f, 0.6f, 1);
   drawRect(paddle.x + 2, paddle.y + paddle.height - 4, paddle.width - 4, 3,
            0.7f, 0.9f, 1);
   drawRect(paddle.x, paddle.y, paddle.width, paddle.height, 1, 1, 1, false);
   // Show timer if wider paddle active
   if (widerPaddleActive)
   {
      drawText(paddle.x + paddle.width / 2 - 15, paddle.y - 15,
               toStr(widerPaddleTimer) + "s", 0, 0.8f, 1,
               GLUT_BITMAP_HELVETICA_12);
   }
}
void drawBall()
{
   drawCircle(ball.x + 2, ball.y - 2, BALL_RADIUS, 0, 0, 0);
   drawCircle(ball.x, ball.y, BALL_RADIUS, 1, 1, 1);
   drawCircle(ball.x - 3, ball.y + 3, BALL_RADIUS * 0.3f, 0.9f, 0.9f, 1);
}
void drawHUD()
{
   drawRect(0, WINDOW_HEIGHT - 40, WINDOW_WIDTH, 40, 0, 0, 0.2f);
   drawRect(0, WINDOW_HEIGHT - 42, WINDOW_WIDTH, 2, 0.3f, 0.6f, 1);
   drawText(10, WINDOW_HEIGHT - 25, "Lives:", 0.8f, 0.8f, 1);
   for (int i = 0; i < lives; i++)
      drawCircle(80 + i * 22, WINDOW_HEIGHT - 20, 8, 1, 0.3f, 0.3f);
   drawText(200, WINDOW_HEIGHT - 25, "Score: " + toStr(score), 1, 1, 0);
   drawText(380, WINDOW_HEIGHT - 25, "Time: " + toStr(gameTime) + "s", 0.5f, 1, 0.5f);
   drawText(540, WINDOW_HEIGHT - 25, "Speed: " + toStr(ball.speed), 1, 0.5f, 0);
   // Perk legend bottom
   drawText(10, 8, "Perks: [L]=Extra Life  [F]=Fast Ball  [W]=Wide Paddle",
            0.5f, 0.5f, 0.7f, GLUT_BITMAP_HELVETICA_12);
}

void drawGameOver()
{
   glColor4f(0, 0, 0, 0.75f);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glBegin(GL_QUADS);
   glVertex2f(0, 0);
   glVertex2f(WINDOW_WIDTH, 0);
   glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
   glVertex2f(0, WINDOW_HEIGHT);
   glEnd();
   glDisable(GL_BLEND);
   drawRect(200, 200, 400, 200, 0.1f, 0, 0);
   drawRect(200, 200, 400, 200, 1, 0, 0, false);
   drawTextLarge(280, 365, "GAME OVER", 1, 0.2f, 0.2f);
   drawText(280, 320, "Score: " + toStr(score), 1, 1, 0.5f);
   drawText(280, 290, "Time:  " + toStr(gameTime) + "s", 0.8f, 0.8f, 0.8f);
   drawText(255, 250, "ENTER - Play Again", 0.9f, 0.9f, 0.9f);
   drawText(270, 225, "ESC   - Main Menu", 0.7f, 0.7f, 0.7f);
}
void drawWin()
{
   glColor4f(0, 0.05f, 0, 0.7f);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glBegin(GL_QUADS);
   glVertex2f(0, 0);
   glVertex2f(WINDOW_WIDTH, 0);
   glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
   glVertex2f(0, WINDOW_HEIGHT);
   glEnd();
   glDisable(GL_BLEND);
   drawRect(180, 190, 440, 220, 0, 0.1f, 0);
   drawRect(180, 190, 440, 220, 0, 1, 0.4f, false);
   drawTextLarge(285, 375, "YOU WIN!", 0, 1, 0.4f);
   drawText(260, 335, "All bricks cleared!", 0.9f, 1, 0.9f);
   drawText(280, 305, "Score: " + toStr(score), 1, 1, 0.5f);
   drawText(280, 275, "Time: " + toStr(gameTime) + "s", 0.8f, 0.8f, 0.8f);
   drawText(260, 240, "ENTER - Play Again", 0.9f, 0.9f, 0.9f);
   drawText(270, 215, "ESC   - Main Menu", 0.7f, 0.7f, 0.7f);
}
void drawMenu()
{
   glBegin(GL_QUADS);
   glColor3f(0, 0, 0.15f);
   glVertex2f(0, 0);
   glVertex2f(WINDOW_WIDTH, 0);
   glColor3f(0, 0, 0.35f);
   glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
   glVertex2f(0, WINDOW_HEIGHT);
   glEnd();
   drawTextLarge(220, 490, "DX BALL", 0, 0.8f, 1);
   drawText(240, 450, "CSE 426 - Computer Graphics Lab",
            0.6f, 0.6f, 0.8f, GLUT_BITMAP_HELVETICA_12);
   drawRect(100, 430, 600, 2, 0.3f, 0.5f, 0.8f);
   std::vector<std::string> items = {"  START GAME  ", "  HOW TO PLAY  ", "  EXIT  "};
   for (int i = 0; i < 3; i++)
   {
      float bx = 280, by = 340 - i * 65, bw = 240, bh = 45;
      if (selectedMenu == i)
      {
         drawRect(bx, by, bw, bh, 0, 0.4f, 0.9f);
         drawRect(bx, by, bw, bh, 0, 0.9f, 1, false);
         drawTextLarge(bx + 30, by + 12, items[i], 1, 1, 1);
      }
      else
      {
         drawRect(bx, by, bw, bh, 0.05f, 0.05f, 0.2f);
         drawRect(bx, by, bw, bh, 0.2f, 0.2f, 0.5f, false);
         drawTextLarge(bx + 30, by + 12, items[i], 0.6f, 0.6f, 0.8f);
      }
   }
   drawText(200, 60, "UP/DOWN arrows | ENTER to select",
            0.4f, 0.4f, 0.6f, GLUT_BITMAP_HELVETICA_12);
}
void drawHowToPlay()
{
   glBegin(GL_QUADS);
   glColor3f(0, 0, 0.15f);
   glVertex2f(0, 0);
   glVertex2f(WINDOW_WIDTH, 0);
   glColor3f(0, 0, 0.3f);
   glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
   glVertex2f(0, WINDOW_HEIGHT);
   glEnd();
   drawTextLarge(265, 545, "HOW TO PLAY", 0, 0.8f, 1);
   std::vector<std::string> lines = {
       "LEFT/RIGHT Arrow  -  Move Paddle",
       "Mouse Movement    -  Move Paddle",
       "SPACE / Click     -  Launch Ball",
       "P                 -  Pause/Resume",
       "ESC               -  Return to Menu",
       "",
       "Break all bricks to win!",
       "Catch falling perks for power-ups:",
       "  GREEN [L] = Extra Life",
       "  RED   [F] = Faster Ball",
       "  BLUE  [W] = Wider Paddle (10s)"};
   for (int i = 0; i < (int)lines.size(); i++)
      drawText(150, 490 - i * 32, lines[i], 0.85f, 0.9f, 1);
   drawText(250, 40, "Press ESC to go back", 0.5f, 0.7f, 1,
            GLUT_BITMAP_HELVETICA_12);
}
void drawPauseOverlay()
{
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glColor4f(0, 0, 0, 0.65f);
   glBegin(GL_QUADS);
   glVertex2f(0, 0);
   glVertex2f(WINDOW_WIDTH, 0);
   glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
   glVertex2f(0, WINDOW_HEIGHT);
   glEnd();
   glDisable(GL_BLEND);
   drawRect(250, 220, 300, 160, 0, 0, 0.3f);
   drawRect(250, 220, 300, 160, 0, 0.8f, 1, false);
   drawTextLarge(333, 345, "PAUSED", 1, 1, 0);
   drawText(290, 305, "P   - Resume Game", 0.9f, 0.9f, 0.9f);
   drawText(290, 278, "ESC - Main Menu", 0.9f, 0.9f, 0.9f);
}

void updateBall(float dt)
{
   if (ballOnPaddle)
   {
      ball.x = paddle.x + paddle.width / 2;
      ball.y = PADDLE_Y + PADDLE_HEIGHT + BALL_RADIUS + 1;
      return;
   }
   ball.speed += BALL_SPEED_INC;
   float mag = sqrtf(ball.dx * ball.dx + ball.dy * ball.dy);
   if (mag > 0)
   {
      ball.dx = (ball.dx / mag) * ball.speed;
      ball.dy = (ball.dy / mag) * ball.speed;
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
       ball.y - BALL_RADIUS >= PADDLE_Y - 5 &&
       ball.x >= paddle.x && ball.x <= paddle.x + paddle.width && ball.dy < 0)
   {
      ball.dy = fabsf(ball.dy);
      float hp = (ball.x - paddle.x) / paddle.width;
      ball.dx = ball.speed * (hp - 0.5f) * 2 * 0.8f;
      float m = sqrtf(ball.dx * ball.dx + ball.dy * ball.dy);
      if (m > 0)
      {
         ball.dx = (ball.dx / m) * ball.speed;
         ball.dy = (ball.dy / m) * ball.speed;
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
            // Spawn perk at brick center
            spawnPerk(brick.x + BRICK_WIDTH / 2,
                      brick.y + BRICK_HEIGHT / 2,
                      brick.perk);
         }
         else
         {
            brick.r *= 0.65f;
            brick.g *= 0.65f;
            brick.b *= 0.65f;
            score += 5;
         }
         break;
      }
   }
   int ab = 0;
   for (auto &b : bricks)
      if (b.active)
         ab++;
   if (ab == 0)
   {
      gameState = WIN;
      return;
   }
   if (ball.y - BALL_RADIUS < 0)
   {
      lives--;
      if (lives <= 0)
         gameState = GAME_OVER;
      else
         initBall();
   }
}

void update(float dt)
{
   if (gameState != PLAYING)
      return;
   gameTime += dt;
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
   updateBall(dt);
   updatePerks(dt); // Update perk drops
}

void display()
{
   glClear(GL_COLOR_BUFFER_BIT);
   glLoadIdentity();
   if (showHelp)
   {
      drawHowToPlay();
      glutSwapBuffers();
      return;
   }
   switch (gameState)
   {
   case MENU:
      drawMenu();
      break;
   case PLAYING:
      drawBackground();
      drawBricks();
      drawPerks();
      drawPaddle();
      drawBall();
      drawHUD();
      if (ballOnPaddle)
         drawText(255, 200, "Press SPACE to launch!", 1, 1, 0);
      break;
   case PAUSED:
      drawBackground();
      drawBricks();
      drawPerks();
      drawPaddle();
      drawBall();
      drawHUD();
      drawPauseOverlay();
      break;
   case GAME_OVER:
      drawBackground();
      drawBricks();
      drawPaddle();
      drawHUD();
      drawGameOver();
      break;
   case WIN:
      drawBackground();
      drawHUD();
      drawWin();
      break;
   }
   glutSwapBuffers();
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
   if (showHelp)
   {
      if (key == 27 || key == 8)
         showHelp = false;
      return;
   }
   switch (gameState)
   {
   case MENU:
      if (key == 13)
      {
         if (selectedMenu == 0)
            initGame();
         else if (selectedMenu == 1)
            showHelp = true;
         else
            exit(0);
      }
      break;
   case PLAYING:
      if (key == ' ' && ballOnPaddle)
      {
         ballOnPaddle = false;
         ball.dy = ball.speed * 0.707f;
         ball.dx = ball.speed * 0.707f;
      }
      if (key == 'p' || key == 'P')
         gameState = PAUSED;
      if (key == 27)
         gameState = MENU;
      break;
   case PAUSED:
      if (key == 'p' || key == 'P')
         gameState = PLAYING;
      if (key == 27)
         gameState = MENU;
      break;
   case GAME_OVER:
   case WIN:
      if (key == 13)
         initGame();
      if (key == 27)
         gameState = MENU;
      break;
   }
}
void specialKeys(int key, int x, int y)
{
   if (gameState == MENU || showHelp)
   {
      if (key == GLUT_KEY_UP)
         selectedMenu = (selectedMenu - 1 + 3) % 3;
      if (key == GLUT_KEY_DOWN)
         selectedMenu = (selectedMenu + 1) % 3;
      return;
   }
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
   if (gameState != PLAYING)
      return;
   paddle.x = (float)x - paddle.width / 2;
   if (paddle.x < 0)
      paddle.x = 0;
   if (paddle.x + paddle.width > WINDOW_WIDTH)
      paddle.x = WINDOW_WIDTH - paddle.width;
}
void mouseClick(int button, int state, int x, int y)
{
   if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && ballOnPaddle && gameState == PLAYING)
   {
      ballOnPaddle = false;
      ball.dy = ball.speed * 0.707f;
      ball.dx = ball.speed * 0.707f;
   }
}

int main(int argc, char **argv)
{
   srand((unsigned)time(0)); // Seed for random perks
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
   glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
   glutInitWindowPosition(100, 50);
   glutCreateWindow("DX Ball - CSE 426");
   glClearColor(0, 0, 0.1f, 1);
   paddle.x = WINDOW_WIDTH / 2 - PADDLE_WIDTH_DEFAULT / 2;
   paddle.y = PADDLE_Y;
   paddle.width = PADDLE_WIDTH_DEFAULT;
   paddle.height = PADDLE_HEIGHT;
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