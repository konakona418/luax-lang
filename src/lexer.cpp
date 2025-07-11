#include "lexer.hpp"
#include "test_helper.hpp"

#define LUAXC_LEXER_IS_WHITESPACE(x) std::isspace(x)
#define LUAXC_LEXER_PERHAPS_COMMENT(_cur, _peek) ((_cur == '/' && _peek == '/') || (_cur == '/' && _peek == '*'))
#define LUAXC_LEXER_IS_DIGIT(x) (x >= '0' && x <= '9')
#define LUAXC_LEXER_PERHAPS_IDENTIFIER(x) (x >= 'a' && x <= 'z' || x >= 'A' && x <= 'Z' || x == '_')
#define LUAXC_LEXER_IS_IDENTIFIER(x) (x >= 'a' && x <= 'z' || x >= 'A' && x <= 'Z' || x == '_' || x >= '0' && x <= '9')
#define LUAXC_LEXER_PERHAPS_STRING_LITERAL(x) (x == '"' || x == '\'')
#define LUAXC_LEXER_IS_VALID_ESCAPE_SEQUENCE(_peek) (_peek == '\\' || _peek == '0' || _peek == 'n' || _peek == 't' || _peek == 'r' || _peek == '"' || _peek == '\'')

namespace luaxc {

    void Lexer::set_statistics_next_line() { 
        statistics.column = 1;
        statistics.line++;
    }

    void Lexer::advance() { 
        pos++;
        statistics.column++;
    }

    void Lexer::skip_whitespace() { 
        test_helper::dbg_print("Skipping whitespace");

        while (LUAXC_LEXER_IS_WHITESPACE(current_char())) {
            if (current_char() == '\n') {
                set_statistics_next_line();
            }
            advance();
        }
    }

    void Lexer::skip_comment() { 
        test_helper::dbg_print("Skip comment");

        if (current_char() == '/' && peek() == '/') { 
            while (current_char() != '\n' && !is_eof()) {
                advance();
            }
            if (current_char() == '\n') {
                advance();
                set_statistics_next_line();
            }
        } else if (current_char() == '/' && peek() == '*') {
            advance();
            advance();
            while (!(current_char() == '*' && peek() == '/')) {
                if (is_eof()) {
                    throw error::LexerError("Unterminated comment", statistics.line, statistics.column);
                }
                advance();
            }

            if (current_char() == '\n') {
                set_statistics_next_line();
            }
            advance();
            advance();
        }

        return;
    }

    Token Lexer::get_digit() { 
        test_helper::dbg_print("Getting digit");

        auto begin_offset = pos;

        while (LUAXC_LEXER_IS_DIGIT(current_char())) {
            advance();

            if (is_eof()) {
                break;
            }
        }

        if (is_eof()) {
            return Token{TokenType::NUMBER, input.substr(begin_offset, pos - begin_offset)};
        }

        bool is_floating_point = false;

        if (current_char() == '.') {
            is_floating_point = true;
            advance();
            while (LUAXC_LEXER_IS_DIGIT(current_char())) {
                advance();
            }

        } else if (current_char() == 'e' || current_char() == 'E') {
            advance();
            if (current_char() == '+' || current_char() == '-') {
                advance();
            } else {
                throw error::LexerError("Expected + or - after exponent", 
                    statistics.line, statistics.column);
            }

            while (LUAXC_LEXER_IS_DIGIT(current_char())) {
                advance();
            }
        } else if (current_char() == 'f') {
            advance();
        } else if (current_char() == 'u' || current_char() == 'i') {
            if (is_floating_point) {
                throw error::LexerError("Floating point number cannot have u or i suffix", 
                    statistics.line, pos - begin_offset);
            }
            advance();

            while (LUAXC_LEXER_IS_DIGIT(current_char())) {
                advance();
            }
        }

        return Token{TokenType::NUMBER, input.substr(begin_offset, pos - begin_offset)};
    }

    Token Lexer::get_identifier() { 
        test_helper::dbg_print("Get identifier");

        auto begin_offset = pos;

        while (LUAXC_LEXER_IS_IDENTIFIER(current_char())) {
            advance();
        }

        return Token{TokenType::IDENTIFIER, input.substr(begin_offset, pos - begin_offset)};
    }

    Token Lexer::get_string_literal() { 
        // the return value contains the quotes enclosing the string
        // and, the escape chars will be converted to their respective values,
        // if possible.

        std::string value;
        value.reserve(32);

        value += current_char();

        advance();

        while (current_char() != '"') {
            if (current_char() == '\n' || is_eof()) {
                throw error::LexerError("Unterminated string literal", statistics.line, statistics.column);
            }

            if (current_char() == '\\') {
                if (!LUAXC_LEXER_IS_VALID_ESCAPE_SEQUENCE(peek())) {
                    throw error::LexerError("Invalid escape sequence: " + std::string(1, peek()), 
                        statistics.line, statistics.column);
                }

                advance();
                switch (current_char()) {
                    case '\\':
                        value += '\\';
                        break;
                    case 'n':
                        value += '\n';
                        break;
                    case 't':
                        value += '\t';
                        break;
                    case 'r':
                        value += '\r';
                        break;
                    case '0':
                        value += '\0';
                        break;
                    case '"':
                        value += '"';
                    default:
                        throw error::LexerError("Invalid escape character", 
                            statistics.line, statistics.column);
                }
                advance();

                continue;
            }

            value += current_char();
            advance();
        }

        value += current_char();
        advance();

        return Token{TokenType::STRING_LITERAL, value};
    }

    Token Lexer::get_next_token() { 
        if (is_eof()) {
            return Token{TokenType::TERMINATOR, ""};
        }

        while (!is_eof()) {
            if (LUAXC_LEXER_IS_WHITESPACE(current_char())) {
                skip_whitespace();
                continue;
            }

            if (LUAXC_LEXER_PERHAPS_COMMENT(current_char(), peek())) {
                skip_comment();
                continue;
            }

            break;
        }

        if (LUAXC_LEXER_IS_DIGIT(current_char())) { 
            return get_digit();
        }

        if (LUAXC_LEXER_PERHAPS_IDENTIFIER(current_char())) { 
            return get_identifier();
        }

        if (LUAXC_LEXER_PERHAPS_STRING_LITERAL(current_char())) {
            return get_string_literal();
        }

        switch (current_char()) { 
            case ':':
                advance();
                return Token{TokenType::COLON, ":"};
            case ',':
                advance();
                return Token{TokenType::COMMA, ","};
            case '.':
                advance();
                return Token{TokenType::DOT, "."};
            case ';':
                advance();
                return Token{TokenType::SEMICOLON, ";"};
            case '(':
                advance();
                return Token{TokenType::L_PARENTHESIS, "("};
            case ')':
                advance();
                return Token{TokenType::R_PARENTHESIS, ")"};
            case '[':
                advance();
                return Token{TokenType::L_SQUARE_BRACE, "["};
            case ']':
                advance();
                return Token{TokenType::R_SQUARE_BRACE, "]"};
            case '{':
                advance();
                return Token{TokenType::L_CURLY_BRACKET, "{"};
            case '}':
                advance();
                return Token{TokenType::R_CURLY_BRACKET, "}"};
            case '<':
                advance();
                return Token{TokenType::LESS_THAN, "<"};
            case '>':
                advance();
                return Token{TokenType::GREATER_THAN, ">"};
            case '=':
                advance();
                return Token{TokenType::ASSIGN, "="};
            case '+':
                advance();
                return Token{TokenType::PLUS, "+"};
            case '-':
                advance();
                return Token{TokenType::MINUS, "-"};
            case '*':
                advance();
                return Token{TokenType::MUL, "*"};
            case '/':
                advance();
                return Token{TokenType::DIV, "/"};
            case '%':
                advance();
                return Token{TokenType::MOD, "%"};
            case '!':
                advance();
                return Token{TokenType::LOGICAL_NOT, "!"};
            case '&':
                advance();
                return Token{TokenType::BITWISE_AND, "&"};
            case '|':
                advance();
                return Token{TokenType::BITWISE_OR, "|"};
            case '^':
                advance();
                return Token{TokenType::BITWISE_XOR, "^"};
            case '~':
                advance();
                return Token{TokenType::BITWISE_NOT, "~"};
            default:
                if (current_char() == '\0') {
                    return Token{TokenType::TERMINATOR, ""};
                } else {
                    auto invalid_token = Token{TokenType::INVALID, std::string(1, current_char())};
                    advance();
                    return invalid_token;
                }
        }
    }

    std::vector<Token> Lexer::lex() { 
        std::vector<Token> tokens;
        while (!is_eof()) {
            auto token = get_next_token();

            if (token.type == TokenType::INVALID) {
                throw error::LexerError("Invalid token", statistics.line, statistics.column);
            }

            tokens.push_back(token);
        }
        return tokens;
    }
}