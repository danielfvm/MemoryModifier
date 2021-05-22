CC = g++
CFLAGS = -fPIC -c -g -Wall
LDFLAGS = -shared -fPIC -ldl
TARGET = libmemmod.so

SOURCE = src/Process.cpp src/MemoryRegion.cpp src/ptrace.cpp
OBJ = $(subst src, obj, $(SOURCE:.cpp=.o))

all: folders $(SOURCE) $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o bin/$@

obj/%.o: src/%.cpp
	$(CC) $(CFLAGS) -o $@ $<

folders:
	mkdir -p bin obj

install: all
	sudo cp src/MemoryModifier.h /usr/include/MemoryModifier.h
	sudo cp bin/$(TARGET) /usr/lib/$(TARGET)

uninstall:
	sudo rm /usr/include/MemoryModifier.h
	sudo rm /usr/lib/$(TARGET)

example: all
	cd examples/ImGuiSDL && make

test: clean all
	$(CC) -fPIC -ldl ./bin/libmemmod.so src/main.cpp -o ./bin/inject

clean:
	rm -rf bin obj
