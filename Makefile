CC = g++
CFLAGS = -Wall

TARGETS = wish

all: $(TARGETS)

wish: wish.c path.c path.h parser.h parse.c lexer.h lexer.c input_processing.h getting_input.h execute.c execute.h
	gcc $(CFLAGS) wish.c execute.c lexer.c parse.c path.c -o wish

clean:
	rm -f $(TARGETS)
