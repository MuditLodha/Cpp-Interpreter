#pragma once
#include "expr.h"
#include <string>

// AstPrinter: walks the expression tree and produces a string like
//   (* (- 123) (group 45.67))
// Useful for debugging — it's the first "real visitor" from Ch5 of the book.

class AstPrinter : public VisitorBase {
public:
    std::string print(Expr* expr) {
        if (expr == nullptr) return "nil";
        return printExpr(expr);
    }

    void visitBinary(Binary* expr) override {
        result = parenthesize(expr->operator_.lexeme, expr->left, expr->right);
    }

    void visitGrouping(Grouping* expr) override {
        result = parenthesize("group", expr->expression);
    }

    void visitLiteral(Literal* expr) override {
        switch (expr->value.type) {
            case ValueType::NIL:      result = "nil"; break;
            case ValueType::NUMBER:   result = std::to_string(expr->value.numberValue); break;
            case ValueType::BOOL:     result = expr->value.boolValue ? "true" : "false"; break;
            case ValueType::STRING:   result = expr->value.stringValue; break;
            case ValueType::CALLABLE: result = "<fn>"; break;
        }
    }

    void visitUnary(Unary* expr) override {
        result = parenthesize(expr->operator_.lexeme, expr->right);
    }

    void visitVariable(Variable* expr) override {
        result = expr->name.lexeme;
    }

    void visitAssignExpr(Assign* expr) override {
        result = "(= " + expr->name.lexeme + " " + printExpr(expr->value) + ")";
    }

    // and / or — show the operator and both sides
    void visitLogical(Logical* expr) override {
        result = parenthesize(expr->operator_.lexeme, expr->left, expr->right);
    }

    // call — show callee(arg1, arg2, ...)
    void visitCall(Call* expr) override {
        result = "(call " + printExpr(expr->callee);
        for (Expr* arg : expr->arguments) {
            result += " " + printExpr(arg);
        }
        result += ")";
    }

private:
    std::string result;

    std::string printExpr(Expr* expr) {
        expr->accept(*this);
        return result;
    }

    std::string parenthesize(const std::string& name, Expr* expr1) {
        return "(" + name + " " + printExpr(expr1) + ")";
    }

    std::string parenthesize(const std::string& name, Expr* expr1, Expr* expr2) {
        return "(" + name + " " + printExpr(expr1) + " " + printExpr(expr2) + ")";
    }
};