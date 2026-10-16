#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <memory>
#include "errors.h"
#include "Environment.h"
#include "expr.h"
#include "Stmt.h"
#include "value.h"
#include "token.h"
#include "LoxCallable.h"
#include "LoxFunction.h"

using namespace std;

void runtimeError(const RuntimeError& error);

class Interpreter : public VisitorBase, public StmtVisitorBase {
public:

    shared_ptr<Environment> globalEnv;
    shared_ptr<Environment> environment;   // current scope

    // Filled by the Resolver: maps each Variable/Assign/This expr
    // to how many scopes out its binding lives.
    unordered_map<Expr*, int> locals;

    Interpreter() {
        globalEnv = make_shared<Environment>();
        environment = globalEnv;
    }

    // Called by Resolver once per resolved reference.
    void resolve(Expr* expr, int depth) {
        locals[expr] = depth;
    }

    //////////////////////////////////////////////////////
    // Entry point
    //////////////////////////////////////////////////////
    void interpret(const vector<Stmt*>& statements) {
        try {
            for (Stmt* stmt : statements) {
                execute(stmt);
            }
        }
        catch (const RuntimeError& error) {
            runtimeError(error);
        }
    }

    //////////////////////////////////////////////////////
    // Execute a block in a given environment
    // Called by visitBlockStmt and LoxFunction::call()
    //////////////////////////////////////////////////////
    void executeBlock(const vector<Stmt*>& statements, shared_ptr<Environment> env) {
        shared_ptr<Environment> previous = environment;
        environment = env;

        try {
            for (Stmt* stmt : statements) {
                execute(stmt);
            }
        } catch (...) {
            environment = previous;   // always restore scope
            throw;
        }

        environment = previous;
    }

    //////////////////////////////////////////////////////
    // Statement Visitors
    //////////////////////////////////////////////////////

    void visitExpressionStmt(ExpressionStmt* stmt) override {
        evaluate(stmt->expression);
    }

    void visitPrintStmt(PrintStmt* stmt) override {
        Value value = evaluate(stmt->expression);
        cout << stringify(value) << endl;
    }

    void visitVarStmt(VarStmt* stmt) override {
        Value value = Value();

        if (stmt->initializer != nullptr) {
            value = evaluate(stmt->initializer);
        }

        environment->define(stmt->name.lexeme, value);
    }

    // { statements... }  — creates a new scope
    void visitBlockStmt(BlockStmt* stmt) override {
        auto blockEnv = make_shared<Environment>(environment);
        executeBlock(stmt->statements, blockEnv);
    }

    // if (cond) then [else else_branch]
    void visitIfStmt(IfStmt* stmt) override {
        Value cond = evaluate(stmt->condition);

        if (isTruthy(cond)) {
            execute(stmt->thenBranch);
        } else if (stmt->elseBranch != nullptr) {
            execute(stmt->elseBranch);
        }
    }

    // while (cond) body
    void visitWhileStmt(WhileStmt* stmt) override {
        while (isTruthy(evaluate(stmt->condition))) {
            execute(stmt->body);
        }
    }

    // fun name(params) { body }
    void visitFunctionStmt(FunctionStmt* stmt) override {
        // capture the current environment as the closure
        LoxFunction* fn = new LoxFunction(stmt, environment);
        allocatedFunctions.push_back(fn);
        environment->define(stmt->name.lexeme, Value(fn));
    }

    // return [value];
    void visitReturnStmt(ReturnStmt* stmt) override {
        Value value = Value();

        if (stmt->value != nullptr) {
            value = evaluate(stmt->value);
        }

        throw ReturnException(value);
    }

    //////////////////////////////////////////////////////
    // Expression Visitors
    //////////////////////////////////////////////////////

    void visitVariable(Variable* expr) override {
        result = lookUpVariable(expr->name, expr);
    }

    void visitLiteral(Literal* expr) override {
        result = expr->value;
    }

    void visitAssignExpr(Assign* expr) override {
        Value value = evaluate(expr->value);
        auto it = locals.find(expr);
        if (it != locals.end()) {
            // Resolver told us exactly which scope to write to
            environment->assignAt(it->second, expr->name, value);
        } else {
            // Not found locally -> must be global
            globalEnv->assign(expr->name, value);
        }
        result = value;
    }

    void visitGrouping(Grouping* expr) override {
        result = evaluate(expr->expression);
    }

    void visitUnary(Unary* expr) override {
        Value right = evaluate(expr->right);

        switch (expr->operator_.type) {
            case MINUS:
                checkNumberOperand(expr->operator_, right);
                result = Value(-right.numberValue);
                return;

            case BANG:
                result = Value(!isTruthy(right));
                return;

            default:
                return;
        }
    }

    // and / or — short circuit
    void visitLogical(Logical* expr) override {
        Value left = evaluate(expr->left);

        if (expr->operator_.type == OR) {
            if (isTruthy(left)) { result = left; return; }
        } else {
            if (!isTruthy(left)) { result = left; return; }
        }

        result = evaluate(expr->right);
    }

    // callee(args...)
    void visitCall(Call* expr) override {
        Value callee = evaluate(expr->callee);

        vector<Value> arguments;
        for (Expr* arg : expr->arguments) {
            arguments.push_back(evaluate(arg));
        }

        if (callee.type != ValueType::CALLABLE) {
            throw RuntimeError(expr->paren, "Can only call functions and classes.");
        }

        LoxCallable* function = callee.callableValue;

        if ((int)arguments.size() != function->arity()) {
            throw RuntimeError(expr->paren,
                "Expected " + to_string(function->arity()) +
                " arguments but got " + to_string(arguments.size()) + ".");
        }

        result = function->call(*this, arguments);
    }

    void visitBinary(Binary* expr) override {
        Value left = evaluate(expr->left);
        Value right = evaluate(expr->right);

        switch (expr->operator_.type) {

            case MINUS:
                checkNumberOperands(expr->operator_, left, right);
                result = Value(left.numberValue - right.numberValue);
                return;

            case SLASH:
                checkNumberOperands(expr->operator_, left, right);
                if (right.numberValue == 0) {
                    throw RuntimeError(expr->operator_, "Division by zero.");
                }
                result = Value(left.numberValue / right.numberValue);
                return;

            case STAR:
                checkNumberOperands(expr->operator_, left, right);
                result = Value(left.numberValue * right.numberValue);
                return;

            case PLUS:
                if (left.type == ValueType::NUMBER && right.type == ValueType::NUMBER) {
                    result = Value(left.numberValue + right.numberValue);
                    return;
                }
                if (left.type == ValueType::STRING && right.type == ValueType::STRING) {
                    result = Value(left.stringValue + right.stringValue);
                    return;
                }
                throw RuntimeError(expr->operator_,
                    "Operands must be two numbers or two strings.");

            case GREATER:
                checkNumberOperands(expr->operator_, left, right);
                result = Value(left.numberValue > right.numberValue);
                return;

            case GREATER_EQUAL:
                checkNumberOperands(expr->operator_, left, right);
                result = Value(left.numberValue >= right.numberValue);
                return;

            case LESS:
                checkNumberOperands(expr->operator_, left, right);
                result = Value(left.numberValue < right.numberValue);
                return;

            case LESS_EQUAL:
                checkNumberOperands(expr->operator_, left, right);
                result = Value(left.numberValue <= right.numberValue);
                return;

            case BANG_EQUAL:
                result = Value(!isEqual(left, right));
                return;

            case EQUAL_EQUAL:
                result = Value(isEqual(left, right));
                return;

            default:
                return;
        }
    }

private:
    Value result;
    vector<LoxCallable*> allocatedFunctions;

    // If the Resolver recorded a distance for this expr, jump straight there.
    // Otherwise fall back to the global env (top-level variables stay dynamic).
    Value lookUpVariable(const Token& name, Expr* expr) {
        auto it = locals.find(expr);
        if (it != locals.end()) {
            return environment->getAt(it->second, name.lexeme);
        }
        return globalEnv->get(name);
    }

    Value evaluate(Expr* expr) {
        expr->accept(*this);
        return result;
    }

    void execute(Stmt* stmt) {
        stmt->accept(*this);
    }

    string stringify(const Value& value) {
        switch (value.type) {
            case ValueType::NIL:      return "nil";
            case ValueType::BOOL:     return value.boolValue ? "true" : "false";
            case ValueType::STRING:   return value.stringValue;
            case ValueType::CALLABLE: return value.callableValue->toString();

            case ValueType::NUMBER: {
                double n = value.numberValue;
                if (n == (long long)n) {
                    return to_string((long long)n);
                }
                string text = to_string(n);
                while (!text.empty() && text.back() == '0') text.pop_back();
                if (!text.empty() && text.back() == '.') text.pop_back();
                return text;
            }
        }
        return "";
    }

    bool isTruthy(const Value& value) {
        if (value.type == ValueType::NIL) return false;
        if (value.type == ValueType::BOOL) return value.boolValue;
        return true;
    }

    bool isEqual(const Value& a, const Value& b) {
        if (a.type != b.type) return false;
        switch (a.type) {
            case ValueType::NIL:    return true;
            case ValueType::NUMBER: return a.numberValue == b.numberValue;
            case ValueType::BOOL:   return a.boolValue == b.boolValue;
            case ValueType::STRING: return a.stringValue == b.stringValue;
            default:                return false;
        }
    }

    void checkNumberOperand(Token op, const Value& operand) {
        if (operand.type == ValueType::NUMBER) return;
        throw RuntimeError(op, "Operand must be a number.");
    }

    void checkNumberOperands(Token op, const Value& left, const Value& right) {
        if (left.type == ValueType::NUMBER && right.type == ValueType::NUMBER) return;
        throw RuntimeError(op, "Operands must be numbers.");
    }
};

// LoxFunction::call() is defined here because it needs the full Interpreter class
inline Value LoxFunction::call(Interpreter& interp, std::vector<Value> args) {
    // new scope, enclosed by the CLOSURE (definition-time env, not call-site!)
    auto funcEnv = std::make_shared<Environment>(closure);

    // bind parameters
    for (int i = 0; i < (int)declaration->params.size(); i++) {
        funcEnv->define(declaration->params[i].lexeme, args[i]);
    }

    try {
        interp.executeBlock(declaration->body, funcEnv);
    } catch (ReturnException& ret) {
        return ret.value;
    }

    return Value();   // implicit nil
}