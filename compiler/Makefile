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

AST_SRCS = AST/ast.cpp AST/astNodes.cpp AST/treePrintVisitor.cpp AST/codeGenerator.cpp
AST_OBJS = ast.o astNodes.o treePrintVisitor.o codeGenerator.o

SEMANTIC_SRCS = SymbolTable/symbolTable.cpp AST/semanticChecks.cpp
SEMANTIC_OBJS = $(SEMANTIC_SRCS:.cpp=.o)

# Object files
OBJS = parser.tab.o lex.yy.o $(AST_OBJS)

# Test Input file (take as command line argument)
input = simple_test.jade

# Default target
all: $(TARGET)

# Build the compiler
$(TARGET): $(OBJS) $(AST_OBJS) $(SEMANTIC_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	rm -f $(OBJS) $(AST_OBJS) $(SEMANTIC_OBJS) parser.tab.c parser.tab.h lex.yy.c ast.o	

# Generate parser files
$(PARSER_OUT) $(PARSER_HDR): $(PARSER_SRC) parserTypes.h
	$(YACC) -d $(PARSER_SRC)

# Generate lexer file
$(LEXER_OUT): $(LEXER_SRC) $(PARSER_HDR) parserTypes.h
	$(LEX) -o $(LEXER_OUT) $(LEXER_SRC)

# Compile object files
parser.tab.o: $(PARSER_OUT) $(PARSER_HDR) parserTypes.h AST/ast.h AST/astNodes.h AST/visitor.h
	$(CXX) $(CXXFLAGS) -c -o parser.tab.o $(PARSER_OUT)

lex.yy.o: $(LEXER_OUT) $(PARSER_HDR) parserTypes.h
	$(CXX) $(CXXFLAGS) -c -o lex.yy.o $(LEXER_OUT)

ast.o: AST/ast.cpp AST/ast.h
	$(CXX) $(CXXFLAGS) -c -o ast.o AST/ast.cpp

astNodes.o: AST/astNodes.cpp AST/astNodes.h AST/ast.h AST/visitor.h
	$(CXX) $(CXXFLAGS) -c -o astNodes.o AST/astNodes.cpp

treePrintVisitor.o: AST/treePrintVisitor.cpp AST/visitor.h AST/astNodes.h
	$(CXX) $(CXXFLAGS) -c -o treePrintVisitor.o AST/treePrintVisitor.cpp

codeGenerator.o: AST/codeGenerator.cpp AST/codeGenerator.h AST/ast.h AST/visitor.h AST/astNodes.h
	$(CXX) $(CXXFLAGS) -c -o codeGenerator.o AST/codeGenerator.cpp

SymbolTable/%.o: SymbolTable/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(TARGET) AST.tree jade.yy.cpp

# Test with sample programs
test: $(TARGET)
	rm -f $(OBJS) $(AST_OBJS) $(SEMANTIC_OBJS) parser.tab.c parser.tab.h lex.yy.c ast.o
# 	@echo "Testing basic program..."
	./$(TARGET) tests/$(input).jade
	@echo "Testing role declarations..."
# 	./$(TARGET) tests/test_roles.jade
# 	@echo "Testing task with communication..."
# 	./$(TARGET) tests/test_comm.jade

.PHONY: all clean test