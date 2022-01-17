#
# 
# 
#

TARGETS = malloc_1 malloc_2 malloc_3 malloc_4
OBJS = malloc_1.o malloc_2.o malloc_3.o malloc_4.o
CC = g++
CFLAGS = -g -Wall


all: malloc_1 malloc_2 malloc_3 malloc_4

malloc_1: malloc_1.cpp
	$(CC) $(CFLAGS) -c  malloc_1.cpp

malloc_2: malloc_2.cpp
	$(CC) $(CFLAGS) -c  malloc_2.cpp

malloc_3: malloc_3.cpp
	$(CC) $(CFLAGS) -c  malloc_3.cpp

malloc_4: malloc_4.cpp
	$(CC) $(CFLAGS) -c  malloc_4.cpp

clean:
	-rm -f $(OBJS)