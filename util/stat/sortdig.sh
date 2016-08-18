#!/bin/sh
bbshome=/var/bbs
for x in `ls $bbshome/boards`; do 
	$bbshome/bin/sortdig $x
done
