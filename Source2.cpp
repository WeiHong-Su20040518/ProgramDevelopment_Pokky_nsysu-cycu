#define _CRT_SECURE_NO_WARNINGS 
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <conio.h>
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

struct Food {
    int x; // 橫坐標 
    int y; // 縱坐標 
    char cc; // 儲存食物種類 
} food;

struct Snake {
    int x[1000];
    int y[1000];
    int len; // 長度 
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

int main(void) {
    system("chcp 65001 > nul"); // 切換終端機為 UTF-8 以顯示中文
    setColor();
    int nnn = 1;
    do {
        nnn = 1;
        ch = UP;
        state = UP;
        grow = 0;
        score = 0; // 初始化分數為0 
        n = 0;
        setColor(wordcol);
        printf("\t選擇遊戲模式:\n\t1.簡單\t2.中等\t3.困難\t\n");
        n = _getch(); // 現代編譯器需加底線
        setColor(defacol);

        switch (n) {
        case '1': sp = 300; break;
        case '2': sp = 230; break;
        case '3': sp = 140; break;
        default:
            printf("    Error!! Re-enter options\n");
            choo = '3';
            printf("    請按任意鍵後，再按Enter，重新選擇[離開]或[重新開始]...");
            scanf_s("%d", &nnn);
            nnn = 1;
            if (nnn) break;
        }

        system("cls");
        create_map();

        // 開始遊戲 
        for (;;) {
            snake.speed = sp;
            hideCursor();
            create_food();
            move_snake();
            Sleep(snake.speed);

            if (!(alive())) {
                break; // 如果死了，跳出迴圈 
            }
            if (score >= 500) { // 分數大於500，就獲勝
                nnn = 2;
                break;
            }
        }

        showCursor();
        Cursor_gotoxy(length / 2, width / 2 - 5);
        setColor(wordcol);

        if (nnn == 0) {
            system("cls");
            Cursor_gotoxy(8, 5);
            choo = 0;
            printf("Game Over!\n");
            printf("\t1.重新開始\t2.離開\n");
            choo = _getch();
        }
        else if (nnn == 1) {
            system("cls");
            Cursor_gotoxy(8, 5);
            choo = 0;
            printf("請選擇!\n");
            printf("\t1.重新開始\t2.離開\n");
            choo = _getch();
        }
        else if (nnn == 2) {
            system("cls");
            Cursor_gotoxy(8, 5);
            choo = 0;
            printf("Congratulations ! ! !\n");
            printf("\tYOU WIN THE GAME ! ! !\n");
            printf("\t1.重新開始\t2.離開\n");
            choo = _getch();
        }
    } while (choo == '1');

    return 0;
}

void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void hideCursor(void) {
    CONSOLE_CURSOR_INFO cursor_info = { 1, 0 };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}

void showCursor(void) {
    CONSOLE_CURSOR_INFO cursor_info = { 1, 1 };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}

void create_map(void) {
    int f, min = 1, max = 3;
    srand(time(NULL));

    food.x = rand() % (length - 2) + 1;
    food.y = rand() % (width - 2) + 1;
    Cursor_gotoxy(food.x, food.y);
    f = rand() % (max - min + 1) + min;
    setColor(foodcol);

    if (f == 1) { printf("$"); food.cc = '$'; }
    else if (f == 2) { printf("@"); food.cc = '@'; }
    else if (f == 3) { printf("?"); food.cc = '?'; }

    setColor(snakecol);
    snake.x[0] = length / 2;
    snake.y[0] = width / 2;
    Cursor_gotoxy(snake.x[0], snake.y[0]);
    printf("+");
    snake.len = 3;
    snake.speed = 200;

    for (k = 1; k < snake.len; k++) {
        snake.x[k] = snake.x[k - 1] + 1;
        snake.y[k] = snake.y[k - 1];
        Cursor_gotoxy(snake.x[k], snake.y[k]);
        printf("+");
    }

    setColor(wallcol);
    for (j = 0; j < width; j++) {
        Cursor_gotoxy(0, j); printf("#");
        Cursor_gotoxy(length - 1, j); printf("#");
    }
    for (i = 0; i < length - 1; i++) {
        Cursor_gotoxy(i, 0); printf("#");
        Cursor_gotoxy(i, width - 1); printf("#");
    }

    Cursor_gotoxy(2, width + 3);
    setColor(wordcol);
    if (n == '1') printf("難度: 簡單");
    else if (n == '2') printf("難度: 中等");
    else if (n == '3') printf("難度: 困難");

    setColor(colcol);
    Cursor_gotoxy(4, width + 3); printf("上: W");
    Cursor_gotoxy(6, width + 3); printf("下: S");
    Cursor_gotoxy(8, width + 3); printf("左: A");
    Cursor_gotoxy(10, width + 3); printf("右: D");

    setColor(sp_col);
    Cursor_gotoxy(12, width + 3); printf("獲得分數:%d", score);

    setColor(colcol);
    Cursor_gotoxy(14, width + 3); printf("規則:分數達到500分就獲勝了 ! ! !");
    Cursor_gotoxy(16, width + 3); printf("食物:$,得10分，蛇身增長 1 個單位");
    Cursor_gotoxy(18, width + 3); printf("食物:@,得20分，蛇身增長 2 個單位");
    Cursor_gotoxy(20, width + 3); printf("食物:?,得30分，蛇身增長 3 個單位");
    setColor(defacol);
}

void create_food(void) {
    int f, min = 1, max = 3;
    f = rand() % (max - min + 1) + min;
    setColor(defacol);

    if (snake.x[0] == food.x && snake.y[0] == food.y) {
        if (food.cc == '$') { score += 10; snake.len++; }
        else if (food.cc == '@') { score += 20; snake.len += 2; }
        else if (food.cc == '?') { score += 30; snake.len += 3; }

        Cursor_gotoxy(12, width + 3);
        setColor(sp_col);
        printf("獲得分數:%d", score);
        setColor(defacol);

        int flag = 1;
        do {
            food.x = rand() % (length - 2) + 1;
            food.y = rand() % (width - 2) + 1;
            flag = 1;
            for (i = 0; i < snake.len; i++) {
                if (food.x == snake.x[i] && food.y == snake.y[i]) {
                    flag = 0;
                    break;
                }
            }
        } while (flag == 0);

        Cursor_gotoxy(food.x, food.y);
        setColor(foodcol);
        if (f == 1) { printf("$"); food.cc = '$'; }
        else if (f == 2) { printf("@"); food.cc = '@'; }
        else if (f == 3) { printf("?"); food.cc = '?'; }
        setColor(defacol);
        grow = 1;
    }
}

void move_snake(void) {
    while (_kbhit()) { // 現代編譯器需加底線
        ch = _getch();
    }
    if (!grow) {
        Cursor_gotoxy(snake.x[snake.len - 1], snake.y[snake.len - 1]);
        printf(" ");
    }
    for (k = snake.len - 1; k > 0; k--) {
        snake.x[k] = snake.x[k - 1];
        snake.y[k] = snake.y[k - 1];
    }
    switch (ch) {
    case UP:
        if (state == DOWN) { snake.x[0]++; break; }
        else { snake.x[0]--; state = UP; break; }
    case DOWN:
        if (state == UP) { snake.x[0]--; break; }
        else { snake.x[0]++; state = DOWN; break; }
    case LEFT:
        if (state == RIGHT) { snake.y[0]++; break; }
        else { snake.y[0]--; state = LEFT; break; }
    case RIGHT:
        if (state == LEFT) { snake.y[0]--; break; }
        else { snake.y[0]++; state = RIGHT; break; }
    default:
        if (state == DOWN) { snake.x[0]++; break; }
        else if (state == UP) { snake.x[0]--; break; }
        else if (state == LEFT) { snake.y[0]--; break; }
        else if (state == RIGHT) { snake.y[0]++; break; }
    }
    Cursor_gotoxy(snake.x[0], snake.y[0]);
    printf("+");
    grow = 0;
    Cursor_gotoxy(length, 0);
}

int alive(void) {
    if (snake.x[0] == 0 || snake.x[0] == length - 1 || snake.y[0] == 0 || snake.y[0] == width - 1)
        return 0;
    for (k = 1; k < snake.len; k++) {
        if (snake.x[0] == snake.x[k] && snake.y[0] == snake.y[k])
            return 0;
    }
    return 1;
}

void Cursor_gotoxy(int x, int y) {
    HANDLE hout;
    COORD cor;
    hout = GetStdHandle(STD_OUTPUT_HANDLE);
    cor.X = y;
    cor.Y = x;
    SetConsoleCursorPosition(hout, cor);
}