#include <GL/glut.h>
#include <iostream>
#include <string>
#include <cmath>

const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;
const float PADDLE_WIDTH  = 100.0f;
const float PADDLE_HEIGHT = 15.0f;
const float PADDLE_Y      = 30.0f;
const float PADDLE_SPEED  = 8.0f;
const float BALL_RADIUS       = 10.0f;
const float BALL_SPEED_INIT   = 4.5f;

struct Paddle {
    float x, y, width, height;
};
struct Ball {
    float x, y;
    float dx, dy;
    float speed;
    bool  active;
};

Paddle paddle;
Ball   ball;
bool   ballOnPaddle = true;
bool   keyLeft  = false;
bool   keyRight = false;
void drawRect(float x, float y, float w, float h,
              float r, float g, float b, bool filled = true) {
    glColor3f(r, g, b);
    if (filled) glBegin(GL_QUADS);
    else        glBegin(GL_LINE_LOOP);
    glVertex2f(x,     y    );
    glVertex2f(x + w, y    );
    glVertex2f(x + w, y + h);
    glVertex2f(x,     y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float radius,
                float r, float g, float b, bool filled = true) {
    glColor3f(r, g, b);
    if (filled) glBegin(GL_POLYGON);
    else        glBegin(GL_LINE_LOOP);

    int segments = 36;
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * 3.14159265f * i / segments;
        glVertex2f(cx + radius * cosf(angle),
                   cy + radius * sinf(angle));
    }
    glEnd();
}
void drawText(float x, float y, const std::string& text,
              float r = 1.0f, float g = 1.0f, float b = 1.0f,
              void* font = GLUT_BITMAP_HELVETICA_18) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(font, c);
}
void drawBackground() {
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.15f);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glColor3f(0.0f, 0.0f, 0.35f);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    glVertex2f(0, WINDOW_HEIGHT);
    glEnd();
}
void drawPaddle() {
    drawRect(paddle.x, paddle.y, paddle.width, paddle.height,
             0.3f, 0.6f, 1.0f);
    drawRect(paddle.x + 2, paddle.y + paddle.height - 4,
             paddle.width - 4, 3, 0.7f, 0.9f, 1.0f);
    drawRect(paddle.x, paddle.y, paddle.width, paddle.height,
             1.0f, 1.0f, 1.0f, false);
}
void drawBall() {
    drawCircle(ball.x + 2, ball.y - 2, BALL_RADIUS,
               0.0f, 0.0f, 0.0f);
    drawCircle(ball.x, ball.y, BALL_RADIUS,
               1.0f, 1.0f, 1.0f);
    drawCircle(ball.x - 3, ball.y + 3, BALL_RADIUS * 0.3f,
               0.9f, 0.9f, 1.0f);
}
void updateBall(float dt) {
    if (ballOnPaddle) {
        ball.x = paddle.x + paddle.width / 2.0f;
        ball.y = PADDLE_Y + PADDLE_HEIGHT + BALL_RADIUS + 1.0f;
        return;
    }
    ball.x += ball.dx;
    ball.y += ball.dy;
    if (ball.x - BALL_RADIUS < 0) {
        ball.x = BALL_RADIUS;
        ball.dx = fabsf(ball.dx);
    }
    if (ball.x + BALL_RADIUS > WINDOW_WIDTH) {
        ball.x = WINDOW_WIDTH - BALL_RADIUS;
        ball.dx = -fabsf(ball.dx); 
    }
    if (ball.y + BALL_RADIUS > WINDOW_HEIGHT) {
        ball.y = WINDOW_HEIGHT - BALL_RADIUS;
        ball.dy = -fabsf(ball.dy);
    }
    bool hitPaddle = (ball.y - BALL_RADIUS <= PADDLE_Y + PADDLE_HEIGHT) &&
                     (ball.y - BALL_RADIUS >= PADDLE_Y - 5.0f)          &&
                     (ball.x >= paddle.x)                                &&
                     (ball.x <= paddle.x + paddle.width)                 &&
                     (ball.dy < 0);
    if (hitPaddle) {
        ball.dy = fabsf(ball.dy);
        float hitPos = (ball.x - paddle.x) / paddle.width;
        float angle  = (hitPos - 0.5f) * 2.0f;
        ball.dx = ball.speed * angle * 0.8f;
        float mag = sqrtf(ball.dx * ball.dx + ball.dy * ball.dy);
        if (mag > 0) {
            ball.dx = (ball.dx / mag) * ball.speed;
            ball.dy = (ball.dy / mag) * ball.speed;
        }
        if (fabsf(ball.dy) < 1.5f)
            ball.dy = (ball.dy < 0) ? -1.5f : 1.5f;
    }
    if (ball.y - BALL_RADIUS < 0) {
        std::cout << "Ball lost!" << std::endl;
        // Reset ball to paddle
        ballOnPaddle = true;
        ball.speed = BALL_SPEED_INIT;
    }
}

void update(float dt) {
    // Paddle movement
    if (keyLeft) {
        paddle.x -= PADDLE_SPEED;
        if (paddle.x < 0) paddle.x = 0;
    }
    if (keyRight) {
        paddle.x += PADDLE_SPEED;
        if (paddle.x + paddle.width > WINDOW_WIDTH)
            paddle.x = WINDOW_WIDTH - paddle.width;
    }

    updateBall(dt);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    drawBackground();
    drawPaddle();
    drawBall();

    if (ballOnPaddle) {
        drawText(255, 200, "Press SPACE or Click to launch!",
                 1.0f, 1.0f, 0.0f);
    }

    glutSwapBuffers();
}

void timer(int value) {
    update(1.0f / 60.0f);
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
    if (key == ' ') {
        if (ballOnPaddle) {
            ballOnPaddle = false;
            // Launch at 45 degree angle
            ball.dy =  ball.speed * 0.707f;
            ball.dx =  ball.speed * 0.707f;
        }
    }
}

void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT)  keyLeft  = true;
    if (key == GLUT_KEY_RIGHT) keyRight = true;
}

void specialKeysUp(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT)  keyLeft  = false;
    if (key == GLUT_KEY_RIGHT) keyRight = false;
}

void mouseMotion(int x, int y) {
    float mx = (float)x;
    paddle.x  = mx - paddle.width / 2.0f;
    if (paddle.x < 0) paddle.x = 0;
    if (paddle.x + paddle.width > WINDOW_WIDTH)
        paddle.x = WINDOW_WIDTH - paddle.width;
}

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (ballOnPaddle) {
            ballOnPaddle = false;
            ball.dy = ball.speed * 0.707f;
            ball.dx = ball.speed * 0.707f;
        }
    }
}

void init() {
    paddle.x      = WINDOW_WIDTH / 2.0f - PADDLE_WIDTH / 2.0f;
    paddle.y      = PADDLE_Y;
    paddle.width  = PADDLE_WIDTH;
    paddle.height = PADDLE_HEIGHT;

    // Ball
    ball.speed  = BALL_SPEED_INIT;
    ball.active = true;
    ballOnPaddle = true;
    ball.x = paddle.x + paddle.width / 2.0f;
    ball.y = PADDLE_Y + PADDLE_HEIGHT + BALL_RADIUS + 1.0f;
    ball.dx = 0; ball.dy = 0;
}

int main(int argc, char** argv) {
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
