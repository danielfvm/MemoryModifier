CC = g++
CFLAGS = -fPIC -c -g -Wall
LDFLAGS = -shared -fPIC -ldl 
TARGET = shared.so

SOURCE = shared.cpp
OBJ = ${SOURCE:.cpp=.o}

all: inject $(SOURCE) $(TARGET)

inject:
	$(CC) -fPIC -ldl ../../bin/libmemmod.so inject.cpp -o inject

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf inject inject.o $(TARGET) $(OBJ)
