#ifndef AST_NODES_H
#define AST_NODES_H

#include "ast.h"
#include "visitor.h"
#include <memory>
#include <vector>
#include <string>

class Program : public Node {
public:
    std::vector<std::unique_ptr<Decl>> decls;
    void accept(Visitor &v) override;
};

class RoleDecl : public Decl {
public:
    std::string id;
    int size = 1; // Default size is 1
    void accept(Visitor &v) override;
};

class RoleTarget : public Node {
public:
    std::string id;
    std::unique_ptr<Expr> index; // For ID[index]
    std::unique_ptr<Expr> range_end; // For ID[index..range_end]
    void accept(Visitor &v) override;
};

class TaskDecl : public Decl {
public:
    std::string id;
    std::unique_ptr<RoleTarget> target;
    std::vector<std::unique_ptr<Stmt>> stmts;
    void accept(Visitor &v) override;
};

class TypeDecl : public Decl {
public:
    std::string id;
    std::unique_ptr<Type> type;
    void accept(Visitor &v) override;
};

class VarDecl : public Stmt {
public:
    std::unique_ptr<Type> type;
    std::string id;
    std::unique_ptr<Expr> expr; // Can be nullptr if not initialized
    void accept(Visitor &v) override;
};

class AssignStmt : public Stmt {
public:
    std::unique_ptr<Identifier> lvalue;
    std::unique_ptr<Expr> expr;
    void accept(Visitor &v) override;
};

class PrintStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
    void accept(Visitor &v) override;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> if_body;
    std::vector<std::unique_ptr<Stmt>> else_body; // Can be empty
    void accept(Visitor &v) override;
};

// New class: Expression Statement (wraps an expression as a statement)
class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
    void accept(Visitor &v) override;
};

class FunctionCall : public Expr {
public:
    std::string id;
    std::vector<std::unique_ptr<Expr>> args;
    void accept(Visitor &v) override;
};

class BroadcastStmt : public CommStmt {
public:
    std::unique_ptr<Expr> expr;
    std::vector<std::unique_ptr<RoleTarget>> targets;
    void accept(Visitor &v) override;
};

class SendStmt : public CommStmt {
public:
    std::unique_ptr<Expr> expr;
    std::unique_ptr<RoleTarget> target;
    void accept(Visitor &v) override;
};

class RecvStmt : public CommStmt {
public:
    std::unique_ptr<Expr> expr; // Variable to receive into
    std::unique_ptr<RoleTarget> from;
    void accept(Visitor &v) override;
};

class GatherStmt : public CommStmt {
public:
    std::vector<std::unique_ptr<RoleTarget>> from_targets;
    void accept(Visitor &v) override;
};

class SpawnStmt : public CommStmt {
public:
    std::string task_id;
    std::vector<std::unique_ptr<RoleTarget>> on_targets;
    void accept(Visitor &v) override;
};

class IntLiteral : public Expr {
public:
    int value;
    void accept(Visitor &v) override;
};

class FloatLiteral : public Expr {
public:
    float value;
    void accept(Visitor &v) override;
};

class StringLiteral : public Expr {
public:
    std::string value;
    void accept(Visitor &v) override;
};

class BoolLiteral : public Expr {
public:
    bool value;
    void accept(Visitor &v) override;
};

class Identifier : public Expr {
public:
    std::string id;
    void accept(Visitor &v) override;
};

class BinaryOp : public Expr {
public:
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    void accept(Visitor &v) override;
};

class PrimitiveType : public Type {
public:
    std::string type_name; // "int", "float", "bool", "string"
    void accept(Visitor &v) override;
};

class TensorType : public Type {
public:
    std::unique_ptr<Type> element_type;
    std::vector<int> dims;
    void accept(Visitor &v) override;
};

class ListType : public Type {
public:
    std::unique_ptr<Type> element_type;
    void accept(Visitor &v) override;
};

class FieldDecl : public Node {
public:
    std::string id;
    std::unique_ptr<Type> type;
    void accept(Visitor &v) override;
};

class RecordType : public Type {
public:
    std::vector<std::unique_ptr<FieldDecl>> fields;
    void accept(Visitor &v) override;
};

#endif // AST_NODES_H