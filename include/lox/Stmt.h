#pragma once
#include "expr.h"
#include <vector>

// Forward declarations
class ExpressionStmt;
class PrintStmt;
class VarStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class FunctionStmt;
class ReturnStmt;

class StmtVisitorBase {
public:
    virtual void visitExpressionStmt(ExpressionStmt* stmt) = 0;
    virtual void visitPrintStmt(PrintStmt* stmt) = 0;
    virtual void visitVarStmt(VarStmt* stmt) = 0;
    virtual void visitBlockStmt(BlockStmt* stmt) = 0;
    virtual void visitIfStmt(IfStmt* stmt) = 0;
    virtual void visitWhileStmt(WhileStmt* stmt) = 0;
    virtual void visitFunctionStmt(FunctionStmt* stmt) = 0;
    virtual void visitReturnStmt(ReturnStmt* stmt) = 0;
};

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitorBase& visitor) = 0;
};

// expression statement:  expr;
class ExpressionStmt : public Stmt {
public:
    Expr* expression;

    ExpressionStmt(Expr* expression)
        : expression(expression) {}

    ~ExpressionStmt() { delete expression; }

    void accept(StmtVisitorBase& visitor) override {
        visitor.visitExpressionStmt(this);
    }
};

// print statement:  print expr;
class PrintStmt : public Stmt {
public:
    Expr* expression;

    PrintStmt(Expr* expression)
        : expression(expression) {}

    ~PrintStmt() { delete expression; }

    void accept(StmtVisitorBase& visitor) override {
        visitor.visitPrintStmt(this);
    }
};

// var declaration:  var name = init;
class VarStmt : public Stmt {
public:
    Token name;
    Expr* initializer;  // can be nullptr

    VarStmt(Token name, Expr* initializer)
        : name(name), initializer(initializer) {}

    ~VarStmt() { delete initializer; }

    void accept(StmtVisitorBase& visitor) override {
        visitor.visitVarStmt(this);
    }
};

// block statement:  { stmts... }
class BlockStmt : public Stmt {
public:
    std::vector<Stmt*> statements;

    BlockStmt(std::vector<Stmt*> statements)
        : statements(statements) {}

    ~BlockStmt() {
        for (Stmt* s : statements) delete s;
    }

    void accept(StmtVisitorBase& visitor) override {
        visitor.visitBlockStmt(this);
    }
};

// if statement:  if (cond) then else
class IfStmt : public Stmt {
public:
    Expr* condition;
    Stmt* thenBranch;
    Stmt* elseBranch;   // can be nullptr

    IfStmt(Expr* condition, Stmt* thenBranch, Stmt* elseBranch)
        : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}

    ~IfStmt() {
        delete condition;
        delete thenBranch;
        delete elseBranch;
    }

    void accept(StmtVisitorBase& visitor) override {
        visitor.visitIfStmt(this);
    }
};

// while statement:  while (cond) body
// for loops get desugared into WhileStmt by the parser
class WhileStmt : public Stmt {
public:
    Expr* condition;
    Stmt* body;

    WhileStmt(Expr* condition, Stmt* body)
        : condition(condition), body(body) {}

    ~WhileStmt() {
        delete condition;
        delete body;
    }

    void accept(StmtVisitorBase& visitor) override {
        visitor.visitWhileStmt(this);
    }
};

// function declaration:  fun name(params) { body }
class FunctionStmt : public Stmt {
public:
    Token name;
    std::vector<Token> params;
    std::vector<Stmt*> body;

    FunctionStmt(Token name, std::vector<Token> params, std::vector<Stmt*> body)
        : name(name), params(params), body(body) {}

    ~FunctionStmt() {
        for (Stmt* s : body) delete s;
    }

    void accept(StmtVisitorBase& visitor) override {
        visitor.visitFunctionStmt(this);
    }
};

// return statement:  return value;
class ReturnStmt : public Stmt {
public:
    Token keyword;   // the 'return' token, for error reporting
    Expr* value;     // can be nullptr (bare return;)

    ReturnStmt(Token keyword, Expr* value)
        : keyword(keyword), value(value) {}

    ~ReturnStmt() { delete value; }

    void accept(StmtVisitorBase& visitor) override {
        visitor.visitReturnStmt(this);
    }
};