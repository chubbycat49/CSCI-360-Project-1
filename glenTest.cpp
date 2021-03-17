#include <iostream>
#include <sstream>
#include <vector>
#include "Variable.h"
#include "Function.h"
using namespace std;
                                //Function &f1
void arithmetic_handler(string line, Function &f1) {

    string lineToParse = line;
    vector<string> parsedLine;//[0] = a, [2] = b, [3] = + , [4] = c {a = b + c}

    lineToParse.erase(lineToParse.length() - 1, lineToParse.length());// remove ";"

    string word;
    istringstream iss(lineToParse);
    while(iss >> word){// split the string into a vector
        parsedLine.push_back(word.c_str());
    }

    //if(isdigit(parsedLine[2].at(0))){
    //auto itr = f1.variables.find(parsedLine[2]);
    //cout << itr->second.addr_offset << itr->second.value << endl;
    //itr->second.value = 99;
    //cout << itr->second.value << endl;

    if(isdigit(parsedLine[2].at(0)) && isdigit(parsedLine[4].at(0))){
        int c3 = stoi(parsedLine[2]) + stoi(parsedLine[4]);//add both consts
        auto itr = f1.variables.find(parsedLine[0]);
        string insStr = "movl $" + to_string(c3) + ", " + to_string(itr->second.addr_offset) + "(%rbp)";
        f1.assembly_instructions.push_back(insStr);
    } 

}

int main(){
    string ln = "a = 3 + 33;";

    Variable var("a", "int", 12, -4);
    Variable var2("b", "int", 120, -8);
    Variable var3("a", "int", 12, -12);
    Function f1;
    f1.function_name = "main";
    f1.variables.insert({var.name, var});
    f1.variables.insert({var2.name, var2});
    f1.variables.insert({var3.name, var3});
    arithmetic_handler(ln, f1);

    //
    return 0;
}