#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#endif
#include <time.h>
#include <cstdlib>

#define length 24 // 地圖的列數
#define width 30  // 地圖的行數

// 操控鍵
#define UP 'w'    // 上
#define DOWN 's'  // 下
#define LEFT 'a'  // 左
#define RIGHT 'd' // 右

// 全域變數
int i, j, k, sp, score;
char ch = UP, state = UP, choo = 0, n; // 初始化方向
int grow = 0;
int defacol = 10; // 定義各個資訊的顏色
int wallcol = 12;
int foodcol = 14;
int snakecol = 16;
int wordcol = 13;
int colcol = 11;
int sp_col = 0xf4;

struct Food
{
    int x;   // 橫坐標
    int y;   // 縱坐標
    char cc; // 儲存食物種類
} food;

struct Snake
{
    int x[1000];
    int y[1000];
    int len;   // 長度
    int speed; // 速度
} snake;

// 函數宣告
void setColor(int color = defacol);
void create_map(void);
void create_food(void);
void move_snake(void);
int alive(void);
void Cursor_gotoxy(int x, int y);
void hideCursor(void);
void showCursor(void);
#ifndef _WIN32
int _kbhit(void);
int _getch(void);
void Sleep(int ms);
void clearScreen(void);
static struct termios g_old_termios;
static int g_termios_ready = 0;
void initTerminal(void);
void restoreTerminal(void);
#endif

int main(void)
{
#ifdef _WIN32
    system("chcp 65001 > nul"); // 切換終端機為 UTF-8 以顯示中文
#else
    initTerminal();
#endif
    setColor();
    int nnn = 1;
    do
    {
        nnn = 1;
        ch = UP;
        state = UP;
        grow = 0;
        score = 0; // 初始化分數為0
        n = 0;

        for (;;)
        {
            setColor(wordcol);
            printf("\t選擇遊戲模式:\n\t1.簡單\t2.中等\t3.困難\t\n");
            fflush(stdout);
            n = _getch();

            if (n == '\r' || n == '\n' || n == ' ')
                continue;
            if (n == 27)
            {
                while (_kbhit())
                    _getch();
                continue;
            }
            if (n == '[' || n == 'A' || n == 'B' || n == 'C' || n == 'D')
                continue;
            if (n == '1' || n == '2' || n == '3')
                break;

            printf("\n    Error!! Re-enter options\n");
            fflush(stdout);
        }
        setColor(defacol);

        switch (n)
        {
        case '1':
            sp = 300;
            break;
        case '2':
            sp = 230;
            break;
        case '3':
            sp = 140;
            break;
        default:
            sp = 300;
            break;
        }

        clearScreen();
        create_map();

        // 開始遊戲
        for (;;)
        {
            snake.speed = sp;
            hideCursor();
            create_food();
            move_snake();
            Sleep(snake.speed);

            if (!(alive()))
            {
                break; // 如果死了，跳出迴圈
            }
            if (score >= 500)
            { // 分數大於500，就獲勝
                nnn = 2;
                break;
            }
        }

        showCursor();
        Cursor_gotoxy(length / 2, width / 2 - 5);
        setColor(wordcol);

        if (nnn == 0)
        {
            clearScreen();
            Cursor_gotoxy(8, 5);
            choo = 0;
            printf("Game Over!\n");
            printf("\t1.重新開始\t2.離開\n");
            do
            {
                choo = _getch();
            } while (choo != '1' && choo != '2' && choo != '\r' && choo != '\n');
            if (choo == '\r' || choo == '\n')
                choo = '1';
        }
        else if (nnn == 1)
        {
            clearScreen();
            Cursor_gotoxy(8, 5);
            choo = 0;
            printf("請選擇!\n");
            printf("\t1.重新開始\t2.離開\n");
            do
            {
                choo = _getch();
            } while (choo != '1' && choo != '2' && choo != '\r' && choo != '\n');
            if (choo == '\r' || choo == '\n')
                choo = '1';
        }
        else if (nnn == 2)
        {
            clearScreen();
            Cursor_gotoxy(8, 5);
            choo = 0;
            printf("Congratulations ! ! !\n");
            printf("\tYOU WIN THE GAME ! ! !\n");
            printf("\t1.重新開始\t2.離開\n");
            do
            {
                choo = _getch();
            } while (choo != '1' && choo != '2' && choo != '\r' && choo != '\n');
            if (choo == '\r' || choo == '\n')
                choo = '1';
        }
    } while (choo == '1');
#ifndef _WIN32
    restoreTerminal();
#endif

    return 0;
}

void setColor(int color)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#else
    // ANSI color fallback for non-Windows terminals.
    switch (color)
    {
    case 12:
        printf("\033[31m");
        break; // red
    case 14:
        printf("\033[33m");
        break; // yellow
    case 13:
        printf("\033[35m");
        break; // magenta
    case 11:
        printf("\033[36m");
        break; // cyan
    case 16:
        printf("\033[32m");
        break; // green
    case 0xf4:
        printf("\033[1;33m");
        break; // bright yellow
    default:
        printf("\033[0m");
        break;
    }
#endif
}

void hideCursor(void)
{
#ifdef _WIN32
    CONSOLE_CURSOR_INFO cursor_info = {1, 0};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
#else
    printf("\033[?25l");
#endif
}

void showCursor(void)
{
#ifdef _WIN32
    CONSOLE_CURSOR_INFO cursor_info = {1, 1};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
#else
    printf("\033[?25h");
#endif
}

void create_map(void)
{
    int f, min = 1, max = 3;
    srand(time(NULL));

    food.x = rand() % (length - 2) + 1;
    food.y = rand() % (width - 2) + 1;
    Cursor_gotoxy(food.x, food.y);
    f = rand() % (max - min + 1) + min;
    setColor(foodcol);

    if (f == 1)
    {
        printf("$");
        food.cc = '$';
    }
    else if (f == 2)
    {
        printf("@");
        food.cc = '@';
    }
    else if (f == 3)
    {
        printf("?");
        food.cc = '?';
    }

    setColor(snakecol);
    snake.x[0] = length / 2;
    snake.y[0] = width / 2;
    Cursor_gotoxy(snake.x[0], snake.y[0]);
    printf("+");
    snake.len = 3;
    snake.speed = 200;

    for (k = 1; k < snake.len; k++)
    {
        snake.x[k] = snake.x[k - 1] + 1;
        snake.y[k] = snake.y[k - 1];
        Cursor_gotoxy(snake.x[k], snake.y[k]);
        printf("+");
    }

    setColor(wallcol);
    for (j = 0; j < width; j++)
    {
        Cursor_gotoxy(0, j);
        printf("#");
        Cursor_gotoxy(length - 1, j);
        printf("#");
    }
    for (i = 0; i < length - 1; i++)
    {
        Cursor_gotoxy(i, 0);
        printf("#");
        Cursor_gotoxy(i, width - 1);
        printf("#");
    }

    Cursor_gotoxy(2, width + 3);
    setColor(wordcol);
    if (n == '1')
        printf("難度: 簡單");
    else if (n == '2')
        printf("難度: 中等");
    else if (n == '3')
        printf("難度: 困難");

    setColor(colcol);
    Cursor_gotoxy(4, width + 3);
    printf("上: W");
    Cursor_gotoxy(6, width + 3);
    printf("下: S");
    Cursor_gotoxy(8, width + 3);
    printf("左: A");
    Cursor_gotoxy(10, width + 3);
    printf("右: D");

    setColor(sp_col);
    Cursor_gotoxy(12, width + 3);
    printf("獲得分數:%d", score);

    setColor(colcol);
    Cursor_gotoxy(14, width + 3);
    printf("規則:分數達到500分就獲勝了 ! ! !");
    Cursor_gotoxy(16, width + 3);
    printf("食物:$,得10分，蛇身增長 1 個單位");
    Cursor_gotoxy(18, width + 3);
    printf("食物:@,得20分，蛇身增長 2 個單位");
    Cursor_gotoxy(20, width + 3);
    printf("食物:?,得30分，蛇身增長 3 個單位");
    setColor(defacol);
}

void create_food(void)
{
    int f, min = 1, max = 3;
    f = rand() % (max - min + 1) + min;
    setColor(defacol);

    if (snake.x[0] == food.x && snake.y[0] == food.y)
    {
        if (food.cc == '$')
        {
            score += 10;
            snake.len++;
        }
        else if (food.cc == '@')
        {
            score += 20;
            snake.len += 2;
        }
        else if (food.cc == '?')
        {
            score += 30;
            snake.len += 3;
        }

        Cursor_gotoxy(12, width + 3);
        setColor(sp_col);
        printf("獲得分數:%d", score);
        setColor(defacol);

        int flag = 1;
        do
        {
            food.x = rand() % (length - 2) + 1;
            food.y = rand() % (width - 2) + 1;
            flag = 1;
            for (i = 0; i < snake.len; i++)
            {
                if (food.x == snake.x[i] && food.y == snake.y[i])
                {
                    flag = 0;
                    break;
                }
            }
        } while (flag == 0);

        Cursor_gotoxy(food.x, food.y);
        setColor(foodcol);
        if (f == 1)
        {
            printf("$");
            food.cc = '$';
        }
        else if (f == 2)
        {
            printf("@");
            food.cc = '@';
        }
        else if (f == 3)
        {
            printf("?");
            food.cc = '?';
        }
        setColor(defacol);
        grow = 1;
    }
}

void move_snake(void)
{
    while (_kbhit())
    { // 現代編譯器需加底線
        ch = _getch();
    }
    if (!grow)
    {
        Cursor_gotoxy(snake.x[snake.len - 1], snake.y[snake.len - 1]);
        printf(" ");
    }
    for (k = snake.len - 1; k > 0; k--)
    {
        snake.x[k] = snake.x[k - 1];
        snake.y[k] = snake.y[k - 1];
    }
    switch (ch)
    {
    case UP:
        if (state == DOWN)
        {
            snake.x[0]++;
            break;
        }
        else
        {
            snake.x[0]--;
            state = UP;
            break;
        }
    case DOWN:
        if (state == UP)
        {
            snake.x[0]--;
            break;
        }
        else
        {
            snake.x[0]++;
            state = DOWN;
            break;
        }
    case LEFT:
        if (state == RIGHT)
        {
            snake.y[0]++;
            break;
        }
        else
        {
            snake.y[0]--;
            state = LEFT;
            break;
        }
    case RIGHT:
        if (state == LEFT)
        {
            snake.y[0]--;
            break;
        }
        else
        {
            snake.y[0]++;
            state = RIGHT;
            break;
        }
    default:
        if (state == DOWN)
        {
            snake.x[0]++;
            break;
        }
        else if (state == UP)
        {
            snake.x[0]--;
            break;
        }
        else if (state == LEFT)
        {
            snake.y[0]--;
            break;
        }
        else if (state == RIGHT)
        {
            snake.y[0]++;
            break;
        }
    }
    Cursor_gotoxy(snake.x[0], snake.y[0]);
    printf("+");
    grow = 0;
    Cursor_gotoxy(length, 0);
}

int alive(void)
{
    if (snake.x[0] == 0 || snake.x[0] == length - 1 || snake.y[0] == 0 || snake.y[0] == width - 1)
        return 0;
    for (k = 1; k < snake.len; k++)
    {
        if (snake.x[0] == snake.x[k] && snake.y[0] == snake.y[k])
            return 0;
    }
    return 1;
}

void Cursor_gotoxy(int x, int y)
{
#ifdef _WIN32
    HANDLE hout;
    COORD cor;
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    cor.X = y;
    cor.Y = x;
    SetConsoleCursorPosition(hout, cor);
#else
    printf("\033[%d;%dH", x + 1, y + 1);
#endif
}

#ifndef _WIN32
void initTerminal(void)
{
    if (tcgetattr(STDIN_FILENO, &g_old_termios) == 0)
    {
        struct termios new_termios = g_old_termios;
        new_termios.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
        new_termios.c_cc[VMIN] = 0;
        new_termios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
        g_termios_ready = 1;
    }
}

void restoreTerminal(void)
{
    if (g_termios_ready)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
    }
    setColor(defacol);
    showCursor();
}

int _kbhit(void)
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int _getch(void)
{
    unsigned char c;
    while (read(STDIN_FILENO, &c, 1) <= 0)
    {
        // Wait until a real key press is available.
    }
    return c;
}

void Sleep(int ms)
{
    usleep((useconds_t)ms * 1000U);
}

void clearScreen(void)
{
    printf("\033[2J\033[H");
}
#else
void clearScreen(void)
{
    system("cls");
}
#endif