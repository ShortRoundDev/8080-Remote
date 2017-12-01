CC=gcc
LIB=./lib
INC=./inc
BIN=./bin
SRC=./src

all: lib8080 libemulator emulator libemunet libhttp libscheduler
	$(CC) -O0 -g -o $(BIN)/emulator $(BIN)/lib8080.o $(BIN)/libemulator.o $(BIN)/emulator.o $(BIN)/libemunet.o $(BIN)/libhttp.o $(BIN)/libscheduler.o -lm -L$(LIB)/ -I$(INC)/ -lpthread -lcrypt

emulator:
	$(CC) -O0 -g -c $(SRC)/emulator.c -L$(LIB)/ -I$(INC)/ -o $(BIN)/emulator.o -lpthread
libemulator:
	$(CC) -O0 -g -c $(SRC)/libemulator.c -L$(LIB)/ -I$(INC)/ -o $(BIN)/libemulator.o
lib8080:
	$(CC) -O0 -g -c $(SRC)/lib8080.c -L$(LIB)/ -I$(INC)/ -o $(BIN)/lib8080.o
libemunet:
	$(CC) -O0 -g -c $(SRC)/libemunet.c -L$(LIB)/ -I$(INC)/ -o $(BIN)/libemunet.o -lcrypt
libhttp:
	$(CC) -O0 -g -c $(SRC)/libhttp.c -L$(LIB)/ -I$(INC)/ -o $(BIN)/libhttp.o
libscheduler:
	$(CC) -O0 -g -c $(SRC)/libscheduler.c -L$(LIB)/ -I$(INC)/ -o $(BIN)/libscheduler.o
