#include "visitor.h"
#include "ast_nodes.h"
#include <iostream>
#include <iomanip>
#include <fstream>

static int indent_level = 0;
static std::ofstream out;
TreePrintVisitor::TreePrintVisitor(const std::string& filename) {
    indent_level = 0;
    out.open(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing\n";
    }
}

TreePrintVisitor::~TreePrintVisitor() {
    if (out.is_open()) {
        out.close();
    }
}

void TreePrintVisitor::print_indent() {
    for (int i = 0; i < indent_level; i++) {
        out << "  ";
    }
}

void TreePrintVisitor::visit(Program& node) {
    out << "Program\n";
    indent_level++;
    for (auto& decl : node.decls) {
        if (decl) {
            decl->accept(*this);
        }
    }
    indent_level--;
}

void TreePrintVisitor::visit(RoleDecl& node) {
    print_indent();
    out << "RoleDecl: " << node.id;
    if (node.size > 0) {
        out << "[" << node.size << "]";
    } else {
        out << "[1]";
    }
    out << "\n";
}

void TreePrintVisitor::visit(TaskDecl& node) {
    print_indent();
    out << "TaskDecl: " << node.id << "\n";
    
    indent_level++;
    print_indent();
    out << "Target:\n";
    indent_level++;
    if (node.target) {
        node.target->accept(*this);
    }
    indent_level--;
    
    print_indent();
    out << "Body:\n";
    indent_level++;
    for (auto& stmt : node.stmts) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    indent_level -= 2;
}

void TreePrintVisitor::visit(TypeDecl& node) {
    print_indent();
    out << "TypeDecl: " << node.id << "\n";
    indent_level++;
    if (node.type) {
        node.type->accept(*this);
    }
    indent_level--;
}

void TreePrintVisitor::visit(VarDecl& node) {
    print_indent();
    out << "VarDecl: " << node.id << "\n";
    
    indent_level++;
    print_indent();
    out << "Type:\n";
    indent_level++;
    if (node.type) {
        node.type->accept(*this);
    }
    indent_level--;
    
    if (node.expr) {
        print_indent();
        out << "Initial Value:\n";
        indent_level++;
        node.expr->accept(*this);
        indent_level--;
    }
    indent_level--;
}

void TreePrintVisitor::visit(AssignStmt& node) {
    print_indent();
    out << "AssignStmt:\n";
    
    indent_level++;
    print_indent();
    out << "LValue:\n";
    indent_level++;
    if (node.lvalue) {
        node.lvalue->accept(*this);
    }
    indent_level--;
    
    print_indent();
    out << "RValue:\n";
    indent_level++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent_level -= 2;
}

void TreePrintVisitor::visit(ExprStmt& node) {
    print_indent();
    out << "ExprStmt:\n";
    indent_level++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent_level--;
}

void TreePrintVisitor::visit(IfStmt& node) {
    print_indent();
    out << "IfStmt:\n";
    
    indent_level++;
    print_indent();
    out << "Condition:\n";
    indent_level++;
    if (node.condition) {
        node.condition->accept(*this);
    }
    indent_level--;
    
    print_indent();
    out << "Then:\n";
    indent_level++;
    for (auto& stmt : node.if_body) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    indent_level--;
    
    if (!node.else_body.empty()) {
        print_indent();
        out << "Else:\n";
        indent_level++;
        for (auto& stmt : node.else_body) {
            if (stmt) {
                stmt->accept(*this);
            }
        }
        indent_level--;
    }
    indent_level--;
}

void TreePrintVisitor::visit(PrintStmt& node) {
    print_indent();
    out << "PrintStmt:\n";
    indent_level++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent_level--;
}

void TreePrintVisitor::visit(BroadcastStmt& node) {
    print_indent();
    out << "BroadcastStmt:\n";
    indent_level++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    print_indent();
    out << "Targets:\n";
    indent_level++;
    for (auto& target : node.targets) {
        if (target) {
            target->accept(*this);
        }
    }
    indent_level -= 2;
}

void TreePrintVisitor::visit(SendStmt& node) {
    print_indent();
    out << "SendStmt:\n";
    indent_level++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    print_indent();
    out << "To:\n";
    indent_level++;
    if (node.target) {
        node.target->accept(*this);
    }
    indent_level -= 2;
}

void TreePrintVisitor::visit(RecvStmt& node) {
    print_indent();
    out << "RecvStmt:\n";
    indent_level++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    print_indent();
    out << "From:\n";
    indent_level++;
    if (node.from) {
        node.from->accept(*this);
    }
    indent_level -= 2;
}

void TreePrintVisitor::visit(GatherStmt& node) {
    print_indent();
    out << "GatherStmt:\n";
    indent_level++;
    print_indent();
    out << "From:\n";
    indent_level++;
    for (auto& target : node.from_targets) {
        if (target) {
            target->accept(*this);
        }
    }
    indent_level -= 2;
}

void TreePrintVisitor::visit(SpawnStmt& node) {
    print_indent();
    out << "SpawnStmt: " << node.task_id << "\n";
    indent_level++;
    print_indent();
    out << "On:\n";
    indent_level++;
    for (auto& target : node.on_targets) {
        if (target) {
            target->accept(*this);
        }
    }
    indent_level -= 2;
}

void TreePrintVisitor::visit(IntLiteral& node) {
    print_indent();
    out << "IntLiteral: " << node.value << "\n";
}

void TreePrintVisitor::visit(FloatLiteral& node) {
    print_indent();
    out << "FloatLiteral: " << node.value << "\n";
}

void TreePrintVisitor::visit(BoolLiteral& node) {
    print_indent();
    out << "BoolLiteral: " << (node.value ? "true" : "false") << "\n";
}

void TreePrintVisitor::visit(StringLiteral& node) {
    print_indent();
    out << "StringLiteral: \"" << node.value << "\"\n";
}

void TreePrintVisitor::visit(Identifier& node) {
    print_indent();
    out << "Identifier: " << node.id << "\n";
}

void TreePrintVisitor::visit(BinaryOp& node) {
    print_indent();
    out << "BinaryOp: " << node.op << "\n";
    indent_level++;
    if (node.left) {
        node.left->accept(*this);
    }
    if (node.right) {
        node.right->accept(*this);
    }
    indent_level--;
}

void TreePrintVisitor::visit(FunctionCall& node) {
    print_indent();
    out << "FunctionCall: " << node.id << "\n";
    indent_level++;
    for (auto& arg : node.args) {
        if (arg) {
            arg->accept(*this);
        }
    }
    indent_level--;
}

void TreePrintVisitor::visit(RoleTarget& node) {
    print_indent();
    out << "RoleTarget: " << node.id;
    if (node.index) {
        out << "[";
        if (auto int_lit = dynamic_cast<IntLiteral*>(node.index.get())) {
            out << int_lit->value;
        }
        if (node.range_end) {
            out << "..";
            if (auto int_lit = dynamic_cast<IntLiteral*>(node.range_end.get())) {
                out << int_lit->value;
            }
        }
        out << "]";
    }
    out << "\n";
}

void TreePrintVisitor::visit(PrimitiveType& node) {
    print_indent();
    out << "PrimitiveType: " << node.type_name << "\n";
}

void TreePrintVisitor::visit(TensorType& node) {
    print_indent();
    out << "TensorType:\n";
    indent_level++;
    print_indent();
    out << "ElementType:\n";
    indent_level++;
    if (node.element_type) {
        node.element_type->accept(*this);
    }
    indent_level--;
    print_indent();
    out << "Dimensions: ";
    for (size_t i = 0; i < node.dims.size(); i++) {
        out << "[" << node.dims[i] << "]";
    }
    out << "\n";
    indent_level--;
}

void TreePrintVisitor::visit(ListType& node) {
    print_indent();
    out << "ListType:\n";
    indent_level++;
    if (node.element_type) {
        node.element_type->accept(*this);
    }
    indent_level--;
}

void TreePrintVisitor::visit(RecordType& node) {
    print_indent();
    out << "RecordType:\n";
    indent_level++;
    for (auto& field : node.fields) {
        if (field) {
            field->accept(*this);
        }
    }
    indent_level--;
}

void TreePrintVisitor::visit(FieldDecl& node) {
    this->print_indent();
    out << "FieldDecl: " << node.id << "\n";
    indent_level++;
    this->print_indent();
    out << "Type:\n";
    indent_level++;
    if (node.type) {
        node.type->accept(*this);
    }
    indent_level -= 2;
}
