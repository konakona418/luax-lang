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
                    return std::stoi(value.substr(0, pos_of_u_mark));
                } else {
                    return std::stoi(value.substr(0, pos_of_i_mark));
                }
            }
            return std::stoi(value);
        } else {
            return std::stod(value);
        }
    }
}