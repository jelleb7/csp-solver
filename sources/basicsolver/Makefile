CC=gcc
CFLAGS=-Wall -O6 
OBJS=grammar.o datatypes.o backup.o solve.o constraint.o variable.o problem.o lex.yy.o main.o
LIBS=-lm

all: grammar.c lex.yy.c ${OBJS}
	${CC} ${CFLAGS} -o solver ${OBJS} ${LIBS}
grammar.c: grammar.g
	LLnextgen grammar.g
lex.yy.c: flex.fl
	flex flex.fl
clean:
	rm -f *~
	rm -f *.o

