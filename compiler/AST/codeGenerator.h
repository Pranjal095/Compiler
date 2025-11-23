#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "visitor.h"
#include "ast.h"
#include "astNodes.h"
#include <sstream>
#include <map>
#include <string>
#include <vector>
#include <set>

class CodeGenerator : public Visitor {
private:
    std::stringstream buffer;
    int indentLevel = 0;
    std::map<std::string, int> roleMap;
    std::map<std::string, int> roleSizes;
    int totalRanks = 0;
    std::set<std::string> spawnedRoles;
    void indent();
    void generate(const std::string& str);
    void generateLine(const std::string& str);
    std::string getCppType(Type* type);
    std::string getMpiType(Type* type);

public:
    CodeGenerator();
    ~CodeGenerator() = default;

    std::string getCode() const;
    void visit(Program &node) override;
    void visit(RoleDecl &node) override;
    void visit(TaskDecl &node) override;
    void visit(TypeDecl &node) override;
    void visit(CppCodeDecl &node) override;
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
};

#endif