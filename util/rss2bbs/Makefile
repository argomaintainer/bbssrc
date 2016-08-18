CC		= gcc
CFLAGS		= -Wall -g -m32 -I../../include

OBJS		= rss2bbs.o misc.o bbspost.o history.o

TARGET		= rss2bbs

all : $(TARGET)

rss2bbs : $(OBJS)
	$(CC) -o rss2bbs $(OBJS) -L../../lib -lBBS -lSystem -lexpat


clean :
	rm -f $(TARGET) *.o
