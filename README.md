# CSCI-360-Project-1

## Background

A C/C++ compiler which shows the assembly output of a given source code. Based on the 
godbolt.org compiler, this program is a simplified version handling some basic instruction translations. This implementation only handles integer values. Instruction translations include variable declaration and initialization, array decleration and initialization, function declerations, `for()` loop instruction, `if()` instructions, `return` statements and arithmetic instructions. The arithmetic instructions include addition, subtraction and multiplication in the form of `destination = operand1 + operand2`.

### Compiling and Executing
Compile program with:
```
make
```
The compiled exe can take one argument for the source.txt file to be translated to assembly.
```
./main <source file name>
```
The program will output a out.txt file containing the assembly output for the given .txt file.

### Design Description

#### Variable.h
This class represents a variable. It contains the information of a variable such as the type, name, offset and value.

#### Function.h
This class represents a function. It contains the information of a function such as the return type and name. It holds the variables of a function in a `map<string, variable>`. Aswell as a `bool` to know if the function is a leaf function or not.

#### util.h
This class contains helper functions. It contains functionality to parse source code lines for translation. Functions to indicate whether an instruction accesses array elements. And functions to read and write .txt files.

#### main.h
This class contains the functions for the core of the program. 

**main.h class interface provides the following functionality:**

`void common_instruction_handler_dispatcher(vector<string> source, int &loc, int max_len, Function &f1, int &addr_offset)`
* Function to figure out what kind of instruction the current source code line is and call the appropriate handler for translation.

`void function_handler(vector<string> source, int loc, int max_len)`
* Function to create Function object and makes the stack for the function. It retrieves function name, return type and the parameters for the function.

`void function_call_handler(string input_str, Function &f1)`


