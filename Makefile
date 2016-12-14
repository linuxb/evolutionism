CC = g++
SRCS = acct.cpp
OBJS = out/acct.o

.PHONY: expe all clean

all: expe	

$(OBJS): $(SRCS)
	@$(CC) -c $< -o $@

expe: $(OBJS)
	@$(CC) $< -o $@

clean:
	@rm out/*.o
	@rm ./expe