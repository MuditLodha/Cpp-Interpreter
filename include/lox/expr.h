#pragma once
#include "token.h"
#include <string>
#include <vector>
#include "value.h"

using namespace std;

// Forward declarations of all expression types
class Binary;
class Literal;
class Unary;
class Grouping;
class Variable;
class Assign;
class Logical;
class Call;

//////////////////////////////////////////////////////////
// Visitor Base (non-template, simple)
//////////////////////////////////////////////////////////

class VisitorBase {
public:
    virtual void visitBinary(Binary* expr) = 0;
    virtual void visitLiteral(Literal* expr) = 0;
    virtual void visitUnary(Unary* expr) = 0;
    virtual void visitGrouping(Grouping* expr) = 0;
    virtual void visitVariable(Variable* expr) = 0;
    virtual void visitAssignExpr(Assign* expr) = 0;
    virtual void visitLogical(Logical* expr) = 0;
    virtual void visitCall(Call* expr) = 0;

    virtual ~VisitorBase() = default;
};

//////////////////////////////////////////////////////////
// Expr Base
//////////////////////////////////////////////////////////

class Expr {
public:
    virtual ~Expr() = default;
    virtual void accept(VisitorBase& visitor) = 0;
};

//////////////////////////////////////////////////////////
// Binary:  left op right    (+ - * / == != < > <= >=)
//////////////////////////////////////////////////////////

class Binary : public Expr {
public:
    Expr* left;
    Token operator_;
    Expr* right;

    Binary(Expr* left, Token operator_, Expr* right)
        : left(left), operator_(operator_), right(right) {}

    ~Binary() {
        delete left;
        delete right;
    }

    void accept(VisitorBase& visitor) override {
        visitor.visitBinary(this);
    }
};

//////////////////////////////////////////////////////////
// Literal:  42  "hello"  true  false  nil
//////////////////////////////////////////////////////////

class Literal : public Expr {
public:
    Value value;

    Literal(const Value& value) : value(value) {}

    void accept(VisitorBase& visitor) override {
        visitor.visitLiteral(this);
    }
};

//////////////////////////////////////////////////////////
// Unary:  -expr   !expr
//////////////////////////////////////////////////////////

class Unary : public Expr {
public:
    Token operator_;
    Expr* right;

    Unary(Token operator_, Expr* right)
        : operator_(operator_), right(right) {}

    ~Unary() {
        delete right;
    }

    void accept(VisitorBase& visitor) override {
        visitor.visitUnary(this);
    }
};

//////////////////////////////////////////////////////////
// Grouping:  ( expr )
//////////////////////////////////////////////////////////

class Grouping : public Expr {
public:
    Expr* expression;

    Grouping(Expr* expression) : expression(expression) {}

    ~Grouping() {
        delete expression;
    }

    void accept(VisitorBase& visitor) override {
        visitor.visitGrouping(this);
    }
};

//////////////////////////////////////////////////////////
// Variable:  identifier
//////////////////////////////////////////////////////////

class Variable : public Expr {
public:
    Token name;

    Variable(Token name) : name(name) {}

    void accept(VisitorBase& visitor) override {
        visitor.visitVariable(this);
    }
};

//////////////////////////////////////////////////////////
// Assign:  name = value
//////////////////////////////////////////////////////////

class Assign : public Expr {
public:
    Token name;
    Expr* value;

    Assign(Token name, Expr* value) : name(name), value(value) {}

    ~Assign() {
        delete value;
    }

    void accept(VisitorBase& visitor) override {
        visitor.visitAssignExpr(this);
    }
};

//////////////////////////////////////////////////////////
// Logical:  left and/or right  (short-circuit!)
//////////////////////////////////////////////////////////

class Logical : public Expr {
public:
    Expr* left;
    Token operator_;   // AND or OR token
    Expr* right;

    Logical(Expr* left, Token operator_, Expr* right)
        : left(left), operator_(operator_), right(right) {}

    ~Logical() {
        delete left;
        delete right;
    }

    void accept(VisitorBase& visitor) override {
        visitor.visitLogical(this);
    }
};

//////////////////////////////////////////////////////////
// Call:  callee(arg1, arg2, ...)
//////////////////////////////////////////////////////////

class Call : public Expr {
public:
    Expr* callee;
    Token paren;              // closing ')' — used for error messages
    vector<Expr*> arguments;

    Call(Expr* callee, Token paren, vector<Expr*> arguments)
        : callee(callee), paren(paren), arguments(arguments) {}

    ~Call() {
        delete callee;
        for (Expr* arg : arguments) delete arg;
    }

    void accept(VisitorBase& visitor) override {
        visitor.visitCall(this);
    }
};
