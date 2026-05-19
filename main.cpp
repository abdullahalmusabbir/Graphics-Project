#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

// ==================== CONSTANTS ====================
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;
const int BRICK_ROWS    = 6;
const int BRICK_COLS    = 10;
const float BRICK_WIDTH   = 70.0f;
const float BRICK_HEIGHT  = 25.0f;
const float BRICK_PADDING = 5.0f;
const float BRICK_START_X = 25.0f;
const float BRICK_START_Y = 420.0f;
const float PADDLE_WIDTH_DEFAULT = 100.0f;
const float PADDLE_HEIGHT = 15.0f;
const float PADDLE_Y      = 30.0f;
const float PADDLE_SPEED  = 8.0f;
const float BALL_RADIUS   = 10.0f;
const float BALL_SPEED_INITIAL   = 4.0f;
const float BALL_SPEED_INCREMENT = 0.0003f;
const float PERK_WIDTH  = 20.0f;
const float PERK_HEIGHT = 20.0f;
const float PERK_SPEED  = 2.5f;
const float BULLET_WIDTH  = 5.0f;
const float BULLET_HEIGHT = 12.0f;
const float BULLET_SPEED  = 8.0f;
const int MAX_LEVELS      = 3;
const int MAX_HIGH_SCORES = 5;

enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, WIN, HELP, HIGH_SCORE };
enum PerkType {
    PERK_EXTRA_LIFE, PERK_FASTER_BALL, PERK_WIDER_PADDLE,
    PERK_FIREBALL, PERK_DEATH, PERK_SMALLER_PADDLE, PERK_SHOOT, PERK_NONE
};

struct Brick  { float x,y,r,g,b; int health; bool active,isWall; PerkType perk; };
struct Ball   { float x,y,dx,dy,speed,fireTimer; bool active,isFireball; };
struct Paddle { float x,y,width; };
struct Perk   { float x,y,dy,r,g,b; PerkType type; bool active; };
struct Bullet { float x,y; bool active; };
struct HighScoreEntry { int score,level; float time; };

bool soundEnabled = true;

#ifdef _WIN32
DWORD WINAPI beepThread(LPVOID param) {
    int id = (int)(intptr_t)param;
    switch(id) {
        case 0: Beep(880,25); break;
        case 1: Beep(440,35); Beep(550,25); break;
        case 2: Beep(1047,50); Beep(1319,50); Beep(1568,60); break;
        case 3: Beep(350,120); Beep(280,120); Beep(220,180); break;
        case 4: Beep(392,120); Beep(330,120); Beep(294,120); Beep(220,250); break;
        case 5: Beep(523,80); Beep(659,80); Beep(784,80); Beep(1047,80); Beep(1319,160); break;
        case 6: Beep(1200,18); break;
        case 7: Beep(784,80); Beep(988,80); Beep(1175,80); Beep(1568,160); break;
        case 8: Beep(660,40); Beep(880,40); break;
        case 9: Beep(800,30); Beep(1000,30); Beep(1200,40); break;
    }
    return 0;
}
void playSound(int id) {
    if(!soundEnabled) return;
    HANDLE h = CreateThread(NULL,0,beepThread,(LPVOID)(intptr_t)id,0,NULL);
    if(h) CloseHandle(h);
}
#else
void playSound(int) {}
#endif

#define SND_BRICK    0
#define SND_PADDLE   1
#define SND_PERK     2
#define SND_DEATH    3
#define SND_GAMEOVER 4
#define SND_WIN      5
#define SND_BULLET   6
#define SND_LEVELUP  7
#define SND_LAUNCH   8
#define SND_FIREBALL 9

GameState gameState = MENU;
Ball      ball;
Paddle    paddle;
std::vector<Brick>          bricks;
std::vector<Perk>           perks;
std::vector<Bullet>         bullets;
std::vector<HighScoreEntry> highScores;

int   lives=3, score=0, currentLevel=1;
float gameTime=0.0f;
bool  ballOnPaddle=true;
bool  keyLeft=false, keyRight=false;
int   selectedMenu=0, selectedPauseMenu=0;
float paddleWidth=PADDLE_WIDTH_DEFAULT;
bool  widerPaddleActive=false;   float widerPaddleTimer=0;
bool  smallerPaddleActive=false; float smallerPaddleTimer=0;
bool  shootActive=false;  float shootTimer=0, bulletCooldown=0;
bool  fireballActive=false; float fireballTimer=0;
float flashTimer=0, flashR=1, flashG=1, flashB=1;
bool  gameStarted=false;

const std::string HS_FILE = "dx_highscores.dat";

void loadHighScores() {
    highScores.clear();
    std::ifstream f(HS_FILE.c_str(), std::ios::binary);
    if(!f) return;
    int n=0;
    f.read(reinterpret_cast<char*>(&n), sizeof(n));
    for(int i=0;i<n&&i<MAX_HIGH_SCORES;i++) {
        HighScoreEntry e;
        f.read(reinterpret_cast<char*>(&e.score), sizeof(e.score));
        f.read(reinterpret_cast<char*>(&e.time),  sizeof(e.time));
        f.read(reinterpret_cast<char*>(&e.level), sizeof(e.level));
        highScores.push_back(e);
    }
    f.close();
}

void saveHighScores() {
    std::ofstream f(HS_FILE.c_str(), std::ios::binary|std::ios::trunc);
    if(!f) return;
    int n=(int)highScores.size();
    f.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for(auto& e:highScores) {
        f.write(reinterpret_cast<const char*>(&e.score), sizeof(e.score));
        f.write(reinterpret_cast<const char*>(&e.time),  sizeof(e.time));
        f.write(reinterpret_cast<const char*>(&e.level), sizeof(e.level));
    }
    f.close();
}

void addHighScore(int sc, float tm, int lv) {
    HighScoreEntry e; e.score=sc; e.time=tm; e.level=lv;
    highScores.push_back(e);
    std::sort(highScores.begin(), highScores.end(),
        [](const HighScoreEntry& a, const HighScoreEntry& b){ return a.score>b.score; });
    if((int)highScores.size()>MAX_HIGH_SCORES) highScores.resize(MAX_HIGH_SCORES);
    saveHighScores();
}

std::string iStr(int v)           { std::ostringstream o; o<<v; return o.str(); }
std::string fStr(float v, int d=1){ std::ostringstream o; o.precision(d); o<<std::fixed<<v; return o.str(); }

void drawText(float x,float y,const std::string& s,
              float r=1,float g=1,float b=1,void* font=GLUT_BITMAP_HELVETICA_18) {
    glColor3f(r,g,b); glRasterPos2f(x,y);
    for(char c:s) glutBitmapCharacter(font,c);
}
void drawTextL(float x,float y,const std::string& s,float r=1,float g=1,float b=1) {
    glColor3f(r,g,b); glRasterPos2f(x,y);
    for(char c:s) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,c);
}
void drawRect(float x,float y,float w,float h,float r,float g,float b,bool fill=true) {
    glColor3f(r,g,b);
    glBegin(fill?GL_QUADS:GL_LINE_LOOP);
    glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
    glEnd();
}
void drawCircleGL(float cx,float cy,float rad,float r,float g,float b,bool fill=true) {
    glColor3f(r,g,b);
    glBegin(fill?GL_POLYGON:GL_LINE_LOOP);
    for(int i=0;i<32;i++) {
        float a=2*3.14159265f*i/32;
        glVertex2f(cx+rad*cosf(a),cy+rad*sinf(a));
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
        e    = 2 * dy - dx;
        inc1 = 2 * (dy - dx);
        inc2 = 2 * dy;
        for (i = 0; i < dx; i++) {
            if (e >= 0) { y += incy; e += inc1; }
            else          e += inc2;
            x += incx;
            draw_pixel(x, y);
        }
    } else {
        draw_pixel(x, y);
        e    = 2 * dx - dy;
        inc1 = 2 * (dx - dy);
        inc2 = 2 * dx;
        for (i = 0; i < dy; i++) {
            if (e >= 0) { x += incx; e += inc1; }
            else          e += inc2;
            y += incy;
            draw_pixel(x, y);
        }
    }
}

void drawBresenhamRect(float fx, float fy, float fw, float fh,
                       float r, float g, float b) {
    glColor3f(r, g, b);
    glPointSize(1.0f);
    int x1=(int)fx, y1=(int)fy;
    int x2=(int)(fx+fw), y2=(int)(fy+fh);
    for (int row = y1; row <= y2; row++)
        drawLine(x1, row, x2, row);
}

void drawBresenhamRectOutline(float fx, float fy, float fw, float fh,
                               float r, float g, float b) {
    glColor3f(r, g, b);
    glPointSize(1.0f);
    int x1=(int)fx,     y1=(int)fy;
    int x2=(int)(fx+fw), y2=(int)(fy+fh);
    drawLine(x1, y1, x2, y1); 
    drawLine(x2, y1, x2, y2); 
    drawLine(x2, y2, x1, y2); 
    drawLine(x1, y2, x1, y1); 
}

int cx_global, cy_global;

void plotCircle(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(cx_global + x, cy_global + y);
    glVertex2i(cx_global - x, cy_global + y);
    glVertex2i(cx_global + x, cy_global - y);
    glVertex2i(cx_global - x, cy_global - y);
    glVertex2i(cx_global + y, cy_global + x);
    glVertex2i(cx_global - y, cy_global + x);
    glVertex2i(cx_global + y, cy_global - x);
    glVertex2i(cx_global - y, cy_global - x);
    glEnd();
}

void drawMidpointCircle(int cx, int cy, int r) {
    cx_global = cx;
    cy_global = cy;

    int x = 0;
    int y = r;
    int d = 1 - r;

    glPointSize(1.0f);
    while (x <= y) {
        plotCircle(x, y);
        if (d < 0)
            d += 2 * x + 3;
        else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

void drawMidpointCircleFilled(int cx, int cy, int r) {
    cx_global = cx;
    cy_global = cy;

    int x = 0;
    int y = r;
    int d = 1 - r;

    glPointSize(1.0f);
    while (x <= y) {
        drawLine(cx - y, cy + x, cx + y, cy + x);
        drawLine(cx - y, cy - x, cx + y, cy - x);
        drawLine(cx - x, cy + y, cx + x, cy + y);
        drawLine(cx - x, cy - y, cx + x, cy - y);

        if (d < 0)
            d += 2 * x + 3;
        else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

PerkType randomPerk() {
    int r=rand()%10;
    if(r==0) return PERK_EXTRA_LIFE;
    if(r==1) return PERK_FASTER_BALL;
    if(r==2) return PERK_WIDER_PADDLE;
    if(r==3) return PERK_FIREBALL;
    if(r==4) return PERK_DEATH;
    if(r==5) return PERK_SMALLER_PADDLE;
    if(r==6) return PERK_SHOOT;
    return PERK_NONE;
}

void getBrickColor(int row, float&r, float&g, float&b) {
    switch(row) {
        case 0: r=1;g=.2f;b=.2f; break;
        case 1: r=1;g=.5f;b=0;   break;
        case 2: r=1;g=1;  b=0;   break;
        case 3: r=0;g=.8f;b=0;   break;
        case 4: r=0;g=.5f;b=1;   break;
        case 5: r=.6f;g=0;b=.8f; break;
        default:r=g=b=1;
    }
}

void initBricks() {
    bricks.clear();
    for(int row=0;row<BRICK_ROWS;row++) {
        for(int col=0;col<BRICK_COLS;col++) {
            Brick b;
            b.x=BRICK_START_X+col*(BRICK_WIDTH+BRICK_PADDING);
            b.y=BRICK_START_Y-row*(BRICK_HEIGHT+BRICK_PADDING);
            b.active=true; b.isWall=false;
            if(currentLevel==1) {
                b.health=(row<2)?2:1;
                getBrickColor(row,b.r,b.g,b.b);
            } else if(currentLevel==2) {
                if((row+col)%4==0) { b.health=3; b.r=.5f;b.g=.5f;b.b=.5f; b.isWall=true; }
                else { b.health=(row<2)?2:1; getBrickColor(row,b.r,b.g,b.b); }
            } else {
                bool w=(col%3==0&&row%2==0)||(col%3==1&&row%2==1);
                if(w) { b.health=4; b.r=.45f;b.g=.3f;b.b=.2f; b.isWall=true; }
                else  { b.health=(row<3)?2:1; getBrickColor(row,b.r,b.g,b.b); }
            }
            b.perk=randomPerk();
            bricks.push_back(b);
        }
    }
}

void resetPaddle() {
    paddle.width = paddleWidth;
    paddle.x     = WINDOW_WIDTH/2.0f - paddle.width/2.0f;
    paddle.y     = PADDLE_Y;
}

void initBall() {
    ball.x=paddle.x+paddle.width/2.0f;
    ball.y=PADDLE_Y+PADDLE_HEIGHT+BALL_RADIUS+1.0f;
    ball.speed=BALL_SPEED_INITIAL+(currentLevel-1)*0.5f;
    ball.dx=ball.speed*0.7f;
    ball.dy=ball.speed*0.7f;
    ball.active=true; ball.isFireball=false; ball.fireTimer=0;
    ballOnPaddle=true;
}

void initGame() {
    lives=3; score=0; gameTime=0; currentLevel=1;
    paddleWidth=PADDLE_WIDTH_DEFAULT;
    widerPaddleActive=false;   widerPaddleTimer=0;
    smallerPaddleActive=false; smallerPaddleTimer=0;
    shootActive=false; shootTimer=0; bulletCooldown=0;
    fireballActive=false; fireballTimer=0;
    flashTimer=0;
    perks.clear(); bullets.clear();
    resetPaddle(); initBricks(); initBall();
    gameStarted=true; gameState=PLAYING;
}

void nextLevel() {
    playSound(SND_LEVELUP);
    currentLevel++;
    paddleWidth=PADDLE_WIDTH_DEFAULT;
    widerPaddleActive=false;   widerPaddleTimer=0;
    smallerPaddleActive=false; smallerPaddleTimer=0;
    shootActive=false; shootTimer=0; bulletCooldown=0;
    fireballActive=false; fireballTimer=0;
    perks.clear(); bullets.clear();
    resetPaddle(); initBricks(); initBall();
    gameState=PLAYING;
}

void spawnPerk(float x, float y, PerkType type) {
    if(type==PERK_NONE) return;
    Perk p; p.x=x; p.y=y; p.dy=-PERK_SPEED; p.type=type; p.active=true;
    switch(type) {
        case PERK_EXTRA_LIFE:     p.r=0;p.g=1;p.b=0;   break;
        case PERK_FASTER_BALL:    p.r=1;p.g=.3f;p.b=0; break;
        case PERK_WIDER_PADDLE:   p.r=0;p.g=.5f;p.b=1; break;
        case PERK_FIREBALL:       p.r=1;p.g=.4f;p.b=0; break;
        case PERK_DEATH:          p.r=.8f;p.g=0;p.b=.8f; break;
        case PERK_SMALLER_PADDLE: p.r=1;p.g=0;p.b=.5f; break;
        case PERK_SHOOT:          p.r=1;p.g=1;p.b=0;   break;
        default: break;
    }
    perks.push_back(p);
}

void applyPerk(PerkType type) {
    switch(type) {
        case PERK_EXTRA_LIFE:
            lives++; playSound(SND_PERK); break;
        case PERK_FASTER_BALL: {
            ball.speed+=1.5f;
            float m=sqrtf(ball.dx*ball.dx+ball.dy*ball.dy);
            if(m>0){ball.dx=ball.dx/m*ball.speed;ball.dy=ball.dy/m*ball.speed;}
            playSound(SND_PERK); break;
        }
        case PERK_WIDER_PADDLE:
            widerPaddleActive=true; widerPaddleTimer=10;
            smallerPaddleActive=false;
            paddle.width=PADDLE_WIDTH_DEFAULT*1.7f; paddleWidth=paddle.width;
            playSound(SND_PERK); break;
        case PERK_FIREBALL:
            ball.isFireball=true; fireballActive=true;
            fireballTimer=8; ball.fireTimer=8;
            playSound(SND_FIREBALL); break;
        case PERK_DEATH:
            flashTimer=.5f; flashR=1;flashG=0;flashB=0;
            playSound(SND_DEATH);
            lives--;
            if(lives<=0) {
                addHighScore(score,gameTime,currentLevel);
                playSound(SND_GAMEOVER);
                gameStarted=false; gameState=GAME_OVER;
            } else initBall();
            break;
        case PERK_SMALLER_PADDLE:
            smallerPaddleActive=true; smallerPaddleTimer=8;
            widerPaddleActive=false;
            paddle.width=PADDLE_WIDTH_DEFAULT*0.5f; paddleWidth=paddle.width;
            playSound(SND_PERK); break;
        case PERK_SHOOT:
            shootActive=true; shootTimer=12; bulletCooldown=0;
            playSound(SND_PERK); break;
        default: break;
    }
}

bool ballBrickCollide(Brick& bk, bool pierce) {
    if(!bk.active) return false;
    float bl=ball.x-BALL_RADIUS, br=ball.x+BALL_RADIUS;
    float bb=ball.y-BALL_RADIUS, bt=ball.y+BALL_RADIUS;
    if(br<bk.x||bl>bk.x+BRICK_WIDTH)  return false;
    if(bt<bk.y||bb>bk.y+BRICK_HEIGHT) return false;
    if(!pierce) {
        float ol=br-bk.x, or2=bk.x+BRICK_WIDTH-bl;
        float ob=bt-bk.y, ot=bk.y+BRICK_HEIGHT-bb;
        float mx=std::min(ol,or2), my=std::min(ob,ot);
        if(mx<my) ball.dx=-ball.dx; else ball.dy=-ball.dy;
    }
    return true;
}

bool bulletBrickCollide(Bullet& blt, Brick& bk) {
    if(!bk.active||!blt.active) return false;
    float bx=blt.x-BULLET_WIDTH/2, by=blt.y;
    if(bx+BULLET_WIDTH<bk.x||bx>bk.x+BRICK_WIDTH)   return false;
    if(by+BULLET_HEIGHT<bk.y||by>bk.y+BRICK_HEIGHT)  return false;
    return true;
}

void updateGame(float dt) {
    if(gameState!=PLAYING) return;
    gameTime+=dt;
    if(flashTimer>0) flashTimer-=dt;
    ball.speed+=BALL_SPEED_INCREMENT;

    if(fireballActive) {
        fireballTimer-=dt; ball.fireTimer-=dt;
        if(fireballTimer<=0) { fireballActive=false; ball.isFireball=false; ball.fireTimer=0; }
    }
    if(widerPaddleActive) {
        widerPaddleTimer-=dt;
        if(widerPaddleTimer<=0) {
            widerPaddleActive=false;
            if(!smallerPaddleActive) { paddle.width=PADDLE_WIDTH_DEFAULT; paddleWidth=PADDLE_WIDTH_DEFAULT; }
        }
    }
    if(smallerPaddleActive) {
        smallerPaddleTimer-=dt;
        if(smallerPaddleTimer<=0) {
            smallerPaddleActive=false;
            if(!widerPaddleActive) { paddle.width=PADDLE_WIDTH_DEFAULT; paddleWidth=PADDLE_WIDTH_DEFAULT; }
        }
    }
    if(shootActive) {
        shootTimer-=dt; bulletCooldown-=dt;
        if(shootTimer<=0) { shootActive=false; bullets.clear(); }
    }

    if(keyLeft)  { paddle.x-=PADDLE_SPEED; if(paddle.x<0) paddle.x=0; }
    if(keyRight) { paddle.x+=PADDLE_SPEED; if(paddle.x+paddle.width>WINDOW_WIDTH) paddle.x=WINDOW_WIDTH-paddle.width; }

    if(ballOnPaddle) {
        ball.x=paddle.x+paddle.width/2.0f;
        ball.y=PADDLE_Y+PADDLE_HEIGHT+BALL_RADIUS+1.0f;
        return;
    }

    float mag=sqrtf(ball.dx*ball.dx+ball.dy*ball.dy);
    if(mag>0) { ball.dx=ball.dx/mag*ball.speed; ball.dy=ball.dy/mag*ball.speed; }

    ball.x+=ball.dx; ball.y+=ball.dy;

    if(ball.x-BALL_RADIUS<0)            { ball.x=BALL_RADIUS;             ball.dx= fabsf(ball.dx); }
    if(ball.x+BALL_RADIUS>WINDOW_WIDTH) { ball.x=WINDOW_WIDTH-BALL_RADIUS; ball.dx=-fabsf(ball.dx); }
    if(ball.y+BALL_RADIUS>WINDOW_HEIGHT){ ball.y=WINDOW_HEIGHT-BALL_RADIUS; ball.dy=-fabsf(ball.dy); }

    if(ball.dy<0 &&
       ball.y-BALL_RADIUS<=PADDLE_Y+PADDLE_HEIGHT &&
       ball.y-BALL_RADIUS>=PADDLE_Y-4 &&
       ball.x>=paddle.x && ball.x<=paddle.x+paddle.width) {
        playSound(SND_PADDLE);
        ball.dy=fabsf(ball.dy);
        float hit=(ball.x-paddle.x)/paddle.width;
        float ang=(hit-0.5f)*2.0f;
        ball.dx=ball.speed*ang*0.85f;
        float nm=sqrtf(ball.dx*ball.dx+ball.dy*ball.dy);
        if(nm>0){ball.dx=ball.dx/nm*ball.speed; ball.dy=ball.dy/nm*ball.speed;}
        if(fabsf(ball.dy)<0.8f) ball.dy=(ball.dy<0)?-0.8f:0.8f;
    }

    if(ball.y-BALL_RADIUS<0) {
        playSound(SND_DEATH);
        lives--;
        if(lives<=0) {
            addHighScore(score,gameTime,currentLevel);
            playSound(SND_GAMEOVER);
            gameStarted=false; gameState=GAME_OVER;
        } else initBall();
        return;
    }

    bool pierce=ball.isFireball;
    for(auto& bk:bricks) {
        if(!bk.active) continue;
        if(ballBrickCollide(bk,pierce)) {
            playSound(SND_BRICK);
            bk.health--;
            if(bk.health<=0) {
                bk.active=false; score+=10*currentLevel;
                spawnPerk(bk.x+BRICK_WIDTH/2,bk.y+BRICK_HEIGHT/2,bk.perk);
            } else { bk.r*=.75f;bk.g*=.75f;bk.b*=.75f; score+=5; }
            if(!pierce) break;
        }
    }

    int alive=0; for(auto&bk:bricks) if(bk.active) alive++;
    if(alive==0) {
        score+=100*currentLevel;
        if(currentLevel>=MAX_LEVELS) {
            addHighScore(score,gameTime,currentLevel);
            playSound(SND_WIN);
            gameStarted=false;
        }
        gameState=WIN; return;
    }

    for(auto&p:perks) {
        if(!p.active) continue;
        p.y+=p.dy;
        if(p.y<=PADDLE_Y+PADDLE_HEIGHT && p.y>=PADDLE_Y-PERK_HEIGHT &&
           p.x+PERK_WIDTH>=paddle.x && p.x<=paddle.x+paddle.width) {
            p.active=false; applyPerk(p.type);
        }
        if(p.y<-PERK_HEIGHT) p.active=false;
    }

    for(auto&blt:bullets) {
        if(!blt.active) continue;
        blt.y+=BULLET_SPEED;
        if(blt.y>WINDOW_HEIGHT) { blt.active=false; continue; }
        for(auto&bk:bricks) {
            if(!bk.active) continue;
            if(bulletBrickCollide(blt,bk)) {
                blt.active=false; bk.health--;
                if(bk.health<=0) {
                    bk.active=false; score+=8*currentLevel;
                    spawnPerk(bk.x+BRICK_WIDTH/2,bk.y+BRICK_HEIGHT/2,bk.perk);
                } else { bk.r*=.75f;bk.g*=.75f;bk.b*=.75f; }
                break;
            }
        }
    }
}

void drawPaddle() {
    float px=paddle.x, py=paddle.y, pw=paddle.width, ph=PADDLE_HEIGHT;

    float pr=0.3f, pg=0.6f, pb=1.0f;
    if(shootActive)         { pr=1.0f; pg=1.0f; pb=0.0f; }
    if(smallerPaddleActive) { pr=1.0f; pg=0.2f; pb=0.5f; }
    if(widerPaddleActive)   { pr=0.0f; pg=0.8f; pb=1.0f; }

    glPointSize(1.0f);
    drawBresenhamRect(px, py, pw, ph, pr, pg, pb);

    float hr=std::min(pr+0.4f,1.0f), hg=std::min(pg+0.3f,1.0f);
    drawBresenhamRect(px+2, py+ph-4, pw-4, 3, hr, hg, 1.0f);
    drawBresenhamRectOutline(px, py, pw, ph, 1.0f, 1.0f, 1.0f);

    if(shootActive) {
        drawBresenhamRect(px+2,      py+ph, 8, 6, 1.0f, 0.8f, 0.0f);
        drawBresenhamRect(px+pw-10,  py+ph, 8, 6, 1.0f, 0.8f, 0.0f);
    }
}

void drawBall() {
    if(!ball.active) return;

    int bx=(int)ball.x;
    int by=(int)ball.y;
    int br=(int)BALL_RADIUS;

    glPointSize(1.0f);

    if(ball.isFireball) {
        glColor3f(1.0f, 0.3f, 0.0f);
        drawMidpointCircleFilled(bx, by, br+5);
        glColor3f(1.0f, 0.6f, 0.0f);
        drawMidpointCircleFilled(bx, by, br+3);
        glColor3f(1.0f, 1.0f, 0.3f);
        drawMidpointCircleFilled(bx, by, br);
        glColor3f(1.0f, 0.5f, 0.0f);
        drawMidpointCircle(bx, by, br+5);

    } else {
        glColor3f(0.0f, 0.0f, 0.0f);
        drawMidpointCircleFilled(bx+2, by-2, br);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawMidpointCircleFilled(bx, by, br);
        glColor3f(0.85f, 0.85f, 1.0f);
        drawMidpointCircleFilled(bx-3, by+3, (int)(BALL_RADIUS*0.35f));
        glColor3f(0.6f, 0.6f, 0.8f);
        drawMidpointCircle(bx, by, br);
    }
}


void drawBackground() {
    glBegin(GL_QUADS);
    glColor3f(0,0,.15f); glVertex2f(0,0); glVertex2f(WINDOW_WIDTH,0);
    glColor3f(0,0,.30f); glVertex2f(WINDOW_WIDTH,WINDOW_HEIGHT); glVertex2f(0,WINDOW_HEIGHT);
    glEnd();
}

void drawBricks() {
    for(auto&bk:bricks) {
        if(!bk.active) continue;
        if(bk.isWall) {
            drawRect(bk.x,bk.y,BRICK_WIDTH,BRICK_HEIGHT,bk.r,bk.g,bk.b);
            drawRect(bk.x,bk.y+BRICK_HEIGHT/2-1,BRICK_WIDTH,2,.25f,.15f,.1f);
            drawRect(bk.x,bk.y,BRICK_WIDTH,BRICK_HEIGHT,.1f,.05f,0,false);
            for(int h=0;h<bk.health&&h<4;h++)
                drawCircleGL(bk.x+8+h*10,bk.y+BRICK_HEIGHT/2,3,1,1,0);
        } else {
            drawRect(bk.x,bk.y,BRICK_WIDTH,BRICK_HEIGHT,bk.r,bk.g,bk.b);
            drawRect(bk.x+1,bk.y+BRICK_HEIGHT-4,BRICK_WIDTH-2,3,
                     std::min(bk.r+.3f,1.f),std::min(bk.g+.3f,1.f),std::min(bk.b+.3f,1.f));
            drawRect(bk.x,bk.y,BRICK_WIDTH,BRICK_HEIGHT,0,0,0,false);
            if(bk.perk!=PERK_NONE) {
                float ccx=bk.x+BRICK_WIDTH/2, ccy=bk.y+BRICK_HEIGHT/2;
                switch(bk.perk) {
                    case PERK_EXTRA_LIFE:     drawCircleGL(ccx,ccy,4,0,1,0);    break;
                    case PERK_FASTER_BALL:    drawCircleGL(ccx,ccy,4,1,.3f,0);  break;
                    case PERK_WIDER_PADDLE:   drawCircleGL(ccx,ccy,4,0,.5f,1);  break;
                    case PERK_FIREBALL:       drawCircleGL(ccx,ccy,4,1,.6f,0);  break;
                    case PERK_DEATH:          drawCircleGL(ccx,ccy,4,.8f,0,.8f);break;
                    case PERK_SMALLER_PADDLE: drawCircleGL(ccx,ccy,4,1,0,.5f);  break;
                    case PERK_SHOOT:          drawCircleGL(ccx,ccy,4,1,1,0);    break;
                    default: break;
                }
            }
        }
    }
}

void drawPerks() {
    for(auto&p:perks) {
        if(!p.active) continue;
        drawRect(p.x-PERK_WIDTH/2,p.y-PERK_HEIGHT/2,PERK_WIDTH,PERK_HEIGHT,p.r,p.g,p.b);
        drawRect(p.x-PERK_WIDTH/2,p.y-PERK_HEIGHT/2,PERK_WIDTH,PERK_HEIGHT,1,1,1,false);
        std::string lbl="?";
        switch(p.type){
            case PERK_EXTRA_LIFE:lbl="L";break; case PERK_FASTER_BALL:lbl="F";break;
            case PERK_WIDER_PADDLE:lbl="W";break; case PERK_FIREBALL:lbl="B";break;
            case PERK_DEATH:lbl="X";break; case PERK_SMALLER_PADDLE:lbl="S";break;
            case PERK_SHOOT:lbl="G";break; default:break;
        }
        drawText(p.x-4,p.y-6,lbl,1,1,1,GLUT_BITMAP_HELVETICA_12);
    }
}

void drawBullets() {
    for(auto&blt:bullets) {
        if(!blt.active) continue;
        drawRect(blt.x-BULLET_WIDTH/2,blt.y,BULLET_WIDTH,BULLET_HEIGHT,1,1,0);
        drawRect(blt.x-BULLET_WIDTH/2,blt.y+BULLET_HEIGHT-3,BULLET_WIDTH,3,1,.5f,0);
    }
}

void drawHUD() {
    drawRect(0,WINDOW_HEIGHT-40,WINDOW_WIDTH,40,0,0,.2f);
    drawRect(0,WINDOW_HEIGHT-41,WINDOW_WIDTH,2,.3f,.6f,1);
    drawText(10,WINDOW_HEIGHT-25,"Lives:",.8f,.8f,1);
    for(int i=0;i<lives&&i<7;i++)
        drawCircleGL(80+i*22,WINDOW_HEIGHT-20,8,1,.3f,.3f);
    drawText(200,WINDOW_HEIGHT-25,"Score: "+iStr(score),1,1,0);
    drawText(360,WINDOW_HEIGHT-25,"Time: "+fStr(gameTime)+"s",.5f,1,.5f);
    drawText(510,WINDOW_HEIGHT-25,"Spd: "+fStr(ball.speed,1),1,.5f,0);
    drawText(630,WINDOW_HEIGHT-25,"Lvl: "+iStr(currentLevel)+"/"+iStr(MAX_LEVELS),.8f,.8f,1);
    std::string st=soundEnabled?"SND:ON":"SND:OFF";
    drawText(740,WINDOW_HEIGHT-25,st,soundEnabled?0:1,soundEnabled?1:0,0,GLUT_BITMAP_HELVETICA_12);

    int hy=10;
    if(widerPaddleActive)   drawText(10, hy,"WIDE:"+fStr(widerPaddleTimer,1)+"s",0,.8f,1,GLUT_BITMAP_HELVETICA_12);
    if(smallerPaddleActive) drawText(110,hy,"SMALL:"+fStr(smallerPaddleTimer,1)+"s",1,0,.5f,GLUT_BITMAP_HELVETICA_12);
    if(shootActive)         drawText(220,hy,"GUN:"+fStr(shootTimer,1)+"s",1,1,0,GLUT_BITMAP_HELVETICA_12);
    if(fireballActive)      drawText(330,hy,"FIRE:"+fStr(fireballTimer,1)+"s",1,.5f,0,GLUT_BITMAP_HELVETICA_12);
    drawText(450,hy,"[L]Life [F]Fast [W]Wide [B]Fire [X]Death [S]Small [G]Gun",
             .5f,.5f,.5f,GLUT_BITMAP_HELVETICA_12);
}

void drawPauseOverlay() {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,.65f);
    glBegin(GL_QUADS);
    glVertex2f(0,0);glVertex2f(WINDOW_WIDTH,0);
    glVertex2f(WINDOW_WIDTH,WINDOW_HEIGHT);glVertex2f(0,WINDOW_HEIGHT);
    glEnd(); glDisable(GL_BLEND);

    drawRect(250,180,300,255,0,0,.25f);
    drawRect(250,180,300,255,0,.8f,1,false);
    drawTextL(328,403,"PAUSED",1,1,0);

    std::string sm=soundEnabled?"Sound: ON  (M)":"Sound: OFF (M)";
    drawText(285,375,sm,soundEnabled?0:1,soundEnabled?1:0,0,GLUT_BITMAP_HELVETICA_12);

    const char* opts[]={"RESUME","MAIN MENU"};
    for(int i=0;i<2;i++) {
        float by=323-(float)i*65;
        if(selectedPauseMenu==i) {
            drawRect(275,by-8,250,44,0,.4f,.8f);
            drawRect(275,by-8,250,44,0,1,1,false);
            drawTextL(318,by+8,opts[i],1,1,1);
        } else {
            drawRect(275,by-8,250,44,.05f,.05f,.2f);
            drawRect(275,by-8,250,44,.3f,.3f,.6f,false);
            drawTextL(318,by+8,opts[i],.7f,.7f,.9f);
        }
    }
    drawText(262,192,"UP/DOWN: select   ENTER: confirm",.5f,.5f,.7f,GLUT_BITMAP_HELVETICA_12);
}

void drawMenu() {
    drawBackground();
    srand(42);
    for(int i=0;i<80;i++) {
        float sx=(float)(rand()%WINDOW_WIDTH), sy=(float)(rand()%WINDOW_HEIGHT);
        float br2=(float)(rand()%100)/100.f;
        drawCircleGL(sx,sy,1.5f,br2,br2,br2);
    }
    srand((unsigned)time(0));

    drawTextL(238,523,"DX BALL - ADVANCED",0,.3f,.6f);
    drawTextL(235,526,"DX BALL - ADVANCED",0,.8f,1);
    drawRect(100,512,600,2,0,.6f,1);
    drawText(278,494,"CSE 426 - Computer Graphics Lab",.7f,.7f,.7f,GLUT_BITMAP_HELVETICA_12);

    std::vector<std::string> items;
    if(gameStarted) items={"RESUME GAME","NEW GAME","HIGH SCORES","HOW TO PLAY","EXIT"};
    else            items={"START GAME","HIGH SCORES","HOW TO PLAY","EXIT"};
    int n=(int)items.size();
    if(selectedMenu>=n) selectedMenu=n-1;

    for(int i=0;i<n;i++) {
        float y=430-(float)i*58;
        float bx=265,by=y-15,bw=270,bh=44;
        if(selectedMenu==i) {
            drawRect(bx,by,bw,bh,0,.4f,.8f);
            drawRect(bx,by,bw,bh,0,.9f,1,false);
            drawText(bx+10,by+15,">",0,1,1);
            drawTextL(bx+28,by+11,items[i],1,1,1);
        } else {
            drawRect(bx,by,bw,bh,.05f,.05f,.2f);
            drawRect(bx,by,bw,bh,.3f,.3f,.6f,false);
            drawTextL(bx+28,by+11,items[i],.7f,.7f,.9f);
        }
    }
    std::string st=soundEnabled?"Sound: ON  (M to toggle)":"Sound: OFF (M to toggle)";
    drawText(270,70,st,soundEnabled?0:1,soundEnabled?1:0,0,GLUT_BITMAP_HELVETICA_12);
    drawText(215,48,"UP/DOWN: navigate     ENTER: select",.5f,.5f,.7f,GLUT_BITMAP_HELVETICA_12);
}

void drawHighScoreScreen() {
    drawBackground();
    drawTextL(298,548,"HIGH SCORES",0,.3f,.6f);
    drawTextL(295,550,"HIGH SCORES",0,.8f,1);
    drawRect(100,535,600,2,0,.6f,1);

    drawRect(150,492,500,30,0,.1f,.3f);
    drawRect(150,492,500,30,0,.5f,.8f,false);
    drawText(178,503,"RANK",0,.8f,1,GLUT_BITMAP_HELVETICA_12);
    drawText(270,503,"SCORE",0,.8f,1,GLUT_BITMAP_HELVETICA_12);
    drawText(390,503,"TIME",0,.8f,1,GLUT_BITMAP_HELVETICA_12);
    drawText(505,503,"LEVEL",0,.8f,1,GLUT_BITMAP_HELVETICA_12);

    if(highScores.empty()) {
        drawRect(150,370,500,90,0,.05f,.15f);
        drawRect(150,370,500,90,0,.3f,.6f,false);
        drawTextL(240,415,"No scores yet!",0.5f,0.5f,0.7f);
        drawText(252,393,"Play a game first!",0.4f,0.4f,0.6f,GLUT_BITMAP_HELVETICA_12);
    } else {
        float rc[5][3]={{1,.84f,0},{.75f,.75f,.75f},{.8f,.5f,.2f},{.7f,.7f,1},{.7f,.7f,1}};
        for(int i=0;i<(int)highScores.size();i++) {
            float ry=455-(float)i*50;
            drawRect(150,ry-15,500,42,.02f+i*.01f,.08f,.22f);
            drawRect(150,ry-15,500,42,rc[i][0]*.3f,rc[i][1]*.3f,rc[i][2]*.3f,false);
            drawCircleGL(185,ry+6,12,rc[i][0],rc[i][1],rc[i][2]);
            drawText(180,ry+1,"#"+iStr(i+1),0,0,0,GLUT_BITMAP_HELVETICA_12);
            drawText(255,ry,iStr(highScores[i].score),rc[i][0],rc[i][1],rc[i][2]);
            drawText(375,ry,fStr(highScores[i].time,1)+"s",.8f,.9f,.8f);
            drawText(505,ry,"Lvl "+iStr(highScores[i].level),.8f,.8f,1);
        }
    }
    drawRect(100,182,600,2,0,.6f,1);
    drawRect(195,148,410,34,0,.05f,.2f);
    drawRect(195,148,410,34,0,.4f,.8f,false);
    drawText(248,160,"Press ESC to return to Menu",.6f,.8f,1,GLUT_BITMAP_HELVETICA_12);
}

void drawHelpScreen() {
    drawBackground();
    drawTextL(268,548,"HOW TO PLAY",0,.8f,1);
    drawRect(100,535,600,2,0,.6f,1);
    const char* lines[]={
        "CONTROLS:",
        "  LEFT / RIGHT Arrow  :  Move Paddle",
        "  Mouse Move          :  Move Paddle",
        "  SPACE / Left-Click  :  Launch Ball  /  Fire Gun",
        "  P                   :  Pause / Resume",
        "  M                   :  Toggle Sound On / Off",
        "  ESC                 :  Pause Menu  (in-game)",
        "",
        "GAME:",
        "  Break ALL bricks to advance level  (3 levels total)",
        "  Gray/Brown bricks = Wall Brick  (needs more hits)",
        "  Score increases per brick hit and on destroy",
        "",
        "POWER-UPS  (catch falling items with paddle):",
        "  [L] GREEN   =  Extra Life  (+1 life)",
        "  [F] ORANGE  =  Faster Ball  (speed +1.5)",
        "  [W] BLUE    =  Wide Paddle  (10 seconds)",
        "  [B] FIRE    =  Fireball - pierces bricks!  (8s)",
        "  [X] PURPLE  =  DEATH - lose a life instantly!",
        "  [S] PINK    =  Smaller Paddle  (8 seconds)",
        "  [G] YELLOW  =  Gun - shoot bullets!  (12 seconds)",
    };
    int nl=sizeof(lines)/sizeof(lines[0]);
    for(int i=0;i<nl;i++)
        drawText(125,510-(float)i*22,lines[i],.9f,.9f,.9f);
    drawRect(195,25,410,32,0,.05f,.2f);
    drawRect(195,25,410,32,0,.4f,.8f,false);
    drawText(238,37,"Press ESC to return to Menu",.5f,.7f,1,GLUT_BITMAP_HELVETICA_12);
}

void drawGameOver() {
    drawBackground(); drawBricks(); drawHUD();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,.75f);
    glBegin(GL_QUADS);
    glVertex2f(0,0);glVertex2f(WINDOW_WIDTH,0);
    glVertex2f(WINDOW_WIDTH,WINDOW_HEIGHT);glVertex2f(0,WINDOW_HEIGHT);
    glEnd(); glDisable(GL_BLEND);

    drawRect(165,182,470,245,.12f,0,0);
    drawRect(165,182,470,245,1,0,0,false);
    drawTextL(260,390,"GAME  OVER",1,.2f,.2f);
    drawText(235,355,"Final Score:   "+iStr(score),1,1,.5f);
    drawText(235,327,"Level Reached: "+iStr(currentLevel),.8f,.8f,1);
    drawText(235,299,"Time Played:   "+fStr(gameTime)+" seconds",.8f,.8f,.8f);
    if(!highScores.empty()&&highScores[0].score==score)
        drawText(268,270,"*** NEW HIGH SCORE! ***",1,.84f,0);
    drawText(230,240,"Press ENTER to Play Again",.9f,.9f,.9f);
    drawText(245,214,"Press ESC  for Main Menu",.7f,.7f,.7f);
}

void drawWin() {
    drawBackground();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,.05f,0,.5f);
    glBegin(GL_QUADS);
    glVertex2f(0,0);glVertex2f(WINDOW_WIDTH,0);
    glVertex2f(WINDOW_WIDTH,WINDOW_HEIGHT);glVertex2f(0,WINDOW_HEIGHT);
    glEnd(); glDisable(GL_BLEND);

    drawRect(155,172,490,280,0,.1f,.05f);
    drawRect(155,172,490,280,0,1,.5f,false);
    if(currentLevel>MAX_LEVELS) {
        drawTextL(215,410,"ALL LEVELS COMPLETE!",0,1,.4f);
        drawText(225,378,"You are a true DX Ball Master!",.9f,1,.9f);
    } else {
        drawTextL(235,410,"LEVEL "+iStr(currentLevel-1)+" CLEAR!",0,1,.4f);
        drawText(225,378,"Get ready for Level "+iStr(currentLevel)+"!",.9f,1,.9f);
    }
    drawText(235,343,"Score: "+iStr(score),1,1,.5f);
    drawText(235,313,"Time:  "+fStr(gameTime)+" seconds",.8f,.8f,.8f);
    drawText(235,283,"Level: "+iStr(std::min(currentLevel-1,MAX_LEVELS))+"/"+iStr(MAX_LEVELS),.8f,.8f,1);
    if(currentLevel<=MAX_LEVELS) drawText(218,250,"Press ENTER for Next Level",0,1,.5f);
    else                         drawText(218,250,"Press ENTER to Play Again",0,1,.5f);
    drawText(238,220,"Press ESC  for Main Menu",.7f,.7f,.7f);
}

void drawFlash() {
    if(flashTimer<=0) return;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    float a=flashTimer*1.5f; if(a>.6f) a=.6f;
    glColor4f(flashR,flashG,flashB,a);
    glBegin(GL_QUADS);
    glVertex2f(0,0);glVertex2f(WINDOW_WIDTH,0);
    glVertex2f(WINDOW_WIDTH,WINDOW_HEIGHT);glVertex2f(0,WINDOW_HEIGHT);
    glEnd(); glDisable(GL_BLEND);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    switch(gameState) {
        case MENU:       drawMenu();            break;
        case HELP:       drawHelpScreen();      break;
        case HIGH_SCORE: drawHighScoreScreen(); break;
        case PLAYING:
            drawBackground();
            drawBricks(); drawPerks(); drawBullets();
            drawPaddle(); drawBall(); drawHUD(); drawFlash();
            if(ballOnPaddle)
                drawText(262,200,"Press SPACE or Click to launch!",1,1,0);
            break;
        case PAUSED:
            drawBackground();
            drawBricks(); drawPerks(); drawBullets();
            drawPaddle(); drawBall(); drawHUD();
            drawPauseOverlay();
            break;
        case GAME_OVER: drawGameOver(); break;
        case WIN:       drawWin();      break;
    }
    glutSwapBuffers();
}

void reshape(int w,int h) {
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0,WINDOW_WIDTH,0,WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

void timerCB(int) {
    updateGame(1.0f/60.0f);
    glutPostRedisplay();
    glutTimerFunc(16,timerCB,0);
}

void fireBullet() {
    if(!shootActive||bulletCooldown>0) return;
    Bullet b1; b1.x=paddle.x+5;               b1.y=paddle.y+PADDLE_HEIGHT+1; b1.active=true;
    Bullet b2; b2.x=paddle.x+paddle.width-5;  b2.y=b1.y;                      b2.active=true;
    bullets.push_back(b1); bullets.push_back(b2);
    bulletCooldown=0.35f;
    playSound(SND_BULLET);
}

void launchBall() {
    ballOnPaddle=false;
    ball.dy= fabsf(ball.speed*0.7f);
    ball.dx= ball.speed*0.7f;
    playSound(SND_LAUNCH);
}

int menuItemCount() { return gameStarted?5:4; }

void menuAction(int idx) {
    if(gameStarted) {
        if(idx==0) gameState=PLAYING;
        else if(idx==1){ gameStarted=false; initGame(); }
        else if(idx==2) gameState=HIGH_SCORE;
        else if(idx==3) gameState=HELP;
        else if(idx==4) exit(0);
    } else {
        if(idx==0) initGame();
        else if(idx==1) gameState=HIGH_SCORE;
        else if(idx==2) gameState=HELP;
        else if(idx==3) exit(0);
    }
}

void keyboard(unsigned char key,int,int) {
    if(key=='m'||key=='M') {
        soundEnabled=!soundEnabled;
        if(soundEnabled) playSound(SND_PERK);
        glutPostRedisplay(); return;
    }
    if(gameState==HELP||gameState==HIGH_SCORE) {
        if(key==27) gameState=MENU;
        glutPostRedisplay(); return;
    }
    switch(gameState) {
        case MENU:
            if(key==13) menuAction(selectedMenu);
            break;
        case PLAYING:
            if(key==' ') { if(ballOnPaddle) launchBall(); else if(shootActive) fireBullet(); }
            if(key=='p'||key=='P') { gameState=PAUSED; selectedPauseMenu=0; }
            if(key==27)            { gameState=PAUSED; selectedPauseMenu=0; }
            break;
        case PAUSED:
            if(key=='p'||key=='P') gameState=PLAYING;
            if(key==13) { if(selectedPauseMenu==0) gameState=PLAYING; else{ gameState=MENU; selectedMenu=0; } }
            if(key==27) gameState=PLAYING;
            break;
        case GAME_OVER:
            if(key==13){ selectedMenu=0; initGame(); }
            if(key==27){ gameState=MENU; selectedMenu=0; }
            break;
        case WIN:
            if(key==13){ if(currentLevel<=MAX_LEVELS) nextLevel(); else{ selectedMenu=0; initGame(); } }
            if(key==27){ gameState=MENU; selectedMenu=0; }
            break;
        default: break;
    }
    glutPostRedisplay();
}

void specialKeys(int key,int,int) {
    if(gameState==MENU) {
        int n=menuItemCount();
        if(key==GLUT_KEY_UP)   selectedMenu=(selectedMenu-1+n)%n;
        if(key==GLUT_KEY_DOWN) selectedMenu=(selectedMenu+1)%n;
        glutPostRedisplay(); return;
    }
    if(gameState==HELP||gameState==HIGH_SCORE){ glutPostRedisplay(); return; }
    if(gameState==PAUSED) {
        if(key==GLUT_KEY_UP||key==GLUT_KEY_DOWN)
            selectedPauseMenu=(selectedPauseMenu+1)%2;
        glutPostRedisplay(); return;
    }
    if(gameState==PLAYING) {
        if(key==GLUT_KEY_LEFT)  keyLeft =true;
        if(key==GLUT_KEY_RIGHT) keyRight=true;
    }
}

void specialKeysUp(int key,int,int) {
    if(key==GLUT_KEY_LEFT)  keyLeft =false;
    if(key==GLUT_KEY_RIGHT) keyRight=false;
}

void mouseMotion(int x,int) {
    if(gameState!=PLAYING) return;
    float nx=(float)x-paddle.width/2.0f;
    if(nx<0) nx=0;
    if(nx+paddle.width>WINDOW_WIDTH) nx=WINDOW_WIDTH-paddle.width;
    paddle.x=nx;
}

void mouseClick(int btn,int state,int x,int) {
    if(gameState==PLAYING&&btn==GLUT_LEFT_BUTTON&&state==GLUT_DOWN) {
        if(ballOnPaddle) launchBall();
        else if(shootActive) fireBullet();
    }
}

int main(int argc,char**argv) {
    srand((unsigned)time(0));
    loadHighScores();
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGHT);
    glutInitWindowPosition(100,50);
    glutCreateWindow("DX Ball Advanced - CSE 426");
    glClearColor(0,0,.1f,1);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutMouseFunc(mouseClick);
    glutTimerFunc(16,timerCB,0);
    glutMainLoop();
    return 0;
}
