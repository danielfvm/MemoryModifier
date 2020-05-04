build:
	mkdir -p bin obj
	gcc -shared -o obj/MemoryModifier.so -fPIC src/MemoryModifier.c -lncurses
	gcc -c src/main.c -o obj/main.o -lncurses
	gcc obj/main.o obj/MemoryModifier.so -o bin/load -lm -lncurses
	gcc -c src/interface.c -o obj/interface.o -lncurses
	gcc obj/interface.o obj/MemoryModifier.so -o bin/mm -lm -lncurses

install:
	sudo cp bin/mm /usr/bin/mm
	sudo cp src/MemoryModifier.h /usr/include/MemoryModifier.h
	sudo cp obj/MemoryModifier.so /usr/lib/libmm.so
