#pragma once

#include <unordered_map>
#include <string>
#include <memory>

#include "value.h"
#include "token.h"
#include "errors.h"

// Environment — holds variable bindings for one scope.
// Each environment has a shared_ptr to its enclosing (outer) scope.
// Using shared_ptr means closures can keep their captured scope alive
// even after the function that created it has returned.

class Environment {
public:
    std::shared_ptr<Environment> enclosing;   // nullptr for global scope

    // global scope
    Environment() : enclosing(nullptr) {}

    // local scope — takes shared_ptr so lifetime is managed automatically
    Environment(std::shared_ptr<Environment> enclosing) : enclosing(enclosing) {}

    //////////////////////////////////////////////////////
    // Define a new variable in THIS scope
    //////////////////////////////////////////////////////
    void define(const std::string& name, const Value& value) {
        values[name] = value;
    }

    //////////////////////////////////////////////////////
    // Look up a variable — walk up the scope chain
    //////////////////////////////////////////////////////
    Value get(const Token& name) {
        if (values.find(name.lexeme) != values.end()) {
            return values[name.lexeme];
        }

        if (enclosing != nullptr) {
            return enclosing->get(name);
        }

        throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    //////////////////////////////////////////////////////
    // Assign to an existing variable — walk up the scope chain
    //////////////////////////////////////////////////////
    void assign(const Token& name, const Value& value) {
        if (values.find(name.lexeme) != values.end()) {
            values[name.lexeme] = value;
            return;
        }

        if (enclosing != nullptr) {
            enclosing->assign(name, value);
            return;
        }

        throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }

    //////////////////////////////////////////////////////
    // Direct-by-distance access — used by the Interpreter
    // after the Resolver has computed how far up to go.
    //////////////////////////////////////////////////////

    // Walk exactly 'distance' hops up the chain and return that env.
    Environment* ancestor(int distance) {
        Environment* env = this;
        for (int i = 0; i < distance; i++) {
            env = env->enclosing.get();
        }
        return env;
    }

    // Read a variable that is exactly 'distance' scopes out.
    Value getAt(int distance, const std::string& name) {
        return ancestor(distance)->values[name];
    }

    // Write a variable that is exactly 'distance' scopes out.
    void assignAt(int distance, const Token& name, const Value& value) {
        ancestor(distance)->values[name.lexeme] = value;
    }

private:
    std::unordered_map<std::string, Value> values;
};