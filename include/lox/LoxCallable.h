#pragma once
#include <vector>
#include <string>
#include "value.h"

// Forward declare so Interpreter.h can forward-declare these too
class Interpreter;

// LoxCallable — interface that anything callable (functions, etc.) must implement
class LoxCallable {
public:
    virtual int arity() = 0;                               // how many args it expects
    virtual Value call(Interpreter& interp,
                       std::vector<Value> args) = 0;       // actually call it
    virtual std::string toString() = 0;                    // for print
    virtual ~LoxCallable() = default;
};
