CC      = gcc
CFLAGS  = -O2 -Wall -g

all: flexls flexget

flexls: flexls.o flexfsu.o

flexget: flexget.o flexfsu.o

flexfsu.o flexls.o flexget.o: flexfsu.h
