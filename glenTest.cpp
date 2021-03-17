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

    if(parsedLine[3] == "+"){
        if(isdigit(parsedLine[2].at(0)) && isdigit(parsedLine[4].at(0))){//const + const
            int c3 = stoi(parsedLine[2]) + stoi(parsedLine[4]);//add both consts
            auto itr = f1.variables.find(parsedLine[0]);//get c3
            string insStr = "movl $" + to_string(c3) + ", " + to_string(itr->second.addr_offset) + "(%rbp)";
            f1.assembly_instructions.push_back(insStr);
            itr->second.value = c3;
            
        }else if(!isdigit(parsedLine[2].at(0)) && isdigit(parsedLine[4].at(0))){// var + const
            auto itr = f1.variables.find(parsedLine[0]);//c3 var
            auto itr2 = f1.variables.find(parsedLine[2]);//c1 var
            int c3 = itr2->second.value + stoi(parsedLine[4]);//new val
            if(parsedLine[0] == parsedLine[2]){
                string insStr = "addl $" + parsedLine[4] + "," + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                cout << insStr << endl;
            }else{
                string insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl $" + parsedLine[4] + ", %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl %eax "+ to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
            }
            itr->second.value = c3;//replace val
            
        }else if(isdigit(parsedLine[2].at(0)) && !isdigit(parsedLine[4].at(0))){//const + var
            auto itr = f1.variables.find(parsedLine[0]);//c3 var
            auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
            int c3 = stoi(parsedLine[2]) + itr2->second.value;

            if(parsedLine[0] == parsedLine[4]){
                string insStr = "addl $" + parsedLine[2] + "," + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
            }else{
                string insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl $" + parsedLine[2] + ", %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl %eax "+ to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
            }
            itr->second.value = c3;
        }else{//both var
            if (parsedLine[0] == parsedLine[2] && parsedLine[0] != parsedLine[4]){//x = x + var
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var 
                int c3 = itr1->second.value + itr2->second.value;
                string insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl %eax " + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val
            }else if(parsedLine[0] == parsedLine[4] && parsedLine[0] != parsedLine[2]){
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
                int c3 = itr1->second.value + itr2->second.value;
                string insStr = "movl " + to_string(itr1->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl %eax " + to_string(itr2->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val
            }else{//both same var 
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
                int c3 = itr1->second.value + itr2->second.value;
                string insStr = "movl " + to_string(itr1->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl %eax, %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl %eax " + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val
            }

        }

    }

}

int main(){
    string ln = "c = a + a;";

    Variable var("a", "int", 12, -4);
    Variable var2("b", "int", 120, -8);
    Variable var3("c", "int", 12, -12);
    Function f1;
    f1.function_name = "main";
    f1.variables.insert({var.name, var});
    f1.variables.insert({var2.name, var2});
    f1.variables.insert({var3.name, var3});
    arithmetic_handler(ln, f1);

    //
    return 0;
}