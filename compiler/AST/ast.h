#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

// Forward declarations for all AST node classes
class Node;
class Program;
class Decl;
class RoleDecl;
class TaskDecl;
class TypeDecl;
class VarDecl;
class Stmt;
class AssignStmt;
class CommStmt;
class BroadcastStmt;
class SendStmt;
class RecvStmt;
class GatherStmt;
class SpawnStmt;
class PrintStmt;
class IfStmt;
class ExprStmt;
class FunctionCall;
class Expr;
class IntLiteral;
class FloatLiteral;
class StringLiteral;
class BoolLiteral;
class Identifier;
class BinaryOp;
class Type;
class PrimitiveType;
class TensorType;
class ListType;
class RecordType;
class FieldDecl;
class RoleTarget;
class Visitor;

// Base class for all AST nodes
class Node {
public:
    virtual ~Node();
    virtual void accept(Visitor &v) = 0;
};

// Base class for all declarations
class Decl : public Node {
public:
    virtual ~Decl();
};

// Base class for all statements
class Stmt : public Node {
public:
    virtual ~Stmt();
};

// Base class for all communication statements
class CommStmt : public Stmt {
public:
    virtual ~CommStmt();
};

// Base class for all expressions
class Expr : public Node {
public:
    virtual ~Expr();
};

// Base class for all type representations
class Type : public Node {
public:
    virtual ~Type();
};

#endif // AST_H