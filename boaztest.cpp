#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include "Function.h"

using namespace std;

vector<Function> functions;
string register_for_argument_32[6] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
string register_for_argument_64[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

/*
    Helper function which given a string and a deliminator, creates a iterator of the split tokens
    Sourced from: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
*/
vector<string> split(const string str, const string regex_str) {
    return {sregex_token_iterator(str.begin(), str.end(), regex(regex_str), -1), sregex_token_iterator()};
}

string add_mov_instruction(string src, string dest, int size) {
  string out;
  switch(size){
    case 8:
      out = "movb ";
      break;
    case 16:
      out = "movs ";
      break;
    case 32:
      out = "movl ";
      break;
    case 64:
      out = "movq ";
      break;
  }
  out += (src + ", ");
  return out + dest;
}

/*
    Handle other function call statements
*/
void function_call_handler(string teststr, Function &f1) {
    std::string input_str = "i = test(a, b, c, d, e, f, g, h);";
    bool assigned = false;
    string regex_str = " ";
    auto tokens = split(input_str, regex_str);
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
    std::string name = callstr.substr(0, callstr.find("("));
    std::string params = callstr.substr(callstr.find("(") + 1);
    params.pop_back(); params.pop_back(); // Get rid of ");"

    // Place first 6 parameters onto stack
    // Concern: pushing an array requires 'lea' instead of 'mov'
    tokens = split(params, ",");
    i = 0;
    for (auto p : tokens){
      for (Variable a : f1.variables){
        if (p == a.name){
          i++;
          if (i <= 6){
            if (a.type == "int")
              f1.assembly_instructions.push_back(add_mov_instruction
                (to_string(a.addr_offset) + "(%rbp)", "%" + register_for_argument_32[i-1], 32));
            else
              f1.assembly_instructions.push_back(add_mov_instruction
                (to_string(a.addr_offset) + "(%rbp)", "%" + register_for_argument_64[i-1], 64));
          }
          else{
            // After the first 6 arguments are placed in registers, the rest are put on the stack
            f1.assembly_instructions.push_back(add_mov_instruction(
              to_string(a.addr_offset) + "(%rbp)", "%rdi", 64));
            f1.assembly_instructions.push_back("pushq %rdi");
          }
        }
      }
    }

    // Call function
    f1.assembly_instructions.push_back("call " + name);

    for (auto s : f1.assembly_instructions)
      cout << s << endl;
}

/*
    Handle return statementsa
*/
void return_handler(string *source, int &loc, Function &f1) {
}

int main(){
  string teststr = "test(a, b, c, d, e, f, g, h);";
  Function f1;
  f1.return_type = "int";
  f1.function_name = "main";
  f1.assembly_instructions.push_back(f1.function_name + ":");
  f1.assembly_instructions.push_back("pushq %rbp");
  f1.assembly_instructions.push_back("movq %rsp, %rbp");
  f1.is_leaf_function = true;
  function_call_handler(teststr, f1);
}
