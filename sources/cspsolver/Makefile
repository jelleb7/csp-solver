CC=gcc
CFLAGS=-Wall -O6 
OBJS=vardb.o grammar.o rewrite.o types.o list.o lex.yy.o
LIBS=-lm

all: grammar.c lex.yy.c ${OBJS}
	${CC} ${CFLAGS} -o csp ${OBJS} ${LIBS}
grammar.c: grammar.g
	LLnextgen grammar.g
lex.yy.c: flex.fl
	flex flex.fl
clean:
	rm -f *~
	rm -f *.o

