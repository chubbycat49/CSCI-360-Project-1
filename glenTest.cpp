#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <string>
#include "Variable.h"
#include "Function.h"
using namespace std;

int label_num = 2;

vector<string> split(const string str, const string regex_str) {
   regex regexz(regex_str);
   vector<string> list(sregex_token_iterator(str.begin(), str.end(), regexz, -1), sregex_token_iterator());
   return list;
}

void printVec(vector<string> vec){
    for(int i = 0; i < vec.size(); i++){
        cout <<vec[i] << endl;
    }
}
                                //Function &f1


/*
    Handle for statements
*/
void FOR_statement_handler(string source, int max_len, Function &f1, int &addr_offset) {
    string loop_label = ".L" + to_string(label_number++);
    string end_label = ".L" + to_string(label_number++);
    
    // for (int i =0; i < 5; i++)
    // int i = 0;
    // push loop label
    // i < 5
    // jump end_label
    // i = i + 1

    // loc++;  // go to next source code line
    // bool next_function = false;
    // while (loc < max_len) {
    //     if ((source[loc].find("int") == 0 || source[loc].find("void") == 0) && source[loc].find("}") == source[loc].length() - 1) {
    //         // start with a new function
    //         next_function = true;
    //         break;
    //     } else if (source[loc] == "}") {
    //         loc++;
    //     } else {
    //         // line is not function call or function end
    //         // send to common handler dispatcher
    //         common_instruction_handler_dispatcher(source, loc, max_len, f1, addr_offset);
    //     }
    // }

    // finished the for loop, already hit the }
    // unconditional jump back to loop_label  jmp     loop_label
    // push end_label


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