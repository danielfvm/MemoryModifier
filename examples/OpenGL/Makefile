TARGET = inject
CC = g++
LIBS = -lm
CFLAGS = -g -Wall

build:
	g++ -c inject.cpp -shared -fPIC -ldl 
	g++ -c shader.cpp -shared -fPIC -ldl 
	g++ -c Renderer.cpp -shared -fPIC -ldl 
	g++ -shared -fPIC -ldl -o $(TARGET).so $(CFLAGS) inject.o shader.o Renderer.o $(LIBS)

clean:
	rm -rf *.so *.o
