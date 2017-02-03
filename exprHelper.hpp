/*
 * exprHelper.hpp
 *
 *  Created on: 01.02.2017
 *      Author: Jan Schöppach
 */

#ifndef EXPRHELPER_HPP_
#define EXPRHELPER_HPP_

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

#include "../tinyexpr/tinyexpr.h"

namespace bestsens {
    class exprHelper {
    public:
        exprHelper();
        exprHelper(std::string expression);
        ~exprHelper();

        int add_variable();
        int update_expression(const std::string expression, const std::vector<std::pair<std::string, void*>> &variables = std::vector<std::pair<std::string, void*>>());
        int update_variables(const std::vector<std::pair<std::string, void*>> &variables);

        double eval();
    private:
        te_expr * n;
        te_variable * vars;
        int vars_len;
        std::string expression;

        int compile();
    };

    exprHelper::exprHelper() {

    }

    exprHelper::exprHelper(std::string expression) {
        this->update_expression(expression);
    }

    exprHelper::~exprHelper() {
        te_free(this->n);
        free(this->vars);
    }

    int exprHelper::update_variables(const std::vector<std::pair<std::string, void*>> &variables) {
        if(this->vars == NULL)
            free(this->vars);

        this->vars = (te_variable*) malloc(variables.size() * sizeof(te_variable));

        auto add_variable = [this](int pos, const char* name, const void* address, int type = 0) {
            this->vars[pos].name = name;
            this->vars[pos].address = address;
            this->vars[pos].type = type;
        };

        int i = 0;
        for(auto element : variables)
            add_variable(i++, element.first.c_str(), element.second);

        return this->compile();
    }

    double exprHelper::eval() {
        double value = 0;

        if(this->n)
            value = te_eval(this->n);

        return value;
    }

    int exprHelper::compile() {
        int error;

        this->n = te_compile(this->expression.c_str(), this->vars, this->vars_len, &error);

        return error;
    }

    int exprHelper::update_expression(const std::string expression, const std::vector<std::pair<std::string, void*>> &variables) {
        this->expression = expression;

        if(variables.size() > 0)
            this->update_variables(variables);

        return this->compile();
    }
}

#endif /* EXPRHELPER_HPP_ */
