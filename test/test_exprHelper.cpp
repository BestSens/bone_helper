#include "catch2/catch.hpp"
#include "../exprHelper.hpp"

TEST_CASE("exprHelper_test") {
    bestsens::exprHelper * expr = new bestsens::exprHelper();

    SECTION("basic") {
        /*
         * test simple math
         */
        expr->update_expression("1+2");
        CHECK(expr->eval() == 3);

        /*
         * test with variable
         */
        double x = 3;
        std::vector<std::pair<std::string, void*>> variables = {
            std::make_pair("x", &x),
        };

        expr->update_expression("1+x", variables);
        CHECK(expr->eval() == 4);

        /*
         * update variable
         */
        x = 4;
        CHECK(expr->eval() == 5);

        /*
         * update expression without updating variables
         */
        expr->update_expression("2+x");
        CHECK(expr->eval() == 6);

        /*
         * update variables
         */
        double y = 5;
        variables = {
            std::make_pair("x", &y)
        };

        expr->update_variables(variables);
        CHECK(expr->eval() == 7);

        /*
         * test parse error
         */
        CHECK(expr->update_expression("2+(x") != 0);
        CHECK(expr->update_expression("2+y") != 0);
    }

    SECTION("compare") {
        expr->update_expression("gt(1,2)");
        CHECK(expr->eval() == 0);
        expr->update_expression("gt(2,2)");
        CHECK(expr->eval() == 0);
        expr->update_expression("gt(2,1)");
        CHECK(expr->eval() == 1);

        expr->update_expression("ge(1,2)");
        CHECK(expr->eval() == 0);
        expr->update_expression("ge(2,2)");
        CHECK(expr->eval() == 1);
        expr->update_expression("ge(2,1)");
        CHECK(expr->eval() == 1);

        expr->update_expression("lt(1,2)");
        CHECK(expr->eval() == 1);
        expr->update_expression("lt(2,2)");
        CHECK(expr->eval() == 0);
        expr->update_expression("lt(2,1)");
        CHECK(expr->eval() == 0);

        expr->update_expression("le(1,2)");
        CHECK(expr->eval() == 1);
        expr->update_expression("le(2,2)");
        CHECK(expr->eval() == 1);
        expr->update_expression("le(2,1)");
        CHECK(expr->eval() == 0);

        expr->update_expression("eq(1,2)");
        CHECK(expr->eval() == 0);
        expr->update_expression("eq(2,2)");
        CHECK(expr->eval() == 1);
    }
}
