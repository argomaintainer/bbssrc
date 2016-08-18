#!/bin/sh
cd /home/bbs/bbs_home/bin
/usr/local/bin/wget -Yon -O /home/bbs/bbs_home/tmp/weather1.html http://www.tqyb.com.cn/public/fcst/inland.htm 1>/dev/null 2>/dev/null
/usr/local/bin/wget -Yon -O /home/bbs/bbs_home/tmp/weather2.html http://www.tqyb.com.cn/public/fcst/gdfc.txt 1>/dev/null 2>/dev/null
./wreport
