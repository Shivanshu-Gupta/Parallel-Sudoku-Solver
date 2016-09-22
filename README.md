# Fast Parallel Sudoku Solver
This is a fast parallel Sudoku Solver implemented using OpenMP. As solving Sudoku is a known NP-complete problem, I have used variety of heuristics to make it fast. A detailed discussion of the heuristics I have used may be found in these places:

*   [Parallelized Sudoku Solving Algorithm using OpenMP](http://www.cse.buffalo.edu/faculty/miller/Courses/CSE633/Sankar-Spring-2014-CSE633.pdf)
*   [Parallel Depth-First Sudoku Solver Algorithm](https://alitarhini.wordpress.com/2011/04/06/parallel-depth-first-sudoku-solver-algorithm/)

The `DesignDoc.pdf` describes the algrithm I have used, the data/task distribution among the threads and the various optimisations I did to speed up the Solver.

### Compilation
	
The code can be compiled as follows. The grid size of the sudoku which the program will be used to solved is set at compile time. `minigridsize` is the size of the smaller squares in the sudoku grid. So that for a 16x16 board, `minigridsize=3`.
	
```sh
$ ./make.sh <minigridsize>
```
Default `minigridsize` is 3.

### Running

The program can be run by passing the number of threads, `thread_count`, the location of the text file containing the sudoku grid.

``` sh
$ ./sudoku <thread_count> <grid_file_path>
```

### Tests
	
A number of test grids of various sizes, and difficulty levels are present in the `TestCases` directory. To solve a 16x16 Sudoku grid using 2 threads

``` sh
$ ./make.sh 4
$ ./sudoku 2 TestCases/16x16_board/sudoku16_input1
```

