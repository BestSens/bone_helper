/*
 * exprHelper.hpp
 *
 *  Created on: 01.02.2017
 *	  Author: Jan Schöppach
 */

#ifndef EXPRHELPER_HPP_
#define EXPRHELPER_HPP_

#include <memory>
#include <string>
#include <vector>
#include <array>
#include <stdexcept>
#include <iostream>

#include "../tinyexpr/tinyexpr.h"

namespace bestsens {
	class exprHelper {
	public:
		exprHelper();
		explicit exprHelper(std::string expression);
		~exprHelper();

		int add_variable();
		int update_expression(const std::string expression, const std::vector<std::pair<std::string, void*>> &variables = std::vector<std::pair<std::string, void*>>());
		int update_variables(const std::vector<std::pair<std::string, void*>> &variables = std::vector<std::pair<std::string, void*>>());

		static double gt(double a, double b) { return a > b; }
		static double ge(double a, double b) { return a >= b; }
		static double lt(double a, double b) { return a < b; }
		static double le(double a, double b) { return a <= b; }
		static double eq(double a, double b) { return a == b; }

		double eval();
	private:
		std::shared_ptr<te_expr> expr;
		std::vector<te_variable> vars;
		std::string expression;

		int compile();
	};

	exprHelper::exprHelper() {
		this->update_variables();
	}

	exprHelper::exprHelper(std::string expression) {
		this->update_variables();
		this->update_expression(expression);
	}

	exprHelper::~exprHelper() {
	}

	int exprHelper::update_variables(const std::vector<std::pair<std::string, void*>> &variables) {
		this->vars.clear();

		auto add_variable = [this](const char* name, const void* address, int type = TE_VARIABLE) {
			te_variable temp = {
				name,
				address,
				type
			};

			this->vars.push_back(temp);
		};

		for(auto element : variables)
			add_variable(element.first.c_str(), element.second);

		/*
		 * add compare functions
		 */
		add_variable("gt", (const void*)exprHelper::gt, TE_FUNCTION2);
		add_variable("ge", (const void*)exprHelper::ge, TE_FUNCTION2);
		add_variable("lt", (const void*)exprHelper::lt, TE_FUNCTION2);
		add_variable("le", (const void*)exprHelper::le, TE_FUNCTION2);
		add_variable("eq", (const void*)exprHelper::eq, TE_FUNCTION2);

		return this->compile();
	}

	double exprHelper::eval() {
		double value = 0;

		if(this->expr)
			value = te_eval(this->expr.get());

		return value;
	}

	int exprHelper::compile() {
		int error;
		te_expr * n = te_compile(this->expression.c_str(), this->vars.data(), this->vars.size(), &error);

		if(error == 0)
			this->expr.reset(n, te_free);

		return error;
	}

	int exprHelper::update_expression(const std::string expression, const std::vector<std::pair<std::string, void*>> &variables) {
		if(this->expression.compare(expression) == 0)
			return 0;

		this->expression = expression;

		if(variables.size() > 0)
			this->update_variables(variables);

		return this->compile();
	}
} // namespace bestsens

#endif /* EXPRHELPER_HPP_ */
