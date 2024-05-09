#!/bin/bash
CC = gcc
SRC = sagun_lab04.c
EXE = program

# Target: all (default target)
all: run

# Target: compile the program
$(EXE): $(SRC)
	gcc -o $(EXE) $(SRC)

# Target: run the program
run: $(EXE)
	./$(EXE)

clean:
	rm -f $(EXE)
