#pragma once

#include <cmath>
#include <optional>
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
        StringObject* identifier;
    };

    struct IRStoreIdentifierParam {
        StringObject* identifier;
    };

    struct IRDeclareIdentifierParam {
        StringObject* identifier;
    };

    struct IRLoadModuleParam {
        size_t module_id;
    };

    struct IRLoadMemberParam {
        StringObject* identifier;
    };

    struct IRMakeObjectParam {
        std::vector<StringObject*> fields;
    };

    struct IRStoreMemberParam {
        StringObject* identifier;
    };

    using IRJumpParam = size_t;
    using IRJumpRelParam = ssize_t;

    struct IRCallParam {
        size_t arguments_count;
    };

    struct IRMakeModuleParam {
        size_t module_id;
    };

    class IRInstruction {
    public:
        enum class InstructionType {
            LOAD_CONST,        // load a const to stack
            DECLARE_IDENTIFIER,// declare an identifier
            LOAD_IDENTIFIER,   // load an identifier to stack
            STORE_IDENTIFIER,  // store stack top to an identifier
            LOAD_MODULE,       // load a module to stack
            ADD,               // pop two values from stack, add them and push result to stack
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

            JMP_REL,         // relative jump
            JMP_IF_FALSE_REL,// relative jump with condition

            POP_STACK,// pop value from stack
            PEEK,     // duplicate stack top

            MAKE_TYPE,// make a type using stack frame local vars

            MAKE_OBJECT,// make an object. pop a sequence of values from stack

            MAKE_MODULE,      // make a module from an exported object
            MAKE_MODULE_LOCAL,// make a module from a local object combinator

            BEGIN_LOCAL,// force push a stack frame
            END_LOCAL,  // force pop

            BEGIN_LOCAL_DERIVED,// force push a stack frame, allow upward propagation

            LOAD_MEMBER, // pop object, store member onto stack. member name in param
            STORE_MEMBER,// pop value, pop object, store. member name in param

            CALL,
            RET,
        };

        using IRParam = std::variant<
                std::monostate,
                IRLoadConstParam,
                IRDeclareIdentifierParam,
                IRLoadIdentifierParam,
                IRLoadModuleParam,
                IRStoreIdentifierParam,
                IRJumpParam,
                IRJumpRelParam,
                IRCallParam,
                IRLoadMemberParam,
                IRStoreMemberParam,
                IRMakeObjectParam,
                IRMakeModuleParam>;

        IRParam param;
        InstructionType type;

        IRInstruction(InstructionType type, IRParam param) : param(param), type(type) {}

        std::string dump() const;
    };

    using ByteCode = std::vector<IRInstruction>;

    std::string dump_bytecode(const ByteCode& bytecode);

    class IRRuntime;

    class IRGenerator {
    public:
        IRGenerator(IRRuntime& runtime, std::unique_ptr<AstNode> ast_base_node)
            : runtime(runtime), ast(std::move(ast_base_node)) {}

        ByteCode generate();

        void inject_compiler(std::function<std::unique_ptr<AstNode>(const std::string&)> fn) { this->fn_compile_module = fn; }

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

        IRRuntime& runtime;

        std::stack<size_t> compiling_module_ids;

        std::vector<size_t> compiling_module_base_offsets;

        std::function<std::unique_ptr<AstNode>(const std::string&)> fn_compile_module = nullptr;

        std::optional<size_t> is_module_present(const std::string& module_name);

        size_t aggregate_compiling_module_base_offsets();

        StringObject* begin_module_compilation(const std::string& module_name, size_t base_offset);

        size_t end_module_compilation();

        size_t get_current_compiling_module_id();

        std::string read_module_file(const std::string& module_file_path);

        bool is_binary_logical_operator(BinaryExpressionNode::BinaryOperator op);

        bool is_combinative_assignment_operator(BinaryExpressionNode::BinaryOperator op);

        void generate_program_or_block(const AstNode* node, ByteCode& byte_code);

        void generate_statement(const StatementNode* statement, ByteCode& byte_code);

        void generate_expression(const ExpressionNode* expression, ByteCode& byte_code);

        void generate_type_decl_expression(const TypeDeclarationExpressionNode* expression, ByteCode& byte_code);

        void generate_module_decl_expression(const ModuleDeclarationExpressionNode* expression, ByteCode& byte_code);

        void generate_module_import_expression(const ModuleImportExpresionNode* expression, ByteCode& byte_code);

        void generate_numeric_literal(const NumericLiteralNode* statement, ByteCode& byte_code);

        void generate_string_literal(const StringLiteralNode* statement, ByteCode& byte_code);

        void generate_binary_expression_statement(const BinaryExpressionNode* statement, ByteCode& byte_code);

        void generate_combinative_assignment_statement(const BinaryExpressionNode* statement, ByteCode& byte_code);

        void generate_unary_expression_statement(const UnaryExpressionNode* statement, ByteCode& byte_code);

        void generate_declaration_statement(const DeclarationStmtNode* statement, ByteCode& byte_code);

        void generate_assignment_statement(const AssignmentExpressionNode* statement, ByteCode& byte_code);

        void generate_assignment_statement_identifier_lvalue(const AssignmentExpressionNode* statement, ByteCode& byte_code);

        void generate_assignment_statement_member_access_lvalue(const AssignmentExpressionNode* statement, ByteCode& byte_code);

        void generate_member_access_statement_rvalue(const MemberAccessExpressionNode* statement, ByteCode& byte_code);

        void generate_member_access(const ExpressionNode* expression, ByteCode& byte_code);

        void generate_initializer_list_expression(const InitializerListExpressionNode* expression, ByteCode& byte_code);

        void generate_if_statement(const IfNode* statement, ByteCode& byte_code);

        void generate_while_statement(const WhileNode* statement, ByteCode& byte_code);

        void generate_for_statement(const ForNode* statement, ByteCode& byte_code);

        void generate_break_statement(ByteCode& byte_code);

        void generate_continue_statement(ByteCode& byte_code);

        void generate_function_invocation_statement(
                const FunctionInvocationExpressionNode* statement,
                ByteCode& byte_code);

        void generate_function_declaration_statement(
                const FunctionDeclarationNode* statement,
                ByteCode& byte_code);

        void generate_method_declaration_statement(const MethodDeclarationNode* statement, ByteCode& byte_code);

        void generate_return_statement(const ReturnNode* statement, ByteCode& byte_code);
    };

    class IRInterpreter {
    public:
        explicit IRInterpreter(IRRuntime& runtime);
        ~IRInterpreter();

        IRInterpreter(IRRuntime& runtime, ByteCode byte_code) : IRInterpreter(runtime) {
            this->byte_code = std::move(byte_code);
        };

        void set_byte_code(ByteCode byte_code) { this->byte_code = std::move(byte_code); };

        void run();

        void declare_identifier(StringObject* identifier);

        IRPrimValue retrieve_raw_value(StringObject* identifier);

        IRPrimValue retrieve_raw_value(const std::string& identifier);

        IRPrimValue& retrieve_raw_value_ref(StringObject* identifier);

        IRPrimValue& retrieve_raw_value_ref(const std::string& identifier);

        void store_raw_value(StringObject* identifier, IRPrimValue value);

        template<typename T>
        T retrieve_value(StringObject* identifier) {
            return retrieve_raw_value(identifier).get_inner_value<T>();
        }

        template<typename T>
        T retrieve_value(const std::string& identifier) {
            return retrieve_raw_value(identifier).get_inner_value<T>();
        }

        bool has_identifier(StringObject* identifier);

        bool has_identifier(const std::string& identifier);

    private:
        struct StackFrame {
            std::unordered_map<StringObject*, IRPrimValue> variables;
            size_t return_addr;
            bool allow_upward_propagation = false;

            explicit StackFrame(size_t return_addr) : return_addr(return_addr) {};
            StackFrame(size_t return_addr, bool allow_propagation)
                : return_addr(return_addr),
                  allow_upward_propagation(allow_propagation) {};
        };

        ByteCode byte_code;
        size_t pc = 0;
        std::vector<StackFrame> stack_frames;
        std::stack<IRPrimValue> stack;
        IRRuntime& runtime;

        void preload_native_functions();

        void push_stack_frame(bool allow_propagation = false);

        void pop_stack_frame();

        StackFrame& current_stack_frame();

        StackFrame& global_stack_frame();

        std::optional<size_t> has_identifier_in_stack_frame(StringObject* identifier);

        bool has_identifier_in_global_scope(StringObject* identifier);

        IRPrimValue retrieve_raw_value_in_desired_stack_frame(StringObject* identifier, size_t idx);

        IRPrimValue& retrieve_value_ref_in_desired_stack_frame(StringObject* identifier, size_t idx);

        IRPrimValue retrieve_raw_value_in_current_stack_frame(StringObject* identifier);

        IRPrimValue& retrieve_value_ref_in_current_stack_frame(StringObject* identifier);

        IRPrimValue retrieve_raw_value_in_global_scope(StringObject* identifier);

        IRPrimValue& retrieve_value_ref_in_global_scope(StringObject* identifier);

        void store_value_in_stack_frame(StringObject* identifier, IRPrimValue value);

        void store_value_in_global_scope(StringObject* identifier, IRPrimValue value);

        void handle_binary_op(IRInstruction::InstructionType op);

        void handle_unary_op(IRInstruction::InstructionType op);

        bool handle_jump(IRInstruction::InstructionType op, IRJumpParam param);

        bool handle_relative_jump(IRInstruction::InstructionType op, IRJumpRelParam param);

        void handle_type_creation();

        void handle_member_load(IRLoadMemberParam param);

        void handle_member_store(IRStoreMemberParam param);

        void handle_module_load(IRLoadModuleParam param);

        void handle_make_object(IRMakeObjectParam param);

        void handle_make_module(IRMakeModuleParam param);

        GCObject* handle_make_module_local();

        void handle_to_bool();

        bool handle_function_invocation(IRCallParam param);

        void handle_binary_op(IRInstruction::InstructionType op, IRPrimValue lhs, IRPrimValue rhs);

        void handle_unary_op(IRInstruction::InstructionType op, IRPrimValue rhs);
    };

    class IRRuntime {
    public:
        IRRuntime() {
            init_builtin_type_info();
            resolve_runtime_ctx();
        }

        IRRuntime(IRRuntime& other) = delete;
        IRRuntime(IRRuntime&& other) {
            constant_pools = std::move(other.constant_pools);

            generator = std::move(other.generator);
            interpreter = std::move(other.interpreter);

            type_info = std::move(other.type_info);

            objects = std::move(other.objects);
            module_manager = std::move(other.module_manager);
            runtime_ctx = std::move(other.runtime_ctx);

            byte_code = std::move(other.byte_code);
        }

        ~IRRuntime() {
            for (auto* object: objects) {
                delete object;
            }
        }

        std::unique_ptr<AstNode> generate(const std::string& input);

        void compile(const std::string& input);

        void run() {
            if (byte_code.size() == 0) {
                throw std::runtime_error("Not compiled");
            }

            interpreter->set_byte_code(byte_code);
            interpreter->run();
        }

        IRInterpreter& get_interpreter() {
            return *this->interpreter.get();
        }

        void push_gc_object(GCObject* object) {
            objects.push_back(object);
        }

        StringObject* push_string_pool_if_not_exists(const std::string& str);

        const ByteCode& get_byte_code() const { return byte_code; }

        void init_builtin_type_info();

        TypeObject* get_type_info(const std::string& name) { return type_info[name]; }

        bool has_type_info(const std::string& name) { return type_info.find(name) != type_info.end(); }

        void push_string_to_pool(const std::string& name, StringObject* obj) {
            constant_pools.string_const_pool.emplace(name, obj);
        }

        StringObject* get_string_from_pool(const std::string& name) {
            return constant_pools.string_const_pool.at(name);
        }

        bool is_string_in_pool(const std::string& name) {
            return constant_pools.string_const_pool.find(name) != constant_pools.string_const_pool.end();
        }

        template<typename T>
        T retrieve_value(const std::string& identifier) {
            return interpreter->retrieve_raw_value(identifier).get_inner_value<T>();
        }

        bool has_identifier(const std::string& identifier) { return interpreter->has_identifier(identifier); }

        std::string find_file_and_read(const std::string& module_path);

        struct ImportedModule {
            StringObject* name;

            size_t id;
            size_t base_offset;

            GCObject* module;
        };

        size_t get_next_module_id() { return module_manager.module_count; }

        size_t add_module(StringObject* module_name, size_t base_offset);

        size_t resolve_function_offset(size_t module_id, size_t function_offset);

        std::optional<size_t> has_module(StringObject* module_name);

        ImportedModule& get_module(size_t id) { return module_manager.modules[id]; };

        struct RuntimeContext {
            std::string import_path;
            std::string cwd;
        };

        const RuntimeContext& get_runtime_context() const { return runtime_ctx; }

    private:
        struct {
            std::unordered_map<std::string, StringObject*> string_const_pool;
        } constant_pools;

        std::unique_ptr<IRGenerator> generator = nullptr;
        std::unique_ptr<IRInterpreter> interpreter = nullptr;

        std::unordered_map<std::string, TypeObject*> type_info;

        std::vector<GCObject*> objects;
        struct {
            std::unordered_map<size_t, ImportedModule> modules;
            size_t module_count = 0;
        } module_manager;

        RuntimeContext runtime_ctx;

        ByteCode byte_code;

        void resolve_runtime_ctx();
    };
}// namespace luaxc