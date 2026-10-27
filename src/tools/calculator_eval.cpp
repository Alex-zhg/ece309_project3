// src/tools/calculator_eval.cpp — YOU IMPLEMENT (spec §4.6).
//
// calc_evaluate: evaluate a well-formed token list with two stacks
// (one for numbers, one for operators and '('). See spec §4.6 and the
// worked trace in Appendix C. std::stack or std::vector are both fine.

#include "tools/calculator.h"

#include <vector>

namespace {

using Kind = CalcToken::Kind;

int precedence(Kind k) {
    switch (k) {
        case Kind::Times:
        case Kind::Divide: return 2;
        case Kind::Plus:
        case Kind::Minus: return 1;
        default: return 0; // '(' and anything that is not an operator
    }
}

bool is_operator(Kind k) { return precedence(k) > 0; }

bool apply_top(std::vector<double>& values, std::vector<Kind>& ops, std::string* error) {
    if (ops.empty() || !is_operator(ops.back()) || values.size() < 2) {
        *error = "malformed expression";
        return false;
    }
    const Kind op = ops.back();
    ops.pop_back();
    const double right = values.back(); values.pop_back();
    const double left = values.back(); values.pop_back();
 
    double result = 0.0;
    switch (op) {
        case Kind::Plus: result = left + right; break;
        case Kind::Minus: result = left - right; break;
        case Kind::Times: result = left * right; break;
        case Kind::Divide:
            if (right == 0.0) {
                *error = "division by zero";
                return false;
            }
            result = left / right;
            break;
        default:
            *error = "malformed expression";
            return false;
    }
    values.push_back(result);
    return true;
}

}

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out, std::string* error) {
    std::string scratch;
    if (error == nullptr) error = &scratch;
    error->clear();

    std::vector<double> values;
    std::vector<Kind> ops;

    for (const CalcToken& t : tokens) {
        switch (t.kind) {
            case Kind::Number:
                values.push_back(t.value);
                break;
            case Kind::LParen:
                ops.push_back(Kind::LParen);
                break;
            case Kind::RParen:
                while (!ops.empty() && ops.back() != Kind::LParen)
                    if (!apply_top(values, ops, error)) return false;
                if (ops.empty()) {
                    *error = "unmatched ')'";
                    return false;
                }
                ops.pop_back();
                break;
            case Kind::Plus:
            case Kind::Minus:
            case Kind::Times:
            case Kind::Divide:
                while (!ops.empty() && is_operator(ops.back()) &&
                       precedence(ops.back()) >= precedence(t.kind))
                    if (!apply_top(values, ops, error)) return false;
                ops.push_back(t.kind);
                break;
        }
    }

    while (!ops.empty()) {
        if (ops.back() == Kind::LParen) { // leftover '('
            *error = "missing ')'";
            return false;
        }
        if (!apply_top(values, ops, error)) return false;
    }

    if (values.size() != 1) { // empty input, or numbers with no operator
        *error = "malformed expression";
        return false;
    }
    if (out != nullptr) *out = values.back();
    return true;
}
