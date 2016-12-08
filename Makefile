CC = g++

.PHONY: expe all

all: expe	

out/main.o: main.cpp
	@$(CC) -c main.cpp -o $@

expe: out/main.o
	@$(CC) $< -o $@
