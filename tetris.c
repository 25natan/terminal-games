#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <wchar.h>
#include <locale.h>

// Move cursor to x,y position
void GoToXY(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

// Block character 
wchar_t block_character = L'█';

// Draw a character at x, y coordinates 
void DrawAtXY(int x, int y, char color) {
    GoToXY(x, y);
    switch (color)
    {
    case 'R': wprintf(L"\033[1;31m%lc", block_character); break;
    case 'G': wprintf(L"\033[1;32m%lc", block_character); break;
    case 'Y': wprintf(L"\033[1;33m%lc", block_character); break;
    case 'B': wprintf(L"\033[1;34m%lc", block_character); break;
    case 'O': wprintf(L"\033[38;5;208m%lc", block_character); break;
    case 'W': wprintf(L"\033[0m%lc", block_character); break;
    case 'E': wprintf(L" ");
    }
    fflush(stdout);
}

// Function to set terminal to non-blocking mode
void SetNonBlockingInput() {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~ICANON; // Disable canonical mode
    ttystate.c_lflag &= ~ECHO;   // Disable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // Non-blocking input
}

// Function to check if a key has been pressed
int KBHit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

// Height and width of the game board
#define HEIGHT 20
#define WIDTH 20

// Game board
char board[HEIGHT][WIDTH];

// Point type
typedef struct {
    int x, y;
} Point;

// Game piece structure
typedef struct {
    char color;
    int transform;
    int numOfTransforms;
    Point location;
    Point transforms[4][4];
} GamePiece;

// Directions enum
enum UserInput {
    NONE,
    ROTATE,
    DOWN,
    RIGHT,
    LEFT
};

// Initial pieces
GamePiece initialPieces[] = {
    // I piece
    { 'R', 0, 2, { WIDTH / 2, 0 }, { { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 } }, { { -1, 1 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } } },
    // J piece
    { 'R', 0, 4, { WIDTH / 2, 0 }, { { { 0, 0 }, { 0, 1 }, { 0, 2 }, { -1, 2 } }, { { -1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } }, { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 0, 2 } }, { { -1, 1 }, { 0, 1 }, { 1, 1 }, { 1, 2 } } } },
    // T piece
    { 'R', 0, 4, { WIDTH / 2, 0 }, { { { -1, 0 }, { 0, 0 }, { 1, 0 }, { 0, 1 } }, { { 0, 0 }, { 0, 1 }, { -1, 1 }, { 0, 2 } }, { { 0, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } }, { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 } } } },
    // Z piece
    { 'R', 0, 2, { WIDTH / 2, 0 }, { { { -1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } }, { { 0, 0 }, { 0, 1 }, { -1, 1 }, { -1, 2 } } } },
    // S piece
    { 'R', 0, 2, { WIDTH / 2, 0 }, { { { 0, 0 }, { 1, 0 }, { 0, 1 }, { -1, 1 } }, { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 } } } },
    // L piece
    { 'R', 0, 4, { WIDTH / 2, 0 }, { { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 1, 2 } }, { { -1, 1 }, { -1, 0 }, { 0, 0 }, { 1, 0 } }, { { -1, 0 }, { 0, 0 }, { 0, 1 }, { 0, 2 } }, { { -1, 1 }, { 0, 1 }, { 1, 1 }, { 1, 0 } } } },
    // O piece
    { 'R', 0, 1, { WIDTH / 2, 0 }, { { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } } } },
};

// Colors
char colors[] = {
    'R', 'G', 'Y', 'B', 'O'
};

// Initialize board
void InitBoard() {
    int i, j;
    for(i = 0;i < HEIGHT;i++)
        for(j = 0;j < WIDTH;j++)
            board[i][j] = 'E';
}

// Get random piece
GamePiece GetRandomPiece() {
    int random = rand() % (sizeof(initialPieces) / sizeof(GamePiece));
    return initialPieces[random];
}

// Set random color to piece
void SetRandomColor(GamePiece *gamePiece) {
    int random = rand() % (sizeof(colors) / sizeof(char));
    gamePiece->color = colors[random];
}


// Get location of one of the piece's points
Point GetPieceSquareLocation(GamePiece* gamePiece, int i) {
    return (Point){
        gamePiece->transforms[gamePiece->transform][i].x + gamePiece->location.x,
        gamePiece->transforms[gamePiece->transform][i].y + gamePiece->location.y
    };
}

// Check piece collision
int CheckCollision(GamePiece* gamePiece) {
    int i;
    for(i = 0;i < 4;i++){
        Point location = GetPieceSquareLocation(gamePiece, i);
        if(location.y < 0 || location.y >= HEIGHT ||
            location.x < 0 || location.x >= WIDTH ||
            board[location.y][location.x] != 'E')
            return 1;
    }
    return 0;
}

// Rotate piece
int RotatePiece(GamePiece* gamePiece, int toTheRight) {
    gamePiece->transform += toTheRight ? 1 : -1;
    gamePiece->transform %= gamePiece->numOfTransforms;
    if(CheckCollision(gamePiece)) {
        gamePiece->transform -= toTheRight ? 1 : -1;
        gamePiece->transform %= gamePiece->numOfTransforms;
        return 0;
    }
    return 1;
}

// Move piece
int MovePiece(GamePiece* gamePiece, int direction) {
    switch (direction)
    {
    case DOWN: gamePiece->location.y += 1; break;
    case RIGHT: gamePiece->location.x += 1; break;
    case LEFT: gamePiece->location.x -= 1;
    }
    if(!CheckCollision(gamePiece))
        return 1;
    switch (direction)
    {
    case DOWN: gamePiece->location.y -= 1; break;
    case RIGHT: gamePiece->location.x -= 1; break;
    case LEFT: gamePiece->location.x += 1;
    }
    return 0;
}

// Draw piece
void DrawPiece(GamePiece* gamePiece) {
    int i;
    for(i = 0;i < 4;i++){
        Point location = GetPieceSquareLocation(gamePiece, i);
        DrawAtXY(location.x + 2, location.y + 2, gamePiece->color);
    }
}

// Draw board
void DrawBoard() {
    int i, j;
    for(i = 0;i < HEIGHT;i++)
        for(j = 0;j < WIDTH;j++)
            DrawAtXY(j + 2, i + 2, board[i][j]);
}

// Draw board frame
void DrawBoardFrame() {
    int i;
    for(i = 1;i <= HEIGHT + 2;i++) {
        DrawAtXY(1, i, 'W');
        DrawAtXY(WIDTH + 2, i, 'W');
    }
    for(i = 1;i <= WIDTH + 2;i++) {
        DrawAtXY(i, 1, 'W');
        DrawAtXY(i, HEIGHT + 2, 'W');
    }
}

// Get user input
int GetUserInput() {
    if(KBHit()) {
        int c = getchar();
        if (c == '\x1b') { 
            if (getchar() == '[')
                switch (getchar())
                {
                case 'B': return DOWN;
                case 'C': return RIGHT;
                case 'D': return LEFT;
                }
        }
        if(c == '\n')
            return ROTATE;
    }
    return NONE;
}

// Check is line full
int IsFullLine(int i) {
    int j;
    for(j = 0;j < WIDTH;j++)
        if(board[i][j] == 'E')
            return 0;
    return 1;
}

// Cleans full line
void CleanFullLine(int i) {
    int j;
    for(;i > 0;i--) 
        for(j = 0;j < WIDTH;j++)
            board[i][j] = board[i - 1][j];
    for(j = 0;j < WIDTH;j++)
        board[0][j] = 'E';
}

// Clean full lines
int CleanFullLines() {
    int i, clean = 0;
    for(i = HEIGHT - 1;i >= 0;) {
        while(IsFullLine(i)) {
            CleanFullLine(i);
            clean = 1;
        }
        i--;
    }
    return clean;
}

// Clear piece
void ClearPiece(GamePiece* gamePiece) {
    int i;
    for(i = 0;i < 4;i++){
        Point location = GetPieceSquareLocation(gamePiece, i);
        DrawAtXY(location.x + 2, location.y + 2, 'E');
    }
}

// Clock structure definition
typedef struct {
    struct timespec ts;
    long start;
} Clock;

// Clock start
void ClockStart(Clock *clock) {
    clock_gettime(0, &(clock->ts));
    clock->start = clock->ts.tv_sec * 1000 + clock->ts.tv_nsec / 1000000;
}

// Get elapsed time in ms
long GetElapsedTime(Clock *clock) {
    clock_gettime(0, &(clock->ts));
    return clock->ts.tv_sec * 1000 + clock->ts.tv_nsec / 1000000 - clock->start;
}

// Piece movement loop
void PieceMovementLoop(GamePiece* gamePiece) {
    Clock clock;
    ClockStart(&clock);
    GamePiece lastPiece;
    DrawPiece(gamePiece);
    while(1) {
        lastPiece = *gamePiece;
        int userInput = GetUserInput();
        if(userInput != NONE && (userInput == ROTATE && RotatePiece(gamePiece, 1) || userInput != ROTATE && MovePiece(gamePiece, userInput))) {
            ClearPiece(&lastPiece);
            DrawPiece(gamePiece);
        }
        if(GetElapsedTime(&clock) > 500) {
            if(!MovePiece(gamePiece, DOWN))
                break;
            ClearPiece(&lastPiece);
            DrawPiece(gamePiece);
            ClockStart(&clock);
        }
    }
}

void AddPieceToBoard(GamePiece* gamePiece) {
    int i;
    for(i = 0;i < 4;i++){
        Point location = GetPieceSquareLocation(gamePiece, i);
        board[location.y][location.x] = gamePiece->color;
    }
}

// Main game loop
void MainGameLoop() {
    InitBoard();
    DrawBoardFrame();
    DrawBoard();
    while(1) {
        GamePiece gamePiece = GetRandomPiece();
        if(CheckCollision(&gamePiece))
            return;
        SetRandomColor(&gamePiece);
        PieceMovementLoop(&gamePiece);
        AddPieceToBoard(&gamePiece);
        if(CleanFullLines())
            DrawBoard();
    }
}

// Set up
void SetUp() {
    SetNonBlockingInput();
    srand(time(NULL));
    printf("\033[?25l");
    system("clear");
    setlocale(LC_CTYPE, "");
}

// Clean up
void CleanUp() {
    printf("\033[?25h");
    system("clear");
}

// Main
int main() {
    SetUp();
    MainGameLoop();
    CleanUp();
    return 0;
}
