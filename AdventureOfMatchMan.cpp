#include <graphics.h>
#include <conio.h>
#include <time.h>
#include "EasyXPng.h"
#include "Timer.h"
#include <vector>
#include <Windows.h>
using namespace std;

// Constants
const int WIDTH = 800;
const int HEIGHT = 600;
const int ANIMATION_FRAMES = 8;
const int JUMP_VELOCITY = -25;
const int GRAVITY = 1;
const int MOVEMENT_SPEED = 5;
const float STAR_SPAWN_CHANCE = 0.5f;

// Game states
enum GameState
{
    running,
    game_over
};

GameState gameState = running;  // Start with the game running

// Player states
enum PlayerStatus
{
    standright, runright, jumpright, die
};

// Scoring variables
int score = 0;
IMAGE im_star;

// Land class
class Land
{
public:
    IMAGE im_land;
    float left_x, right_x, top_y;
    float land_width, land_height;
    bool hasStar;

    void initialize()
    {
        loadimage(&im_land, _T("land.png"));
        land_width = im_land.getwidth();
        land_height = im_land.getheight();
        left_x = WIDTH / 2;
        right_x = left_x + land_width;
        top_y = HEIGHT / 2;

        // Each land tile has its own chance to have a star
        hasStar = ((rand() / (float)RAND_MAX) < STAR_SPAWN_CHANCE);
    }

    void draw()
    {
        putimage(left_x, top_y, &im_land);

        // Display the star if this land has one, using transparency
        if (hasStar)
        {
            putimagePng(left_x + 10, top_y - 40, &im_star);
        }
    }

};


// Scene class
class Scene
{
public:
    IMAGE im_bk[2]; // Use two images for seamless scrolling
    float bg_x[2];  // X positions for the background images
    float bg_width;  // Width of the background images
    vector<Land> lands;

    void initialize()
    {
        loadimage(&im_bk[0], _T("landscape1.png"));
        loadimage(&im_bk[1], _T("landscape2.png"));
        bg_width = im_bk[0].getwidth();

        bg_x[0] = 0;
        bg_x[1] = bg_width;
        lands.clear();

        for (int i = 0; i < 10; i++)
        {
            Land land;
            land.initialize();
            land.left_x = i * land.land_width;
            land.right_x = land.left_x + land.land_width;
            land.top_y = HEIGHT / 2 + rand() % 2 * HEIGHT / 10;
            lands.push_back(land);
        }
    }


    void drawScore()
    {
        TCHAR scoreText[20];
        _stprintf_s(scoreText, _T("Score: %d"), score);
        setbkcolor(WHITE);
        clearrectangle(WIDTH - 150, 10, WIDTH - 10, 50);
        settextcolor(BLACK);
        outtextxy(WIDTH - 140, 20, scoreText);
    }

    void draw()
    {
        for (int i = 0; i < 2; i++)
        {
            putimage(bg_x[i], 0, &im_bk[i]);
        }
        for (size_t i = 0; i < lands.size(); i++)
        {
            lands[i].draw();
        }
    }

    void updateBackground()
    {
        for (int i = 0; i < 2; i++)
        {
            bg_x[i] -= 1;

            if (bg_x[i] <= -bg_width)
            {
                bg_x[i] += bg_width * 2;
            }
        }
    }

    void updateLands()
    {
        for (size_t i = 0; i < lands.size(); i++)
        {
            lands[i].left_x -= MOVEMENT_SPEED;
            lands[i].right_x -= MOVEMENT_SPEED;

            if (lands[i].right_x < 0)
            {
                Land recycled_land = lands[i];
                recycled_land.left_x = lands.back().right_x;
                recycled_land.right_x = recycled_land.left_x + recycled_land.land_width;
                recycled_land.top_y = HEIGHT / 2 + rand() % 2 * HEIGHT / 10;

                // Reassign the star chance for the recycled land
                recycled_land.hasStar = ((rand() / (float)RAND_MAX) < STAR_SPAWN_CHANCE);

                lands.erase(lands.begin() + i);
                lands.push_back(recycled_land);
            }

        }
    }
};

// Player class
class Player
{
public:
    IMAGE im_show;
    IMAGE im_standright;
    IMAGE im_jumpright;
    IMAGE im_die;
    vector<IMAGE> ims_runright;
    int animId;
    PlayerStatus playerStatus;
    float x_left; // Only y position will change
    float y_bottom;
    float vy;
    float gravity;
    float width, height;
    bool isJumpPressed;

    void initialize()
    {
        ims_runright.clear();

        loadimage(&im_standright, _T("standright.png"));
        loadimage(&im_jumpright, _T("jumpright.png"));
        loadimage(&im_die, _T("die.png"));

        playerStatus = standright;
        im_show = im_standright;
        width = im_standright.getwidth();
        height = im_standright.getheight();

        TCHAR filename[80];
        for (int i = 0; i < ANIMATION_FRAMES; i++)
        {
            swprintf_s(filename, 80, _T("runright%d.png"), i);
            IMAGE im;
            loadimage(&im, filename);
            ims_runright.push_back(im);
        }

        animId = 0;
        x_left = 0; // Fixed X position in the center
        y_bottom = HEIGHT / 2;
        vy = 0;
        gravity = GRAVITY;
        isJumpPressed = false;
    }

    void draw()
    {
        putimagePng(x_left, y_bottom - height, &im_show);
    }

    bool isOnLand(Land& land, float ySpeed)
    {
        float x_right = x_left + width;
        if (ySpeed <= 0)
            ySpeed = 0;

        return (land.left_x - x_left <= width * 0.6 &&
            x_right - land.right_x <= width * 0.6 &&
            abs(y_bottom - land.top_y) <= 5 + ySpeed);
    }

    bool isNotOnAllLands(vector<Land>& lands, float speed)
    {
        for (size_t i = 0; i < lands.size(); i++)
        {
            if (isOnLand(lands[i], speed))
                return false;
        }
        return true;
    }

    void runRight(Scene& scene)
    {
        if (isNotOnAllLands(scene.lands, vy))
        {
            im_show = im_jumpright;
            playerStatus = jumpright;
            return;
        }

        if (playerStatus == jumpright)
        {
            im_show = im_jumpright;
        }
        else
        {
            if (playerStatus != runright)
            {
                playerStatus = runright;
                animId = 0;
            }
            else
            {
                animId = (animId + 1) % ANIMATION_FRAMES;
            }
            im_show = ims_runright[animId];
        }
    }

    void beginJump()
    {
        if (playerStatus != jumpright && !isJumpPressed)
        {
            im_show = im_jumpright;
            playerStatus = jumpright;
            vy = JUMP_VELOCITY;
            isJumpPressed = true;
        }
    }

    void releaseJump()
    {
        isJumpPressed = false;
    }

    void updateYcoordinate(Scene& scene)
    {
        if (playerStatus == jumpright)
        {
            vy += gravity;
            y_bottom += vy;

            for (size_t i = 0; i < scene.lands.size(); i++)
            {
                if (isOnLand(scene.lands[i], vy))
                {
                    y_bottom = scene.lands[i].top_y;
                    playerStatus = standright;
                    return;
                }
            }

            // Check if the player has fallen below the screen
            if (y_bottom > HEIGHT)
            {
                playerStatus = die;
                im_show = im_die;
                gameState = game_over;  // Set game state to game over
            }
        }
    }

    bool isKeyPressed(int virtualKey)
    {
        return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    }

    void updatePlayerPosition(Scene& scene)
    {
        runRight(scene);

        if (isKeyPressed(VK_SPACE))
        {
            beginJump();
        }
        else
        {
            releaseJump();
        }
    }


    bool collectStar(vector<Land>& lands)
    {
        for (size_t i = 0; i < lands.size(); i++)
        {
            if (lands[i].hasStar &&
                x_left >= lands[i].left_x - width * 0.6 &&
                x_left <= lands[i].right_x + width * 0.6 &&
                y_bottom >= lands[i].top_y - height)
            {
                lands[i].hasStar = false;
                return true;
            }
        }
        return false;
    }
};



// Function to display the game-over screen
void showGameOverScreen()
{
    cleardevice();  // Clear the screen
    setbkcolor(BLACK);  // Set background color to black
    cleardevice();  // Clear screen with black background

    // Display Game Over message and score
    settextstyle(40, 0, _T("Arial"));
    settextcolor(RED);
    outtextxy(WIDTH / 2 - 100, HEIGHT / 2 - 100, _T("Game Over"));

    TCHAR scoreText[20];
    _stprintf_s(scoreText, _T("Score: %d"), score);
    settextcolor(WHITE);
    outtextxy(WIDTH / 2 - 60, HEIGHT / 2, scoreText);

    // Display instructions to restart or exit
    settextstyle(20, 0, _T("Arial"));
    outtextxy(WIDTH / 2 - 120, HEIGHT / 2 + 80, _T("Press Enter to Restart or Esc to Exit"));
    FlushBatchDraw();
}



// Global variables
Player player;
Scene scene;
Timer timer;

void startup()
{
    srand(static_cast<unsigned>(time(nullptr)));
    initgraph(WIDTH, HEIGHT);
    scene.initialize();
    player.initialize();
    loadimage(&im_star, _T("star.png"));
    BeginBatchDraw();
}

void show()
{
    cleardevice();
    scene.draw();
    player.draw();
    scene.drawScore(); // Draw the score
    FlushBatchDraw();
    timer.Sleep(50);
}

void updateWithoutInput()
{
    player.updateYcoordinate(scene);
    player.updatePlayerPosition(scene);

    if (player.collectStar(scene.lands))
    {
        score++;
    }
}

void updateWithInput() { }

void resetGame()
{
    score = 0;
    player.initialize();
    scene.initialize();
    gameState = running;
}

int main()
{
    startup();

    while (true)
    {
        if (gameState == running)
        {
            show();
            updateWithoutInput();
            scene.updateLands();
            scene.updateBackground();

            // Check for game over
            if (player.playerStatus == die)
            { 
                gameState = game_over;
            }
        }
        else if (gameState == game_over)
        {
            showGameOverScreen();

            // Handle restart or exit
            if (GetAsyncKeyState(VK_RETURN) & 0x8000)
            {
                resetGame();  // Reset the game if Enter is pressed
            }
            else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            {
                break;  // Exit the game if Esc is pressed
            }
        }
    }

    closegraph();
    return 0;
}