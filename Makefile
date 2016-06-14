.PHONY: all clean re

PROGS=$(patsubst %.c,%,$(wildcard *.c))
all: $(PROGS)

%: %.c $(wildcard *.h)
	gcc -Wall $< -o $@ -lpthread

clean:
	rm -f $(PROGS)

re: clean all
