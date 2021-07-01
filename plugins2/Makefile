CC=gcc
CFLAGS=-Wall -O2 -s
BINS=lib4.so lib5.so

all: $(BINS)

%.so: %.c
	$(CC) $(CFLAGS) -fpic -shared $^ -o $@

%: %.c
	$(CC) $(CFLAGS) -ldl $^ -o $@

clean:
	rm $(BINS)
