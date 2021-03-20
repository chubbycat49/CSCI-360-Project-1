#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>

using namespace std;

class Variable {
   public:
    string name;
    string type;
    int value;
    int addr_offset;
    bool is_param;

    Variable(string n, string t, int v, int a, bool is_p = false);
};

Variable::Variable(string n, string t, int v, int a, bool is_p) {
    name = n;
    type = t;
    value = v;
    addr_offset = a;
    is_param = is_p;
}

#endif
