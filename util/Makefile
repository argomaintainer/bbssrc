DIRS = game mail stat util

clean:
	@echo "== cleanning junks, object files, and binaries .. =="
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
