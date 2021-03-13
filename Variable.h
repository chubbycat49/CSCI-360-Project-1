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

    Variable(string n, string t, int v, int a);
};

Variable::Variable(string n, string t, int v, int a) {
    name = n;
    type = t;
    value = v;
    addr_offset = a;
}

#endif