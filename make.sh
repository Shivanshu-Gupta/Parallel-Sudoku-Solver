#!/bin/sh

minigridsize=3

if [ $1 ]; then
	minigridsize=$1
fi

g++ -DMINIGRIDSIZE=$minigridsize -fopenmp -O3 -Wall -g main.c sudoku.c -o sudoku
