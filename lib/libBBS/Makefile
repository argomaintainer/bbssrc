
#############################################################
###        You needn't modify the following stuff      ######
#############################################################
LIBNAME	 = libBBS.a

GARBAGE  = a.out core out.* *.bak DEAD*

CFILE	 = post.c ../../src/record.c datetime.c

OBJS	 = post.o record.o datetime.o

.SUFFIXES: .o .c

.c.o:   ;   $(CC) -s $(CFLAGS) $(OSDEF) -c -I../../include $*.c

#--------------------- Dependency starts here -------------------

all: $(OBJS)
	ar rv ../$(LIBNAME) $(OBJS)
	ranlib ../$(LIBNAME)

record.o:
	$(CC) -s $(CFLAGS) $(OSDEF) -c -I../../include ../../src/record.c -o ./record.o

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
