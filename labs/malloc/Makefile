#
# Students' Makefile for the Malloc Lab
#
TEAM = bovik
VERSION = 1
HANDINDIR = /afs/cs.cmu.edu/academic/class/15213-f01/malloclab/handin
# MMC = ./myanswer/mm4.c
# MMC = mm.c
MMC = ./ich/mm_others_v2_1.c

CC = gcc
CFLAGS = -Wall -m32 -g -pg

OBJS = mdriver.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

all: clean mm mdriver

mdriver: $(OBJS)
	# $(CC) $(CFLAGS) -c -o mm.o $(MMC) 
	$(CC) $(CFLAGS) -o mdriver $(OBJS) mm.o
	rm -rf *~ *.o
	
mm: $(MMC)
	$(CC) $(CFLAGS) -c -o mm.o $(MMC)

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: $(MMC) mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h

handin:
	cp mm.c $(HANDINDIR)/$(TEAM)-$(VERSION)-mm.c

clean:
	@echo "MMC file: $(MMC)"
	rm -f *~ *.o mdriver


