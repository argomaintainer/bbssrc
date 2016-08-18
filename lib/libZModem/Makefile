#############################################################
###        You needn't modify the following stuff      ######
#############################################################
LIBNAME	 = libZModem.a

GARBAGE  = a.out core out.* *.bak DEAD*

DEFS	 = -DNFGVMIN

CFILE	 = canit.c  crctab.c  lrz.c  lsz.c  rbsb.c  zm.c  zmio.c zreadline.c

OBJS	 = canit.o  crctab.o  lrz.o  lsz.o  rbsb.o  zm.o  zmio.o zreadline.o

.SUFFIXES: .o .c

.c.o:   ;   $(CC) -s $(CFLAGS) $(OSDEF) $(DEFS) -Wunused -I../../include -c $*.c

#--------------------- Dependency starts here -------------------

all: $(OBJS)
	ar rv ../$(LIBNAME) $(OBJS)
	ranlib ../$(LIBNAME)

tag:
	ctags $(CFILE)

clean: /tmp
	@echo --- Cleaning Garbage ... ---
	-rm -fr $(GARBAGE) *.o *~

cleanall: clean
	@echo --- Cleanning Grabage with $(LIBNAME) ---
	-rm -fr $(LIBNAME) ../$(LIBNAME)

install:
	@echo --- lib: skip ---
