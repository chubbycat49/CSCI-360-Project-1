#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>
#include <algorithm> 
#include <cctype>
#include <locale>

#include "Function.h"
#include "Variable.h"

using namespace std;

/*
    Helper function which given a string and a deliminator, creates a iterator of the split tokens
    Sourced from: https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c
*/
vector<string> split(const string str, const string regex_str) {
    return {sregex_token_iterator(str.begin(), str.end(), regex(regex_str), -1), sregex_token_iterator()};
}

// trim from both ends (in place)
static inline void trim(string &s) {
    // trim from start
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));

    // trim from end
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !isspace(ch);
    }).base(), s.end());

    // remove trailing semicolon in last element
    if (s.find(";") == s.length() - 1) {
        s.pop_back();
    }
}

/*
    Handle variable declaration statements.
    If "[" and "]" in line, then it's an array declaration,
    otherwise it is a primative variable declaration.
*/
void variable_offset_allocation(vector<string> &source, int &loc, Function &f1, int &addr_offset) {
    if (source[loc].find("[") != string::npos && source[loc].find("]") != string::npos) {
        /*
            Split line by = and then by the [] to parse the name, size and values
        */
        string var_type = "int";
        string delimiter = " = ";
        auto tokens = split(source[loc].substr(source[loc].find(" ")+1), delimiter); // only work with part after "int "
        trim(tokens[0]);
        trim(tokens[1]);

        auto array_name = tokens[0];
        auto temp = split(array_name, "\\["); // [ needs to be escaped with \ and then that \ needs to be escaped for C++ complier
        array_name = temp[0];
        
        int array_size = stoi(temp[1].substr(0, temp[1].size()-1));
        
        auto array_values_str = tokens[1];
        auto array_values = split(array_values_str.substr(1, array_values_str.size()-2), ", "); // removes { and } and splits into int values

        for (int i = array_size-1; i >= 0; --i) {
            int val = stoi(array_values[i]);
            string name = array_name + "[" + to_string(i) + "]";
            int arr_addr_offset = addr_offset - (i * 4);

            Variable var (name, var_type, val, addr_offset);
            f1.variables.insert(pair<string, Variable> (name, var));
            f1.assembly_instructions.push_back("movl $" + array_values[i] + ", " + to_string(arr_addr_offset) + "(%rbp)");
        }
        addr_offset -= (array_size * 4);
    } else {
        
        /*
            Split variables by , and then by space to parse the name and value
        */
        string var_type = "int";
        string delimiter = ",";
        auto tokens = split(source[loc].substr(source[loc].find(" ")+1), delimiter); // only work with part after "int "

        for (auto& item: tokens) {
            trim(item);

            delimiter = " ";
            auto var_tokens = split(item, delimiter);
            string var_name = var_tokens[0];
            int var_value = stoi(var_tokens[2]);

            Variable var (var_name, var_type, var_value, addr_offset);
            f1.variables.insert(pair<string, Variable> (var_name, var));
            f1.assembly_instructions.push_back("movl $" + var_tokens[2] + ", " + to_string(addr_offset) + "(%rbp)");
            addr_offset -= 4;
        }
    }
}

void intialize_function(Function &f1) {
    f1.return_type = "int";
    f1.function_name = "test";
}

void view_function(Function &f1) {
    for (auto x: f1.assembly_instructions) {
        cout << x << endl;
    }

    for (auto const& x: f1.variables) {
        cout << x.second.type << " " << x.first << " " << x.second.value << " " << x.second.addr_offset << endl;
    }
}

int main() {
    string str1 = "int a = 1, b = 2, c = 3, d = 4;";
    string str2 = "int e[3] = {5, 6, 7};";
    string str3 = "int f = 8, g = 9;";

    vector<string> sources;
    sources.push_back(str1);
    sources.push_back(str2);
    sources.push_back(str3);
    int loc = 0;
    Function f1;
    intialize_function(f1);
    int addr_offset = -4;

    for (int i = 0; i < sources.size(); ++i) {
        variable_offset_allocation(sources, i, f1, addr_offset);
    }
    view_function(f1);
}