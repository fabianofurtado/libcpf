CC=gcc
CFLAGS=-Wall -O2 -z noseparate-code -Wl,--build-id=none -ldl -lcrypto -g
LIB_DIR=libcpf
EXECUTABLE=example
OBJECTS=\
example.o \
$(LIB_DIR)/cpf.o \
$(LIB_DIR)/sha1.o \
$(LIB_DIR)/fp_prototype.o \
$(LIB_DIR)/plugin_manager.o

all: $(EXECUTABLE)

# Now we'll compile!
# Explicit rule needed 'cause we have multiple objects.
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

# Implicit rules (all .c files will be compiled!)
#/%.o: /%.c
#$(LIB_DIR)/%.o: $(LIB_DIR)/%.c

clean:
	@echo "Deleting .o files and '$(EXECUTABLE)' binary..."
	-find . -type f -name '*.o' -delete
	-rm -f $(EXECUTABLE)
