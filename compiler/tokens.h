/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_TOKENS_H_INCLUDED
# define YY_YY_TOKENS_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    T_ROLE = 258,                  /* T_ROLE  */
    T_TASK = 259,                  /* T_TASK  */
    T_ON = 260,                    /* T_ON  */
    T_IF = 261,                    /* T_IF  */
    T_ELSE = 262,                  /* T_ELSE  */
    T_PRINT = 263,                 /* T_PRINT  */
    T_INT = 264,                   /* T_INT  */
    T_FLOAT = 265,                 /* T_FLOAT  */
    T_BOOL = 266,                  /* T_BOOL  */
    T_STRING = 267,                /* T_STRING  */
    T_TENSOR = 268,                /* T_TENSOR  */
    T_LIST = 269,                  /* T_LIST  */
    T_RECORD = 270,                /* T_RECORD  */
    T_BROADCAST = 271,             /* T_BROADCAST  */
    T_SEND = 272,                  /* T_SEND  */
    T_RECV = 273,                  /* T_RECV  */
    T_GATHER = 274,                /* T_GATHER  */
    T_FROM = 275,                  /* T_FROM  */
    T_SPAWN = 276,                 /* T_SPAWN  */
    T_TO = 277,                    /* T_TO  */
    T_INT_LITERAL = 278,           /* T_INT_LITERAL  */
    T_BOOL_LITERAL = 279,          /* T_BOOL_LITERAL  */
    T_FLOAT_LITERAL = 280,         /* T_FLOAT_LITERAL  */
    T_STRING_LITERAL = 281,        /* T_STRING_LITERAL  */
    T_ID = 282,                    /* T_ID  */
    T_AND = 283,                   /* T_AND  */
    T_OR = 284,                    /* T_OR  */
    T_EQ = 285,                    /* T_EQ  */
    T_NEQ = 286,                   /* T_NEQ  */
    T_LT = 287,                    /* T_LT  */
    T_GT = 288,                    /* T_GT  */
    T_LTE = 289,                   /* T_LTE  */
    T_GTE = 290,                   /* T_GTE  */
    T_ASSIGN = 291,                /* T_ASSIGN  */
    T_RANGE = 292                  /* T_RANGE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */


// extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_TOKENS_H_INCLUDED  */
