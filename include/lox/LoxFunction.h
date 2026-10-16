#pragma once
#include "LoxCallable.h"
#include "Stmt.h"
#include "Environment.h"
#include <vector>
#include <string>
#include <memory>

// ReturnException — thrown when "return" is hit, caught in LoxFunction::call()
// Using exceptions for control flow is the simplest approach (same as book Ch10)
struct ReturnException {
    Value value;
    ReturnException(Value value) : value(value) {}
};

// LoxFunction — a user-defined function created by a "fun" statement
// It stores:
//   - the FunctionStmt (params + body, owned by the AST)
//   - the closure: the environment where the function was DEFINED (shared_ptr so it
//     stays alive even after the outer function returns — this is what makes closures work)
class LoxFunction : public LoxCallable {
public:
    FunctionStmt* declaration;
    std::shared_ptr<Environment> closure;   // captured at definition time

    LoxFunction(FunctionStmt* declaration, std::shared_ptr<Environment> closure)
        : declaration(declaration), closure(closure) {}

    int arity() override {
        return (int)declaration->params.size();
    }

    // Defined after Interpreter is complete (in Interpreter.h)
    Value call(Interpreter& interp, std::vector<Value> args) override;

    std::string toString() override {
        return "<fn " + declaration->name.lexeme + ">";
    }
};
