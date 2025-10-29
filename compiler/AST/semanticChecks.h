#ifndef SEMANTIC_CHECKS_H
#define SEMANTIC_CHECKS_H

#include "ast.h"
#include "astNodes.h"
#include "../SymbolTable/symbolTable.h"
#include <memory>
#include <string>

class SemanticAnalyzer {
private:
    std::unique_ptr<SymbolTable> global_scope;
    SymbolTable* current_scope;
    int error_count;
    int warning_count;
    
    void error(const std::string& msg, int line = 0);
    void warning(const std::string& msg, int line = 0);
    
    std::string get_type_name(Type* type);
    bool types_compatible(const std::string& t1, const std::string& t2, bool strict = false);
    
    void analyze_program(Program* prog);
    void analyze_decl(Decl* decl);
    void analyze_role_decl(RoleDecl* decl);
    void analyze_task_decl(TaskDecl* decl);
    void analyze_type_decl(TypeDecl* decl);
    void analyze_stmt(Stmt* stmt);
    void analyze_var_decl(VarDecl* decl);
    void analyze_assign_stmt(AssignStmt* stmt);
    void analyze_expr_stmt(ExprStmt* stmt);
    void analyze_if_stmt(IfStmt* stmt);
    void analyze_print_stmt(PrintStmt* stmt);
    void analyze_comm_stmt(Stmt* stmt);
    std::string analyze_expr(Expr* expr);
    std::string analyze_function_call(FunctionCall* call);
    void analyze_role_target(RoleTarget* target);

public:
    SemanticAnalyzer();
    ~SemanticAnalyzer();
    
    bool analyze(Program* ast);
    int get_error_count() const { return error_count; }
    int get_warning_count() const { return warning_count; }
    void print_symbol_table() const;
};

#endif

