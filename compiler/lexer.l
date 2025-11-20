%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tokens.h"
#include "parserTypes.h"

extern YYSTYPE yylval;
extern int yylineno;

#ifdef YY_INITIAL_VALUE
#undef YY_INITIAL_VALUE
#endif
%}

%option yylineno
%option nounput

DIGIT       [0-9]
ID          [a-zA-Z_][a-zA-Z0-9_]*

%x CPP_BLOCK

%%
[ \t\n]+    { /* Ignore whitespace */ }
\$.*        { /* Ignore comments starting with $ */ }

"role"      { return T_ROLE; }
"task"      { return T_TASK; }
"on"        { return T_ON; }
"if"        { return T_IF; }
"else"      { return T_ELSE; }
"print"     { return T_PRINT; }

"int"       { return T_INT; }
"float"     { return T_FLOAT; }
"bool"      { return T_BOOL; }
"string"    { return T_STRING; }

"tensor"    { return T_TENSOR; }
"list"      { return T_LIST; }
"record"    { return T_RECORD; }

"broadcast" { return T_BROADCAST; }
"send"      { return T_SEND; }
"recv"      { return T_RECV; }
"gather"    { return T_GATHER; }
"from"      { return T_FROM; }
"spawn"     { return T_SPAWN; }
"to"        { return T_TO; }

"cpp"       { BEGIN(CPP_BLOCK); }

".."        { return T_RANGE; }

<CPP_BLOCK>[ \t\n]+ { /* Ignore whitespace */ }

<CPP_BLOCK>"{{" {
    int braces = 2;
    int capacity = 1024;
    int length = 0;
    char* code = (char*)malloc(capacity);
    
    int c;
    while((c = yyinput()) != EOF){
        if (length >= capacity-2){
            capacity *= 2;
            code = (char*)realloc(code,capacity);
        }
        code[length++] = c;

        if (c == '{') {
            braces++;
        }
        else if(c == '}') {
            braces--;
            if (braces == 0) {
                length--;
                if (length > 0 && code[length-1] == '}') {
                    length--;
                    break;
                }
                else {
                    braces = 1;
                }
            }
        }
        else if (c == '\n') {
            yylineno++;
        }
    }

    code[length] = '\0';
    yylval.sval = code;
    BEGIN(INITIAL);
    return T_CPP_BLOCK;
}

<CPP_BLOCK>. {
    fprintf(stderr, "Lexical error at line %d: Expected '{{' after 'cpp'\n", yylineno);
    BEGIN(INITIAL);
    return 0;
}

{DIGIT}+    { 
    yylval.ival = atoi(yytext);
    return T_INT_LITERAL; 
}

{DIGIT}+\.{DIGIT}+      { 
    yylval.fval = atof(yytext);
    return T_FLOAT_LITERAL;
}

\.{DIGIT}+              { 
    yylval.fval = atof(yytext);
    return T_FLOAT_LITERAL;
}

{DIGIT}+\.{DIGIT}*[eE][+-]?{DIGIT}+  { 
    yylval.fval = atof(yytext);
    return T_FLOAT_LITERAL;
}

\.{DIGIT}+[eE][+-]?{DIGIT}+  { 
    yylval.fval = atof(yytext);
    return T_FLOAT_LITERAL;
}

{DIGIT}+[eE][+-]?{DIGIT}+  { 
    yylval.fval = atof(yytext);
    return T_FLOAT_LITERAL;
}

\"([^"\\]|\\.)*\" {
    char* str = strdup(yytext + 1);
    str[strlen(str) - 1] = '\0';
    yylval.sval = str;
    return T_STRING_LITERAL;
}

\"([^"\\]|\\.)* {
    fprintf(stderr, "Lexical error at line %d: Unterminated string literal\n", yylineno);
    return 0;
}

"true"      { yylval.ival = 1; return T_BOOL_LITERAL; }
"false"     { yylval.ival = 0; return T_BOOL_LITERAL; }

{ID}        { 
    yylval.sval = strdup(yytext);
    return T_ID; 
}

"+"         { return T_PLUS; }
"-"         { return T_MINUS; }
"*"         { return T_MUL; }
"/"         { return T_DIV; }
"%"         { return T_MOD; }
"&&"        { return T_AND; }
"||"        { return T_OR; }
"=="        { return T_EQ; }
"!="        { return T_NEQ; }
"<"         { return T_LT; }
">"         { return T_GT; }
"<="        { return T_LTE; }
">="        { return T_GTE; }
"="         { return T_ASSIGN; }
";"         { return ';'; }
":"         { return ':'; }
","         { return ','; }
"["         { return '['; }
"]"         { return ']'; }
"{"         { return '{'; }
"}"         { return '}'; }
"("         { return '('; }
")"         { return ')'; }

.           { 
    fprintf(stderr, "Lexical error at line %d: Unrecognized character '%s'\n", yylineno, yytext); 
    return 0;
}
%%

int yywrap() {
    return 1;
}