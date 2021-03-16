#ifndef FUNCTION_H
#define FUNCTION_H

#include <map>
#include <string>
#include <vector>

#include "Variable.h"

using namespace std;

class Function {
   public:
    string return_type;
    string function_name;
    vector<string> sourceCode;
    vector<Variable> variables;
    vector<string> assembly_instructions;

    bool is_leaf_function;
};

#endif
