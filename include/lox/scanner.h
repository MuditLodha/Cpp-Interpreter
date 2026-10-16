#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include "token.h"

using namespace std;

void error(int line, const string& message);

class Scanner {
private:
    string source;
    vector<Token> tokens;

    int start = 0;
    int current = 0;
    int line = 1;

    unordered_map<string, TokenType> keywords = {
        {"and", AND}, {"class", CLASS}, {"else", ELSE}, {"false", FALSE},
        {"for", FOR}, {"fun", FUN}, {"if", IF}, {"nil", NIL},
        {"or", OR}, {"print", PRINT}, {"return", RETURN},
        {"super", SUPER}, {"this", THIS}, {"true", TRUE},
        {"var", VAR}, {"while", WHILE}
    };

public:
    Scanner(const string& source)
        : source(source) {}

    vector<Token> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }

        tokens.push_back(Token(END_OF_FILE, "", Value(), line));
        return tokens;
    }

private:
    bool isAtEnd() {
        return current >= (int)source.length();
    }

    char advance() {
        return source[current++];
    }

    void addToken(TokenType type) {
        addToken(type, Value());
    }

    void addToken(TokenType type, const Value& literal) {
        string text = source.substr(start, current - start);
        tokens.push_back(Token(type, text, literal, line));
    }

    bool match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        current++;
        return true;
    }

    char peek() {
        if (isAtEnd()) return '\0';
        return source[current];
    }

    char peekNext() {
        if (current + 1 >= (int)source.length()) return '\0';
        return source[current + 1];
    }

    void scanToken() {
        char c = advance();

        switch (c) {
            case '(': addToken(LEFT_PAREN); break;
            case ')': addToken(RIGHT_PAREN); break;
            case '{': addToken(LEFT_BRACE); break;
            case '}': addToken(RIGHT_BRACE); break;
            case ',': addToken(COMMA); break;
            case '.': addToken(DOT); break;
            case '-': addToken(MINUS); break;
            case '+': addToken(PLUS); break;
            case ';': addToken(SEMICOLON); break;
            case '*': addToken(STAR); break;

            case '!':
                addToken(match('=') ? BANG_EQUAL : BANG);
                break;

            case '=':
                addToken(match('=') ? EQUAL_EQUAL : EQUAL);
                break;

            case '<':
                addToken(match('=') ? LESS_EQUAL : LESS);
                break;

            case '>':
                addToken(match('=') ? GREATER_EQUAL : GREATER);
                break;

            case '/':
                if (match('/')) {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    addToken(SLASH);
                }
                break;

            case ' ':
            case '\r':
            case '\t':
                break;

            case '\n':
                line++;
                break;

            case '"':
                stringLiteral();
                break;

            default:
                if (isdigit(c)) {
                    number();
                }
                else if (isalpha(c) || c == '_') {
                    identifier();
                }
                else {
                    error(line, "Unexpected character.");
                }
                break;
        }
    }

    void stringLiteral() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            error(line, "Unterminated string.");
            return;
        }

        advance(); // closing "

        string value = source.substr(start + 1, current - start - 2);
        addToken(STRING, Value(value));
    }

    void number() {
        while (isdigit(peek())) advance();

        if (peek() == '.' && isdigit(peekNext())) {
            advance();
            while (isdigit(peek())) advance();
        }

        // ✅ FIX: If a digit sequence is immediately followed by a letter or '_',
        // it's an invalid token like "4_" or "123abc". Report it as an error.
        if (isalpha(peek()) || peek() == '_') {
            // consume the rest of the bad token
            while (isalnum(peek()) || peek() == '_') advance();
            string bad = source.substr(start, current - start);
            error(line, "Invalid number literal '" + bad + "'.");
            return;
        }

        string value = source.substr(start, current - start);
        addToken(NUMBER, Value(stod(value)));
    }

    void identifier() {
        while (isalnum(peek()) || peek() == '_') advance();

        string text = source.substr(start, current - start);

        if (keywords.count(text)) {
            addToken(keywords[text]);
        } else {
            addToken(IDENTIFIER);
        }
    }
};