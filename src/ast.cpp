#include "ast.hpp"

namespace luaxc {
    void ProgramNode::add_statement(std::unique_ptr<AstNode> statement) {
        statements.push_back(std::move(statement));
    }

    NumericLiteralNode::NumericVariant 
    NumericLiteralNode::parse_numeric_literal(const std::string& value, NumericLiteralType type) {
        if (type == NumericLiteralType::Integer) {
            auto pos_of_u_mark = value.find_first_of('u');
            auto pos_of_i_mark = value.find_first_of('i');

            if (pos_of_u_mark != std::string::npos || pos_of_i_mark != std::string::npos) {
                if (pos_of_u_mark != std::string::npos) {
                    return PrimValue::from_i64(std::stoll(value.substr(0, pos_of_u_mark)));
                } else {
                    return PrimValue::from_i64(std::stoll(value.substr(0, pos_of_i_mark)));
                }
            }
            return PrimValue::from_i64(std::stoi(value));
        } else {
            return PrimValue::from_f64(std::stod(value));
        }
    }
}