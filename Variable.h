#ifndef VARIABLE_H
#define VARIABLE_H

#include <string>

using namespace std;

class Variable {
   public:
    string type;
    int addr_offset;

    Variable(string t, int a);
};

Variable::Variable(string t, int a) {
    type = t;
    addr_offset = a;
}

#endif