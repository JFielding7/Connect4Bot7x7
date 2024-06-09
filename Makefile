CC = clang++
CFLAGS = -O3

compile:
	$(CC) $(CFLAGS) main.cpp engine.cpp database_generator.cpp -o c4

run: compile
	./c4
