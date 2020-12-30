
CC = gcc
IDIR = ./include
CCFLAGS = -g -Wall -std=c11 -I $(IDIR)
SRC_DIR = source
SOURCES = main.c lbp.c lbp-ui.c
HEADERS = lbp.h
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLES = $(SOURCES:.c=)

.PHONY: all
all: $(OBJECTS)
	$(CC) $(CCFLAGS) lbp.o main.o -o lbp
	$(CC) $(CCFLAGS) lbp.o lbp-ui.o -o lbp-ui

lbp.o: $(SRC_DIR)/lbp.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/lbp.c -c -o lbp.o

main.o: $(SRC_DIR)/main.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/main.c -c -o main.o	

lbp-ui.o: $(SRC_DIR)/lbp-ui.c
	$(CC) $(CCFLAGS) $(SRC_DIR)/lbp-ui.c -c -o lbp-ui.o	

.PHONY: clean
clean:
	rm *.o
	rm lbp
	rm lbp-ui