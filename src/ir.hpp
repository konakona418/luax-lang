#pragma once

#include <list>
#define LUAXC_RUNTIME_MAX_STACK_SIZE 1024
#define LUAXC_RUNTIME_STACK_OVERFLOW_PROTECTION_ENABLED

#include <cmath>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>


#include "ast.hpp"
#include "gc.hpp"
#include "parser.hpp"
#include "value.hpp"


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
        bool force_pop_return_value = false;
        // this is a hack for some dispatched assigment operations.
        // e.g. index member access expression.
        // by default, such assignment expression won't return a value;
        // however, in some cases, when this operation is dispatched to a user-defined function,
        // an object is returned. but this return value will not be properly discarded by default.
        // so we need to manually discard it.
    };

    struct IRMakeModuleParam {
        size_t module_id;
    };

    struct IRMakeFunctionParam {
        size_t begin_offset;
        size_t module_id;
        size_t arity;
        bool is_method;
        bool is_closure;
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

            MAKE_STRING,
            MAKE_FUNC,
            MAKE_RULE,

            MAKE_MODULE,      // make a module from an exported object
            MAKE_MODULE_LOCAL,// make a module from a local object combinator

            BEGIN_LOCAL,// force push a stack frame
            END_LOCAL,  // force pop

            BEGIN_LOCAL_DERIVED,// force push a stack frame, allow upward propagation

            LOAD_MEMBER, // pop object, store member onto stack. member name in param
            STORE_MEMBER,// pop value, pop object, store. member name in param

            LOAD_INDEXOF, // pop index, pop object, store member onto stack
            STORE_INDEXOF,// pop index, pop value, pop object. store

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
                IRMakeFunctionParam,
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

        void inject_module_compiler(std::function<std::unique_ptr<AstNode>(const std::string&, const std::string&)> fn) { this->fn_compile_module = fn; }

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

        std::function<std::unique_ptr<AstNode>(const std::string&, const std::string&)> fn_compile_module = nullptr;

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

        void generate_module_access_expression(const ExpressionNode* expression, ByteCode& byte_code);

        void generate_closure_expression(const ClosureExpressionNode* expression, ByteCode& byte_code);

        void generate_rule_expression(const RuleExpressionNode* expression, ByteCode& byte_code);

        void generate_numeric_literal(const NumericLiteralNode* statement, ByteCode& byte_code);

        void generate_string_literal(const StringLiteralNode* statement, ByteCode& byte_code);

        void generate_binary_expression_statement(const BinaryExpressionNode* statement, ByteCode& byte_code);

        void generate_combinative_assignment_statement(const BinaryExpressionNode* statement, ByteCode& byte_code);

        void generate_unary_expression_statement(const UnaryExpressionNode* statement, ByteCode& byte_code);

        void generate_boolean_literal(const BoolLiteralNode* statement, ByteCode& byte_code);

        void generate_null_literal(ByteCode& byte_code);

        void generate_declaration_statement(const DeclarationStmtNode* statement, ByteCode& byte_code);

        void generate_assignment_statement(const AssignmentExpressionNode* statement, ByteCode& byte_code);

        void generate_assignment_statement_identifier_lvalue(const AssignmentExpressionNode* statement, ByteCode& byte_code);

        void generate_assignment_statement_member_access_lvalue(const AssignmentExpressionNode* statement, ByteCode& byte_code);

        void generate_member_access_statement_rvalue(const MemberAccessExpressionNode* statement, ByteCode& byte_code);

        void generate_member_access(const ExpressionNode* expression, ByteCode& byte_code);

        void generate_implicit_receiver(ByteCode& byte_code);

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
        friend IRRuntime;

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

        std::vector<PrimValue>* get_op_stack_ptr() { return &stack; }

        std::list<StackFrame>* get_stack_frames_ptr() { return &stack_frames; };

        void set_program_counter(size_t pc) { this->pc = pc; }

        size_t get_program_counter() const { return pc; }

        bool running() const { return pc < byte_code.size(); }

    private:
        ByteCode byte_code;
        size_t pc = 0;
        std::list<StackFrame> stack_frames;
        std::vector<IRPrimValue> stack;
        std::vector<FrozenContextObject*> context_stack;
        IRRuntime& runtime;

        void preload_native_functions();

        PrimValue pop_op_stack() {
            auto value = stack.back();
            stack.pop_back();
            return value;
        }

        void push_op_stack(PrimValue value) { stack.push_back(value); }

        void peek_op_stack() { push_op_stack(op_stack_top()); }

        PrimValue& op_stack_top() { return stack.back(); }

        void push_stack_frame(bool allow_propagation = false, bool force_pop_return_value = false);

        void pop_stack_frame();

        void load_context(FrozenContextObject* ctx) { context_stack.push_back(ctx); }

        void restore_context() { context_stack.pop_back(); }

        bool has_context() { return !context_stack.empty(); }

        FrozenContextObject* get_context() { return context_stack.back(); }

        FrozenContextObject* freeze_context();

        StackFrame& current_stack_frame();

        StackFrame& global_stack_frame();

        std::optional<PrimValue> retrieve_identifier_in_stack_frame(StringObject* identifier);

        std::optional<PrimValue*> retrieve_identifier_ref_in_stack_frame(StringObject* identifier);

        bool has_identifier_in_global_scope(StringObject* identifier);

        std::optional<PrimValue> retrieve_value_in_stored_context(StringObject* identifier);

        IRPrimValue retrieve_raw_value_in_current_stack_frame(StringObject* identifier);

        IRPrimValue& retrieve_value_ref_in_current_stack_frame(StringObject* identifier);

        IRPrimValue retrieve_raw_value_in_global_scope(StringObject* identifier);

        IRPrimValue& retrieve_value_ref_in_global_scope(StringObject* identifier);

        void store_value_in_stack_frame(StringObject* identifier, IRPrimValue value);

        void store_value_in_global_scope(StringObject* identifier, IRPrimValue value);

        std::optional<std::array<IRPrimValue, 2>> is_binary_op_dynamically_dispatchable(IRPrimValue lhs, IRPrimValue rhs);

        bool dispatch_binary_op(IRPrimValue lhs, IRPrimValue rhs, const std::string& identifier);

        bool handle_binary_op(IRInstruction::InstructionType op);

        bool dispatch_unary_op(IRPrimValue value, const std::string& identifier);

        bool handle_unary_op(IRInstruction::InstructionType op);

        bool handle_jump(IRInstruction::InstructionType op, IRJumpParam param);

        bool handle_relative_jump(IRInstruction::InstructionType op, IRJumpRelParam param);

        void handle_type_creation();

        void handle_make_rule();

        bool handle_member_load(IRLoadMemberParam param);

        bool handle_member_store(IRStoreMemberParam param);

        bool handle_index_load();

        bool handle_index_store();

        void handle_module_load(IRLoadModuleParam param);

        void handle_make_string();

        void handle_make_function(IRMakeFunctionParam param);

        void handle_make_object(IRMakeObjectParam param);

        void handle_make_module(IRMakeModuleParam param);

        GCObject* handle_make_module_local();

        bool handle_static_method_invocation(const PrimValue& value, StringObject* name);

        void handle_to_bool();

        bool handle_function_invocation(IRCallParam param);

        void handle_return();

        bool handle_binary_op(IRInstruction::InstructionType op, IRPrimValue lhs, IRPrimValue rhs);

        bool handle_unary_op(IRInstruction::InstructionType op, IRPrimValue rhs);
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

            gc = std::move(other.gc);
            module_manager = std::move(other.module_manager);
            runtime_ctx = std::move(other.runtime_ctx);

            byte_code = std::move(other.byte_code);
        }

        ~IRRuntime() = default;

        std::unique_ptr<AstNode> generate(
                const std::string& input, const std::string& filename,
                Parser::ParserState init_state = Parser::ParserState::Start);

        void compile(const std::string& input);

        void run();

        void abort(const std::string& reason) { throw std::runtime_error("Runtime aborted: " + reason); }

        IRInterpreter& get_interpreter() {
            return *this->interpreter.get();
        }

        GarbageCollector::GCGuard gc_guard() {
            return this->gc.guard();
        }

        void gc_regist_no_collect(GCObject* object) {
            gc.regist_no_collect(object);
        }

        template<typename ObjectType, typename... Args, typename = std::enable_if_t<std::is_base_of_v<GCObject, ObjectType>>>
        ObjectType* gc_allocate(Args&&... args) {
            return gc.allocate<ObjectType>(std::forward<Args>(args)...);
        }

        void gc_collect() { gc.collect(); }

        void gc_regist(GCObject* object) { gc.regist(object); }

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

        void init_type_info(GCObject* object, const std::string& type_name);

        ImportedModule& get_module(size_t id) { return module_manager.modules[id]; };

        struct RuntimeContext {
            std::string import_path;
            std::string cwd;
        };

        RuntimeContext& get_runtime_context() { return runtime_ctx; }

        void set_gc_heap_size(size_t size) { gc.set_max_heap_size(size); }

        size_t get_gc_heap_size() const { return gc.get_max_heap_size(); }

        void invoke_function(FunctionObject* function, std::vector<PrimValue> args,
                             bool force_discard_return_value, ssize_t jump_offset = 0);

    private:
        struct {
            std::unordered_map<std::string, StringObject*> string_const_pool;
        } constant_pools;

        std::unique_ptr<IRGenerator> generator = nullptr;
        std::unique_ptr<IRInterpreter> interpreter = nullptr;

        std::unordered_map<std::string, TypeObject*> type_info;

        struct {
            std::unordered_map<size_t, ImportedModule> modules;
            size_t module_count = 0;
        } module_manager;

        RuntimeContext runtime_ctx;

        ByteCode byte_code;

        GarbageCollector gc;

        void resolve_runtime_ctx();
    };
}// namespace luaxc