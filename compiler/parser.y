%{
#include <stdio.h>
#include <string.h>
#include <vector>
#include <memory>
#include "AST/ast.h"
#include "AST/ast_nodes.h"
#include "AST/visitor.h"
#include "parser_types.h"

// AST root
Program* ast_root = nullptr;

extern int yylineno;
extern char* yytext;
extern FILE* yyin;

int yylex(void);
void yyerror(const char *s);
void print_error(const char* msg, const char* token);
%}

/* Union for semantic values is now in parser_types.h */

/* Token declarations */
%token T_ROLE T_TASK T_ON T_IF T_ELSE T_PRINT
%token T_INT T_FLOAT T_BOOL T_STRING
%token T_TENSOR T_LIST T_RECORD
%token T_BROADCAST T_SEND T_RECV T_GATHER T_FROM T_SPAWN T_TO
%token <ival> T_INT_LITERAL
%token <ival> T_BOOL_LITERAL
%token <fval> T_FLOAT_LITERAL
%token <sval> T_STRING_LITERAL T_ID
%token T_AND T_OR T_EQ T_NEQ T_LT T_GT T_LTE T_GTE
%token T_ASSIGN T_RANGE

/* Non-terminal types */
%type <program_node> program
%type <decl_list> decl_list
%type <decl_node> decl type_alias_decl
%type <role_decl_node> role_decl
%type <task_decl_node> task_decl
%type <role_target_node> role_target
%type <role_targets> role_targets
%type <stmt_list> stmt_list
%type <stmt_node> stmt comm_stmt control_flow_stmt
%type <var_decl_node> var_decl
%type <assign_stmt_node> assign_stmt
%type <expr_stmt_node> expr_stmt
%type <expr_node> expr
%type <function_call_node> function_call
%type <arg_list> arg_list arg_list_nonempty
%type <type_node> type_decl tensor_type list_type record_type
%type <primitive_type_node> primitive_type
%type <tensor_dims> tensor_dims
%type <field_list> field_list
%type <field_decl_node> field_decl

/* Precedence and associativity */
%right T_ASSIGN
%left T_OR
%left T_AND
%left T_EQ T_NEQ
%left T_LT T_GT T_LTE T_GTE

%%

program
    : decl_list {
        $$ = new Program();
        $$->decls = std::move(*$1);
        delete $1;
        ast_root = $$;
    }
    ;

decl_list
    : decl_list decl {
        $1->push_back(std::unique_ptr<Decl>($2));
        $$ = $1;
    }
    | decl {
        $$ = new std::vector<std::unique_ptr<Decl>>();
        if($1) $$->push_back(std::unique_ptr<Decl>($1));
    }
    ;

decl
    : role_decl         { $$ = $1; }
    | task_decl         { $$ = $1; }
    | type_alias_decl   { $$ = $1; }
    | type_decl ';'     { /* Catches standalone types like tensor<int>[3]; and ignores them */ $$ = nullptr; }
    | error ';' { 
        print_error("Invalid declaration", yytext); 
        yyerrok; 
        $$ = nullptr;
    }
    ;

type_alias_decl
    : T_RECORD T_ID '{' field_list '}' ';' {
        auto rec_type = new RecordType();
        rec_type->fields = std::move(*$4);
        delete $4;
        auto type_decl_node = new TypeDecl();
        type_decl_node->id = $2;
        type_decl_node->type = std::unique_ptr<Type>(rec_type);
        free($2);
        $$ = type_decl_node;
    }
    | type_decl T_ID ';' {
        auto node = new TypeDecl();
        node->type = std::unique_ptr<Type>($1);
        node->id = $2;
        free($2);
        $$ = node;
    }
    ;

role_decl
    : T_ROLE T_ID ';' {
        $$ = new RoleDecl();
        $$->id = $2;
        free($2);
    }
    | T_ROLE T_ID '[' T_INT_LITERAL ']' ';' {
        $$ = new RoleDecl();
        $$->id = $2;
        $$->size = $4;
        free($2);
    }
    | T_ROLE error ';' { 
        print_error("Expected role name after 'role'", yytext); 
        yyerrok; 
        $$ = nullptr;
    }
    | T_ROLE T_ID error ';' { 
        print_error("Invalid role declaration syntax", yytext); 
        yyerrok; 
        $$ = nullptr;
    }
    ;

task_decl
    : T_TASK T_ID T_ON role_target '{' stmt_list '}' {
        $$ = new TaskDecl();
        $$->id = $2;
        $$->target = std::unique_ptr<RoleTarget>($4);
        $$->stmts = std::move(*$6);
        delete $6;
        free($2);
    }
    | T_TASK error '{' stmt_list '}' { 
        print_error("Invalid task declaration syntax", yytext); 
        yyerrok; 
        $$ = nullptr;
    }
    | T_TASK T_ID T_ON role_target '{' stmt_list error { 
        print_error("Missing '}' in task declaration", yytext); 
        yyerrok; 
        $$ = nullptr;
    }
    ;

role_target
    : T_ID {
        $$ = new RoleTarget();
        $$->id = $1;
        free($1);
    }
    | T_ID '[' T_INT_LITERAL ']' {
        $$ = new RoleTarget();
        $$->id = $1;
        auto index_lit = new IntLiteral();
        index_lit->value = $3;
        $$->index = std::unique_ptr<Expr>(index_lit);
        free($1);
    }
    | T_ID '[' T_INT_LITERAL T_RANGE T_INT_LITERAL ']' {
        $$ = new RoleTarget();
        $$->id = $1;
        auto index_lit = new IntLiteral();
        index_lit->value = $3;
        $$->index = std::unique_ptr<Expr>(index_lit);
        auto range_lit = new IntLiteral();
        range_lit->value = $5;
        $$->range_end = std::unique_ptr<Expr>(range_lit);
        free($1);
    }
    ;

stmt_list
    : stmt_list stmt {
        if ($2) $1->push_back(std::unique_ptr<Stmt>($2));
        $$ = $1;
    }
    | stmt {
        $$ = new std::vector<std::unique_ptr<Stmt>>();
        if ($1) $$->push_back(std::unique_ptr<Stmt>($1));
    }
    ;

stmt
    : var_decl          { $$ = $1; }
    | assign_stmt ';'   { $$ = $1; }
    | comm_stmt ';'     { $$ = $1; }
    | expr_stmt ';'     { $$ = $1; }
    | control_flow_stmt { $$ = $1; }
    | T_PRINT '(' expr ')' ';' {
        auto node = new PrintStmt();
        node->expr = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | error ';' { 
        print_error("Invalid statement", yytext); 
        yyerrok; 
        $$ = nullptr;
    }
    | T_PRINT error ';' { 
        print_error("Expected '(expression)' after 'print'", yytext); 
        yyerrok; 
        $$ = nullptr;
    }
    ;

expr_stmt
    : function_call {
        auto node = new ExprStmt();
        node->expr = std::unique_ptr<Expr>($1);
        $$ = node;
    }
    ;

var_decl
    : type_decl T_ID ';' {
        $$ = new VarDecl();
        $$->type = std::unique_ptr<Type>($1);
        $$->id = $2;
        free($2);
    }
    | type_decl T_ID T_ASSIGN expr ';' {
        $$ = new VarDecl();
        $$->type = std::unique_ptr<Type>($1);
        $$->id = $2;
        $$->expr = std::unique_ptr<Expr>($4);
        free($2);
    }
    ;

assign_stmt
    : T_ID T_ASSIGN expr {
        $$ = new AssignStmt();
        auto ident = new Identifier();
        ident->id = $1;
        $$->lvalue = std::unique_ptr<Identifier>(ident);
        $$->expr = std::unique_ptr<Expr>($3);
        free($1);
    }
    ;

expr
    : T_INT_LITERAL {
        auto node = new IntLiteral();
        node->value = $1;
        $$ = node;
    }
    | T_FLOAT_LITERAL {
        auto node = new FloatLiteral();
        node->value = $1;
        $$ = node;
    }
    | T_STRING_LITERAL {
        auto node = new StringLiteral();
        node->value = $1;
        free($1);
        $$ = node;
    }
    | T_BOOL_LITERAL {
        auto node = new BoolLiteral();
        node->value = ($1 != 0);
        $$ = node;
    }
    | T_ID {
        auto node = new Identifier();
        node->id = $1;
        free($1);
        $$ = node;
    }
    | function_call { $$ = $1; }
    | expr T_AND expr  { 
        auto node = new BinaryOp();
        node->op = "&&";
        node->left = std::unique_ptr<Expr>($1);
        node->right = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | expr T_OR expr   { 
        auto node = new BinaryOp();
        node->op = "||";
        node->left = std::unique_ptr<Expr>($1);
        node->right = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | expr T_EQ expr   { 
        auto node = new BinaryOp();
        node->op = "==";
        node->left = std::unique_ptr<Expr>($1);
        node->right = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | expr T_NEQ expr  { 
        auto node = new BinaryOp();
        node->op = "!=";
        node->left = std::unique_ptr<Expr>($1);
        node->right = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | expr T_LT expr   { 
        auto node = new BinaryOp();
        node->op = "<";
        node->left = std::unique_ptr<Expr>($1);
        node->right = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | expr T_GT expr   { 
        auto node = new BinaryOp();
        node->op = ">";
        node->left = std::unique_ptr<Expr>($1);
        node->right = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | expr T_LTE expr  { 
        auto node = new BinaryOp();
        node->op = "<=";
        node->left = std::unique_ptr<Expr>($1);
        node->right = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | expr T_GTE expr  { 
        auto node = new BinaryOp();
        node->op = ">=";
        node->left = std::unique_ptr<Expr>($1);
        node->right = std::unique_ptr<Expr>($3);
        $$ = node;
    }
    | '(' expr ')'     { $$ = $2; }
    | '(' error ')' { 
        print_error("Invalid expression in parentheses", yytext); 
        yyerrok; 
        $$ = nullptr;
    }
    ;

function_call
    : T_ID '(' arg_list ')' {
        $$ = new FunctionCall();
        $$->id = $1;
        if ($3) {
            $$->args = std::move(*$3);
            delete $3;
        }
        free($1);
    }
    ;

arg_list
    : /* empty */ { $$ = nullptr; }
    | arg_list_nonempty { $$ = $1; }
    ;

arg_list_nonempty
    : expr {
        $$ = new std::vector<std::unique_ptr<Expr>>();
        $$->push_back(std::unique_ptr<Expr>($1));
    }
    | arg_list_nonempty ',' expr {
        $1->push_back(std::unique_ptr<Expr>($3));
        $$ = $1;
    }
    ;

comm_stmt
    : T_BROADCAST expr T_TO role_targets {
        auto node = new BroadcastStmt();
        node->expr = std::unique_ptr<Expr>($2);
        node->targets = std::move(*$4);
        delete $4;
        $$ = node;
    }
    | T_SEND expr T_TO role_target {
        auto node = new SendStmt();
        node->expr = std::unique_ptr<Expr>($2);
        node->target = std::unique_ptr<RoleTarget>($4);
        $$ = node;
    }
    | T_RECV expr T_FROM role_target {
        auto node = new RecvStmt();
        node->expr = std::unique_ptr<Expr>($2);
        node->from = std::unique_ptr<RoleTarget>($4);
        $$ = node;
    }
    | T_GATHER T_FROM role_targets {
        auto node = new GatherStmt();
        node->from_targets = std::move(*$3);
        delete $3;
        $$ = node;
    }
    | T_SPAWN T_ID T_ON role_targets {
        auto node = new SpawnStmt();
        node->task_id = $2;
        node->on_targets = std::move(*$4);
        delete $4;
        free($2);
        $$ = node;
    }
    ;

role_targets
    : role_target {
        $$ = new std::vector<std::unique_ptr<RoleTarget>>();
        $$->push_back(std::unique_ptr<RoleTarget>($1));
    }
    | role_targets ',' role_target {
        $1->push_back(std::unique_ptr<RoleTarget>($3));
        $$ = $1;
    }
    ;

control_flow_stmt
    : T_IF '(' expr ')' '{' stmt_list '}' {
        auto node = new IfStmt();
        node->condition = std::unique_ptr<Expr>($3);
        node->if_body = std::move(*$6);
        delete $6;
        $$ = node;
    }
    | T_IF '(' expr ')' '{' stmt_list '}' T_ELSE '{' stmt_list '}' {
        auto node = new IfStmt();
        node->condition = std::unique_ptr<Expr>($3);
        node->if_body = std::move(*$6);
        node->else_body = std::move(*$10);
        delete $6;
        delete $10;
        $$ = node;
    }
    ;

type_decl
    : primitive_type { $$ = $1; }
    | tensor_type    { $$ = $1; }
    | list_type      { $$ = $1; }
    | record_type    { $$ = $1; }
    | T_ID { // This allows using a type alias like 'Person'
        auto node = new PrimitiveType();
        node->type_name = $1;
        free($1);
        $$ = node;
    }
    ;

primitive_type
    : T_INT    { 
        $$ = new PrimitiveType(); 
        $$->type_name = "int"; 
    }
    | T_FLOAT  { 
        $$ = new PrimitiveType(); 
        $$->type_name = "float"; 
    }
    | T_BOOL   { 
        $$ = new PrimitiveType(); 
        $$->type_name = "bool"; 
    }
    | T_STRING { 
        $$ = new PrimitiveType(); 
        $$->type_name = "string"; 
    }
    ;

tensor_type
    : T_TENSOR T_LT type_decl T_GT tensor_dims {
        auto node = new TensorType();
        node->element_type = std::unique_ptr<Type>($3);
        node->dims = *$5;
        delete $5;
        $$ = node;
    }
    ;

tensor_dims
    : tensor_dims '[' T_INT_LITERAL ']' {
        $1->push_back($3);
        $$ = $1;
    }
    | '[' T_INT_LITERAL ']' {
        $$ = new std::vector<int>();
        $$->push_back($2);
    }
    ;

list_type
    : T_LIST T_LT type_decl T_GT {
        auto node = new ListType();
        node->element_type = std::unique_ptr<Type>($3);
        $$ = node;
    }
    ;

record_type
    : T_RECORD '{' field_list '}' {
        auto node = new RecordType();
        node->fields = std::move(*$3);
        delete $3;
        $$ = node;
    }
    ;

field_list
    : field_list field_decl {
        $1->push_back(std::unique_ptr<FieldDecl>($2));
        $$ = $1;
    }
    | field_decl {
        $$ = new std::vector<std::unique_ptr<FieldDecl>>();
        $$->push_back(std::unique_ptr<FieldDecl>($1));
    }
    ;

field_decl
    : T_ID ':' type_decl ';' {
        $$ = new FieldDecl();
        $$->id = $1;
        $$->type = std::unique_ptr<Type>($3);
        free($1);
    }
    ;

%%

static char* current_filename = NULL;
static int error_count = 0;

void print_error(const char* msg, const char* token) {
    error_count++;
    if (current_filename) {
        fprintf(stderr, "Error in %s:%d: %s", current_filename, yylineno, msg);
    } else {
        fprintf(stderr, "Error at line %d: %s", yylineno, msg);
    }
    
    if (token && strlen(token) > 0) {
        fprintf(stderr, " near '%s'", token);
    }
    fprintf(stderr, "\n");
}

void yyerror(const char *s) {
    error_count++;
    if (current_filename) {
        fprintf(stderr, "Parse error in %s:%d: %s", current_filename, yylineno, s);
    } else {
        fprintf(stderr, "Parse error at line %d: %s", yylineno, s);
    }
    
    if (yytext && strlen(yytext) > 0) {
        fprintf(stderr, " near '%s'", yytext);
    }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    error_count = 0;
    
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            perror(argv[1]);
            return 1;
        }
        yyin = file;
        current_filename = argv[1];
        printf("Parsing file: %s\n", argv[1]);
    } else {
        current_filename = NULL;
        printf("Reading from stdin (press Ctrl+D to end):\n");
    }
    
    int result = yyparse();
    
    if (result == 0 && error_count == 0) {
        printf("✓ Parse successful! No errors found.\n");
        if (ast_root) {
            printf("AST root created successfully.\n");
            
            // Generate AST tree visualization
            TreePrintVisitor visitor("AST.tree");
            ast_root->accept(visitor);
            printf("AST tree generated in AST.tree\n");
            
            // TODO: Add semantic analysis and code generation here
            delete ast_root;
        }
    } else if (result == 0) {
        printf("⚠ Parse completed with %d error(s) recovered.\n", error_count);
    } else {
        printf("✗ Parse failed with %d error(s).\n", error_count);
    }
    
    if (argc > 1) {
        fclose(yyin);
    }
    
    return (error_count > 0) ? 1 : 0;
}