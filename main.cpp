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
string register_for_argument_32[6] = {edi, esi, edx, ecx, r8d, r9d};

map variable_handler() {
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

void function_handler(vector<string> source, int loc, int max_len) {
    // Create a function object, get function return type and function name

    Function f1;
    string head = source[loc];
    f1.return_type = head.substr(0, head.find(' '));  // get return type
    string tempstr = head.substr(head.find(' ') + 1, head.length());
    f1.function_name = tempstr.substr(0, tempstr.find('('));  // get fxn name
    f1.assembly_instructions.push_back(f1.function_name + ":");
    f1.assembly_instructions.push_back("pushq %rbp");
    f1.assembly_instructions.push_back("movq %rsp, %rbp");
    f1.is_leaf_function = true;

    // Get parameter list and read parameter values from registers

    int addr_offset = -4;
    tempstr = tempstr.substr(tempstr.find('(') + 1, tempstr.length() - f1.function_name.length() - 3);
    string parameter_str = tempstr.substr(tempstr.find('(') + 1, tempstr.find('('));
    if (parameter_str.length() > 0) {
        f1.variables = variable_handler(parameter_str, 2, addr_offset);
        int number_of_parameter = 0;
        for (auto &var : f1.variables) {
            number_of_parameter++;
            // First 6 parameters <- registers
            if (number_of_parameter <= 6) {
                if (var.type == "int") {
                    // this parameter is a 32 bits variable
                    f1.assembly_instructions.push_back(add_mov_instruction("%" + register_for_argument_32[number_of_parameter - 1], to_string(var.addr_offset) + "(%rbp)", 32));
                } else {
                    // this parameter is a 64 bits variable
                    f1.assembly_instructions.push_back(add_mov_instruction("%" + register_for_argument_64[number_of_parameter - 1], to_string(var.addr_offset) + "(%rbp)", 64));
                }
                // other than the first 6, the rest need to reset their offset
                // 16 = return address + saved %rbp
            } else {
                var.addr_offset = 16 + (number_of_parameter - 6 - 1) * 8;
            }
        }
    }

    // Go through each instruction

    loc++;  // go to next source code line
    bool next_function = false;
    while (loc < max_len) {
        if ((source[loc].find("int") == 0 || source[loc].find("void") == 0) && source[loc].find("}") == source[loc].length() - 1) {
            // start with a new function
            next_function = true;
            break;
        } else if (source[loc] == "}") {
            loc++;
        } else {
            // line is not function call or function end
            // send to common handler dispatcher
            common_instruction_handler_dispatcher(source, loc, max_len, f1, addr_offset);
        }
    }

    if (f1.is_leaf_function == false && f1.variables.size() > 0) {
        int last_offset = 0 - f1.variables.back().addr_offset;
        // if last offset is not divisible by 16, then do 16 bytes address alignment: multiples of 16
        if (last_offset % 16 != 0) {
            last_offset = ceil((float)last_offset / 16) * 16;
        }
        f1.assembly_instructions.insert(f1.assembly_instructions.begin() + 3, "subq $" + to_string(last_offset) + ",%rsp");
    }

    functions.push_back(f1);
    if (next_function == true) {
        function_handler(source, loc, max_len);
    }
}

/*
    Return if given code line is a function call
*/
bool is_function_call(string line) {
}

/*
    According to the instruction type, call the corresponding handler.
*/
void common_instruction_handler_dispatcher(string *source, int &loc, int max_len, Function &f1, int &addr_offset) {
    /*
        code line starts with variable declaration keyword "int" and ends with semicolon
    */
    if (source[loc].find("int") == 0 && source[loc].find(";") == source[loc].length() - 1) {
        variable_offset_allocation(source, loc, f1, addr_offset);
        loc++;
    }
    /*
        code line starts with "if"
    */
    else if (source[loc].find("if") == 0) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        IF_statement_handler(source, loc, max_len, f1, addr_offset);
    }
    /*
        code line starts with "for"
    */
    else if (source[loc].find("for") == 0) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        FOR_statement_handler(source, loc, max_len, f1, addr_offset);
    }
    /*
        code line starts with "return"
    */
    else if (source[loc].find("return") == 0) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        return_handler(source, loc, f1);
        loc++;
    }
    /*
        code line starts with a function
    */
    else if (is_function_call(source[loc])) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        function_call_handler(source, loc, f1);
        f1.is_leaf_function = false;
        loc++;
    }
    /*
        otherwise, code line is an arithmetic instruction
    */
    else {
        f1.assembly_instructions.push_back("#" + source[loc]);
        arithmetic_handler(source[loc], f1);
        loc++;
    }
}

/*
    Helper function which given a string and a deliminator, creates a iterator of the split tokens
    Sourced from: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
*/
vector<string> split(const string str, const string regex_str) {
    return {sregex_token_iterator(str.begin(), str.end(), regex(regex_str), -1), sregex_token_iterator()};
}

/*
    Handle variable declaration statements
*/
void variable_offset_allocation(string *source, int &loc, Function &f1, int &addr_offset) {
}

/*
    Handle if statements
*/
void IF_statement_handler(string *source, int &loc, int max_len, Function &f1, int &addr_offset) {
}

/*
    Handle for statements
*/
void FOR_statement_handler(string *source, int &loc, int max_len, Function &f1, int &addr_offset) {
}

/*
    Handle return statements
*/
void return_handler(string *source, int &loc, Function &f1) {
}

/*
    Handle other function call statements
*/
void function_call_handler(string *source, int &loc, Function &f1) {
}

/*
    Handle arithemetic statements
*/
void arithmetic_handler(string line, Function &f1) {
}

vector<string> loadFile(string filename, int &maxlen) {
    ifstream inputFile;
    inputFile.open(filename);
    string inputLine;

    if (inputFile.fail()) {
        cout << "Failed to open file." << endl;
    } else {
        cout << "Opening file successful" << endl;
    }

    vector<string> sourceCode;
    while (!inputFile.eof()) {
        getline(inputFile, inputLine);
        sourceCode.push_back(inputLine);
        maxlen++;
    }

    inputFile.close();

    return sourceCode;
}

int main() {
    int max_len = 0;
    vector source = loadFile("test1.cpp", max_len);

    function_handler(source, 0, max_len);

    return 0;
}
