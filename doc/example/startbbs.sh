#
#    启动 bbs 全套服务的 Script，在 /etc/rc.local 加入 startbbs.sh，
#    或是把这个档案丢到 /usr/local/etc/rc.d 中亦可，这样开机便可自动
#    执行。
#
#    P.S.使用前记得 chmod +x startbbs.sh
#
#!/bin/sh
#if [ -x /home/bbs/bin/in.zbbsd ]; then
#	/home/bbs/bin/in.zbbsd && echo -n ' in.zbbsd'
#fi
if [ -x /home/bbs/bin/bbsd ]; then
	/home/bbs/bin/bbsd 23&& echo -n ' bbsd'
fi
if [ -x /home/bbs/bin/bbspop3d ]; then
	/home/bbs/bin/bbspop3d && echo -n ' bbspop3d'
fi
if [ -x /home/bbs/bin/gopherd ]; then
	/home/bbs/bin/gopherd && echo -n ' gopherd'
fi
if [ -x /home/bbs/bin/fingerd ]; then
	/home/bbs/bin/fingerd && echo -n ' fingerd'
fi
if [ -x /home/bbs/innd/innbbsd ]; then
	su bbs -c '/home/bbs/innd/innbbsd' && echo -n ' innbbsd'
fi
