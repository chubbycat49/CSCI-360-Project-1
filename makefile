main: Function.h Variable.h util.h main.h
	g++ -std=c++11 util.cpp main.cpp -o main

test: Function.h Variable.h boaztest.cpp
	g++ -std=c++11 -o test boaztest.cpp

kuntest: Function.h Variable.h util.h kuntest.cpp
	g++ -std=c++11 util.cpp kuntest.cpp -o kuntest

clean:
	rm -f main test kuntest out.txt
