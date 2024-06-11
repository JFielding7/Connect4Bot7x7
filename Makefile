COMPILER = clang++
FLAGS = -O3

compile:
	$(COMPILER) $(FLAGS) main.cpp engine.cpp database_generator.cpp -o c4

run: compile
	./c4
