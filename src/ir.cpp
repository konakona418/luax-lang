#include "ir.hpp"
#include "lexer.hpp"
#include "lib.hpp"
#include "parser.hpp"


#include <cassert>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>


namespace luaxc {
#define __LUAXC_IR_RUNTIME_UTILS_EXTRACT_STRING_FROM_PRIM_VALUE(_prim_value) \
    (static_cast<StringObject*>(static_cast<GCObject*>(_prim_value.get_inner_value<GCObject*>()))->contained_string())

    std::string IRInstruction::dump() const {
        std::string out;

        switch (type) {
            case IRInstruction::InstructionType::LOAD_CONST: {
                out = "LOAD_CONST";
                auto& p = std::get<IRLoadConstParam>(param);
                if (p.is_string()) {
                    out += " [string object \"" + __LUAXC_IR_RUNTIME_UTILS_EXTRACT_STRING_FROM_PRIM_VALUE(p) + "\"]";
                } else {
                    out += " " + p.to_string();
                }
                break;
            }
            case IRInstruction::InstructionType::DECLARE_IDENTIFIER: {
                out = "DECLARE_IDENTIFIER ";
                out += std::get<IRDeclareIdentifierParam>(param).identifier->to_string();
                break;
            }
            case IRInstruction::InstructionType::LOAD_IDENTIFIER:
                out = "LOAD_IDENTIFIER ";
                out += std::get<IRLoadIdentifierParam>(param).identifier->to_string();
                break;
            case IRInstruction::InstructionType::STORE_IDENTIFIER:
                out = "STORE_IDENTIFIER ";
                out += std::get<IRStoreIdentifierParam>(param).identifier->to_string();
                break;
            case IRInstruction::InstructionType::LOAD_MODULE:
                out = "LOAD_MODULE ";
                out += "[module id=" + std::to_string(std::get<IRLoadModuleParam>(param).module_id) + "]";
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
            case IRInstruction::InstructionType::POP_STACK:
                out = "POP_STACK";
                break;
            case IRInstruction::InstructionType::PEEK:
                out = "PEEK";
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
                out = "JMP ";
                out += std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JMP_IF_FALSE:
                out = "JMP_IF_FALSE ";
                out += std::to_string(std::get<IRJumpParam>(param));
                break;
            case IRInstruction::InstructionType::JMP_REL:
                out = "JMP_REL ";
                out += std::to_string(std::get<IRJumpRelParam>(param));
                break;
            case IRInstruction::InstructionType::JMP_IF_FALSE_REL:
                out = "JMP_IF_FALSE_REL ";
                out += std::to_string(std::get<IRJumpRelParam>(param));
                break;
            case IRInstruction::InstructionType::TO_BOOL:
                out = "TO_BOOL";
                break;
            case IRInstruction::InstructionType::CALL:
                out = "CALL ";
                out += std::to_string(std::get<IRCallParam>(param).arguments_count);
                break;
            case IRInstruction::InstructionType::RET:
                out += "RET";
                break;
            case IRInstruction::InstructionType::BEGIN_LOCAL:
                out += "BEGIN_LOCAL";
                break;
            case IRInstruction::InstructionType::END_LOCAL:
                out += "END_LOCAL";
                break;
            case IRInstruction::InstructionType::BEGIN_LOCAL_DERIVED:
                out += "BEGIN_LOCAL_DERIVED";
                break;
            case IRInstruction::InstructionType::MAKE_STRING:
                out += "MAKE_STRING";
                break;
            case IRInstruction::InstructionType::MAKE_FUNC:
                out += "MAKE_FUNC";
                break;
            case IRInstruction::InstructionType::MAKE_TYPE:
                out += "MAKE_TYPE";
                break;
            case IRInstruction::InstructionType::MAKE_OBJECT:
                out += "MAKE_OBJECT";
                break;
            case IRInstruction::InstructionType::MAKE_MODULE:
                out += "MAKE_MODULE";
                break;
            case IRInstruction::InstructionType::MAKE_MODULE_LOCAL:
                out += "MAKE_MODULE_LOCAL";
                break;
            case IRInstruction::InstructionType::LOAD_MEMBER:
                out += "LOAD_MEMBER ";
                out += std::get<IRLoadMemberParam>(param).identifier->to_string();
                break;
            case IRInstruction::InstructionType::STORE_MEMBER:
                out += "STORE_MEMBER ";
                out += std::get<IRStoreMemberParam>(param).identifier->to_string();
                break;
            case IRInstruction::InstructionType::LOAD_INDEXOF:
                out += "LOAD_INDEXOF";
                break;
            case IRInstruction::InstructionType::STORE_INDEXOF:
                out += "STORE_INDEXOF";
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

        auto* module_name = begin_module_compilation("<main>", 0);
        generate_program_or_block(ast.get(), byte_code);
        size_t module_id = end_module_compilation();

        assert(module_id == 0);
        return byte_code;
    }

    StringObject* IRRuntime::push_string_pool_if_not_exists(const std::string& str) {
        StringObject* string_obj;

        if (is_string_in_pool(str)) {
            string_obj = get_string_from_pool(str);
        } else {
            string_obj = static_cast<StringObject*>(StringObject::from_string(str));
            gc_regist_no_collect(string_obj);
            push_string_to_pool(str, string_obj);
        }

        return string_obj;
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
            case StatementNode::StatementType::FunctionDeclarationStmt:
                return generate_function_declaration_statement(static_cast<const FunctionDeclarationNode*>(statement_node), byte_code);
            case StatementNode::StatementType::MethodDeclarationStmt:
                return generate_method_declaration_statement(static_cast<const MethodDeclarationNode*>(statement_node), byte_code);
            case StatementNode::StatementType::ReturnStmt:
                return generate_return_statement(static_cast<const ReturnNode*>(statement_node), byte_code);
            default:
                throw IRGeneratorException("Unsupported statement type");
        }
    }

    void IRGenerator::generate_expression(const ExpressionNode* node, ByteCode& byte_code) {
        switch (node->get_expression_type()) {
            case ExpressionNode::ExpressionType::Identifier: {
                auto* cached_string =
                        runtime.push_string_pool_if_not_exists(static_cast<const IdentifierNode*>(node)->get_name());
                byte_code.push_back(
                        IRInstruction(IRInstruction::InstructionType::LOAD_IDENTIFIER,
                                      {IRLoadIdentifierParam{cached_string}}));
                break;
            }
            case ExpressionNode::ExpressionType::MemberAccessExpr: {
                generate_member_access_statement_rvalue(static_cast<const MemberAccessExpressionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::MethodInvokeExpr: {
                generate_member_access(node, byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::NumericLiteral: {
                generate_numeric_literal(static_cast<const NumericLiteralNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::StringLiteral: {
                generate_string_literal(static_cast<const StringLiteralNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::BoolLiteral: {
                generate_boolean_literal(static_cast<const BoolLiteralNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::NullLiteral: {
                generate_null_literal(byte_code);
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
            case ExpressionNode::ExpressionType::TypeDecl: {
                generate_type_decl_expression(static_cast<const TypeDeclarationExpressionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::ModuleDecl: {
                generate_module_decl_expression(static_cast<const ModuleDeclarationExpressionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::ModuleImportExpr: {
                generate_module_import_expression(static_cast<const ModuleImportExpresionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::ModuleAccessExpr: {
                generate_module_access_expression(static_cast<const ExpressionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::ClousureExpr: {
                generate_closure_expression(static_cast<const ClosureExpressionNode*>(node), byte_code);
                break;
            }
            case ExpressionNode::ExpressionType::InitializerListExpr: {
                generate_initializer_list_expression(static_cast<const InitializerListExpressionNode*>(node), byte_code);
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

    void IRGenerator::generate_type_decl_expression(const TypeDeclarationExpressionNode* expression, ByteCode& byte_code) {
        auto* decl_block = static_cast<BlockNode*>(expression->get_type_statements_block().get());

        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::BEGIN_LOCAL_DERIVED,
                              {std::monostate()}));

        // recursively assign all
        for (auto& s: decl_block->get_statements()) {
            auto stmt = static_cast<StatementNode*>(s.get());
            if (stmt->get_statement_type() == StatementNode::StatementType::FieldDeclarationStmt) {
                auto field = static_cast<FieldDeclarationStatementNode*>(stmt);
                auto field_name = static_cast<IdentifierNode*>(field->get_field_identifier().get())->get_name();

                if (field->get_type_declaration_expr() == nullptr) {
                    throw IRGeneratorException("Any type is not supported");
                }

                generate_expression(static_cast<ExpressionNode*>(field->get_type_declaration_expr().get()), byte_code);

                auto* cached_string = runtime.push_string_pool_if_not_exists(field_name);

                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::DECLARE_IDENTIFIER,
                        IRDeclareIdentifierParam{cached_string}));

                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::STORE_IDENTIFIER,
                        IRStoreIdentifierParam{cached_string}));

            } else if (stmt->get_statement_type() == StatementNode::StatementType::MethodDeclarationStmt) {
                generate_method_declaration_statement(static_cast<const MethodDeclarationNode*>(stmt), byte_code);
            } else if (stmt->get_statement_type() == StatementNode::StatementType::FunctionDeclarationStmt) {
                generate_function_declaration_statement(static_cast<const FunctionDeclarationNode*>(stmt), byte_code);
            } else {
                throw IRGeneratorException("Unknown type declaration statement");
            }
        }

        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::MAKE_TYPE,
                              {std::monostate()}));

        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::END_LOCAL,
                              {std::monostate()}));
    }

    void IRGenerator::generate_module_decl_expression(const ModuleDeclarationExpressionNode* expression, ByteCode& byte_code) {
        auto* decl_block = static_cast<const BlockNode*>(expression->get_module_statements_block().get());

        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::BEGIN_LOCAL_DERIVED,
                              {std::monostate()}));

        for (auto& statement: decl_block->get_statements()) {
            generate_statement(static_cast<const StatementNode*>(statement.get()), byte_code);
        }

        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::MAKE_MODULE_LOCAL,
                              {std::monostate()}));
        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::END_LOCAL,
                              {std::monostate()}));
    }

    void IRGenerator::generate_module_import_expression(const ModuleImportExpresionNode* expression, ByteCode& byte_code) {
        auto module_name_str =
                static_cast<const StringLiteralNode*>(expression->get_module_name().get())->get_value();

        // if the module is already loaded,
        // we can just use it
        if (auto loaded_module = is_module_present(module_name_str)) {
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::LOAD_MODULE,
                                              IRLoadModuleParam{loaded_module.value()}));
            return;
        }

        auto module_content = runtime.find_file_and_read(module_name_str);
        if (module_content.empty()) {
            throw IRGeneratorException("Module '" + module_name_str + "' not found");
        }

        auto module_ast = fn_compile_module(module_content);

        ByteCode module_byte_code;

        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::BEGIN_LOCAL_DERIVED,
                              {std::monostate()}));

        auto* module_name = begin_module_compilation(module_name_str, byte_code.size());
        generate_program_or_block(module_ast.get(), module_byte_code);
        size_t module_id = end_module_compilation();

        byte_code.reserve(module_byte_code.size());
        byte_code.insert(byte_code.end(), module_byte_code.begin(), module_byte_code.end());

        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::MAKE_MODULE,
                              IRMakeModuleParam{module_id}));
        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::END_LOCAL,
                              {std::monostate()}));
    }

    void IRGenerator::generate_module_access_expression(const ExpressionNode* expression, ByteCode& byte_code) {
        if (expression->get_expression_type() == ExpressionNode::ExpressionType::ModuleAccessExpr) {
            auto* expr = static_cast<const ModuleAccessExpressionNode*>(expression);
            auto* left = expr->get_object_expr().get();

            auto* identifier_or_function_call =
                    static_cast<const ExpressionNode*>(expr->get_module_identifier().get());

            switch (identifier_or_function_call->get_expression_type()) {
                case (ExpressionNode::ExpressionType::Identifier): {
                    auto* identifier = static_cast<const IdentifierNode*>(identifier_or_function_call);
                    auto* str_obj = runtime.push_string_pool_if_not_exists(identifier->get_name());

                    generate_module_access_expression(static_cast<const ExpressionNode*>(left), byte_code);

                    byte_code.push_back(IRInstruction(
                            IRInstruction::InstructionType::LOAD_MEMBER,
                            IRLoadMemberParam{str_obj}));

                    break;
                }
                case (ExpressionNode::ExpressionType::FuncInvokeExpr): {
                    auto* fn_expr = static_cast<const FunctionInvocationExpressionNode*>(identifier_or_function_call);
                    auto* fn_identifier = static_cast<const IdentifierNode*>(fn_expr->get_function_identifier().get());

                    auto* str_obj = runtime.push_string_pool_if_not_exists(fn_identifier->get_name());

                    auto& args = fn_expr->get_arguments();
                    auto arguments_count = args.size();

                    for (int i = arguments_count - 1; i >= 0; i--) {
                        generate_expression(static_cast<const ExpressionNode*>(args[i].get()), byte_code);
                    }

                    generate_module_access_expression(static_cast<const ExpressionNode*>(left), byte_code);

                    byte_code.push_back(IRInstruction(
                            IRInstruction::InstructionType::LOAD_MEMBER,
                            IRLoadMemberParam{str_obj}));

                    byte_code.push_back(IRInstruction(
                            IRInstruction::InstructionType::CALL, IRCallParam{arguments_count}));
                    break;
                }
                default: {
                    throw IRGeneratorException("The right operand of module access expr is neither a function nor an identifier");
                    break;
                }
            }

            return;
        }
        generate_expression(expression, byte_code);
    }

    void IRGenerator::generate_closure_expression(const ClosureExpressionNode* expression, ByteCode& byte_code) {
        size_t jump_over_function_instruction_index = byte_code.size();
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::JMP_REL,
                IRJumpRelParam(0)));

        size_t fn_start_index = byte_code.size();

        for (auto& param: expression->get_parameters()) {
            auto identifier =
                    runtime.push_string_pool_if_not_exists(dynamic_cast<IdentifierNode*>(param.get())->get_name());
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::DECLARE_IDENTIFIER,
                    IRDeclareIdentifierParam{identifier}));
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::STORE_IDENTIFIER,
                    IRStoreIdentifierParam{identifier}));
        }

        generate_program_or_block(expression->get_body().get(), byte_code);

        if (byte_code.back().type != IRInstruction::InstructionType::RET) {
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::LOAD_CONST, IRLoadConstParam{IRPrimValue::unit()}));
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::RET, {std::monostate()}));
        }

        byte_code[jump_over_function_instruction_index].param =
                IRJumpRelParam(byte_code.size() - jump_over_function_instruction_index);

        auto current_module_id = get_current_compiling_module_id();

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::MAKE_FUNC,
                IRMakeFunctionParam{
                        fn_start_index,
                        current_module_id,
                        expression->get_parameters().size(),
                        false,
                        true}));
    }

    void IRGenerator::generate_numeric_literal(const NumericLiteralNode* statement, ByteCode& byte_code) {
        auto type = statement->get_type();
        auto value = IRLoadConstParam(statement->get_value());
        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::LOAD_CONST,
                              {value}));
    }

    void IRGenerator::generate_string_literal(const StringLiteralNode* statement, ByteCode& byte_code) {
        StringObject* string_obj = runtime.push_string_pool_if_not_exists(statement->get_value());

        auto value = IRPrimValue(ValueType::String, string_obj);

        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::LOAD_CONST,
                              {IRLoadConstParam(value)}));

        // we need to copy the string from constant pool!
        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::MAKE_STRING,
                              {std::monostate()}));
    }

    void IRGenerator::generate_declaration_statement(const DeclarationStmtNode* node, ByteCode& byte_code) {
        auto expr = static_cast<const StatementNode*>(node->get_value_or_initializer());
        auto& identifiers = node->get_identifiers();

        // expr not present or has multiple identifiers declared.
        if (identifiers.size() > 1 || expr == nullptr) {
            assert(expr == nullptr);

            for (auto& identifier: identifiers) {
                auto* identifier_node = static_cast<IdentifierNode*>(identifier.get());

                auto* cached_string = runtime.push_string_pool_if_not_exists(identifier_node->get_name());

                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::DECLARE_IDENTIFIER,
                        IRDeclareIdentifierParam{cached_string}));
            }
            return;
        }

        generate_expression(static_cast<const ExpressionNode*>(expr), byte_code);

        auto* identifier_node = static_cast<IdentifierNode*>(identifiers[0].get());
        auto* cached_string = runtime.push_string_pool_if_not_exists(identifier_node->get_name());

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::DECLARE_IDENTIFIER,
                IRDeclareIdentifierParam{cached_string}));
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::STORE_IDENTIFIER,
                IRStoreIdentifierParam{cached_string}));
    }

    void IRGenerator::generate_assignment_statement(const AssignmentExpressionNode* node, ByteCode& byte_code) {
        auto lvalue = static_cast<ExpressionNode*>(node->get_identifier().get());
        switch (lvalue->get_expression_type()) {
            case ExpressionNode::ExpressionType::Identifier:
                generate_assignment_statement_identifier_lvalue(node, byte_code);
                break;
            case ExpressionNode::ExpressionType::MemberAccessExpr:
                generate_assignment_statement_member_access_lvalue(node, byte_code);
                break;
            case ExpressionNode::ExpressionType::ModuleAccessExpr:
                throw IRGeneratorException("Using module access expression as lvalue is not supported yet");
            default:
                throw IRGeneratorException("Assigning value to an invalid lvalue expr");
        }
    }

    void IRGenerator::generate_assignment_statement_identifier_lvalue(
            const AssignmentExpressionNode* statement, ByteCode& byte_code) {
        auto identifier = static_cast<const IdentifierNode*>(statement->get_identifier().get());

        auto expr = static_cast<const StatementNode*>(statement->get_value().get());
        generate_expression(static_cast<const ExpressionNode*>(expr), byte_code);

        auto* cached_string = runtime.push_string_pool_if_not_exists(identifier->get_name());
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::STORE_IDENTIFIER,
                IRStoreIdentifierParam{cached_string}));
    }

    void IRGenerator::generate_initializer_list_expression(const InitializerListExpressionNode* expression, ByteCode& byte_code) {
        auto* type_expr = static_cast<const ExpressionNode*>(expression->get_type_expr().get());
        // nullable

        auto* block = static_cast<const BlockNode*>(expression->get_initializer_list_block().get());

        std::vector<StringObject*> fields;
        for (auto& stmt: block->get_statements()) {
            if (static_cast<const StatementNode*>(stmt.get())->get_statement_type() != StatementNode::StatementType::ExpressionStmt) {
                throw IRGeneratorException("Initializer list requires valid expressions");
            }

            auto* expr = static_cast<const ExpressionNode*>(stmt.get());
            if (expr->get_expression_type() != ExpressionNode::ExpressionType::AssignmentExpr) {
                throw IRGeneratorException("Initializer list requires valid assignment expressions");
            }

            auto* assign = static_cast<const AssignmentExpressionNode*>(expr);
            auto* field = static_cast<const IdentifierNode*>(assign->get_identifier().get());
            auto* value = static_cast<const ExpressionNode*>(assign->get_value().get());

            auto* field_name = runtime.push_string_pool_if_not_exists(field->get_name());

            fields.push_back(field_name);

            generate_expression(value, byte_code);
        }

        if (type_expr == nullptr) {
            // anonymous initializer list
            auto* any_type = TypeObject::any();
            auto type_value = PrimValue(ValueType::Type, (GCObject*){any_type});
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::LOAD_CONST, IRLoadConstParam(type_value)));
        } else {
            // push the type info onto the stack
            generate_expression(type_expr, byte_code);
        }

        // load the field names in reverse order, as we will acquire the values in reverse order
        auto fields_rev = std::vector<StringObject*>(fields.rbegin(), fields.rend());
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::MAKE_OBJECT, IRMakeObjectParam{fields_rev}));
    }

    void IRGenerator::generate_member_access(const ExpressionNode* expression, ByteCode& byte_code) {
        switch (expression->get_expression_type()) {
            case (ExpressionNode::ExpressionType::MethodInvokeExpr): {
                auto* method = static_cast<const MethodInvocationExpressionNode*>(expression);
                auto* prefixed_expr = static_cast<const ExpressionNode*>(method->get_initial_expr().get());
                auto* method_identifier = static_cast<const IdentifierNode*>(method->get_method_identifier().get());

                auto* cached_identifier = runtime.push_string_pool_if_not_exists(method_identifier->get_name());

                auto& args = method->get_arguments();
                auto arguments_count = args.size();

                for (int i = arguments_count - 1; i >= 0; i--) {
                    generate_expression(static_cast<const ExpressionNode*>(args[i].get()), byte_code);
                }

                auto real_arguments_count = arguments_count + 1;// include self

                generate_expression(prefixed_expr, byte_code);

                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::PEEK, {std::monostate()}));

                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::LOAD_MEMBER,
                        IRLoadMemberParam{cached_identifier}));

                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::CALL, IRCallParam{real_arguments_count}));
                break;
            }
            case (ExpressionNode::ExpressionType::MemberAccessExpr): {
                auto* expr = static_cast<const MemberAccessExpressionNode*>(expression);
                auto* left = expr->get_object_expr().get();
                generate_member_access(static_cast<const MemberAccessExpressionNode*>(left), byte_code);

                if (expr->get_access_type() == MemberAccessExpressionNode::MemberAccessType::DotMemberAccess) {
                    auto* identifier = static_cast<const IdentifierNode*>(expr->get_member_identifier().get());
                    auto* str_obj = runtime.push_string_pool_if_not_exists(identifier->get_name());

                    byte_code.push_back(IRInstruction(
                            IRInstruction::InstructionType::LOAD_MEMBER,
                            IRLoadMemberParam{str_obj}));
                } else {
                    auto* index =
                            static_cast<const ExpressionNode*>(expr->get_member_identifier().get());

                    generate_expression(index, byte_code);

                    byte_code.push_back(IRInstruction(
                            IRInstruction::InstructionType::LOAD_INDEXOF,
                            {std::monostate()}));
                }
                break;
            }
            default: {
                generate_expression(expression, byte_code);
                break;
            }
        }
    }

    void IRGenerator::generate_member_access_statement_rvalue(const MemberAccessExpressionNode* statement, ByteCode& byte_code) {
        generate_member_access(static_cast<const ExpressionNode*>(statement), byte_code);
    }

    void IRGenerator::generate_assignment_statement_member_access_lvalue(
            const AssignmentExpressionNode* statement, ByteCode& byte_code) {
        auto* member_access = static_cast<const MemberAccessExpressionNode*>(statement->get_identifier().get());
        auto* left_member_access =
                static_cast<const ExpressionNode*>(member_access->get_object_expr().get());

        // till now, we only generated the byte code for accessing all the parts before the final member access.
        generate_member_access(left_member_access, byte_code);

        // now generate the rvalue, which will be popped from the stack first when assigning
        auto* right_expr = static_cast<const ExpressionNode*>(statement->get_value().get());
        generate_expression(right_expr, byte_code);

        if (member_access->get_access_type() == MemberAccessExpressionNode::MemberAccessType::DotMemberAccess) {
            auto* identifier = runtime.push_string_pool_if_not_exists(
                    static_cast<const IdentifierNode*>(
                            member_access->get_member_identifier().get())
                            ->get_name());

            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::STORE_MEMBER,
                    IRStoreMemberParam{identifier}));
        } else {
            auto* index =
                    static_cast<const ExpressionNode*>(member_access->get_member_identifier().get());

            generate_expression(index, byte_code);

            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::STORE_INDEXOF,
                    {std::monostate()}));
        }
    }

    std::optional<size_t> IRGenerator::is_module_present(const std::string& module_name) {
        auto* module_identifier = runtime.push_string_pool_if_not_exists(module_name);
        return runtime.has_module(module_identifier);
    }

    size_t IRGenerator::aggregate_compiling_module_base_offsets() {
        size_t aggregated = 0;
        for (size_t offset: compiling_module_base_offsets) {
            aggregated += offset;
        }
        return aggregated;
    }

    StringObject* IRGenerator::begin_module_compilation(const std::string& module_name, size_t base_offset) {
        auto* module_name_obj = runtime.push_string_pool_if_not_exists(module_name);

        compiling_module_base_offsets.push_back(base_offset);
        size_t aggregated_base_offset = aggregate_compiling_module_base_offsets();

        size_t module_id = runtime.add_module(module_name_obj, aggregated_base_offset);

        compiling_module_ids.push(module_id);

        return module_name_obj;
    }

    size_t IRGenerator::end_module_compilation() {
        size_t module_id = compiling_module_ids.top();
        compiling_module_ids.pop();
        compiling_module_base_offsets.pop_back();
        return module_id;
    }

    size_t IRGenerator::get_current_compiling_module_id() {
        return compiling_module_ids.top();
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

        auto* left_expr = static_cast<const ExpressionNode*>(left.get());

        if (left_expr->get_expression_type() == ExpressionNode::ExpressionType::Identifier) {
            // generate right-hand side
            generate_expression(static_cast<const ExpressionNode*>(right.get()), byte_code);

            auto& left_identifier = static_cast<const IdentifierNode*>(left_expr)->get_name();

            // load the identifier and push onto the stack
            auto* cached_string = runtime.push_string_pool_if_not_exists(left_identifier);
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::LOAD_IDENTIFIER,
                    IRLoadIdentifierParam{cached_string}));

            byte_code.push_back(IRInstruction(instruction_type, {std::monostate()}));

            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::STORE_IDENTIFIER,
                    IRStoreIdentifierParam{cached_string}));
        } else if (left_expr->get_expression_type() == ExpressionNode::ExpressionType::MemberAccessExpr) {
            auto* member_access = static_cast<const MemberAccessExpressionNode*>(left_expr);
            auto* member_access_left = static_cast<const ExpressionNode*>(member_access->get_object_expr().get());

            // push object first
            generate_member_access(member_access_left, byte_code);

            // then push value
            generate_member_access(left_expr, byte_code);
            generate_expression(static_cast<const ExpressionNode*>(right.get()), byte_code);
            byte_code.push_back(IRInstruction(instruction_type, {std::monostate()}));

            if (member_access->get_access_type() == MemberAccessExpressionNode::MemberAccessType::DotMemberAccess) {
                auto* identifier = runtime.push_string_pool_if_not_exists(
                        static_cast<const IdentifierNode*>(
                                member_access->get_member_identifier().get())
                                ->get_name());

                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::STORE_MEMBER,
                        IRStoreMemberParam{identifier}));
            } else {
                auto* index =
                        static_cast<const ExpressionNode*>(member_access->get_member_identifier().get());

                generate_expression(index, byte_code);

                byte_code.push_back(IRInstruction(
                        IRInstruction::InstructionType::STORE_INDEXOF,
                        {std::monostate()}));
            }
        } else {
            throw IRGeneratorException("Invalid conbinative assignment");
        }
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

    void IRGenerator::generate_boolean_literal(const BoolLiteralNode* statement, ByteCode& byte_code) {
        byte_code.push_back(
                IRInstruction(IRInstruction::InstructionType::LOAD_CONST,
                              IRLoadConstParam{PrimValue::from_bool(statement->get_value())}));
    }

    void IRGenerator::generate_null_literal(ByteCode& byte_code) {
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::LOAD_CONST,
                IRLoadConstParam{PrimValue::null()}));
    }

    void IRGenerator::generate_if_statement(const IfNode* statement, ByteCode& byte_code) {
        auto* expr = statement->get_condition().get();

        generate_expression(static_cast<const ExpressionNode*>(expr), byte_code);

        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_IF_FALSE_REL, {std::monostate()}));

        bool has_else_clause = statement->get_else_body().get() != nullptr;

        size_t zero_index = byte_code.size() - 1;// j*
        size_t if_end_index = 0;
        size_t else_clause_index = 0;

        generate_statement(static_cast<StatementNode*>(statement->get_body().get()), byte_code);
        if_end_index = byte_code.size();

        if (has_else_clause) {
            // in this case the 'end of if clause' is the jump to the end of the if statement
            // as we don't want it to jump directly to the end, so we add the offset by 1
            byte_code[zero_index].param = IRJumpRelParam(if_end_index + 1 - zero_index);

            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_REL, IRJumpRelParam(0)));
            generate_statement(static_cast<StatementNode*>(statement->get_else_body().get()), byte_code);
            else_clause_index = byte_code.size();
            byte_code[if_end_index].param = IRJumpRelParam(else_clause_index - if_end_index);
        } else {
            byte_code[zero_index].param = IRJumpRelParam(if_end_index - zero_index);
        }
    }

    void IRGenerator::generate_while_statement(const WhileNode* statement, ByteCode& byte_code) {
        auto* expr = statement->get_condition().get();

        size_t while_start_index, while_end_index, while_start_jmp_index;
        while_start_index = byte_code.size();

        generate_expression(static_cast<const ExpressionNode*>(expr), byte_code);

        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_IF_FALSE_REL, {std::monostate()}));

        while_start_jmp_index = byte_code.size() - 1;

        while_loop_generation_stack.push(WhileLoopGenerationContext{});

        generate_statement(
                static_cast<StatementNode*>(statement->get_body().get()),
                byte_code);

        ssize_t current_jmp_index = byte_code.size() - 1;

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::JMP_REL,
                IRJumpRelParam(while_start_index - current_jmp_index - 1)));

        // the first command after the loop
        while_end_index = byte_code.size();

        // fill in jump target for unmet condition
        byte_code[while_start_jmp_index].param = IRJumpRelParam(while_end_index - while_start_jmp_index);

        auto generation_ctx = while_loop_generation_stack.top();
        while_loop_generation_stack.pop();
        // fill in jump targets of breaks
        for (auto break_instruction: generation_ctx.break_instructions) {
            byte_code[break_instruction].param = IRJumpRelParam(while_end_index - break_instruction);
        }

        // fill in jump targets of continues
        for (auto continue_instruction: generation_ctx.continue_instructions) {
            byte_code[continue_instruction].param = IRJumpRelParam(while_start_index - continue_instruction);
        }
    }

    void IRGenerator::generate_for_statement(const ForNode* statement, ByteCode& byte_code) {
        auto* init_stmt = static_cast<StatementNode*>(statement->get_init_stmt().get());
        generate_statement(init_stmt, byte_code);

        size_t condition_start_index = byte_code.size();// the command after decl / assignment

        auto* cond_expr = statement->get_condition_expr().get();
        generate_expression(static_cast<const ExpressionNode*>(cond_expr), byte_code);

        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::TO_BOOL, {std::monostate()}));
        byte_code.push_back(IRInstruction(IRInstruction::InstructionType::JMP_IF_FALSE_REL, {std::monostate()}));

        size_t jump_to_end_of_loop = byte_code.size() - 1;// jmp_if_false

        while_loop_generation_stack.push(WhileLoopGenerationContext{});

        auto* body = static_cast<StatementNode*>(statement->get_body().get());
        generate_statement(body, byte_code);

        // update at the end of the loop
        auto* update_stmt = static_cast<StatementNode*>(statement->get_update_stmt().get());
        generate_statement(update_stmt, byte_code);

        ssize_t current_jmp_index = byte_code.size() - 1;

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::JMP_REL, IRJumpRelParam(condition_start_index - current_jmp_index - 1)));

        size_t loop_end_index = byte_code.size();

        byte_code[jump_to_end_of_loop].param = IRJumpRelParam(loop_end_index - jump_to_end_of_loop);

        auto generation_context = while_loop_generation_stack.top();
        while_loop_generation_stack.pop();

        // similar to while loops
        for (auto break_instruction: generation_context.break_instructions) {
            byte_code[break_instruction].param = IRJumpRelParam(loop_end_index - break_instruction);
        }

        for (auto continue_instruction: generation_context.continue_instructions) {
            byte_code[continue_instruction].param = IRJumpRelParam(condition_start_index - continue_instruction);
        }
    }

    void IRGenerator::generate_break_statement(ByteCode& byte_code) {
        assert(while_loop_generation_stack.size() > 0);

        auto& ctx = while_loop_generation_stack.top();
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::JMP_REL, IRJumpRelParam(0)));
        ctx.register_break_instruction(byte_code.size() - 1);
    }

    void IRGenerator::generate_continue_statement(ByteCode& byte_code) {
        assert(while_loop_generation_stack.size() > 0);

        auto& ctx = while_loop_generation_stack.top();
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::JMP_REL, IRJumpRelParam(0)));
        ctx.register_continue_instruction(byte_code.size() - 1);
    }

    IRInterpreter::IRInterpreter(IRRuntime& runtime) : runtime(runtime) {
        push_stack_frame();// global scope
        preload_native_functions();
    }

    IRInterpreter::~IRInterpreter() {
        // assert(stack_frames.size() == 1);
        if (stack_frames.size() != 1) {
            std::cerr << "Abnormal quit with corrupted stack!" << std::endl;
        }
        pop_stack_frame();
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
        generate_expression(static_cast<const ExpressionNode*>(node->get_function_identifier().get()), byte_code);

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::CALL, IRCallParam{arguments_count}));
    }

    void IRGenerator::generate_function_declaration_statement(
            const FunctionDeclarationNode* statement,
            ByteCode& byte_code) {
        // forward declaration
        if (statement->get_function_body() == nullptr) {
            // todo: i'm not sure if this requires further handling.
            return;
        }

        size_t jump_over_function_instruction_index = byte_code.size();
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::JMP_REL,
                IRJumpRelParam(0)));

        size_t fn_start_index = byte_code.size();

        for (auto& param: statement->get_parameters()) {
            auto identifier =
                    runtime.push_string_pool_if_not_exists(dynamic_cast<IdentifierNode*>(param.get())->get_name());
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::DECLARE_IDENTIFIER,
                    IRDeclareIdentifierParam{identifier}));
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::STORE_IDENTIFIER,
                    IRStoreIdentifierParam{identifier}));
        }

        generate_program_or_block(statement->get_function_body().get(), byte_code);

        // no return value
        if (byte_code.back().type != IRInstruction::InstructionType::RET) {
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::LOAD_CONST, IRLoadConstParam{IRPrimValue::unit()}));
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::RET, {std::monostate()}));
        }

        byte_code[jump_over_function_instruction_index].param =
                IRJumpRelParam(byte_code.size() - jump_over_function_instruction_index);

        auto current_module_id = get_current_compiling_module_id();
        auto function_identifier =
                static_cast<IdentifierNode*>(statement->get_identifier().get())->get_name();

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::MAKE_FUNC,
                IRMakeFunctionParam{
                        fn_start_index,
                        current_module_id,
                        statement->get_parameters().size(),
                        false, false}));

        auto* cached_function_identifier = runtime.push_string_pool_if_not_exists(function_identifier);

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::DECLARE_IDENTIFIER,
                IRDeclareIdentifierParam{cached_function_identifier}));

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::STORE_IDENTIFIER,
                IRStoreIdentifierParam{cached_function_identifier}));
    }

    void IRGenerator::generate_method_declaration_statement(const MethodDeclarationNode* statement, ByteCode& byte_code) {
        // forward declaration
        if (statement->get_function_body() == nullptr) {
            return;
        }

        size_t jump_over_function_instruction_index = byte_code.size();
        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::JMP_REL,
                IRJumpRelParam(0)));

        size_t fn_start_index = byte_code.size();

        for (auto& param: statement->get_parameters()) {
            auto identifier =
                    runtime.push_string_pool_if_not_exists(dynamic_cast<IdentifierNode*>(param.get())->get_name());

            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::DECLARE_IDENTIFIER,
                    IRDeclareIdentifierParam{identifier}));

            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::STORE_IDENTIFIER,
                    IRStoreIdentifierParam{identifier}));
        }

        generate_program_or_block(statement->get_function_body().get(), byte_code);

        // no return value
        if (byte_code.back().type != IRInstruction::InstructionType::RET) {
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::LOAD_CONST, IRLoadConstParam{IRPrimValue::unit()}));
            byte_code.push_back(IRInstruction(IRInstruction::InstructionType::RET, {std::monostate()}));
        }

        byte_code[jump_over_function_instruction_index].param =
                IRJumpRelParam(byte_code.size() - jump_over_function_instruction_index);

        auto current_module_id = get_current_compiling_module_id();
        auto function_identifier =
                static_cast<IdentifierNode*>(statement->get_identifier().get())->get_name();

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::MAKE_FUNC,
                IRMakeFunctionParam{
                        fn_start_index,
                        current_module_id,
                        statement->get_parameters().size(),
                        true, false}));

        auto* cached_function_identifier = runtime.push_string_pool_if_not_exists(function_identifier);

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::DECLARE_IDENTIFIER,
                IRDeclareIdentifierParam{cached_function_identifier}));

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::STORE_IDENTIFIER, IRStoreIdentifierParam{cached_function_identifier}));
    }

    void IRGenerator::generate_return_statement(const ReturnNode* statement, ByteCode& byte_code) {
        if (statement->get_expression().get() == nullptr) {
            // no return value
            byte_code.push_back(IRInstruction(
                    IRInstruction::InstructionType::LOAD_CONST, IRLoadConstParam{IRPrimValue::unit()}));
        } else {
            generate_expression(static_cast<ExpressionNode*>(statement->get_expression().get()), byte_code);
        }

        byte_code.push_back(IRInstruction(
                IRInstruction::InstructionType::RET, {std::monostate()}));
    }

    void IRInterpreter::run() {
        size_t size = byte_code.size();
        while (pc < size) {
            bool jumped = false;

            auto& instruction = byte_code[pc];
            switch (instruction.type) {
                case IRInstruction::InstructionType::LOAD_CONST:
                    push_op_stack(std::get<IRLoadConstParam>(instruction.param));
                    break;
                case IRInstruction::InstructionType::DECLARE_IDENTIFIER: {
                    declare_identifier(std::get<IRDeclareIdentifierParam>(instruction.param).identifier);
                    break;
                }
                case IRInstruction::InstructionType::LOAD_IDENTIFIER: {
                    auto param = std::get<IRLoadIdentifierParam>(instruction.param);
                    push_op_stack(retrieve_raw_value(param.identifier));
                    break;
                }
                case IRInstruction::InstructionType::STORE_IDENTIFIER: {
                    auto param = std::get<IRStoreIdentifierParam>(instruction.param);

                    auto value = pop_op_stack();

                    store_raw_value(param.identifier, value);
                    break;
                }

                case IRInstruction::InstructionType::LOAD_MODULE: {
                    handle_module_load(std::get<IRLoadModuleParam>(instruction.param));
                    break;
                }

                case IRInstruction::InstructionType::POP_STACK: {
                    pop_op_stack();
                    break;
                }

                case IRInstruction::InstructionType::PEEK: {
                    peek_op_stack();
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
                    jumped = handle_binary_op(instruction.type);
                    break;

                case IRInstruction::InstructionType::NOT:
                case IRInstruction::InstructionType::LOGICAL_NOT:
                case IRInstruction::InstructionType::NEGATE:
                    jumped = handle_unary_op(instruction.type);
                    break;

                case IRInstruction::InstructionType::JMP:
                case IRInstruction::InstructionType::JMP_IF_FALSE:
                    jumped = handle_jump(instruction.type, std::get<IRJumpParam>(instruction.param));
                    break;

                case IRInstruction::InstructionType::JMP_REL:
                case IRInstruction::InstructionType::JMP_IF_FALSE_REL:
                    jumped = handle_relative_jump(instruction.type, std::get<IRJumpRelParam>(instruction.param));
                    break;

                case IRInstruction::InstructionType::CALL: {
                    auto param = std::get<IRCallParam>(instruction.param);
                    jumped = handle_function_invocation(param);
                    break;
                }

                case IRInstruction::InstructionType::RET: {
                    handle_return();
                    jumped = true;
                    break;
                }

                case IRInstruction::InstructionType::MAKE_STRING: {
                    handle_make_string();
                    break;
                }

                case IRInstruction::InstructionType::MAKE_FUNC: {
                    handle_make_function(std::get<IRMakeFunctionParam>(instruction.param));
                    break;
                }

                case IRInstruction::InstructionType::MAKE_TYPE: {
                    handle_type_creation();
                    break;
                }

                case IRInstruction::InstructionType::MAKE_OBJECT: {
                    handle_make_object(std::get<IRMakeObjectParam>(instruction.param));
                    break;
                }

                case IRInstruction::InstructionType::MAKE_MODULE: {
                    handle_make_module(std::get<IRMakeModuleParam>(instruction.param));
                    break;
                }
                case IRInstruction::InstructionType::MAKE_MODULE_LOCAL: {
                    handle_make_module_local();
                    break;
                }

                case IRInstruction::InstructionType::BEGIN_LOCAL: {
                    push_stack_frame(false);
                    break;
                }
                case IRInstruction::InstructionType::END_LOCAL: {
                    pop_stack_frame();
                    break;
                }

                case IRInstruction::InstructionType::BEGIN_LOCAL_DERIVED: {
                    push_stack_frame(true);
                    break;
                }

                case IRInstruction::InstructionType::LOAD_MEMBER: {
                    handle_member_load(std::get<IRLoadMemberParam>(instruction.param));
                    break;
                }
                case IRInstruction::InstructionType::STORE_MEMBER: {
                    handle_member_store(std::get<IRStoreMemberParam>(instruction.param));
                    break;
                }

                case IRInstruction::InstructionType::LOAD_INDEXOF: {
                    jumped = handle_index_load();
                    break;
                }
                case IRInstruction::InstructionType::STORE_INDEXOF: {
                    jumped = handle_index_store();
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
                auto cond = pop_op_stack().to_bool();
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


    bool IRInterpreter::handle_relative_jump(IRInstruction::InstructionType op, IRJumpRelParam param) {
        switch (op) {
            case IRInstruction::InstructionType::JMP_REL: {
                pc += param;
                return true;
            }
            case IRInstruction::InstructionType::JMP_IF_FALSE_REL: {
                auto cond = pop_op_stack().to_bool();
                if (!(cond & 1)) {
                    pc += param;
                    return true;
                }
                break;
            }
            default:
                throw IRInterpreterException("Unknown instruction type");
        }
        return false;
    }

    void IRInterpreter::handle_type_creation() {
        auto guard = runtime.gc_guard();

        TypeObject* type_info = runtime.gc_allocate<TypeObject>();

        std::vector<FunctionObject*> functions;

        for (auto& [name, value]: current_stack_frame().variables) {
            if (value.get_type() == ValueType::Type) {
                type_info->add_field(
                        name,
                        TypeObject::TypeField{
                                static_cast<TypeObject*>(value.get_inner_value<GCObject*>())});
            } else if (value.get_type() == ValueType::Function) {
                type_info->add_field(name, TypeObject::TypeField{TypeObject::function()});

                auto* fn = static_cast<FunctionObject*>(value.get_inner_value<GCObject*>());
                if (fn->is_method_function()) {
                    type_info->add_method(name, fn);
                } else {
                    type_info->add_static_method(name, fn);
                }
                functions.push_back(fn);

            } else {
                throw IRInterpreterException("Not a valid type");
            }
        }

        // set the context of the functions
        auto* ctx = freeze_context();
        for (auto* fn: functions) {
            fn->set_context(ctx);
        }

        push_op_stack(IRPrimValue(ValueType::Type, static_cast<GCObject*>(type_info)));
    }

    bool IRInterpreter::handle_static_method_invocation(const PrimValue& value, StringObject* name) {
        auto* type = static_cast<TypeObject*>(value.get_inner_value<GCObject*>());
        if (type->has_static_method(name)) {
            push_op_stack(IRPrimValue(ValueType::Function, (GCObject*){type->get_static_method(name)}));
            return true;
        }
        return false;
    }

    void IRInterpreter::handle_member_load(IRLoadMemberParam param) {
        auto name = param.identifier;
        auto object = pop_op_stack();

        if (!object.is_gc_object()) {
            throw IRInterpreterException("Not a valid object");
        }

        if (object.get_type() == ValueType::Type) {
            if (handle_static_method_invocation(object, name)) {
                return;
            }
        }

        auto* object_ptr = object.get_inner_value<GCObject*>();

        if (object_ptr->storage.fields.find(name) == object_ptr->storage.fields.end()) {
            std::string name_str = name->to_string();
            throw IRInterpreterException("Object does not contain such field: " + name_str);
        }

        push_op_stack(object_ptr->storage.fields[name]);
    }

    void IRInterpreter::handle_member_store(IRStoreMemberParam param) {
        auto name = param.identifier;
        auto value = pop_op_stack();
        auto object = pop_op_stack();

        if (!object.is_gc_object()) {
            throw IRInterpreterException("Not a valid object");
        }

        auto* object_ptr = object.get_inner_value<GCObject*>();

        if (object_ptr->storage.fields.find(name) == object_ptr->storage.fields.end()) {
            std::string name_str = name->to_string();
            throw IRInterpreterException("Object does not contain such field: " + name_str);
        } else {
            // force the type of the value to the desired member type
            // as sometimes the member type can be 'Any'
            value.set_type_info(object.get_type_info()->get_field(name).type_ptr);
        }

        object_ptr->storage.fields[name] = value;
    }

    bool IRInterpreter::handle_index_load() {
        bool jumped = false;

        auto index = pop_op_stack();

        size_t index_value = index.get_inner_value<Int>();

        auto object = pop_op_stack();

        if (object.get_type() == ValueType::Array) {
            if (index.get_type() != ValueType::Int) {
                throw IRInterpreterException("Non-integer index value is not supported for arrays");
            }

            auto* array = static_cast<ArrayObject*>(object.get_inner_value<GCObject*>());

            if (index_value >= array->get_size()) {
                throw IRInterpreterException("Index out of bounds");
            }

            push_op_stack(array->get_element(index_value));

            return jumped;
        } else if (object.get_type() == ValueType::Object) {
            auto* gc_object = object.get_inner_value<GCObject*>();
            auto* op_index_at = runtime.push_string_pool_if_not_exists("opIndexAt");

            if (gc_object->storage.fields.find(op_index_at) != gc_object->storage.fields.end()) {
                push_op_stack(index);
                push_op_stack(object);
                push_op_stack(gc_object->storage.fields[op_index_at]);

                jumped = handle_function_invocation(IRCallParam{2});

                return jumped;
            }
        }

        throw IRInterpreterException("The object is neither an array nor an object with 'opIndexAt' method");
    }

    bool IRInterpreter::handle_index_store() {
        bool jumped = false;

        auto index = pop_op_stack();

        auto value = pop_op_stack();

        auto object = pop_op_stack();

        if (object.get_type() == ValueType::Array) {
            if (index.get_type() != ValueType::Int) {
                throw IRInterpreterException("Non-integer index value is not supported for arrays");
            }
            size_t index_value = index.get_inner_value<Int>();

            auto* array = static_cast<ArrayObject*>(object.get_inner_value<GCObject*>());

            if (index_value >= array->get_size()) {
                throw IRInterpreterException("Index out of bounds");
            }

            if (value.get_type_info() != array->get_element_type()) {
                throw IRInterpreterException("R-value of assignment does not correspond with the array element type");
            }

            array->get_element_ref(index_value) = value;

            return jumped;
        } else if (object.get_type() == ValueType::Object) {
            auto* gc_object = object.get_inner_value<GCObject*>();
            auto* op_index_at = runtime.push_string_pool_if_not_exists("opIndexAssign");

            if (gc_object->storage.fields.find(op_index_at) != gc_object->storage.fields.end()) {
                push_op_stack(value);
                push_op_stack(index);
                push_op_stack(object);
                push_op_stack(gc_object->storage.fields[op_index_at]);

                jumped = handle_function_invocation(IRCallParam{3});

                return jumped;
            }
        }

        throw IRInterpreterException("The object is neither an array nor an object with 'opIndexAssign' method");
    }

    void IRInterpreter::handle_module_load(IRLoadModuleParam param) {
        auto* module = runtime.get_module(param.module_id).module;

        auto value = PrimValue(ValueType::Module, (GCObject*){module});
        push_op_stack(value);
    }

    void IRInterpreter::handle_make_string() {
        auto string_literal_ref = pop_op_stack();

        StringObject* string_object = new StringObject(
                *static_cast<StringObject*>(string_literal_ref.get_inner_value<GCObject*>()));
        runtime.gc_regist(string_object);

        auto value = IRPrimValue(ValueType::String, (GCObject*){string_object});
        push_op_stack(value);
    }

    void IRInterpreter::handle_make_function(IRMakeFunctionParam param) {
        auto guard = runtime.gc_guard();

        FunctionObject* func_obj;
        if (param.is_method) {
            func_obj = FunctionObject::create_method(param.begin_offset, param.module_id, param.arity);
        } else {
            func_obj = FunctionObject::create_function(param.begin_offset, param.module_id, param.arity);
        }

        // don't freeze context when not necessary
        if (param.is_closure) {
            auto* ctx = freeze_context();

            if (has_context()) {
                ctx->set_next(get_context());
            }

            func_obj->set_context(ctx);
        }

        runtime.gc_regist(func_obj);

        auto value = IRPrimValue(ValueType::Function, (GCObject*){func_obj});
        push_op_stack(value);
    }

    void IRInterpreter::handle_make_object(IRMakeObjectParam param) {
        auto guard = runtime.gc_guard();

        auto type = pop_op_stack();
        if (type.get_type() != ValueType::Type) {
            throw IRInterpreterException("Not a valid type for object creation");
        }

        auto* type_info = static_cast<TypeObject*>(type.get_inner_value<GCObject*>());

        auto& fields = param.fields;
        auto* gc_object = runtime.gc_allocate<GCObject>();

        bool validation_enabled = type_info != TypeObject::any();

        for (auto [name, _]: type_info->get_fields()) {
            gc_object->storage.fields[name] = PrimValue::null();
        }

        for (auto [name, fn]: type_info->get_methods()) {
            gc_object->storage.fields[name] = PrimValue(ValueType::Function, fn);
        }

        for (auto* field: fields) {
            // validation
            if (validation_enabled && !type_info->has_field(field)) {
                throw IRInterpreterException("Object has no field named: " + field->to_string());
            }

            gc_object->storage.fields[field] = pop_op_stack();
        }

        auto value = PrimValue(ValueType::Object, (GCObject*){gc_object});
        value.set_type_info(type_info);

        push_op_stack(value);
    }

    void IRInterpreter::handle_make_module(IRMakeModuleParam param) {
        auto* gc_object = handle_make_module_local();

        auto& module_registry_metadata = runtime.get_module(param.module_id);
        module_registry_metadata.module = gc_object;
    }

    GCObject* IRInterpreter::handle_make_module_local() {
        auto guard = runtime.gc_guard();

        auto* gc_object = runtime.gc_allocate<GCObject>();

        std::vector<FunctionObject*> functions;
        for (auto& [name, value]: current_stack_frame().variables) {
            gc_object->storage.fields[name] = value;

            if (value.get_type() == ValueType::Function) {
                functions.push_back(static_cast<FunctionObject*>(value.get_inner_value<GCObject*>()));
            }
        }

        auto* ctx = freeze_context();
        for (auto* fn: functions) {
            fn->set_context(ctx);
        }

        auto value = PrimValue(ValueType::Module, (GCObject*){gc_object});
        value.set_type_info(TypeObject::any());

        push_op_stack(value);

        return gc_object;
    }

    void IRInterpreter::handle_to_bool() {
        auto& top = op_stack_top();
        top = PrimValue::from_bool(top.to_bool());
    }

    bool IRInterpreter::handle_function_invocation(IRCallParam param) {
        auto fn_obj = pop_op_stack();

        if (fn_obj.get_type() != ValueType::Function) {
            throw IRInterpreterException("Cannot invoke non-function");
        }

        auto fn =
                dynamic_cast<FunctionObject*>(fn_obj.get_inner_value<GCObject*>());

        if (fn->is_native_function()) {
            // only non-native functions generate RET command
            // so we don't push a stack frame here,
            // for no one will be responsible for popping it.
            std::vector<IRPrimValue> args(param.arguments_count);
            for (size_t i = 0; i < param.arguments_count; i++) {
                args[i] = pop_op_stack();
            }
            auto ret = fn->call_native(args);
            push_op_stack(ret);
            return false;
        } else {
            size_t arg_size = fn->get_arity();

            if (param.arguments_count != arg_size) {
                throw IRInterpreterException(
                        "Function argument count mismatch, expected " +
                        std::to_string(arg_size) +
                        " got " +
                        std::to_string(param.arguments_count));
            }

            load_context(fn->get_context());
            push_stack_frame();
            size_t jump_target =
                    runtime.resolve_function_offset(
                            fn->get_module_id(),
                            fn->get_begin_offset());

            pc = jump_target;
            return true;
        }
    }

    void IRInterpreter::handle_return() {
        auto return_addr = current_stack_frame().return_addr;
        pop_stack_frame();
        restore_context();
        pc = return_addr;
    }

    std::optional<std::array<IRPrimValue, 2>> IRInterpreter::is_binary_op_dynamically_dispatchable(IRPrimValue lhs, IRPrimValue rhs) {
        if (lhs.is_gc_object()) {
            return {{lhs, rhs}};
        } else if (rhs.is_gc_object()) {
            return {{rhs, lhs}};
        }
        return std::nullopt;
    }

    bool IRInterpreter::dispatch_binary_op(IRPrimValue lhs, IRPrimValue rhs, const std::string& identifier) {
        auto* dispatch_operator_identifier = runtime.push_string_pool_if_not_exists(identifier);

        // by default, the converted lhs must be a gc object
        auto* lhs_object = lhs.get_inner_value<GCObject*>();

        if (lhs_object->storage.fields.find(dispatch_operator_identifier) != lhs_object->storage.fields.end()) {
            push_op_stack(rhs);
            push_op_stack(lhs);
            push_op_stack(lhs_object->storage.fields[dispatch_operator_identifier]);

            return handle_function_invocation(IRCallParam{2});
        } else if (rhs.is_gc_object()) {
            // if the lhs does not have a dispatched operator, try to see if the rhs has one

            auto* rhs_object = rhs.get_inner_value<GCObject*>();
            if (rhs_object->storage.fields.find(dispatch_operator_identifier) != rhs_object->storage.fields.end()) {
                push_op_stack(lhs);
                push_op_stack(rhs);
                push_op_stack(rhs_object->storage.fields[dispatch_operator_identifier]);

                return handle_function_invocation(IRCallParam{2});
            }
        }
        throw IRInterpreterException("Cannot find operator '" + dispatch_operator_identifier->contained_string() + "'");
    }

    bool IRInterpreter::handle_binary_op(IRInstruction::InstructionType op) {
        auto rhs_variant = pop_op_stack();
        auto lhs_variant = pop_op_stack();
        // the left node is visited first and entered the stack first,
        // so the right node is the top of the stack

        return handle_binary_op(op, lhs_variant, rhs_variant);
    }

    bool IRInterpreter::handle_unary_op(IRInstruction::InstructionType op) {
        auto rhs_variant = pop_op_stack();

        return handle_unary_op(op, rhs_variant);
    }

#define __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP(_op) \
    if (dynamically_dispatchable) {                              \
        return dispatch_binary_op(                               \
                dispatchable_pair.value()[0],                    \
                dispatchable_pair.value()[1],                    \
                _op);                                            \
    }

    bool IRInterpreter::handle_binary_op(IRInstruction::InstructionType op, IRPrimValue lhs, IRPrimValue rhs) {
        auto dispatchable_pair = is_binary_op_dynamically_dispatchable(lhs, rhs);
        bool dynamically_dispatchable = dispatchable_pair.has_value();

        switch (op) {
            case IRInstruction::InstructionType::ADD:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opAdd");
                push_op_stack(detail::prim_value_add(lhs, rhs));
                break;
            case IRInstruction::InstructionType::SUB:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opSub");
                push_op_stack(detail::prim_value_sub(lhs, rhs));
                break;
            case IRInstruction::InstructionType::MUL:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opMul");
                push_op_stack(detail::prim_value_mul(lhs, rhs));
                break;
            case IRInstruction::InstructionType::DIV:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opDiv");
                push_op_stack(detail::prim_value_div(lhs, rhs));
                break;
            case IRInstruction::InstructionType::MOD:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opModulo");
                push_op_stack(detail::prim_value_mod(lhs, rhs));
                break;
            case IRInstruction::InstructionType::AND:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opBitwiseAnd");
                push_op_stack(detail::prim_value_band(lhs, rhs));
                break;
            case IRInstruction::InstructionType::LOGICAL_AND: {
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opLogicalAnd");
                push_op_stack(detail::prim_value_land(lhs, rhs));
                break;
            }
            case IRInstruction::InstructionType::OR:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opBitwiseOr");
                push_op_stack(detail::prim_value_bor(lhs, rhs));
                break;
            case IRInstruction::InstructionType::LOGICAL_OR:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opLogicalOr");
                push_op_stack(detail::prim_value_lor(lhs, rhs));
                break;
            case IRInstruction::InstructionType::XOR:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opXor");
                push_op_stack(detail::prim_value_bxor(lhs, rhs));
                break;
            case IRInstruction::InstructionType::SHL:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opShiftLeft");
                push_op_stack(detail::prim_value_shl(lhs, rhs));
                break;
            case IRInstruction::InstructionType::SHR:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opShiftRight");
                push_op_stack(detail::prim_value_shr(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_EQ:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opCompareEqual");
                push_op_stack(detail::prim_value_eq(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_NE:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opCompareNotEqual");
                push_op_stack(detail::prim_value_neq(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_LT:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opCompareLessThan");
                push_op_stack(detail::prim_value_lt(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_LE:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opCompareLessThanOrEqual");
                push_op_stack(detail::prim_value_lte(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_GT:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opCompareGreaterThan");
                push_op_stack(detail::prim_value_gt(lhs, rhs));
                break;
            case IRInstruction::InstructionType::CMP_GE:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_BINARY_OP("opCompareGreaterThanOrEqual");
                push_op_stack(detail::prim_value_gte(lhs, rhs));
                break;
            default:
                throw IRInterpreterException("Invalid instruction type");
        }
        return false;
    }

    bool IRInterpreter::dispatch_unary_op(IRPrimValue value, const std::string& identifier) {
        auto* dispatch_operator_identifier = runtime.push_string_pool_if_not_exists(identifier);

        auto* object = value.get_inner_value<GCObject*>();
        if (object->storage.fields.find(dispatch_operator_identifier) != object->storage.fields.end()) {
            push_op_stack(value);
            push_op_stack(object->storage.fields[dispatch_operator_identifier]);

            return handle_function_invocation(IRCallParam{1});
        }
        throw IRInterpreterException("Cannot find unary operator '" + dispatch_operator_identifier->contained_string() + "'");
    }

#define __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_UNARY_OP(_op) \
    if (dynamically_dispatchable) {                             \
        return dispatch_unary_op(rhs, _op);                     \
    }

    bool IRInterpreter::handle_unary_op(IRInstruction::InstructionType op, IRPrimValue rhs) {
        bool dynamically_dispatchable = rhs.is_gc_object();

        switch (op) {
            case IRInstruction::InstructionType::NEGATE:
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_UNARY_OP("opNegate");
                push_op_stack(detail::prim_value_neg(rhs));
                break;
            case IRInstruction::InstructionType::NOT: {
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_UNARY_OP("opBitwiseNot");
                push_op_stack(detail::prim_value_bnot(rhs));
                break;
            }
            case IRInstruction::InstructionType::LOGICAL_NOT: {
                __LUAXC_IR_INTERPRETER_UTILS_TRY_DISPATCH_UNARY_OP("opLogicalNot");
                push_op_stack(detail::prim_value_lnot(rhs));
                break;
            }
            default:
                throw IRInterpreterException("Invalid instruction type");
        }
        return false;
    }

    void IRInterpreter::declare_identifier(StringObject* identifier) {
        current_stack_frame().variables[identifier] = IRPrimValue::null();
    }

    IRPrimValue IRInterpreter::retrieve_raw_value(StringObject* identifier) {
        if (auto value = retrieve_identifier_in_stack_frame(identifier)) {
            return value.value();
        } else if (auto captured_ctx = retrieve_value_in_stored_context(identifier)) {
            return captured_ctx.value();
        } else if (has_identifier_in_global_scope(identifier)) {
            return retrieve_raw_value_in_global_scope(identifier);
        } else {
            throw IRInterpreterException("Identifier not found: " + identifier->to_string());
        }
    }

    IRPrimValue IRInterpreter::retrieve_raw_value(const std::string& identifier) {
        return retrieve_raw_value(runtime.push_string_pool_if_not_exists(identifier));
    }

    IRPrimValue& IRInterpreter::retrieve_raw_value_ref(StringObject* identifier) {
        if (auto value = retrieve_identifier_ref_in_stack_frame(identifier)) {
            return *value.value();
        } else if (has_identifier_in_global_scope(identifier)) {
            return retrieve_value_ref_in_global_scope(identifier);
        } else {
            throw IRInterpreterException("Identifier not found: " + identifier->to_string());
        }
    }

    IRPrimValue& IRInterpreter::retrieve_raw_value_ref(const std::string& identifier) {
        return retrieve_raw_value_ref(runtime.push_string_pool_if_not_exists(identifier));
    }

    void IRInterpreter::store_raw_value(StringObject* identifier, IRPrimValue value) {
        if (auto ref = retrieve_identifier_ref_in_stack_frame(identifier)) {
            *ref.value() = value;
        } else if (has_identifier_in_global_scope(identifier)) {
            store_value_in_global_scope(identifier, value);
        } else if (auto captured_ctx = retrieve_value_in_stored_context(identifier)) {
            throw IRInterpreterException("Cannot modify immutable captured variable " + identifier->to_string());
        } else {
            throw IRInterpreterException("Identifier not found " + identifier->to_string());
        }
    }

    bool IRInterpreter::has_identifier(StringObject* identifier) {
        return retrieve_identifier_in_stack_frame(identifier).has_value() || has_identifier_in_global_scope(identifier);
    }

    bool IRInterpreter::has_identifier(const std::string& identifier) {
        return has_identifier(runtime.push_string_pool_if_not_exists(identifier));
    }

    std::unique_ptr<AstNode> IRRuntime::generate(const std::string& input, Parser::ParserState init_state) {
        auto lexer = Lexer(input);
        auto parser = Parser(lexer);

        return parser.parse_program(init_state);
    }

    void IRRuntime::compile(const std::string& input) {
        auto program = generate(input);

        generator = std::make_unique<IRGenerator>(*this, std::move(program));
        generator->inject_module_compiler(
                [this](const std::string& name) {
                    return generate(name, Parser::ParserState::InModuleDeclarationScope);
                });

        interpreter = std::make_unique<IRInterpreter>(*this);

        byte_code = generator->generate();
    }

    void IRRuntime::run() {
        if (byte_code.size() == 0) {
            throw std::runtime_error("Not compiled");
        }

        gc.init(interpreter->get_op_stack_ptr(),
                interpreter->get_stack_frames_ptr());

        gc.set_gc_enabled(true);

        interpreter->set_byte_code(byte_code);
        interpreter->run();
    }

    std::string IRRuntime::find_file_and_read(const std::string& module_path) {
        auto& cwd = runtime_ctx.cwd;
        auto& import_path = runtime_ctx.import_path;

        std::filesystem::path full_path;

        full_path = std::filesystem::path(cwd) / module_path;
        if (std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path)) {
            std::ifstream file_stream(full_path);
            if (file_stream.is_open()) {
                std::ostringstream oss;
                oss << file_stream.rdbuf();
                return oss.str();
            }
        }

        full_path = std::filesystem::path(import_path) / module_path;
        if (std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path)) {
            std::ifstream file_stream(full_path);
            if (file_stream.is_open()) {
                std::ostringstream oss;
                oss << file_stream.rdbuf();
                return oss.str();
            }
        }

        return "";
    }

    size_t IRRuntime::add_module(StringObject* module_name, size_t base_offset) {
        size_t id = module_manager.module_count;
        module_manager.module_count++;

        module_manager.modules.emplace(
                id, ImportedModule{module_name, id,
                                   base_offset});

        return id;
    }

    size_t IRRuntime::resolve_function_offset(size_t module_id, size_t function_offset) {
        return module_manager.modules[module_id].base_offset + function_offset;
    }

    std::optional<size_t> IRRuntime::has_module(StringObject* module_name) {
        for (auto& [id, module]: module_manager.modules) {
            if (module.name == module_name) {
                return id;
            }
        }
        return std::nullopt;
    }

    void IRRuntime::resolve_runtime_ctx() {
        runtime_ctx.cwd = std::filesystem::current_path().string();
        runtime_ctx.import_path = runtime_ctx.cwd;
    }

    void IRInterpreter::preload_native_functions() {
        Functions native_funtions;

        auto io_fn = luaxc::io().load(runtime);
        native_funtions.insert(native_funtions.end(), io_fn.begin(), io_fn.end());

        auto typing_fn = luaxc::typing().load(runtime);
        native_funtions.insert(native_funtions.end(), typing_fn.begin(), typing_fn.end());

        auto runtime_fn = luaxc::runtime().load(runtime);
        native_funtions.insert(native_funtions.end(), runtime_fn.begin(), runtime_fn.end());

        for (auto [name, fn]: native_funtions) {
            store_value_in_global_scope(name, fn);
        }
    }

    FrozenContextObject* IRInterpreter::freeze_context() {
        auto guard = runtime.gc_guard();

        auto* ctx = runtime.gc_allocate<FrozenContextObject>();

        std::deque<SharedStackFrameRef> frozen;

        for (auto it = stack_frames.rbegin(); it != stack_frames.rend(); ++it) {
            frozen.push_front(it->make_ref());

            if (!it->allow_upward_propagation) {
                break;
            }
        }

        // global scope variables
        // frozen.push_front(global_stack_frame());

        ctx->set_stack_frame(std::vector(frozen.begin(), frozen.end()));
        return ctx;
    }

    void IRInterpreter::push_stack_frame(bool allow_propagation) {

#ifdef LUAXC_RUNTIME_STACK_OVERFLOW_PROTECTION_ENABLED
        if (stack_frames.size() >= LUAXC_RUNTIME_MAX_STACK_SIZE) {
            throw IRInterpreterException("Stack overflow");
        }
#endif

        size_t return_addr = pc + 1;
        stack_frames.emplace_back(return_addr, allow_propagation);
    }

    void IRInterpreter::pop_stack_frame() {
        // set the value of all the pending StackFrameRefs
        stack_frames.back().notify_return();

        stack_frames.pop_back();
    }

    StackFrame& IRInterpreter::current_stack_frame() {
        assert(stack_frames.size() > 0);
        return stack_frames.back();
    }

    StackFrame& IRInterpreter::global_stack_frame() {
        assert(stack_frames.size() > 0);
        return stack_frames.front();
    }

    std::optional<PrimValue> IRInterpreter::retrieve_identifier_in_stack_frame(StringObject* identifier) {
        for (auto frame = stack_frames.rbegin(); frame != stack_frames.rend(); frame++) {
            if (frame->variables.find(identifier) != frame->variables.end()) {
                return {frame->variables[identifier]};
            }

            if (!frame->allow_upward_propagation) {
                break;
            }
        }

        return std::nullopt;
    }

    std::optional<PrimValue*> IRInterpreter::retrieve_identifier_ref_in_stack_frame(StringObject* identifier) {
        for (auto frame = stack_frames.rbegin(); frame != stack_frames.rend(); frame++) {
            if (frame->variables.find(identifier) != frame->variables.end()) {
                return {&frame->variables[identifier]};
            }

            if (!frame->allow_upward_propagation) {
                break;
            }
        }

        return std::nullopt;
    }

    bool IRInterpreter::has_identifier_in_global_scope(StringObject* identifier) {
        return global_stack_frame().variables.find(identifier) != global_stack_frame().variables.end();
    }

    std::optional<PrimValue> IRInterpreter::retrieve_value_in_stored_context(StringObject* identifier) {
        if (context_stack.size() > 0) {
            if (get_context() == nullptr) {
                return std::nullopt;
            }

            if (auto obj = get_context()->query(identifier)) {
                return obj;
            }
        }
        return std::nullopt;
    }

    IRPrimValue IRInterpreter::retrieve_raw_value_in_current_stack_frame(StringObject* identifier) {
        return current_stack_frame().variables.at(identifier);
    }

    IRPrimValue& IRInterpreter::retrieve_value_ref_in_current_stack_frame(StringObject* identifier) {
        return current_stack_frame().variables.at(identifier);
    }

    IRPrimValue IRInterpreter::retrieve_raw_value_in_global_scope(StringObject* identifier) {
        return global_stack_frame().variables.at(identifier);
    }

    IRPrimValue& IRInterpreter::retrieve_value_ref_in_global_scope(StringObject* identifier) {
        return global_stack_frame().variables.at(identifier);
    }

    void IRInterpreter::store_value_in_stack_frame(StringObject* identifier, IRPrimValue value) {
        current_stack_frame().variables[identifier] = value;
    }

    void IRInterpreter::store_value_in_global_scope(StringObject* identifier, IRPrimValue value) {
        global_stack_frame().variables[identifier] = value;
    }

    void IRRuntime::init_builtin_type_info() {
        auto static_type_info = TypeObject::get_all_static_type_info();

        for (const auto [name, type]: static_type_info) {
            type_info.emplace(name, type);
            gc_regist_no_collect(type);
        }
    }
}// namespace luaxc