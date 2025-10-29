#ifndef VISITOR_H
#define VISITOR_H

#include <iostream>
#include <fstream>
#include <string>

// Forward declarations
class Program;
class RoleDecl;
class TaskDecl;
class TypeDecl;
class RoleTarget;
class VarDecl;
class AssignStmt;
class PrintStmt;
class IfStmt;
class ExprStmt;
class FunctionCall;
class BroadcastStmt;
class SendStmt;
class RecvStmt;
class GatherStmt;
class SpawnStmt;
class IntLiteral;
class FloatLiteral;
class StringLiteral;
class BoolLiteral;
class Identifier;
class BinaryOp;
class PrimitiveType;
class TensorType;
class ListType;
class RecordType;
class FieldDecl;

#include "ast.h"

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(Program &node) = 0;
    virtual void visit(RoleDecl &node) = 0;
    virtual void visit(TaskDecl &node) = 0;
    virtual void visit(TypeDecl &node) = 0;
    virtual void visit(RoleTarget &node) = 0;
    virtual void visit(VarDecl &node) = 0;
    virtual void visit(AssignStmt &node) = 0;
    virtual void visit(PrintStmt &node) = 0;
    virtual void visit(IfStmt &node) = 0;
    virtual void visit(ExprStmt &node) = 0;
    virtual void visit(FunctionCall &node) = 0;
    virtual void visit(BroadcastStmt &node) = 0;
    virtual void visit(SendStmt &node) = 0;
    virtual void visit(RecvStmt &node) = 0;
    virtual void visit(GatherStmt &node) = 0;
    virtual void visit(SpawnStmt &node) = 0;
    virtual void visit(IntLiteral &node) = 0;
    virtual void visit(FloatLiteral &node) = 0;
    virtual void visit(StringLiteral &node) = 0;
    virtual void visit(BoolLiteral &node) = 0;
    virtual void visit(Identifier &node) = 0;
    virtual void visit(BinaryOp &node) = 0;
    virtual void visit(PrimitiveType &node) = 0;
    virtual void visit(TensorType &node) = 0;
    virtual void visit(ListType &node) = 0;
    virtual void visit(FieldDecl &node) = 0;
    virtual void visit(RecordType &node) = 0;
};

// Tree print visitor for visualization
class TreePrintVisitor : public Visitor {
private:
    std::ofstream outFile;
    int indent = 0;

    void printIndent() {
        for (int i = 0; i < indent; i++) {
            outFile << "  ";
        }
    }

public:
    TreePrintVisitor(const std::string& filename) {
        outFile.open(filename);
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open file " << filename << " for writing\n";
        }
    }

    ~TreePrintVisitor() {
        if (outFile.is_open()) {
            outFile.close();
        }
    }

    void visit(Program &node) override;
    void visit(RoleDecl &node) override;
    void visit(TaskDecl &node) override;
    void visit(TypeDecl &node) override;
    void visit(RoleTarget &node) override;
    void visit(VarDecl &node) override;
    void visit(AssignStmt &node) override;
    void visit(PrintStmt &node) override;
    void visit(IfStmt &node) override;
    void visit(ExprStmt &node) override;
    void visit(FunctionCall &node) override;
    void visit(BroadcastStmt &node) override;
    void visit(SendStmt &node) override;
    void visit(RecvStmt &node) override;
    void visit(GatherStmt &node) override;
    void visit(SpawnStmt &node) override;
    void visit(IntLiteral &node) override;
    void visit(FloatLiteral &node) override;
    void visit(StringLiteral &node) override;
    void visit(BoolLiteral &node) override;
    void visit(Identifier &node) override;
    void visit(BinaryOp &node) override;
    void visit(PrimitiveType &node) override;
    void visit(TensorType &node) override;
    void visit(ListType &node) override;
    void visit(FieldDecl &node) override;
    void visit(RecordType &node) override;
    void print_indent();
};

#endif // VISITOR_H