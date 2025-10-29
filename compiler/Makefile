# Makefile for the Jade Language Compiler

CXX = g++
CXXFLAGS = -Wall -g -std=c++17 -I. -I./AST
LEX = flex
YACC = bison

# Target executable
TARGET = jade_compiler

# Source files
LEXER_SRC = lexer.l
PARSER_SRC = parser.y
LEXER_OUT = lex.yy.c
PARSER_OUT = parser.tab.c
PARSER_HDR = parser.tab.h

AST_SRCS = AST/ast.cpp AST/ast_nodes.cpp AST/tree_print_visitor.cpp
AST_OBJS = ast.o ast_nodes.o tree_print_visitor.o

SEMANTIC_SRCS = SymbolTable/symbol_table.cpp AST/semantic_checks.cpp
SEMANTIC_OBJS = $(SEMANTIC_SRCS:.cpp=.o)

# Object files
OBJS = parser.tab.o lex.yy.o $(AST_OBJS)

# Default target
all: $(TARGET)

# Build the compiler
$(TARGET): $(OBJS) $(AST_OBJS) $(SEMANTIC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generate parser files
$(PARSER_OUT) $(PARSER_HDR): $(PARSER_SRC) parser_types.h
	$(YACC) -d $(PARSER_SRC)

# Generate lexer file
$(LEXER_OUT): $(LEXER_SRC) $(PARSER_HDR) parser_types.h
	$(LEX) -o $(LEXER_OUT) $(LEXER_SRC)

# Compile object files
parser.tab.o: $(PARSER_OUT) $(PARSER_HDR) parser_types.h AST/ast.h AST/ast_nodes.h AST/visitor.h
	$(CXX) $(CXXFLAGS) -c -o parser.tab.o $(PARSER_OUT)

lex.yy.o: $(LEXER_OUT) $(PARSER_HDR) parser_types.h
	$(CXX) $(CXXFLAGS) -c -o lex.yy.o $(LEXER_OUT)

ast.o: AST/ast.cpp AST/ast.h
	$(CXX) $(CXXFLAGS) -c -o ast.o AST/ast.cpp

ast_nodes.o: AST/ast_nodes.cpp AST/ast_nodes.h AST/ast.h AST/visitor.h
	$(CXX) $(CXXFLAGS) -c -o ast_nodes.o AST/ast_nodes.cpp

tree_print_visitor.o: AST/tree_print_visitor.cpp AST/visitor.h AST/ast_nodes.h
	$(CXX) $(CXXFLAGS) -c -o tree_print_visitor.o AST/tree_print_visitor.cpp

SymbolTable/%.o: SymbolTable/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(TARGET) $(OBJS) $(AST_OBJS) $(SEMANTIC_OBJS) parser.tab.c parser.tab.h lex.yy.c AST.tree ast.o

# Test with sample programs
test: 
# 	@echo "Testing basic program..."
	./$(TARGET) tests/simple_test.jade
	@echo "Testing role declarations..."
# 	./$(TARGET) tests/test_roles.jade
# 	@echo "Testing task with communication..."
# 	./$(TARGET) tests/test_comm.jade

.PHONY: all clean test