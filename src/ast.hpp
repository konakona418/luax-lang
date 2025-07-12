#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace luaxc {
    enum class AstNodeType {
        Program,
        Stmt,
        Declaration,
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

    class ProgramNode : public AstNode {
    public:
        ProgramNode() : AstNode(AstNodeType::Program) {}

        void add_statement(std::unique_ptr<AstNode> statement);

        const std::vector<std::unique_ptr<AstNode>>& get_statements() const { return statements; }

    private:
        std::vector<std::unique_ptr<AstNode>> statements;
    };

    class StatementNode : public AstNode {
    public:
        enum class StatementType {
            DeclarationStmt,
            AssignmentStmt,
            BinaryExprStmt,
            UnaryExprStmt,
        };

        explicit StatementNode(StatementType statement_type) 
            : AstNode(AstNodeType::Stmt), statement_type(statement_type) {}
        
        StatementType get_type() const { return statement_type; }

    private:
        StatementType statement_type;
    };

    class DeclarationStmtNode : public StatementNode { 
    public:
        explicit DeclarationStmtNode(std::unique_ptr<AstNode> identifier, std::unique_ptr<AstNode> value) 
            : StatementNode(StatementType::DeclarationStmt), identifier(std::move(identifier)), value_or_initializer(std::move(value)) {}

        const std::unique_ptr<AstNode>& get_identifier() const { return identifier; }

        const AstNode* get_value_or_initializer() const { return value_or_initializer.get(); }

    private:
        std::unique_ptr<AstNode> identifier;
        std::unique_ptr<AstNode> value_or_initializer;
    };

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

    class NumericLiteralNode : public AstNode { 
    public:
        using NumericVariant = std::variant<int32_t, double>;

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
    private:
        UnaryOperator op;
        std::unique_ptr<AstNode> operand;
    };
    
    class IdentifierNode : public AstNode {
    public:
        explicit IdentifierNode(std::string name) : AstNode(AstNodeType::Identifier), name(std::move(name)) {}

        const std::string& get_name() const { return name; }
    private:
        std::string name;
    };
}
