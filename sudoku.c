/*<2013CS50298>*/
// eliminate, lonerangers, twins implemented. twins for boxes not tested.
// lone rangers fixed with update peers.
// works on moodle submission.
// twins are useful on larger/harder grids.
// no solution returned from solveLogical when cell has no possible values.
// 25x25piazza.txt solved in a minimum of ~1.1 s for 10 threads.
// working to remove memory leaks.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <limits.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "sudoku.h"

#define true 1
#define false 0

typedef struct {
   // int** matrix;
   char* matrix;
   uint64_t* mask;
   int i;
} grid_struct;
typedef grid_struct* grid_t;

#define Curr_value(grid) (grid->matrix[grid->i])
#define idx(x, y) ( (x) * SIZE + (y) )
#define valCount(x) ( __builtin_popcountll(x) )
#define valSmallest(x) ( __builtin_ctzll(x) + 1 )

typedef struct {
   grid_t* list;
   int list_sz;
   int list_alloc;
}  stack_struct;
typedef stack_struct* stack_t;

typedef grid_t stackelem_t;

const int INIT_STACK_SIZE = 128;
const int M = SIZE, N = MINIGRIDSIZE;
extern int thread_count;

int soln_found = 0;
char* soln_matrix;
stack_t avail_stack;
stack_t allotment_stack;

// grid methods
void printMatrix(int **matrix);
int isValidGrid(grid_t grid);
void moveToNextUnsetCell(grid_t grid);
int getLeastUnsureCell(grid_t grid);
void Copy_grid(grid_t grid1, grid_t grid2);
void Populate_grid(grid_t grid, int **matrix);
grid_t Alloc_grid(stack_t avail);
void Free_grid(grid_t grid, stack_t avail);

// Stack Methods
stack_t Alloc_stack(void);
void Free_stack(stack_t stack);
int Empty_stack(stack_t stack);
void Push(stack_t stack, grid_t grid);
void Push_copy(stack_t stack, grid_t grid, stack_t avail);
grid_t Pop(stack_t stack);

// humanistic approach methods
void printPossibleValues(grid_t grid, int i);
int eliminate(grid_t grid);
void setCellPossibleValues(grid_t grid, int i);
int setLoneRangersRow(grid_t grid);
int setLoneRangersColumn(grid_t grid);
int setLoneRangersBox(grid_t grid);
int setTwinsRow(grid_t grid);
int setTwinsColumn(grid_t grid);
int setTwinsBox(grid_t grid);
void updatePeers(grid_t grid, int i);

// top level methods
int solveLogical(grid_t grid);
int Prepare_allotment_stack(grid_t grid);
int **solveSudoku(int **);


/*------------------------------------------------------------------*/

void printMatrix(int **matrix) {
   int i, j;
   for (i = 0; i < SIZE; i++) {
      for (j = 0; j < SIZE; j++) {
         printf("%2d ", matrix[i][j]);
      }
      printf("\n");
   }
   printf("\n");
}

void printGrid(grid_t grid) {
   int j;
   for(j = 0; j < SIZE * SIZE; j++) {
      if(j != grid->i){
         printf("%2d ", grid->matrix[j]);
      } else {
         printf("[%d]", grid->matrix[j]);
      }
      if(j % SIZE == SIZE-1) {
         printf("\n");
      }
   }
   printf("\n");
}

void printStack(stack_t stack) {
   printf("STACK: size = %d\n", stack->list_sz);
   int i;
   for(i = 0; i<stack->list_sz; i++)
      printGrid(stack->list[i]);
}

int isValidGrid(grid_t grid) {
   int i = grid->i;
   int x = i / M, y = i % M;
   int v = grid->matrix[i];

   int bx = (x / N) * N, by = (y / N) * N;
   int j, ox, oy;
   for (j = 0; j < SIZE; j++) {
      ox = j / N;
      oy = j % N;
      // check row
      if (y != j && v == grid->matrix[x * SIZE + j]) return false;

      // check column
      if (x != j && v == grid->matrix[j * SIZE + y]) return false;

      // check box
      if (i != ((bx + ox) * SIZE + by + oy) 
            && v == grid->matrix[(bx + ox) * SIZE + by + oy]) 
         return false;
   }
   return true;
}

void moveToNextUnsetCell(grid_t grid){
   while(grid->i < SIZE * SIZE && Curr_value(grid)) {
      grid->i++;
   }
}

int getLeastUnsureCell(grid_t grid) {
   int idx = 0, min = SIZE;
   int i;
   for(i = 0; i < SIZE * SIZE; i++) {
      if(grid->matrix[i] == 0 && valCount(grid->mask[i]) < min) {
         idx = i;
         min = valCount(grid->mask[idx]);
         // printf("min = %d\n", min);
      }
   }
   return idx;
}

void Copy_grid(grid_t grid1, grid_t grid2) {
   memcpy(grid2->matrix, grid1->matrix, SIZE*SIZE*sizeof(char));
   memcpy(grid2->mask, grid1->mask, SIZE*SIZE*sizeof(uint64_t));
   grid2->i = grid1->i;
}

void Populate_grid(grid_t grid, int **matrix) {
   int i;
   for(i = 0; i< SIZE*SIZE; i++) {
      grid->matrix[i] = matrix[i/SIZE][i%SIZE];
      if(grid->matrix[i] > 0) {
         grid->mask[i] = (1 << (grid->matrix[i] - 1));
      } else {
         // all values from 1 to SIZE possible for now.
         grid->mask[i] = SIZE == 64 ? ULLONG_MAX : ((1 << SIZE) - 1); 
      }
   }
}


grid_t Alloc_grid(stack_t avail) {
   grid_t tmp;
   if (avail == NULL || Empty_stack(avail)) {
      tmp = (grid_t) malloc(sizeof(grid_struct));
      tmp->matrix = (char *)malloc(SIZE*SIZE*sizeof(char));
      tmp->mask = (uint64_t *)malloc(SIZE*SIZE*sizeof(uint64_t));
      return tmp;
   } else {
      return Pop(avail);
   }
}

void Free_grid(grid_t grid, stack_t avail) {
   
   if(grid != NULL) {
      if (avail == NULL) {
         free(grid->matrix);
         free(grid->mask);
         free(grid);
      } else {
         // push the grid in the avail stack only if it wouldn't cause
         // reallocation of the stack
         Push(avail, grid);
      }
   }
}

/* Stack Methods */
stack_t Alloc_stack() {
   
   stack_t tmp;
   tmp = (stack_t)malloc(sizeof(stack_struct));
   tmp->list = (grid_t *)malloc((INIT_STACK_SIZE) * sizeof(grid_t));
   tmp->list_sz = 0;
   tmp->list_alloc = INIT_STACK_SIZE;
   return tmp;
}

void Free_stack(stack_t stack) {
   
   if(stack != NULL) {
      if(!Empty_stack(stack)) {
         int i = 0;
         for(; i<stack->list_sz; i++) {
            Free_grid(stack->list[i], NULL);
         }
      }
      free(stack->list);
      free(stack);
   }
}

int Empty_stack(stack_t stack) {
   return stack == NULL || !stack->list_sz;
}

void Push(stack_t stack, grid_t grid) {

   if(stack->list_sz == stack->list_alloc) {
      grid_t *tmp = (grid_t *)malloc((stack->list_alloc) * 2 * sizeof(grid_t));
      memcpy(tmp, stack->list, (stack->list_sz)*sizeof(grid_t));
      free(stack->list);
      stack->list = tmp;
      stack->list_alloc *= 2;
      // printf("stack->list_alloc = %d\n", stack->list_alloc);
   }

   stack->list[stack->list_sz++] = grid;
}

void Push_copy(stack_t stack, grid_t grid, stack_t avail) {
   grid_t tmp = Alloc_grid(avail);
   Copy_grid(grid, tmp);
   Push(stack, tmp);
}

grid_t Pop(stack_t stack) {
   
   if(Empty_stack(stack))
      return NULL;
   stack->list_sz--;
   return stack->list[stack->list_sz];
}
/* Stack Methods */



int Prepare_allotment_stack(grid_t grid) {
   grid_t curr_grid = Alloc_grid(avail_stack);

   // // Temporary stacks for work assignment
   stack_t stack1 = Alloc_stack(), stack2 = Alloc_stack();
   stack_t allot_stack1, allot_stack2;

   grid->i = 0;
   Push_copy(stack1, grid, avail_stack);

   int j = 0, idx, v;
   uint64_t vals;
   do {
      if(j % 2) {
         allot_stack1 = stack2;
         allot_stack2 = stack1;
      } else {
         allot_stack1 = stack1;
         allot_stack2 = stack2;
      }

      // pop from allot_stack1, expand next level, push into allot_atack2
      while(!Empty_stack(allot_stack1)) {
         curr_grid = Pop(allot_stack1);

         if(0) printGrid(curr_grid);
         curr_grid->i = 0;
         while(curr_grid->i < SIZE * SIZE && Curr_value(curr_grid))
            curr_grid->i++;
         if(curr_grid->i == SIZE * SIZE) {
            // solution has been found
            soln_found = 1;
            soln_matrix = curr_grid->matrix;
            return 1;
         }

         idx = getLeastUnsureCell(curr_grid);
         vals = curr_grid->mask[idx];
         v = 1;
         do {
            if(vals % 2){
               curr_grid->i = idx;
               curr_grid->matrix[idx] = v;
               Copy_grid(curr_grid, grid);
               updatePeers(grid, idx);
               if(isValidGrid(grid)){
                  Push_copy(allot_stack2, grid, avail_stack);
               }
            }
            vals /= 2;
            v++;
         } while(vals > 0);

         Free_grid(curr_grid, avail_stack);

      }

      if(Empty_stack(allot_stack2)) {
         // no solution possible
         return -1;
      }
      j++;
   } while(allot_stack2->list_sz < thread_count);

   if(grid != NULL) {
      Free_grid(grid, avail_stack);
   }
   if(curr_grid != NULL) {
      Free_grid(curr_grid, avail_stack);
   }

   allotment_stack = allot_stack2;
   return 0;
}

int solveLogical(grid_t grid) {
   int changes = false;
   int i;
   
   do {
      // printf("Setting Possible Values\n");
      for(i = 0; i < SIZE * SIZE; i++) {
         if(grid->matrix[i] == 0) {
            setCellPossibleValues(grid, i);

            if(grid->mask[i] == 0) return -1;
         }
         else 
            grid->mask[i] = (1 << (grid->matrix[i] - 1));
      }
      changes = eliminate(grid);
      // printGrid(grid);

      if(!changes) {
         changes = setLoneRangersRow(grid);
         if(!changes) {
            changes = setLoneRangersColumn(grid);
            if(!changes) {
               changes = setLoneRangersBox(grid);
               if(!changes) {
                  changes = setTwinsRow(grid);
                  if(!changes) {
                     changes = setTwinsColumn(grid);
                  }
               }
            }
         }
      }
      // TODO : set soln_found
   } while(!soln_found && changes);
   return 0;
}


int **solveSudoku(int ** original_matrix) {
   avail_stack = Alloc_stack();

   grid_t init_grid = Alloc_grid(avail_stack);
   Populate_grid(init_grid, original_matrix);
   init_grid->i = 0;

   int r = solveLogical(init_grid);
   if(r < 0) {
      goto end;
   }

   r = Prepare_allotment_stack(init_grid);
   if(r != 0){
      goto end;
   }

   if(0) printStack(allotment_stack);
   // exit(0);
   #pragma omp parallel shared(soln_found)
   {
      int tid, i, nthrds;
      int idx, v;
      uint64_t vals;
      grid_t curr_grid, grid = Alloc_grid(avail_stack);
      stack_t avail_stack_local = Alloc_stack();
      stack_t search_stack_local = Alloc_stack();
      
      tid = omp_get_thread_num();
      nthrds = omp_get_num_threads();
      for(i = tid; i < allotment_stack->list_sz && !soln_found; i+=nthrds) {
         curr_grid = allotment_stack->list[i];

         do {
            
            if(solveLogical(curr_grid) < 0){
               goto next;
            }
            curr_grid->i = 0;
            while(curr_grid->i < SIZE * SIZE && Curr_value(curr_grid))
               curr_grid->i++;
            if(curr_grid->i == SIZE*SIZE) {
               #pragma omp critical (soln)
               {
                  soln_found = 1;
                  soln_matrix = curr_grid->matrix;
               }
               break;
            }
            
            /*****************find the next cell to expand****************/
            idx = getLeastUnsureCell(curr_grid);
            vals = curr_grid->mask[idx];
            if(0) printPossibleValues(curr_grid, idx);
            v = 1;
            do {
               if(vals % 2){
                  curr_grid->i = idx;
                  curr_grid->matrix[idx] = v;
                  Copy_grid(curr_grid, grid);
                  updatePeers(grid, idx);
                  if(isValidGrid(grid)){
                     Push_copy(search_stack_local, grid, avail_stack_local);
                  }
               }
               vals /= 2;
               v++;
            } while(vals > 0);
            /*****************find the next cell to expand****************/

            next:
               Free_grid(curr_grid, avail_stack_local);
               curr_grid = Pop(search_stack_local);
         } while(curr_grid != NULL && !soln_found);
      }
      // Free_stack(search_stack_local);
      Free_stack(avail_stack_local);
   }
   // printMatrix(soln_matrix);
   
   end: ;
   //  ^ need to put a ';'as labels can be followed only by statements.
   // and declarations are not statements.
   int i;
   int** ret_matrix = (int **)malloc(SIZE * sizeof(int *));
   for(i = 0; i<SIZE; i++) {
      ret_matrix[i] = (int *)malloc(SIZE * sizeof(int));
   }

   // Free_stack(allotment_stack);
   // Free_stack(avail_stack);

   if(soln_found) {
      for(i = 0; i < SIZE * SIZE; i++) {
         ret_matrix[i/SIZE][i%SIZE] = soln_matrix[i];
      }
   }
   else {
      ret_matrix = original_matrix;
   }

   return ret_matrix;
}

int eliminate(grid_t grid) {
   uint64_t vals;
   int changes = false;
   int i;
   for(i = 0; i < SIZE * SIZE; i++) {
      vals = grid->mask[i];
      if(grid->matrix[i] == 0 && valCount(vals) == 1) {
         changes = true;
         grid->matrix[i] = valSmallest(vals);
         updatePeers(grid, i);
         if(0) printf("(%d, %d) set to %d\n", i/SIZE, i%SIZE, grid->matrix[i]);
      }
   }
   return changes;
}

// returns the possible values for current cell of grid.
void setCellPossibleValues(grid_t grid, int i) {
   if(grid->matrix[i] == 0) {
      // int i = grid->i;
      int x = i / SIZE, y = i % SIZE;
      // uint64_t possValues = grid->mask[i];
      // printPossibleValues(grid, i);
      int bx = (x / N) * N, by = (y / N) * N;
      int j, ox, oy, v;
      for(j = 0; j < SIZE; j++) {
         ox = j / N;
         oy = j % N;
         v = grid->matrix[idx(x,j)];
         // check row
         if (y != j && v > 0) {
            v = grid->matrix[idx(x,j)];
            // printf("row : %d\n", v);
            grid->mask[i] &= ~(1 << (v - 1));
            // printPossibleValues(grid, i);
         }

         v = grid->matrix[idx(j,y)];
         // check column
         if (x != j && v > 0) {
            // printf("column : %d\n", v);
            grid->mask[i] &= ~(1 << (v - 1));
            // printPossibleValues(grid, i);
         }

         v = grid->matrix[idx(bx + ox, by + oy)];
         // check box
         if (i != idx(bx + ox, by + oy) && v > 0) {
            // printf("box : %d\n", v);
            grid->mask[i] &= ~(1 << (v - 1));
            // printPossibleValues(grid, i);
         }
      }
   } else {
      grid->mask[i] = (1 << (grid->matrix[i] - 1));
   }

   // return possValues;
}

// set the values of the loners in each the row.
int setLoneRangersRow(grid_t grid) {
   int changes = false, totalChanges = false;
   int x, y;
   int i, j, v, count;

   // loop over all rows
   // repeat if change occurred.
   do {

      changes = false;
      //row number
      for(i = 0; i < SIZE; i++) {
         // value
         for(v = 0; v < SIZE; v++) {
            count = 0;
            // column number
            for(j = 0; j < SIZE; j++) {
               
               if(grid->matrix[idx(i, j)] == 0 
                  && (grid->mask[idx(i, j)] & (1 << v))) {
                  
                  count++;
                  
                  if(count > 1)
                     break;

                  x = i;
                  y = j;
               }
            }

            if(count == 1) {
               // this value occured in only one cell in row. Set it.
               grid->matrix[idx(x, y)] = v + 1;
               if(0) printf("(%d, %d) set to %d\n", x, y, grid->matrix[idx(x, y)]);
               updatePeers(grid, i);

               // no need to updatePeers() as they won't be touched in this call

               changes = true;
               totalChanges = true;
            }

            // TODO: if count = 0, then NO POSSIBLE SOLUTION!
         }
      }
   } while(changes);

   return totalChanges;
}

int setLoneRangersColumn(grid_t grid) {
   int changes = false, totalChanges = false;
   int x, y;
   int i, j, v, count;

   // loop over all columns
   // repeat if change occurred.
   do {

      changes = false;
      // column number
      for(j = 0; j < SIZE; j++) {
         // value
         for(v = 0; v < SIZE; v++) {
            count = 0;
            // row number
            for(i = 0; i < SIZE; i++) {
               
               if(grid->matrix[idx(i, j)] == 0 
                  && (grid->mask[idx(i, j)] & (1 << v))) {
                  
                  count++;
                  
                  if(count > 1)
                     break;

                  x = i;
                  y = j;
               }
            }

            if(count == 1) {
               // this value occured in only one cell in column. Set it.
               grid->matrix[idx(x, y)] = v + 1;
               if(0) printf("(%d, %d) set to %d\n", x, y, grid->matrix[idx(x, y)]);
               updatePeers(grid, i);
               // no need to updatePeers() as they won't be touched in this call

               changes = true;
               totalChanges = true;
            }

            // TODO: if count = 0, then NO POSSIBLE SOLUTION!
         }
      }
   }while(changes);

   return totalChanges;
}

int setLoneRangersBox(grid_t grid) {
   int changes = false, totalChanges = false;
   int x, y;
   int i, j, bi, bj, v, count;

   // loop over all boxes
   // repeat if change occurred.
   do {
      changes = false;
      // value
      for(v = 0; v < SIZE; v++) {
         // vertical box index
         for(bi = 0; bi < SIZE; bi+=N) {
            // horizontal box index
            for(bj = 0; bj < SIZE; bj+=N) {
               count = 0;
               
               // look for a loner in this box
               for(i = bi; i < bi + N; i++) {
                  for(j = bj; j < bj + N; j++) {

                     if(grid->matrix[idx(i, j)] == 0 
                        && (grid->mask[idx(i, j)] & (1 << v))) {
                        
                        count++;
                        
                        if(count > 1) {
                           // value v is not a loner in this box.
                           break;
                        }

                        x = i;
                        y = j;
                     }
                  }

                  if(count > 1) 
                     break;
               }

               if(count == 1) {
                  // v is a loner in this box. Set it.
                  grid->matrix[idx(x, y)] = v + 1;
                  if(0) printf("(%d, %d) set to %d\n", x, y, grid->matrix[idx(x, y)]);
                  updatePeers(grid, i);

                  // no need to updatePeers() as they won't be touched in this call

                  changes = true;
                  totalChanges = true;
               }

               // TODO: if count = 0, then NO POSSIBLE SOLUTION!
            }
         }
      }
   } while(changes);

   return totalChanges;
}

int setTwinsRow(grid_t grid) {
   int changes = false;
   int i, j, k, l;
   uint64_t vals;

   //row number
   for(i = 0; i < SIZE; i++) {
      // first cell with just two possible values
      for(j = 0; j < SIZE; j++) {
         vals = grid->mask[idx(i, j)];
         if(valCount(vals) == 2) {
            
            // second cell with just two possible values
            for(k = j + 1; k < SIZE; k++) {

               if(vals == grid->mask[idx(i, k)]) {
                  // twin found. Remove the pair of possible values from 
                  // all unset cells in row.
                  for(l = 0; l < SIZE; l++) {
                     if (l != j && l != k && grid->matrix[idx(i, l)] == 0 &&
                           (grid->mask[idx(i, l)] & vals)) {
                        grid->mask[idx(i, l)] &= ~vals;
                        changes = true;
                        if(0) {
                           printPossibleValues(grid, idx(i, l));
                        }

                        // leaving the elimination part for eliminate() as that will do 
                        // update peers as well.
                     }

                  }
               }   
            }
         }
      }
   }

   return changes;
}

int setTwinsColumn(grid_t grid) {
   int changes = false;
   int i, j, k, l;
   uint64_t vals;

   //column number
   for(i = 0; i < SIZE; i++) {
      // first cell with just two possible values
      for(j = 0; j < SIZE; j++) {
         vals = grid->mask[idx(j, i)];
         if(valCount(vals) == 2) {
            
            // second cell with just two possible values
            for(k = j + 1; k < SIZE; k++) {

               if(vals == grid->mask[idx(k, i)]) {
                  // twin found. Remove the pair of possible values from 
                  // all unset cells in row.
                  for(l = 0; l < SIZE; l++) {
                     if (l != j && l != k && grid->matrix[idx(l, i)] == 0 &&
                           (grid->mask[idx(l, i)] & vals)) {
                        grid->mask[idx(l, i)] &= ~vals;
                        changes = true;
                        if(0) {
                           printPossibleValues(grid, idx(l, i));
                        }
                        // leaving the elimination part for eliminate() as that will do 
                        // update peers as well.
                     }

                  }
               }   
            }
         }
      }
   }

   return changes;
}

int setTwinsBox(grid_t grid) {
   int changes = false;
   int j, k, l, bi, bj;
   int idx1, idx2, idx3;

   //box number
   for(bi = 0; bi < N; bi++) {
      for(bj = 0; bj < N; bj++) {
         // first cell with just two possible values
         for(j = 0; j < SIZE; j++) {
            idx1 = idx(bi + j / N, bj + j % N);
            if(valCount(grid->mask[idx1]) == 2) {
               // second cell with just two possible values
               for(k = j + 1; k < SIZE; k++) {
                  
                  idx2 = idx(bi + k / N, bj + k % N);
                  if(grid->mask[idx1] == grid->mask[idx2]) {
                     // twin found. Remove the pair of possible values from 
                     // all unset cells in box.
                     for(l = 0; l < SIZE; l++) {
                        if (l != j && l != k) {
                           idx3 = idx(bi + k / N, bj + k % N);
                           if(grid->matrix[idx3] == 0 && 
                                 (grid->mask[idx3] & grid->mask[idx1])) {
                              grid->mask[idx3] &= ~(grid->mask[idx1]);
                              changes = true;
                              
                              // leaving the elimination part for eliminate() as that will do 
                              // update peers as well.
                           }
                        }
                     }
                  }   
               }
            }
         }
      }
   }

   return changes;
}

void printPossibleValues(grid_t grid, int i) {
   printf("(%d, %d) has possible values : ", i/SIZE, i%SIZE);
   uint64_t vals = grid->mask[i];
   int v = 1;
   do {
      if(vals % 2)
         printf("%2d ", v);
      vals /= 2;
      v++;
   } while(vals > 0);
   printf("\n");
}

void updatePeers(grid_t grid, int i) {
   int v = grid->matrix[i];
   int x = i / M, y = i % M;
   int bx = (x / N) * N, by = (y / N) * N;
   int j, ox, oy;
   for (j = 0; j < SIZE; j++) {
      ox = j / N;
      oy = j % N;
      // check row
      if (y != j) 
         grid->mask[idx(x, j)] &= ~(1 << (v - 1));

      if (x != j) 
         grid->mask[idx(j, y)] &= ~(1 << (v - 1));

      // check box
      if (i != idx(bx + ox, by + oy)) 
         grid->mask[idx(bx + ox, by + oy)] &= ~(1 << (v - 1));
   }
}