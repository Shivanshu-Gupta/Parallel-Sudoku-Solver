#ifndef SUDOKU_H
#define SUDOKU_H

#ifndef MINIGRIDSIZE
#define MINIGRIDSIZE 3
#endif
#define SIZE ((MINIGRIDSIZE)*(MINIGRIDSIZE))

int **readInput(char *);
int isValid(int **, int **);
int **solveSudoku(int **);

#endif
