#################################
#				#
# Makefile for Weather Report   #
#				#
#################################

CC = gcc
ALL = wreport

all: $(ALL)

wreport: wreport.c ../../include/bbs.h
	$(CC) -m32 -o wreport wreport.c -I../../include

clean: 
	-rm -fr *~ *.o *.bak 
	-rm -fr $(ALL)

install: all
	cp $(ALL) /home/bbs/bbs_home/bin

