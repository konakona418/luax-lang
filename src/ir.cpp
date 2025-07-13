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
            case IRInstruction::InstructionType::PUSH_STACK:
                out = "PUSH_STACK";
                break;
            case IRInstruction::InstructionType::POP_STACK:
                out = "POP_STACK";
                break;
            case IRInstruction::InstructionType::CMP:
                out = "CMP";
                break;
            case IRInstruction::InstructionType::JMP:
                out = "JMP";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JE:
                out = "JE";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JNE:
                out = "JNE";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JG:
                out = "JG";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JL:
                out = "JL";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JGE:
                out = "JGE";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JLE:
                out = "JLE";
                out += " " + std::to_string(std::get<IRJumpParam>(param));
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

        generate_program_or_block(ast.get(), byte_code, nullptr);

        return byte_code;
    }

    void IRGenerator::generate_program_or_block(const AstNode* node, ByteCode& byte_code, void* ctx) {
        if (node->get_type() == AstNodeType::Program) { 
            for (const auto& statement : static_cast<const ProgramNode *>(node)->get_statements()) {
                auto statement_node = static_cast<const StatementNode *>(statement.get());
                generate_statement(statement_node, byte_code, ctx);
            }
        } else if (node->get_type() == AstNodeType::Stmt) { 
            auto stmt = static_cast<const StatementNode *>(node);
            if (stmt->get_statement_type() != StatementNode::StatementType::BlockStmt) { 
                throw IRGeneratorException("generate_program_or_block requires a proper block statement");
            }

            for (const auto& statement : static_cast<const BlockNode *>(node)->get_statements()) {
                auto statement_node = static_cast<const StatementNode *>(statement.get());
                generate_statement(statement_node, byte_code, ctx);
            }
        } else {
            throw IRGeneratorException("generate_program_or_block requires either a program or block statement");
        }
    }

    void IRGenerator::generate_statement(const StatementNode* statement_node, ByteCode& byte_code, void* ctx) { 
        switch (statement_node->get_statement_type()) {
            case StatementNode::StatementType::DeclarationStmt:
                generate_declaration_statement(
                    static_cast<const DeclarationStmtNode*>(statement_node), byte_code, ctx);
                break;
            case StatementNode::StatementType::AssignmentStmt:
                generate_assignment_statement(
                    static_cast<const AssignmentStmtNode*>(statement_node), byte_code, ctx);
                break;
            case StatementNode::StatementType::BinaryExprStmt:
                return generate_binary_expression_statement(
                    static_cast<const BinaryExpressionNode *>(statement_node), byte_code, ctx);
                break;
            case StatementNode::StatementType::UnaryExprStmt: 
                throw IRGeneratorException("Unsupported statement type");
                break;
            case StatementNode::StatementType::BlockStmt:
                return generate_program_or_block(statement_node, byte_code, ctx);
            case StatementNode::StatementType::IfStmt:
                return generate_if_statement(static_cast<const IfNode *>(statement_node), byte_code, ctx);
            case StatementNode::StatementType::WhileStmt:
                return generate_while_statement(static_cast<const WhileNode *>(statement_node), byte_code, ctx);
            case StatementNode::StatementType::BreakStmt:
                return generate_break_statement(byte_code, static_cast<WhileLoopGenerationContext *>(ctx));
            case StatementNode::StatementType::ContinueStmt:
                return generate_continue_statement(byte_code, static_cast<WhileLoopGenerationContext *>(ctx));
            default:
                throw IRGeneratorException("Unsupported statement type");
        }
    }

    void IRGenerator::generate_expression(const AstNode* node, ByteCode& byte_code, void* ctx) {
        switch (node->get_type()) {
            case AstNodeType::Stmt:
                generate_statement(static_cast<const StatementNode *>(node), byte_code, ctx);
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

    void IRGenerator::generate_declaration_statement(const DeclarationStmtNode* node, ByteCode& byte_code, void* ctx) {
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

        generate_expression(expr, byte_code, ctx);

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::POP_STACK,
            {std::monostate()}
        ));
        
        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::STORE_IDENTIFIER, 
            IRStoreIdentifierParam{identifier->get_name()}));
    }

    void IRGenerator::generate_assignment_statement(const AssignmentStmtNode* node, ByteCode& byte_code, void* ctx) { 
        auto expr = static_cast<const StatementNode *>(node->get_value().get());
        auto identifier = static_cast<const IdentifierNode *>(node->get_identifier().get());
        
        generate_expression(expr, byte_code, ctx);

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::POP_STACK,
            {std::monostate()}
        ));
        
        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::STORE_IDENTIFIER, 
            IRStoreIdentifierParam{identifier->get_name()}));
    }

    void IRGenerator::generate_binary_expression_statement(const BinaryExpressionNode* node, ByteCode& byte_code, void* ctx) {
        const auto& left = node->get_left();
        const auto& right = node->get_right();

        generate_expression(left.get(), byte_code, ctx);
        generate_expression(right.get(), byte_code, ctx);

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

    void IRGenerator::generate_if_statement(const IfNode* statement, ByteCode& byte_code, void* ctx) {
        auto* expr = statement->get_condition().get();

        generate_expression(expr, byte_code, ctx);

        generate_conditional_comparision_bytecode(expr, byte_code, ctx);
        
        bool has_else_clause = statement->get_else_body().get() != nullptr;

        size_t zero_index = byte_code.size() - 1; // j*
        size_t if_end_index = 0;
        size_t else_clause_index = 0;

        generate_statement(static_cast<StatementNode *>(statement->get_body().get()), byte_code, ctx);
        if_end_index = byte_code.size();

        if (has_else_clause) {
            // in this case the 'end of if clause' is the jump to the end of the if statement
            // as we don't want it to jump directly to the end, so we add the offset by 1
            byte_code[zero_index].param = IRJumpParam(if_end_index + 1);

            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP, IRJumpParam(0)));
            generate_statement(static_cast<StatementNode *>(statement->get_else_body().get()), byte_code, ctx);
            else_clause_index = byte_code.size();
            byte_code[if_end_index].param = IRJumpParam(else_clause_index);
        } else {
            byte_code[zero_index].param = IRJumpParam(if_end_index);
        }
    }

    void IRGenerator::generate_while_statement(const WhileNode* statement, ByteCode& byte_code, void* ctx) {
        auto* expr = statement->get_condition().get();

        size_t while_start_index, while_end_index, while_start_jmp_index;
        while_start_index = byte_code.size();

        generate_expression(expr, byte_code, ctx);

        generate_conditional_comparision_bytecode(expr, byte_code, ctx);
        while_start_jmp_index = byte_code.size() - 1;
        
        WhileLoopGenerationContext generationCtx;
        generate_statement(
            static_cast<StatementNode *>(statement->get_body().get()), 
            byte_code, &generationCtx);

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::JMP, 
            IRJumpParam(while_start_index)));
        
        // the first command after the loop
        while_end_index = byte_code.size();

        // fill in jump target for unmet condition
        byte_code[while_start_jmp_index].param = IRJumpParam(while_end_index);

        // fill in jump targets of breaks
        for (auto break_instruction : generationCtx.break_instructions) {
            byte_code[break_instruction].param = IRJumpParam(while_end_index);
        }

        // fill in jump targets of continues
        for (auto continue_instruction : generationCtx.continue_instructions) {
            byte_code[continue_instruction].param = IRJumpParam(while_start_index);
        }
    }

    void IRGenerator::generate_break_statement(ByteCode& byte_code, WhileLoopGenerationContext* ctx) {
        assert(ctx != nullptr);

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::JMP, IRJumpParam(0)));
        ctx->register_break_instruction(byte_code.size() - 1);
    }

    void IRGenerator::generate_continue_statement(ByteCode& byte_code, WhileLoopGenerationContext* ctx) {
        assert(ctx != nullptr);

        byte_code.push_back(IRInstruction(
            IRInstruction::InstructionType::JMP, IRJumpParam(0)));
        ctx->register_continue_instruction(byte_code.size() - 1);
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
        auto cmp = std::get<int32_t>(output);
        switch (op) {
            case IRInstruction::InstructionType::JMP:
                pc = param;
                return true;
            case IRInstruction::InstructionType::JE:
                if (cmp == 0) {
                    pc = param;
                    return true;
                }
                break;
            case IRInstruction::InstructionType::JNE: 
                if (cmp != 0) {
                    pc = param;
                    return true;
                }
                break;
            case IRInstruction::InstructionType::JL:
                if (cmp < 0) {
                    pc = param;
                    return true;
                }
                break;
            case IRInstruction::InstructionType::JLE:
                if (cmp <= 0) {
                    pc = param;
                    return true;
                }
                break;
            case IRInstruction::InstructionType::JG:
                if (cmp > 0) {
                    pc = param;
                    return true;
                }
                break;
            case IRInstruction::InstructionType::JGE:
                if (cmp >= 0) {
                    pc = param;
                    return true;
                }
                break;
            default:
                throw IRInterpreterException("Unknown instruction type");
        }
        return false;
    }

    void IRGenerator::generate_conditional_comparision_bytecode(const AstNode* node, ByteCode& byte_code, void* ctx) {
        if (node->get_type() == AstNodeType::Stmt) {
            auto* stmt = dynamic_cast<const StatementNode*>(node);
            if (stmt->get_statement_type() == StatementNode::StatementType::BinaryExprStmt) {
                IRInstruction::InstructionType jmp_op;
                auto* binary_expr = 
                    dynamic_cast<const BinaryExpressionNode*>(stmt);

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