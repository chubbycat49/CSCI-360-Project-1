#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

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

/*
    Handle variable declaration statements
*/
void variable_offset_allocation(vector<string> &source, int &loc, Function &f1, int &addr_offset) {
    // if brackets, array declaration
    // else primative

    if (source[loc].find("[") != string::npos && source[loc].find("]") != string::npos) {
        int a = 1;
    } else {
        int b = 2;
    }
}

void intialize_function(Function &f1) {
    f1.return_type = "int";
    f1.function_name = "test";
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

    cout << f1.return_type << endl;
}