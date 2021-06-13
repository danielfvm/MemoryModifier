CC = g++
CFLAGS = -lSDL -fPIC -c -g -Wall
LDFLAGS = -lSDL -shared -fPIC $(realpath ../../bin/libmemmod.so) -ldl 
TARGET = shared.so

SOURCE = shared.cpp # imgui/imgui_impl_sdl.cpp imgui/imgui_impl_opengl3.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
OBJ = ${SOURCE:.cpp=.o}

all: inject $(SOURCE) $(TARGET)

inject:
	$(CC) -fPIC -ldl $(realpath ../../bin/libmemmod.so) inject.cpp -o inject


$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf inject inject.o $(TARGET) $(OBJ)
