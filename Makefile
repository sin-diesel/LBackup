
CC = gcc
IDIR = ./include
CCFLAGS = -g -Wall -std=c11 -I $(IDIR)
SRC_DIR = source
SOURCES = main.c copy.c
HEADERS = copy.h
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLES = $(SOURCES:.c=)

.PHONY: all
all: $(OBJECTS)
	$(CC) $(CCFLAGS) $(OBJECTS) -o rescp

copy.o: $(SRC_DIR)/copy.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/copy.c -c -o copy.o

main.o: $(SRC_DIR)/main.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/main.c -c -o main.o	

.PHONY: clean
clean:
	rm log.txt
	rm *.o 