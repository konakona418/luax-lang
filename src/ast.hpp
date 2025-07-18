#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gc.hpp"

namespace luaxc {
    enum class AstNodeType {
        Program,
        Stmt,
    };

    class AstNode {
    public:
        virtual ~AstNode() = default;

        AstNode(AstNodeType type) : type(type) {}

        AstNodeType get_type() const { return type; }


    private:
        AstNodeType type;
    };

    // program -> stmt*
    class ProgramNode : public AstNode {
    public:
        ProgramNode() : AstNode(AstNodeType::Program) {}

        void add_statement(std::unique_ptr<AstNode> statement);

        const std::vector<std::unique_ptr<AstNode>>& get_statements() const { return statements; }

    private:
        std::vector<std::unique_ptr<AstNode>> statements;
    };

    // stmt -> expr | decl | assign | ...
    class StatementNode : public AstNode {
    public:
        enum class StatementType {
            DeclarationStmt,
            FieldDeclarationStmt,
            MethodDeclarationStmt,
            ForwardDeclarationStmt,// use
            ExpressionStmt,
            BlockStmt,
            IfStmt,
            WhileStmt,
            ForStmt,
            BreakStmt,
            ContinueStmt,
            FunctionDeclarationStmt,
            ReturnStmt,
        };

        explicit StatementNode(StatementType statement_type)
            : AstNode(AstNodeType::Stmt), statement_type(statement_type) {}

        StatementType get_statement_type() const { return statement_type; }

    private:
        StatementType statement_type;
    };

    class ExpressionNode : public StatementNode {
    public:
        enum class ExpressionType {
            Identifier,
            NumericLiteral,
            StringLiteral,
            TypeDecl,
            AssignmentExpr,
            BinaryExpr,
            UnaryExpr,
            FuncInvokeExpr,
            MethodInvokeExpr,
            MemberAccessExpr,
            InitializerListExpr,
        };

        ExpressionNode(ExpressionType expression_type)
            : StatementNode(StatementType::ExpressionStmt),
              expression_type(expression_type) {}

        ExpressionType get_expression_type() const { return expression_type; }

        bool is_result_discarded() const { return result_discarded; }

        void set_result_discarded(bool result_discarded) { this->result_discarded = result_discarded; }

    private:
        ExpressionType expression_type;
        bool result_discarded = false;
    };

    // decl -> ('let' identifier '=' expr ';') | ('let' identifier ';')
    class DeclarationStmtNode : public StatementNode {
    public:
        DeclarationStmtNode(std::vector<std::unique_ptr<AstNode>>&& identifiers, std::unique_ptr<AstNode> value)
            : StatementNode(StatementType::DeclarationStmt), identifiers(std::move(identifiers)), value_or_initializer(std::move(value)) {}

        const std::vector<std::unique_ptr<AstNode>>& get_identifiers() const { return identifiers; }

        const AstNode* get_value_or_initializer() const { return value_or_initializer.get(); }

    private:
        std::vector<std::unique_ptr<AstNode>> identifiers;
        std::unique_ptr<AstNode> value_or_initializer;
    };

    class ForwardDeclarationStmtNode : public StatementNode {
    public:
        explicit ForwardDeclarationStmtNode(std::unique_ptr<AstNode> identifier)
            : StatementNode(StatementNode::StatementType::ForwardDeclarationStmt),
              identifier(std::move(identifier)) {}

        const AstNode* get_identifier() const { return identifier.get(); }

    private:
        std::unique_ptr<AstNode> identifier;
    };

    // assign -> identifier = expr ';'
    class AssignmentExpressionNode : public ExpressionNode {
    public:
        explicit AssignmentExpressionNode(std::unique_ptr<AstNode> identifier, std::unique_ptr<AstNode> value)
            : ExpressionNode(ExpressionType::AssignmentExpr),
              identifier(std::move(identifier)), value(std::move(value)) {}

        const std::unique_ptr<AstNode>& get_identifier() const { return identifier; }
        const std::unique_ptr<AstNode>& get_value() const { return value; }

    private:
        std::unique_ptr<AstNode> identifier;
        std::unique_ptr<AstNode> value;
    };

    // numeric -> numeric_literal
    class NumericLiteralNode : public ExpressionNode {
    public:
        using NumericVariant = PrimValue;

        enum class NumericLiteralType {
            Integer,
            Float
        };

        NumericLiteralNode(NumericLiteralType type, std::string value)
            : ExpressionNode(ExpressionType::NumericLiteral),
              type(type), string_value(value) {
            this->value = parse_numeric_literal(value, type);
        }

        NumericLiteralType get_type() const { return type; }

        NumericVariant get_value() const { return value; }

        const std::string& get_string_value() const { return string_value; }

    private:
        NumericLiteralType type;
        NumericVariant value;
        std::string string_value;

        static NumericVariant parse_numeric_literal(const std::string& value, NumericLiteralType type);
    };

    class StringLiteralNode : public ExpressionNode {
    public:
        StringLiteralNode(const std::string& value)
            : ExpressionNode(ExpressionType::StringLiteral),
              value(value) {}

        const std::string& get_value() const { return value; }

    private:
        std::string value;
    };

    class IdentifierNode : public ExpressionNode {
    public:
        explicit IdentifierNode(std::string name)
            : ExpressionNode(ExpressionType::Identifier),
              name(std::move(name)) {}

        const std::string& get_name() const { return name; }

    private:
        std::string name;
    };

    // binary expr -> expr op expr
    class BinaryExpressionNode : public ExpressionNode {
    public:
        enum class BinaryOperator {
            Invalid,
            Add,
            Subtract,
            Multiply,
            Divide,
            Modulo,
            IncrementBy,
            DecrementBy,
            BitwiseAnd,
            BitwiseOr,
            BitwiseXor,
            BitwiseShiftLeft,
            BitwiseShiftRight,
            Equal,
            NotEqual,
            LessThan,
            LessThanEqual,
            GreaterThan,
            GreaterThanEqual,
            LogicalAnd,
            LogicalOr,
        };

        BinaryExpressionNode(std::unique_ptr<AstNode> left, std::unique_ptr<AstNode> right, BinaryOperator op)
            : ExpressionNode(ExpressionType::BinaryExpr), left(std::move(left)), right(std::move(right)), op(op) {}

        const std::unique_ptr<AstNode>& get_left() const { return left; }

        const std::unique_ptr<AstNode>& get_right() const { return right; }

        BinaryOperator get_op() const { return op; }

    private:
        std::unique_ptr<AstNode> left;
        std::unique_ptr<AstNode> right;
        BinaryOperator op;
    };

    // unary expr -> (op) expr
    class UnaryExpressionNode : public ExpressionNode {
    public:
        enum class UnaryOperator {
            BitwiseNot,
            LogicalNot,
            Minus,
            Plus,
            Decrement,
            Increment,
        };

        UnaryExpressionNode(std::unique_ptr<AstNode> operand, UnaryOperator op)
            : ExpressionNode(ExpressionType::UnaryExpr), operand(std::move(operand)), op(op) {}

        UnaryOperator get_operator() const { return op; }

        const std::unique_ptr<AstNode>& get_operand() const { return operand; }

    private:
        UnaryOperator op;
        std::unique_ptr<AstNode> operand;
    };

    // block -> '{' stmt* '}'
    class BlockNode : public StatementNode {
    public:
        explicit BlockNode(std::vector<std::unique_ptr<AstNode>> statements)
            : StatementNode(StatementNode::StatementType::BlockStmt), statements(std::move(statements)) {}

        const std::vector<std::unique_ptr<AstNode>>& get_statements() const { return statements; }

    private:
        std::vector<std::unique_ptr<AstNode>> statements;
    };

    // if -> 'if' '(' expr ')' (block | stmt) (('elif' '(' expr ')' (block | stmt))*)? ('else' (block | stmt))?
    class IfNode : public StatementNode {
    public:
        explicit IfNode(std::unique_ptr<AstNode> condition, std::unique_ptr<AstNode> body, std::unique_ptr<AstNode> else_body)
            : StatementNode(StatementNode::StatementType::IfStmt), condition(std::move(condition)), body(std::move(body)), else_body(std::move(else_body)) {}

        const std::unique_ptr<AstNode>& get_condition() const { return condition; }

        const std::unique_ptr<AstNode>& get_body() const { return body; }

        const std::unique_ptr<AstNode>& get_else_body() const { return else_body; }

    private:
        std::unique_ptr<AstNode> condition;
        std::unique_ptr<AstNode> body;
        std::unique_ptr<AstNode> else_body;
    };

    class WhileNode : public StatementNode {
    public:
        explicit WhileNode(std::unique_ptr<AstNode> condition, std::unique_ptr<AstNode> body)
            : StatementNode(StatementType::WhileStmt), condition(std::move(condition)), body(std::move(body)) {};

        const std::unique_ptr<AstNode>& get_condition() const { return condition; }

        const std::unique_ptr<AstNode>& get_body() const { return body; }

    private:
        std::unique_ptr<AstNode> condition;
        std::unique_ptr<AstNode> body;
    };

    class BreakNode : public StatementNode {
    public:
        BreakNode() : StatementNode(StatementType::BreakStmt) {}
    };

    class ContinueNode : public StatementNode {
    public:
        ContinueNode() : StatementNode(StatementType::ContinueStmt) {}
    };

    class ForNode : public StatementNode {
    public:
        ForNode(
                std::unique_ptr<AstNode> init_stmt,
                std::unique_ptr<AstNode> condition,
                std::unique_ptr<AstNode> update,
                std::unique_ptr<AstNode> body)
            : StatementNode(StatementType::ForStmt),
              init_stmt(std::move(init_stmt)),
              condition_expr(std::move(condition)),
              update_stmt(std::move(update)),
              body(std::move(body)) {}

        const std::unique_ptr<AstNode>& get_init_stmt() const { return init_stmt; }

        const std::unique_ptr<AstNode>& get_condition_expr() const { return condition_expr; }

        const std::unique_ptr<AstNode>& get_update_stmt() const { return update_stmt; }

        const std::unique_ptr<AstNode>& get_body() const { return body; }

    private:
        std::unique_ptr<AstNode> init_stmt;
        std::unique_ptr<AstNode> condition_expr;
        std::unique_ptr<AstNode> update_stmt;

        std::unique_ptr<AstNode> body;
    };

    class RangeBasedForNode : public StatementNode {
        // todo
    };

    class FunctionDeclarationNode : public StatementNode {
    public:
        FunctionDeclarationNode(
                std::unique_ptr<AstNode> identifier,
                std::vector<std::unique_ptr<AstNode>> parameters,
                std::unique_ptr<AstNode> function_body)
            : StatementNode(StatementType::FunctionDeclarationStmt),
              identifier(std::move(identifier)), parameters(std::move(parameters)),
              body(std::move(function_body)) {}

        const std::unique_ptr<AstNode>& get_identifier() const { return identifier; }

        const std::vector<std::unique_ptr<AstNode>>& get_parameters() const { return parameters; }

        const std::unique_ptr<AstNode>& get_function_body() const { return body; }

    private:
        std::unique_ptr<AstNode> identifier;
        std::vector<std::unique_ptr<AstNode>> parameters;
        std::unique_ptr<AstNode> body;
    };

    class ReturnNode : public StatementNode {
    public:
        explicit ReturnNode(std::unique_ptr<AstNode> expression)
            : StatementNode(StatementType::ReturnStmt),
              expression(std::move(expression)) {}

        const std::unique_ptr<AstNode>& get_expression() const { return expression; }

    private:
        std::unique_ptr<AstNode> expression;
    };

    class FunctionInvocationExpressionNode : public ExpressionNode {
    public:
        explicit FunctionInvocationExpressionNode(std::unique_ptr<AstNode> function_identifier)
            : ExpressionNode(ExpressionType::FuncInvokeExpr), function_identifier(std::move(function_identifier)) {}

        FunctionInvocationExpressionNode(std::unique_ptr<AstNode> function_identifier, std::vector<std::unique_ptr<AstNode>> arguments)
            : ExpressionNode(ExpressionType::FuncInvokeExpr), function_identifier(std::move(function_identifier)), arguments_expr(std::move(arguments)) {}

        const std::unique_ptr<AstNode>& get_function_identifier() const { return function_identifier; }

        const std::vector<std::unique_ptr<AstNode>>& get_arguments() const { return arguments_expr; }

    private:
        std::unique_ptr<AstNode> function_identifier;
        std::vector<std::unique_ptr<AstNode>> arguments_expr;
    };

    class MethodInvocationExpressionNode : public ExpressionNode {
    public:
        explicit MethodInvocationExpressionNode(std::unique_ptr<AstNode> initial, std::unique_ptr<AstNode> method_identifier)
            : ExpressionNode(ExpressionType::MethodInvokeExpr),
              initial_expr(std::move(initial)),
              method_identifier(std::move(method_identifier)) {}

        MethodInvocationExpressionNode(std::unique_ptr<AstNode> initial, std::unique_ptr<AstNode> method_identifier, std::vector<std::unique_ptr<AstNode>> arguments)
            : ExpressionNode(ExpressionType::MethodInvokeExpr),
              initial_expr(std::move(initial)),
              method_identifier(std::move(method_identifier)),
              arguments_expr(std::move(arguments)) {}

        const std::unique_ptr<AstNode>& get_method_identifier() const { return method_identifier; }

        const std::vector<std::unique_ptr<AstNode>>& get_arguments() const { return arguments_expr; }

        const std::unique_ptr<AstNode>& get_initial_expr() const { return initial_expr; }

    private:
        std::unique_ptr<AstNode> initial_expr;
        std::unique_ptr<AstNode> method_identifier;
        std::vector<std::unique_ptr<AstNode>> arguments_expr;
    };

    class TypeDeclarationExpressionNode : public ExpressionNode {
    public:
        explicit TypeDeclarationExpressionNode(std::unique_ptr<AstNode> type_statements_block)
            : ExpressionNode(ExpressionType::TypeDecl),
              type_statements_block(std::move(type_statements_block)) {};

        const std::unique_ptr<AstNode>& get_type_statements_block() const { return type_statements_block; }

    private:
        std::unique_ptr<AstNode> type_statements_block;
    };

    class MemberAccessExpressionNode : public ExpressionNode {
    public:
        explicit MemberAccessExpressionNode(std::unique_ptr<AstNode> object, std::unique_ptr<AstNode> member)
            : ExpressionNode(ExpressionType::MemberAccessExpr),
              object_expr(std::move(object)), member_identifier(std::move(member)) {}

        const std::unique_ptr<AstNode>& get_object_expr() const { return object_expr; }

        const std::unique_ptr<AstNode>& get_member_identifier() const { return member_identifier; }

    private:
        std::unique_ptr<AstNode> object_expr;
        std::unique_ptr<AstNode> member_identifier;
    };

    class FieldDeclarationStatementNode : public StatementNode {
    public:
        explicit FieldDeclarationStatementNode(std::unique_ptr<AstNode> identifier)
            : StatementNode(StatementType::FieldDeclarationStmt),
              field_identifier(std::move(identifier)) {}

        FieldDeclarationStatementNode(std::unique_ptr<AstNode> identifier, std::unique_ptr<AstNode> type_decl)
            : StatementNode(StatementType::FieldDeclarationStmt), field_identifier(std::move(identifier)),
              type_declaration_expr(std::move(type_decl)) {}

        const std::unique_ptr<AstNode>& get_field_identifier() const { return field_identifier; }

        const std::unique_ptr<AstNode>& get_type_declaration_expr() const { return type_declaration_expr; }

    private:
        std::unique_ptr<AstNode> field_identifier;
        std::unique_ptr<AstNode> type_declaration_expr;// nullable
    };

    class MethodDeclarationNode : public StatementNode {
    public:
        MethodDeclarationNode(
                std::unique_ptr<AstNode> identifier,
                std::vector<std::unique_ptr<AstNode>> parameters,
                std::unique_ptr<AstNode> function_body)
            : StatementNode(StatementType::MethodDeclarationStmt),
              identifier(std::move(identifier)), parameters(std::move(parameters)),
              body(std::move(function_body)) {}

        const std::unique_ptr<AstNode>& get_identifier() const { return identifier; }

        const std::vector<std::unique_ptr<AstNode>>& get_parameters() const { return parameters; }

        const std::unique_ptr<AstNode>& get_function_body() const { return body; }

    private:
        std::unique_ptr<AstNode> identifier;
        std::vector<std::unique_ptr<AstNode>> parameters;
        std::unique_ptr<AstNode> body;
    };

    class InitializerListExpressionNode : public ExpressionNode {
    public:
        InitializerListExpressionNode(std::unique_ptr<AstNode> type, std::unique_ptr<AstNode> initializers)
            : ExpressionNode(ExpressionType::InitializerListExpr),
              type_expr(std::move(type)), initializer_list_block(std::move(initializers)) {}

        const std::unique_ptr<AstNode>& get_type_expr() const { return type_expr; }

        const std::unique_ptr<AstNode>& get_initializer_list_block() const { return initializer_list_block; }

    private:
        std::unique_ptr<AstNode> type_expr;
        std::unique_ptr<AstNode> initializer_list_block;
    };

}// namespace luaxc
