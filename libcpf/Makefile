CC=gcc # C compiler
CFLAGS=-Wall -O0 -g -fpic -z noseparate-code -Wl,--build-id=none -ldl -lcrypto # C flags
LDFLAGS=-shared # linking flags
TARGET=libcpf.so
OBJECTS=\
cpf.o \
blake2.o \
fp_prototype.o \
plugin_manager.o

all: $(TARGET)

# Now we'll compile!
# Explicit rule needed 'cause we have multiple objects.
#$(TARGET): $(OBJECTS)
#    $(CC) $(FLAGS) $(CFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) ${LDFLAGS} $^ -o ../libs/$@

# Implicit rules (all .c files will be compiled!)
#/%.o: /%.c
#$(LIB_DIR)/%.o: $(LIB_DIR)/%.c

clean:
	@echo "Deleting .o files and '$(TARGET)' binary..."
	-find . -type f -name '*.o' -delete
	-rm -f ../libs/$(TARGET)
