#include "visitor.h"
#include "astNodes.h"
#include <string>

void TreePrintVisitor::visit(Program& node) {
    outFile << "Program\n";
    indent++;
    for (const auto& decl : node.decls) {
        if (decl) {
            decl->accept(*this);
        }
    }
    indent--;
}

void TreePrintVisitor::visit(RoleDecl& node) {
    printIndent();
    outFile << "RoleDecl: " << node.id;
    if (node.size > 0) {
        outFile << "[" << node.size << "]";
    }
    outFile << "\n";
}

void TreePrintVisitor::visit(TaskDecl& node) {
    printIndent();
    outFile << "TaskDecl: " << node.id << "\n";
    
    indent++;
    
    printIndent();
    outFile << "Target:\n";
    indent++;
    node.target->accept(*this);
    indent--;
    
    printIndent();
    outFile << "Body:\n";
    indent++;
    for (const auto& stmt : node.stmts) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(TypeDecl& node) {
    printIndent();
    outFile << "TypeDecl: " << node.id << "\n";
    indent++;
    if (node.type) {
        node.type->accept(*this);
    }
    indent--;
}

void TreePrintVisitor::visit(RoleTarget& node) {
    printIndent();
    outFile << "RoleTarget: " << node.id;
    if (node.index) {
        outFile << "[";
        node.index->accept(*this);
        if (node.range_end) {
            outFile << "..";
            node.range_end->accept(*this);
        }
        outFile << "]";
    }
    outFile << "\n";
}

void TreePrintVisitor::visit(VarDecl& node) {
    printIndent();
    outFile << "VarDecl: " << node.id << "\n";
    indent++;
    
    printIndent();
    outFile << "Type:\n";
    indent++;
    if (node.type) {
        node.type->accept(*this);
    }
    indent--;
    
    if (node.expr) {
        printIndent();
        outFile << "Initial Value:\n";
        indent++;
        node.expr->accept(*this);
        indent--;
    }
    
    indent--;
}

void TreePrintVisitor::visit(AssignStmt& node) {
    printIndent();
    outFile << "AssignStmt:\n";
    indent++;
    
    printIndent();
    outFile << "LValue:\n";
    indent++;
    if (node.lvalue) {
        node.lvalue->accept(*this);
    }
    indent--;
    
    printIndent();
    outFile << "RValue:\n";
    indent++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(PrintStmt& node) {
    printIndent();
    outFile << "PrintStmt:\n";
    indent++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent--;
}

void TreePrintVisitor::visit(IfStmt& node) {
    printIndent();
    outFile << "IfStmt:\n";
    indent++;
    
    printIndent();
    outFile << "Condition:\n";
    indent++;
    if (node.condition) {
        node.condition->accept(*this);
    }
    indent--;
    
    printIndent();
    outFile << "Then Branch:\n";
    indent++;
    for (const auto& stmt : node.if_body) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
    indent--;
    
    if (!node.else_body.empty()) {
        printIndent();
        outFile << "Else Branch:\n";
        indent++;
        for (const auto& stmt : node.else_body) {
            if (stmt) {
                stmt->accept(*this);
            }
        }
        indent--;
    }
    
    indent--;
}

void TreePrintVisitor::visit(ExprStmt& node) {
    printIndent();
    outFile << "ExprStmt:\n";
    indent++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent--;
}

void TreePrintVisitor::visit(FunctionCall& node) {
    printIndent();
    outFile << "FunctionCall: " << node.id << "()\n";
    indent++;
    
    if (!node.args.empty()) {
        printIndent();
        outFile << "Arguments:\n";
        indent++;
        for (const auto& arg : node.args) {
            if (arg) {
                arg->accept(*this);
            }
        }
        indent--;
    }
    
    indent--;
}

void TreePrintVisitor::visit(BroadcastStmt& node) {
    printIndent();
    outFile << "BroadcastStmt:\n";
    indent++;
    
    printIndent();
    outFile << "Value:\n";
    indent++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent--;
    
    printIndent();
    outFile << "Targets:\n";
    indent++;
    for (const auto& target : node.targets) {
        if (target) {
            target->accept(*this);
        }
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(SendStmt& node) {
    printIndent();
    outFile << "SendStmt:\n";
    indent++;
    
    printIndent();
    outFile << "Value:\n";
    indent++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent--;
    
    printIndent();
    outFile << "Target:\n";
    indent++;
    if (node.target) {
        node.target->accept(*this);
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(RecvStmt& node) {
    printIndent();
    outFile << "RecvStmt:\n";
    indent++;
    
    printIndent();
    outFile << "Variable:\n";
    indent++;
    if (node.expr) {
        node.expr->accept(*this);
    }
    indent--;
    
    printIndent();
    outFile << "From:\n";
    indent++;
    if (node.from) {
        node.from->accept(*this);
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(GatherStmt& node) {
    printIndent();
    outFile << "GatherStmt:\n";
    indent++;
    
    printIndent();
    outFile << "From:\n";
    indent++;
    for (const auto& target : node.from_targets) {
        if (target) {
            target->accept(*this);
        }
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(SpawnStmt& node) {
    printIndent();
    outFile << "SpawnStmt: " << node.task_id << "\n";
    indent++;
    
    printIndent();
    outFile << "On:\n";
    indent++;
    for (const auto& target : node.on_targets) {
        if (target) {
            target->accept(*this);
        }
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(IntLiteral& node) {
    printIndent();
    outFile << "IntLiteral: " << node.value << "\n";
}

void TreePrintVisitor::visit(FloatLiteral& node) {
    printIndent();
    outFile << "FloatLiteral: " << node.value << "\n";
}

void TreePrintVisitor::visit(StringLiteral& node) {
    printIndent();
    outFile << "StringLiteral: \"" << node.value << "\"\n";
}

void TreePrintVisitor::visit(BoolLiteral& node) {
    printIndent();
    outFile << "BoolLiteral: " << (node.value ? "true" : "false") << "\n";
}

void TreePrintVisitor::visit(Identifier& node) {
    printIndent();
    outFile << "Identifier: " << node.id << "\n";
}

void TreePrintVisitor::visit(BinaryOp& node) {
    printIndent();
    outFile << "BinaryOp: " << node.op << "\n";
    indent++;
    
    printIndent();
    outFile << "Left:\n";
    indent++;
    if (node.left) {
        node.left->accept(*this);
    }
    indent--;
    
    printIndent();
    outFile << "Right:\n";
    indent++;
    if (node.right) {
        node.right->accept(*this);
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(PrimitiveType& node) {
    printIndent();
    outFile << "PrimitiveType: " << node.type_name << "\n";
}

void TreePrintVisitor::visit(TensorType& node) {
    printIndent();
    outFile << "TensorType: [";
    for (size_t i = 0; i < node.dims.size(); ++i) {
        outFile << node.dims[i];
        if (i < node.dims.size() - 1) {
            outFile << ", ";
        }
    }
    outFile << "]\n";
    indent++;
    
    printIndent();
    outFile << "Element Type:\n";
    indent++;
    if (node.element_type) {
        node.element_type->accept(*this);
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(ListType& node) {
    printIndent();
    outFile << "ListType:\n";
    indent++;
    
    printIndent();
    outFile << "Element Type:\n";
    indent++;
    if (node.element_type) {
        node.element_type->accept(*this);
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(FieldDecl& node) {
    printIndent();
    outFile << "FieldDecl: " << node.id << "\n";
    indent++;
    
    printIndent();
    outFile << "Type:\n";
    indent++;
    if (node.type) {
        node.type->accept(*this);
    }
    indent--;
    
    indent--;
}

void TreePrintVisitor::visit(RecordType& node) {
    printIndent();
    outFile << "RecordType:\n";
    indent++;
    
    for (const auto& field : node.fields) {
        if (field) {
            field->accept(*this);
        }
    }
    
    indent--;
}
