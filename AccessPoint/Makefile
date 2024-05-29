
CC = g++
CFLAGS  = -g -Wall -std=c++11
INCLUDES = -I src/Includes/ -I src/InfluxDB

default: all

%.o: src/%.cpp
	mkdir -p bin
	$(CC) $(CFLAGS) $(INCLUDES) -o bin/$@ -c $^

ic.o: src/InfluxDB/ic.c
	$(CC) $(CFLAGS) $(INCLUDES) -o bin/ic.o -c $^

app: main.o ESPNOW_manager.o ESPNOW_types.o ic.o
	$(CC) $(CFLAGS) -o bin/exec $(addprefix bin/,$^) -pthread
	cp conf bin/

all: clear clean app

clean: 
	$(RM) bin/*

clear:
	clear
