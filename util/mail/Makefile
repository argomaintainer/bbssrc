DIRS = bbspop3d mail2bbs gsend

clean:
	-rm -fr *~ *.o *.bak *.tmp
	@for x in $(DIRS); \
	do \
		echo "--> $$x"; \
		(cd $$x; make clean ; cd .. ); \
		echo "<-- $$x"; \
		echo " "; \
	done
	echo "done."
        
cleanall: clean
