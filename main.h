#ifndef MAIN_H
#define MAIN_H

#include <vector>
#include <string>
#include <map>

map<Variable,int> variable_handler();
string add_mov_instruction(string src, string dest, int size);
bool is_function_call(string line);
void get_arr_val_into_register(const string s, const string reg, Function &f1);
void comparison_handler(string &s, Function &f1, int &loc);

void function_handler(vector<string> source, int loc, int max_len);
void common_instruction_handler_dispatcher(vector<string> source, int &loc, int max_len, Function &f1, int &addr_offset);
void variable_offset_allocation(vector<string> &source, int &loc, Function &f1, int &addr_offset);
void IF_statement_handler(string source, int max_len, Function &f1, int &addr_offset);
void FOR_statement_handler(string source, int max_len, Function &f1, int &addr_offset);
void return_handler(string source, Function &f1);
void function_call_handler(string source, Function &f1);
void arithmetic_handler(string line, Function &f1);

#endif