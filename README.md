# 🧠 Mini Compiler in C

A modular mini compiler implemented in **C (C99)** that demonstrates the major stages of a compiler pipeline, from lexical analysis to optimized x86-style assembly code generation.

## 🚀 Compiler Pipeline

```text
Source Program

      │
      ▼
┌─────────────────────┐
│  Lexical Analysis   │
│      (Lexer)        │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Syntax Analysis /   │
│      Parser         │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Three-Address Code  │
│      (TAC)          │
└──────────┬──────────┘
           ▼       
┌─────────────────────┐
│   Code Optimizer    │
│ Constant Folding    │
│ CSE + DCE           │
└──────────┬──────────┘
           ▼
┌─────────────────────┐
│ Assembly Generator  │
│   x86 NASM-style    │
└─────────────────────┘

✨ Features
Lexical analysis and token identification
Recursive-descent expression parsing
Operator precedence handling
Three-address code generation
Constant folding
Common Subexpression Elimination (CSE)
Dead Code Elimination (DCE)
x86 NASM-style assembly code generation
Multiple built-in test programs
Complete compiler pipeline integration
🧩 Compiler Modules
1️⃣ Lexical Analysis

The lexer identifies different types of tokens including:

Keywords
Identifiers
Numbers
Operators
Multi-character operators

Example:

Token: int             -> KEYWORD
Token: a               -> IDENTIFIER
Token: 10              -> NUMBER
Token: +               -> OPERATOR
2️⃣ Syntax Analysis / Parser

The parser uses a recursive-descent approach to process arithmetic expressions.

It handles:

Addition
Subtraction
Multiplication
Division
Parentheses

The parser follows operator precedence through separate expression, term, and factor functions.
3️⃣ Three-Address Code Generation

The compiler converts expressions into intermediate Three-Address Code (TAC) using temporary variables.

For example:

a = 2 * 3 + 4;

can generate:

t1 = 2 * 3
t2 = t1 + 4
a = t2

TAC provides an intermediate representation between parsing and code generation.

4️⃣ Code Optimization

The optimizer performs three optimization techniques.

Constant Folding

Numeric expressions are evaluated during compilation.

2 * 3

can be reduced to:

6
Common Subexpression Elimination

Previously calculated expressions can be reused instead of being calculated again.

Dead Code Elimination

Temporary calculations whose results are never used are removed from the optimized output.

5️⃣ Assembly Code Generation

The final stage converts optimized TAC instructions into x86 NASM-style assembly.

The generator uses registers including:

EAX
EBX

and generates instructions such as:

MOV
ADD
SUB
IMUL
IDIV

The generated assembly also includes .data, .bss, and .text sections.

🧪 Built-in Test Programs

The compiler currently includes five test programs:

a=2*3+4; print a;

a=2*3; b=2*3; print a; print b;

x=10+5; y=x*2; z=y-3; print z;

a=(2+3)*4; print a;

a=4*5; b=a+3; c=b*2; print c;

Each test program is processed through the compiler pipeline.

🛠️ Technologies
C99
GCC
Compiler Design
Lexical Analysis
Recursive-Descent Parsing
Three-Address Code
Code Optimization
DAG-based Optimization
x86 Assembly
▶️ How to Run
Prerequisites

Install GCC.

Check whether GCC is installed:

gcc --version
Linux / macOS

Compile:

gcc mini_compiler.c -o mini_compiler

Run:

./mini_compiler
Windows

If using MinGW GCC:

Compile:

gcc mini_compiler.c -o mini_compiler.exe

Run:

mini_compiler.exe
📤 Expected Output

When the program runs, it displays the different stages of the compiler.

Example structure:

==============================================
  TEST PROGRAM 1 : a=2*3+4; print a;
==============================================

--- LEXER OUTPUT (Module 1) ---

--- PARSER OUTPUT (Module 2 & 3) ---

--- THREE ADDRESS CODE (Module 4) ---

--- OPTIMIZED CODE (Module 5 - DAG Optimizer) ---

--- ASSEMBLY OUTPUT (Module 6) ---

The program processes all five built-in test cases.

🎯 Learning Objectives

This project demonstrates the implementation and integration of major compiler concepts:

Source Code
     ↓
Lexical Analysis
     ↓
Parsing
     ↓
Intermediate Representation
     ↓
Optimization
     ↓
Target Code Generation

It was developed to understand how a compiler transforms source code into lower-level representations and optimized target instructions.

👨‍💻 Author
Vivek Kumar Gupta

B.Tech Computer Science Engineering
IILM University, Greater Noida
```text

