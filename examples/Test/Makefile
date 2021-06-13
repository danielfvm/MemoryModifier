CC = g++
CFLAGS = -fPIC -c -g -Wall
LDFLAGS = -shared -fPIC $(realpath ../../bin/libmemmod.so) -ldl 
TARGET = shared.so

SOURCE = shared.cpp
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
