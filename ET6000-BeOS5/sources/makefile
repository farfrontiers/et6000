SUBDIRS := driver accelerant

default .DEFAULT:
	@curDir=`pwd`; \
	for dir in $(SUBDIRS); do \
		if [ -d "$$dir" ]; then \
			cd $$dir && $(MAKE) $@; \
			cd $$curDir; \
		fi \
	done
