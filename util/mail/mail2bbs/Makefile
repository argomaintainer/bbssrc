CC       = gcc
CFLAGS   = -g -Wunused -m32 -D__NO_PROTOTYPE__ -I../../../include
BBSHOME	 = /home/bbs

.c.o:   ;   $(CC) $(CFLAGS) -c $*.c

all: bbsmail

install: bbsmail
	cp bbsmail $(BBSHOME)/bin 
	chmod +s $(BBSHOME)/bin/bbsmail

clean:
	rm -fr *.o *.BAK *.log *core* DEAD* *~ bbsmail

distclean:
	rm $(BBSHOME)/bin/bbsmail

bbsmail: bbsmail.c mailcheck.c
	$(CC) $(CFLAGS) -o bbsmail bbsmail.c mailcheck.c -L../../../lib -lBBS -lSystem
