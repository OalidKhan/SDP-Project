/*
 * ============================================================
 *          SUDOKU MASTER PRO - Complete Management System
 *          Written in C | GCC Compatible
 *          Features: Auth, Gameplay, Hints, Scores
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ============================================================
 *                     CONSTANTS & MACROS
 * ============================================================ */
#define MAX_USERS        50
#define MAX_SAVES        5
#define MAX_HIGHSCORES   10
#define USERNAME_LEN     32
#define PASSWORD_LEN     32
#define BOARD_SIZE       9
#define BOX_SIZE         3

#define BASE_SCORE       1000
#define WRONG_PENALTY    10
#define HINT_PENALTY     50
#define COMPLETE_BONUS   500
#define NO_HINT_BONUS    200

#define FILE_USERS       "users.dat"
#define FILE_SAVES       "saves.dat"
#define FILE_HIGHSCORES  "highscores.dat"

#define TUTORIAL_FILE    "tutorial.txt"
#define TUTORIAL_LINE_LEN 256

/* Difficulty IDs */
#define DIFF_EASY   1
#define DIFF_MEDIUM 2
#define DIFF_HARD   3

/* ============================================================
 *                     STRUCTURE DEFINITIONS
 * ============================================================ */

/* Registered user account */
typedef struct {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    int  active;   /* 1 = slot used */
} User;

/* Active game state */
typedef struct {
    char username[USERNAME_LEN];
    int  board[BOARD_SIZE][BOARD_SIZE];      /* current board  */
    int  solution[BOARD_SIZE][BOARD_SIZE];   /* full solution   */
    int  locked[BOARD_SIZE][BOARD_SIZE];     /* original cells  */
    int  difficulty;
    int  hintsLeft;
    int  wrongMoves;
    int  hintsUsed;
    int  score;
    int  active;   /* 1 = slot used  */
    int  slot;     /* save-slot index */
} Game;

/* High-score entry */
typedef struct {
    char username[USERNAME_LEN];
    int  difficulty;
    int  score;
    int  active;
} Score;

/* ============================================================
 *                   GLOBAL VARIABLES
 * ============================================================ */

static User       g_users[MAX_USERS];
static Game       g_saves[MAX_SAVES];
static Score      g_scores[MAX_HIGHSCORES];

static char       g_currentUser[USERNAME_LEN] = "";
static Game       g_currentGame;            /* live game session */
static int        g_loggedIn = 0;

/* ============================================================
 *              BUILT-IN PUZZLE BANKS (Easy/Medium/Hard)
 *   0 = empty cell to be solved by player
 * ============================================================ */

/* Five easy puzzles (many givens) */
static int easyPuzzles[5][BOARD_SIZE][BOARD_SIZE] = {
    {
        {5,3,0, 0,7,0, 0,0,0},
        {6,0,0, 1,9,5, 0,0,0},
        {0,9,8, 0,0,0, 0,6,0},
        {8,0,0, 0,6,0, 0,0,3},
        {4,0,0, 8,0,3, 0,0,1},
        {7,0,0, 0,2,0, 0,0,6},
        {0,6,0, 0,0,0, 2,8,0},
        {0,0,0, 4,1,9, 0,0,5},
        {0,0,0, 0,8,0, 0,7,9}
    },
    {
        {0,0,3, 0,2,0, 6,0,0},
        {9,0,0, 3,0,5, 0,0,1},
        {0,0,1, 8,0,6, 4,0,0},
        {0,0,8, 1,0,2, 9,0,0},
        {7,0,0, 0,0,0, 0,0,8},
        {0,0,6, 7,0,8, 2,0,0},
        {0,0,2, 6,0,9, 5,0,0},
        {8,0,0, 2,0,3, 0,0,9},
        {0,0,5, 0,1,0, 3,0,0}
    },
    {
        {2,0,0, 3,0,0, 0,0,0},
        {8,0,4, 0,6,2, 0,0,3},
        {0,1,3, 8,0,0, 2,0,0},
        {0,0,0, 0,2,0, 3,9,0},
        {5,0,7, 0,0,0, 6,2,1},
        {0,3,2, 0,0,6, 0,0,0},
        {0,2,0, 0,0,9, 1,4,0},
        {6,0,1, 2,5,0, 8,0,9},
        {0,0,0, 0,0,1, 0,0,2}
    },
    {
        {0,0,0, 2,6,0, 7,0,1},
        {6,8,0, 0,7,0, 0,9,0},
        {1,9,0, 0,0,4, 5,0,0},
        {8,2,0, 1,0,0, 0,4,0},
        {0,0,4, 6,0,2, 9,0,0},
        {0,5,0, 0,0,3, 0,2,8},
        {0,0,9, 3,0,0, 0,7,4},
        {0,4,0, 0,5,0, 0,3,6},
        {7,0,3, 0,1,8, 0,0,0}
    },
    {
        {1,0,0, 4,8,9, 0,0,6},
        {7,3,0, 0,0,0, 0,4,0},
        {0,0,0, 0,0,1, 2,9,5},
        {0,0,7, 1,2,0, 6,0,0},
        {5,0,0, 7,0,3, 0,0,8},
        {0,0,6, 0,9,5, 7,0,0},
        {9,1,4, 6,0,0, 0,0,0},
        {0,2,0, 0,0,0, 0,3,7},
        {8,0,0, 5,1,2, 0,0,4}
    }
};

/* Five medium puzzles */
static int mediumPuzzles[5][BOARD_SIZE][BOARD_SIZE] = {
    {
        {0,0,0, 2,0,0, 0,6,3},
        {3,0,0, 0,0,5, 4,0,1},
        {0,0,1, 0,0,3, 9,8,0},
        {0,0,0, 0,0,0, 0,9,0},
        {0,0,0, 5,3,8, 0,0,0},
        {0,3,0, 0,0,0, 0,0,0},
        {0,2,6, 3,0,0, 5,0,0},
        {5,0,3, 7,0,0, 0,0,8},
        {4,7,0, 0,0,1, 0,0,0}
    },
    {
        {0,2,0, 6,0,8, 0,0,0},
        {5,8,0, 0,0,9, 7,0,0},
        {0,0,0, 0,4,0, 0,0,0},
        {3,7,0, 0,0,0, 5,0,0},
        {6,0,0, 0,0,0, 0,0,4},
        {0,0,8, 0,0,0, 0,1,3},
        {0,0,0, 0,2,0, 0,0,0},
        {0,0,9, 8,0,0, 0,3,6},
        {0,0,0, 3,0,6, 0,9,0}
    },
    {
        {0,0,0, 6,0,0, 4,0,0},
        {7,0,0, 0,0,3, 6,0,0},
        {0,0,0, 0,9,1, 0,8,0},
        {0,0,0, 0,0,0, 0,0,0},
        {0,5,0, 1,8,0, 0,0,3},
        {0,0,0, 3,0,6, 0,4,5},
        {0,4,0, 2,0,0, 0,6,0},
        {9,0,3, 0,0,0, 0,0,0},
        {0,2,0, 0,0,0, 1,0,0}
    },
    {
        {2,0,0, 3,0,0, 0,0,0},
        {0,6,0, 0,0,0, 0,4,0},
        {0,0,0, 0,7,0, 5,0,3},
        {0,0,3, 0,0,0, 1,0,0},
        {0,2,0, 5,0,6, 0,3,0},
        {0,0,6, 0,0,0, 7,0,0},
        {3,0,5, 0,1,0, 0,0,0},
        {0,4,0, 0,0,0, 0,8,0},
        {0,0,0, 0,0,7, 0,0,6}
    },
    {
        {0,8,0, 0,0,0, 0,0,9},
        {0,0,0, 4,0,0, 0,7,0},
        {0,0,6, 0,0,3, 8,0,0},
        {0,0,3, 0,5,0, 0,1,0},
        {8,0,0, 0,0,0, 0,0,5},
        {0,9,0, 0,7,0, 3,0,0},
        {0,0,8, 7,0,0, 1,0,0},
        {0,6,0, 0,0,9, 0,0,0},
        {4,0,0, 0,0,0, 0,5,0}
    }
};

/* Five hard puzzles (few givens) */
static int hardPuzzles[5][BOARD_SIZE][BOARD_SIZE] = {
    {
        {0,0,0, 0,0,0, 0,0,0},
        {0,0,0, 0,0,3, 0,8,5},
        {0,0,1, 0,2,0, 0,0,0},
        {0,0,0, 5,0,7, 0,0,0},
        {0,0,4, 0,0,0, 1,0,0},
        {0,9,0, 0,0,0, 0,0,0},
        {5,0,0, 0,0,0, 0,7,3},
        {0,0,2, 0,1,0, 0,0,0},
        {0,0,0, 0,4,0, 0,0,9}
    },
    {
        {0,0,5, 3,0,0, 0,0,0},
        {8,0,0, 0,0,0, 0,2,0},
        {0,7,0, 0,1,0, 5,0,0},
        {4,0,0, 0,0,5, 3,0,0},
        {0,1,0, 0,7,0, 0,0,6},
        {0,0,3, 2,0,0, 0,8,0},
        {0,6,0, 5,0,0, 0,0,9},
        {0,0,4, 0,0,0, 0,3,0},
        {0,0,0, 0,0,9, 7,0,0}
    },
    {
        {1,0,0, 0,0,7, 0,9,0},
        {0,3,0, 0,2,0, 0,0,8},
        {0,0,9, 6,0,0, 5,0,0},
        {0,0,5, 3,0,0, 9,0,0},
        {0,1,0, 0,8,0, 0,0,2},
        {6,0,0, 0,0,4, 0,0,0},
        {3,0,0, 0,0,0, 0,1,0},
        {0,4,0, 0,0,0, 0,0,7},
        {0,0,7, 0,0,0, 3,0,0}
    },
    {
        {0,0,0, 0,0,0, 0,0,1},
        {0,0,0, 0,0,2, 0,0,0},
        {0,0,0, 0,3,0, 0,4,0},
        {0,0,0, 0,0,0, 5,0,0},
        {4,0,1, 6,0,0, 0,0,0},
        {0,0,7, 1,0,0, 0,0,0},
        {0,5,0, 0,0,0, 2,0,0},
        {0,0,0, 0,8,0, 0,0,0},
        {0,0,0, 0,0,0, 0,6,0}
    },
    {
        {0,2,0, 0,0,0, 0,0,0},
        {0,0,0, 6,0,0, 0,0,3},
        {0,7,4, 0,8,0, 0,0,0},
        {0,0,0, 0,0,3, 0,0,2},
        {0,8,0, 0,4,0, 0,1,0},
        {6,0,0, 5,0,0, 0,0,0},
        {0,0,0, 0,1,0, 7,8,0},
        {5,0,0, 0,0,9, 0,0,0},
        {0,0,0, 0,0,0, 0,4,0}
    }
};

/* ============================================================
 *                   FORWARD DECLARATIONS
 * ============================================================ */

/* File I/O */
void loadAllData(void);
void saveAllData(void);
void loadUsers(void);
void saveUsers(void);
void loadSaves(void);
void saveSaves(void);
void loadHighScores(void);
void saveHighScores(void);

/* Auth */
void signUp(void);
void signIn(void);
void logout(void);

/* Menus */
void printBanner(void);
void authMenu(void);
void mainMenu(void);

/* Gameplay */
void newGame(void);
void continueGame(void);
void saveGame(void);
void loadGame(int slot);
void playGame(void);
void printBoard(void);

/* Sudoku logic */
int  isValidMove(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int num);
int  solveSudoku(int board[BOARD_SIZE][BOARD_SIZE]);
void copyBoard(int dst[BOARD_SIZE][BOARD_SIZE], int src[BOARD_SIZE][BOARD_SIZE]);
int  isBoardComplete(int board[BOARD_SIZE][BOARD_SIZE]);
void selectPuzzle(int diff, int puzzle[BOARD_SIZE][BOARD_SIZE]);

/* Hint, score */
void giveHint(void);
int  calculateScore(void);

/* Leaderboard */
void showHighScores(void);
void updateLeaderboard(int score, int diff);

/* Under-construction placeholder for not-yet-implemented menu features */
void underConstruction(const char *featureName);

/* Tutorial */
void showTutorial(void);

/* Utility */
void clearScreen(void);
void pressEnter(void);
void printSeparator(int width);
void printCentered(const char *text, int width);
void printBoxBorder(int width);
void printBoxLine(const char *text, int width);
void printBoxLineCentered(const char *text, int width);
const char *diffName(int diff);
int  findUser(const char *username);

/* ============================================================
 *                      FILE I/O FUNCTIONS
 * ============================================================ */

void loadUsers(void) {
    FILE *f = fopen(FILE_USERS, "rb");
    if (!f) { memset(g_users, 0, sizeof(g_users)); return; }
    fread(g_users, sizeof(User), MAX_USERS, f);
    fclose(f);
}

void saveUsers(void) {
    FILE *f = fopen(FILE_USERS, "wb");
    if (!f) { printf("[ERROR] Cannot write %s\n", FILE_USERS); return; }
    fwrite(g_users, sizeof(User), MAX_USERS, f);
    fclose(f);
}

void loadSaves(void) {
    FILE *f = fopen(FILE_SAVES, "rb");
    if (!f) { memset(g_saves, 0, sizeof(g_saves)); return; }
    fread(g_saves, sizeof(Game), MAX_SAVES, f);
    fclose(f);
}

void saveSaves(void) {
    FILE *f = fopen(FILE_SAVES, "wb");
    if (!f) { printf("[ERROR] Cannot write %s\n", FILE_SAVES); return; }
    fwrite(g_saves, sizeof(Game), MAX_SAVES, f);
    fclose(f);
}

void loadHighScores(void) {
    FILE *f = fopen(FILE_HIGHSCORES, "rb");
    if (!f) { memset(g_scores, 0, sizeof(g_scores)); return; }
    fread(g_scores, sizeof(Score), MAX_HIGHSCORES, f);
    fclose(f);
}

void saveHighScores(void) {
    FILE *f = fopen(FILE_HIGHSCORES, "wb");
    if (!f) { printf("[ERROR] Cannot write %s\n", FILE_HIGHSCORES); return; }
    fwrite(g_scores, sizeof(Score), MAX_HIGHSCORES, f);
    fclose(f);
}

void loadAllData(void) {
    loadUsers();
    loadSaves();
    loadHighScores();
}

void saveAllData(void) {
    saveUsers();
    saveSaves();
    saveHighScores();
}

/* ============================================================
 *                    UTILITY FUNCTIONS
 * ============================================================ */

void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pressEnter(void) {
    printf("\n  Press [ENTER] to continue...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    getchar();
}

void printSeparator(int width) {
    printf("  ");
    for (int i = 0; i < width; i++) printf("=");
    printf("\n");
}

void printCentered(const char *text, int width) {
    int len   = (int)strlen(text);
    int left  = (width - len) / 2;
    printf("  ");
    for (int i = 0; i < left; i++) printf(" ");
    printf("%s\n", text);
}

void printBoxBorder(int width) {
    printf("  +");
    for (int i = 0; i < width + 2; i++) printf("-");
    printf("+\n");
}

void printBoxLine(const char *text, int width) {
    printf("  | %-*s |\n", width, text);
}

void printBoxLineCentered(const char *text, int width) {
    int inner = width + 2;
    int len   = (int)strlen(text);
    int left  = (inner - len) / 2;
    if (left < 0) left = 0;
    int right = inner - len - left;
    if (right < 0) right = 0;

    printf("  |");
    for (int i = 0; i < left;  i++) printf(" ");
    printf("%s", text);
    for (int i = 0; i < right; i++) printf(" ");
    printf("|\n");
}

const char *diffName(int diff) {
    if (diff == DIFF_EASY)   return "Easy";
    if (diff == DIFF_MEDIUM) return "Medium";
    if (diff == DIFF_HARD)   return "Hard";
    return "Unknown";
}

int findUser(const char *username) {
    for (int i = 0; i < MAX_USERS; i++)
        if (g_users[i].active && strcmp(g_users[i].username, username) == 0)
            return i;
    return -1;
}

/* ============================================================
 *                      BANNER & MENUS
 * ============================================================ */

void printBanner(void) {
    clearScreen();
    printf("\n");
    printf("  \xc9\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbb\n");
    printf("  \xba                                   \xba\n");
    printf("  \xba      SUDOKU  MASTER  PRO          \xba\n");
    printf("  \xba      ~~~~~~~~~~~~~~~~~~~~~~       \xba\n");
    printf("  \xba      Console Edition  v1.0        \xba\n");
    printf("  \xba                                   \xba\n");
    printf("  \xc8\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xbc\n\n");
}

/* Fallback ASCII banner (if box-drawing chars don't render) */
void printBannerASCII(void) {
    clearScreen();
    printf("\n");
    printSeparator(37);
    printCentered("SUDOKU  MASTER  PRO", 37);
    printCentered("Console Edition  v1.0", 37);
    printSeparator(37);
    printf("\n");
}

void authMenu(void) {
    int choice;
    do {
        printBannerASCII();
        printf("  +-----------------------------+\n");
        printf("  |        WELCOME MENU         |\n");
        printf("  +-----------------------------+\n");
        printf("  |  1. Sign In                 |\n");
        printf("  |  2. Sign Up                 |\n");
        printf("  |  3. Exit                    |\n");
        printf("  +-----------------------------+\n");
        printf("\n  Enter choice: ");
        if (scanf("%d", &choice) != 1) { choice = 0; while(getchar()!='\n'); }

        switch (choice) {
            case 1: signIn();  break;
            case 2: signUp();  break;
            case 3: printf("\n  Thank you for playing Sudoku Master Pro!\n\n"); saveAllData(); exit(0);
            default: printf("\n  [!] Invalid choice.\n"); pressEnter();
        }
    } while (!g_loggedIn);
}

void mainMenu(void) {
    int choice;
    do {
        printBannerASCII();
        printf("  Logged in as: [ %s ]\n\n", g_currentUser);
        printf("  +-----------------------------+\n");
        printf("  |          MAIN MENU          |\n");
        printf("  +-----------------------------+\n");
        printf("  |  1. New Game                |\n");
        printf("  |  2. Continue Game           |\n");
        printf("  |  3. High Scores             |\n");
        printf("  |  4. Tutorial                |\n");
        printf("  |  5. Profile Statistics      |\n");
        printf("  |  6. Logout                  |\n");
        printf("  |  7. Exit                    |\n");
        printf("  +-----------------------------+\n");
        printf("\n  Enter choice: ");
        if (scanf("%d", &choice) != 1) { choice = 0; while(getchar()!='\n'); }

        switch (choice) {
            case 1: newGame();       break;
            case 2: continueGame();  break;
            case 3: showHighScores(); break;
            case 4: showTutorial();  break;
            case 5: underConstruction("Profile Statistics"); break;
            case 6: logout(); return;
            case 7:
                saveAllData();
                printf("\n  Thank you for playing Sudoku Master Pro!\n\n");
                exit(0);
            default:
                printf("\n  [!] Invalid option.\n");
                pressEnter();
        }
    } while (g_loggedIn);
}

/* ============================================================
 *                   AUTHENTICATION FUNCTIONS
 * ============================================================ */

void signUp(void) {
    char uname[USERNAME_LEN], pass[PASSWORD_LEN], pass2[PASSWORD_LEN];
    printBannerASCII();
    printf("  +-----------------------------+\n");
    printf("  |           SIGN UP           |\n");
    printf("  +-----------------------------+\n\n");

    printf("  Username (max 31 chars, no spaces): ");
    if (scanf("%31s", uname) != 1) return;

    /* Check for spaces */
    for (int i = 0; uname[i]; i++) {
        if (isspace((unsigned char)uname[i])) {
            printf("\n  [!] Username cannot contain spaces.\n");
            pressEnter(); return;
        }
    }

    /* Check duplicate */
    if (findUser(uname) >= 0) {
        printf("\n  [!] Username '%s' already exists.\n", uname);
        pressEnter(); return;
    }

    printf("  Password (max 31 chars): ");
    if (scanf("%31s", pass) != 1) return;

    if (strlen(pass) < 4) {
        printf("\n  [!] Password must be at least 4 characters.\n");
        pressEnter(); return;
    }

    printf("  Confirm Password: ");
    if (scanf("%31s", pass2) != 1) return;

    if (strcmp(pass, pass2) != 0) {
        printf("\n  [!] Passwords do not match.\n");
        pressEnter(); return;
    }

    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_USERS; i++) {
        if (!g_users[i].active) { slot = i; break; }
    }
    if (slot < 0) {
        printf("\n  [!] User database is full.\n");
        pressEnter(); return;
    }

    strncpy(g_users[slot].username, uname, USERNAME_LEN - 1);
    strncpy(g_users[slot].password, pass,  PASSWORD_LEN - 1);
    g_users[slot].active = 1;
    saveUsers();

    printf("\n  [OK] Account created! You can now sign in.\n");
    pressEnter();
}

void signIn(void) {
    char uname[USERNAME_LEN], pass[PASSWORD_LEN];
    printBannerASCII();
    printf("  +-----------------------------+\n");
    printf("  |           SIGN IN           |\n");
    printf("  +-----------------------------+\n\n");

    printf("  Username: ");
    if (scanf("%31s", uname) != 1) return;
    printf("  Password: ");
    if (scanf("%31s", pass) != 1) return;

    int idx = findUser(uname);
    if (idx < 0 || strcmp(g_users[idx].password, pass) != 0) {
        printf("\n  [!] Invalid username or password.\n");
        pressEnter(); return;
    }

    strncpy(g_currentUser, uname, USERNAME_LEN - 1);
    g_loggedIn = 1;
    printf("\n  [OK] Welcome back, %s!\n", g_currentUser);
    pressEnter();
}

void logout(void) {
    saveAllData();
    g_loggedIn = 0;
    memset(g_currentUser, 0, sizeof(g_currentUser));
    memset(&g_currentGame, 0, sizeof(Game));
    printf("\n  You have been logged out.\n");
    pressEnter();
}

/* ============================================================
 *                     SUDOKU LOGIC
 * ============================================================ */

/* Deep copy a 9x9 board */
void copyBoard(int dst[BOARD_SIZE][BOARD_SIZE], int src[BOARD_SIZE][BOARD_SIZE]) {
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            dst[r][c] = src[r][c];
}

/* Returns 1 if placing 'num' at (row,col) is valid */
int isValidMove(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int num) {
    /* Check row */
    for (int c = 0; c < BOARD_SIZE; c++)
        if (board[row][c] == num) return 0;

    /* Check column */
    for (int r = 0; r < BOARD_SIZE; r++)
        if (board[r][col] == num) return 0;

    /* Check 3x3 box */
    int boxRow = (row / BOX_SIZE) * BOX_SIZE;
    int boxCol = (col / BOX_SIZE) * BOX_SIZE;
    for (int r = boxRow; r < boxRow + BOX_SIZE; r++)
        for (int c = boxCol; c < boxCol + BOX_SIZE; c++)
            if (board[r][c] == num) return 0;

    return 1;
}

/* Backtracking solver � returns 1 if solved, 0 if unsolvable */
int solveSudoku(int board[BOARD_SIZE][BOARD_SIZE]) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (board[row][col] == 0) {
                for (int num = 1; num <= 9; num++) {
                    if (isValidMove(board, row, col, num)) {
                        board[row][col] = num;
                        if (solveSudoku(board)) return 1;
                        board[row][col] = 0;  /* backtrack */
                    }
                }
                return 0; /* no valid number found */
            }
        }
    }
    return 1; /* all cells filled */
}

/* Returns 1 if no empty cells remain */
int isBoardComplete(int board[BOARD_SIZE][BOARD_SIZE]) {
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (board[r][c] == 0) return 0;
    return 1;
}

/* Pick a random puzzle from the bank for the given difficulty */
void selectPuzzle(int diff, int puzzle[BOARD_SIZE][BOARD_SIZE]) {
    srand((unsigned int)time(NULL));
    int idx = rand() % 5;
    if (diff == DIFF_EASY)
        copyBoard(puzzle, easyPuzzles[idx]);
    else if (diff == DIFF_MEDIUM)
        copyBoard(puzzle, mediumPuzzles[idx]);
    else
        copyBoard(puzzle, hardPuzzles[idx]);
}

/* ============================================================
 *                      PRINT BOARD
 * ============================================================ */

void printBoard(void) {
    const int boxWidth = 42;
    char line[64];

    printf("\n");
    printBoxBorder(boxWidth);
    snprintf(line, sizeof(line), "Player  : %s", g_currentUser);
    printBoxLine(line, boxWidth);
    snprintf(line, sizeof(line), "Diff.   : %-8s  Score: %d",
             diffName(g_currentGame.difficulty), g_currentGame.score);
    printBoxLine(line, boxWidth);
    snprintf(line, sizeof(line), "Hints   : %-3d     Wrongs: %d",
             g_currentGame.hintsLeft, g_currentGame.wrongMoves);
    printBoxLine(line, boxWidth);
    printBoxBorder(boxWidth);
    printf("\n");

    /* Column headers */
    printf("        1   2   3   4   5   6   7   8   9\n");
    printf("      +===========+===========+===========+\n");

    for (int r = 0; r < BOARD_SIZE; r++) {
        printf("   %d  |", r + 1);
        for (int c = 0; c < BOARD_SIZE; c++) {
            int val = g_currentGame.board[r][c];

            /* Mark original (locked) cells with no brackets; user cells with ( ) */
            if (val == 0)
                printf(" . ");
            else if (g_currentGame.locked[r][c])
                printf(" %d ", val);
            else
                printf("(%d)", val);

            /* Box divider */
            if ((c + 1) % BOX_SIZE == 0) printf("|");
            else printf(" ");
        }
        printf("\n");

        /* Horizontal box dividers */
        if ((r + 1) % BOX_SIZE == 0 && r < BOARD_SIZE - 1)
            printf("      +===========+===========+===========+\n");
        else if (r < BOARD_SIZE - 1)
            printf("      +---+---+---+---+---+---+---+---+---+\n");
    }
    printf("      +===========+===========+===========+\n");
    printf("\n  Legend: (n) = your move   n = original\n");
}

/* ============================================================
 *                    GAME FLOW FUNCTIONS
 * ============================================================ */

void newGame(void) {
    printBannerASCII();
    printf("  +-----------------------------+\n");
    printf("  |       SELECT DIFFICULTY     |\n");
    printf("  +-----------------------------+\n");
    printf("  |  1. Easy    (5 hints)       |\n");
    printf("  |  2. Medium  (3 hints)       |\n");
    printf("  |  3. Hard    (1 hint)        |\n");
    printf("  |  4. Back                    |\n");
    printf("  +-----------------------------+\n");
    printf("\n  Choice: ");

    int choice;
    if (scanf("%d", &choice) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        printf("\n  [!] Invalid choice.\n");
        pressEnter();
        return;
    }
    if (choice == 4) return;              /* Back to main menu, no message */
    if (choice < 1 || choice > 3) {
        printf("\n  [!] Invalid choice.\n");
        pressEnter();
        return;
    }

    /* Initialise game struct */
    memset(&g_currentGame, 0, sizeof(Game));
    strncpy(g_currentGame.username, g_currentUser, USERNAME_LEN - 1);
    g_currentGame.difficulty = choice;
    g_currentGame.hintsLeft  = (choice == DIFF_EASY) ? 5 :
                                (choice == DIFF_MEDIUM) ? 3 : 1;
    g_currentGame.score      = BASE_SCORE;
    g_currentGame.active     = 1;
    g_currentGame.slot       = -1; /* not saved yet */

    /* Load puzzle and compute solution */
    selectPuzzle(choice, g_currentGame.board);
    copyBoard(g_currentGame.solution, g_currentGame.board);
    solveSudoku(g_currentGame.solution);  /* fill solution array */

    /* Mark locked (original) cells */
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            g_currentGame.locked[r][c] = (g_currentGame.board[r][c] != 0) ? 1 : 0;

    printf("\n  [OK] New %s game started! Good luck!\n", diffName(choice));
    pressEnter();
    playGame();
}

void continueGame(void) {
    const int boxWidth = 38;
    char line[64];

    printBannerASCII();
    printBoxBorder(boxWidth);
    printBoxLineCentered("SAVED GAME SLOTS", boxWidth);
    printBoxBorder(boxWidth);

    int found = 0;
    for (int i = 0; i < MAX_SAVES; i++) {
        if (g_saves[i].active &&
            strcmp(g_saves[i].username, g_currentUser) == 0)
        {
            snprintf(line, sizeof(line), "Slot %d: %-8s Score:%-5d Hints:%-2d",
                     i + 1, diffName(g_saves[i].difficulty),
                     g_saves[i].score, g_saves[i].hintsLeft);
            found = 1;
        } else {
            snprintf(line, sizeof(line), "Slot %d: [Empty]", i + 1);
        }
        printBoxLine(line, boxWidth);
    }
    snprintf(line, sizeof(line), "%d. Back", MAX_SAVES + 1);
    printBoxLine(line, boxWidth);
    printBoxBorder(boxWidth);

    if (!found) {
        printf("\n  No saved games found for your account.\n");
        pressEnter(); return;
    }

    printf("\n  Select slot (1-%d) or %d to back: ", MAX_SAVES, MAX_SAVES + 1);
    int slot;
    if (scanf("%d", &slot) != 1) { while(getchar()!='\n'); return; }
    if (slot == MAX_SAVES + 1) return;
    if (slot < 1 || slot > MAX_SAVES) { printf("  [!] Invalid slot.\n"); pressEnter(); return; }

    slot--; /* 0-indexed */
    if (!g_saves[slot].active ||
        strcmp(g_saves[slot].username, g_currentUser) != 0)
    {
        printf("\n  [!] That slot belongs to another user or is empty.\n");
        pressEnter(); return;
    }

    loadGame(slot);
    pressEnter();
    playGame();
}

void saveGame(void) {
    const int boxWidth = 38;
    char line[64];

    printBannerASCII();
    printBoxBorder(boxWidth);
    printBoxLineCentered("SAVE GAME", boxWidth);
    printBoxBorder(boxWidth);
    for (int i = 0; i < MAX_SAVES; i++) {
        if (g_saves[i].active && strcmp(g_saves[i].username, g_currentUser) == 0)
            snprintf(line, sizeof(line), "Slot %d: %-8s (overwrite)",
                     i + 1, diffName(g_saves[i].difficulty));
        else
            snprintf(line, sizeof(line), "Slot %d: [Empty]", i + 1);
        printBoxLine(line, boxWidth);
    }
    printBoxBorder(boxWidth);
    printf("\n  Choose slot (1-%d): ", MAX_SAVES);

    int slot;
    if (scanf("%d", &slot) != 1) { while(getchar()!='\n'); return; }
    if (slot < 1 || slot > MAX_SAVES) { printf("  [!] Invalid slot.\n"); pressEnter(); return; }
    slot--;

    g_currentGame.slot = slot;
    g_saves[slot] = g_currentGame;
    saveSaves();
    printf("\n  [OK] Game saved in slot %d.\n", slot + 1);
    pressEnter();
}

void loadGame(int slot) {
    g_currentGame = g_saves[slot];
    printf("\n  [OK] Game loaded from slot %d.\n", slot + 1);
}

/* ============================================================
 *                   HINT & SCORE FUNCTIONS
 * ============================================================ */

void giveHint(void) {
    if (g_currentGame.hintsLeft <= 0) {
        printf("\n  [!] No hints remaining!\n");
        pressEnter(); return;
    }

    /* Find a random empty cell and reveal solution */
    int empties[81][2];
    int count = 0;
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (g_currentGame.board[r][c] == 0) {
                empties[count][0] = r;
                empties[count][1] = c;
                count++;
            }

    if (count == 0) {
        printf("\n  [!] Board is already complete.\n");
        pressEnter(); return;
    }

    srand((unsigned int)time(NULL));
    int pick = rand() % count;
    int r    = empties[pick][0];
    int c    = empties[pick][1];

    g_currentGame.board[r][c] = g_currentGame.solution[r][c];
    g_currentGame.hintsLeft--;
    g_currentGame.hintsUsed++;
    g_currentGame.score -= HINT_PENALTY;
    if (g_currentGame.score < 0) g_currentGame.score = 0;

    printf("\n  [HINT] Revealed cell (%d, %d) = %d\n",
           r + 1, c + 1, g_currentGame.solution[r][c]);
    pressEnter();
}

int calculateScore(void) {
    int score = g_currentGame.score;

    /* Bonus for no hints */
    if (g_currentGame.hintsUsed == 0) score += NO_HINT_BONUS;
    /* Completion bonus */
    score += COMPLETE_BONUS;

    return (score > 0) ? score : 1;
}

/* ============================================================
 *                     PLAY LOOP
 * ============================================================ */

void playGame(void) {
    int  choice, row, col, num;

    while (1) {
        printBoard();

        /* Check for completion */
        if (isBoardComplete(g_currentGame.board)) {
            int finalScore = calculateScore();

            printf("\n");
            printSeparator(42);
            printCentered("CONGRATULATIONS! PUZZLE SOLVED!", 42);
            printSeparator(42);
            printf("\n");
            printf("  Difficulty : %s\n",  diffName(g_currentGame.difficulty));
            printf("  Hints Used : %d\n",   g_currentGame.hintsUsed);
            printf("  Final Score: %d\n",   finalScore);
            printf("\n");

            updateLeaderboard(finalScore, g_currentGame.difficulty);
            saveAllData();
            pressEnter();
            return;
        }

        printf("\n");
        printf("  +----------------------------------+\n");
        printf("  |  IN-GAME MENU                    |\n");
        printf("  |  1. Enter a number               |\n");
        printf("  |  2. Use a hint                   |\n");
        printf("  |  3. Save game                    |\n");
        printf("  |  4. Quit to main menu            |\n");
        printf("  +----------------------------------+\n");
        printf("  Choice: ");

        if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); continue; }

        switch (choice) {

            /* ---- Enter a number ---- */
            case 1:
                printf("  Row    (1-9): ");
                if (scanf("%d", &row) != 1) { while(getchar()!='\n'); break; }
                printf("  Column (1-9): ");
                if (scanf("%d", &col) != 1) { while(getchar()!='\n'); break; }
                printf("  Number (1-9): ");
                if (scanf("%d", &num) != 1) { while(getchar()!='\n'); break; }

                row--; col--;  /* convert to 0-indexed */

                if (row < 0 || row >= BOARD_SIZE ||
                    col < 0 || col >= BOARD_SIZE) {
                    printf("\n  [!] Row/column out of range.\n");
                    pressEnter(); break;
                }
                if (num < 1 || num > 9) {
                    printf("\n  [!] Number must be 1-9.\n");
                    pressEnter(); break;
                }
                if (g_currentGame.locked[row][col]) {
                    printf("\n  [!] That cell is locked (original number).\n");
                    pressEnter(); break;
                }
                if (!isValidMove(g_currentGame.board, row, col, num)) {
                    g_currentGame.wrongMoves++;
                    g_currentGame.score -= WRONG_PENALTY;
                    if (g_currentGame.score < 0) g_currentGame.score = 0;
                    printf("\n  [!] Invalid move! -%d score penalty. (Wrong: %d)\n",
                           WRONG_PENALTY, g_currentGame.wrongMoves);
                    pressEnter(); break;
                }
                /* Valid move */
                g_currentGame.board[row][col] = num;
                printf("\n  [OK] Move accepted.\n");
                pressEnter();
                break;

            /* ---- Hint ---- */
            case 2:
                giveHint();
                break;

            /* ---- Save ---- */
            case 3:
                saveGame();
                break;

            /* ---- Quit ---- */
            case 4:
                saveAllData();
                printf("\n  Game progress not saved. Returning to main menu.\n");
                pressEnter();
                return;

            default:
                printf("\n  [!] Invalid option.\n");
                pressEnter();
        }
    }
}

/* ============================================================
 *                       LEADERBOARD
 * ============================================================ */

void updateLeaderboard(int score, int diff) {
    /* Find lowest-scoring active slot or empty slot */
    int replaceIdx = -1;
    int lowestScore = score + 1;

    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (!g_scores[i].active) {
            replaceIdx = i;
            break;
        }
        if (g_scores[i].score < lowestScore) {
            lowestScore  = g_scores[i].score;
            replaceIdx   = i;
        }
    }

    if (replaceIdx >= 0 &&
        (!g_scores[replaceIdx].active || score > g_scores[replaceIdx].score))
    {
        strncpy(g_scores[replaceIdx].username, g_currentUser, USERNAME_LEN - 1);
        g_scores[replaceIdx].difficulty = diff;
        g_scores[replaceIdx].score      = score;
        g_scores[replaceIdx].active     = 1;
        saveHighScores();
        printf("\n  *** NEW HIGH SCORE! You made the Top 10! ***\n");
    }
}

void showHighScores(void) {
    const int boxWidth = 36;
    char line[64];

    printBannerASCII();
    printBoxBorder(boxWidth);
    printBoxLineCentered("TOP 10 HIGH SCORES", boxWidth);
    printBoxBorder(boxWidth);
    snprintf(line, sizeof(line), "%-4s %-16.16s %-6.6s %-6s",
             "Rank", "Username", "Diff", "Score");
    printBoxLine(line, boxWidth);
    printBoxBorder(boxWidth);

    /* Selection sort (descending score) into temp array */
    Score temp[MAX_HIGHSCORES];
    memcpy(temp, g_scores, sizeof(g_scores));

    for (int i = 0; i < MAX_HIGHSCORES - 1; i++) {
        int maxIdx = i;
        for (int j = i + 1; j < MAX_HIGHSCORES; j++)
            if (temp[j].active && (!temp[maxIdx].active ||
                temp[j].score > temp[maxIdx].score))
                maxIdx = j;
        if (maxIdx != i) {
            Score tmp = temp[i];
            temp[i]   = temp[maxIdx];
            temp[maxIdx] = tmp;
        }
    }

    int printed = 0;
    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        if (!temp[i].active) continue;
        printed++;
        snprintf(line, sizeof(line), "%-4d %-16.16s %-6.6s %-6d",
                 printed, temp[i].username, diffName(temp[i].difficulty),
                 temp[i].score);
        printBoxLine(line, boxWidth);
    }
    if (printed == 0)
        printBoxLineCentered("No scores recorded yet.", boxWidth);

    printBoxBorder(boxWidth);
    pressEnter();
}

/* ============================================================
 *                  UNDER-CONSTRUCTION PLACEHOLDER
 * ============================================================ */

void underConstruction(const char *featureName) {
    const int boxWidth = 36;

    printBannerASCII();
    printBoxBorder(boxWidth);
    printBoxLineCentered("UNDER CONSTRUCTION", boxWidth);
    printBoxBorder(boxWidth);
    printf("\n  The \"%s\" feature is not ready yet.\n", featureName);
    printf("  It is currently under construction.\n");
    printf("  Please check back in a future update.\n");
    pressEnter();
}

/* ============================================================
 *                       TUTORIAL
 * ============================================================ */

void showTutorial(void) {
    FILE *fp = fopen(TUTORIAL_FILE, "r");

    clearScreen();
    printSeparator(50);
    printCentered("SUDOKU MASTER PRO -- TUTORIAL", 50);
    printSeparator(50);
    printf("\n");

    if (fp == NULL) {
        printf("  Tutorial file \"%s\" was not found.\n", TUTORIAL_FILE);
        printf("  Please make sure it is in the same folder as\n");
        printf("  the program and try again.\n\n");
        pressEnter();
        return;
    }

    char line[TUTORIAL_LINE_LEN];
    while (fgets(line, sizeof(line), fp) != NULL)
        printf("%s", line);

    fclose(fp);
    printf("\n");
    pressEnter();
}

/* ============================================================
 *                          MAIN
 * ============================================================ */

int main(void) {
    /* Load all persistent data from files */
    loadAllData();

    /* Authentication loop */
    authMenu();

    /* Main game loop */
    while (g_loggedIn) {
        mainMenu();

        /* After logout, offer to re-login */
        if (!g_loggedIn) {
            int answered = 0;
            while (!answered) {
                printf("\n  Return to login screen? (y/n): ");
                char c;
                if (scanf(" %c", &c) != 1) c = 'n'; /* EOF/read error -> treat as no */

                /* Discard any leftover characters on the input line */
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) { }

                if (c == 'y' || c == 'Y') {
                    authMenu();
                    answered = 1;
                } else if (c == 'n' || c == 'N') {
                    printf("\n  Thank you for playing Sudoku Master Pro!\n\n");
                    answered = 1;
                } else {
                    printf("\n  [!] Invalid input. Please enter 'y' or 'n'.\n");
                }
            }
        }
    }

    saveAllData();
    return 0;
}
