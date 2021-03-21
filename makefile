main: Function.h Variable.h util.h main.h
	g++ -std=c++11 util.cpp main.cpp -o main

clean:
	rm -f main out.txt