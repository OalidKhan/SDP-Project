/*
 * ============================================================
 *          SUDOKU MASTER PRO - Complete Management System
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
#define USERNAME_LEN     32
#define PASSWORD_LEN     32
#define BOARD_SIZE       9
#define BOX_SIZE         3

#define FILE_USERS       "users.dat"
#define FILE_SAVES       "saves.dat"

/* Difficulty IDs */
#define DIFF_EASY   1
#define DIFF_MEDIUM 2
#define DIFF_HARD   3

/* ============================================================
 *                     STRUCTURE DEFINITIONS
 * ============================================================ */

/* Registered user account */
typedef struct
{
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    int  active;   /* 1 = slot used */
} User;

/* Active game state */
typedef struct
{
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

/* ============================================================
 *                   GLOBAL VARIABLES
 * ============================================================ */
static User       g_users[MAX_USERS];

static char       g_currentUser[USERNAME_LEN] = "";
static Game       g_currentGame;            /* live game session */
static int        g_loggedIn = 0;

/* ============================================================
 *              BUILT-IN PUZZLE BANKS (Easy/Medium/Hard)
 *               0 = empty cell to be solved by player
 * ============================================================ */

/* Five easy puzzles (many givens) */
static int easyPuzzles[5][BOARD_SIZE][BOARD_SIZE] =
{
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
static int mediumPuzzles[5][BOARD_SIZE][BOARD_SIZE] =
{
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
static int hardPuzzles[5][BOARD_SIZE][BOARD_SIZE] =
{
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

/* Auth */
void signUp(void);
void signIn(void);
void logout(void);

/* Menus */
void printBanner(void);
void authMenu(void);
void mainMenu(void);
void underConstruction(void);

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

/* Hint */
void giveHint(void);

/* Tutorial */
void showTutorial(void);
FILE *openTutorialFile(void);

/* Utility */
void clearScreen(void);
void pressEnter(void);
void printSeparator(int width);
void printCentered(const char *text, int width);
const char *diffName(int diff);
int  findUser(const char *username);
int  findStatUser(const char *username);

/* ============================================================
 *                      FILE I/O FUNCTIONS
 * ============================================================ */

void loadUsers(void)
{
    FILE *f = fopen(FILE_USERS, "rb");
    if (!f)
    {
        memset(g_users, 0, sizeof(g_users));
        return;
    }
    fread(g_users, sizeof(User), MAX_USERS, f);
    fclose(f);
}

void saveUsers(void)
{
    FILE *f = fopen(FILE_USERS, "wb");
    if (!f)
    {
        printf("[ERROR] Cannot write %s\n", FILE_USERS);
        return;
    }
    fwrite(g_users, sizeof(User), MAX_USERS, f);
    fclose(f);
}

void loadAllData(void)
{
    loadUsers();
}

void saveAllData(void)
{
    saveUsers();
}

/* ============================================================
 *                    UTILITY FUNCTIONS
 * ============================================================ */

void clearScreen(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pressEnter(void)
{
    printf("\n  Press [ENTER] to continue...");
    /* flush stdin before waiting */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    getchar();
}

void printSeparator(int width)
{
    printf("  ");
    for (int i = 0; i < width; i++) printf("=");
    printf("\n");
}

void printCentered(const char *text, int width)
{
    int len   = (int)strlen(text);
    int left  = (width - len) / 2;
    printf("  ");
    for (int i = 0; i < left; i++) printf(" ");
    printf("%s\n", text);
}

const char *diffName(int diff)
{
    if (diff == DIFF_EASY)   return "Easy";
    if (diff == DIFF_MEDIUM) return "Medium";
    if (diff == DIFF_HARD)   return "Hard";
    return "Unknown";
}

int findUser(const char *username)
{
    for (int i = 0; i < MAX_USERS; i++)
        if (g_users[i].active && strcmp(g_users[i].username, username) == 0)
            return i;
    return -1;
}


/* ============================================================
 *                      BANNER & MENUS
 * ============================================================ */

void printBanner(void)
{
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
void printBannerASCII(void)
{
    clearScreen();
    printf("\n");
    printSeparator(37);
    printCentered("SUDOKU  MASTER  PRO", 37);
    printCentered("Console Edition  v1.0", 37);
    printSeparator(37);
    printf("\n");
}

void authMenu(void)
{
    int choice;
    do
    {
        printBannerASCII();
        printf("  +-----------------------------+\n");
        printf("  |        WELCOME MENU         |\n");
        printf("  +-----------------------------+\n");
        printf("  |  1. Sign In                 |\n");
        printf("  |  2. Sign Up                 |\n");
        printf("  |  3. Exit                    |\n");
        printf("  +-----------------------------+\n");
        printf("\n  Enter choice: ");
        if (scanf("%d", &choice) != 1)
        {
            choice = 0;
            while(getchar()!='\n');
        }

        switch (choice)
        {
        case 1:
            signIn();
            break;
        case 2:
            signUp();
            break;
        case 3:
            printf("\n  Goodbye!\n\n");
            saveAllData();
            exit(0);
        default:
            printf("\n  [!] Invalid choice.\n");
            pressEnter();
        }
    }
    while (!g_loggedIn);
}

/* Shown for menu options reserved for a future version. */
void underConstruction(void)
{
    printf("\n");
    printf("  ==========================================\n");
    printf("  This feature is currently under construction.\n");
    printf("  It will be available in a future version.\n");
    printf("  ==========================================\n");
    pressEnter();
}

void mainMenu(void)
{
    int choice;
    do
    {
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
        if (scanf("%d", &choice) != 1)
        {
            choice = 0;
            while(getchar()!='\n');
        }

        switch (choice)
        {
        case 1:
            newGame();
            break;
        case 2:
            underConstruction();
            break; /* Continue Game - future version */
        case 3:
            underConstruction();
            break; /* High Scores   - future version */
        case 4:
            showTutorial();
            break;
        case 5:
            underConstruction();
            break; /* Profile Statistics - future version */
        case 6:
            logout();
            return;
        case 7:
            saveAllData();
            printf("\n  Goodbye!\n\n");
            exit(0);
        default:
            printf("\n  [!] Invalid option.\n");
            pressEnter();
        }
    }
    while (g_loggedIn);
}

/* ============================================================
 *                   AUTHENTICATION FUNCTIONS
 * ============================================================ */

void signUp(void)
{
    char uname[USERNAME_LEN], pass[PASSWORD_LEN], pass2[PASSWORD_LEN];
    printBannerASCII();
    printf("  +-----------------------------+\n");
    printf("  |           SIGN UP           |\n");
    printf("  +-----------------------------+\n\n");

    printf("  Username (max 31 chars, no spaces): ");
    if (scanf("%31s", uname) != 1) return;

    /* Check for spaces */
    for (int i = 0; uname[i]; i++)
    {
        if (isspace((unsigned char)uname[i]))
        {
            printf("\n  [!] Username cannot contain spaces.\n");
            pressEnter();
            return;
        }
    }

    /* Check duplicate */
    if (findUser(uname) >= 0)
    {
        printf("\n  [!] Username '%s' already exists.\n", uname);
        pressEnter();
        return;
    }

    printf("  Password (max 31 chars): ");
    if (scanf("%31s", pass) != 1) return;

    if (strlen(pass) < 4)
    {
        printf("\n  [!] Password must be at least 4 characters.\n");
        pressEnter();
        return;
    }

    printf("  Confirm Password: ");
    if (scanf("%31s", pass2) != 1) return;

    if (strcmp(pass, pass2) != 0)
    {
        printf("\n  [!] Passwords do not match.\n");
        pressEnter();
        return;
    }

    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_USERS; i++)
    {
        if (!g_users[i].active)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0)
    {
        printf("\n  [!] User database is full.\n");
        pressEnter();
        return;
    }

    strncpy(g_users[slot].username, uname, USERNAME_LEN - 1);
    strncpy(g_users[slot].password, pass,  PASSWORD_LEN - 1);
    g_users[slot].active = 1;
    saveUsers();

    printf("\n  [OK] Account created! You can now sign in.\n");
    pressEnter();
}

void signIn(void)
{
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
    if (idx < 0 || strcmp(g_users[idx].password, pass) != 0)
    {
        printf("\n  [!] Invalid username or password.\n");
        pressEnter();
        return;
    }

    strncpy(g_currentUser, uname, USERNAME_LEN - 1);
    g_loggedIn = 1;
    printf("\n  [OK] Welcome back, %s!\n", g_currentUser);
    pressEnter();
}

void logout(void)
{
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
void copyBoard(int dst[BOARD_SIZE][BOARD_SIZE], int src[BOARD_SIZE][BOARD_SIZE])
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            dst[r][c] = src[r][c];
}

/* Returns 1 if placing 'num' at (row,col) is valid */
int isValidMove(int board[BOARD_SIZE][BOARD_SIZE], int row, int col, int num)
{
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
int solveSudoku(int board[BOARD_SIZE][BOARD_SIZE])
{
    for (int row = 0; row < BOARD_SIZE; row++)
    {
        for (int col = 0; col < BOARD_SIZE; col++)
        {
            if (board[row][col] == 0)
            {
                for (int num = 1; num <= 9; num++)
                {
                    if (isValidMove(board, row, col, num))
                    {
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
int isBoardComplete(int board[BOARD_SIZE][BOARD_SIZE])
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (board[r][c] == 0) return 0;
    return 1;
}

/* Pick a random puzzle from the bank for the given difficulty */
void selectPuzzle(int diff, int puzzle[BOARD_SIZE][BOARD_SIZE])
{
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

void printBoard(void)
{

    printf("\n");
    printf("  +------------------------------------------+\n");
    printf("  |  Player  : %-20s          |\n", g_currentUser);
    printf("  |  Wrongs  : %-3d                           |\n",
           g_currentGame.wrongMoves);
    printf("  +------------------------------------------+\n\n");

    /* Column headers */
    printf("        1   2   3   4   5   6   7   8   9\n");
    printf("      +===========+===========+===========+\n");

    for (int r = 0; r < BOARD_SIZE; r++)
    {
        printf("   %d  |", r + 1);
        for (int c = 0; c < BOARD_SIZE; c++)
        {
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

void newGame(void)
{
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
    if (scanf("%d", &choice) != 1)
    {
        while(getchar()!='\n');
        return;
    }
    if (choice < 1 || choice > 3) return;

    /* Initialise game struct */
    memset(&g_currentGame, 0, sizeof(Game));
    strncpy(g_currentGame.username, g_currentUser, USERNAME_LEN - 1);
    g_currentGame.difficulty = choice;
    g_currentGame.hintsLeft  = (choice == DIFF_EASY) ? 5 :
                               (choice == DIFF_MEDIUM) ? 3 : 1;

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

/* ============================================================
 *                   HINT & SCORE FUNCTIONS
 * ============================================================ */

void giveHint(void)
{
    if (g_currentGame.hintsLeft <= 0)
    {
        printf("\n  [!] No hints remaining!\n");
        pressEnter();
        return;
    }

    /* Find a random empty cell and reveal solution */
    int empties[81][2];
    int count = 0;
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            if (g_currentGame.board[r][c] == 0)
            {
                empties[count][0] = r;
                empties[count][1] = c;
                count++;
            }

    if (count == 0)
    {
        printf("\n  [!] Board is already complete.\n");
        pressEnter();
        return;
    }

    srand((unsigned int)time(NULL));
    int pick = rand() % count;
    int r    = empties[pick][0];
    int c    = empties[pick][1];

    g_currentGame.board[r][c] = g_currentGame.solution[r][c];
    g_currentGame.hintsLeft--;
    g_currentGame.hintsUsed++;

    printf("\n  [HINT] Revealed cell (%d, %d) = %d\n",
           r + 1, c + 1, g_currentGame.solution[r][c]);
    pressEnter();
}


/* ============================================================
 *                     PLAY LOOP
 * ============================================================ */

void playGame(void)
{
    int  choice, row, col, num;

    while (1)
    {
        printBoard();

        /* Check for completion */
        if (isBoardComplete(g_currentGame.board))
        {

            printf("\n");
            printSeparator(42);
            printCentered("CONGRATULATIONS! PUZZLE SOLVED!", 42);
            printSeparator(42);
            printf("\n");
            printf("  Difficulty : %s\n",  diffName(g_currentGame.difficulty));
            printf("\n");

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

        if (scanf("%d", &choice) != 1)
        {
            while(getchar()!='\n');
            continue;
        }

        switch (choice)
        {

        /* ---- Enter a number ---- */
        case 1:
            printf("  Row    (1-9): ");
            if (scanf("%d", &row) != 1)
            {
                while(getchar()!='\n');
                break;
            }
            printf("  Column (1-9): ");
            if (scanf("%d", &col) != 1)
            {
                while(getchar()!='\n');
                break;
            }
            printf("  Number (1-9): ");
            if (scanf("%d", &num) != 1)
            {
                while(getchar()!='\n');
                break;
            }

            row--;
            col--;  /* convert to 0-indexed */

            if (row < 0 || row >= BOARD_SIZE ||
                    col < 0 || col >= BOARD_SIZE)
            {
                printf("\n  [!] Row/column out of range.\n");
                pressEnter();
                break;
            }
            if (num < 1 || num > 9)
            {
                printf("\n  [!] Number must be 1-9.\n");
                pressEnter();
                break;
            }
            if (g_currentGame.locked[row][col])
            {
                printf("\n  [!] That cell is locked (original number).\n");
                pressEnter();
                break;
            }

            /* Check if the move matches the solution */
            if (g_currentGame.solution[row][col] == num)
            {
                g_currentGame.board[row][col] = num;
                printf("\n  [OK] Correct move!\n");
            }
            else
            {
                g_currentGame.wrongMoves++;
                printf("\n  [X] Incorrect move! Wrong move count: %d\n", g_currentGame.wrongMoves);
            }

            pressEnter();
            break;

        /* ---- Hint ---- */
        case 2:
            giveHint();
            break;

        /* ---- Save (feature disabled, menu entry kept) ---- */
        case 3:
            underConstruction();
            break;

        /* ---- Quit ---- */
        case 4:
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
 *                       TUTORIAL
 * ============================================================ */

FILE *openTutorialFile(void)
{
    static const char *candidates[] =
    {
        "tutorial.txt",
        "../tutorial.txt",
        "../../tutorial.txt",
        "../../../tutorial.txt"
    };
    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++)
    {
        FILE *f = fopen(candidates[i], "r");
        if (f) return f;
    }
    return NULL;
}

void showTutorial(void)
{
    clearScreen();
    printSeparator(50);
    printCentered("SUDOKU MASTER PRO -- TUTORIAL", 50);
    printSeparator(50);
    printf("\n");

    /* Tutorial content now lives in tutorial.txt, not hardcoded here. */
    FILE *f = openTutorialFile();
    if (!f)
    {
        printf("  [ERROR] Could not open tutorial.txt.\n");
        printf("  Please make sure tutorial.txt is in the same folder\n");
        printf("  as the program executable (or the project root).\n\n");
        pressEnter();
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f) != NULL)
    {
        printf("%s", line);
    }

    fclose(f);
    printf("\n");
    pressEnter();
}

/* ============================================================
 *                          MAIN
 * ============================================================ */

int main(void)
{
    /* Load all persistent data from files */
    loadAllData();

    /* Authentication loop */
    authMenu();

    /* Main game loop */
    while (g_loggedIn)
    {
        mainMenu();

        /* After logout, offer to re-login */
        if (!g_loggedIn)
        {
            printf("\n  Return to login screen? (y/n): ");
            char c;
            scanf(" %c", &c);
            if (c == 'y' || c == 'Y')
            {
                authMenu();
            }
            else if (c == 'n' || c == 'N')
            {
                printf("\n  Thank you for playing Sudoku Master Pro!\n\n");
                break;
            }
            else
            {
                printf("\n  [!] Invalid choice. Continuing session...\n");
                g_loggedIn = 1; // Keep the loop running
            }
        }
    }

    saveAllData();
    return 0;
}
