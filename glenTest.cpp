#include <iostream>
#include <sstream>
#include <vector>
#include "Variable.h"
#include "Function.h"
using namespace std;

void printVec(vector<string> vec){
    for(int i = 0; i < vec.size(); i++){
        cout <<vec[i] << endl;
    }
}
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


    if(parsedLine[3] == "+"){
        if(isdigit(parsedLine[2].at(0)) && isdigit(parsedLine[4].at(0))){//const + const
            int c3 = stoi(parsedLine[2]) + stoi(parsedLine[4]);//add both consts
            auto itr = f1.variables.find(parsedLine[0]);//get c3
            string insStr = "movl $" + to_string(c3) + ", " + to_string(itr->second.addr_offset) + "(%rbp)";
            f1.assembly_instructions.push_back(insStr);
            itr->second.value = c3;
 cout << "1" << endl;           
        }else if(!isdigit(parsedLine[2].at(0)) && isdigit(parsedLine[4].at(0))){// var + const
            auto itr = f1.variables.find(parsedLine[0]);//c3 var
            auto itr2 = f1.variables.find(parsedLine[2]);//c1 var
            int c3 = itr2->second.value + stoi(parsedLine[4]);//new val
            if(parsedLine[0] == parsedLine[2]){
                string insStr = "addl $" + parsedLine[4] + "," + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                cout << insStr << endl;
cout << "2" << endl;   
            }else{
                string insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl $" + parsedLine[4] + ", %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl %eax "+ to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
cout << "3" << endl;   
            }
            itr->second.value = c3;//replace val
            
        }else if(isdigit(parsedLine[2].at(0)) && !isdigit(parsedLine[4].at(0))){//const + var
            auto itr = f1.variables.find(parsedLine[0]);//c3 var
            auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
            int c3 = stoi(parsedLine[2]) + itr2->second.value;
cout << "4" << endl;
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
cout << "5" << endl;   
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var 
                int c3 = itr1->second.value + itr2->second.value;
                string insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl %eax " + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val
            }else if(parsedLine[0] == parsedLine[4] && parsedLine[0] != parsedLine[2]){//x = var + x
    cout << "6" << endl;   
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
                int c3 = itr1->second.value + itr2->second.value;
                string insStr = "movl " + to_string(itr1->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl %eax " + to_string(itr2->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val
            }else if(parsedLine[2] == parsedLine[4]){//both same var 
     cout << "7" << endl;          
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
            }else{
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var 
                int c3 = itr1->second.value + itr2->second.value;
                string insStr = "movl " + to_string(itr1->second.addr_offset) + "(%rbp), %edx";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "addl %edx, %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl %eax " + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val
      
            }

        }

    }else if(parsedLine[3] == "-"){
         if(isdigit(parsedLine[2].at(0)) && isdigit(parsedLine[4].at(0))){//const - const
            int c3 = stoi(parsedLine[2]) - stoi(parsedLine[4]);//sub both consts
            auto itr = f1.variables.find(parsedLine[0]);//get c3
            string insStr = "movl $" + to_string(c3) + ", " + to_string(itr->second.addr_offset) + "(%rbp)";
            f1.assembly_instructions.push_back(insStr);
            itr->second.value = c3;
        }else if(!isdigit(parsedLine[2].at(0)) && isdigit(parsedLine[4].at(0))){// var - const
            auto itr = f1.variables.find(parsedLine[0]);//c3 var
            auto itr2 = f1.variables.find(parsedLine[2]);//c1 var
            int c3 = itr2->second.value - stoi(parsedLine[4]);//new val
            
            if(parsedLine[0] == parsedLine[2]){
                string insStr = "subl $" + parsedLine[4] + "," + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                
            }else{
                string insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "subl $" + parsedLine[4] + ", %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl %eax "+ to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                
            }
            itr->second.value = c3;//replace val
            
        }else if(isdigit(parsedLine[2].at(0)) && !isdigit(parsedLine[4].at(0))){//const - var
            auto itr = f1.variables.find(parsedLine[0]);//c3 var
            auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
            int c3 = stoi(parsedLine[2]) - itr2->second.value;

            if(parsedLine[0] == parsedLine[4]){
                string insStr = "subl $" + parsedLine[2] + "," + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
            }else{
                string insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "subl $" + parsedLine[2] + ", %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl %eax "+ to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
            }
            itr->second.value = c3;
        }else{//both var
            if (parsedLine[0] == parsedLine[2] && parsedLine[0] != parsedLine[4]){//x = x - var
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var 
                int c3 = itr1->second.value - itr2->second.value;
                string insStr = "movl " + to_string(itr2->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "subl %eax " + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val

            }else if(parsedLine[0] == parsedLine[4] && parsedLine[0] != parsedLine[2]){//c3=c2
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
                int c3 = itr1->second.value - itr2->second.value;

                string insStr = "movl " + to_string(itr1->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "subl %eax " + to_string(itr2->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val
            }else if(parsedLine[2] == parsedLine[4]){//both same var 
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
                int c3 = itr1->second.value - itr2->second.value;
                string insStr = "movl $" + to_string(0) + " " + to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                itr->second.value = c3;//replace val
               
            }else{//both different vars
                auto itr = f1.variables.find(parsedLine[0]);//c3 var
                auto itr1 = f1.variables.find(parsedLine[2]);//c1 var
                auto itr2 = f1.variables.find(parsedLine[4]);//c2 var
                int c3 = itr1->second.value - itr2->second.value; 
                string insStr = "movl " + to_string(itr1->second.addr_offset) + "(%rbp), %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "subl " + to_string(itr2->second.addr_offset) + "(%rbp)" + ", %eax";
                f1.assembly_instructions.push_back(insStr);
                insStr = "movl %eax "+ to_string(itr->second.addr_offset) + "(%rbp)";
                f1.assembly_instructions.push_back(insStr);
                printVec(f1.assembly_instructions);
            }

        }   

    }else if(parsedLine[3] == "*"){
        cout << "mult" << endl;
    }

}

int main(){
    string ln = "c = a * b;";

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