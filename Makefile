all:
	gcc -I/usr/include/libmilter -I./lua/include -L./lua/lib -g -Wall hex.c main.c -lmilter -lpthread -llua -lm -lffi -ldl
