#include <stdio.h>
#define SIZE 9
int board[SIZE][SIZE] = {
    {5,3,0,0,7,0,0,0,0},
    {6,0,0,1,9,5,0,0,0},
    {0,9,8,0,0,0,0,6,0},
    {8,0,0,0,6,0,0,0,3},
    {4,0,0,8,0,3,0,0,1},
    {7,0,0,0,2,0,0,0,6},
    {0,6,0,0,0,0,2,8,0},
    {0,0,0,4,1,9,0,0,5},
    {0,0,0,0,8,0,0,7,9}
};

int fixed[SIZE][SIZE];
void initializeFixed() {
    for(int i=0;i<SIZE;i++) {
        for(int j=0;j<SIZE;j++) {
            if(board[i][j] != 0)
                fixed[i][j] = 1;
            else
                fixed[i][j] = 0;
        }
    }
}

void printBoard() {
    printf("\n");
    for(int i=0;i<SIZE;i++) {
        if(i % 3 == 0)
            printf(" -------------------------\n");
        for(int j=0;j<SIZE;j++) {
            if(j % 3 == 0)
                printf(" |");
            if(board[i][j] == 0)
                printf(" .");
            else
                printf(" %d", board[i][j]);
        }

        printf(" |\n");
    }

    printf(" -------------------------\n");
}

int isValid(int row, int col, int num) {

    for(int i=0;i<9;i++) {
        if(board[row][i] == num)
            return 0;
    }

    for(int i=0;i<9;i++) {
        if(board[i][col] == num)
            return 0;
    }

    int startRow = row - row % 3;
    int startCol = col - col % 3;

    for(int i=0;i<3;i++) {
        for(int j=0;j<3;j++) {
            if(board[startRow+i][startCol+j] == num)
                return 0;
        }
    }

    return 1;
}

int isComplete() {

    for(int i=0;i<9;i++) {
        for(int j=0;j<9;j++) {
            if(board[i][j] == 0)
                return 0;
        }
    }

    return 1;
}

int main() {

    initializeFixed();

    int row, col, num;

    while(1) {

        printBoard();

        if(isComplete()) {
            printf("\nCongratulations! You solved the Sudoku!\n");
            break;
        }

        printf("\nEnter Row (1-9) [0 to exit]: ");
        scanf("%d", &row);

        if(row == 0)
            break;

        printf("Enter Column (1-9): ");
        scanf("%d", &col);

        printf("Enter Number (1-9): ");
        scanf("%d", &num);

        row--;
        col--;

        if(row < 0 || row > 8 || col < 0 || col > 8 ||
           num < 1 || num > 9) {

            printf("\nInvalid input!\n");
            continue;
        }

        if(fixed[row][col]) {
            printf("\nYou cannot modify this cell!\n");
            continue;
        }

        if(isValid(row, col, num)) {
            board[row][col] = num;
        }
        else {
            printf("\nInvalid move according to Sudoku rules!\n");
        }
    }

    printf("\nGame Done.\n");

    return 0;
}
