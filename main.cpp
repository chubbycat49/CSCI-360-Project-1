#include "main.h"

using namespace std;

vector<Function> functions;
int label_num = 2;

string register_for_argument_32[6] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
string register_for_argument_64[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

/*
    Helper function to debug a string and its size
*/
void view_var(string s) {
    cout << s << "\t" << s.size() << endl;
}

/*
    Helper function to print a function's instructions and variables
*/
void view_function(Function f1, bool show_vars) {
    for (auto x : f1.assembly_instructions) {
        cout << x << endl;
    }

    if (show_vars) {
        for (auto const &x : f1.variables) {
            cout << x.second.type << " " << x.first << " " << x.second.value << " " << x.second.addr_offset << endl;
        }
    }
}

/*
    Helper function used to create a modifed variable map that only includes
    arr[0] if the first 6 parameters has arrays
*/
map<string, Variable> actual_function_params(map<string, Variable> vars) {
    map<string, Variable> params;
    int counter = 0;

    for (auto v : vars) {
        string var_name = v.first;
        if (is_array_accessor(var_name) && (var_name[var_name.find("[") + 1] != '0') && counter <= 6) {
            continue;
        }
        counter++;

        params.insert(v);
    }

    return params;
}

/*
    Helper function to handle instructions related to loading values from parameters onto the stack
*/
map<string, Variable> variable_handler(string input_str, int &addr_offset) {
    map<string, Variable> out;
    auto var_tokens = split(input_str, ", ");
    for (auto c : var_tokens) {
        if (is_array_accessor(c)) {
            /*
            Split line by = and then by the [] to parse the name, size and values
        */
            c = split(c, " ")[1];

            auto temp = split(c, "\\[");  // [ needs to be escaped with \ and then that \ needs to be escaped for C++ complier
            string array_name = temp[0];

            int array_size = stoi(temp[1].substr(0, temp[1].size() - 1));

            for (int i = array_size - 1; i >= 0; --i) {
                string name = array_name + "[" + to_string(i) + "]";
                int arr_addr_offset = addr_offset - (i * 4);

                Variable var(name, "intptr", 0, arr_addr_offset, true);
                out.insert(pair<string, Variable>(name, var));
            }
            addr_offset -= (array_size * 4);
        } else {
            auto tmp = split(c, " ");
            string var_type = tmp[0];
            string var_name = tmp[1];

            Variable var(var_name, var_type, 0, addr_offset, true);
            out.insert(pair<string, Variable>(var_name, var));
            addr_offset -= 4;
        }
    }

    return out;
}

/*
    Helper function used to translate a move instruction depending on variable size
*/
string add_mov_instruction(string src, string dest, int size) {
    string out;
    switch (size) {
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
    Examples:
    i = test(a, b, c, d, e, f, g, h);
    test(a, b, c, d, e, f, g, h);
*/
bool is_function_call(string line) {
    return (is_substr(line, "(") && !is_substr(line, "+") && !is_substr(line, "-") && !is_substr(line, "*"));
}

/*
    Returns if +,-,* are in the string, aka code has arithmetic instructions
*/
bool is_arithmetic_line(const string s) {
    return is_substr(s, "+") || is_substr(s, "-") || is_substr(s, "*");
}

/*
    Return if variable is a parameter variable
*/
bool is_param_var(const string s, Function &f1) {
    if (is_int(s)) {
        return false;
    }

    string lookup_str = s;
    if (is_array_accessor_dynamic(s)) {
        string arr_name = s.substr(0, s.find("["));

        lookup_str = arr_name + "[0]";
    }
    return f1.variables.at(lookup_str).is_param;
}

/*
    Helper function to move immediate values into registers
    Pushes required assembly instructions
*/
void move_immediate_val_into_register(const string s, const string reg, Function &f1) {
    f1.assembly_instructions.push_back("movl $" + s + ", " + reg);
}

/*
    Helper function to move variable values into registers
    Pushes required assembly instructions
*/
void move_var_val_into_register(const string s, const string reg, Function &f1) {
    f1.assembly_instructions.push_back("movl " + to_string(f1.variables.at(s).addr_offset) + "(%rbp), " + reg);
}

/*
    Helper function to handle code like a[0] and a[f] which accesses array elements
    Pushes required assembly instructions to get that value into specified register
*/
void move_arr_val_into_register(const string s, const string reg, Function &f1) {
    string arr_name = s.substr(0, s.find("["));
    string arr_index = substr_between_indices(s, s.find("[") + 1, s.find("]"));

    if (is_int(arr_index)) {
        /*
            a[immediate]
            meaning this should be in f1.variables
        */
        f1.assembly_instructions.push_back("movl " + to_string(f1.variables.at(s).addr_offset) + "(%rbp), " + reg);
    } else {
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
    Helper function to handle code like a[0] and a[f] which accesses array elements
    This function is only used when the array is a function parameter
    Pushes required assembly instructions to get that value into specified register

    f = d + x[i]
    leaq    0(,%rax,4), %rdx
    movq    -40(%rbp), %rax
    addq    %rdx, %rax
    movl    (%rax), %edx

    f = d + x[0]
    movq    -40(%rbp), %rax
    movl    (%rax), %edx

    f = d + x[2]
    movq    -40(%rbp), %rax
    addq    $8, %rax
    movl    (%rax), %edx
*/
void move_param_arr_val_into_register(const string s, const string reg, Function &f1) {
    string arr_name = s.substr(0, s.find("["));
    string arr_index = substr_between_indices(s, s.find("[") + 1, s.find("]"));
    string arr_zero_index = to_string(f1.variables.at(arr_name + "[0]").addr_offset);

    if (is_array_accessor_dynamic(s)) {
        move_var_val_into_register(arr_index, "%eax", f1);
        f1.assembly_instructions.push_back("cltq");
        f1.assembly_instructions.push_back("leaq 0(,%rax,4), %rdx");
        f1.assembly_instructions.push_back("movq " + arr_zero_index + "(%rbp), %rax");
        f1.assembly_instructions.push_back("addq %rdx, %rax");
    } else {
        f1.assembly_instructions.push_back("movq " + arr_zero_index + "(%rbp), %rax");

        if (arr_index != "0") {
            string addq_amt = "$" + to_string(stoi(arr_index) * 4);
            f1.assembly_instructions.push_back("addq " + addq_amt + ", %rax");
        }
    }

    f1.assembly_instructions.push_back("movl (%rax), %" + reg);
}

/*
    Helper function to move immediate value into a specified destionation
    Pushes required assembly instrucitons to move immediate value into specified destination
*/
void store_immedaite_val(const string dest, const string val, Function &f1) {
    if (is_array_accessor_dynamic(dest)) {
        /*
            storing in array via variable
        */
        string arr_name = dest.substr(0, dest.find("["));
        string arr_index = substr_between_indices(dest, dest.find("[") + 1, dest.find("]"));

        f1.assembly_instructions.push_back("movl " + to_string(f1.variables.at(arr_index).addr_offset) + "(%rbp), %eax");
        f1.assembly_instructions.push_back("cltq");
        // get where arr_name[0] is
        string arr_start_offset = to_string(f1.variables.at(arr_name + "[0]").addr_offset);
        f1.assembly_instructions.push_back("movl $" + val + ", " + arr_start_offset + "(%rbp, %rax, 4)");
    } else {
        /*
            storing in variable or static array (a[0])
        */
        f1.assembly_instructions.push_back("movl $" + val + ", " + to_string(f1.variables.at(dest).addr_offset) + "(%rbp)");
    }
}

/*
    Helper function to move register value into a specified destionation
    Pushes required assembly instrucitons to move register value into specified destination
*/
void store_reg_val(const string dest, const string reg, Function &f1) {
    if (is_array_accessor_dynamic(dest)) {
        /*
            storing in array via variable
        */
        string arr_name = dest.substr(0, dest.find("["));
        string arr_index = substr_between_indices(dest, dest.find("[") + 1, dest.find("]"));

        f1.assembly_instructions.push_back("movl " + to_string(f1.variables.at(arr_index).addr_offset) + "(%rbp), %eax");
        f1.assembly_instructions.push_back("cltq");
        // get where arr_name[0] is
        string arr_start_offset = to_string(f1.variables.at(arr_name + "[0]").addr_offset);
        f1.assembly_instructions.push_back("movl " + reg + ", " + arr_start_offset + "(%rbp, %rax, 4)");
    } else {
        /*
            storing in variable or static array (a[0])
        */
        f1.assembly_instructions.push_back("movl " + reg + ", " + to_string(f1.variables.at(dest).addr_offset) + "(%rbp)");
    }
}

/*
    Helper function to handle code like thing1 comparator thing2
    Pushes required assembly instructions for comparison
*/
void comparison_handler(string &s, Function &f1, bool jump_if_false) {
    // mapping of all comparators and their associated assembly command
    map<string, string> comparators_jump_if_true;
    comparators_jump_if_true.insert(pair<string, string>("<", "jge"));
    comparators_jump_if_true.insert(pair<string, string>(">", "jle"));
    comparators_jump_if_true.insert(pair<string, string>("<=", "jg"));
    comparators_jump_if_true.insert(pair<string, string>(">=", "jl"));
    comparators_jump_if_true.insert(pair<string, string>("==", "jne"));
    comparators_jump_if_true.insert(pair<string, string>("!=", "je"));

    map<string, string> comparators_jump_if_false;
    comparators_jump_if_false.insert(pair<string, string>("<", "jle"));
    comparators_jump_if_false.insert(pair<string, string>(">", "jge"));
    comparators_jump_if_false.insert(pair<string, string>("<=", "jl"));
    comparators_jump_if_false.insert(pair<string, string>(">=", "jg"));
    comparators_jump_if_false.insert(pair<string, string>("==", "je"));
    comparators_jump_if_false.insert(pair<string, string>("!=", "jne"));

    auto comparators = jump_if_false ? comparators_jump_if_true : comparators_jump_if_false;

    auto tokens = split(s, " ");
    trim_vector(tokens);
    string l_comp = tokens[0];
    string r_comp = tokens[2];
    string comp = tokens[1];

    for (auto const &c : comparators) {
        string comparator = c.first;
        string command = c.second;

        if (is_substr(s, comparator)) {
            string label_instruction = ".L" + to_string(label_num);

            if (is_array_accessor(l_comp) || is_array_accessor(r_comp)) {
                /*
                    Handles where one of the comparands is accessing an array
                    - array array
                    - array var
                    - array immediate
                    - var array
                    - immediate array
                */
                if (is_array_accessor(l_comp) && is_array_accessor(r_comp)) {
                    // array array
                    move_arr_val_into_register(l_comp, "%edx", f1);
                    move_arr_val_into_register(r_comp, "%eax", f1);

                    f1.assembly_instructions.push_back("cmpl %eax, %edx");
                } else if (is_array_accessor(l_comp)) {
                    move_arr_val_into_register(l_comp, "%eax", f1);
                    if (is_int(r_comp)) {
                        // array immediate
                        r_comp = "$" + r_comp;
                    } else {
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
                } else if (is_array_accessor(r_comp)) {
                    move_arr_val_into_register(r_comp, "%eax", f1);
                    if (is_int(l_comp)) {
                        // immediate array
                        l_comp = "$" + l_comp;
                    } else {
                        // var array
                        l_comp = to_string(f1.variables.at(l_comp).addr_offset) + "(%rbp)";
                    }

                    f1.assembly_instructions.push_back("cmpl %eax, " + l_comp);
                }
            } else if (is_int(l_comp) || is_int(r_comp)) {
                /*
                    Handles where one of the comparands is an immediate and none are arrays
                    - immediate immediate
                    - immediate var
                    - var immedaite
                */
                if (is_int(l_comp) && is_int(r_comp)) {
                    /*
                        Doesn't handle case of when both are immediates
                    */
                } else if (is_int(l_comp)) {
                    r_comp = to_string(f1.variables.at(r_comp).addr_offset) + "(%rbp)";

                    f1.assembly_instructions.push_back("cmpl $" + l_comp + ", " + r_comp);
                } else {
                    l_comp = to_string(f1.variables.at(l_comp).addr_offset) + "(%rbp)";

                    f1.assembly_instructions.push_back("cmpl $" + r_comp + ", " + l_comp);
                }
            } else {
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

/*
    Create a function object, get function return type and function name
*/
void function_handler(vector<string> source, int loc, int max_len) {
    Function f1;
    string head = source[loc];
    f1.return_type = head.substr(0, head.find(' '));  // get return type
    string tempstr = head.substr(head.find(' ') + 1, head.length());
    f1.function_name = tempstr.substr(0, tempstr.find('('));  // get fxn name
    f1.assembly_instructions.push_back(f1.function_name + ":");
    f1.assembly_instructions.push_back("#" + substr_between_indices(source[loc], 0, source[loc].find(")") + 1));
    f1.assembly_instructions.push_back("pushq %rbp");
    f1.assembly_instructions.push_back("movq %rsp, %rbp");
    f1.is_leaf_function = true;

    // Get parameter list and read parameter values from registers

    int addr_offset = -4;
    tempstr = tempstr.substr(tempstr.find('(') + 1, tempstr.length() - f1.function_name.length() - 3);
    string parameter_str = tempstr.substr(tempstr.find('(') + 1, tempstr.find('('));
    parameter_str.pop_back();
    if (parameter_str.length() > 0) {
        int number_of_parameter = 0;

        // variable_handler makes variable for e[0], e[1], e[2], increaseing number of parameters
        f1.variables = variable_handler(parameter_str, addr_offset);

        for (auto &varpair : f1.variables) {
            Variable var = varpair.second;

            if (var.name.size() > 1 && var.name.substr(1) != "[0]" && number_of_parameter <= 6)
                continue;  // i.e., 'e[1]' doesn't need to be dealt with separately.

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
                f1.variables.at(varpair.first).addr_offset = 16 + (number_of_parameter - 6 - 1) * 4;

                addr_offset += 4;
            }
        }
    }

    // Go through each instruction

    loc++;  // go to next source code line
    bool next_function = false;
    while (loc < max_len) {
        if ((source[loc].find("int") == 0 || source[loc].find("void") == 0) && source[loc].find("{") == source[loc].length() - 1) {
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
        int last_offset = 0 - (--(f1.variables.end()))->second.addr_offset;
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
        f1.assembly_instructions.push_back("#" + source[loc]);
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
        return_handler(source[loc], f1);
        loc++;
    }
    /*
        code line starts with a function
    */
    else if (is_function_call(source[loc])) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        function_call_handler(source[loc], f1);
        f1.is_leaf_function = false;
        loc++;
    }
    /*
        code line has +, -, * and is an arithmetic instruction
    */
    else if (is_arithmetic_line(source[loc])) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        arithmetic_handler(source[loc], f1);
        loc++;
    }
    /*
        code line is an assignment instruction
    */
    else if (is_substr(source[loc], " = ")) {
        f1.assembly_instructions.push_back("#" + source[loc]);
        assignment_handler(source[loc], f1);
        loc++;
    }
    /*
        otherwise, line is blank
    */
    else {
        loc++;
    }
}

/*
    Handle variable declaration statements.
    If "[" and "]" in line, then it's an array declaration,
    otherwise it is a primative variable declaration.
*/
void variable_offset_allocation(vector<string> &source, int &loc, Function &f1, int &addr_offset) {
    const string var_type = "int";

    if (is_array_accessor(split(source[loc], " = ")[0])) {
        auto tokens = split(source[loc], " = ");
        trim_vector(tokens);
        remove_ending_semicolon_vector(tokens);

        string dest = split(tokens[0], " ")[1];
        string src = tokens[1];

        auto temp = split(dest, "\\[");  // [ needs to be escaped with \ and then that \ needs to be escaped for C++ complier
        string array_name = temp[0];

        temp[1].pop_back();
        int array_size = stoi(temp[1]);

        auto array_values_str = tokens[1];
        auto array_values = split(array_values_str.substr(1, array_values_str.size() - 2), ", ");  // removes { and } and splits into int values

        for (int i = array_size - 1; i >= 0; --i) {
            int val = stoi(array_values[i]);
            string name = array_name + "[" + to_string(i) + "]";
            int arr_addr_offset = addr_offset - (i * 4);

            Variable var(name, var_type, val, arr_addr_offset);
            f1.variables.insert(pair<string, Variable>(name, var));
            f1.assembly_instructions.push_back("movl $" + array_values[i] + ", " + to_string(arr_addr_offset) + "(%rbp)");
        }
        addr_offset -= (array_size * 4);
    } else {
        /*
            int a = 0; b = 1; c = 2; d = 3;
        */
        auto var_tokens = split(source[loc].substr(source[loc].find(" ") + 1), ",");
        trim_vector(var_tokens);
        remove_ending_semicolon_vector(var_tokens);

        for (string s : var_tokens) {
            auto tokens = split(s, " = ");
            trim_vector(tokens);
            remove_ending_semicolon_vector(tokens);

            string var_name = tokens[0];
            string var_value_str = tokens[1];
            int var_value = is_int(var_value_str) ? stoi(var_value_str) : 0;

            string src = "%eax";

            if (is_arithmetic_line(var_value_str)) {
                /*
                    var = arithmetic
                */
                arithmetic_handler(s, f1, false);
            } else if (is_array_accessor(var_value_str)) {
                /*
                    var = arr[i], var = arr[0]
                */
                move_arr_val_into_register(var_value_str, "%eax", f1);
            } else if (is_int(var_value_str)) {
                /*
                    var = num
                */
                src = "$" + var_value_str;
            } else {
                /*
                    var = var
                */
                move_var_val_into_register(var_value_str, "%eax", f1);
            }

            f1.assembly_instructions.push_back("movl " + src + ", " + to_string(addr_offset) + "(%rbp)");

            Variable var(var_name, var_type, var_value, addr_offset);
            f1.variables.insert(pair<string, Variable>(var_name, var));

            addr_offset -= 4;
        }
    }
}

/*
    Handle if statements
*/
void IF_statement_handler(vector<string> &source, int &loc, int max_len, Function &f1, int &addr_offset) {
    string comparison = substr_between_indices(source[loc], source[loc].find("(") + 1, source[loc].find(")"));
    comparison_handler(comparison, f1);
    loc++;

    while (source[loc] != "}") {
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
void FOR_statement_handler(vector<string> &source, int &loc, int max_len, Function &f1, int &addr_offset) {
    string loop_label = ".L" + to_string(label_num++);
    string end_label = ".L" + to_string(label_num++);

    string line = source[loc];
    line = substr_between_indices(line, line.find("(") + 1, line.find(")"));
    vector<string> tokens = split(line, "; ");

    vector<string> temp;
    temp.push_back(tokens[0]);
    int loc_temp = 0;
    variable_offset_allocation(temp, loc_temp, f1, addr_offset);
    f1.assembly_instructions.push_back("jmp " + end_label);
    f1.assembly_instructions.push_back(loop_label);

    loc++;
    while (source[loc] != "}") {
        common_instruction_handler_dispatcher(source, loc, max_len, f1, addr_offset);
    }

    arithmetic_handler(tokens[2], f1);
    f1.assembly_instructions.push_back(end_label);

    // jmp instruction from comparison jumps to the wrong place
    // need to change it to loop_label
    comparison_handler(tokens[1], f1, false);
    f1.assembly_instructions.back().replace(f1.assembly_instructions.back().find("."), 3, loop_label);

    f1.assembly_instructions.push_back("# }");
    loc++;
}

/*
    Handle return statements
*/
void return_handler(string source, Function &f1) {
    // If we return something then we have to move it to %eax
    if (f1.function_name == "main")
        f1.assembly_instructions.push_back("movl $0, %eax");

    if (source[6] != ';') {
        string rvalue = source.substr(7);
        rvalue.pop_back();

        if (f1.variables.count(rvalue) > 0) {
            Variable a = f1.variables.at(rvalue);
            if (f1.variables.at(rvalue).type == "int")
                f1.assembly_instructions.push_back(add_mov_instruction(to_string(a.addr_offset) + "(%rbp)", "%eax", 32));
            else
                f1.assembly_instructions.push_back(add_mov_instruction(to_string(a.addr_offset) + "(%rbp)", "%rax", 64));
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
    string dest;
    string firstparam;
    string regex_str = " ";
    auto tokens = split(input_str, regex_str);
    // Check if the line with the function call assigns the returned value
    if (tokens[1] == "=") {
        dest = tokens[0];
        assigned = true;
    }

    // Get string containing just function call
    std::string callstr = "";
    int i = 0;
    for (auto c : tokens) {
        if (i < 2 && assigned) {
            i++;
            continue;
        }
        callstr += c;
    }

    // Get name of function and parameter list
    std::string name = callstr.substr(0, callstr.find("("));
    std::string params = callstr.substr(callstr.find("(") + 1);
    params.pop_back();
    params.pop_back();  // Get rid of ");"

    // Place first 6 parameters onto stack in reverse order
    // The first parameter gets saved away in %eax so %rdi can be used to push
    tokens = split(params, ",");
    trim_vector(tokens);
    vector<string> paramsv;
    for (auto p : tokens) {
        paramsv.push_back(p);
    }

    vector<string> extraArgs;

    i = -1;
    for (auto p : tokens) {
        /* If the argument is a non-array variable */
        if (f1.variables.count(p) > 0) {
            Variable a = f1.variables.at(p);
            i++;
            if (i > 0 && i < 6) {
                if (a.type == "int")
                    f1.assembly_instructions.push_back(add_mov_instruction(to_string(a.addr_offset) + "(%rbp)", "%" + register_for_argument_32[i], 32));
                else
                    f1.assembly_instructions.push_back(add_mov_instruction(to_string(a.addr_offset) + "(%rbp)", "%" + register_for_argument_64[i], 64));
            } else if (i == 0) {  // We put the first param in %eax/%rax for now
                if (a.type == "int") {
                    f1.assembly_instructions.push_back(add_mov_instruction(to_string(a.addr_offset) + "(%rbp)", "%eax", 32));
                    firstparam = "%eax";
                } else {
                    f1.assembly_instructions.push_back(add_mov_instruction(to_string(a.addr_offset) + "(%rbp)", "%rax", 64));
                    firstparam = "%rax";
                }
            } else {
                // After the first 6 arguments are placed in registers, the rest are put on the stack
                // f1.assembly_instructions.push_back(add_mov_instruction(
                //   to_string(a.addr_offset) + "(%rbp)", "%rdi", 64));
                // f1.assembly_instructions.push_back("pushq %rdi");
                extraArgs.insert(extraArgs.begin(), p);
            }
        }

        /* If the argument is an array variable */
        else if (f1.variables.count(p + "[0]") > 0) {  // f1.variables contains p[0]
            Variable a = f1.variables.at(p + "[0]");
            i++;
            if (i > 0 && i < 6) {
                f1.assembly_instructions.push_back("leaq " + to_string(a.addr_offset) + "(%rbp), " + "%" + register_for_argument_64[i]);
            } else if (i == 0) {  // We put the first param address in %rax for now
                f1.assembly_instructions.push_back("leaq " + to_string(a.addr_offset) + "(%rbp), " + "%rax");
                firstparam = "%rax";
            } else {
                // After the first 6 arguments are placed in registers, the rest are put on the stack
                // f1.assembly_instructions.push_back("leaq " + to_string(a.addr_offset) + "(%rbp), " + "%rdi");
                // f1.assembly_instructions.push_back("pushq %rdi");
                extraArgs.insert(extraArgs.begin(), p + "[0]");
            }
        }

        /* If the argument is not a variable but a literal (only works for ints) */
        else {
            if (i > 0)
                f1.assembly_instructions.push_back("movl $" + p + ", %" + register_for_argument_32[i]);
            else if (i == 0) {
                f1.assembly_instructions.push_back("movl $" + p + ", %eax");
                firstparam = "%eax";
            } else
                // f1.assembly_instructions.push_back("pushq $" + p);
                extraArgs.insert(extraArgs.begin(), "$" + p);
        }
    }

    /* Second loop for extra arguments */
    for (string ext : extraArgs) {
        Variable a = f1.variables.at(ext);
        if (f1.variables.count(ext) > 0) {
            f1.assembly_instructions.push_back(add_mov_instruction(
                to_string(a.addr_offset) + "(%rbp)", "%rdi", 64));
            f1.assembly_instructions.push_back("pushq %rdi");
        } else if (f1.variables.count(ext + "[0]") > 0) {
            f1.assembly_instructions.push_back("leaq " + to_string(a.addr_offset) + "(%rbp), " + "%rdi");
            f1.assembly_instructions.push_back("pushq %rdi");
        } else {
            f1.assembly_instructions.push_back("pushq $" + ext);
        }
    }

    // Put the first parameter back into %edi or %rdi
    if (firstparam == "%eax")
        f1.assembly_instructions.push_back("movl %eax, %edi");
    else
        f1.assembly_instructions.push_back("movl %rax, %rdi");

    // Call function
    f1.assembly_instructions.push_back("call " + name);

    // Move stack pointer
    f1.assembly_instructions.push_back("addq $16, %rsp");

    // Assign returned value
    store_reg_val(dest, "%eax", f1);
}

/*
    Handle arithmetic statements
*/
void arithmetic_handler(string &s, Function &f1, bool store_result) {
    if (is_substr(s, "++")) {
        string var = substr_between_indices(s, 0, s.find("++"));
        if (is_array_accessor_dynamic(var)) {
            // a[i]++;
            string arr_name = s.substr(0, s.find("["));
            string arr_index = substr_between_indices(s, s.find("[") + 1, s.find("]"));
            string arr_zero = arr_name + "[0]";

            if (is_param_var(var, f1)) {
                // param[i]++
                move_param_arr_val_into_register(var, "%edx", f1);
                f1.assembly_instructions.push_back("addl $1, %edx");
                f1.assembly_instructions.push_back("movl %edx, (%rax)");
            } else {
                move_arr_val_into_register(var, "%eax", f1);
                f1.assembly_instructions.push_back("leal 1(%rax), %edx");
                f1.assembly_instructions.push_back("movl " + to_string(f1.variables.at(arr_index).addr_offset) + "(%rbp), %eax");
                f1.assembly_instructions.push_back("cltq");

                // movl    %edx, -16(%rbp,%rax,4)
                // -16(%rbp)[4*%rax]
                f1.assembly_instructions.push_back("movl %edx, " + to_string(f1.variables.at(arr_zero).addr_offset) + "(%rbp, %rax, 4)");
            }
        } else if (is_array_accessor(var)) {
            // a[0]++;
            string arr_name = s.substr(0, s.find("["));
            string arr_index = substr_between_indices(s, s.find("[") + 1, s.find("]"));
            string arr_addr = to_string(f1.variables.at(var).addr_offset);

            if (is_param_var(var, f1)) {
                if (arr_index == "0") {
                    /*
                        param[0]++

                        movq    -40(%rbp), %rax
                        movl    (%rax), %eax
                        leal    1(%rax), %edx
                        movq    -40(%rbp), %rax
                        movl    %edx, (%rax)
                    */
                    f1.assembly_instructions.push_back("movq " + arr_addr + "(%rbp), %rax");
                    f1.assembly_instructions.push_back("movl (%rax), %eax");
                    f1.assembly_instructions.push_back("leal 1(%rax), %edx");
                    f1.assembly_instructions.push_back("movq " + arr_addr + "(%rbp), %rax");
                } else {
                    /*
                        param[2]++

                        movq    -40(%rbp), %rax
                        addq    $8, %rax
                        movl    (%rax), %edx
                        addl    $1, %edx
                        movl    %edx, (%rax)
                    */
                    move_param_arr_val_into_register(var, "%edx", f1);
                    f1.assembly_instructions.push_back("addl $1, %edx");
                }

                f1.assembly_instructions.push_back("movl %edx, (%rax)");
            } else {
                move_arr_val_into_register(var, "%eax", f1);
                f1.assembly_instructions.push_back("addl $1, %eax");
                store_reg_val(var, "%eax", f1);
            }
        } else {
            // i++;
            f1.assembly_instructions.push_back("addl $1, " + to_string(f1.variables.at(var).addr_offset) + "(%rbp");
        }
        store_result = false;
    } else if (is_substr(s, "--")) {
        string var = substr_between_indices(s, 0, s.find("--"));
        if (is_array_accessor_dynamic(var)) {
            // a[i]--;
            if (is_param_var(var, f1)) {
                // param[i]--
                move_param_arr_val_into_register(var, "%edx", f1);
                f1.assembly_instructions.push_back("subl $1, %edx");
                f1.assembly_instructions.push_back("movl %edx, (%rax)");
            } else {
                string arr_name = s.substr(0, s.find("["));
                string arr_index = substr_between_indices(s, s.find("[") + 1, s.find("]"));
                string arr_zero = arr_name + "[0]";

                move_arr_val_into_register(var, "%eax", f1);
                f1.assembly_instructions.push_back("leal -1(%rax), %edx");
                f1.assembly_instructions.push_back("movl " + to_string(f1.variables.at(arr_index).addr_offset) + "(%rbp), %eax");
                f1.assembly_instructions.push_back("cltq");
                // movl    %edx, -16(%rbp,%rax,4)
                // -16(%rbp)[4*%rax]
                f1.assembly_instructions.push_back("movl %edx, " + to_string(f1.variables.at(arr_zero).addr_offset) + "(%rbp, %rax, 4)");
            }
        } else if (is_array_accessor(var)) {
            // a[0]--;
            string arr_name = s.substr(0, s.find("["));
            string arr_index = substr_between_indices(s, s.find("[") + 1, s.find("]"));
            string arr_addr = to_string(f1.variables.at(var).addr_offset);

            if (is_param_var(var, f1)) {
                if (arr_index == "0") {
                    /*
                        param[0]--

                        movq    -40(%rbp), %rax
                        movl    (%rax), %eax
                        leal    1(%rax), %edx
                        movq    -40(%rbp), %rax
                        movl    %edx, (%rax)
                    */
                    f1.assembly_instructions.push_back("movq " + arr_addr + "(%rbp), %rax");
                    f1.assembly_instructions.push_back("movl (%rax), %eax");
                    f1.assembly_instructions.push_back("leal -1(%rax), %edx");
                    f1.assembly_instructions.push_back("movq " + arr_addr + "(%rbp), %rax");
                } else {
                    /*
                        param[2]--

                        movq    -40(%rbp), %rax
                        addq    $8, %rax
                        movl    (%rax), %edx
                        addl    $1, %edx
                        movl    %edx, (%rax)
                    */
                    move_param_arr_val_into_register(var, "%edx", f1);
                    f1.assembly_instructions.push_back("subl $1, %edx");
                }

                f1.assembly_instructions.push_back("movl %edx, (%rax)");
            } else {
                move_arr_val_into_register(var, "%eax", f1);
                f1.assembly_instructions.push_back("subl $1, %eax");
                store_reg_val(var, "%eax", f1);
            }
        } else {
            // i--;
            f1.assembly_instructions.push_back("subl $1, " + to_string(f1.variables.at(var).addr_offset) + "(%rbp");
        }
        store_result = false;
    } else {
        auto tokens = split(s, " = ");
        trim_vector(tokens);
        remove_ending_semicolon_vector(tokens);
        string dest = tokens[0];
        string arithmetic_str = tokens[1];

        auto arithmetic_tokens = split(arithmetic_str, " ");
        trim_vector(arithmetic_tokens);
        string l_val = arithmetic_tokens[0];
        string op = arithmetic_tokens[1];
        string r_val = arithmetic_tokens[2];

        if (is_param_var(l_val, f1) || is_param_var(r_val, f1)) {
            /*
                This doesn't deal with if the destination is a parameter value
                It's different if the destination is e[>0] or e[var]
            */
            if (op == "+") {
                if (is_array_accessor(l_val) || is_array_accessor(r_val)) {
                    if (is_array_accessor(l_val) && is_array_accessor(r_val)) {
                        // arr arr
                        move_param_arr_val_into_register(l_val, "%edx", f1);
                        move_param_arr_val_into_register(r_val, "%eax", f1);

                        f1.assembly_instructions.push_back("addl %edx, %eax");
                    } else if (is_array_accessor(l_val)) {
                        if (is_int(r_val)) {
                            // arr num
                            move_param_arr_val_into_register(l_val, "%eax", f1);

                            r_val = "$" + r_val;
                        } else {
                            // arr var
                            move_param_arr_val_into_register(l_val, "%edx", f1);
                            move_var_val_into_register(r_val, "%eax", f1);

                            r_val = "%edx";
                        }

                        f1.assembly_instructions.push_back("addl " + r_val + ", %eax");
                    } else if (is_array_accessor(r_val)) {
                        if (is_int(l_val)) {
                            // num arr
                            move_param_arr_val_into_register(r_val, "%eax", f1);

                            r_val = "$" + r_val;
                        } else {
                            // var arr
                            move_param_arr_val_into_register(r_val, "%edx", f1);
                            move_var_val_into_register(l_val, "%eax", f1);

                            r_val = "%edx";
                        }

                        f1.assembly_instructions.push_back("addl " + r_val + ", %eax");
                    }
                } else if (is_int(l_val) || is_int(r_val)) {
                    if (is_int(l_val) && is_int(r_val)) {
                        /*
                        Doesn't handle case of when both are immediates
                    */
                    } else if (is_int(l_val)) {
                        // num + var
                        move_var_val_into_register(r_val, "%eax", f1);

                        f1.assembly_instructions.push_back("addl $" + l_val + ", %eax");
                    } else if (is_int(r_val)) {
                        // var + num
                        move_var_val_into_register(l_val, "%eax", f1);

                        f1.assembly_instructions.push_back("addl $" + r_val + ", %eax");
                    }
                } else {
                    // both operands are varaiables(non-arrays)
                    move_var_val_into_register(l_val, "%edx", f1);
                    move_var_val_into_register(r_val, "%eax", f1);

                    f1.assembly_instructions.push_back("addl %edx, %eax");
                }
            } else if (op == "-") {
                if (is_array_accessor(l_val) || is_array_accessor(r_val)) {
                    if (is_array_accessor(l_val) && is_array_accessor(r_val)) {
                        // arr arr
                        move_param_arr_val_into_register(l_val, "%eax", f1);
                        move_param_arr_val_into_register(r_val, "%edx", f1);

                        f1.assembly_instructions.push_back("subl %eax, %edx");
                        f1.assembly_instructions.push_back("movl %edx, %eax");
                    } else if (is_array_accessor(l_val)) {
                        if (is_int(r_val)) {
                            // arr num
                            move_param_arr_val_into_register(l_val, "%eax", f1);

                            r_val = "$" + r_val;
                        } else {
                            // arr var
                            move_param_arr_val_into_register(l_val, "%eax", f1);

                            r_val = to_string(f1.variables.at(r_val).addr_offset) + "(%rbp)";
                        }

                        f1.assembly_instructions.push_back("subl " + r_val + ", %eax");
                    } else if (is_array_accessor(r_val)) {
                        if (is_int(l_val)) {
                            // num arr
                            move_param_arr_val_into_register(r_val, "%eax", f1);
                            move_immediate_val_into_register(l_val, "%edx", f1);
                        } else {
                            // var arr
                            move_param_arr_val_into_register(r_val, "%eax", f1);
                            move_var_val_into_register(l_val, "%edx", f1);
                        }

                        f1.assembly_instructions.push_back("subl %eax, %edx");
                        f1.assembly_instructions.push_back("movl %edx, %eax");
                    }
                } else if (is_int(l_val) || is_int(r_val)) {
                    if (is_int(l_val) && is_int(r_val)) {
                        /*
                        Doesn't handle case of when both are immediates
                    */
                    } else if (is_int(l_val)) {
                        // num - var
                        move_immediate_val_into_register(l_val, "%eax", f1);

                        f1.assembly_instructions.push_back("subl " + to_string(f1.variables.at(r_val).addr_offset) + "(%rbp), %eax");
                    } else if (is_int(r_val)) {
                        // var - num
                        move_var_val_into_register(l_val, "%eax", f1);

                        f1.assembly_instructions.push_back("subl $" + r_val + ", %eax");
                    }
                } else {
                    // both operands are varaiables(non-arrays)
                    move_var_val_into_register(l_val, "%eax", f1);

                    f1.assembly_instructions.push_back("subl " + to_string(f1.variables.at(r_val).addr_offset) + "(%rbp), %eax");
                }
            } else if (op == "*") {
                if (is_array_accessor(l_val) || is_array_accessor(r_val)) {
                    if (is_array_accessor(l_val) && is_array_accessor(r_val)) {
                        // arr arr
                        move_param_arr_val_into_register(l_val, "%edx", f1);
                        move_param_arr_val_into_register(r_val, "%eax", f1);

                        f1.assembly_instructions.push_back("imull %edx, %eax");
                    } else if (is_array_accessor(l_val)) {
                        if (is_int(r_val)) {
                            // arr num
                            move_param_arr_val_into_register(l_val, "%eax", f1);

                            f1.assembly_instructions.push_back("imull $" + r_val + ", %eax, %eax");
                        } else {
                            // arr var
                            move_param_arr_val_into_register(l_val, "%eax", f1);

                            f1.assembly_instructions.push_back("imull " + to_string(f1.variables.at(r_val).addr_offset) + "(%rbp), %eax");
                        }
                    } else if (is_array_accessor(r_val)) {
                        if (is_int(l_val)) {
                            // num arr
                            move_param_arr_val_into_register(r_val, "%eax", f1);

                            f1.assembly_instructions.push_back("imull $" + l_val + ", %eax, %eax");
                        } else {
                            // var arr
                            move_param_arr_val_into_register(r_val, "%eax", f1);

                            f1.assembly_instructions.push_back("imull %edx, " + to_string(f1.variables.at(l_val).addr_offset) + "(%rbp)");
                        }
                    }
                } else if (is_int(l_val) || is_int(r_val)) {
                    if (is_int(l_val) && is_int(r_val)) {
                        /*
                        Doesn't handle case of when both are immediates
                    */
                    } else if (is_int(l_val)) {
                        // num - var
                        move_var_val_into_register(r_val, "%eax", f1);

                        f1.assembly_instructions.push_back("imull $" + l_val + ", %eax, %eax");
                    } else if (is_int(r_val)) {
                        // var - num
                        move_var_val_into_register(l_val, "%eax", f1);

                        f1.assembly_instructions.push_back("imull $" + r_val + ", %eax, %eax");
                    }
                } else {
                    // both operands are varaiables(non-arrays)
                    move_var_val_into_register(l_val, "%eax", f1);

                    f1.assembly_instructions.push_back("imull " + to_string(f1.variables.at(r_val).addr_offset) + "(%rbp), %eax");
                }
            }
        } else {
            if (op == "+") {
                if (is_array_accessor(l_val) || is_array_accessor(r_val)) {
                    if (is_array_accessor(l_val) && is_array_accessor(r_val)) {
                        // arr arr
                        move_arr_val_into_register(l_val, "%edx", f1);
                        move_arr_val_into_register(r_val, "%eax", f1);

                        f1.assembly_instructions.push_back("addl %edx, %eax");
                    } else if (is_array_accessor(l_val)) {
                        if (is_int(r_val)) {
                            // arr num
                            move_arr_val_into_register(l_val, "%eax", f1);

                            r_val = "$" + r_val;
                        } else {
                            // arr var
                            move_arr_val_into_register(l_val, "%edx", f1);
                            move_var_val_into_register(r_val, "%eax", f1);

                            r_val = "%edx";
                        }

                        f1.assembly_instructions.push_back("addl " + r_val + ", %eax");
                    } else if (is_array_accessor(r_val)) {
                        if (is_int(l_val)) {
                            // num arr
                            move_arr_val_into_register(r_val, "%eax", f1);

                            r_val = "$" + r_val;
                        } else {
                            // var arr
                            move_arr_val_into_register(r_val, "%edx", f1);
                            move_var_val_into_register(l_val, "%eax", f1);

                            r_val = "%edx";
                        }

                        f1.assembly_instructions.push_back("addl " + r_val + ", %eax");
                    }
                } else if (is_int(l_val) || is_int(r_val)) {
                    if (is_int(l_val) && is_int(r_val)) {
                        /*
                        Doesn't handle case of when both are immediates
                    */
                    } else if (is_int(l_val)) {
                        // num + var
                        move_var_val_into_register(r_val, "%eax", f1);

                        f1.assembly_instructions.push_back("addl $" + l_val + ", %eax");
                    } else if (is_int(r_val)) {
                        // var + num
                        move_var_val_into_register(l_val, "%eax", f1);

                        f1.assembly_instructions.push_back("addl $" + r_val + ", %eax");
                    }
                } else {
                    // both operands are varaiables(non-arrays)
                    move_var_val_into_register(l_val, "%edx", f1);
                    move_var_val_into_register(r_val, "%eax", f1);

                    f1.assembly_instructions.push_back("addl %edx, %eax");
                }
            } else if (op == "-") {
                if (is_array_accessor(l_val) || is_array_accessor(r_val)) {
                    if (is_array_accessor(l_val) && is_array_accessor(r_val)) {
                        // arr arr
                        move_arr_val_into_register(l_val, "%eax", f1);
                        move_arr_val_into_register(r_val, "%edx", f1);

                        f1.assembly_instructions.push_back("subl %eax, %edx");
                        f1.assembly_instructions.push_back("movl %edx, %eax");
                    } else if (is_array_accessor(l_val)) {
                        if (is_int(r_val)) {
                            // arr num
                            move_arr_val_into_register(l_val, "%eax", f1);

                            r_val = "$" + r_val;
                        } else {
                            // arr var
                            move_arr_val_into_register(l_val, "%eax", f1);

                            r_val = to_string(f1.variables.at(r_val).addr_offset) + "(%rbp)";
                        }

                        f1.assembly_instructions.push_back("subl " + r_val + ", %eax");
                    } else if (is_array_accessor(r_val)) {
                        if (is_int(l_val)) {
                            // num arr
                            move_arr_val_into_register(r_val, "%eax", f1);
                            move_immediate_val_into_register(l_val, "%edx", f1);
                        } else {
                            // var arr
                            move_arr_val_into_register(r_val, "%eax", f1);
                            move_var_val_into_register(l_val, "%edx", f1);
                        }

                        f1.assembly_instructions.push_back("subl %eax, %edx");
                        f1.assembly_instructions.push_back("movl %edx, %eax");
                    }
                } else if (is_int(l_val) || is_int(r_val)) {
                    if (is_int(l_val) && is_int(r_val)) {
                        /*
                        Doesn't handle case of when both are immediates
                    */
                    } else if (is_int(l_val)) {
                        // num - var
                        move_immediate_val_into_register(l_val, "%eax", f1);

                        f1.assembly_instructions.push_back("subl " + to_string(f1.variables.at(r_val).addr_offset) + "(%rbp), %eax");
                    } else if (is_int(r_val)) {
                        // var - num
                        move_var_val_into_register(l_val, "%eax", f1);

                        f1.assembly_instructions.push_back("subl $" + r_val + ", %eax");
                    }
                } else {
                    // both operands are varaiables(non-arrays)
                    move_var_val_into_register(l_val, "%eax", f1);

                    f1.assembly_instructions.push_back("subl " + to_string(f1.variables.at(r_val).addr_offset) + "(%rbp), %eax");
                }
            } else if (op == "*") {
                if (is_array_accessor(l_val) || is_array_accessor(r_val)) {
                    if (is_array_accessor(l_val) && is_array_accessor(r_val)) {
                        // arr arr
                        move_arr_val_into_register(l_val, "%edx", f1);
                        move_arr_val_into_register(r_val, "%eax", f1);

                        f1.assembly_instructions.push_back("imull %edx, %eax");
                    } else if (is_array_accessor(l_val)) {
                        if (is_int(r_val)) {
                            // arr num
                            move_arr_val_into_register(l_val, "%eax", f1);

                            f1.assembly_instructions.push_back("imull $" + r_val + ", %eax, %eax");
                        } else {
                            // arr var
                            move_arr_val_into_register(l_val, "%eax", f1);

                            f1.assembly_instructions.push_back("imull " + to_string(f1.variables.at(r_val).addr_offset) + "(%rbp), %eax");
                        }
                    } else if (is_array_accessor(r_val)) {
                        if (is_int(l_val)) {
                            // num arr
                            move_arr_val_into_register(r_val, "%eax", f1);

                            f1.assembly_instructions.push_back("imull $" + l_val + ", %eax, %eax");
                        } else {
                            // var arr
                            move_arr_val_into_register(r_val, "%eax", f1);

                            f1.assembly_instructions.push_back("imull %edx, " + to_string(f1.variables.at(l_val).addr_offset) + "(%rbp)");
                        }
                    }
                } else if (is_int(l_val) || is_int(r_val)) {
                    if (is_int(l_val) && is_int(r_val)) {
                        /*
                        Doesn't handle case of when both are immediates
                    */
                    } else if (is_int(l_val)) {
                        // num - var
                        move_var_val_into_register(r_val, "%eax", f1);

                        f1.assembly_instructions.push_back("imull $" + l_val + ", %eax, %eax");
                    } else if (is_int(r_val)) {
                        // var - num
                        move_var_val_into_register(l_val, "%eax", f1);

                        f1.assembly_instructions.push_back("imull $" + r_val + ", %eax, %eax");
                    }
                } else {
                    // both operands are varaiables(non-arrays)
                    move_var_val_into_register(l_val, "%eax", f1);

                    f1.assembly_instructions.push_back("imull " + to_string(f1.variables.at(r_val).addr_offset) + "(%rbp), %eax");
                }
            }
        }
        if (store_result) {
            string reg;
            if (is_array_accessor(dest)) {
                // add move to edx line
                f1.assembly_instructions.push_back("movl %eax, %edx");
                reg = "%edx";
            } else {
                reg = "%eax";
            }
            store_reg_val(dest, reg, f1);
        }
    }
}

/*
    Handles assignment statements
*/
void assignment_handler(string &s, Function &f1) {
    auto tokens = split(s, " = ");
    trim_vector(tokens);
    remove_ending_semicolon_vector(tokens);
    string dest = tokens[0];
    string src = tokens[1];

    if (is_array_accessor_dynamic(dest)) {
        /*
            arr[i] = var
            arr[i] = num
        */
        string arr_name = dest.substr(0, dest.find("["));
        string arr_index = substr_between_indices(dest, dest.find("[") + 1, dest.find("]"));

        if (is_int(src)) {
            // arr[i] = 0
            store_immedaite_val(dest, src, f1);

        } else if (is_array_accessor(src)) {
            // arr[i] = d[0], arr[i] = d[b]
            move_arr_val_into_register(src, "%edx", f1);
            store_reg_val(dest, "%edx", f1);
        } else {
            // arr[i] = b
            move_var_val_into_register(src, "%edx", f1);
            store_reg_val(dest, "%edx", f1);
        }
    } else {
        if (is_int(src)) {
            // a = 0, arr[0] = 0
            store_immedaite_val(dest, src, f1);
        } else if (is_array_accessor(src)) {
            // a = c[1], a = c[x], arr[0] = c[1], arr[0] = c[x]
            move_arr_val_into_register(src, "%eax", f1);
            store_reg_val(dest, "%eax", f1);
        } else {
            // a = c, arr[0] = c
            move_var_val_into_register(src, "%eax", f1);
            store_reg_val(dest, "%eax", f1);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Please proved an input file and output file name. Exitting..." << endl;
        return 0;
    }

    string input_fn = argv[1];
    string output_fn = argv[2];

    int max_len = 0;
    vector<string> source = loadFile(input_fn, max_len);

    function_handler(source, 0, max_len);

    cout << "Finished translating file. Outputting to: " << output_fn << endl;

    ofstream fileOUT(output_fn, ios::out | ios::trunc);
    fileOUT.close();

    for (Function f : functions) {
        writeFile(output_fn, f.assembly_instructions, f.function_name);
    }

    cout << "Finished writing to: " << output_fn << endl;

    return 0;
}
