#pragma once
#include <string>
#include "value.h"

using namespace std;

enum TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // One or two character tokens
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // Literals
    IDENTIFIER, STRING, NUMBER,

    // Keywords
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    END_OF_FILE
};

struct Token {
    TokenType type;
    string lexeme;
    Value literal;
    int line;

    Token()
        : type(END_OF_FILE), lexeme(""), literal(Value()), line(0) {}

    Token(TokenType type, const string& lexeme, const Value& literal, int line)
        : type(type), lexeme(lexeme), literal(literal), line(line) {}

    string toString() const {
        return lexeme;
    }
};