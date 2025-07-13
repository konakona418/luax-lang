#include "ir.hpp"

namespace luaxc {
    ByteCode IRGenerator::generate() {
        ByteCode byte_code;

        generate_program_or_block(ast.get(), byte_code);

        return byte_code;
    }

    void IRGenerator::generate_program_or_block(const AstNode* node, ByteCode& byte_code) {
        if (node->get_type() == AstNodeType::Program) { 
            for (const auto& statement : static_cast<const ProgramNode *>(node)->get_statements()) {
                auto statement_node = static_cast<const StatementNode *>(statement.get());
                generate_statement(statement_node, byte_code);
            }
        } else if (node->get_type() == AstNodeType::Stmt) { 
            auto stmt = static_cast<const StatementNode *>(node);
            if (stmt->get_statement_type() != StatementNode::StatementType::BlockStmt) { 
                throw IRGeneratorException("generate_program_or_block requires a proper block statement");
            }

            for (const auto& statement : static_cast<const BlockNode *>(node)->get_statements()) {
                auto statement_node = static_cast<const StatementNode *>(statement.get());
                generate_statement(statement_node, byte_code);
            }
        } else {
            throw IRGeneratorException("generate_program_or_block requires either a program or block statement");
        }
    }

    void IRGenerator::generate_statement(const StatementNode* statement_node, ByteCode& byte_code) { 
        switch (statement_node->get_statement_type()) {
            case StatementNode::StatementType::DeclarationStmt:
                generate_declaration_statement(
                    static_cast<const DeclarationStmtNode*>(statement_node), byte_code);
                break;
            case StatementNode::StatementType::AssignmentStmt:
                generate_assignment_statement(
                    static_cast<const AssignmentStmtNode*>(statement_node), byte_code);
                break;
            case StatementNode::StatementType::BinaryExprStmt:
                return generate_binary_expression_statement(
                    static_cast<const BinaryExpressionNode *>(statement_node), byte_code);
                break;
            case StatementNode::StatementType::UnaryExprStmt: 
                throw IRGeneratorException("Unsupported statement type");
                break;
            case StatementNode::StatementType::BlockStmt:
                return generate_program_or_block(statement_node, byte_code);
            case StatementNode::StatementType::IfStmt:
                return generate_if_statement(static_cast<const IfNode *>(statement_node), byte_code);
            default:
                throw IRGeneratorException("Unsupported statement type");
        }
    }

    void IRGenerator::generate_expression(const AstNode* node, ByteCode& byte_code) {
        switch (node->get_type()) {
            case AstNodeType::Stmt:
                generate_statement(static_cast<const StatementNode *>(node), byte_code);
                break;
            case AstNodeType::NumericLiteral: 
                byte_code.push_back(
                    IRInstruction(IRInstruction::InstructionType::LOAD_CONST, 
                    {IRLoadConstParam(static_cast<const NumericLiteralNode *>(node)->get_value())}));
                byte_code.push_back(
                    IRInstruction(IRInstruction::InstructionType::PUSH_STACK, 
                    {std::monostate()}));
                break;
            case AstNodeType::Identifier:
                byte_code.push_back(
                    IRInstruction(IRInstruction::InstructionType::LOAD_IDENTIFIER, 
                    {IRLoadIdentifierParam(static_cast<const IdentifierNode *>(node)->get_name())}));
                byte_code.push_back(
                    IRInstruction(IRInstruction::InstructionType::PUSH_STACK, 
                    {std::monostate()}));
                break;
            default:
                throw IRGeneratorException("Unsupported expression type");
        }
    }

    void IRGenerator::generate_declaration_statement(const DeclarationStmtNode* node, ByteCode& byte_code) {
        auto expr = static_cast<const StatementNode *>(node->get_value_or_initializer());
        auto identifier = static_cast<const IdentifierNode *>(node->get_identifier().get());

        // expr not present
        if (expr == nullptr) {
            // by this time, the value of the identifier is not certain
            byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::STORE_IDENTIFIER, 
                IRStoreIdentifierParam{identifier->get_name()}));

            return;
        }

        generate_expression(expr, byte_code);

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::POP_STACK,
            {std::monostate()}
        ));
        
        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::STORE_IDENTIFIER, 
            IRStoreIdentifierParam{identifier->get_name()}));
    }

    void IRGenerator::generate_assignment_statement(const AssignmentStmtNode* node, ByteCode& byte_code) { 
        auto expr = static_cast<const StatementNode *>(node->get_value().get());
        auto identifier = static_cast<const IdentifierNode *>(node->get_identifier().get());
        
        generate_expression(expr, byte_code);

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::POP_STACK,
            {std::monostate()}
        ));
        
        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::STORE_IDENTIFIER, 
            IRStoreIdentifierParam{identifier->get_name()}));
    }

    void IRGenerator::generate_binary_expression_statement(const BinaryExpressionNode* node, ByteCode& byte_code) {
        const auto& left = node->get_left();
        const auto& right = node->get_right();

        generate_expression(left.get(), byte_code);
        generate_expression(right.get(), byte_code);

        IRInstruction::InstructionType op_type;
        switch (node->get_op()) {
            case BinaryExpressionNode::BinaryOperator::Add:
                op_type = IRInstruction::InstructionType::ADD;
                break;
            case BinaryExpressionNode::BinaryOperator::Subtract:
                op_type = IRInstruction::InstructionType::SUB;
                break;
            case BinaryExpressionNode::BinaryOperator::Multiply:
                op_type = IRInstruction::InstructionType::MUL;
                break;
            case BinaryExpressionNode::BinaryOperator::Divide:
                op_type = IRInstruction::InstructionType::DIV;
                break;
            case BinaryExpressionNode::BinaryOperator::Modulo:
                op_type = IRInstruction::InstructionType::MOD;
                break;
            case BinaryExpressionNode::BinaryOperator::Equal:
            case BinaryExpressionNode::BinaryOperator::NotEqual:
            case BinaryExpressionNode::BinaryOperator::LessThan:
            case BinaryExpressionNode::BinaryOperator::LessThanEqual:
            case BinaryExpressionNode::BinaryOperator::GreaterThan:
            case BinaryExpressionNode::BinaryOperator::GreaterThanEqual:
                op_type = IRInstruction::InstructionType::CMP;
                break;
            default:
                throw IRGeneratorException("Unsupported binary operator");
        }

        byte_code.push_back(IRInstruction(
            op_type, 
            { std::monostate() }
        ));
    }

    void IRGenerator::generate_if_statement(const IfNode* statement, ByteCode& byte_code) {
        auto* expr = statement->get_condition().get();

        generate_expression(expr, byte_code);

        if (expr->get_type() == AstNodeType::Stmt) {
            auto* stmt = dynamic_cast<StatementNode*>(expr);
            if (stmt->get_statement_type() == StatementNode::StatementType::BinaryExprStmt) {
                IRInstruction::InstructionType jmp_op;
                auto* binary_expr = dynamic_cast<BinaryExpressionNode*>(stmt);

                // we want to jump when the result of the expression is false,
                // so the ir operator is the opposite of the binary operator
                switch (binary_expr->get_op()) {
                    case BinaryExpressionNode::BinaryOperator::Equal:
                        jmp_op = IRInstruction::InstructionType::JNE;
                        break;
                    case BinaryExpressionNode::BinaryOperator::NotEqual:
                        jmp_op = IRInstruction::InstructionType::JE;
                        break;
                    case BinaryExpressionNode::BinaryOperator::LessThan:
                        jmp_op = IRInstruction::InstructionType::JGE;
                        break;
                    case BinaryExpressionNode::BinaryOperator::LessThanEqual:
                        jmp_op = IRInstruction::InstructionType::JG;
                        break;
                    case BinaryExpressionNode::BinaryOperator::GreaterThan:
                        jmp_op = IRInstruction::InstructionType::JLE;
                        break;
                    case BinaryExpressionNode::BinaryOperator::GreaterThanEqual:
                        jmp_op = IRInstruction::InstructionType::JL;
                        break;
                    default:
                        // when the value is not 0
                        jmp_op = IRInstruction::InstructionType::JE;
                        //throw IRGeneratorException("Invalid binary operator");
                        break;
                }
                // pop a value from stack,
                // get ready to compare and jump
                byte_code.push_back(IRInstruction(IRInstruction::InstructionType::POP_STACK, {std::monostate()}));
                byte_code.push_back(IRInstruction(jmp_op, {std::monostate()}));
            }
        }
        
        bool has_else_clause = statement->get_else_body().get() != nullptr;

        size_t zero_index = byte_code.size() - 1; // j*
        size_t if_end_index = 0;
        size_t else_clause_index = 0;

        generate_statement(static_cast<StatementNode *>(statement->get_body().get()), byte_code);
        if_end_index = byte_code.size();
        byte_code[zero_index].param = IRJumpParam(if_end_index);

        if (has_else_clause) {
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP, IRJumpParam(0)));
            generate_statement(static_cast<StatementNode *>(statement->get_else_body().get()), byte_code);
            else_clause_index = byte_code.size();
            byte_code[if_end_index].param = IRJumpParam(else_clause_index);
        }
    }

    void IRInterpreter::run() {
        size_t size = byte_code.size();
        while (pc < size) {
            auto& instruction = byte_code[pc];
            switch (instruction.type) {
                case IRInstruction::InstructionType::LOAD_CONST:
                    output = std::get<IRLoadConstParam>(instruction.param);
                    break;
                case IRInstruction::InstructionType::LOAD_IDENTIFIER: {
                        auto param = std::get<IRLoadIdentifierParam>(instruction.param);
                        if (variables.find(param) != variables.end()) {
                            output = variables[param];
                        } else {
                            throw IRInterpreterException("Identifier not found: " + param);
                        }
                        break;
                    }
                case IRInstruction::InstructionType::STORE_IDENTIFIER: {
                        auto param = std::get<IRStoreIdentifierParam>(instruction.param);
                        variables[param.identifier] = output;
                        break;
                    }
                case IRInstruction::InstructionType::PUSH_STACK: {
                        stack.push(output);
                        break;
                    }
                case IRInstruction::InstructionType::POP_STACK: {
                        output = stack.top();
                        stack.pop();
                        break;
                    }
                case IRInstruction::InstructionType::ADD: 
                case IRInstruction::InstructionType::SUB:
                case IRInstruction::InstructionType::MUL:
                case IRInstruction::InstructionType::DIV:
                case IRInstruction::InstructionType::MOD:

                case IRInstruction::InstructionType::CMP:
                    handle_binary_op(instruction.type);
                    break;

                case IRInstruction::InstructionType::JE:
                case IRInstruction::InstructionType::JNE:
                case IRInstruction::InstructionType::JL:
                case IRInstruction::InstructionType::JLE:
                case IRInstruction::InstructionType::JG:
                case IRInstruction::InstructionType::JGE:
                case IRInstruction::InstructionType::JMP:
                    handle_jump(instruction.type, std::get<IRJumpParam>(instruction.param));
                    break;
                
                default:
                    throw IRInterpreterException("Invalid instruction type");
            }
            pc++;
        }
    }

    void IRInterpreter::handle_jump(IRInstruction::InstructionType op, IRJumpParam param) {
        auto cmp = std::get<int32_t>(output);
        switch (op) {
            case IRInstruction::InstructionType::JMP:
                pc = param;
                break;
            case IRInstruction::InstructionType::JE:
                if (cmp == 0) {
                    pc = param;
                }
                break;
            case IRInstruction::InstructionType::JNE: 
                if (cmp != 0) {
                    pc = param;
                }
                break;
            case IRInstruction::InstructionType::JL:
                if (cmp < 0) {
                    pc = param;
                }
                break;
            case IRInstruction::InstructionType::JLE:
                if (cmp <= 0) {
                    pc = param;
                }
                break;
            case IRInstruction::InstructionType::JG:
                if (cmp > 0) {
                    pc = param;
                }
                break;
            case IRInstruction::InstructionType::JGE:
                if (cmp >= 0) {
                    pc = param;
                }
                break;
            default:
                throw IRInterpreterException("Unknown instruction type");
        }
    }

    void IRInterpreter::handle_binary_op(IRInstruction::InstructionType op) {
        auto rhs_variant = stack.top();
        stack.pop();
        auto lhs_variant = stack.top();
        stack.pop();
        // the left node is visited first and entered the stack first,
        // so the right node is the top of the stack

        if (std::holds_alternative<int32_t>(lhs_variant) && std::holds_alternative<int32_t>(rhs_variant)) {
            handle_binary_op(op, std::get<int32_t>(lhs_variant), std::get<int32_t>(rhs_variant));
        } else if (std::holds_alternative<double>(lhs_variant) && std::holds_alternative<double>(rhs_variant)) {
            handle_binary_op(op, std::get<double>(lhs_variant), std::get<double>(rhs_variant));
        } else {
            double lhs_double, rhs_double;

            if (std::holds_alternative<int32_t>(lhs_variant)) {
                lhs_double = static_cast<double>(std::get<int32_t>(lhs_variant));
            } else {
                lhs_double = std::get<double>(lhs_variant);
            }

            if (std::holds_alternative<int32_t>(rhs_variant)) {
                rhs_double = static_cast<double>(std::get<int32_t>(rhs_variant));
            } else {
                rhs_double = std::get<double>(rhs_variant);
            }

            handle_binary_op(op, lhs_double, rhs_double);
        }
    }

    IRPrimValue IRInterpreter::retrieve_value(const std::string& identifier) {
        if (variables.find(identifier) != variables.end()) {
            return variables[identifier];
        } else {
            throw IRInterpreterException("Identifier not found: " + identifier);
        }
    }

    bool IRInterpreter::has_identifier(const std::string& identifier) const {
        return variables.find(identifier) != variables.end();
    }
}