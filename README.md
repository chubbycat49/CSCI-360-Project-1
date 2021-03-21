# CSCI-360-Project-1

Group Members  
Kun Yu  
Boaz Kaufman  
Mariglen Jahaj  

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
./main <source file name> <output file name>
```
The program will output a .txt file containing the assembly output for the given .txt file.

To run included testcase files, run 
```
./main test1.txt out.txt
```
or
```
./main test2.txt out.txt
```

### Design Description

#### Variable.h
This class represents a variable. It contains the information of a variable such as the type, name, offset and value.

#### Function.h
This class represents a function. It contains the information of a function such as the return type and name. It holds the variables of a function in a `map<string, variable>`. As well as a `bool` to indicate if the function is a leaf function.

#### util.h
This class contains helper functions. It contains functionality to parse source code lines for translation. Functions to indicate whether an instruction accesses array elements. And functions to read and write .txt files.

#### main.h
This class contains the core functions for the program. 

**main.h class interface provides the following functionality:**

`void common_instruction_handler_dispatcher(vector<string> source, int &loc, int max_len, Function &f1, int &addr_offset)`
* Function to figure out what kind of instruction the current source code line is and call the appropriate function for the translation.

`void function_handler(vector<string> source, int loc, int max_len)`
* Function to create Function object and makes the stack for the function. It retrieves function name, return type and the parameters for the function.

`void function_call_handler(string input_str, Function &f1)`
* Function to translate instructions that call a function with passed parameters.

`void variable_offset_allocation(vector<string> &source, int &loc, Function &f1, int &addr_offset)`
* Function to translate variable declaration instructions.

`void assignment_handler(string &s, Function &f1)`
* Function to translate assignment instructions for variable and array values.

`void IF_statement_handler(vector<string> &source, int &loc, int max_len, Function &f1, int &addr_offset)`
* Function to translate `if()` instructions.

`void FOR_statement_handler(vector<string> &source, int &loc, int max_len, Function &f1, int &addr_offset)`
* Function to translate `for()` instructions.

`void return_handler(string source, Function &f1)`
* Function to translate `return` statements in a function.

`void arithmetic_handler(string &s, Function &f1, bool store_result = true)`
* Function to translate arithmetic instructions for addition subtraction and multiplication.

Source Code Style Requirements  
Indentation: Tabs  
Curly braces: Inline  
Spaces between variable name/equals/value  
Scalar/Dynamic variables initialized on different lines  
Comments: Own lines  
Else: same line as closing if curly brace  
