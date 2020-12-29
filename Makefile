
CC = gcc
IDIR = ./include
CCFLAGS = -g -Wall -std=c11 -I $(IDIR)
SRC_DIR = source
SOURCES = main.c lbp.c
HEADERS = lbp.h
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLES = $(SOURCES:.c=)

.PHONY: all
all: $(OBJECTS)
	$(CC) $(CCFLAGS) $(OBJECTS) -o lbp

lbp.o: $(SRC_DIR)/lbp.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/lbp.c -c -o lbp.o

main.o: $(SRC_DIR)/main.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/main.c -c -o main.o	

.PHONY: clean
clean:
	rm *.o
	rm lbp