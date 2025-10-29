#include "semanticChecks.h"
#include <iostream>
#include <iomanip>

// SemanticAnalyzer Implementation
SemanticAnalyzer::SemanticAnalyzer() 
    : error_count(0), warning_count(0) {
    global_scope = std::make_unique<SymbolTable>(nullptr, "global");
    current_scope = global_scope.get();
}

SemanticAnalyzer::~SemanticAnalyzer() {
}

void SemanticAnalyzer::error(const std::string& msg, int line) {
    error_count++;
    if (line > 0) {
        std::cerr << "Semantic error at line " << line << ": " << msg << "\n";
    } else {
        std::cerr << "Semantic error: " << msg << "\n";
    }
}

void SemanticAnalyzer::warning(const std::string& msg, int line) {
    warning_count++;
    if (line > 0) {
        std::cerr << "Warning at line " << line << ": " << msg << "\n";
    } else {
        std::cerr << "Warning: " << msg << "\n";
    }
}

bool SemanticAnalyzer::analyze(Program* ast) {
    if (!ast) {
        error("NULL AST provided");
        return false;
    }
    
    std::cout << "\n=== Starting Semantic Analysis ===\n";
    analyze_program(ast);
    
    std::cout << "\n=== Semantic Analysis Complete ===\n";
    std::cout << "Errors: " << error_count << ", Warnings: " << warning_count << "\n";
    
    return error_count == 0;
}

std::string SemanticAnalyzer::get_type_name(Type* type) {
    if (!type) return "unknown";
    
    if (auto prim = dynamic_cast<PrimitiveType*>(type)) {
        return prim->type_name;
    } else if (auto tensor = dynamic_cast<TensorType*>(type)) {
        std::string base = "tensor<" + get_type_name(tensor->element_type.get()) + ">";
        for (int dim : tensor->dims) {
            base += "[" + std::to_string(dim) + "]";
        }
        return base;
    } else if (auto list = dynamic_cast<ListType*>(type)) {
        return "list<" + get_type_name(list->element_type.get()) + ">";
    } else if (dynamic_cast<RecordType*>(type)) {
        return "record";
    }
    
    return "unknown";
}

bool SemanticAnalyzer::types_compatible(const std::string& t1, const std::string& t2, bool strict) {
    if (t1 == t2) return true;
    if (t1 == "unknown" || t2 == "unknown") return true;
    
    if (strict) return false;
    
    // Allow numeric conversions
    if ((t1 == "int" || t1 == "float") && (t2 == "int" || t2 == "float")) {
        return true;
    }
    
    return false;
}

void SemanticAnalyzer::analyze_program(Program* prog) {
    for (auto& decl : prog->decls) {
        analyze_decl(decl.get());
    }
}

void SemanticAnalyzer::analyze_decl(Decl* decl) {
    if (!decl) return;
    
    if (auto role = dynamic_cast<RoleDecl*>(decl)) {
        analyze_role_decl(role);
    } else if (auto task = dynamic_cast<TaskDecl*>(decl)) {
        analyze_task_decl(task);
    } else if (auto type = dynamic_cast<TypeDecl*>(decl)) {
        analyze_type_decl(type);
    }
}

void SemanticAnalyzer::analyze_role_decl(RoleDecl* decl) {
    if (!current_scope->insert(decl->id, SYM_ROLE, "", 0)) {
        error("Role '" + decl->id + "' already declared");
        return;
    }
    
    auto sym = current_scope->lookup_current_scope(decl->id);
    if (sym) {
        sym->role_size = (decl->size > 0) ? decl->size : 1;
    }
}

void SemanticAnalyzer::analyze_task_decl(TaskDecl* decl) {
    if (!current_scope->insert(decl->id, SYM_TASK, "", 0)) {
        error("Task '" + decl->id + "' already declared");
        return;
    }
    
    analyze_role_target(decl->target.get());
    
    current_scope = current_scope->create_child_scope("task_" + decl->id);
    
    for (auto& stmt : decl->stmts) {
        analyze_stmt(stmt.get());
    }
    
    current_scope = current_scope->get_parent();
}

void SemanticAnalyzer::analyze_type_decl(TypeDecl* decl) {
    std::string type_name = get_type_name(decl->type.get());
    
    // Check if declaration has no identifier (incomplete declaration)
    if (decl->id.empty()) {
        error("Type declaration missing variable or type alias name");
        
        // Additional check for tensor type
        if (auto tensor = dynamic_cast<TensorType*>(decl->type.get())) {
            if (auto prim = dynamic_cast<PrimitiveType*>(tensor->element_type.get())) {
                auto type_sym = current_scope->lookup(prim->type_name);
                if (!type_sym && prim->type_name != "int" && prim->type_name != "float" && 
                    prim->type_name != "bool" && prim->type_name != "string") {
                    error("Undefined type '" + prim->type_name + "' in tensor declaration");
                }
            }
        }
        return;
    }
    
    // Check if it's an incomplete type (like tensor without variable name)
    if (auto tensor = dynamic_cast<TensorType*>(decl->type.get())) {
        // Check if element type exists
        std::string elem_type = get_type_name(tensor->element_type.get());
        if (auto prim = dynamic_cast<PrimitiveType*>(tensor->element_type.get())) {
            auto type_sym = current_scope->lookup(prim->type_name);
            if (!type_sym && prim->type_name != "int" && prim->type_name != "float" && 
                prim->type_name != "bool" && prim->type_name != "string") {
                error("Undefined type '" + prim->type_name + "' in tensor declaration");
            }
        }
    }
    
    if (!current_scope->insert(decl->id, SYM_TYPE, type_name, 0)) {
        error("Type '" + decl->id + "' already declared");
    }
}

void SemanticAnalyzer::analyze_stmt(Stmt* stmt) {
    if (!stmt) return;
    
    if (auto var_decl = dynamic_cast<VarDecl*>(stmt)) {
        analyze_var_decl(var_decl);
    } else if (auto assign = dynamic_cast<AssignStmt*>(stmt)) {
        analyze_assign_stmt(assign);
    } else if (auto expr_stmt = dynamic_cast<ExprStmt*>(stmt)) {
        analyze_expr_stmt(expr_stmt);
    } else if (auto if_stmt = dynamic_cast<IfStmt*>(stmt)) {
        analyze_if_stmt(if_stmt);
    } else if (auto print_stmt = dynamic_cast<PrintStmt*>(stmt)) {
        analyze_print_stmt(print_stmt);
    } else if (dynamic_cast<BroadcastStmt*>(stmt) || dynamic_cast<SendStmt*>(stmt) || 
               dynamic_cast<RecvStmt*>(stmt) || dynamic_cast<GatherStmt*>(stmt) ||
               dynamic_cast<SpawnStmt*>(stmt)) {
        analyze_comm_stmt(stmt);
    }
}

void SemanticAnalyzer::analyze_var_decl(VarDecl* decl) {
    std::string type_name = get_type_name(decl->type.get());
    
    // Check if type is defined (for user-defined types)
    if (auto prim = dynamic_cast<PrimitiveType*>(decl->type.get())) {
        if (prim->type_name != "int" && prim->type_name != "float" && 
            prim->type_name != "bool" && prim->type_name != "string") {
            auto type_sym = current_scope->lookup(prim->type_name);
            if (!type_sym || type_sym->kind != SYM_TYPE) {
                error("Undefined type '" + prim->type_name + "'");
                return;
            }
        }
    }
    
    if (!current_scope->insert(decl->id, SYM_VARIABLE, type_name, 0)) {
        error("Variable '" + decl->id + "' already declared in this scope");
        return;
    }
    
    if (decl->expr) {
        std::string expr_type = analyze_expr(decl->expr.get());
        
        if (!types_compatible(type_name, expr_type)) {
            error("Type mismatch in initialization of '" + decl->id + 
                  "': cannot convert " + expr_type + " to " + type_name);
        } else if (type_name != expr_type) {
            warning("Implicit conversion in initialization of '" + decl->id + 
                   "' from " + expr_type + " to " + type_name);
        }
        
        auto sym = current_scope->lookup_current_scope(decl->id);
        if (sym) {
            sym->is_initialized = true;
        }
    }
}

void SemanticAnalyzer::analyze_assign_stmt(AssignStmt* stmt) {
    auto sym = current_scope->lookup(stmt->lvalue->id);
    if (!sym) {
        error("Undeclared variable '" + stmt->lvalue->id + "'");
        return;
    }
    
    if (sym->kind != SYM_VARIABLE) {
        error("'" + stmt->lvalue->id + "' is not a variable");
        return;
    }
    
    std::string expr_type = analyze_expr(stmt->expr.get());
    
    if (!types_compatible(sym->type_name, expr_type)) {
        error("Type mismatch in assignment to '" + stmt->lvalue->id + 
              "': cannot convert " + expr_type + " to " + sym->type_name);
    } else if (sym->type_name != expr_type) {
        warning("Implicit conversion in assignment to '" + stmt->lvalue->id + 
               "' from " + expr_type + " to " + sym->type_name);
    }
    
    sym->is_initialized = true;
}

void SemanticAnalyzer::analyze_expr_stmt(ExprStmt* stmt) {
    analyze_expr(stmt->expr.get());
}

void SemanticAnalyzer::analyze_if_stmt(IfStmt* stmt) {
    std::string cond_type = analyze_expr(stmt->condition.get());
    if (cond_type != "bool" && cond_type != "int" && cond_type != "unknown") {
        warning("Condition should be boolean, got " + cond_type);
    }
    
    current_scope = current_scope->create_child_scope("if_body");
    for (auto& s : stmt->if_body) {
        analyze_stmt(s.get());
    }
    current_scope = current_scope->get_parent();
    
    if (!stmt->else_body.empty()) {
        current_scope = current_scope->create_child_scope("else_body");
        for (auto& s : stmt->else_body) {
            analyze_stmt(s.get());
        }
        current_scope = current_scope->get_parent();
    }
}

void SemanticAnalyzer::analyze_print_stmt(PrintStmt* stmt) {
    analyze_expr(stmt->expr.get());
}

void SemanticAnalyzer::analyze_comm_stmt(Stmt* stmt) {
    if (auto broadcast = dynamic_cast<BroadcastStmt*>(stmt)) {
        analyze_expr(broadcast->expr.get());
        for (auto& target : broadcast->targets) {
            analyze_role_target(target.get());
        }
    } else if (auto send = dynamic_cast<SendStmt*>(stmt)) {
        analyze_expr(send->expr.get());
        analyze_role_target(send->target.get());
    } else if (auto recv = dynamic_cast<RecvStmt*>(stmt)) {
        analyze_expr(recv->expr.get());
        analyze_role_target(recv->from.get());
    } else if (auto gather = dynamic_cast<GatherStmt*>(stmt)) {
        for (auto& target : gather->from_targets) {
            analyze_role_target(target.get());
        }
    } else if (auto spawn = dynamic_cast<SpawnStmt*>(stmt)) {
        auto task_sym = current_scope->lookup(spawn->task_id);
        if (!task_sym || task_sym->kind != SYM_TASK) {
            error("Undefined task '" + spawn->task_id + "'");
        }
        
        for (auto& target : spawn->on_targets) {
            analyze_role_target(target.get());
        }
    }
}

std::string SemanticAnalyzer::analyze_expr(Expr* expr) {
    if (!expr) return "unknown";
    
    if (auto int_lit = dynamic_cast<IntLiteral*>(expr)) {
        return "int";
    } else if (auto float_lit = dynamic_cast<FloatLiteral*>(expr)) {
        return "float";
    } else if (auto bool_lit = dynamic_cast<BoolLiteral*>(expr)) {
        return "bool";
    } else if (auto str_lit = dynamic_cast<StringLiteral*>(expr)) {
        return "string";
    } else if (auto ident = dynamic_cast<Identifier*>(expr)) {
        auto sym = current_scope->lookup(ident->id);
        if (!sym) {
            error("Undeclared identifier '" + ident->id + "'");
            return "unknown";
        }
        
        if (sym->kind == SYM_VARIABLE) {
            if (!sym->is_initialized) {
                warning("Variable '" + ident->id + "' may be used before initialization");
            }
            return sym->type_name;
        }
        return "unknown";
    } else if (auto binop = dynamic_cast<BinaryOp*>(expr)) {
        std::string left_type = analyze_expr(binop->left.get());
        std::string right_type = analyze_expr(binop->right.get());
        
        if (binop->op == "==" || binop->op == "!=" || binop->op == "<" || 
            binop->op == ">" || binop->op == "<=" || binop->op == ">=") {
            if (!types_compatible(left_type, right_type)) {
                warning("Comparing incompatible types: " + left_type + " " + binop->op + " " + right_type);
            }
            return "bool";
        }
        
        if (binop->op == "&&" || binop->op == "||") {
            return "bool";
        }
        
        if (!types_compatible(left_type, right_type)) {
            warning("Type mismatch in binary operation: " + left_type + " " + binop->op + " " + right_type);
        }
        
        if (left_type == "float" || right_type == "float") return "float";
        if (left_type == "int" || right_type == "int") return "int";
        return left_type;
    } else if (auto call = dynamic_cast<FunctionCall*>(expr)) {
        return analyze_function_call(call);
    }
    
    return "unknown";
}

std::string SemanticAnalyzer::analyze_function_call(FunctionCall* call) {
    for (auto& arg : call->args) {
        analyze_expr(arg.get());
    }
    return "unknown";
}

void SemanticAnalyzer::analyze_role_target(RoleTarget* target) {
    if (!target) return;
    
    auto sym = current_scope->lookup(target->id);
    if (!sym || sym->kind != SYM_ROLE) {
        error("Undefined role '" + target->id + "'");
        return;
    }
    
    if (target->index) {
        std::string index_type = analyze_expr(target->index.get());
        if (index_type != "int" && index_type != "unknown") {
            error("Role index must be an integer, got " + index_type);
        }
    }
    
    if (target->range_end) {
        std::string range_type = analyze_expr(target->range_end.get());
        if (range_type != "int" && range_type != "unknown") {
            error("Role range end must be an integer, got " + range_type);
        }
    }
}

void SemanticAnalyzer::print_symbol_table() const {
    std::cout << "\n=== Symbol Table ===\n";
    global_scope->print();
}