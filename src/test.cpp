#include <iostream>
#include "expr.h"
#include "AstPrinter.h"

int main() {
    // Test case: -123 * (45.67)
    Expr* expression = new Binary(
        new Unary(
            Token(MINUS, "-", "", 1),
            new Literal("123")
        ),
        Token(STAR, "*", "", 1),
        new Grouping(
            new Literal("45.67")
        )
    );

    AstPrinter printer;
    std::cout << printer.print(expression) << std::endl;

    delete expression; 
    return 0;
}