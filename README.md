# Jade Compiler Project

## Overview
This project implements a compiler for the Jade programming language, designed for distributed systems. It supports role-based programming, task definitions, and communication primitives, targeting MPI (Message Passing Interface) for execution.

## Key Features
- **Jade Language Support**:
  - **Roles**: Define distinct roles for distributed nodes.
  - **Tasks**: Encapsulate logic within tasks assigned to roles.
  - **Communication**: Built-in `send` and `recv` primitives for inter-node communication.
  - **Inline C++**: Support for embedding C++ code directly within Jade source.
- **Compiler Architecture**:
  - **Lexer/Parser**: Built using Flex and Bison.
  - **AST**: Abstract Syntax Tree generation and traversal.
  - **Semantic Analysis**: Symbol table management and semantic checks.
  - **Static Analysis**: Deadlock detection using a Wait-For Graph to identify:
    - Circular wait conditions.
    - Send/Recv mismatches.
    - Gather/Spawn inconsistencies.
  - **Code Generation**: Transpiles Jade code into C++ with MPI calls.
- **Optimizations**:
  - **Topology Parsing**: Tools to parse and validate network topology files (`.top`).
  - **Expander Clustering**: MPI-based implementation for graph clustering and optimization.

## Current Status
- The compiler is functional and can parse, analyze, and generate C++ code for valid Jade programs.
- Basic test cases for roles, communication, and inline C++ are available.
- Optimization modules for topology processing are implemented.

## Dependencies
To build and run this project, you need the following installed on your system:
- **GCC/G++**: Support for C++17 or later.
- **OpenMPI**: `mpic++` and `mpirun` for distributed execution.
- **Flex**
- **Bison**
- **Make**

## Build Instructions

### Building the Compiler
Navigate to the `compiler` directory and run:
```bash
cd compiler
make
```
This will produce the `jade_compiler` executable.

### Running Tests
To run the basic test suite:
```bash
make test
```

### Building and Running Optimizations
To build the topology parser and clustering tools:
```bash
make run_optimizations
```
This command will:
1. Build `topology_parser` and `clustering_mpi`.
2. Process all `.top` files in the `tests` directory.
3. Validate topologies and run MPI clustering.
4. Output results to `output_topologies/`.

### Cleaning the Build
To remove generated files and executables:
```bash
make clean
```

## Running Examples

### Matrix Multiplication
```bash
cd compiler
./jade_compiler examples/matrix_mult.jade
mpic++ -o matrix_mult jade.yy.cpp
mpirun -np 5 ./matrix_mult
```

### Sudoku Validator
```bash
cd compiler
./jade_compiler examples/sudoku.jade
mpic++ -o sudoku jade.yy.cpp
mpirun -np 4 ./sudoku
```