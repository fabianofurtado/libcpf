CC=gcc
CFLAGS=-Wall -O2 -s
BINS=lib1.so lib2.so

all: $(BINS)

%.so: %.c
	$(CC) $(CFLAGS) -fpic -shared $^ -o $@

%: %.c
	$(CC) $(CFLAGS) -ldl $^ -o $@

clean:
	rm $(BINS)
