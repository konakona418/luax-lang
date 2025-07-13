#pragma once

#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <cmath>

#include "ast.hpp"

namespace luaxc {

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

    using IRPrimValue = std::variant<int32_t, double>;
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
            LOAD_CONST, // load a const to output
            LOAD_IDENTIFIER, // load an identifier to output
            STORE_IDENTIFIER, // store output to an identifier
            ADD, // pop two values from stack, add them and push result to stack
            SUB,
            MUL,
            DIV,
            MOD,

            CMP, // compare two values on stack, 
            JMP, // jump to an address unconditionally
            JE, // jump to an address, based on the value in the output register
            JNE,
            JG,
            JL,
            JGE,
            JLE,

            PUSH_STACK, // push output to stack
            POP_STACK, // pop value from stack
        };

        using IRParam = std::variant<
            std::monostate,
            IRLoadConstParam,
            IRLoadIdentifierParam,
            IRStoreIdentifierParam,
            IRJumpParam
        >;

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

        void generate_program_or_block(const AstNode* node, ByteCode& byte_code, void* ctx);

        void generate_statement(const StatementNode* statement, ByteCode& byte_code, void* ctx);

        void generate_expression(const AstNode* expression, ByteCode& byte_code, void* ctx);

        void generate_binary_expression_statement(const BinaryExpressionNode* statement, ByteCode& byte_code, void* ctx);

        void generate_declaration_statement(const DeclarationStmtNode* statement, ByteCode& byte_code, void* ctx);

        void generate_assignment_statement(const AssignmentStmtNode* statement, ByteCode& byte_code, void* ctx);

        void generate_if_statement(const IfNode* statement, ByteCode& byte_code, void* ctx);

        void generate_while_statement(const WhileNode* statement, ByteCode& byte_code, void* ctx);

        void generate_conditional_comparision_bytecode(const AstNode* node, ByteCode& byte_code, void* ctx);

        void generate_break_statement(ByteCode& byte_code, WhileLoopGenerationContext* ctx);

        void generate_continue_statement(ByteCode& byte_code, WhileLoopGenerationContext* ctx);
    };

    class IRInterpreter { 
    public:
        IRInterpreter();
        explicit IRInterpreter(ByteCode byte_code) : byte_code(std::move(byte_code)) {};

        void set_byte_code(ByteCode byte_code) { this->byte_code = std::move(byte_code); };

        void run();
        
        IRPrimValue retrieve_value(const std::string& identifier);
        
        bool has_identifier(const std::string& identifier) const;

    private:
        ByteCode byte_code;
        size_t pc = 0;
        std::unordered_map<std::string, IRPrimValue> variables;
        std::stack<IRPrimValue> stack;
        IRPrimValue output;

        void handle_binary_op(IRInstruction::InstructionType op);

        bool handle_jump(IRInstruction::InstructionType op, IRJumpParam param);

        template <typename T>
        void handle_binary_op(IRInstruction::InstructionType op, T lhs, T rhs) {
            switch (op) {
                case IRInstruction::InstructionType::ADD: 
                    stack.push(lhs + rhs);
                    break;
                case IRInstruction::InstructionType::SUB: 
                    stack.push(lhs - rhs);
                    break;
                case IRInstruction::InstructionType::MUL: 
                    stack.push(lhs * rhs);
                    break;
                case IRInstruction::InstructionType::DIV: 
                    stack.push(lhs / rhs);
                    break;
                case IRInstruction::InstructionType::MOD:
                    if constexpr (std::is_integral_v<T>) {
                        stack.push(lhs % rhs);
                    } else {
                        stack.push(std::fmod(lhs, rhs));
                    }
                case IRInstruction::InstructionType::CMP:
                    if (lhs > rhs) {
                        stack.push(1);
                    } else if (lhs < rhs) {
                        stack.push(-1);
                    } else {
                        stack.push(0);
                    }
                    break;
                default:
                    throw IRInterpreterException("Invalid instruction type");
                    return;
            }
        }
    };
}