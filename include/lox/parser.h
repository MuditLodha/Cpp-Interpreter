#pragma once
#include "token.h"
#include "expr.h"
#include <vector>
#include <stdexcept>
#include "Stmt.h"

using namespace std;

// Forward declaration of the global error reporter in main.cpp
void error(Token token, const string& message);

class Parser {
private:
    class ParseError : public runtime_error {
    public:
        ParseError(const string& msg)
            : runtime_error(msg) {}
    };

public:
    Parser(const vector<Token>& tokens)
        : tokens(tokens) {}

    // Entry point — returns a list of statements
    vector<Stmt*> parse() {
        vector<Stmt*> statements;

        while (!isAtEnd()) {
            statements.push_back(declaration());
        }

        return statements;
    }

private:
    vector<Token> tokens;
    int current = 0;

    //////////////////////////////////////////////////////
    // Declarations (top-level things)
    //////////////////////////////////////////////////////

    Stmt* declaration() {
        try {
            if (match({FUN})) return function("function");
            if (match({VAR})) return varDeclaration();
            return statement();
        } catch (ParseError err) {
            synchronize();
            return nullptr;
        }
    }

    // fun name(params) { body }
    FunctionStmt* function(const string& kind) {
        Token name = consume(IDENTIFIER, "Expect " + kind + " name.");

        consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");

        vector<Token> params;

        if (!check(RIGHT_PAREN)) {
            do {
                if (params.size() >= 255) {
                    error(peek(), "Can't have more than 255 parameters.");
                }
                params.push_back(consume(IDENTIFIER, "Expect parameter name."));
            } while (match({COMMA}));
        }

        consume(RIGHT_PAREN, "Expect ')' after parameters.");
        consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");

        vector<Stmt*> body = block();
        return new FunctionStmt(name, params, body);
    }

    // var name = init;
    Stmt* varDeclaration() {
        Token name = consume(IDENTIFIER, "Expect variable name.");
        Expr* initializer = nullptr;
        if (match({EQUAL})) {
            initializer = expression();
        }
        consume(SEMICOLON, "Expect ';' after variable declaration.");
        return new VarStmt(name, initializer);
    }

    //////////////////////////////////////////////////////
    // Statements
    //////////////////////////////////////////////////////

    Stmt* statement() {
        if (match({IF}))    return ifStatement();
        if (match({PRINT})) return printStatement();
        if (match({RETURN})) return returnStatement();
        if (match({WHILE})) return whileStatement();
        if (match({FOR}))   return forStatement();
        if (match({LEFT_BRACE})) return new BlockStmt(block());
        return expressionStatement();
    }

    // if (cond) then [else else_branch]
    Stmt* ifStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'if'.");
        Expr* condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after if condition.");

        Stmt* thenBranch = statement();
        Stmt* elseBranch = nullptr;
        if (match({ELSE})) {
            elseBranch = statement();
        }

        return new IfStmt(condition, thenBranch, elseBranch);
    }

    // print expr;
    Stmt* printStatement() {
        Expr* value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new PrintStmt(value);
    }

    // return [expr];
    Stmt* returnStatement() {
        Token keyword = previous();
        Expr* value = nullptr;
        if (!check(SEMICOLON)) {
            value = expression();
        }
        consume(SEMICOLON, "Expect ';' after return value.");
        return new ReturnStmt(keyword, value);
    }

    // while (cond) body
    Stmt* whileStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'while'.");
        Expr* condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after while condition.");
        Stmt* body = statement();
        return new WhileStmt(condition, body);
    }

    // for (init; cond; incr) body
    // We desugar this into a while loop — same trick as the book
    Stmt* forStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'for'.");

        // initializer
        Stmt* initializer = nullptr;
        if (match({SEMICOLON})) {
            initializer = nullptr;
        } else if (match({VAR})) {
            initializer = varDeclaration();
        } else {
            initializer = expressionStatement();
        }

        // condition
        Expr* condition = nullptr;
        if (!check(SEMICOLON)) {
            condition = expression();
        }
        consume(SEMICOLON, "Expect ';' after for loop condition.");

        // increment
        Expr* increment = nullptr;
        if (!check(RIGHT_PAREN)) {
            increment = expression();
        }
        consume(RIGHT_PAREN, "Expect ')' after for clauses.");

        Stmt* body = statement();

        // desugar: tack increment onto end of body
        if (increment != nullptr) {
            body = new BlockStmt({ body, new ExpressionStmt(increment) });
        }

        // desugar: wrap in while; no condition means "while true"
        if (condition == nullptr) {
            condition = new Literal(Value(true));
        }
        body = new WhileStmt(condition, body);

        // desugar: wrap initializer before the while
        if (initializer != nullptr) {
            body = new BlockStmt({ initializer, body });
        }

        return body;
    }

    // { statements... }
    vector<Stmt*> block() {
        vector<Stmt*> statements;

        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            statements.push_back(declaration());
        }

        consume(RIGHT_BRACE, "Expect '}' after block.");
        return statements;
    }

    // expr;
    Stmt* expressionStatement() {
        Expr* expr = expression();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new ExpressionStmt(expr);
    }

    //////////////////////////////////////////////////////
    // Grammar Rules (Precedence Climbing)
    //////////////////////////////////////////////////////

    Expr* expression() {
        return assignment();
    }

    // name = value   (right-associative)
    Expr* assignment() {
        Expr* expr = orExpr();

        if (match({EQUAL})) {
            Token equals = previous();
            Expr* value = assignment();

            if (Variable* variable = dynamic_cast<Variable*>(expr)) {
                Token name = variable->name;
                delete expr;
                return new Assign(name, value);
            }

            throw error(equals, "Invalid assignment target.");
        }

        return expr;
    }

    // or  (short-circuit)
    Expr* orExpr() {
        Expr* expr = andExpr();

        while (match({OR})) {
            Token op = previous();
            Expr* right = andExpr();
            expr = new Logical(expr, op, right);
        }

        return expr;
    }

    // and  (short-circuit)
    Expr* andExpr() {
        Expr* expr = equality();

        while (match({AND})) {
            Token op = previous();
            Expr* right = equality();
            expr = new Logical(expr, op, right);
        }

        return expr;
    }

    Expr* equality() {
        Expr* expr = comparison();

        while (match({BANG_EQUAL, EQUAL_EQUAL})) {
            Token operator_ = previous();
            Expr* right = comparison();
            expr = new Binary(expr, operator_, right);
        }

        return expr;
    }

    Expr* comparison() {
        Expr* expr = term();

        while (match({GREATER, GREATER_EQUAL, LESS, LESS_EQUAL})) {
            Token operator_ = previous();
            Expr* right = term();
            expr = new Binary(expr, operator_, right);
        }

        return expr;
    }

    Expr* term() {
        Expr* expr = factor();

        while (match({MINUS, PLUS})) {
            Token operator_ = previous();
            Expr* right = factor();
            expr = new Binary(expr, operator_, right);
        }

        return expr;
    }

    Expr* factor() {
        Expr* expr = unary();

        while (match({SLASH, STAR})) {
            Token operator_ = previous();
            Expr* right = unary();
            expr = new Binary(expr, operator_, right);
        }

        return expr;
    }

    Expr* unary() {
        if (match({BANG, MINUS})) {
            Token operator_ = previous();
            Expr* right = unary();
            return new Unary(operator_, right);
        }

        return call();
    }

    // callee(args...)  — left associative chain: f(1)(2)(3)
    Expr* call() {
        Expr* expr = primary();

        while (true) {
            if (match({LEFT_PAREN})) {
                expr = finishCall(expr);
            } else {
                break;
            }
        }

        return expr;
    }

    // once we've seen '(' , collect arguments and closing ')'
    Expr* finishCall(Expr* callee) {
        vector<Expr*> arguments;

        if (!check(RIGHT_PAREN)) {
            do {
                if (arguments.size() >= 255) {
                    error(peek(), "Can't have more than 255 arguments.");
                }
                arguments.push_back(expression());
            } while (match({COMMA}));
        }

        Token paren = consume(RIGHT_PAREN, "Expect ')' after arguments.");
        return new Call(callee, paren, arguments);
    }

    Expr* primary() {
        if (match({FALSE})) return new Literal(Value(false));
        if (match({TRUE}))  return new Literal(Value(true));
        if (match({NIL}))   return new Literal(Value());

        if (match({NUMBER, STRING})) {
            return new Literal(previous().literal);
        }

        if (match({IDENTIFIER})) {
            return new Variable(previous());
        }

        if (match({LEFT_PAREN})) {
            Expr* expr = expression();
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Grouping(expr);
        }

        throw error(peek(), "Expect expression.");
    }

    //////////////////////////////////////////////////////
    // Helper Methods
    //////////////////////////////////////////////////////

    bool match(const vector<TokenType>& types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    Token consume(TokenType type, const string& message) {
        if (check(type)) return advance();
        throw error(peek(), message);
    }

    bool check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    bool isAtEnd() {
        return peek().type == END_OF_FILE;
    }

    Token peek() {
        return tokens[current];
    }

    Token previous() {
        return tokens[current - 1];
    }

    ParseError error(Token token, const string& message) {
        ::error(token, message);   // call global error reporter in main.cpp
        return ParseError(message);
    }

    //////////////////////////////////////////////////////
    // Panic Mode Recovery — skip to next statement boundary
    //////////////////////////////////////////////////////
    void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == SEMICOLON) return;

            switch (peek().type) {
                case CLASS:
                case FUN:
                case VAR:
                case FOR:
                case IF:
                case WHILE:
                case PRINT:
                case RETURN:
                    return;
                default:
                    break;
            }

            advance();
        }
    }
};
