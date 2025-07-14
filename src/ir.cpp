#include "ir.hpp"

#include <cassert>
#include <sstream>

namespace luaxc {
    std::string IRInstruction::dump() const {
        std::string out;

        const auto cvt_numeric = [](IRPrimValue value) {
            if (std::holds_alternative<int>(value)) {
                return std::to_string(std::get<int32_t>(value));
            } else if (std::holds_alternative<double>(value)) {
                return std::to_string(std::get<double>(value));
            } else {
                throw IRInterpreterException("Invalid IRPrimValue type");
            }
        };

        switch (type) {
            case IRInstruction::InstructionType::LOAD_CONST:
                out = "LOAD_CONST";
                out += " " + (cvt_numeric(std::get<IRLoadConstParam>(param)));
                break;
            case IRInstruction::InstructionType::LOAD_IDENTIFIER:
                out = "LOAD_IDENTIFIER";
                out += " " + std::get<IRLoadIdentifierParam>(param).identifier;
                break;
            case IRInstruction::InstructionType::STORE_IDENTIFIER:

                out = "STORE_IDENTIFIER";
                out += " " + std::get<IRStoreIdentifierParam>(param).identifier;
                break;
            case IRInstruction::InstructionType::ADD:
                out = "ADD";
                break;
            case IRInstruction::InstructionType::SUB:
                out = "SUB";
                break;
            case IRInstruction::InstructionType::MUL:
                out = "MUL";
                break;
            case IRInstruction::InstructionType::DIV:
                out = "DIV";
                break;
            case IRInstruction::InstructionType::MOD:
                out = "MOD";
                break;
            case IRInstruction::InstructionType::AND:
                out = "AND";
                break;
            case IRInstruction::InstructionType::LOGICAL_AND:
                out = "LOGICAL_AND";
                break;
            case IRInstruction::InstructionType::OR:
                out = "OR";
                break;
            case IRInstruction::InstructionType::LOGICAL_OR:
                out = "LOGICAL_OR";
                break;
            case IRInstruction::InstructionType::NOT:
                out = "NOT";
                break;
            case IRInstruction::InstructionType::LOGICAL_NOT:
                out = "LOGICAL_NOT";
                break;
            case IRInstruction::InstructionType::XOR:
                out = "XOR";
                break;
            case IRInstruction::InstructionType::NEGATE:
                out = "NEGATE";
                break;
            case IRInstruction::InstructionType::PUSH_STACK:
                out = "PUSH_STACK";
                break;
            case IRInstruction::InstructionType::POP_STACK:
                out = "POP_STACK";
                break;
            case IRInstruction::InstructionType::CMP_EQ:
                out = "CMP_EQ";
                break;
            case IRInstruction::InstructionType::CMP_NE:
                out = "CMP_NE";
                break;
            case IRInstruction::InstructionType::CMP_LT:
                out = "CMP_LT";
                break;
            case IRInstruction::InstructionType::CMP_GT:
                out = "CMP_GT";
                break;
            case IRInstruction::InstructionType::CMP_LE:
                out = "CMP_LE";
                break;
            case IRInstruction::InstructionType::CMP_GE:
                out = "CMP_GE";
                break;
            case IRInstruction::InstructionType::JMP:
                out = "JMP";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JMP_IF_FALSE:
                out = "JMP_IF_FALSE";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::TO_BOOL:
                out = "TO_BOOL";
                break;
            default:
                out = "UNKNOWN";
                break;
        }
        return out;
    }

    std::string dump_bytecode(const ByteCode& bytecode) {
        std::stringstream out;
        size_t line = 0;
        for (const auto& instruction : bytecode) {
            out << line << ": " << instruction.dump() << std::endl;
            line++;
        }
        return out.str();
    }

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
                return generate_unary_expression_statement(
                    static_cast<const UnaryExpressionNode *>(statement_node), byte_code);
                break;
            case StatementNode::StatementType::BlockStmt:
                return generate_program_or_block(statement_node, byte_code);
            case StatementNode::StatementType::IfStmt:
                return generate_if_statement(static_cast<const IfNode *>(statement_node), byte_code);
            case StatementNode::StatementType::WhileStmt:
                return generate_while_statement(static_cast<const WhileNode *>(statement_node), byte_code);
            case StatementNode::StatementType::BreakStmt:
                return generate_break_statement(byte_code);
            case StatementNode::StatementType::ContinueStmt:
                return generate_continue_statement(byte_code);
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
                    {IRLoadIdentifierParam{static_cast<const IdentifierNode *>(node)->get_name()}}));
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
        auto& identifiers = node->get_identifiers();

        // expr not present or has multiple identifiers declared.
        if (identifiers.size() > 1 || expr == nullptr) {
            assert(expr == nullptr);

            for (auto& identifier : identifiers) {
                auto* identifier_node = static_cast<IdentifierNode *>(identifier.get());
                byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::STORE_IDENTIFIER, 
                    IRStoreIdentifierParam{identifier_node->get_name()}));
            }
            return;
        }

        generate_expression(expr, byte_code);

        auto* identifier_node = static_cast<IdentifierNode *>(identifiers[0].get());

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::POP_STACK,
            {std::monostate()}
        ));
        
        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::STORE_IDENTIFIER, 
            IRStoreIdentifierParam{identifier_node->get_name()}));
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

    bool IRGenerator::is_binary_logical_operator(BinaryExpressionNode::BinaryOperator op) {
        return (op == BinaryExpressionNode::BinaryOperator::LogicalAnd || 
            op == BinaryExpressionNode::BinaryOperator::LogicalOr);
    }

    void IRGenerator::generate_binary_expression_statement(const BinaryExpressionNode* statement, ByteCode& byte_code) {
        const auto& left = statement->get_left();
        const auto& right = statement->get_right();

        auto node_op = statement->get_op();

        generate_expression(left.get(), byte_code);
        if (is_binary_logical_operator(node_op)) {
            // when the logical operator is used, we need to convert the lhs and rhs value to boolean
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        }
        
        generate_expression(right.get(), byte_code);
        if (is_binary_logical_operator(node_op)) {
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        }

        IRInstruction::InstructionType op_type;


        switch (node_op) {
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
            case BinaryExpressionNode::BinaryOperator::LogicalAnd:
                op_type = IRInstruction::InstructionType::LOGICAL_AND;
                break;
            case BinaryExpressionNode::BinaryOperator::BitwiseAnd:
                op_type = IRInstruction::InstructionType::AND;
                break;
            case BinaryExpressionNode::BinaryOperator::LogicalOr:
                op_type = IRInstruction::InstructionType::LOGICAL_OR;
                break;
            case BinaryExpressionNode::BinaryOperator::BitwiseOr:
                op_type = IRInstruction::InstructionType::OR;
                break;
            case BinaryExpressionNode::BinaryOperator::BitwiseXor:
                op_type = IRInstruction::InstructionType::XOR;
                break;
            case BinaryExpressionNode::BinaryOperator::BitwiseShiftLeft:
                op_type = IRInstruction::InstructionType::SHL;
                break;
            case BinaryExpressionNode::BinaryOperator::BitwiseShiftRight:
                op_type = IRInstruction::InstructionType::SHR;
                break;
            
            case BinaryExpressionNode::BinaryOperator::Equal:
                op_type = IRInstruction::InstructionType::CMP_EQ;
                break;
            case BinaryExpressionNode::BinaryOperator::NotEqual:
                op_type = IRInstruction::InstructionType::CMP_NE;
                break;
            case BinaryExpressionNode::BinaryOperator::LessThan:
                op_type = IRInstruction::InstructionType::CMP_LT;
                break;
            case BinaryExpressionNode::BinaryOperator::LessThanEqual:
                op_type = IRInstruction::InstructionType::CMP_LE;
                break;
            case BinaryExpressionNode::BinaryOperator::GreaterThan:
                op_type = IRInstruction::InstructionType::CMP_GT;
                break;
            case BinaryExpressionNode::BinaryOperator::GreaterThanEqual:
                op_type = IRInstruction::InstructionType::CMP_GE;
                break;
            default:
                throw IRGeneratorException("Unsupported binary operator");
        }

        byte_code.push_back(IRInstruction(
            op_type, 
            { std::monostate() }
        ));
    }

    void IRGenerator::generate_unary_expression_statement(const UnaryExpressionNode* statement, ByteCode& byte_code) {
        const auto& operand = statement->get_operand();

        generate_expression(operand.get(), byte_code);
        if (statement->get_operator() == UnaryExpressionNode::UnaryOperator::BitwiseNot) {
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        }

        IRInstruction::InstructionType op_type;
        switch (statement->get_operator()) {
            case UnaryExpressionNode::UnaryOperator::Minus:
                op_type = IRInstruction::InstructionType::NEGATE;
                break;
            case UnaryExpressionNode::UnaryOperator::BitwiseNot:
                op_type = IRInstruction::InstructionType::NOT;
                break;
            case UnaryExpressionNode::UnaryOperator::LogicalNot:
                op_type = IRInstruction::InstructionType::LOGICAL_NOT;
                break;
            case UnaryExpressionNode::UnaryOperator::Plus:
                break;
            default:
                throw IRGeneratorException("Unsupported unary operator");
        }
        byte_code.push_back(IRInstruction(op_type, {std::monostate()}));
    }

    void IRGenerator::generate_if_statement(const IfNode* statement, ByteCode& byte_code) {
        auto* expr = statement->get_condition().get();

        generate_expression(expr, byte_code);

        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::POP_STACK, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_IF_FALSE, {std::monostate()}));
        
        bool has_else_clause = statement->get_else_body().get() != nullptr;

        size_t zero_index = byte_code.size() - 1; // j*
        size_t if_end_index = 0;
        size_t else_clause_index = 0;

        generate_statement(static_cast<StatementNode *>(statement->get_body().get()), byte_code);
        if_end_index = byte_code.size();

        if (has_else_clause) {
            // in this case the 'end of if clause' is the jump to the end of the if statement
            // as we don't want it to jump directly to the end, so we add the offset by 1
            byte_code[zero_index].param = IRJumpParam(if_end_index + 1);

            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP, IRJumpParam(0)));
            generate_statement(static_cast<StatementNode *>(statement->get_else_body().get()), byte_code);
            else_clause_index = byte_code.size();
            byte_code[if_end_index].param = IRJumpParam(else_clause_index);
        } else {
            byte_code[zero_index].param = IRJumpParam(if_end_index);
        }
    }

    void IRGenerator::generate_while_statement(const WhileNode* statement, ByteCode& byte_code) {
        auto* expr = statement->get_condition().get();

        size_t while_start_index, while_end_index, while_start_jmp_index;
        while_start_index = byte_code.size();

        generate_expression(expr, byte_code);

        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::POP_STACK, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_IF_FALSE, {std::monostate()}));

        while_start_jmp_index = byte_code.size() - 1;
        
        while_loop_generation_stack.push(WhileLoopGenerationContext {});

        generate_statement(
            static_cast<StatementNode *>(statement->get_body().get()), 
            byte_code);

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::JMP, 
            IRJumpParam(while_start_index)));
        
        // the first command after the loop
        while_end_index = byte_code.size();

        // fill in jump target for unmet condition
        byte_code[while_start_jmp_index].param = IRJumpParam(while_end_index);

        auto generation_ctx = while_loop_generation_stack.top();
        while_loop_generation_stack.pop();
        // fill in jump targets of breaks
        for (auto break_instruction : generation_ctx.break_instructions) {
            byte_code[break_instruction].param = IRJumpParam(while_end_index);
        }

        // fill in jump targets of continues
        for (auto continue_instruction : generation_ctx.continue_instructions) {
            byte_code[continue_instruction].param = IRJumpParam(while_start_index);
        }
    }

    void IRGenerator::generate_break_statement(ByteCode& byte_code) {
        assert(while_loop_generation_stack.size() > 0);

        auto& ctx = while_loop_generation_stack.top();
        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::JMP, IRJumpParam(0)));
        ctx.register_break_instruction(byte_code.size() - 1);
    }

    void IRGenerator::generate_continue_statement(ByteCode& byte_code) {
        assert(while_loop_generation_stack.size() > 0);

        auto& ctx = while_loop_generation_stack.top();
        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::JMP, IRJumpParam(0)));
        ctx.register_continue_instruction(byte_code.size() - 1);
    }

    void IRInterpreter::run() {
        size_t size = byte_code.size();
        while (pc < size) {
            bool jumped = false;

            auto& instruction = byte_code[pc];
            switch (instruction.type) {
                case IRInstruction::InstructionType::LOAD_CONST:
                    output = std::get<IRLoadConstParam>(instruction.param);
                    break;
                case IRInstruction::InstructionType::LOAD_IDENTIFIER: {
                        auto param = std::get<IRLoadIdentifierParam>(instruction.param);
                        if (variables.find(param.identifier) != variables.end()) {
                            output = variables[param.identifier];
                        } else {
                            throw IRInterpreterException("Identifier not found: " + param.identifier);
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

                case IRInstruction::InstructionType::TO_BOOL: 
                    handle_to_bool();
                    break;

                case IRInstruction::InstructionType::ADD: 
                case IRInstruction::InstructionType::SUB:
                case IRInstruction::InstructionType::MUL:
                case IRInstruction::InstructionType::DIV:
                case IRInstruction::InstructionType::MOD:

                case IRInstruction::InstructionType::AND:
                case IRInstruction::InstructionType::OR:
                case IRInstruction::InstructionType::XOR:
                case IRInstruction::InstructionType::SHL:
                case IRInstruction::InstructionType::SHR:

                case IRInstruction::InstructionType::LOGICAL_AND:
                case IRInstruction::InstructionType::LOGICAL_OR:

                case IRInstruction::InstructionType::CMP_EQ:
                case IRInstruction::InstructionType::CMP_NE:
                case IRInstruction::InstructionType::CMP_LT:
                case IRInstruction::InstructionType::CMP_LE:
                case IRInstruction::InstructionType::CMP_GT:
                case IRInstruction::InstructionType::CMP_GE:
                    handle_binary_op(instruction.type);
                    break;

                case IRInstruction::InstructionType::NOT:
                case IRInstruction::InstructionType::LOGICAL_NOT:
                case IRInstruction::InstructionType::NEGATE:
                    handle_unary_op(instruction.type);
                    break;

                case IRInstruction::InstructionType::JMP:
                case IRInstruction::InstructionType::JMP_IF_FALSE:
                    jumped = handle_jump(instruction.type, std::get<IRJumpParam>(instruction.param));
                    break;
                
                default:
                    throw IRInterpreterException("Invalid instruction type");
            }

            if (!jumped) {
                pc++;
            }
        }
    }

    bool IRInterpreter::handle_jump(IRInstruction::InstructionType op, IRJumpParam param) {
        switch (op) {
            case IRInstruction::InstructionType::JMP:
                pc = param;
                return true;
            case IRInstruction::InstructionType::JMP_IF_FALSE: {
                auto cond = std::get<int32_t>(output);
                if (!(cond & 1)) {
                    // we only care about the first bit
                    // as, if everything works fine, there should be a TO_BOOL before this.
                    pc = param;
                    return true;
                }
                break;
            }
            default:
                throw IRInterpreterException("Unknown instruction type");
        }
        return false;
    }

    void IRInterpreter::handle_to_bool() {
        auto& top = stack.top();
        if (std::holds_alternative<int32_t>(top)) {
            stack.top() = std::get<int32_t>(top) != 0;
        } else if (std::holds_alternative<double>(top)) {
            stack.top() = std::get<double>(top) != 0.0;
        } else {
            throw IRInterpreterException("Cannot convert to bool or not implemented yet");
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

    void IRInterpreter::handle_unary_op(IRInstruction::InstructionType op) {
        auto rhs_variant = stack.top();
        stack.pop();

        if (std::holds_alternative<double>(rhs_variant)) {
            double rhs_double = std::get<double>(rhs_variant);
            handle_unary_op(op, rhs_double);
        } else if (std::holds_alternative<int32_t>(rhs_variant)) { 
            int32_t rhs_int = std::get<int32_t>(rhs_variant);
            handle_unary_op(op, rhs_int);
        } else {
            throw IRInterpreterException("Unary operation not supported for this type");
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