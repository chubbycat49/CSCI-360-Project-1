#include "main.h"
#include "Function.h"
#include "Variable.h"
#include "util.h"

using namespace std;

vector<Function> functions;
int label_number = 2;

string register_for_argument_32[6] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
string register_for_argument_64[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

map<Variable,int> variable_handler() {
}

string add_mov_instruction(string src, string dest, int size) {
  string out;
  switch(size){
    case 8:
      out = "movb ";
      break;
    case 16:
      out = "movs ";
      break;
    case 32:
      out = "movl ";
      break;
    case 64:
      out = "movq ";
      break;
  }
  out += (src + ", ");
  return out + dest;
}

/*
    Return if given code line is a function call
*/
bool is_function_call(string line) {
}

/*
    Helper function to handle code like a[0] and a[f] which accesses array elements
    Pushes required assembly instructions to get that value into specified register
*/
void move_arr_val_into_register(const string s, const string reg, Function &f1)
{
    string arr_name = s.substr(0, s.find("["));
    string arr_index = substr_between_indices(s, s.find("[") + 1, s.find("]"));

    if (is_int(arr_index))
    {
        /*
            a[immediate]
            meaning this should be in f1.variables
        */
        f1.assembly_instructions.push_back("movl " + to_string(f1.variables.at(s).addr_offset) + "(%rbp), " + reg);
    }
    else
    {
        /*
            a[var]
        */
        // move arr_index into eax
        f1.assembly_instructions.push_back("movl " + to_string(f1.variables.at(arr_index).addr_offset) + "(%rbp), %eax");
        f1.assembly_instructions.push_back("cltq");
        // get where arr_name[0] is
        string arr_start_offset = to_string(f1.variables.at(arr_name + "[0]").addr_offset);
        f1.assembly_instructions.push_back("movl " + arr_start_offset + "(%rbp, %rax, 4), " + reg);
    }
}

/*
    Helper function to handle code like thing1 comparator thing2
    Pushes required assembly instructions for comparison
*/
void comparison_handler(string &s, Function &f1, int &loc)
{
    // mapping of all comparators and their associated assembly command
    map<string, string> comparators;
    comparators.insert(pair<string, string>("<", "jge"));
    comparators.insert(pair<string, string>(">", "jle"));
    comparators.insert(pair<string, string>("<=", "jg"));
    comparators.insert(pair<string, string>(">=", "jl"));
    comparators.insert(pair<string, string>("==", "jne"));
    comparators.insert(pair<string, string>("!=", "je"));

    auto tokens = split(s, " ");
    trim_vector(tokens);
    string l_comp = tokens[0];
    string r_comp = tokens[2];
    string comp = tokens[1];

    for (auto const &c : comparators)
    {
        string comparator = c.first;
        string command = c.second;

        if (is_substr(s, comparator))
        {
            string label_instruction = ".L" + to_string(label_num);

            if (is_array_accessor(l_comp) || is_array_accessor(r_comp))
            {
                /*
                    Handles where one of the comparands is accessing an array
                    - array array
                    - array var
                    - array immediate
                    - var array
                    - immediate array
                */
                if (is_array_accessor(l_comp) && is_array_accessor(r_comp))
                {
                    // array array
                    move_arr_val_into_register(l_comp, "%edx", f1);
                    move_arr_val_into_register(r_comp, "%eax", f1);

                    f1.assembly_instructions.push_back("cmpl %eax, %edx");
                }
                else if (is_array_accessor(l_comp))
                {
                    move_arr_val_into_register(l_comp, "%eax", f1);
                    if (is_int(r_comp))
                    {
                        // array immediate
                        r_comp = "$" + r_comp;
                    }
                    else
                    {
                        // array var
                        r_comp = to_string(f1.variables.at(r_comp).addr_offset) + "(%rbp)";
                    }
                    /*
                        reverse the comparands here to ensure that the map holds
                        godbolt shows c[f] < a as
                            cmpl %eax, -4(%rbp)
                            jle .L3
                        but we want it as:
                            cmpl -4(%rbp), %eax
                            jge .L3
                    */
                    f1.assembly_instructions.push_back("cmpl " + r_comp + ", %eax");
                }
                else if (is_array_accessor(r_comp))
                {
                    move_arr_val_into_register(r_comp, "%eax", f1);
                    if (is_int(l_comp))
                    {
                        // immediate array
                        l_comp = "$" + l_comp;
                    }
                    else
                    {
                        // var array
                        l_comp = to_string(f1.variables.at(l_comp).addr_offset) + "(%rbp)";
                    }

                    f1.assembly_instructions.push_back("cmpl %eax, " + l_comp);
                }
            }
            else if (is_int(l_comp) || is_int(r_comp))
            {
                /*
                    Handles where one of the comparands is an immediate and none are arrays
                    - immediate immediate
                    - immediate var
                    - var immedaite
                */
                if (is_int(l_comp) && is_int(r_comp))
                {
                    /*
                        Doesn't handle case of when both are immediates
                    */
                }
                else if (is_int(l_comp))
                {
                    r_comp = to_string(f1.variables.at(r_comp).addr_offset) + "(%rbp)";

                    f1.assembly_instructions.push_back("cmpl $" + l_comp + ", " + r_comp);
                }
                else
                {
                    l_comp = to_string(f1.variables.at(l_comp).addr_offset) + "(%rbp)";

                    f1.assembly_instructions.push_back("cmpl $" + r_comp + ", " + l_comp);
                }
            }
            else
            {
                /*
                    Else case is if both comparands are variables(non-array)
                    - var var
                */
                l_comp = to_string(f1.variables.at(l_comp).addr_offset) + "(%rbp)";
                r_comp = to_string(f1.variables.at(r_comp).addr_offset) + "(%rbp)";

                f1.assembly_instructions.push_back("movl " + l_comp + ", %eax");
                f1.assembly_instructions.push_back("cmpl " + r_comp + ", %eax");
            }

            f1.assembly_instructions.push_back(command + " " + label_instruction);
        }
    }
}

void function_handler(vector<string> source, int loc, int max_len) {
    // Create a function object, get function return type and function name

    Function f1;
    string head = source[loc];
    f1.return_type = head.substr(0, head.find(' '));  // get return type
    string tempstr = head.substr(head.find(' ') + 1, head.length());
    f1.function_name = tempstr.substr(0, tempstr.find('('));  // get fxn name
    f1.assembly_instructions.push_back(f1.function_name + ":");
    f1.assembly_instructions.push_back("pushq %rbp");
    f1.assembly_instructions.push_back("movq %rsp, %rbp");
    f1.is_leaf_function = true;

    // Get parameter list and read parameter values from registers

    int addr_offset = -4;
    tempstr = tempstr.substr(tempstr.find('(') + 1, tempstr.length() - f1.function_name.length() - 3);
    string parameter_str = tempstr.substr(tempstr.find('(') + 1, tempstr.find('('));
    if (parameter_str.length() > 0) {
        f1.variables = variable_handler(parameter_str, 2, addr_offset);
        int number_of_parameter = 0;
        for (auto &var : f1.variables) {
            number_of_parameter++;
            // First 6 parameters <- registers
            if (number_of_parameter <= 6) {
                if (var.type == "int") {
                    // this parameter is a 32 bits variable
                    f1.assembly_instructions.push_back(add_mov_instruction("%" + register_for_argument_32[number_of_parameter - 1], to_string(var.addr_offset) + "(%rbp)", 32));
                } else {
                    // this parameter is a 64 bits variable
                    f1.assembly_instructions.push_back(add_mov_instruction("%" + register_for_argument_64[number_of_parameter - 1], to_string(var.addr_offset) + "(%rbp)", 64));
                }
                // other than the first 6, the rest need to reset their offset
                // 16 = return address + saved %rbp
            } else {
                var.addr_offset = 16 + (number_of_parameter - 6 - 1) * 8;
            }
        }
    }

    // Go through each instruction

    loc++;  // go to next source code line
    bool next_function = false;
    while (loc < max_len) {
        if ((source[loc].find("int") == 0 || source[loc].find("void") == 0) && source[loc].find("}") == source[loc].length() - 1) {
            // start with a new function
            next_function = true;
            break;
        } else if (source[loc] == "}") {
            loc++;
        } else {
            // line is not function call or function end
            // send to common handler dispatcher
            common_instruction_handler_dispatcher(source, loc, max_len, f1, addr_offset);
        }
    }

    if (f1.is_leaf_function == false && f1.variables.size() > 0) {
        int last_offset = 0 - f1.variables.back().addr_offset;
        // if last offset is not divisible by 16, then do 16 bytes address alignment: multiples of 16
        if (last_offset % 16 != 0) {
            last_offset = ceil((float)last_offset / 16) * 16;
        }
        f1.assembly_instructions.insert(f1.assembly_instructions.begin() + 3, "subq $" + to_string(last_offset) + ",%rsp");
    }

    functions.push_back(f1);
    if (next_function == true) {
        function_handler(source, loc, max_len);
    }
}

/*
    According to the instruction type, call the corresponding handler.
*/
void common_instruction_handler_dispatcher(vector<string> source, int &loc, int max_len, Function &f1, int &addr_offset) {
    /*
        code line starts with variable declaration keyword "int" and ends with semicolon
    */
    if (source[loc].find("int") == 0 && source[loc].find(";") == source[loc].length() - 1) {
        variable_offset_allocation(source, loc, f1, addr_offset);
        loc++;
    }
    /*
        code line starts with "if"
    */
    else if (source[loc].find("if") == 0) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        IF_statement_handler(source, loc, max_len, f1, addr_offset);
    }
    /*
        code line starts with "for"
    */
    else if (source[loc].find("for") == 0) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        FOR_statement_handler(source, loc, max_len, f1, addr_offset);
    }
    /*
        code line starts with "return"
    */
    else if (source[loc].find("return") == 0) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        return_handler(source, loc, f1);
        loc++;
    }
    /*
        code line starts with a function
    */
    else if (is_function_call(source[loc])) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        function_call_handler(source, loc, f1);
        f1.is_leaf_function = false;
        loc++;
    }
    /*
        code line has +, -, * and is an arithmetic instruction
    */
    else if (is_arithmetic_line(source[loc]){
        f1.assembly_instructions.push_back("#" + source[loc]);
        arithmetic_handler(source[loc], f1);
        loc++;
    }
    /*
        otherwise, code line is an assignment instruction
    */
    else {
        // a = b
        // i = test(a, b, c, d, e, f, g, h);
        // assignment_handler();
        // loc++;
    }
}

/*
    Handle variable declaration statements.
    If "[" and "]" in line, then it's an array declaration,
    otherwise it is a primative variable declaration.
*/
void variable_offset_allocation(vector<string> &source, int &loc, Function &f1, int &addr_offset)
{
    if (is_array_accessor(source[loc]))
    {
        /*
            Split line by = and then by the [] to parse the name, size and values
        */
        string var_type = "int";
        auto tokens = split(source[loc].substr(source[loc].find(" ") + 1), " = "); // only work with part after "int "
        trim_vector(tokens);
        remove_ending_semicolon_vector(tokens);

        auto array_name = tokens[0];
        auto temp = split(array_name, "\\["); // [ needs to be escaped with \ and then that \ needs to be escaped for C++ complier
        array_name = temp[0];

        int array_size = stoi(temp[1].substr(0, temp[1].size() - 1));

        auto array_values_str = tokens[1];
        auto array_values = split(array_values_str.substr(1, array_values_str.size() - 2), ", "); // removes { and } and splits into int values

        for (int i = array_size - 1; i >= 0; --i)
        {
            int val = stoi(array_values[i]);
            string name = array_name + "[" + to_string(i) + "]";
            int arr_addr_offset = addr_offset - (i * 4);

            Variable var(name, var_type, val, arr_addr_offset);
            f1.variables.insert(pair<string, Variable>(name, var));
            f1.assembly_instructions.push_back("movl $" + array_values[i] + ", " + to_string(arr_addr_offset) + "(%rbp)");
        }
        addr_offset -= (array_size * 4);
    }
    else
    {

        /*
            Split variables by , and then by space to parse the name and value
        */
        string var_type = "int";
        auto tokens = split(source[loc].substr(source[loc].find(" ") + 1), ","); // only work with part after "int "
        trim_vector(tokens);
        remove_ending_semicolon_vector(tokens);

        for (auto &item : tokens)
        {
            auto var_tokens = split(item, " ");
            string var_name = var_tokens[0];
            int var_value = stoi(var_tokens[2]);

            Variable var(var_name, var_type, var_value, addr_offset);
            f1.variables.insert(pair<string, Variable>(var_name, var));
            f1.assembly_instructions.push_back("movl $" + var_tokens[2] + ", " + to_string(addr_offset) + "(%rbp)");
            addr_offset -= 4;
        }
    }
}

/*
    Handle if statements
*/
void IF_statement_handler(vector<string> &source, int &loc, int max_len, Function &f1, int &addr_offset)
{
    string comparison = substr_between_indices(source[loc], source[loc].find("(") + 1, source[loc].find(")"));
    comparison_handler(comparison, f1, loc);
    loc++;

    while (source[loc] != "}")
    {
        common_instruction_handler_dispatcher(source, loc, max_len, f1, addr_offset);
    }

    f1.assembly_instructions.push_back("# }");
    f1.assembly_instructions.push_back(".L" + to_string(label_num) + ":");
    label_num++;
    loc++;
}

/*
    Handle for statements
*/
void FOR_statement_handler(string source, int max_len, Function &f1, int &addr_offset) {
    string loop_label = ".L" + to_string(label_number++);
    string end_label = ".L" + to_string(label_number++);
    
    // for (int i =0; i < 5; i++)
    // int i = 0;
    // push loop label
    // i < 5
    // jump end_label
    // i = i + 1

    // loc++;  // go to next source code line
    // bool next_function = false;
    // while (loc < max_len) {
    //     if ((source[loc].find("int") == 0 || source[loc].find("void") == 0) && source[loc].find("}") == source[loc].length() - 1) {
    //         // start with a new function
    //         next_function = true;
    //         break;
    //     } else if (source[loc] == "}") {
    //         loc++;
    //     } else {
    //         // line is not function call or function end
    //         // send to common handler dispatcher
    //         common_instruction_handler_dispatcher(source, loc, max_len, f1, addr_offset);
    //     }
    // }

    // finished the for loop, already hit the }
    // unconditional jump back to loop_label  jmp     loop_label
    // push end_label


}

/*
    Handle return statements
*/
void return_handler(string source, Function &f1) {
  // If we return something then we have to move it to %eax
  if (source[6] != ';'){
    string rvalue = source.substr(7);
    if(std::find(f1.variables.begin(), f1.variables.end(), rvalue) != f1.variables.end())
    {
      if (f1.variables[std::find(f1.variables.begin(), f1.variables.end(), rvalue)].type == "int")
        f1.assembly_instructions.push_back(add_mov_instruction(rvalue, "%eax", 32));
      else
        f1.assembly_instructions.push_back(add_mov_instruction(rvalue, "%eax", 64));
    }
  }

  // Leave isn't always the most optimal, but if we want to use just
  // popq %rbp then we'll have to keep track of rbp and rsp in this
  // program. We could do that, but leave is always safe anyway so it's fine.
  f1.assembly_instructions.push_back("leave");
  f1.assembly_instructions.push_back("ret");
}

/*
    Handle other function call statements
*/
void function_call_handler(string input_str, Function &f1) {
  bool assigned = false;
  string regex_str = " ";
  auto tokens = split(input_str, regex_str);
  // Check if the line with the function call assigns the returned value
  if (tokens[1] == "="){
    assigned = true;
  }

  // Get string containing just function call
  std::string callstr = "";
  int i = 0;
  for (auto c : tokens){
      if (i < 2 && assigned){
          i++;
          continue;
      }
      callstr += c;
  }

  // Get name of function and parameter list
  std::string name = callstr.substr(0, callstr.find("("));
  std::string params = callstr.substr(callstr.find("(") + 1);
  params.pop_back(); params.pop_back(); // Get rid of ");"

  // Place first 6 parameters onto stack in reverse order
  // Concern: pushing an array requires 'lea' instead of 'mov'
  tokens = split(params, ",");

  vector<string> paramsv;
  for (auto p : tokens){
    paramsv.push_back(p);
  }

  i = 0;
  for (vector::iterator p = tokens.end(); p != tokens.start(); p--){
    for (Variable a : f1.variables){
      if (p == a.name){
        i++;
        if (i <= 6){
          if (a.type == "int")
            f1.assembly_instructions.push_back(add_mov_instruction
              (to_string(a.addr_offset) + "(%rbp)", "%" + register_for_argument_32[6-i], 32));
          else
            f1.assembly_instructions.push_back(add_mov_instruction
              (to_string(a.addr_offset) + "(%rbp)", "%" + register_for_argument_64[6-i], 64));
        }
        else{
          // After the first 6 arguments are placed in registers, the rest are put on the stack
          f1.assembly_instructions.push_back(add_mov_instruction(
            to_string(a.addr_offset) + "(%rbp)", "%rdi", 64));
          f1.assembly_instructions.push_back("pushq %rdi");
        }
      }
    }
  }

  // Call function
  f1.assembly_instructions.push_back("call " + name);

  for (auto s : f1.assembly_instructions)
    cout << s << endl;
}

/*
    Handle arithemetic statements
*/
void arithmetic_handler(string line, Function &f1) {

    string lineToParse = line;
    vector<string> parsedLine;//[0] = a, [2] = b, [3] = + , [4] = c {a = b + c}

    lineToParse.erase(lineToParse.length() - 1, lineToParse.length());// remove ";"

    string word;
    istringstream iss(lineToParse);
    while(iss >> word){// split the string into a vector
        parsedLine.push_back(word.c_str());
    }
}

int main() {
    int max_len = 0;
    vector<string> source = loadFile("test1.cpp", max_len);

    function_handler(source, 0, max_len);

    return 0;
}
