# makefile

all: shell

main.o : main.cpp
	g++ -c -g main.cpp -std=c++11

shell: main.o
	g++ -o shell main.o -std=c++11

clean:
	rm *.o
	rm *.txt

	