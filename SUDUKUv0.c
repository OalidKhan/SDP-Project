/* ============================================================
 *          SUDOKU MASTER PRO
 *
 *          NOTE: Continue Game, High Scores and Profile
 *          Statistics are placeholders for a future version
 *          and are intentionally not implemented (see the
 *          "under construction" message below).
 *
 *          MILESTONE SCOPE NOTE: There is intentionally NO
 *          Sudoku rule validation, no win/completion detection,
 *          no scoring, no hints, and no save/load logic.
 *          Players may freely enter or erase numbers in
 *          non-fixed cells. These are reserved for a future
 *          version of the project.
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
 *                     CONSTANTS & MACROS
 * ============================================================ */
#define USERNAME_LEN   32
#define PASSWORD_LEN   32
#define MAX_USERS      50
#define FILE_USERS     "users.dat"

#define BOARD_SIZE     9
#define BOX_SIZE       3

#define DIFF_EASY      1
#define DIFF_MEDIUM    2
#define DIFF_HARD      3

/* ============================================================
 *                     STRUCTURE DEFINITIONS
 * ============================================================ */

/* Registered user account, stored in users.dat */
typedef struct {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    int  active;   /* 1 = slot in use, 0 = free */
} User;

/* Current Sudoku board state */
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];   /* current, editable board  */
    int locked[BOARD_SIZE][BOARD_SIZE];  /* 1 = fixed/original cell  */
    int difficulty;
} Board;

/* ============================================================
 *                     GLOBAL VARIABLES
 * ============================================================ */
static User g_users[MAX_USERS];
static Board g_board;

static char g_currentUser[USERNAME_LEN] = "";
static int  g_loggedIn = 0;

/* ============================================================
 *              PREDEFINED PUZZLE BANK (one per difficulty)
 *              0 = empty cell for the player to fill in
 * ============================================================ */

static const int easyPuzzle[BOARD_SIZE][BOARD_SIZE] = {
    {5,3,0, 0,7,0, 0,0,0},
    {6,0,0, 1,9,5, 0,0,0},
    {0,9,8, 0,0,0, 0,6,0},
    {8,0,0, 0,6,0, 0,0,3},
    {4,0,0, 8,0,3, 0,0,1},
    {7,0,0, 0,2,0, 0,0,6},
    {0,6,0, 0,0,0, 2,8,0},
    {0,0,0, 4,1,9, 0,0,5},
    {0,0,0, 0,8,0, 0,7,9}
};

static const int mediumPuzzle[BOARD_SIZE][BOARD_SIZE] = {
    {0,0,0, 2,0,0, 0,6,3},
    {3,0,0, 0,0,5, 4,0,1},
    {0,0,1, 0,0,3, 9,8,0},
    {0,0,0, 0,0,0, 0,9,0},
    {0,0,0, 5,3,8, 0,0,0},
    {0,3,0, 0,0,0, 0,0,0},
    {0,2,6, 3,0,0, 5,0,0},
    {5,0,3, 7,0,0, 0,0,8},
    {4,7,0, 0,0,1, 0,0,0}
};

static const int hardPuzzle[BOARD_SIZE][BOARD_SIZE] = {
    {0,0,0, 0,0,0, 0,0,0},
    {0,0,0, 0,0,3, 0,8,5},
    {0,0,1, 0,2,0, 0,0,0},
    {0,0,0, 5,0,7, 0,0,0},
    {0,0,4, 0,0,0, 1,0,0},
    {0,9,0, 0,0,0, 0,0,0},
    {5,0,0, 0,0,0, 0,7,3},
    {0,0,2, 0,1,0, 0,0,0},
    {0,0,0, 0,4,0, 0,0,9}
};

/* ============================================================
 *                   FORWARD DECLARATIONS
 * ============================================================ */

/* Shared console UI utilities */
void clearScreen(void);
void pressEnter(void);
void printSeparator(int width);
void printCentered(const char *text, int width);
void printBanner(void);

/* Authentication */
void loadUsers(void);
void saveUsers(void);
void signUp(void);
void signIn(void);
void logout(void);
int  findUser(const char *username);

/* Gameplay */
static const char *diffName(int diff);
static void loadPuzzle(int diff);
static void printBoard(void);
static int  promptCell(int *row, int *col);
static void enterNumber(void);
static void eraseNumber(void);
static void playGame(void);
void newGame(void);

/* Tutorial */
void showTutorial(void);

/* Screens */
static void underConstruction(void);
static void homeScreen(void);
static void mainMenu(void);

/* ============================================================
 *                    SHARED CONSOLE UTILITIES
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
    while ((c = getchar()) != '\n' && c != EOF) { /* flush stdin */ }
    getchar();
}

void printSeparator(int width) {
    printf("  ");
    for (int i = 0; i < width; i++) printf("=");
    printf("\n");
}

void printCentered(const char *text, int width) {
    int len  = (int)strlen(text);
    int left = (width - len) / 2;
    if (left < 0) left = 0;
    printf("  ");
    for (int i = 0; i < left; i++) printf(" ");
    printf("%s\n", text);
}

void printBanner(void) {
    clearScreen();
    printf("\n");
    printSeparator(36);
    printCentered("SUDOKU MASTER PRO", 36);
    printSeparator(36);
    printf("\n");
}

/* ============================================================
 *                      FILE HANDLING (users.dat)
 * ============================================================ */

void loadUsers(void) {
    FILE *f = fopen(FILE_USERS, "rb");
    if (!f) {
        memset(g_users, 0, sizeof(g_users));
        return;
    }
    fread(g_users, sizeof(User), MAX_USERS, f);
    fclose(f);
}

void saveUsers(void) {
    FILE *f = fopen(FILE_USERS, "wb");
    if (!f) {
        printf("[ERROR] Cannot write %s\n", FILE_USERS);
        return;
    }
    fwrite(g_users, sizeof(User), MAX_USERS, f);
    fclose(f);
}

/* Returns index of user in g_users[], or -1 if not found */
int findUser(const char *username) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (g_users[i].active && strcmp(g_users[i].username, username) == 0)
            return i;
    }
    return -1;
}

/* ============================================================
 *                   AUTHENTICATION FUNCTIONS
 * ============================================================ */

void signUp(void) {
    char uname[USERNAME_LEN], pass[PASSWORD_LEN], pass2[PASSWORD_LEN];

    printBanner();
    printf("  +-----------------------------+\n");
    printf("  |           SIGN UP           |\n");
    printf("  +-----------------------------+\n\n");

    printf("  Username (max 31 chars, no spaces): ");
    if (scanf("%31s", uname) != 1) return;

    /* Reject usernames containing whitespace */
    for (int i = 0; uname[i]; i++) {
        if (isspace((unsigned char)uname[i])) {
            printf("\n  [!] Username cannot contain spaces.\n");
            pressEnter();
            return;
        }
    }

    /* Enforce username uniqueness */
    if (findUser(uname) >= 0) {
        printf("\n  [!] Username '%s' already exists.\n", uname);
        pressEnter();
        return;
    }

    printf("  Password (min 4 chars, max 31): ");
    if (scanf("%31s", pass) != 1) return;

    if (strlen(pass) < 4) {
        printf("\n  [!] Password must be at least 4 characters.\n");
        pressEnter();
        return;
    }

    printf("  Confirm Password: ");
    if (scanf("%31s", pass2) != 1) return;

    if (strcmp(pass, pass2) != 0) {
        printf("\n  [!] Passwords do not match.\n");
        pressEnter();
        return;
    }

    /* Locate a free slot in the user table */
    int slot = -1;
    for (int i = 0; i < MAX_USERS; i++) {
        if (!g_users[i].active) { slot = i; break; }
    }
    if (slot < 0) {
        printf("\n  [!] User database is full.\n");
        pressEnter();
        return;
    }

    strncpy(g_users[slot].username, uname, USERNAME_LEN - 1);
    g_users[slot].username[USERNAME_LEN - 1] = '\0';
    strncpy(g_users[slot].password, pass, PASSWORD_LEN - 1);
    g_users[slot].password[PASSWORD_LEN - 1] = '\0';
    g_users[slot].active = 1;

    saveUsers();

    printf("\n  [OK] Account created successfully! You can now sign in.\n");
    pressEnter();
}

void signIn(void) {
    char uname[USERNAME_LEN], pass[PASSWORD_LEN];

    printBanner();
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
        pressEnter();
        return;
    }

    strncpy(g_currentUser, uname, USERNAME_LEN - 1);
    g_currentUser[USERNAME_LEN - 1] = '\0';
    g_loggedIn = 1;

    printf("\n  [OK] Welcome back, %s!\n", g_currentUser);
    pressEnter();
}

void logout(void) {
    g_loggedIn = 0;
    memset(g_currentUser, 0, sizeof(g_currentUser));
    printf("\n  You have been logged out.\n");
    pressEnter();
}

/* ============================================================
 *                      GAMEPLAY HELPERS
 * ============================================================ */

static const char *diffName(int diff) {
    if (diff == DIFF_EASY)   return "Easy";
    if (diff == DIFF_MEDIUM) return "Medium";
    if (diff == DIFF_HARD)   return "Hard";
    return "Unknown";
}

/* Loads the predefined puzzle for the given difficulty into g_board,
 * and marks every non-zero starting cell as locked. */
static void loadPuzzle(int diff) {
    const int (*source)[BOARD_SIZE];

    if (diff == DIFF_EASY)        source = easyPuzzle;
    else if (diff == DIFF_MEDIUM) source = mediumPuzzle;
    else                          source = hardPuzzle;

    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            g_board.board[r][c]  = source[r][c];
            g_board.locked[r][c] = (source[r][c] != 0) ? 1 : 0;
        }
    }
    g_board.difficulty = diff;
}

/* ============================================================
 *                        PRINT BOARD
 * ============================================================ */

static void printBoard(void) {
    printf("\n");
    printf("  +------------------------------------------+\n");
    printf("  |  Player : %-20s               |\n", g_currentUser);
    printf("  |  Diff.  : %-10s                       |\n", diffName(g_board.difficulty));
    printf("  +------------------------------------------+\n\n");

    printf("        1   2   3   4   5   6   7   8   9\n");
    printf("      +===========+===========+===========+\n");

    for (int r = 0; r < BOARD_SIZE; r++) {
        printf("   %d  |", r + 1);
        for (int c = 0; c < BOARD_SIZE; c++) {
            int val = g_board.board[r][c];

            if (val == 0)
                printf(" . ");
            else if (g_board.locked[r][c])
                printf(" %d ", val);
            else
                printf("(%d)", val);

            if ((c + 1) % BOX_SIZE == 0) printf("|");
            else printf(" ");
        }
        printf("\n");

        if ((r + 1) % BOX_SIZE == 0 && r < BOARD_SIZE - 1)
            printf("      +===========+===========+===========+\n");
        else if (r < BOARD_SIZE - 1)
            printf("      +---+---+---+---+---+---+---+---+---+\n");
    }
    printf("      +===========+===========+===========+\n");
    printf("\n  Legend: n = original cell   (n) = your entry   . = empty\n");
}

/* ============================================================
 *                    CELL EDITING FUNCTIONS
 * ============================================================ */

/* Prompts for row/column and validates they are within range and
 * not a locked/fixed cell. Returns 1 on success, 0 on failure
 * (an appropriate message has already been printed). */

static int promptCell(int *row, int *col) {
    printf("  Row    (1-9): ");
    if (scanf("%d", row) != 1) { while (getchar() != '\n'); return 0; }
    printf("  Column (1-9): ");
    if (scanf("%d", col) != 1) { while (getchar() != '\n'); return 0; }

    (*row)--; (*col)--; /* convert to 0-indexed */

    if (*row < 0 || *row >= BOARD_SIZE || *col < 0 || *col >= BOARD_SIZE) {
        printf("\n  [!] Row/column must be between 1 and 9.\n");
        pressEnter();
        return 0;
    }
    if (g_board.locked[*row][*col]) {
        printf("\n  [!] That cell is an original number and cannot be changed.\n");
        pressEnter();
        return 0;
    }
    return 1;
}

static void enterNumber(void) {
    int row, col, num;
    if (!promptCell(&row, &col)) return;

    printf("  Number (1-9): ");
    if (scanf("%d", &num) != 1) { while (getchar() != '\n'); return; }

    if (num < 1 || num > 9) {
        printf("\n  [!] Number must be between 1 and 9.\n");
        pressEnter();
        return;
    }

    g_board.board[row][col] = num;
    printf("\n  [OK] Cell updated.\n");
    pressEnter();
}

static void eraseNumber(void) {
    int row, col;
    if (!promptCell(&row, &col)) return;

    g_board.board[row][col] = 0;
    printf("\n  [OK] Cell cleared.\n");
    pressEnter();
}

/* ============================================================
 *                        PLAY LOOP
 * ============================================================ */

static void playGame(void) {
    int choice;

    while (1) {
        printBanner();
        printBoard();

        printf("\n");
        printf("  +----------------------------------+\n");
        printf("  |            GAME MENU              |\n");
        printf("  +----------------------------------+\n");
        printf("  |  1. Enter a number                |\n");
        printf("  |  2. Erase a number                 |\n");
        printf("  |  3. Return to Main Menu           |\n");
        printf("  +----------------------------------+\n");
        printf("\n  Choice: ");

        if (scanf("%d", &choice) != 1) { while (getchar() != '\n'); continue; }

        switch (choice) {
            case 1: enterNumber(); break;
            case 2: eraseNumber(); break;
            case 3: return;
            default:
                printf("\n  [!] Invalid option.\n");
                pressEnter();
        }
    }
}

/* ============================================================
 *                    DIFFICULTY SELECTION
 * ============================================================ */

void newGame(void) {
    int choice;

    do {
        printBanner();
        printf("  +-----------------------------+\n");
        printf("  |       SELECT DIFFICULTY     |\n");
        printf("  +-----------------------------+\n");
        printf("  |  1. Easy                    |\n");
        printf("  |  2. Medium                  |\n");
        printf("  |  3. Hard                    |\n");
        printf("  |  4. Back                    |\n");
        printf("  +-----------------------------+\n");
        printf("\n  Choice: ");

        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); /* flush invalid input */
            choice = 0;
        }

        if (choice < 1 || choice > 4) {
            printf("\n  [!] Invalid option.\n");
            pressEnter();
        }
    } while (choice < 1 || choice > 4);

    if (choice == 4) return; /* Back to Main Menu */

    loadPuzzle(choice);

    printf("\n  [OK] New %s game started! Good luck.\n", diffName(choice));
    pressEnter();

    playGame();
}

/* ============================================================
 *                          TUTORIAL
 * ============================================================ */

void showTutorial(void) {
    printBanner();
    printSeparator(46);
    printCentered("HOW TO PLAY SUDOKU", 46);
    printSeparator(46);

    printf("\n");
    printf("  WHAT IS SUDOKU?\n");
    printf("  ---------------\n");
    printf("  Sudoku is a logic-based number placement puzzle\n");
    printf("  played on a 9x9 grid, divided into nine 3x3 boxes.\n\n");

    printf("  THE RULES\n");
    printf("  ---------\n");
    printf("  1. Fill every empty cell with a number from 1 to 9.\n");
    printf("  2. Each ROW should contain 1-9 without repeats.\n");
    printf("  3. Each COLUMN should contain 1-9 without repeats.\n");
    printf("  4. Each 3x3 BOX should contain 1-9 without repeats.\n\n");

    printf("  HOW TO USE THE CONTROLS\n");
    printf("  ------------------------\n");
    printf("  From the Game Menu you can:\n");
    printf("  1 -> Enter a number   (choose a row, column, then digit)\n");
    printf("  2 -> Erase a number   (clears a cell you previously filled)\n");
    printf("  3 -> Return to the Main Menu\n\n");
    printf("  Note: Original puzzle numbers are locked and cannot be\n");
    printf("  changed or erased.\n\n");

    printf("  DIFFICULTY LEVELS\n");
    printf("  ------------------\n");
    printf("  Easy   -> More starting numbers, fewer empty cells.\n");
    printf("  Medium -> A balanced number of starting clues.\n");
    printf("  Hard   -> Very few starting numbers, more of a challenge.\n\n");

    printf("  BOARD LEGEND\n");
    printf("  ------------\n");
    printf("   n   = original (locked) cell -- cannot be changed\n");
    printf("  (n)  = number YOU entered\n");
    printf("   .   = empty cell to fill\n\n");

    printf("  PROJECT OBJECTIVE\n");
    printf("  ------------------\n");
    printf("  Sudoku Master Pro is a Software Development Project\n");
    printf("  demonstrating modular C programming: user authentication,\n");
    printf("  file handling, and a console-based Sudoku board interface.\n");
    printf("  This milestone focuses on account management and letting\n");
    printf("  players load and edit a puzzle; rule checking and scoring\n");
    printf("  are planned for a future version.\n\n");

    pressEnter();
}

/* ============================================================
 *                      SCREENS & MENUS
 * ============================================================ */

/* Shown for menu options reserved for a future version. */
static void underConstruction(void) {
    printf("\n");
    printf("  ==========================================\n");
    printf("  This feature is currently under construction.\n");
    printf("  It will be available in a future version.\n");
    printf("  ==========================================\n");
    pressEnter();
}

/* Home screen: Sign In / Sign Up / Exit. Loops until the user logs in. */
static void homeScreen(void) {
    int choice;

    do {
        printBanner();
        printf("  1. Sign In\n");
        printf("  2. Sign Up\n");
        printf("  3. Exit\n\n");
        printf("  Enter choice: ");

        if (scanf("%d", &choice) != 1) { while (getchar() != '\n'); choice = 0; }

        switch (choice) {
            case 1: signIn(); break;
            case 2: signUp(); break;
            case 3:
                printf("\n  Goodbye!\n\n");
                exit(0);
            default:
                printf("\n  [!] Invalid choice.\n");
                pressEnter();
        }
    } while (!g_loggedIn);
}

/* Main menu, shown after a successful login. */
static void mainMenu(void) {
    int choice;

    while (g_loggedIn) {
        printBanner();
        printf("  Logged in as: [ %s ]\n\n", g_currentUser);
        printf("  ==========================\n");
        printf("          MAIN MENU\n");
        printf("  ==========================\n\n");
        printf("  1. New Game\n");
        printf("  2. Continue Game\n");
        printf("  3. High Scores\n");
        printf("  4. Tutorial\n");
        printf("  5. Profile Statistics\n");
        printf("  6. Logout\n");
        printf("  7. Exit\n\n");
        printf("  Enter choice: ");

        if (scanf("%d", &choice) != 1) { while (getchar() != '\n'); choice = 0; }

        switch (choice) {
            case 1: newGame();          break;
            case 2: underConstruction();break;
            case 3: underConstruction();break;
            case 4: showTutorial();     break;
            case 5: underConstruction();break;
            case 6: logout();           break;
            case 7:
                printf("\n  Goodbye!\n\n");
                exit(0);
            default:
                printf("\n  [!] Invalid option.\n");
                pressEnter();
        }
    }
}

/* ============================================================
 *                          MAIN
 * ============================================================ */

int main(void) {
    loadUsers();

    while (1) {
        homeScreen();   /* returns once g_loggedIn == 1 */
        mainMenu();     /* returns when the user logs out */
    }

    return 0;
}
