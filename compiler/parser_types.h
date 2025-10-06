#ifndef PARSER_TYPES_H
#define PARSER_TYPES_H

#include <vector>
#include <memory>
#include <string>

// Forward declare all AST node types to avoid circular dependencies
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
class FunctionCall;
class ControlFlowStmt;
class IfStmt;
class PrintStmt;
class ExprStmt;
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

// This union will be used by both the lexer and the parser
// for the semantic values of tokens and non-terminals.
typedef union YYSTYPE {
    int    ival;
    float  fval;
    char*  sval;
    // AST node pointers
    Program* program_node;
    Decl* decl_node;
    std::vector<std::unique_ptr<Decl>>* decl_list;
    RoleDecl* role_decl_node;
    TaskDecl* task_decl_node;
    RoleTarget* role_target_node;
    std::vector<std::unique_ptr<RoleTarget>>* role_targets;
    Stmt* stmt_node;
    std::vector<std::unique_ptr<Stmt>>* stmt_list;
    VarDecl* var_decl_node;
    AssignStmt* assign_stmt_node;
    CommStmt* comm_stmt_node;
    PrintStmt* print_stmt_node;
    IfStmt* if_stmt_node;
    ExprStmt* expr_stmt_node;
    Expr* expr_node;
    std::vector<std::unique_ptr<Expr>>* arg_list;
    FunctionCall* function_call_node;
    Type* type_node;
    PrimitiveType* primitive_type_node;
    FieldDecl* field_decl_node;
    std::vector<std::unique_ptr<FieldDecl>>* field_list;
    std::vector<int>* tensor_dims;
} YYSTYPE;

// Tell Flex and Bison that YYSTYPE is defined
#define YYSTYPE_IS_DECLARED 1

#endif // PARSER_TYPES_H