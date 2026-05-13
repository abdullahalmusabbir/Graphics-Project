#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <algorithm>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int BRICK_ROWS = 6;
const int BRICK_COLS = 10;
const float BRICK_WIDTH = 70.0f;
const float BRICK_HEIGHT = 25.0f;
const float BRICK_PADDING = 5.0f;
const float BRICK_START_X = 25.0f;
const float BRICK_START_Y = 420.0f;
const float PADDLE_WIDTH_DEFAULT = 100.0f;
const float PADDLE_HEIGHT = 15.0f;
const float PADDLE_Y = 30.0f;
const float PADDLE_SPEED = 8.0f;
const float BALL_RADIUS = 10.0f;
const float BALL_SPEED_INITIAL = 4.0f;
const float BALL_SPEED_INCREMENT = 0.0005f;
const float PERK_WIDTH = 20.0f;
const float PERK_HEIGHT = 20.0f;
const float PERK_SPEED = 2.5f;
const float BULLET_WIDTH = 5.0f;
const float BULLET_HEIGHT = 12.0f;
const float BULLET_SPEED = 8.0f;
const int MAX_LEVELS = 3;

enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, WIN, HELP };
enum PerkType {
    PERK_EXTRA_LIFE,
    PERK_FASTER_BALL,
    PERK_WIDER_PADDLE,
    PERK_FIREBALL,
    PERK_DEATH,
    PERK_SMALLER_PADDLE,
    PERK_SHOOT,
    PERK_NONE
};

struct Brick {
    float x, y;
    bool active;
    int health;
    float r, g, b;
    PerkType perk;
    bool isWall;
};

struct Ball {
    float x, y;
    float dx, dy;
    float speed;
    bool active;
    bool isFireball;
    float fireTimer;
};

struct Paddle {
    float x, y;
    float width;
};

struct Perk {
    float x, y;
    float dy;
    PerkType type;
    bool active;
    float r, g, b;
};

struct Bullet {
    float x, y;
    bool active;
};

GameState gameState = MENU;
GameState prevState = MENU;
Ball ball;
Paddle paddle;
std::vector<Brick> bricks;
std::vector<Perk> perks;
std::vector<Bullet> bullets;

int lives = 3;
int score = 0;
float gameTime = 0.0f;
bool ballOnPaddle = true;
bool keyLeft = false;
bool keyRight = false;
int selectedMenu = 0;
int selectedPauseMenu = 0;
float paddleWidth = PADDLE_WIDTH_DEFAULT;
bool widerPaddleActive = false;
float widerPaddleTimer = 0.0f;
bool smallerPaddleActive = false;
float smallerPaddleTimer = 0.0f;
bool shootActive = false;
float shootTimer = 0.0f;
float bulletCooldown = 0.0f;
bool fireballActive = false;
float fireballTimer = 0.0f;
int currentLevel = 1;

float flashTimer = 0.0f;
float flashR = 1.0f, flashG = 1.0f, flashB = 1.0f;

void drawText(float x, float y, const std::string& text,
    float r = 1.0f, float g = 1.0f, float b = 1.0f,
    void* font = GLUT_BITMAP_HELVETICA_18) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(font, c);
}

void drawTextLarge(float x, float y, const std::string& text,
    float r = 1.0f, float g = 1.0f, float b = 1.0f) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
}

std::string intToString(int val) {
    std::ostringstream oss; oss << val; return oss.str();
}

std::string floatToString(float val, int decimals = 1) {
    std::ostringstream oss;
    oss.precision(decimals);
    oss << std::fixed << val;
    return oss.str();
}

void drawRect(float x, float y, float w, float h,
    float r, float g, float b, bool filled = true) {
    glColor3f(r, g, b);
    glBegin(filled ? GL_QUADS : GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float radius,
    float r, float g, float b, bool filled = true) {
    glColor3f(r, g, b);
    glBegin(filled ? GL_POLYGON : GL_LINE_LOOP);
    int segments = 32;
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * 3.14159265f * i / segments;
        glVertex2f(cx + radius * cos(angle), cy + radius * sin(angle));
    }
    glEnd();
}

void draw_pixel(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void drawLine(int x1, int y1, int x2, int y2) {
    int dx, dy, i, e;
    int incx, incy, inc1, inc2;
    int x, y;

    dx = x2 - x1;
    dy = y2 - y1;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    incx = 1;
    if (x2 < x1) incx = -1;
    incy = 1;
    if (y2 < y1) incy = -1;

    x = x1; y = y1;

    if (dx > dy) {
        draw_pixel(x, y);
        e = 2 * dy - dx;
        inc1 = 2 * (dy - dx);
        inc2 = 2 * dy;
        for (i = 0; i < dx; i++) {
            if (e >= 0) {
                y += incy;
                e += inc1;
            } else {
                e += inc2;
            }
            x += incx;
            draw_pixel(x, y);
        }
    } else {
        draw_pixel(x, y);
        e = 2 * dx - dy;
        inc1 = 2 * (dx - dy);
        inc2 = 2 * dx;
        for (i = 0; i < dy; i++) {
            if (e >= 0) {
                x += incx;
                e += inc1;
            } else {
                e += inc2;
            }
            y += incy;
            draw_pixel(x, y);
        }
    }
}

void drawBresenhamRect(float fx, float fy, float fw, float fh,
    float r, float g, float b) {
    int x1 = (int)fx;
    int y1 = (int)fy;
    int x2 = (int)(fx + fw);
    int y2 = (int)(fy + fh);

    glColor3f(r, g, b);
    glPointSize(1.0f);

    for (int row = y1; row <= y2; row++) {
        drawLine(x1, row, x2, row);
    }
}

void drawBresenhamRectOutline(float fx, float fy, float fw, float fh,
    float r, float g, float b) {
    int x1 = (int)fx;
    int y1 = (int)fy;
    int x2 = (int)(fx + fw);
    int y2 = (int)(fy + fh);

    glColor3f(r, g, b);
    glPointSize(1.0f);

    drawLine(x1, y1, x2, y1); 
    drawLine(x2, y1, x2, y2); 
    drawLine(x2, y2, x1, y2); 
    drawLine(x1, y2, x1, y1); 
}

int g_cx, g_cy;

void plotCirclePoints(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(g_cx + x, g_cy + y);
    glVertex2i(g_cx - x, g_cy + y);
    glVertex2i(g_cx + x, g_cy - y);
    glVertex2i(g_cx - x, g_cy - y);
    glVertex2i(g_cx + y, g_cy + x);
    glVertex2i(g_cx - y, g_cy + x);
    glVertex2i(g_cx + y, g_cy - x);
    glVertex2i(g_cx - y, g_cy - x);
    glEnd();
}

void midpointCircleFilled(int cx, int cy, int r) {
    g_cx = cx;
    g_cy = cy;

    int x = 0;
    int y = r;
    int d = 1 - r;

    while (x <= y) {
        glBegin(GL_LINES);
        glVertex2i(cx - y, cy + x);
        glVertex2i(cx + y, cy + x);

        glVertex2i(cx - y, cy - x);
        glVertex2i(cx + y, cy - x);

        glVertex2i(cx - x, cy + y);
        glVertex2i(cx + x, cy + y);

        glVertex2i(cx - x, cy - y);
        glVertex2i(cx + x, cy - y);
        glEnd();

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void midpointCircleOutline(int cx, int cy, int r) {
    g_cx = cx;
    g_cy = cy;

    int x = 0;
    int y = r;
    int d = 1 - r;

    glPointSize(1.0f);
    while (x <= y) {
        plotCirclePoints(x, y);

        if (d < 0) {
            d += 2 * x + 3;
        } else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

PerkType randomPerk() {
    int r = rand() % 10;
    if (r == 0) return PERK_EXTRA_LIFE;
    if (r == 1) return PERK_FASTER_BALL;
    if (r == 2) return PERK_WIDER_PADDLE;
    if (r == 3) return PERK_FIREBALL;
    if (r == 4) return PERK_DEATH;
    if (r == 5) return PERK_SMALLER_PADDLE;
    if (r == 6) return PERK_SHOOT;
    return PERK_NONE;
}

void getBrickColor(int row, float& r, float& g, float& b) {
    switch (row) {
    case 0: r = 1.0f; g = 0.2f; b = 0.2f; break;
    case 1: r = 1.0f; g = 0.5f; b = 0.0f; break;
    case 2: r = 1.0f; g = 1.0f; b = 0.0f; break;
    case 3: r = 0.0f; g = 0.8f; b = 0.0f; break;
    case 4: r = 0.0f; g = 0.5f; b = 1.0f; break;
    case 5: r = 0.6f; g = 0.0f; b = 0.8f; break;
    default: r = 1.0f; g = 1.0f; b = 1.0f; break;
    }
}

void initBricks() {
    bricks.clear();
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            Brick b;
            b.x = BRICK_START_X + col * (BRICK_WIDTH + BRICK_PADDING);
            b.y = BRICK_START_Y - row * (BRICK_HEIGHT + BRICK_PADDING);
            b.active = true;
            b.isWall = false;

            if (currentLevel == 1) {
                b.health = (row < 2) ? 2 : 1;
                getBrickColor(row, b.r, b.g, b.b);
                b.isWall = false;
            } else if (currentLevel == 2) {
                if ((row + col) % 4 == 0) {
                    b.health = 3;
                    b.r = 0.5f; b.g = 0.5f; b.b = 0.5f;
                    b.isWall = true;
                } else {
                    b.health = (row < 2) ? 2 : 1;
                    getBrickColor(row, b.r, b.g, b.b);
                    b.isWall = false;
                }
            } else if (currentLevel >= 3) {
                bool isWallBrick = (col % 3 == 0 && row % 2 == 0) ||
                    (col % 3 == 1 && row % 2 == 1);
                if (isWallBrick) {
                    b.health = 4;
                    b.r = 0.45f; b.g = 0.3f; b.b = 0.2f;
                    b.isWall = true;
                } else {
                    b.health = (row < 3) ? 2 : 1;
                    getBrickColor(row, b.r, b.g, b.b);
                    b.isWall = false;
                }
            }

            b.perk = randomPerk();
            bricks.push_back(b);
        }
    }
}

void initBall() {
    ball.x = paddle.x + paddle.width / 2.0f;
    ball.y = PADDLE_Y + PADDLE_HEIGHT + BALL_RADIUS + 1.0f;
    ball.speed = BALL_SPEED_INITIAL + (currentLevel - 1) * 0.5f;
    ball.dx = ball.speed * 0.7f;
    ball.dy = ball.speed * 0.7f;
    ball.active = true;
    ball.isFireball = false;
    ball.fireTimer = 0.0f;
    ballOnPaddle = true;
}

void initGame() {
    lives = 3;
    score = 0;
    gameTime = 0.0f;
    currentLevel = 1;
    paddleWidth = PADDLE_WIDTH_DEFAULT;
    widerPaddleActive = false;
    widerPaddleTimer = 0.0f;
    smallerPaddleActive = false;
    smallerPaddleTimer = 0.0f;
    shootActive = false;
    shootTimer = 0.0f;
    bulletCooldown = 0.0f;
    fireballActive = false;
    fireballTimer = 0.0f;
    flashTimer = 0.0f;
    perks.clear();
    bullets.clear();

    paddle.x = WINDOW_WIDTH / 2.0f - paddleWidth / 2.0f;
    paddle.y = PADDLE_Y;
    paddle.width = paddleWidth;

    initBricks();
    initBall();
    gameState = PLAYING;
    prevState = PLAYING;
}

void nextLevel() {
    currentLevel++;
    paddleWidth = PADDLE_WIDTH_DEFAULT;
    widerPaddleActive = false;
    widerPaddleTimer = 0.0f;
    smallerPaddleActive = false;
    smallerPaddleTimer = 0.0f;
    shootActive = false;
    shootTimer = 0.0f;
    bulletCooldown = 0.0f;
    fireballActive = false;
    fireballTimer = 0.0f;
    perks.clear();
    bullets.clear();
    paddle.x = WINDOW_WIDTH / 2.0f - paddleWidth / 2.0f;
    paddle.width = paddleWidth;
    initBricks();
    initBall();
    gameState = PLAYING;
}

void spawnPerk(float x, float y, PerkType type) {
    if (type == PERK_NONE) return;
    Perk p;
    p.x = x; p.y = y;
    p.dy = -PERK_SPEED;
    p.type = type;
    p.active = true;
    if (type == PERK_EXTRA_LIFE)      { p.r = 0.0f; p.g = 1.0f; p.b = 0.0f; }
    else if (type == PERK_FASTER_BALL) { p.r = 1.0f; p.g = 0.3f; p.b = 0.0f; }
    else if (type == PERK_WIDER_PADDLE){ p.r = 0.0f; p.g = 0.5f; p.b = 1.0f; }
    else if (type == PERK_FIREBALL)    { p.r = 1.0f; p.g = 0.4f; p.b = 0.0f; }
    else if (type == PERK_DEATH)       { p.r = 0.8f; p.g = 0.0f; p.b = 0.8f; }
    else if (type == PERK_SMALLER_PADDLE){ p.r = 1.0f; p.g = 0.0f; p.b = 0.5f; }
    else if (type == PERK_SHOOT)       { p.r = 1.0f; p.g = 1.0f; p.b = 0.0f; }
    perks.push_back(p);
}

void applyPerk(PerkType type) {
    if (type == PERK_EXTRA_LIFE) {
        lives++;
    } else if (type == PERK_FASTER_BALL) {
        ball.speed += 1.5f;
        float mag = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
        if (mag > 0) { ball.dx = (ball.dx / mag) * ball.speed; ball.dy = (ball.dy / mag) * ball.speed; }
    } else if (type == PERK_WIDER_PADDLE) {
        widerPaddleActive = true;
        widerPaddleTimer = 10.0f;
        smallerPaddleActive = false;
        paddle.width = PADDLE_WIDTH_DEFAULT * 1.7f;
    } else if (type == PERK_FIREBALL) {
        ball.isFireball = true;
        fireballActive = true;
        fireballTimer = 8.0f;
        ball.fireTimer = 8.0f;
    } else if (type == PERK_DEATH) {
        flashTimer = 0.5f;
        flashR = 1.0f; flashG = 0.0f; flashB = 0.0f;
        lives--;
        if (lives <= 0) {
            gameState = GAME_OVER;
        } else {
            initBall();
        }
    } else if (type == PERK_SMALLER_PADDLE) {
        smallerPaddleActive = true;
        smallerPaddleTimer = 8.0f;
        widerPaddleActive = false;
        paddle.width = PADDLE_WIDTH_DEFAULT * 0.5f;
    } else if (type == PERK_SHOOT) {
        shootActive = true;
        shootTimer = 12.0f;
        bulletCooldown = 0.0f;
    }
}

bool checkBallBrickCollision(Brick& brick, bool pierce) {
    if (!brick.active) return false;
    float ballLeft   = ball.x - BALL_RADIUS;
    float ballRight  = ball.x + BALL_RADIUS;
    float ballBottom = ball.y - BALL_RADIUS;
    float ballTop    = ball.y + BALL_RADIUS;
    float brickLeft   = brick.x;
    float brickRight  = brick.x + BRICK_WIDTH;
    float brickBottom = brick.y;
    float brickTop    = brick.y + BRICK_HEIGHT;

    if (ballRight < brickLeft || ballLeft > brickRight) return false;
    if (ballTop < brickBottom || ballBottom > brickTop) return false;

    if (!pierce) {
        float overlapLeft   = ballRight - brickLeft;
        float overlapRight  = brickRight - ballLeft;
        float overlapBottom = ballTop - brickBottom;
        float overlapTop    = brickTop - ballBottom;
        float minOverlapX = std::min(overlapLeft, overlapRight);
        float minOverlapY = std::min(overlapBottom, overlapTop);
        if (minOverlapX < minOverlapY) ball.dx = -ball.dx;
        else ball.dy = -ball.dy;
    }
    return true;
}

bool checkBulletBrickCollision(Bullet& bullet, Brick& brick) {
    if (!brick.active || !bullet.active) return false;
    float bx = bullet.x - BULLET_WIDTH / 2;
    float by = bullet.y;
    if (bx + BULLET_WIDTH < brick.x || bx > brick.x + BRICK_WIDTH) return false;
    if (by + BULLET_HEIGHT < brick.y || by > brick.y + BRICK_HEIGHT) return false;
    return true;
}

void updateGame(float dt) {
    if (gameState != PLAYING) return;
    gameTime += dt;

    if (flashTimer > 0) flashTimer -= dt;

    ball.speed += BALL_SPEED_INCREMENT;
    float mag = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
    if (mag > 0 && !ballOnPaddle) {
        ball.dx = (ball.dx / mag) * ball.speed;
        ball.dy = (ball.dy / mag) * ball.speed;
    }

    if (fireballActive) {
        fireballTimer -= dt;
        ball.fireTimer -= dt;
        if (fireballTimer <= 0) {
            fireballActive = false;
            ball.isFireball = false;
            ball.fireTimer = 0.0f;
        }
    }

    if (widerPaddleActive) {
        widerPaddleTimer -= dt;
        if (widerPaddleTimer <= 0) {
            widerPaddleActive = false;
            if (!smallerPaddleActive)
                paddle.width = PADDLE_WIDTH_DEFAULT;
        }
    }

    if (smallerPaddleActive) {
        smallerPaddleTimer -= dt;
        if (smallerPaddleTimer <= 0) {
            smallerPaddleActive = false;
            if (!widerPaddleActive)
                paddle.width = PADDLE_WIDTH_DEFAULT;
        }
    }

    if (shootActive) {
        shootTimer -= dt;
        bulletCooldown -= dt;
        if (shootTimer <= 0) {
            shootActive = false;
            bullets.clear();
        }
    }

    if (keyLeft)  { paddle.x -= PADDLE_SPEED; if (paddle.x < 0) paddle.x = 0; }
    if (keyRight) { paddle.x += PADDLE_SPEED; if (paddle.x + paddle.width > WINDOW_WIDTH) paddle.x = WINDOW_WIDTH - paddle.width; }

    if (ballOnPaddle) {
        ball.x = paddle.x + paddle.width / 2.0f;
        ball.y = PADDLE_Y + PADDLE_HEIGHT + BALL_RADIUS + 1.0f;
        return;
    }

    ball.x += ball.dx;
    ball.y += ball.dy;

    if (ball.x - BALL_RADIUS < 0)             { ball.x = BALL_RADIUS; ball.dx = fabs(ball.dx); }
    if (ball.x + BALL_RADIUS > WINDOW_WIDTH)  { ball.x = WINDOW_WIDTH - BALL_RADIUS; ball.dx = -fabs(ball.dx); }
    if (ball.y + BALL_RADIUS > WINDOW_HEIGHT) { ball.y = WINDOW_HEIGHT - BALL_RADIUS; ball.dy = -fabs(ball.dy); }

    if (ball.y - BALL_RADIUS <= PADDLE_Y + PADDLE_HEIGHT &&
        ball.y - BALL_RADIUS >= PADDLE_Y &&
        ball.x >= paddle.x && ball.x <= paddle.x + paddle.width &&
        ball.dy < 0) {
        ball.dy = fabs(ball.dy);
        float hitPos = (ball.x - paddle.x) / paddle.width;
        float angle = (hitPos - 0.5f) * 2.0f;
        ball.dx = ball.speed * angle * 0.8f;
        float newMag = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
        if (newMag > 0) { ball.dx = (ball.dx / newMag) * ball.speed; ball.dy = (ball.dy / newMag) * ball.speed; }
        if (ball.dy > -1.0f) ball.dy = 1.0f;
    }

    if (ball.y - BALL_RADIUS < 0) {
        lives--;
        if (lives <= 0) gameState = GAME_OVER;
        else initBall();
        return;
    }

    bool pierce = ball.isFireball;
    for (auto& brick : bricks) {
        if (!brick.active) continue;
        if (checkBallBrickCollision(brick, pierce)) {
            brick.health--;
            if (brick.health <= 0) {
                brick.active = false;
                score += 10 * currentLevel;
                spawnPerk(brick.x + BRICK_WIDTH / 2.0f, brick.y + BRICK_HEIGHT / 2.0f, brick.perk);
            } else {
                brick.r *= 0.75f; brick.g *= 0.75f; brick.b *= 0.75f;
                score += 5;
            }
            if (!pierce) break;
        }
    }

    int activeBricks = 0;
    for (auto& brick : bricks) if (brick.active) activeBricks++;
    if (activeBricks == 0) {
        if (currentLevel < MAX_LEVELS) {
            score += 100 * currentLevel;
            gameState = WIN;
        } else {
            gameState = WIN;
        }
    }

    for (auto& p : perks) {
        if (!p.active) continue;
        p.y += p.dy;
        if (p.y <= PADDLE_Y + PADDLE_HEIGHT && p.y >= PADDLE_Y - PERK_HEIGHT &&
            p.x + PERK_WIDTH >= paddle.x && p.x <= paddle.x + paddle.width) {
            p.active = false;
            applyPerk(p.type);
        }
        if (p.y < -PERK_HEIGHT) p.active = false;
    }

    for (auto& blt : bullets) {
        if (!blt.active) continue;
        blt.y += BULLET_SPEED;
        if (blt.y > WINDOW_HEIGHT) { blt.active = false; continue; }
        for (auto& brick : bricks) {
            if (!brick.active) continue;
            if (checkBulletBrickCollision(blt, brick)) {
                blt.active = false;
                brick.health--;
                if (brick.health <= 0) {
                    brick.active = false;
                    score += 8 * currentLevel;
                    spawnPerk(brick.x + BRICK_WIDTH / 2.0f, brick.y + BRICK_HEIGHT / 2.0f, brick.perk);
                } else {
                    brick.r *= 0.75f; brick.g *= 0.75f; brick.b *= 0.75f;
                }
                break;
            }
        }
    }
}

void drawBackground() {
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.15f); glVertex2f(0, 0); glVertex2f(WINDOW_WIDTH, 0);
    glColor3f(0.0f, 0.0f, 0.3f);  glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT); glVertex2f(0, WINDOW_HEIGHT);
    glEnd();
}

void drawPaddle() {
    float px = paddle.x, py = paddle.y, pw = paddle.width, ph = PADDLE_HEIGHT;

    float pr = 0.3f, pg = 0.6f, pb = 1.0f;
    if (shootActive)         { pr = 1.0f; pg = 1.0f; pb = 0.0f; }
    if (smallerPaddleActive) { pr = 1.0f; pg = 0.2f; pb = 0.5f; }
    if (widerPaddleActive)   { pr = 0.0f; pg = 0.8f; pb = 1.0f; }

    glPointSize(1.0f);
    drawBresenhamRect(px, py, pw, ph, pr, pg, pb);

    float hr = std::min(pr + 0.4f, 1.0f);
    float hg = std::min(pg + 0.3f, 1.0f);
    glColor3f(hr, hg, 1.0f);
    drawBresenhamRect(px + 2, py + ph - 4, pw - 4, 3, hr, hg, 1.0f);

    drawBresenhamRectOutline(px, py, pw, ph, 1.0f, 1.0f, 1.0f);

    if (shootActive) {
        drawBresenhamRect(px + 2, py + ph, 8, 6, 1.0f, 0.8f, 0.0f);
        drawBresenhamRect(px + pw - 10, py + ph, 8, 6, 1.0f, 0.8f, 0.0f);
    }
}

void drawBall() {
    if (!ball.active) return;

    int bx = (int)ball.x;
    int by = (int)ball.y;
    int br = (int)BALL_RADIUS;

    glPointSize(1.0f);

    if (ball.isFireball) {
        glColor3f(1.0f, 0.3f, 0.0f);
        midpointCircleFilled(bx, by, br + 5);

        glColor3f(1.0f, 0.6f, 0.0f);
        midpointCircleFilled(bx, by, br + 3);

        glColor3f(1.0f, 1.0f, 0.3f);
        midpointCircleFilled(bx, by, br);
    } else {
        glColor3f(0.0f, 0.0f, 0.0f);
        midpointCircleFilled(bx + 2, by - 2, br);

        glColor3f(1.0f, 1.0f, 1.0f);
        midpointCircleFilled(bx, by, br);

        glColor3f(0.9f, 0.9f, 1.0f);
        midpointCircleFilled(bx - 3, by + 3, (int)(BALL_RADIUS * 0.3f));
    }
}

void drawBricks() {
    for (auto& brick : bricks) {
        if (!brick.active) continue;

        if (brick.isWall) {
            drawRect(brick.x, brick.y, BRICK_WIDTH, BRICK_HEIGHT, brick.r, brick.g, brick.b);
            drawRect(brick.x, brick.y + BRICK_HEIGHT / 2 - 1, BRICK_WIDTH, 2, 0.25f, 0.15f, 0.1f);
            int brickIndex = (int)((brick.y - BRICK_START_Y) / -(BRICK_HEIGHT + BRICK_PADDING));
            float vertX = (brickIndex % 2 == 0) ? brick.x + BRICK_WIDTH * 0.5f : brick.x + BRICK_WIDTH * 0.25f;
            drawRect(vertX, brick.y, 2, BRICK_HEIGHT / 2, 0.25f, 0.15f, 0.1f);
            drawRect(brick.x, brick.y, BRICK_WIDTH, BRICK_HEIGHT, 0.1f, 0.05f, 0.0f, false);
            for (int h = 0; h < brick.health && h < 4; h++) {
                drawCircle(brick.x + 8 + h * 10, brick.y + BRICK_HEIGHT / 2, 3, 1.0f, 1.0f, 0.0f);
            }
        } else {
            drawRect(brick.x, brick.y, BRICK_WIDTH, BRICK_HEIGHT, brick.r, brick.g, brick.b);
            drawRect(brick.x + 1, brick.y + BRICK_HEIGHT - 4, BRICK_WIDTH - 2, 3,
                std::min(brick.r + 0.3f, 1.0f), std::min(brick.g + 0.3f, 1.0f), std::min(brick.b + 0.3f, 1.0f));
            drawRect(brick.x, brick.y, BRICK_WIDTH, BRICK_HEIGHT, 0.0f, 0.0f, 0.0f, false);
            if (brick.perk != PERK_NONE) {
                float cx = brick.x + BRICK_WIDTH / 2.0f, cy = brick.y + BRICK_HEIGHT / 2.0f;
                if      (brick.perk == PERK_EXTRA_LIFE)     drawCircle(cx, cy, 4, 0.0f, 1.0f, 0.0f);
                else if (brick.perk == PERK_FASTER_BALL)    drawCircle(cx, cy, 4, 1.0f, 0.3f, 0.0f);
                else if (brick.perk == PERK_WIDER_PADDLE)   drawCircle(cx, cy, 4, 0.0f, 0.5f, 1.0f);
                else if (brick.perk == PERK_FIREBALL)       drawCircle(cx, cy, 4, 1.0f, 0.6f, 0.0f);
                else if (brick.perk == PERK_DEATH)          drawCircle(cx, cy, 4, 0.8f, 0.0f, 0.8f);
                else if (brick.perk == PERK_SMALLER_PADDLE) drawCircle(cx, cy, 4, 1.0f, 0.0f, 0.5f);
                else if (brick.perk == PERK_SHOOT)          drawCircle(cx, cy, 4, 1.0f, 1.0f, 0.0f);
            }
        }
    }
}

void drawPerks() {
    for (auto& p : perks) {
        if (!p.active) continue;
        drawRect(p.x - PERK_WIDTH / 2, p.y - PERK_HEIGHT / 2, PERK_WIDTH, PERK_HEIGHT, p.r, p.g, p.b);
        drawRect(p.x - PERK_WIDTH / 2, p.y - PERK_HEIGHT / 2, PERK_WIDTH, PERK_HEIGHT, 1.0f, 1.0f, 1.0f, false);
        std::string label = "?";
        if      (p.type == PERK_EXTRA_LIFE)     label = "L";
        else if (p.type == PERK_FASTER_BALL)    label = "F";
        else if (p.type == PERK_WIDER_PADDLE)   label = "W";
        else if (p.type == PERK_FIREBALL)       label = "B";
        else if (p.type == PERK_DEATH)          label = "X";
        else if (p.type == PERK_SMALLER_PADDLE) label = "S";
        else if (p.type == PERK_SHOOT)          label = "G";
        drawText(p.x - 4, p.y - 6, label, 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_12);
    }
}

void drawBullets() {
    for (auto& blt : bullets) {
        if (!blt.active) continue;
        drawRect(blt.x - BULLET_WIDTH / 2, blt.y, BULLET_WIDTH, BULLET_HEIGHT, 1.0f, 1.0f, 0.0f);
        drawRect(blt.x - BULLET_WIDTH / 2, blt.y + BULLET_HEIGHT - 3, BULLET_WIDTH, 3, 1.0f, 0.5f, 0.0f);
    }
}

void drawHUD() {
    drawRect(0, WINDOW_HEIGHT - 40, WINDOW_WIDTH, 40, 0.0f, 0.0f, 0.2f);
    drawRect(0, WINDOW_HEIGHT - 41, WINDOW_WIDTH, 2, 0.3f, 0.6f, 1.0f);
    drawText(10, WINDOW_HEIGHT - 25, "Lives:", 0.8f, 0.8f, 1.0f);
    for (int i = 0; i < lives && i < 7; i++)
        drawCircle(80 + i * 22, WINDOW_HEIGHT - 20, 8, 1.0f, 0.3f, 0.3f);
    drawText(200, WINDOW_HEIGHT - 25, "Score: " + intToString(score), 1.0f, 1.0f, 0.0f);
    drawText(360, WINDOW_HEIGHT - 25, "Time: " + floatToString(gameTime) + "s", 0.5f, 1.0f, 0.5f);
    drawText(520, WINDOW_HEIGHT - 25, "Spd: " + floatToString(ball.speed, 1), 1.0f, 0.5f, 0.0f);
    drawText(640, WINDOW_HEIGHT - 25, "Lvl: " + intToString(currentLevel) + "/" + intToString(MAX_LEVELS), 0.8f, 0.8f, 1.0f);

    int hintY = 10;
    if (widerPaddleActive)
        drawText(10, hintY, "WIDE:" + floatToString(widerPaddleTimer, 1) + "s", 0.0f, 0.8f, 1.0f, GLUT_BITMAP_HELVETICA_12);
    if (smallerPaddleActive)
        drawText(110, hintY, "SMALL:" + floatToString(smallerPaddleTimer, 1) + "s", 1.0f, 0.0f, 0.5f, GLUT_BITMAP_HELVETICA_12);
    if (shootActive)
        drawText(220, hintY, "GUN:" + floatToString(shootTimer, 1) + "s", 1.0f, 1.0f, 0.0f, GLUT_BITMAP_HELVETICA_12);
    if (fireballActive)
        drawText(330, hintY, "FIRE:" + floatToString(fireballTimer, 1) + "s", 1.0f, 0.5f, 0.0f, GLUT_BITMAP_HELVETICA_12);
    drawText(460, hintY, "[L]Life [F]Fast [W]Wide [B]Fire [X]Death [S]Small [G]Gun",
        0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_12);
}

void drawPauseOverlay() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0); glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT); glVertex2f(0, WINDOW_HEIGHT);
    glEnd();
    glDisable(GL_BLEND);

    drawRect(260, 210, 280, 180, 0.0f, 0.0f, 0.25f);
    drawRect(260, 210, 280, 180, 0.0f, 0.8f, 1.0f, false);
    drawTextLarge(340, 365, "PAUSED", 1.0f, 1.0f, 0.0f);

    std::vector<std::string> opts = { "RESUME", "MAIN MENU" };
    for (int i = 0; i < (int)opts.size(); i++) {
        float by2 = 310 - i * 55;
        if (selectedPauseMenu == i) {
            drawRect(285, by2 - 5, 230, 38, 0.0f, 0.4f, 0.8f);
            drawRect(285, by2 - 5, 230, 38, 0.0f, 1.0f, 1.0f, false);
            drawTextLarge(330, by2 + 8, opts[i], 1.0f, 1.0f, 1.0f);
        } else {
            drawRect(285, by2 - 5, 230, 38, 0.05f, 0.05f, 0.2f);
            drawRect(285, by2 - 5, 230, 38, 0.3f, 0.3f, 0.6f, false);
            drawTextLarge(330, by2 + 8, opts[i], 0.7f, 0.7f, 0.9f);
        }
    }
    drawText(280, 222, "UP/DOWN to select, ENTER to confirm", 0.5f, 0.5f, 0.7f, GLUT_BITMAP_HELVETICA_12);
}

void drawMenu() {
    drawBackground();
    drawTextLarge(240, 500, "DX BALL - ADVANCED", 0.0f, 0.8f, 1.0f);
    drawTextLarge(238, 498, "DX BALL - ADVANCED", 0.0f, 0.3f, 0.6f);
    drawText(280, 460, "CSE 426 - Computer Graphics Lab", 0.7f, 0.7f, 0.7f, GLUT_BITMAP_HELVETICA_12);
    drawText(260, 440, "Level System | Fireball | Gun | Death Item", 0.5f, 0.9f, 0.5f, GLUT_BITMAP_HELVETICA_12);

    std::vector<std::string> items = { "START GAME", "HOW TO PLAY", "EXIT" };
    for (int i = 0; i < (int)items.size(); i++) {
        float y = 360 - i * 65;
        float bx = 270, by = y - 15, bw = 260, bh = 45;
        if (selectedMenu == i) {
            drawRect(bx, by, bw, bh, 0.0f, 0.4f, 0.8f);
            drawRect(bx, by, bw, bh, 0.0f, 0.8f, 1.0f, false);
            drawTextLarge(bx + 40, by + 12, items[i], 1.0f, 1.0f, 1.0f);
        } else {
            drawRect(bx, by, bw, bh, 0.05f, 0.05f, 0.2f);
            drawRect(bx, by, bw, bh, 0.3f, 0.3f, 0.6f, false);
            drawTextLarge(bx + 40, by + 12, items[i], 0.7f, 0.7f, 0.9f);
        }
    }
    drawText(220, 60, "Use UP/DOWN arrows to navigate, ENTER to select", 0.5f, 0.5f, 0.7f, GLUT_BITMAP_HELVETICA_12);
}

void drawHelpScreen() {
    drawBackground();
    drawTextLarge(270, 548, "HOW TO PLAY", 0.0f, 0.8f, 1.0f);
    std::vector<std::string> lines = {
        "CONTROLS:",
        "  LEFT/RIGHT Arrow : Move Paddle",
        "  Mouse Move       : Move Paddle",
        "  SPACE / L-Click  : Launch Ball",
        "  P                : Pause / Resume",
        "  ESC              : Menu (in-game)",
        "",
        "GAME:",
        "  Break ALL bricks to advance levels!",
        "  3 Levels total - harder each level",
        "  Gray bricks = Wall (needs more hits)",
        "",
        "POWER-UPS (catch falling items):",
        "  [L] GREEN    = Extra Life",
        "  [F] ORANGE   = Faster Ball",
        "  [W] BLUE     = Wide Paddle (10s)",
        "  [B] FIRE     = Fireball - pierces bricks (8s)",
        "  [X] PURPLE   = DEATH - lose a life instantly!",
        "  [S] PINK     = Smaller Paddle (8s)",
        "  [G] YELLOW   = Gun - shoot bullets (12s)",
        "",
        "  SPACE also fires gun when active"
    };
    for (int i = 0; i < (int)lines.size(); i++)
        drawText(130, 512 - i * 22, lines[i], 0.9f, 0.9f, 0.9f);
    drawText(240, 28, "Press ESC to go back", 0.5f, 0.7f, 1.0f, GLUT_BITMAP_HELVETICA_12);
}

void drawGameOver() {
    drawBackground(); drawBricks(); drawHUD();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
    glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT); glVertex2f(0, WINDOW_HEIGHT); glEnd();
    glDisable(GL_BLEND);
    drawRect(190, 190, 420, 220, 0.12f, 0.0f, 0.0f);
    drawRect(190, 190, 420, 220, 1.0f, 0.0f, 0.0f, false);
    drawTextLarge(275, 368, "GAME OVER", 1.0f, 0.2f, 0.2f);
    drawText(270, 328, "Final Score: " + intToString(score), 1.0f, 1.0f, 0.5f);
    drawText(270, 298, "Level Reached: " + intToString(currentLevel), 0.8f, 0.8f, 1.0f);
    drawText(270, 268, "Time: " + floatToString(gameTime) + " seconds", 0.8f, 0.8f, 0.8f);
    drawText(250, 238, "Press ENTER to Play Again", 0.9f, 0.9f, 0.9f);
    drawText(260, 213, "Press ESC for Main Menu", 0.7f, 0.7f, 0.7f);
}

void drawWin() {
    drawBackground();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.05f, 0.0f, 0.5f);
    glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT); glVertex2f(0, WINDOW_HEIGHT); glEnd();
    glDisable(GL_BLEND);
    drawRect(170, 170, 460, 260, 0.0f, 0.1f, 0.05f);
    drawRect(170, 170, 460, 260, 0.0f, 1.0f, 0.5f, false);
    if (currentLevel > MAX_LEVELS) {
        drawTextLarge(240, 390, "ALL LEVELS COMPLETE!", 0.0f, 1.0f, 0.4f);
        drawText(240, 355, "You are a true DX Ball Master!", 0.9f, 1.0f, 0.9f);
    } else {
        drawTextLarge(255, 390, "LEVEL " + intToString(currentLevel - 1) + " CLEAR!", 0.0f, 1.0f, 0.4f);
        drawText(240, 355, "Level " + intToString(currentLevel) + " Awaits!", 0.9f, 1.0f, 0.9f);
    }
    drawText(250, 322, "Score: " + intToString(score), 1.0f, 1.0f, 0.5f);
    drawText(250, 292, "Time: " + floatToString(gameTime) + " seconds", 0.8f, 0.8f, 0.8f);
    if (currentLevel <= MAX_LEVELS)
        drawText(230, 258, "Press ENTER for Next Level", 0.0f, 1.0f, 0.5f);
    else
        drawText(230, 258, "Press ENTER to Play Again", 0.0f, 1.0f, 0.5f);
    drawText(250, 228, "Press ESC for Main Menu", 0.7f, 0.7f, 0.7f);
}

void drawFlashOverlay() {
    if (flashTimer <= 0) return;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float alpha = flashTimer * 1.5f; if (alpha > 0.6f) alpha = 0.6f;
    glColor4f(flashR, flashG, flashB, alpha);
    glBegin(GL_QUADS); glVertex2f(0, 0); glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT); glVertex2f(0, WINDOW_HEIGHT); glEnd();
    glDisable(GL_BLEND);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    switch (gameState) {
    case MENU:
        drawMenu();
        break;
    case HELP:
        drawHelpScreen();
        break;
    case PLAYING:
        drawBackground();
        drawBricks();
        drawPerks();
        drawBullets();
        drawPaddle();
        drawBall();
        drawHUD();
        drawFlashOverlay();
        if (ballOnPaddle)
            drawText(280, 200, "Press SPACE or Click to launch!", 1.0f, 1.0f, 0.0f);
        if (shootActive && ballOnPaddle)
            drawText(280, 175, "SPACE also fires gun!", 1.0f, 0.8f, 0.0f);
        break;
    case PAUSED:
        drawBackground();
        drawBricks();
        drawPerks();
        drawBullets();
        drawPaddle();
        drawBall();
        drawHUD();
        drawPauseOverlay();
        break;
    case GAME_OVER:
        drawGameOver();
        break;
    case WIN:
        drawWin();
        break;
    }
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

void timer(int value) {
    float dt = 1.0f / 60.0f;
    updateGame(dt);
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void fireBullet() {
    if (!shootActive) return;
    if (bulletCooldown > 0) return;
    Bullet b1; b1.x = paddle.x + 5; b1.y = paddle.y + PADDLE_HEIGHT + 1; b1.active = true;
    Bullet b2; b2.x = paddle.x + paddle.width - 5; b2.y = b1.y; b2.active = true;
    bullets.push_back(b1);
    bullets.push_back(b2);
    bulletCooldown = 0.35f;
}

void keyboard(unsigned char key, int x, int y) {
    if (gameState == HELP) {
        if (key == 27) gameState = MENU;
        glutPostRedisplay(); return;
    }
    switch (gameState) {
    case MENU:
        if (key == 13) {
            if (selectedMenu == 0) initGame();
            else if (selectedMenu == 1) gameState = HELP;
            else if (selectedMenu == 2) exit(0);
        }
        break;
    case PLAYING:
        if (key == ' ') {
            if (ballOnPaddle) {
                ballOnPaddle = false;
                ball.dy = fabs(ball.speed * 0.7f);
                ball.dx = ball.speed * 0.7f;
            } else if (shootActive) {
                fireBullet();
            }
        }
        if (key == 'p' || key == 'P') {
            gameState = PAUSED;
            selectedPauseMenu = 0;
        }
        if (key == 27) {
            gameState = PAUSED;
            selectedPauseMenu = 1;
        }
        break;
    case PAUSED:
        if (key == 'p' || key == 'P') {
            gameState = PLAYING;
        }
        if (key == 13) {
            if (selectedPauseMenu == 0) gameState = PLAYING;
            else if (selectedPauseMenu == 1) { gameState = MENU; selectedMenu = 0; }
        }
        if (key == 27) {
            gameState = PLAYING;
        }
        break;
    case GAME_OVER:
        if (key == 13) initGame();
        if (key == 27) { gameState = MENU; selectedMenu = 0; }
        break;
    case WIN:
        if (key == 13) {
            if (currentLevel <= MAX_LEVELS) nextLevel();
            else initGame();
        }
        if (key == 27) { gameState = MENU; selectedMenu = 0; }
        break;
    default: break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    if (gameState == MENU || gameState == HELP) {
        if (key == GLUT_KEY_UP)   selectedMenu = (selectedMenu - 1 + 3) % 3;
        if (key == GLUT_KEY_DOWN) selectedMenu = (selectedMenu + 1) % 3;
        glutPostRedisplay(); return;
    }
    if (gameState == PAUSED) {
        if (key == GLUT_KEY_UP)   selectedPauseMenu = (selectedPauseMenu - 1 + 2) % 2;
        if (key == GLUT_KEY_DOWN) selectedPauseMenu = (selectedPauseMenu + 1) % 2;
        glutPostRedisplay(); return;
    }
    if (gameState == PLAYING) {
        if (key == GLUT_KEY_LEFT)  keyLeft = true;
        if (key == GLUT_KEY_RIGHT) keyRight = true;
    }
}

void specialKeysUp(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT)  keyLeft = false;
    if (key == GLUT_KEY_RIGHT) keyRight = false;
}

void mouseMotion(int x, int y) {
    if (gameState != PLAYING) return;
    paddle.x = (float)x - paddle.width / 2.0f;
    if (paddle.x < 0) paddle.x = 0;
    if (paddle.x + paddle.width > WINDOW_WIDTH) paddle.x = WINDOW_WIDTH - paddle.width;
    glutPostRedisplay();
}

void mouseClick(int button, int state, int x, int y) {
    if (gameState == PLAYING && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (ballOnPaddle) {
            ballOnPaddle = false;
            ball.dy = fabs(ball.speed * 0.7f);
            ball.dx = ball.speed * 0.7f;
        } else if (shootActive) {
            fireBullet();
        }
    }
}

int main(int argc, char** argv) {
    srand((unsigned int)time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("DX Ball Advanced - CSE 426");
    glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
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
