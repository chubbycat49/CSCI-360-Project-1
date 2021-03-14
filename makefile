main: Function.h Variable.h main.cpp
	g++ -std=c++11 -o main main.cpp

test: Function.h Variable.h boaztest.cpp
	g++ -std=c++11 -o test boaztest.cpp

clean:
	rm -f main test
