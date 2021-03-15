#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include "Function.h"
#include "Variable.h"

using namespace std;

vector<Function> functions;

/*
    Helper function which given a string and a deliminator, creates a iterator of the split tokens
    Sourced from: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
*/
vector<string> split(const string str, const string regex_str) {
    return {sregex_token_iterator(str.begin(), str.end(), regex(regex_str), -1), sregex_token_iterator()};
}

/*
    Handle other function call statements
*/
void function_call_handler(string teststr, Function &f1) {
    std::string input_str = "i = test(a, b, c, d, e, f, g, h);";
    bool assigned = false;
    auto tokens = split(input_str, " ");

    // Check if the line with the function call assigns the returned value
    if (tokens[1] == "="){
      assigned = true;
    }

    // Get string with just function call
    std::string callstr = "";
    int i = 0;
    for (auto c : tokens){
        if (i < 2 && assigned){
            i++;
            continue;
        }
        callstr += c;
    }

    // Get name of function and parameter list
    tokens = split(callstr, "(");
    std::string name = tokens[0];
    std::string params = tokens[1];
    params.pop_back();params.pop_back(); // Get rid of ");"

    // Place first 6 parameters onto stack, 
    tokens = split(params, ",");
    for (auto p : tokens){
      for (Variable a : f1.variables){
        if (p == a.name){
          f1.assembly_instructions.push_back("")
        }
      }
    }
}

/*
    Handle return statements
*/
void return_handler(string *source, int &loc, Function &f1) {
}

int main(){
  teststr = "test(a, b, c, d, e, f, g, h);";
  Function f1;
  f1.return_type = "int"
  f1.function_name = "main"
  f1.assembly_instructions.push_back(f1.function_name + ":");
  f1.assembly_instructions.push_back("pushq %rbp");
  f1.assembly_instructions.push_back("movq %rsp, %rbp");
  f1.is_leaf_function = true;
  function_call_handler(teststr, )
}
