#ifndef TOKENS_H
#define TOKENS_H

/* Enumerate all Jade DSL tokens for use with Flex/Bison */
enum jade_tokens {
    T_ROLE = 258,
    T_TASK,
    T_ON,
    T_IF,
    T_ELSE,
    T_PRINT,
    T_INT,
    T_FLOAT,
    T_BOOL,
    T_STRING,
    T_TENSOR,
    T_LIST,
    T_RECORD,
    T_BROADCAST,
    T_SEND,
    T_RECV,
    T_GATHER,
    T_FROM,
    T_SPAWN,
    T_TO,
    T_INT_LITERAL,
    T_FLOAT_LITERAL,
    T_STRING_LITERAL,
    T_BOOL_LITERAL,
    T_ID,
    T_AND,
    T_OR,
    T_EQ,
    T_NEQ,
    T_LT,
    T_GT,
    T_LTE,
    T_GTE,
    T_ASSIGN,
    T_RANGE
};

#endif /* TOKENS_H */