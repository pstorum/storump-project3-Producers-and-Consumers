pcseml: pcseml.c
	gcc -Wall -Wextra -o pcseml pcseml.c eventbuf.c -lpthread

pcseml.zip:
	rm -f $@
	zip $@ Makefile pcseml.c
