#include "ir.hpp"

#include <cassert>
#include <sstream>

namespace luaxc {
    std::string IRInstruction::dump() const {
        std::string out;

        const auto cvt_numeric = [](IRPrimValue value) {
            return value.to_string();
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
            case IRInstruction::InstructionType::CALL:
                out = "CALL";
                out += " " + std::to_string(std::get<IRCallParam>(param).arguments_count);
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
        for (const auto& instruction: bytecode) {
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
            for (const auto& statement: static_cast<const ProgramNode*>(node)->get_statements()) {
                auto statement_node = static_cast<const StatementNode*>(statement.get());
                generate_statement(statement_node, byte_code);
            }
        } else if (node->get_type() == AstNodeType::Stmt) {
            auto stmt = static_cast<const StatementNode*>(node);
            if (stmt->get_statement_type() != StatementNode::StatementType::BlockStmt) {
                throw IRGeneratorException("generate_program_or_block requires a proper block statement");
            }

            for (const auto& statement: static_cast<const BlockNode*>(node)->get_statements()) {
                auto statement_node = static_cast<const StatementNode*>(statement.get());
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
            case StatementNode::StatementType::ForwardDeclarationStmt:
                // todo: in the future this may be used to handle imports.
                break;
            case StatementNode::StatementType::BlockStmt:
                return generate_program_or_block(statement_node, byte_code);
            case StatementNode::StatementType::IfStmt:
                return generate_if_statement(static_cast<const IfNode*>(statement_node), byte_code);
            case StatementNode::StatementType::WhileStmt:
                return generate_while_statement(static_cast<const WhileNode*>(statement_node), byte_code);
            case StatementNode::StatementType::ForStmt:
                return generate_for_statement(static_cast<const ForNode*>(statement_node), byte_code);
            case StatementNode::StatementType::BreakStmt:
                return generate_break_statement(byte_code);
            case StatementNode::StatementType::ContinueStmt:
                return generate_continue_statement(byte_code);
            case StatementNode::StatementType::ExpressionStmt:
                return generate_expression(static_cast<const ExpressionNode*>(statement_node), byte_code);
            default:
                throw IRGeneratorException("Unsupported statement type");
        }
    }

    void IRGenerator::generate_expression(const ExpressionNode* node, ByteCode& byte_code) {
        switch (node->get_expression_type()) {
            case ExpressionNode::ExpressionType::Identifier: {
                byte_code.push_back(
                        IRInstruction(IRInstruction::InstructionType::LOAD_IDENTIFIER,
                                      {IRLoadIdentifierParam{static_cast<const IdentifierNode*>(node)->get_name()}}));
                byte_code.push_back(
                        IRInstruction(IRInstruction::InstructionType::PUSH_STACK,
                                      {std::monostate()}));
                break;
            }
            case ExpressionNode::ExpressionType::NumericLiteral: {
                byte_code.push_back(
                        IRInstruction(IRInstruction::InstructionType::LOAD_CONST,
                                      {IRLoadConstParam(static_cast<const NumericLiteralNode*>(node)->get_value())}));
                byte_code.push_back(
                        IRInstruction(IRInstruction::InstructionType::PUSH_STACK,
                                      {std::monostate()}));
                break;
            }
            case ExpressionNode::ExpressionType::StringLiteral: {
                // todo: support strings
                throw IRGeneratorException("String literals are not supported yet");
                break;
            }
            case ExpressionNode::ExpressionType::AssignmentExpr: {
                generate_assignment_statement(static_cast<const AssignmentExpressionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::BinaryExpr: {
                generate_binary_expression_statement(static_cast<const BinaryExpressionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::UnaryExpr: {
                generate_unary_expression_statement(static_cast<const UnaryExpressionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::FuncInvokeExpr: {
                generate_function_invocation_statement(static_cast<const FunctionInvocationExpressionNode*>(node), byte_code);
                break;
            }
            default:
                throw IRGeneratorException("Unsupported expression type");
        }

        if (node->is_result_discarded()) {
            // discard the value
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::POP_STACK, {std::monostate()}));
        }
    }

    void IRGenerator::generate_declaration_statement(const DeclarationStmtNode* node, ByteCode& byte_code) {
        auto expr = static_cast<const StatementNode*>(node->get_value_or_initializer());
        auto& identifiers = node->get_identifiers();

        // expr not present or has multiple identifiers declared.
        if (identifiers.size() > 1 || expr == nullptr) {
            assert(expr == nullptr);

            for (auto& identifier: identifiers) {
                auto* identifier_node = static_cast<IdentifierNode*>(identifier.get());
                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::STORE_IDENTIFIER,
                        IRStoreIdentifierParam{identifier_node->get_name()}));
            }
            return;
        }

        generate_expression(static_cast<const ExpressionNode*>(expr), byte_code);

        auto* identifier_node = static_cast<IdentifierNode*>(identifiers[0].get());

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::POP_STACK,
                {std::monostate()}));

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::STORE_IDENTIFIER,
                IRStoreIdentifierParam{identifier_node->get_name()}));
    }

    void IRGenerator::generate_assignment_statement(const AssignmentExpressionNode* node, ByteCode& byte_code) {
        auto expr = static_cast<const StatementNode*>(node->get_value().get());
        auto identifier = static_cast<const IdentifierNode*>(node->get_identifier().get());

        generate_expression(static_cast<const ExpressionNode*>(expr), byte_code);

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::POP_STACK,
                {std::monostate()}));

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::STORE_IDENTIFIER,
                IRStoreIdentifierParam{identifier->get_name()}));
    }

    bool IRGenerator::is_binary_logical_operator(BinaryExpressionNode::BinaryOperator op) {
        return (op == BinaryExpressionNode::BinaryOperator::LogicalAnd ||
                op == BinaryExpressionNode::BinaryOperator::LogicalOr);
    }

    bool IRGenerator::is_combinative_assignment_operator(BinaryExpressionNode::BinaryOperator op) {
        return (op == BinaryExpressionNode::BinaryOperator::IncrementBy ||
                op == BinaryExpressionNode::BinaryOperator::DecrementBy);
    }

    void IRGenerator::generate_binary_expression_statement(const BinaryExpressionNode* statement, ByteCode& byte_code) {
        auto node_op = statement->get_op();

        if (is_combinative_assignment_operator(node_op)) {
            generate_combinative_assignment_statement(statement, byte_code);
            return;
        }

        const auto& left = statement->get_left();
        const auto& right = statement->get_right();

        generate_expression(static_cast<const ExpressionNode*>(left.get()), byte_code);
        if (is_binary_logical_operator(node_op)) {
            // when the logical operator is used, we need to convert the lhs and rhs value to boolean
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        }

        generate_expression(static_cast<const ExpressionNode*>(right.get()), byte_code);
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
                {std::monostate()}));
    }

    void IRGenerator::generate_combinative_assignment_statement(const BinaryExpressionNode* statement, ByteCode& byte_code) {
        const auto& left = statement->get_left();
        const auto& right = statement->get_right();
        // todo: check if 'left' is a left-value
        // this requires left to be a left-value,
        // however I haven't figured out how to implement this checking stuff yet.

        generate_expression(static_cast<const ExpressionNode*>(right.get()), byte_code);

        auto& left_identifier = dynamic_cast<IdentifierNode*>(left.get())->get_name();

        // load the identifier and push onto the stack
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::LOAD_IDENTIFIER,
                IRLoadIdentifierParam{left_identifier}));
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::PUSH_STACK,
                {std::monostate()}));

        // load the operator
        auto op = statement->get_op();
        IRInstruction::InstructionType instruction_type;
        if (op == BinaryExpressionNode::BinaryOperator::IncrementBy) {
            instruction_type = IRInstruction::InstructionType::ADD;
        } else if (op == BinaryExpressionNode::BinaryOperator::DecrementBy) {
            instruction_type = IRInstruction::InstructionType::SUB;
        } else {
            throw IRGeneratorException("Unsupported combinative assignment operator");
        }
        byte_code.push_back(IRInstruction(instruction_type, {std::monostate()}));

        // write back
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::POP_STACK, {std::monostate()}));
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::STORE_IDENTIFIER,
                IRStoreIdentifierParam{left_identifier}));
    }

    void IRGenerator::generate_unary_expression_statement(const UnaryExpressionNode* statement, ByteCode& byte_code) {
        const auto& operand = statement->get_operand();

        generate_expression(static_cast<const ExpressionNode*>(operand.get()), byte_code);
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

        generate_expression(static_cast<const ExpressionNode*>(expr), byte_code);

        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::POP_STACK, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_IF_FALSE, {std::monostate()}));

        bool has_else_clause = statement->get_else_body().get() != nullptr;

        size_t zero_index = byte_code.size() - 1;// j*
        size_t if_end_index = 0;
        size_t else_clause_index = 0;

        generate_statement(static_cast<StatementNode*>(statement->get_body().get()), byte_code);
        if_end_index = byte_code.size();

        if (has_else_clause) {
            // in this case the 'end of if clause' is the jump to the end of the if statement
            // as we don't want it to jump directly to the end, so we add the offset by 1
            byte_code[zero_index].param = IRJumpParam(if_end_index + 1);

            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP, IRJumpParam(0)));
            generate_statement(static_cast<StatementNode*>(statement->get_else_body().get()), byte_code);
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

        generate_expression(static_cast<const ExpressionNode*>(expr), byte_code);

        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::POP_STACK, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_IF_FALSE, {std::monostate()}));

        while_start_jmp_index = byte_code.size() - 1;

        while_loop_generation_stack.push(WhileLoopGenerationContext{});

        generate_statement(
                static_cast<StatementNode*>(statement->get_body().get()),
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
        for (auto break_instruction: generation_ctx.break_instructions) {
            byte_code[break_instruction].param = IRJumpParam(while_end_index);
        }

        // fill in jump targets of continues
        for (auto continue_instruction: generation_ctx.continue_instructions) {
            byte_code[continue_instruction].param = IRJumpParam(while_start_index);
        }
    }

    void IRGenerator::generate_for_statement(const ForNode* statement, ByteCode& byte_code) {
        auto* init_stmt = static_cast<StatementNode*>(statement->get_init_stmt().get());
        generate_statement(init_stmt, byte_code);

        size_t update_and_condition_start_index = byte_code.size();// the command after decl / assignment

        auto* update_stmt = static_cast<StatementNode*>(statement->get_update_stmt().get());
        generate_statement(update_stmt, byte_code);

        auto* cond_expr = statement->get_condition_expr().get();
        generate_expression(static_cast<const ExpressionNode*>(cond_expr), byte_code);

        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::POP_STACK, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_IF_FALSE, {std::monostate()}));

        size_t jump_to_end_of_loop = byte_code.size() - 1;// jmp_if_false

        while_loop_generation_stack.push(WhileLoopGenerationContext{});

        auto* body = static_cast<StatementNode*>(statement->get_body().get());
        generate_statement(body, byte_code);

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::JMP, IRJumpParam(update_and_condition_start_index)));

        size_t loop_end_index = byte_code.size();

        byte_code[jump_to_end_of_loop].param = IRJumpParam(loop_end_index);

        auto generation_context = while_loop_generation_stack.top();
        while_loop_generation_stack.pop();

        // similar to while loops
        for (auto break_instruction: generation_context.break_instructions) {
            byte_code[break_instruction].param = IRJumpParam(loop_end_index);
        }

        for (auto continue_instruction: generation_context.continue_instructions) {
            byte_code[continue_instruction].param = IRJumpParam(update_and_condition_start_index);
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

    IRInterpreter::IRInterpreter() {
        push_stack_frame();// global scope
        preload_native_functions();
    }

    IRInterpreter::~IRInterpreter() {
        assert(stack_frames.size() == 1);
        pop_stack_frame();
        free_native_functions();
    }

    void IRGenerator::generate_function_invocation_statement(const FunctionInvocationExpressionNode* node, ByteCode& byte_code) {
        const auto& args = node->get_arguments();
        size_t arguments_count = args.size();

        for (int i = arguments_count - 1; i >= 0; i--) {
            // the arguments are pushed in reverse order
            // so the first argument is at the top of the stack
            generate_expression(static_cast<const ExpressionNode*>(args[i].get()), byte_code);
        }

        // load the function object
        auto func_identifier = static_cast<IdentifierNode*>(node->get_function_identifier().get());
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::LOAD_IDENTIFIER, IRLoadIdentifierParam{func_identifier->get_name()}));

        // push function object onto the stack
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::PUSH_STACK, {std::monostate()}));

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::CALL, IRCallParam{arguments_count}));
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
                    output = retrieve_raw_value(param.identifier);
                    break;
                }
                case IRInstruction::InstructionType::STORE_IDENTIFIER: {
                    auto param = std::get<IRStoreIdentifierParam>(instruction.param);
                    store_value_in_stack_frame(param.identifier, output);
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

                case IRInstruction::InstructionType::CALL: {
                    auto param = std::get<IRCallParam>(instruction.param);
                    handle_function_invocation(param);
                    break;
                }

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
                auto cond = output.to_bool();
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
        top = PrimValue::from_bool(top.to_bool());
    }

    void IRInterpreter::handle_function_invocation(IRCallParam param) {
        auto fn_obj = stack.top();
        stack.pop();

        if (fn_obj.get_type() != ValueType::Function) {
            throw IRInterpreterException("Cannot invoke non-function");
        }

        auto fn = fn_obj.get_inner_value<FunctionObject*>();
        if (fn->is_native_function()) {
            std::vector<IRPrimValue> args(param.arguments_count);
            for (size_t i = 0; i < param.arguments_count; i++) {
                args[i] = stack.top();
                stack.pop();
            }
            auto ret = fn->call_native(args);
            stack.push(ret);
        } else {
            throw IRInterpreterException("Cannot invoke non-native function");
        }
    }

    void IRInterpreter::handle_binary_op(IRInstruction::InstructionType op) {
        auto rhs_variant = stack.top();
        stack.pop();
        auto lhs_variant = stack.top();
        stack.pop();
        // the left node is visited first and entered the stack first,
        // so the right node is the top of the stack

        handle_binary_op(op, lhs_variant, rhs_variant);
    }

    void IRInterpreter::handle_unary_op(IRInstruction::InstructionType op) {
        auto rhs_variant = stack.top();
        stack.pop();

        handle_unary_op(op, rhs_variant);
    }

    void IRInterpreter::handle_binary_op(IRInstruction::InstructionType op, IRPrimValue lhs, IRPrimValue rhs) {
        switch (op) {
            case IRInstruction::InstructionType::ADD:
                stack.push(detail::prim_value_add(lhs, rhs));
                break;
            case IRInstruction::InstructionType::SUB:
                stack.push(detail::prim_value_sub(lhs, rhs));
                break;
            case IRInstruction::InstructionType::MUL:
                stack.push(detail::prim_value_mul(lhs, rhs));
                break;
            case IRInstruction::InstructionType::DIV:
                stack.push(detail::prim_value_div(lhs, rhs));
                break;
            case IRInstruction::InstructionType::MOD:
                stack.push(detail::prim_value_mod(lhs, rhs));
                break;
            case IRInstruction::InstructionType::AND:
                stack.push(detail::prim_value_band(lhs, rhs));
                break;
            case IRInstruction::InstructionType::LOGICAL_AND: {
                stack.push(detail::prim_value_land(lhs, rhs));
                break;
            }
            case IRInstruction::InstructionType::OR:
                stack.push(detail::prim_value_bor(lhs, rhs));
                break;
            case IRInstruction::InstructionType::LOGICAL_OR:
                stack.push(detail::prim_value_lor(lhs, rhs));
                break;
            case IRInstruction::InstructionType::XOR:
                stack.push(detail::prim_value_bxor(lhs, rhs));
                break;
            case IRInstruction::InstructionType::SHL:
                stack.push(detail::prim_value_shl(lhs, rhs));
                break;
            case IRInstruction::InstructionType::SHR:
                stack.push(detail::prim_value_shr(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_EQ:
                stack.push(detail::prim_value_eq(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_NE:
                stack.push(detail::prim_value_neq(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_LT:
                stack.push(detail::prim_value_lt(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_LE:
                stack.push(detail::prim_value_lte(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_GT:
                stack.push(detail::prim_value_gt(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_GE:
                stack.push(detail::prim_value_gte(lhs, rhs));
                break;
            default:
                throw IRInterpreterException("Invalid instruction type");
                return;
        }
    }

    void IRInterpreter::handle_unary_op(IRInstruction::InstructionType op, IRPrimValue rhs) {
        switch (op) {
            case IRInstruction::InstructionType::NEGATE:
                stack.push(detail::prim_value_neg(rhs));
                break;
            case IRInstruction::InstructionType::NOT: {
                stack.push(detail::prim_value_bnot(rhs));
                break;
            }
            case IRInstruction::InstructionType::LOGICAL_NOT: {
                stack.push(detail::prim_value_lnot(rhs));
                break;
            }
            default:
                throw IRInterpreterException("Invalid instruction type");
        }
    }

    IRPrimValue IRInterpreter::retrieve_raw_value(const std::string& identifier) {
        if (has_identifier(identifier)) {
            return retrieve_raw_value_in_stack_frame(identifier);
        } else {
            throw IRInterpreterException("Identifier not found: " + identifier);
        }
    }

    IRPrimValue& IRInterpreter::retrieve_raw_value_ref(const std::string& identifier) {
        if (has_identifier(identifier)) {
            return retrieve_value_ref_in_stack_frame(identifier);
        } else {
            throw IRInterpreterException("Identifier not found: " + identifier);
        }
    }

    bool IRInterpreter::has_identifier(const std::string& identifier) {
        return has_identifier_in_stack_frame(identifier);
    }

    void IRInterpreter::preload_native_functions() {
        FunctionObject* println = FunctionObject::create_native_function(
                [](std::vector<PrimValue> args) -> PrimValue {
                    printf("%s\n", args[0].to_string().c_str());
                    return PrimValue::unit();
                });
        native_functions.push_back(println);
        store_value_in_stack_frame("println", PrimValue(ValueType::Function, println));
    }

    void IRInterpreter::free_native_functions() {
        for (auto function: native_functions) {
            delete function;
        }
    }

    void IRInterpreter::push_stack_frame() {
        stack_frames.emplace_back();
    }

    void IRInterpreter::pop_stack_frame() {
        stack_frames.pop_back();
    }

    IRInterpreter::StackFrame& IRInterpreter::current_stack_frame() {
        return stack_frames.back();
    }

    bool IRInterpreter::has_identifier_in_stack_frame(const std::string& identifier) {
        return current_stack_frame().variables.find(identifier) != current_stack_frame().variables.end();
    }

    IRPrimValue IRInterpreter::retrieve_raw_value_in_stack_frame(const std::string& identifier) {
        return current_stack_frame().variables.at(identifier);
    }

    IRPrimValue& IRInterpreter::retrieve_value_ref_in_stack_frame(const std::string& identifier) {
        return current_stack_frame().variables.at(identifier);
    }

    void IRInterpreter::store_value_in_stack_frame(const std::string& identifier, IRPrimValue value) {
        current_stack_frame().variables[identifier] = value;
    }
}// namespace luaxc