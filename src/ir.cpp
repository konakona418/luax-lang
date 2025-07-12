#include "ir.hpp"

namespace luaxc {
    ByteCode IRGenerator::generate() {
        ByteCode byte_code;

        auto program_node = static_cast<const ProgramNode *>(ast.get());
        for (const auto& statement : program_node->get_statements()) {
            auto statement_node = static_cast<const StatementNode *>(statement.get());
            generate_statement(statement_node, byte_code);
        }

        return byte_code;
    }

    void IRGenerator::generate_statement(const StatementNode* statement_node, ByteCode& byte_code) { 
        switch (statement_node->get_type()) {
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
            default:
                throw IRGeneratorException("Unsupported expression type");
        }
    }

    void IRGenerator::generate_declaration_statement(const DeclarationStmtNode* node, ByteCode& byte_code) {
        auto expr = static_cast<const StatementNode *>(node->get_value_or_initializer());
        auto identifier = static_cast<const IdentifierNode *>(node->get_identifier().get());

        // todo: the initializer value can be non-present
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

        // todo: this does not handle when an operand is an identifier
        if (left->get_type() == AstNodeType::NumericLiteral) {
            byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::LOAD_CONST,
                {IRLoadConstParam(static_cast<const NumericLiteralNode *>(left.get())->get_value())}));
            byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::PUSH_STACK,
                {std::monostate()}));
        } else {
            generate_binary_expression_statement(static_cast<const BinaryExpressionNode *>(left.get()), byte_code);
        }

        if (right->get_type() == AstNodeType::NumericLiteral) {
            byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::LOAD_CONST,
                {IRLoadConstParam(static_cast<const NumericLiteralNode *>(right.get())->get_value())}));
            byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::PUSH_STACK,
                {std::monostate()}));
        } else {
            generate_binary_expression_statement(static_cast<const BinaryExpressionNode *>(right.get()), byte_code);
        }

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
            default:
                throw IRGeneratorException("Unsupported binary operator");
        }

        byte_code.push_back(IRInstruction(
            op_type, 
            { std::monostate() }
        ));
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
                    handle_binary_op(instruction.type);
                    break;
                default:
                    throw IRInterpreterException("Invalid instruction type");
            }
            pc++;
        }
    }

    void IRInterpreter::handle_binary_op(IRInstruction::InstructionType op) {
        auto lhs_variant = stack.top();
        stack.pop();
        auto rhs_variant = stack.top();
        stack.pop();

        if (std::holds_alternative<int32_t>(lhs_variant) && std::holds_alternative<int32_t>(rhs_variant)) {
            handle_binary_op(op, std::get<int32_t>(lhs_variant), std::get<int32_t>(rhs_variant));
        } else if (std::holds_alternative<double>(lhs_variant) && std::holds_alternative<double>(rhs_variant)) {
            handle_binary_op(op, std::get<double>(lhs_variant), std::get<double>(rhs_variant));
        } else {
            double lhs_double = std::get<double>(lhs_variant);
            double rhs_double = std::get<double>(rhs_variant);
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
}