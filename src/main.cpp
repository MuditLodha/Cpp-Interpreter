#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "scanner.h"
#include "parser.h"
#include "expr.h"
#include "AstPrinter.h"
#include "token.h"
#include "value.h"
#include "Stmt.h"
#include "errors.h"
#include "Interpreter.h"
#include "Resolver.h"

using namespace std;

static bool hadError = false;
static bool hadRuntimeError = false;

void report(int line, const string& where, const string& message) {
    cerr << "[line " << line << "] Error" << where << ": " << message << endl;
    hadError = true;
}

void error(Token token, const string& message) {
    if (token.type == END_OF_FILE) {
        report(token.line, " at end", message);
    } else {
        report(token.line, " at '" + token.lexeme + "'", message);
    }
}

void error(int line, const string& message) {
    report(line, "", message);
}

void runtimeError(const RuntimeError& error) {
    cerr << "[line " << error.token.line << "] Runtime error: "
         << error.what() << endl;
    hadRuntimeError = true;
}

void run(const string& source); // forward declaration

void runFile(const string& path) {
    ifstream file(path);

    if (!file.is_open()) {
        cerr << "Error: Could not open file " << path << endl;
        return;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string source = buffer.str();

    run(source);
    if (hadError) exit(65);
    if (hadRuntimeError) exit(70);
}

void runPrompt() {
    string line;
    while (true) {
        cout << "> ";

        if (!getline(cin, line)) break;  // EOF (Ctrl+D / Ctrl+Z)

        run(line);
        hadError = false;         // reset for next line
        hadRuntimeError = false;  // reset for next line
    }
}

void run(const string& source) {
    Scanner scanner(source);
    vector<Token> tokens = scanner.scanTokens();

    if (hadError) return;

    Parser parser(tokens);
    vector<Stmt*> statements = parser.parse();

    if (hadError) return;

    Interpreter interpreter;

    // --- Resolver pass (Chapter 11) ---
    // Walk the AST once before running to compute scope distances.
    // Also catches static errors: var a = a; and top-level return.
    Resolver resolver(interpreter);
    resolver.resolve(statements);

    if (hadError) return;   // stop if resolver found a static error

    interpreter.interpret(statements);

    for (Stmt* stmt : statements) {
        delete stmt;
    }
}

int main(int argc, char* argv[]) {
    if (argc > 2) {
        cout << "Usage: compiler [script]\n";
        return 64;
    }
    else if (argc == 2) {
        runFile(argv[1]);
    }
    else {
        runPrompt();
    }

    return 0;
}