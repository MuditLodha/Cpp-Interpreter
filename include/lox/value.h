#pragma once
#include <string>

// Forward declare LoxCallable — defined in LoxCallable.h
// We store a raw pointer here so Value stays simple
class LoxCallable;

enum class ValueType {
    NIL,
    NUMBER,
    BOOL,
    STRING,
    CALLABLE    // functions / native functions
};

class Value {
public:
    ValueType type;

    double      numberValue;
    bool        boolValue;
    std::string stringValue;
    LoxCallable* callableValue;   // not owned — managed by Interpreter

    // nil
    Value() : type(ValueType::NIL), numberValue(0), boolValue(false), callableValue(nullptr) {}

    // number
    Value(double num) : type(ValueType::NUMBER), numberValue(num), boolValue(false), callableValue(nullptr) {}

    // bool
    Value(bool b) : type(ValueType::BOOL), numberValue(0), boolValue(b), callableValue(nullptr) {}

    // string
    Value(const std::string& str)
        : type(ValueType::STRING), numberValue(0), boolValue(false),
          stringValue(str), callableValue(nullptr) {}

    // callable (function)
    Value(LoxCallable* fn)
        : type(ValueType::CALLABLE), numberValue(0), boolValue(false), callableValue(fn) {}
};