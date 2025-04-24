SUB_MAKEFILES:=\
	Makefile.bgservice\
	Makefile.client

all:
	for i in $(SUB_MAKEFILES); do \
		echo -e "\n $$i "; \
		make -f ./$$i; \
	done
clean: 
	for i in $(SUB_MAKEFILES); do \
		make -f ./$$i clean ; \
	done
