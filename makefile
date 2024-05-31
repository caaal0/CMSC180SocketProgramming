#!/bin/bash
CC = gcc
SRC = exer5.c
EXE = exer5

# Target: all (default target)
all: run

# Target: compile the program
$(EXE): $(SRC)
	gcc -o $(EXE) $(SRC) -lpthread -lm

# Target: run the program
run: $(EXE)
	./$(EXE)

clean:
	rm -f $(EXE)
