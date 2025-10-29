#include "astNodes.h"

void Program::accept(Visitor &v) { v.visit(*this); }
void RoleDecl::accept(Visitor &v) { v.visit(*this); }
void RoleTarget::accept(Visitor &v) { v.visit(*this); }
void TaskDecl::accept(Visitor &v) { v.visit(*this); }
void TypeDecl::accept(Visitor &v) { v.visit(*this); }  // Make sure this line exists
void VarDecl::accept(Visitor &v) { v.visit(*this); }
void AssignStmt::accept(Visitor &v) { v.visit(*this); }
void PrintStmt::accept(Visitor &v) { v.visit(*this); }
void IfStmt::accept(Visitor &v) { v.visit(*this); }
void ExprStmt::accept(Visitor &v) { v.visit(*this); }
void FunctionCall::accept(Visitor &v) { v.visit(*this); }
void BroadcastStmt::accept(Visitor &v) { v.visit(*this); }
void SendStmt::accept(Visitor &v) { v.visit(*this); }
void RecvStmt::accept(Visitor &v) { v.visit(*this); }
void GatherStmt::accept(Visitor &v) { v.visit(*this); }
void SpawnStmt::accept(Visitor &v) { v.visit(*this); }
void IntLiteral::accept(Visitor &v) { v.visit(*this); }
void FloatLiteral::accept(Visitor &v) { v.visit(*this); }
void StringLiteral::accept(Visitor &v) { v.visit(*this); }
void BoolLiteral::accept(Visitor &v) { v.visit(*this); }
void Identifier::accept(Visitor &v) { v.visit(*this); }
void BinaryOp::accept(Visitor &v) { v.visit(*this); }
void PrimitiveType::accept(Visitor &v) { v.visit(*this); }
void TensorType::accept(Visitor &v) { v.visit(*this); }
void ListType::accept(Visitor &v) { v.visit(*this); }
void FieldDecl::accept(Visitor &v) { v.visit(*this); }
void RecordType::accept(Visitor &v) { v.visit(*this); }