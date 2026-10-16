#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "expr.h"
#include "Stmt.h"
#include "Interpreter.h"

// error(Token, string) is defined in main.cpp — declare it here so Resolver can call it.
void error(Token token, const std::string& message);

// ================================================================
// Resolver — Chapter 11 of Crafting Interpreters
//
// A second pass over the AST, done BEFORE the interpreter runs.
// It walks every statement and expression and, for each variable
// reference, counts how many scopes out its declaration sits.
// That count (called "distance") is handed to the interpreter via
// interpreter.resolve(expr, distance), so at runtime the interpreter
// can jump directly to the right scope instead of searching by name.
//
// The Resolver also catches two static errors that the parser can't:
//   1. var a = a;  -- reading a variable in its own initializer
//   2. return;     -- at the top level (outside any function)
// ================================================================

class Resolver : public VisitorBase, public StmtVisitorBase {
public:
    Resolver(Interpreter& interpreter) : interpreter(interpreter) {}

    // Entry point: resolve a whole program (list of statements).
    void resolve(const std::vector<Stmt*>& statements) {
        for (Stmt* stmt : statements) resolve(stmt);
    }

    ///////////////////////////////////////////////////////////////
    // Statement visitors
    ///////////////////////////////////////////////////////////////

    // { stmts... }  opens a new local scope, resolves contents, closes it.
    void visitBlockStmt(BlockStmt* stmt) override {
        beginScope();
        resolve(stmt->statements);
        endScope();
    }

    // var x = initializer;
    // Note: declare first (marks as "not yet ready"), then resolve
    // the initializer, then define (marks as "ready").
    // This is what catches:  var a = a;
    void visitVarStmt(VarStmt* stmt) override {
        declare(stmt->name);
        if (stmt->initializer != nullptr) resolve(stmt->initializer);
        define(stmt->name);
    }

    // fun name(params) { body }
    // We define the name BEFORE resolving the body so recursive calls work.
    void visitFunctionStmt(FunctionStmt* stmt) override {
        declare(stmt->name);
        define(stmt->name);
        resolveFunction(stmt, FunctionType::FUNCTION);
    }

    // if (cond) then [else elseBranch]
    void visitIfStmt(IfStmt* stmt) override {
        resolve(stmt->condition);
        resolve(stmt->thenBranch);
        if (stmt->elseBranch != nullptr) resolve(stmt->elseBranch);
    }

    // while (cond) body
    void visitWhileStmt(WhileStmt* stmt) override {
        resolve(stmt->condition);
        resolve(stmt->body);
    }

    // return [value];
    // Static error if used outside any function.
    void visitReturnStmt(ReturnStmt* stmt) override {
        if (currentFunction == FunctionType::NONE) {
            error(stmt->keyword, "Can't return from top-level code.");
        }
        if (stmt->value != nullptr) resolve(stmt->value);
    }

    // expression;
    void visitExpressionStmt(ExpressionStmt* stmt) override {
        resolve(stmt->expression);
    }

    // print expr;
    void visitPrintStmt(PrintStmt* stmt) override {
        resolve(stmt->expression);
    }

    ///////////////////////////////////////////////////////////////
    // Expression visitors
    ///////////////////////////////////////////////////////////////

    // a variable reference: resolve where it lives
    void visitVariable(Variable* expr) override {
        // Check for the self-initializer bug: if the variable exists
        // in the innermost scope but is marked false (= declared, not
        // yet defined), it means we're inside its own initializer.
        if (!scopes.empty()) {
            auto& scope = scopes.back();
            auto it = scope.find(expr->name.lexeme);
            if (it != scope.end() && it->second == false) {
                error(expr->name,
                    "Can't read local variable in its own initializer.");
            }
        }
        resolveLocal(expr, expr->name);
    }

    // name = value
    void visitAssignExpr(Assign* expr) override {
        resolve(expr->value);
        resolveLocal(expr, expr->name);
    }

    // binary op (+ - * / == != < > <= >=) -- just recurse both sides
    void visitBinary(Binary* expr) override {
        resolve(expr->left);
        resolve(expr->right);
    }

    // and / or -- just recurse both sides
    void visitLogical(Logical* expr) override {
        resolve(expr->left);
        resolve(expr->right);
    }

    // -expr  or  !expr
    void visitUnary(Unary* expr) override {
        resolve(expr->right);
    }

    // (expr)
    void visitGrouping(Grouping* expr) override {
        resolve(expr->expression);
    }

    // 42  "hello"  true  nil -- nothing to resolve
    void visitLiteral(Literal* expr) override {
        (void)expr; // nothing to do
    }

    // callee(args...)
    void visitCall(Call* expr) override {
        resolve(expr->callee);
        for (Expr* arg : expr->arguments) resolve(arg);
    }

private:
    // Track whether we're currently inside a function body.
    // NONE = top-level code (where return is illegal).
    enum class FunctionType { NONE, FUNCTION };

    Interpreter& interpreter;

    // Stack of scopes. Each scope maps variable name -> bool.
    // false = declared but not yet defined (initializer not done yet)
    // true  = fully defined and ready to use
    std::vector<std::unordered_map<std::string, bool>> scopes;

    FunctionType currentFunction = FunctionType::NONE;

    ///////////////////////////////////////////////////////////////
    // Helpers
    ///////////////////////////////////////////////////////////////

    void resolve(Stmt* stmt) { if (stmt) stmt->accept(*this); }
    void resolve(Expr* expr) { if (expr) expr->accept(*this); }

    // Open a new local scope.
    void beginScope() {
        scopes.push_back({});
    }

    // Close (discard) the innermost local scope.
    void endScope() {
        scopes.pop_back();
    }

    // Mark a name as "exists but not ready yet" in the innermost scope.
    // Also catches duplicate declarations in the same scope.
    void declare(const Token& name) {
        if (scopes.empty()) return;   // globals: not tracked here
        auto& scope = scopes.back();
        if (scope.count(name.lexeme)) {
            error(name, "Already a variable with this name in this scope.");
        }
        scope[name.lexeme] = false;
    }

    // Mark a name as "fully ready to use" in the innermost scope.
    void define(const Token& name) {
        if (scopes.empty()) return;
        scopes.back()[name.lexeme] = true;
    }

    // Walk from the innermost scope outward looking for 'name'.
    // When found, tell the interpreter the distance (0 = innermost scope).
    // If not found in any local scope -> leave it unresolved (= global).
    void resolveLocal(Expr* expr, const Token& name) {
        for (int i = (int)scopes.size() - 1; i >= 0; i--) {
            if (scopes[i].count(name.lexeme)) {
                // distance = how many hops from the current scope to scope[i]
                interpreter.resolve(expr, (int)scopes.size() - 1 - i);
                return;
            }
        }
        // Not found in any local scope: global variable, no distance stored.
        // The interpreter will fall back to globalEnv->get() at runtime.
    }

    // Resolve a function's parameters and body in a fresh scope.
    void resolveFunction(FunctionStmt* fn, FunctionType type) {
        FunctionType enclosing = currentFunction;
        currentFunction = type;

        beginScope();
        for (Token& param : fn->params) {
            declare(param);
            define(param);
        }
        resolve(fn->body);
        endScope();

        currentFunction = enclosing;   // restore (handles nested functions)
    }
};
