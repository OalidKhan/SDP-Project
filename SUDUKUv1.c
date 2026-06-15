/*
 * ============================================================
 *          SUDOKU MASTER PRO - Complete Management System
 *          Written in C | GCC Compatible
 *          Features: Auth, Gameplay, Hints, Scores, Stats
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
#define TIME_PENALTY_DIV 10   /* score -= elapsed_seconds / TIME_PENALTY_DIV */
#define COMPLETE_BONUS   500
#define NO_HINT_BONUS    200

#define FILE_USERS       "users.dat"
#define FILE_SAVES       "saves.dat"
#define FILE_HIGHSCORES  "highscores.dat"
#define FILE_STATISTICS  "statistics.dat"

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
    time_t startTime;
    int  active;   /* 1 = slot used  */
    int  slot;     /* save-slot index */
} Game;

/* High-score entry */
typedef struct {
    char username[USERNAME_LEN];
    int  difficulty;
    int  score;
    long timeTaken;  /* seconds */
    int  active;
} Score;

/* Per-user statistics */
typedef struct {
    char username[USERNAME_LEN];
    int  gamesPlayed;
    int  gamesWon;
    int  easyCompleted;
    int  mediumCompleted;
    int  hardCompleted;
    int  bestScore;
    long bestTime;    /* seconds, 0 = unset */
    int  totalHints;
    int  active;
} Statistics;

/* ============================================================
 *                   GLOBAL VARIABLES
 * ============================================================ */
static User       g_users[MAX_USERS];
static Game       g_saves[MAX_SAVES];
static Score      g_scores[MAX_HIGHSCORES];
static Statistics g_stats[MAX_USERS];

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
void loadStatistics(void);
void saveStatistics(void);

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

/* Leaderboard / stats */
void showHighScores(void);
void updateLeaderboard(int score, long timeTaken, int diff);
void showStatistics(void);
void updateStatistics(int won, int diff, int score, long timeTaken, int hints);

/* Tutorial */
void showTutorial(void);

/* Utility */
void clearScreen(void);
void pressEnter(void);
void printSeparator(int width);
void printCentered(const char *text, int width);
long elapsedSeconds(void);
const char *diffName(int diff);
int  findUser(const char *username);
int  findStatUser(const char *username);

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

void loadStatistics(void) {
    FILE *f = fopen(FILE_STATISTICS, "rb");
    if (!f) { memset(g_stats, 0, sizeof(g_stats)); return; }
    fread(g_stats, sizeof(Statistics), MAX_USERS, f);
    fclose(f);
}

void saveStatistics(void) {
    FILE *f = fopen(FILE_STATISTICS, "wb");
    if (!f) { printf("[ERROR] Cannot write %s\n", FILE_STATISTICS); return; }
    fwrite(g_stats, sizeof(Statistics), MAX_USERS, f);
    fclose(f);
}

void loadAllData(void) {
    loadUsers();
    loadSaves();
    loadHighScores();
    loadStatistics();
}

void saveAllData(void) {
    saveUsers();
    saveSaves();
    saveHighScores();
    saveStatistics();
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
    /* flush stdin before waiting */
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

const char *diffName(int diff) {
    if (diff == DIFF_EASY)   return "Easy";
    if (diff == DIFF_MEDIUM) return "Medium";
    if (diff == DIFF_HARD)   return "Hard";
    return "Unknown";
}

long elapsedSeconds(void) {
    return (long)(time(NULL) - g_currentGame.startTime);
}

int findUser(const char *username) {
    for (int i = 0; i < MAX_USERS; i++)
        if (g_users[i].active && strcmp(g_users[i].username, username) == 0)
            return i;
    return -1;
}

int findStatUser(const char *username) {
    for (int i = 0; i < MAX_USERS; i++)
        if (g_stats[i].active && strcmp(g_stats[i].username, username) == 0)
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
            case 3: printf("\n  Goodbye!\n\n"); saveAllData(); exit(0);
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
            case 5: showStatistics(); break;
            case 6: logout(); return;
            case 7:
                saveAllData();
                printf("\n  Goodbye!\n\n");
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

    /* Create statistics record */
    for (int i = 0; i < MAX_USERS; i++) {
        if (!g_stats[i].active) {
            memset(&g_stats[i], 0, sizeof(Statistics));
            strncpy(g_stats[i].username, uname, USERNAME_LEN - 1);
            g_stats[i].active = 1;
            saveStatistics();
            break;
        }
    }

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

/* Backtracking solver — returns 1 if solved, 0 if unsolvable */
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
    long elapsed = elapsedSeconds();
    int  hrs     = (int)(elapsed / 3600);
    int  mins    = (int)((elapsed % 3600) / 60);
    int  secs    = (int)(elapsed % 60);

    printf("\n");
    printf("  +------------------------------------------+\n");
    printf("  |  Player  : %-20s        |\n", g_currentUser);
    printf("  |  Diff.   : %-10s  Score: %-8d   |\n",
           diffName(g_currentGame.difficulty), g_currentGame.score);
    printf("  |  Time    : %02d:%02d:%02d    Hints: %-3d        |\n",
           hrs, mins, secs, g_currentGame.hintsLeft);
    printf("  |  Wrongs  : %-3d                           |\n",
           g_currentGame.wrongMoves);
    printf("  +------------------------------------------+\n\n");

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
    if (scanf("%d", &choice) != 1) { while(getchar()!='\n'); return; }
    if (choice < 1 || choice > 3) return;

    /* Initialise game struct */
    memset(&g_currentGame, 0, sizeof(Game));
    strncpy(g_currentGame.username, g_currentUser, USERNAME_LEN - 1);
    g_currentGame.difficulty = choice;
    g_currentGame.hintsLeft  = (choice == DIFF_EASY) ? 5 :
                                (choice == DIFF_MEDIUM) ? 3 : 1;
    g_currentGame.score      = BASE_SCORE;
    g_currentGame.startTime  = time(NULL);
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
    printBannerASCII();
    printf("  +-----------------------------+\n");
    printf("  |       SAVED GAME SLOTS      |\n");
    printf("  +-----------------------------+\n");

    int found = 0;
    for (int i = 0; i < MAX_SAVES; i++) {
        if (g_saves[i].active &&
            strcmp(g_saves[i].username, g_currentUser) == 0)
        {
            printf("  |  Slot %d: %-8s  Score:%-5d  Hints:%-2d |\n",
                   i + 1, diffName(g_saves[i].difficulty),
                   g_saves[i].score, g_saves[i].hintsLeft);
            found = 1;
        } else {
            printf("  |  Slot %d: [Empty]                      |\n", i + 1);
        }
    }
    printf("  |  %d. Back                          |\n", MAX_SAVES + 1);
    printf("  +-----------------------------+\n");

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
    printBannerASCII();
    printf("  +-----------------------------+\n");
    printf("  |         SAVE GAME           |\n");
    printf("  +-----------------------------+\n");
    for (int i = 0; i < MAX_SAVES; i++) {
        if (g_saves[i].active && strcmp(g_saves[i].username, g_currentUser) == 0)
            printf("  |  Slot %d: %-8s (overwrite)        |\n",
                   i + 1, diffName(g_saves[i].difficulty));
        else
            printf("  |  Slot %d: [Empty]                      |\n", i + 1);
    }
    printf("  +-----------------------------+\n");
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
    long elapsed = elapsedSeconds();
    int  penalty = (int)(elapsed / TIME_PENALTY_DIV);
    int  score   = g_currentGame.score - penalty;

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
            long  timeTaken = elapsedSeconds();
            int   finalScore = calculateScore();

            printf("\n");
            printSeparator(42);
            printCentered("CONGRATULATIONS! PUZZLE SOLVED!", 42);
            printSeparator(42);
            printf("\n");
            printf("  Difficulty : %s\n",  diffName(g_currentGame.difficulty));
            printf("  Time Taken : %ld seconds\n", timeTaken);
            printf("  Hints Used : %d\n",   g_currentGame.hintsUsed);
            printf("  Final Score: %d\n",   finalScore);
            printf("\n");

            updateLeaderboard(finalScore, timeTaken, g_currentGame.difficulty);
            updateStatistics(1, g_currentGame.difficulty, finalScore, timeTaken,
                             g_currentGame.hintsUsed);
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
                updateStatistics(0, g_currentGame.difficulty, 0, 0, 0);
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
 *                  LEADERBOARD & STATISTICS
 * ============================================================ */

void updateLeaderboard(int score, long timeTaken, int diff) {
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
        g_scores[replaceIdx].timeTaken  = timeTaken;
        g_scores[replaceIdx].active     = 1;
        saveHighScores();
        printf("\n  *** NEW HIGH SCORE! You made the Top 10! ***\n");
    }
}

void showHighScores(void) {
    printBannerASCII();
    printf("  +-------------------------------------------+\n");
    printf("  |              TOP 10 HIGH SCORES           |\n");
    printf("  +-------------------------------------------+\n");
    printf("  | Rank  Username         Diff    Score  Time |\n");
    printf("  +-------------------------------------------------+\n");

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
        int  mins = (int)(temp[i].timeTaken / 60);
        int  secs = (int)(temp[i].timeTaken % 60);
        printf("  | %-4d  %-16s %-7s %-6d %02d:%02d |\n",
               printed,
               temp[i].username,
               diffName(temp[i].difficulty),
               temp[i].score,
               mins, secs);
    }
    if (printed == 0)
        printf("  |          No scores recorded yet.          |\n");

    printf("  +-------------------------------------------------+\n");
    pressEnter();
}

void updateStatistics(int won, int diff, int score, long timeTaken, int hints) {
    int idx = findStatUser(g_currentUser);
    if (idx < 0) {
        /* Create new stats record */
        for (int i = 0; i < MAX_USERS; i++) {
            if (!g_stats[i].active) {
                memset(&g_stats[i], 0, sizeof(Statistics));
                strncpy(g_stats[i].username, g_currentUser, USERNAME_LEN - 1);
                g_stats[i].active = 1;
                idx = i;
                break;
            }
        }
        if (idx < 0) return;
    }

    g_stats[idx].gamesPlayed++;
    g_stats[idx].totalHints += hints;

    if (won) {
        g_stats[idx].gamesWon++;
        if (diff == DIFF_EASY)   g_stats[idx].easyCompleted++;
        if (diff == DIFF_MEDIUM) g_stats[idx].mediumCompleted++;
        if (diff == DIFF_HARD)   g_stats[idx].hardCompleted++;

        if (score > g_stats[idx].bestScore)
            g_stats[idx].bestScore = score;

        if (g_stats[idx].bestTime == 0 || timeTaken < g_stats[idx].bestTime)
            g_stats[idx].bestTime = timeTaken;
    }

    saveStatistics();
}

void showStatistics(void) {
    printBannerASCII();
    int idx = findStatUser(g_currentUser);
    printf("  +--------------------------------------+\n");
    printf("  |       PROFILE STATISTICS             |\n");
    printf("  |  Player: %-26s|\n", g_currentUser);
    printf("  +--------------------------------------+\n");

    if (idx < 0 || !g_stats[idx].active) {
        printf("  |  No statistics available yet.        |\n");
        printf("  +--------------------------------------+\n");
        pressEnter(); return;
    }

    Statistics *s = &g_stats[idx];
    int winRate = (s->gamesPlayed > 0)
                  ? (s->gamesWon * 100 / s->gamesPlayed) : 0;

    long bt_mins = s->bestTime / 60;
    long bt_secs = s->bestTime % 60;

    printf("  |  Games Played     : %-16d|\n", s->gamesPlayed);
    printf("  |  Games Won        : %-16d|\n", s->gamesWon);
    printf("  |  Win Rate         : %-15d%%|\n", winRate);
    printf("  +--------------------------------------+\n");
    printf("  |  Easy Completed   : %-16d|\n", s->easyCompleted);
    printf("  |  Medium Completed : %-16d|\n", s->mediumCompleted);
    printf("  |  Hard Completed   : %-16d|\n", s->hardCompleted);
    printf("  +--------------------------------------+\n");
    printf("  |  Best Score       : %-16d|\n", s->bestScore);
    if (s->bestTime > 0)
        printf("  |  Best Time        : %02ld:%02ld           |\n", bt_mins, bt_secs);
    else
        printf("  |  Best Time        : N/A              |\n");
    printf("  |  Total Hints Used : %-16d|\n", s->totalHints);
    printf("  +--------------------------------------+\n");
    pressEnter();
}

/* ============================================================
 *                       TUTORIAL
 * ============================================================ */

void showTutorial(void) {
    clearScreen();
    printSeparator(50);
    printCentered("SUDOKU MASTER PRO -- TUTORIAL", 50);
    printSeparator(50);

    printf("\n");
    printf("  WHAT IS SUDOKU?\n");
    printf("  ---------------\n");
    printf("  Sudoku is a logic-based number puzzle played on a\n");
    printf("  9x9 grid divided into nine 3x3 boxes.\n\n");

    printf("  THE RULES\n");
    printf("  ---------\n");
    printf("  1. Fill every empty cell with a number from 1 to 9.\n");
    printf("  2. Each ROW must contain numbers 1-9 without repeats.\n");
    printf("  3. Each COLUMN must contain numbers 1-9 without repeats.\n");
    printf("  4. Each 3x3 BOX must contain numbers 1-9 without repeats.\n\n");

    printf("  CONTROLS\n");
    printf("  --------\n");
    printf("  During a game, use the in-game menu:\n");
    printf("  1 -> Enter a number (enter row, column, then digit)\n");
    printf("  2 -> Use a hint (reveals one correct cell)\n");
    printf("  3 -> Save the current game to a slot\n");
    printf("  4 -> Quit to main menu\n\n");

    printf("  DIFFICULTY & HINTS\n");
    printf("  ------------------\n");
    printf("  Easy   -> More given numbers | 5 hints available\n");
    printf("  Medium -> Moderate clues     | 3 hints available\n");
    printf("  Hard   -> Fewer given numbers| 1 hint available\n\n");

    printf("  SCORING SYSTEM\n");
    printf("  --------------\n");
    printf("  Base score      : %d points\n",  BASE_SCORE);
    printf("  Wrong move      : -%d points\n", WRONG_PENALTY);
    printf("  Hint used       : -%d points\n", HINT_PENALTY);
    printf("  Time penalty    : -1 per %d seconds elapsed\n", TIME_PENALTY_DIV);
    printf("  Completion bonus: +%d points\n", COMPLETE_BONUS);
    printf("  No-hint bonus   : +%d points\n", NO_HINT_BONUS);
    printf("\n  Tip: Solve quickly and without hints for the best score!\n\n");

    printf("  BOARD LEGEND\n");
    printf("  ------------\n");
    printf("   n   = original (locked) cell -- cannot be changed\n");
    printf("  (n)  = number YOU entered\n");
    printf("   .   = empty cell to fill\n\n");

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
            printf("\n  Return to login screen? (y/n): ");
            char c;
            scanf(" %c", &c);
            if (c == 'y' || c == 'Y') {
                authMenu();
            } else {
                printf("\n  Thank you for playing Sudoku Master Pro!\n\n");
                break;
            }
        }
    }

    saveAllData();
    return 0;
}
