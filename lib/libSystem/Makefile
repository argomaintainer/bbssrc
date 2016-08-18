#############################################################
###        You needn't modify the following stuff      ######
#############################################################
LIBNAME	 = libSystem.a

GARBAGE  = a.out core out.* *.bak DEAD*

CFILE	 = string.c fileio.c crypt.c net.c system.c snprintf.c setproctitle.c \
	   stringlist.c cygwin.c sphpsingal.c

OBJS	 = string.o fileio.o crypt.o net.o system.o snprintf.o setproctitle.o \
	   stringlist.o cygwin.o sphpsingal.o

.SUFFIXES: .o .c

.c.o:   ;   $(CC) -s $(CFLAGS) $(OSDEF) -I../../include -c $*.c

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
