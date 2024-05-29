#!/bin/bash
CC = gcc
SRC = test.c
EXE = program

# Target: all (default target)
all: run

# Target: compile the program
$(EXE): $(SRC)
	gcc -o $(EXE) $(SRC) -lpthread

# Target: run the program
run: $(EXE)
	./$(EXE)

clean:
	rm -f $(EXE)
