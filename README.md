# CSCI-360-Project-1

## Background

A C/C++ compiler which shows the assembly output of a given source code. Based on the 
godbolt.org compiler, this program is a simplified version handling some basic instruction translations. This implementation only handles integer values. Instruction translations include variable declaration and initialization, array decleration and initialization, function declerations, `for()` loop instruction, `if()` instructions, `return` statements and arithmetic instructions. The arithmetic instructions include addition, subtraction and multiplication in the form of `destination = operand1 + operand2`.