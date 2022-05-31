CC=gcc
CFLAGS=-Wall -O0 -g -z noseparate-code -Wl,--build-id=none -L/tmp/libcpf/libs -Wl,-rpath=/tmp/libcpf/libs -lcpf -ldl -lcrypto
EXECUTABLE=example
OBJECTS=example.o

all: $(EXECUTABLE)

# Now we'll compile!
# Explicit rule needed 'cause we have multiple objects.
$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $@ $(CFLAGS)

# Implicit rules (all .c files will be compiled!)
#/%.o: /%.c
#$(LIB_DIR)/%.o: $(LIB_DIR)/%.c

clean:
	@echo "Deleting .o files, .so files and '$(EXECUTABLE)' binary..."
	-find . -type f -name '*.o' -delete
	-find . -type f -name '*.so' -delete
	-rm -f $(EXECUTABLE)
