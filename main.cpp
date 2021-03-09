#include <fstream> 
#include <iostream>
using namespace std;


int main(){

    string inputLine;

    ifstream inputFile;
    inputFile.open("test1.cpp");

    if(inputFile.fail()){
        cout << "Failed to open file." << endl;
    }else{
        cout << "Opening file successful"<< endl;
    }

    getline(inputFile, inputLine);

    cout << inputLine << endl;

    while(!inputFile.eof()){
        getline(inputFile, inputLine);

        cout << inputLine << endl;
    }

    inputFile.close();
    return 0;

}