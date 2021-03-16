main: Function.h Variable.h main.cpp
	g++ -std=c++11 -o main main.cpp

test: Function.h Variable.h boaztest.cpp
	g++ -std=c++11 -o test boaztest.cpp

kuntest: Function.h Variable.h test.cpp
	g++ -std=c++11 -o kuntest kuntest.cpp

clean:
	rm -f main test
