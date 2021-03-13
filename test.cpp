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
    Handle variable declaration statements
*/
void variable_offset_allocation(vector<string> &source, int &loc, Function &f1, int &addr_offset) {
    // if brackets, array declaration
    // else primative
    if (source[loc].find("[") != string::npos && source[loc].find("]") != string::npos) {
        cout << "array" << endl;
    } else {
        string var_type = "int";

        string delimiter = ",";
        auto tokens = split(source[loc].substr(source[loc].find(" ")+1), delimiter);

        // movl value dest
        for (auto& item: tokens) {
            trim(item);

            delimiter = " ";
            auto var_tokens = split(item, delimiter);
            string var_name = var_tokens[0];
            int var_value = stoi(var_tokens[2]);

            Variable var (var_name, var_type, var_value, addr_offset);
            f1.variables.push_back(var);
            f1.assembly_instructions.push_back("movl $" + var_tokens[2] + ", " + to_string(addr_offset) + "(%rbp)");
            addr_offset -= 4;
            // for (auto& t: var_tokens) {

            // }
            // cout << item << endl;
            // cout << endl;
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

    for (auto x: f1.variables) {
        cout << x.type << " " << x.name << " " << x.value << " " << x.addr_offset << endl;
    }
}

int main() {
    string str1 = "int a = 1, b = 2, c = 3, d = 4;";
    string str2 = "int e[3] = {5, 6, 7};";

    vector<string> sources;
    sources.push_back(str1);
    sources.push_back(str2);
    int loc = 0;
    Function f1;
    intialize_function(f1);
    int addr_offset = -4;

    variable_offset_allocation(sources, loc, f1, addr_offset);
    view_function(f1);
}