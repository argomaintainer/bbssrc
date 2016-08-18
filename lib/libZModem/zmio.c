#include <time.h>
#include <unistd.h>

#define ZMODEM_RATE 5000

int
raw_write(int fd, const char *buf, int len)
{
	static int lastcounter = 0;
	int nowcounter, i;
	static int bufcounter;
	int retlen = 0;

	nowcounter = time(0);
	if (lastcounter == nowcounter) {
		if (bufcounter >= ZMODEM_RATE) {
			sleep(1);
			nowcounter = time(0);
			bufcounter = len;
		} else
			bufcounter += len;
	} else {
		/*
		 * time clocked, clear bufcounter
		 */
		bufcounter = len;
	}
	lastcounter = nowcounter;

	for (i = 0; i < len; i++) {
		int mylen;

		if ((unsigned char) buf[i] == 0xff)
			mylen = write(fd, "\xff\xff", 2);
		else if (buf[i] == 13)
			mylen = write(fd, "\x0d\x00", 2);
		else
			mylen = write(fd, &buf[i], 1);
		if (mylen < 0)
			break;
		retlen += mylen;
	}
	return retlen;
}

void
raw_ochar(char c)
{
	raw_write(0, &c, 1);
}

int
raw_read(int fd, char *buf, int len)
{
	int i, j, retlen;
    
	retlen = read(fd,buf,len);
	for(i = 0; i < retlen; i++) {
		if(i > 0 && ((unsigned char)buf[i - 1] == 0xff) && ((unsigned char)buf[i] == 0xff)) {
			retlen--;  
			for(j = i; j < retlen; j++)
				buf[j] = buf[j + 1];
			continue;
		}
        
		if(i > 0 && buf[i - 1]==0x0d && buf[i] == 0x00) {
			retlen--;
			for(j = i;j < retlen; j++)
				buf[j] = buf[j + 1];
			continue;
		}
	}
    
	return retlen;
}
