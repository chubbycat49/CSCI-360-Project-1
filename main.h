#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include <string>
#include <map>
#include "Function.h"
#include "Variable.h"
#include "util.h"

using namespace std;

map<Variable,int> variable_handler();
string add_mov_instruction(string src, string dest, int size);
bool is_function_call(string line);
bool is_arithmetic_line(const string s);
void move_immediate_val_into_register(const string s, const string reg, Function &f1);
void move_var_val_into_register(const string s, const string reg, Function &f1);
void move_arr_val_into_register(const string s, const string reg, Function &f1);
void store_immedaite_val(const string dest, const string val, Function &f1);
void store_reg_val(const string dest, const string reg, Function &f1);
void comparison_handler(string &s, Function &f1, int &loc);

void function_handler(vector<string> source, int loc, int max_len);
void common_instruction_handler_dispatcher(vector<string> source, int &loc, int max_len, Function &f1, int &addr_offset);
void variable_offset_allocation(vector<string> &source, int &loc, Function &f1, int &addr_offset);
void IF_statement_handler(vector<string> &source, int &loc, int max_len, Function &f1, int &addr_offset);
void FOR_statement_handler(vector<string> &source, int &loc, int max_len, Function &f1, int &addr_offset);
void return_handler(string source, Function &f1);
void function_call_handler(string source, Function &f1);
void arithmetic_handler(string &s, Function &f1);
void assignment_handler(string &s, Function &f1);

#endif
