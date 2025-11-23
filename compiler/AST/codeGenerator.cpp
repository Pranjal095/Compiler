#include "codeGenerator.h"
#include <iostream>

CodeGenerator::CodeGenerator() {}

std::string CodeGenerator::getCode() const {
    return buffer.str();
}

void CodeGenerator::indent() {
    for (int i = 0; i < indentLevel; i++) {
        buffer << "    ";
    }
}

void CodeGenerator::generate(const std::string& string) {
    buffer << string;
}

void CodeGenerator::generateLine(const std::string& string) {
    indent();
    buffer << string << "\n";
}

std::string CodeGenerator::getCppType(Type* type) {
    if (auto primitiveType = dynamic_cast<PrimitiveType*>(type)) {
        if (primitiveType->type_name == "int") {
            return "int";
        }
        if (primitiveType->type_name == "float") {
            return "float";
        }
        if (primitiveType->type_name == "bool") {
            return "bool";
        }
        if (primitiveType->type_name == "string") {
            return "std::string";
        }
        return primitiveType->type_name;
    }
    else if (auto listType = dynamic_cast<ListType*>(type)) {
        return "std::vector<" + getCppType(listType->element_type.get()) + ">";
    }
    else if (auto tensorType = dynamic_cast<TensorType*>(type)) {
        std::string base = getCppType(tensorType->element_type.get());
        for (size_t i = 0; i < tensorType->dims.size(); i++) {
            base = "std::vector<" + base + ">";
        }
        return base;
    }
    return "auto";
}

std::string CodeGenerator::getMpiType(Type* type) {
    if (auto primitiveType = dynamic_cast<PrimitiveType*>(type)) {
        if (primitiveType->type_name == "int") {
            return "MPI_INT";
        }
        if (primitiveType->type_name == "float") {
            return "MPI_FLOAT";
        }
        if (primitiveType->type_name == "bool") {
            return "MPI_C_BOOL";
        }
        if (primitiveType->type_name == "string") {
            return "MPI_CHAR";
        }
    }
    return "MPI_BYTE";
}

void CodeGenerator::visit(Program &node) {
    generateLine("#include <mpi.h>");
    generateLine("#include <iostream>");
    generateLine("#include <vector>");
    generateLine("#include <string>");
    generateLine("#include <algorithm>");
    generateLine("");
    generateLine("// role definitions");
    int currentRank = 0;
    for (const auto& decl : node.decls) {
        if (auto roleDecl = dynamic_cast<RoleDecl*>(decl.get())) {
            roleMap[roleDecl->id] = currentRank;
            roleSizes[roleDecl->id] = roleDecl->size;
            currentRank += roleDecl->size;
        }
    }
    totalRanks = currentRank;
    generateLine("int worldRank;");
    generateLine("int worldSize;");
    generateLine("");
    for (const auto& decl : node.decls) {
        if (auto typeDecl = dynamic_cast<TypeDecl*>(decl.get())) {
            typeDecl->accept(*this);
        }
    }
    for (const auto& decl : node.decls) {
        if (auto taskDecl = dynamic_cast<TaskDecl*>(decl.get())) {
            taskDecl->accept(*this);
        }
        else if (auto cppDecl = dynamic_cast<CppCodeDecl*>(decl.get())) {
             cppDecl->accept(*this);
        }
    }

    generateLine("int main(int argc, char** argv) {");
    indentLevel++;
    generateLine("MPI_Init(&argc, &argv);");
    generateLine("MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);");
    generateLine("MPI_Comm_size(MPI_COMM_WORLD, &worldSize);");
    generateLine("");
    generateLine("if (worldSize < " + std::to_string(totalRanks) + ") {");
    indentLevel++;
    generateLine("if (worldRank == 0) std::cerr << \"Error: This program requires at least " + std::to_string(totalRanks) + " ranks.\" << std::endl;");
    generateLine("MPI_Abort(MPI_COMM_WORLD, 1);");
    indentLevel--;
    generateLine("}");
    generateLine("");
    spawnedRoles.clear();
    for (const auto& decl : node.decls) {
        if (auto taskDecl = dynamic_cast<TaskDecl*>(decl.get())) {
            std::vector<Stmt*> q;
            for (auto& s : taskDecl->stmts) {
                q.push_back(s.get());
            }
            
            size_t head = 0;
            while (head < q.size()) {
                Stmt* s = q[head++];
                if (auto spawn = dynamic_cast<SpawnStmt*>(s)) {
                    for (auto& t : spawn->on_targets) {
                        spawnedRoles.insert(t->id);
                    }
                }
                else if (auto ifStmt = dynamic_cast<IfStmt*>(s)) {
                    for (auto& stmt : ifStmt->if_body) {
                        q.push_back(stmt.get());
                    }
                    for (auto& stmt : ifStmt->else_body) {
                        q.push_back(stmt.get());
                    }
                }
            }
        }
    }

    bool first = true;
    for (const auto& decl : node.decls) {
        if (auto taskDecl = dynamic_cast<TaskDecl*>(decl.get())) {
            std::string roleName = taskDecl->target->id;
            int startRank = roleMap[roleName];
            int endRank = startRank + roleSizes[roleName];
            std::string condition = "worldRank >= " + std::to_string(startRank) + " && worldRank < " + std::to_string(endRank);
            indent();
            generate("if (" + condition + ") {\n");
            indentLevel++;
            
            bool isSpawned = spawnedRoles.count(roleName);
            if (isSpawned) {
                generateLine("// wait for start signal");
                generateLine("int start;");
                generateLine("MPI_Status status;");
                generateLine("MPI_Recv(&start, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);");
            }
            generateLine(taskDecl->id + "();");
            if (isSpawned) {
                generateLine("// send completion signal back to starter");
                generateLine("int complete = 1;");
                generateLine("MPI_Send(&complete, 1, MPI_INT, status.MPI_SOURCE, 2, MPI_COMM_WORLD);");
            }
            
            indentLevel--;
            generateLine("}");
        }
    }
    generateLine("");
    generateLine("MPI_Finalize();");
    generateLine("return 0;");
    indentLevel--;
    generateLine("}");
}

void CodeGenerator::visit(RoleDecl &node) {
}

void CodeGenerator::visit(TaskDecl &node) {
    generateLine("void " + node.id + "(){");
    indentLevel++;
    for (const auto& stmt : node.stmts) {
        stmt->accept(*this);
    }
    indentLevel--;
    generateLine("}");
    generateLine("");
}

void CodeGenerator::visit(TypeDecl &node) {
    if (auto recordType = dynamic_cast<RecordType*>(node.type.get())) {
        generateLine("struct " + node.id + " {");
        indentLevel++;
        for (const auto& field : recordType->fields) {
            std::string typeStr = getCppType(field->type.get());
            generateLine(typeStr + " " + field->id + ";");
        }
        indentLevel--;
        generateLine("};");
    }
    else{
        std::string typeStr = getCppType(node.type.get());
        generateLine("using " + node.id + " = " + typeStr + ";");
    }
    generateLine("");
}

void CodeGenerator::visit(CppCodeDecl &node) {
    generateLine(node.code);
    generateLine("");
}

void CodeGenerator::visit(RoleTarget &node) {
}

void CodeGenerator::visit(VarDecl &node) {
    indent();
    std::string typeStr = getCppType(node.type.get());
    generate(typeStr + " " + node.id);
    if (node.expr) {
        generate(" = ");
        node.expr->accept(*this);
    }
    generate(";\n");
}

void CodeGenerator::visit(AssignStmt &node) {
    indent();
    node.lvalue->accept(*this);
    generate(" = ");
    node.expr->accept(*this);
    generate(";\n");
}

void CodeGenerator::visit(PrintStmt &node) {
    indent();
    generate("std::cout << ");
    node.expr->accept(*this);
    generate(" << std::endl;\n");
}

void CodeGenerator::visit(IfStmt &node) {
    indent();
    generate("if (");
    node.condition->accept(*this);
    generate(") {\n");
    indentLevel++;
    for (const auto& stmt : node.if_body) {
        stmt->accept(*this);
    }
    indentLevel--;
    indent();
    generate("}");
    if (!node.else_body.empty()) {
        generate(" else {\n");
        indentLevel++;
        for (const auto& stmt : node.else_body) {
            stmt->accept(*this);
        }
        indentLevel--;
        indent();
        generate("}");
    }
    generate("\n");
}

void CodeGenerator::visit(ExprStmt &node) {
    indent();
    node.expr->accept(*this);
    generate(";\n");
}

void CodeGenerator::visit(FunctionCall &node) {
    generate(node.id + "(");
    for (size_t i = 0; i < node.args.size(); i++) {
        node.args[i]->accept(*this);
        if (i < node.args.size() - 1) {
            generate(", ");
        }
    }
    generate(")");
}

void CodeGenerator::visit(BroadcastStmt &node) {
    indent();
    generate("{\n");
    indentLevel++;
    
    for (const auto& target : node.targets) {
        std::string roleName = target->id;
        int baseRank = roleMap[roleName];
        int count = roleSizes[roleName];
        indent();
        generate("// broadcast to role " + roleName + "\n");
        indent();
        generate("for (int r = " + std::to_string(baseRank) + "; r < " + std::to_string(baseRank + count) + "; r++) {\n");
        indentLevel++;
        indent();
        generate("MPI_Send(&");
        node.expr->accept(*this);
        generate(", 1, MPI_INT, r, 0, MPI_COMM_WORLD);\n");
        indentLevel--;
        indent();
        generate("}\n");
    }
    indentLevel--;
    indent();
    generate("}\n");
}

void CodeGenerator::visit(SendStmt &node) {
    indent();
    generate("{\n");
    indentLevel++;
    indent();
    std::string roleName = node.target->id;
    int baseRank = roleMap[roleName];
    generate("int dest_rank = " + std::to_string(baseRank));
    if (node.target->index) {
        generate(" + ");
        node.target->index->accept(*this);
    }
    generate(";\n");
    indent();
    generate("MPI_Send(&");
    node.expr->accept(*this);
    generate(", 1, MPI_INT, dest_rank, 0, MPI_COMM_WORLD);\n");
    indentLevel--;
    indent();
    generate("}\n");
}

void CodeGenerator::visit(RecvStmt &node) {
    indent();
    generate("{\n");
    indentLevel++;
    indent();
    std::string roleName = node.from->id;
    int baseRank = roleMap[roleName];
    
    generate("int src_rank = " + std::to_string(baseRank));
    if (node.from->index) {
        generate(" + ");
        node.from->index->accept(*this);
    }
    generate(";\n");
    indent();
    generate("MPI_Status status;\n");
    indent();
    generate("MPI_Recv(&");
    node.expr->accept(*this);
    generate(", 1, MPI_INT, src_rank, 0, MPI_COMM_WORLD, &status);\n");
    indentLevel--;
    indent();
    generate("}\n");
}

void CodeGenerator::visit(GatherStmt &node) {
    indent();
    generate("{\n");
    indentLevel++;
    indent();
    generateLine("// gather: Wait for completion signal from targets");
    generateLine("int complete;");
    generateLine("MPI_Status status;");
    
    for (const auto& target : node.from_targets) {
        std::string roleName = target->id;
        int baseRank = roleMap[roleName];
        int count = roleSizes[roleName];
        indent();
        generate("// wait for role " + roleName + "\n");
        indent();
        generate("for (int r = " + std::to_string(baseRank) + "; r < " + std::to_string(baseRank + count) + "; r++) {\n");
        indentLevel++;
        indent();
        generateLine("MPI_Recv(&complete, 1, MPI_INT, r, 2, MPI_COMM_WORLD, &status);");
        indentLevel--;
        indent();
        generate("}\n");
    }
    indentLevel--;
    indent();
    generate("}\n");
}

void CodeGenerator::visit(SpawnStmt &node) {
    indent();
    generate("{\n");
    indentLevel++;
    indent();
    generateLine("// spawn: Send start signal to targets");
    generateLine("int start = 1;");
    
    for (const auto& target : node.on_targets) {
        std::string roleName = target->id;
        int baseRank = roleMap[roleName];
        int count = roleSizes[roleName];
        indent();
        generate("// trigger role " + roleName + "\n");
        indent();
        generate("for (int r = " + std::to_string(baseRank) + "; r < " + std::to_string(baseRank + count) + "; r++) {\n");
        indentLevel++;
        indent();
        generateLine("MPI_Send(&start, 1, MPI_INT, r, 1, MPI_COMM_WORLD);");
        indentLevel--;
        indent();
        generate("}\n");
    }
    indentLevel--;
    indent();
    generate("}\n");
}

void CodeGenerator::visit(IntLiteral &node) {
    generate(std::to_string(node.value));
}

void CodeGenerator::visit(FloatLiteral &node) {
    generate(std::to_string(node.value));
}

void CodeGenerator::visit(StringLiteral &node) {
    generate("\"" + node.value + "\"");
}

void CodeGenerator::visit(BoolLiteral &node) {
    generate(node.value ? "true" : "false");
}

void CodeGenerator::visit(Identifier &node) {
    generate(node.id);
}

void CodeGenerator::visit(BinaryOp &node) {
    generate("(");
    node.left->accept(*this);
    generate(" " + node.op + " ");
    node.right->accept(*this);
    generate(")");
}

void CodeGenerator::visit(PrimitiveType &node) {
}

void CodeGenerator::visit(TensorType &node) {
}

void CodeGenerator::visit(ListType &node) {
}

void CodeGenerator::visit(FieldDecl &node) {
}

void CodeGenerator::visit(RecordType &node) {
}