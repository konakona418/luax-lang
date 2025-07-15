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

    struct IRCallParam {
        size_t arguments_count;
    };

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

            CALL,
            RET,
        };

        using IRParam = std::variant<
                std::monostate,
                IRLoadConstParam,
                IRLoadIdentifierParam,
                IRStoreIdentifierParam,
                IRJumpParam,
                IRCallParam>;

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

        void generate_function_invocation_statement(const FunctionInvocationNode* statement, ByteCode& byte_code);
    };

    class IRInterpreter {
    public:
        IRInterpreter();
        ~IRInterpreter();

        explicit IRInterpreter(ByteCode byte_code) : IRInterpreter() {
            this->byte_code = std::move(byte_code);
        };

        void set_byte_code(ByteCode byte_code) { this->byte_code = std::move(byte_code); };

        void run();

        IRPrimValue retrieve_raw_value(const std::string& identifier);

        IRPrimValue& retrieve_raw_value_ref(const std::string& identifier);

        template<typename T>
        T retrieve_value(const std::string& identifier) {
            return retrieve_raw_value(identifier).get_inner_value<T>();
        }

        bool has_identifier(const std::string& identifier);

    private:
        struct StackFrame {
            std::unordered_map<std::string, IRPrimValue> variables;
            size_t return_addr;
        };

        ByteCode byte_code;
        size_t pc = 0;
        std::vector<StackFrame> stack_frames;
        std::stack<IRPrimValue> stack;
        IRPrimValue output;

        std::vector<FunctionObject*> native_functions;

        void preload_native_functions();

        void free_native_functions();

        void push_stack_frame();

        void pop_stack_frame();

        StackFrame& current_stack_frame();

        bool has_identifier_in_stack_frame(const std::string& identifier);

        IRPrimValue retrieve_raw_value_in_stack_frame(const std::string& identifier);

        IRPrimValue& retrieve_value_ref_in_stack_frame(const std::string& identifier);

        void store_value_in_stack_frame(const std::string& identifier, IRPrimValue value);

        void handle_binary_op(IRInstruction::InstructionType op);

        void handle_unary_op(IRInstruction::InstructionType op);

        bool handle_jump(IRInstruction::InstructionType op, IRJumpParam param);

        void handle_to_bool();

        void handle_function_invocation(IRCallParam param);

        void handle_binary_op(IRInstruction::InstructionType op, IRPrimValue lhs, IRPrimValue rhs);

        void handle_unary_op(IRInstruction::InstructionType op, IRPrimValue rhs);
    };
}// namespace luaxc