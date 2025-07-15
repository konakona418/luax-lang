#pragma once

#include <cmath>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ast.hpp"
#include "gc.hpp"

namespace luaxc {

#define LUAXC_IR_RUNTIME_FORCE_BOOLEAN(_value) (!!(_value))

    class IRGeneratorException : public std::exception {
    public:
        std::string message;

        explicit IRGeneratorException(std::string message) : message(message) {}

        const char* what() const noexcept override { return message.c_str(); }
    };

    class IRInterpreterException : public std::exception {
    public:
        std::string message;

        explicit IRInterpreterException(std::string message) : message(message) {}

        const char* what() const noexcept override { return message.c_str(); }
    };

    using IRPrimValue = PrimValue;
    using IRLoadConstParam = IRPrimValue;

    struct IRLoadIdentifierParam {
        std::string identifier;
    };

    struct IRStoreIdentifierParam {
        std::string identifier;
    };

    using IRJumpParam = size_t;

    class IRInstruction {
    public:
        enum class InstructionType {
            LOAD_CONST,      // load a const to output
            LOAD_IDENTIFIER, // load an identifier to output
            STORE_IDENTIFIER,// store output to an identifier
            ADD,             // pop two values from stack, add them and push result to stack
            SUB,
            MUL,
            DIV,
            MOD,
            NEGATE,

            AND,
            LOGICAL_AND,
            OR,
            LOGICAL_OR,
            NOT,
            LOGICAL_NOT,
            XOR,
            SHL,
            SHR,

            CMP_EQ,
            CMP_NE,
            CMP_LT,
            CMP_GT,
            CMP_LE,
            CMP_GE,
            TO_BOOL,

            JMP,
            JMP_IF_FALSE,

            PUSH_STACK,// push output to stack
            POP_STACK, // pop value from stack
        };

        using IRParam = std::variant<
                std::monostate,
                IRLoadConstParam,
                IRLoadIdentifierParam,
                IRStoreIdentifierParam,
                IRJumpParam>;

        IRParam param;
        InstructionType type;

        IRInstruction(InstructionType type, IRParam param) : param(param), type(type) {}

        std::string dump() const;
    };

    using ByteCode = std::vector<IRInstruction>;

    std::string dump_bytecode(const ByteCode& bytecode);

    class IRGenerator {
    public:
        explicit IRGenerator(std::unique_ptr<AstNode> ast_base_node) : ast(std::move(ast_base_node)) {}

        ByteCode generate();

    private:
        struct WhileLoopGenerationContext {
            std::vector<size_t> break_instructions;
            std::vector<size_t> continue_instructions;
            // note: do not use pointer to the ast node here,
            // for that reallocations can happen in vectors.

            void register_break_instruction(size_t instruction) { break_instructions.push_back(instruction); }

            void register_continue_instruction(size_t instruction) { continue_instructions.push_back(instruction); }
        };

        std::unique_ptr<luaxc::AstNode> ast;

        std::stack<WhileLoopGenerationContext> while_loop_generation_stack;

        bool is_binary_logical_operator(BinaryExpressionNode::BinaryOperator op);

        bool is_combinative_assignment_operator(BinaryExpressionNode::BinaryOperator op);

        void generate_program_or_block(const AstNode* node, ByteCode& byte_code);

        void generate_statement(const StatementNode* statement, ByteCode& byte_code);

        void generate_expression(const AstNode* expression, ByteCode& byte_code);

        void generate_binary_expression_statement(const BinaryExpressionNode* statement, ByteCode& byte_code);

        void generate_combinative_assignment_statement(const BinaryExpressionNode* statement, ByteCode& byte_code);

        void generate_unary_expression_statement(const UnaryExpressionNode* statement, ByteCode& byte_code);

        void generate_declaration_statement(const DeclarationStmtNode* statement, ByteCode& byte_code);

        void generate_assignment_statement(const AssignmentStmtNode* statement, ByteCode& byte_code);

        void generate_if_statement(const IfNode* statement, ByteCode& byte_code);

        void generate_while_statement(const WhileNode* statement, ByteCode& byte_code);

        void generate_for_statement(const ForNode* statement, ByteCode& byte_code);

        void generate_break_statement(ByteCode& byte_code);

        void generate_continue_statement(ByteCode& byte_code);
    };

    class IRInterpreter {
    public:
        IRInterpreter();
        explicit IRInterpreter(ByteCode byte_code) : byte_code(std::move(byte_code)) {};

        void set_byte_code(ByteCode byte_code) { this->byte_code = std::move(byte_code); };

        void run();

        IRPrimValue retrieve_raw_value(const std::string& identifier);

        template<typename T>
        T retrieve_value(const std::string& identifier) {
            return retrieve_raw_value(identifier).get_inner_value<T>();
        }

        bool has_identifier(const std::string& identifier) const;

    private:
        ByteCode byte_code;
        size_t pc = 0;
        std::unordered_map<std::string, IRPrimValue> variables;
        std::stack<IRPrimValue> stack;
        IRPrimValue output;

        void handle_binary_op(IRInstruction::InstructionType op);

        void handle_unary_op(IRInstruction::InstructionType op);

        bool handle_jump(IRInstruction::InstructionType op, IRJumpParam param);

        void handle_to_bool();

        void handle_binary_op(IRInstruction::InstructionType op, IRPrimValue lhs, IRPrimValue rhs);

        void handle_unary_op(IRInstruction::InstructionType op, IRPrimValue rhs);
    };
}// namespace luaxc