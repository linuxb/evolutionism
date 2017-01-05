CC = g++
SRCS = sync.cpp
OBJS = out/aync.o

.PHONY: expe all clean

all: expe	

$(OBJS): $(SRCS)
	@$(CC) -c $< -o $@

expe: $(OBJS)
	@$(CC) $< /usr/local/lib/libboost_regex.a -o $@

clean:
	@rm out/*.o
	@rm ./expe