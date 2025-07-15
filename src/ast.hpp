#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gc.hpp"

namespace luaxc {
    enum class AstNodeType {
        Program,
        Stmt,
        NumericLiteral,
        StringLiteral,
        Identifier,
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
            ForwardDeclarationStmt,// use
            AssignmentStmt,
            BinaryExprStmt,
            UnaryExprStmt,
            FuncInvokeStmt,
            BlockStmt,
            IfStmt,
            WhileStmt,
            ForStmt,
            BreakStmt,
            ContinueStmt,
        };

        explicit StatementNode(StatementType statement_type)
            : AstNode(AstNodeType::Stmt), statement_type(statement_type) {}

        StatementType get_statement_type() const { return statement_type; }

    private:
        StatementType statement_type;
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
    class AssignmentStmtNode : public StatementNode {
    public:
        explicit AssignmentStmtNode(std::unique_ptr<AstNode> identifier, std::unique_ptr<AstNode> value)
            : StatementNode(StatementType::AssignmentStmt), identifier(std::move(identifier)), value(std::move(value)) {}

        const std::unique_ptr<AstNode>& get_identifier() const { return identifier; }
        const std::unique_ptr<AstNode>& get_value() const { return value; }

    private:
        std::unique_ptr<AstNode> identifier;
        std::unique_ptr<AstNode> value;
    };

    // numeric -> numeric_literal
    class NumericLiteralNode : public AstNode {
    public:
        using NumericVariant = PrimValue;

        enum class NumericLiteralType {
            Integer,
            Float
        };

        NumericLiteralNode(NumericLiteralType type, std::string value) : AstNode(AstNodeType::NumericLiteral), type(type), string_value(value) {
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

    class StringLiteralNode : public AstNode {
    public:
        StringLiteralNode(std::string value) : AstNode(AstNodeType::StringLiteral), value(std::move(value)) {}

        const std::string& get_value() const { return value; }

    private:
        std::string value;
    };

    class IdentifierNode : public AstNode {
    public:
        explicit IdentifierNode(std::string name) : AstNode(AstNodeType::Identifier), name(std::move(name)) {}

        const std::string& get_name() const { return name; }

    private:
        std::string name;
    };

    // binary expr -> expr op expr
    class BinaryExpressionNode : public StatementNode {
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
            : StatementNode(StatementNode::StatementType::BinaryExprStmt), left(std::move(left)), right(std::move(right)), op(op) {}

        const std::unique_ptr<AstNode>& get_left() const { return left; }

        const std::unique_ptr<AstNode>& get_right() const { return right; }

        BinaryOperator get_op() const { return op; }

    private:
        std::unique_ptr<AstNode> left;
        std::unique_ptr<AstNode> right;
        BinaryOperator op;
    };

    // unary expr -> (op) expr
    class UnaryExpressionNode : public StatementNode {
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
            : StatementNode(StatementNode::StatementType::UnaryExprStmt), operand(std::move(operand)), op(op) {}

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
                std::unique_ptr<AstNode> body) : StatementNode(StatementType::ForStmt),
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

    class FunctionInvocationNode : public StatementNode {
    public:
        explicit FunctionInvocationNode(std::unique_ptr<AstNode> function_identifier)
            : StatementNode(StatementType::FuncInvokeStmt), function_identifier(std::move(function_identifier)) {}

        FunctionInvocationNode(std::unique_ptr<AstNode> function_identifier, std::vector<std::unique_ptr<AstNode>> arguments)
            : StatementNode(StatementType::FuncInvokeStmt), function_identifier(std::move(function_identifier)), arguments_expr(std::move(arguments)) {}

        const std::unique_ptr<AstNode>& get_function_identifier() const { return function_identifier; }

        const std::vector<std::unique_ptr<AstNode>>& get_arguments() const { return arguments_expr; }

    private:
        std::unique_ptr<AstNode> function_identifier;
        std::vector<std::unique_ptr<AstNode>> arguments_expr;
    };
}// namespace luaxc
