CC       = gcc
CFLAGS   =  -Wunused -m32 -D__NO_PROTOTYPE__ -I../../../include
BBSHOME	 = /home/bbs

.c.o:   ;   $(CC) $(CFLAGS) -c $*.c

all: gsend

install: gsend
	cp gsend $(BBSHOME)/bin 

clean:
	rm -fr *.o *.BAK *.log *core* DEAD* *~ gsend

distclean:
	rm $(BBSHOME)/bin/gsend

gsend: gsend.c
	$(CC) $(CFLAGS) -o gsend gsend.c -L../../../lib -lBBS -lSystem
