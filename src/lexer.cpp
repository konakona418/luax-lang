#include <unordered_set>

#include "lexer.hpp"

#define LUAXC_LEXER_IS_WHITESPACE(x) std::isspace(x)
#define LUAXC_LEXER_PERHAPS_COMMENT(_cur, _peek) ((_cur == '/' && _peek == '/') || (_cur == '/' && _peek == '*'))
#define LUAXC_LEXER_IS_DIGIT(x) (x >= '0' && x <= '9')
#define LUAXC_LEXER_PERHAPS_IDENTIFIER_OR_KEYWORD(x) (x >= 'a' && x <= 'z' || x >= 'A' && x <= 'Z' || x == '_')
#define LUAXC_LEXER_IS_IDENTIFIER(x) (x >= 'a' && x <= 'z' || x >= 'A' && x <= 'Z' || x == '_' || x >= '0' && x <= '9')
#define LUAXC_LEXER_PERHAPS_STRING_LITERAL(x) (x == '"' || x == '\'')
#define LUAXC_LEXER_IS_VALID_ESCAPE_SEQUENCE(_peek) (_peek == '\\' || _peek == '0' || _peek == 'n' || _peek == 't' || _peek == 'r' || _peek == '"' || _peek == '\'')
#define LUAXC_LEXER_IS_ALNUM(x) (LUAXC_LEXER_IS_IDENTIFIER(x) || x >= '0' && x <= '9')
#define LUAXC_LEXER_IS_ALNUM_OR_UNDERSCORE(x) (LUAXC_LEXER_IS_IDENTIFIER(x) || x == '_')

#define LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS(_first, _second, _type, _current, _peek) \
    if (_first == _current && _second == _peek) {                                   \
        advance();                                                                  \
        advance();                                                                  \
        return Token{_type, std::string{_first, _second}};                          \
    }

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
        while (LUAXC_LEXER_IS_WHITESPACE(current_char())) {
            if (current_char() == '\n') {
                set_statistics_next_line();
            }
            advance();
        }
    }

    void Lexer::skip_comment() {
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
        }

        if (current_char() == 'e' || current_char() == 'E') {
            is_floating_point = true;

            advance();
            if (current_char() == '-') {
                advance();
            }

            if (!LUAXC_LEXER_IS_DIGIT(current_char())) {
                throw error::LexerError("Expected digit after exponent",
                                        statistics.line, statistics.column);
            }

            while (LUAXC_LEXER_IS_DIGIT(current_char())) {
                advance();
            }
        } else if (current_char() == 'f') {
            advance();

            if (LUAXC_LEXER_IS_ALNUM_OR_UNDERSCORE(current_char())) {
                throw error::LexerError("Unexpected character '" + std::string(1, current_char()) + "' after float literal",
                                        statistics.line, statistics.column);
            }
        } else if (current_char() == 'u' || current_char() == 'i') {
            if (is_floating_point) {
                throw error::LexerError("Floating point number cannot have u or i suffix",
                                        statistics.line, pos - begin_offset);
            }
            advance();

            if (!LUAXC_LEXER_IS_DIGIT(current_char())) {
                throw error::LexerError("Expected digit (size in bytes) after u or i suffix",
                                        statistics.line, pos - begin_offset);
            }

            while (LUAXC_LEXER_IS_DIGIT(current_char())) {
                advance();
            }

            if (LUAXC_LEXER_IS_ALNUM_OR_UNDERSCORE(current_char())) {
                throw error::LexerError("Unexpected character '" + std::string(1, current_char()) + "' after interger literal",
                                        statistics.line, pos - begin_offset);
            }
        }

        return Token{TokenType::NUMBER, input.substr(begin_offset, pos - begin_offset)};
    }

    Token Lexer::get_identifier_or_keyword() {
        auto begin_offset = pos;

        while (LUAXC_LEXER_IS_IDENTIFIER(current_char())) {
            advance();
        }

        auto str = input.substr(begin_offset, pos - begin_offset);

        if (is_keyword(str)) {
            return Token{keyword_type(str), str};
        }

        return Token{TokenType::IDENTIFIER, str};
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

        if (LUAXC_LEXER_PERHAPS_IDENTIFIER_OR_KEYWORD(current_char())) {
            return get_identifier_or_keyword();
        }

        if (LUAXC_LEXER_PERHAPS_STRING_LITERAL(current_char())) {
            return get_string_literal();
        }

        if (!is_peek_eof()) {
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('=', '=',
                                               TokenType::EQUAL, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('!', '=',
                                               TokenType::NOT_EQUAL, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('<', '=',
                                               TokenType::LESS_THAN_EQUAL, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('>', '=',
                                               TokenType::GREATER_THAN_EQUAL, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('+', '+',
                                               TokenType::INCREMENT, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('-', '-',
                                               TokenType::DECREMENT, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('+', '=',
                                               TokenType::INCREMENT_BY, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('-', '=',
                                               TokenType::DECREMENT_BY, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('<', '<',
                                               TokenType::BITWISE_SHIFT_LEFT, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('>', '>',
                                               TokenType::BITWISE_SHIFT_RIGHT, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('&', '&',
                                               TokenType::LOGICAL_AND, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS('|', '|',
                                               TokenType::LOGICAL_OR, current_char(), peek())
            LUAXC_LEXER_HANDLE_TWO_CHAR_TOKENS(':', ':',
                                               TokenType::MODULE_ACCESS, current_char(), peek())
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
            case '@':
                advance();
                return Token{TokenType::AT, "@"};
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

    bool Lexer::is_keyword(const std::string& candidate) {
        const static std::unordered_set<std::string> keywords = {
                "let", "const",
                "func",
                "if", "else", "elif",
                "for", "while", "do",
                "break", "continue",
                "return", "use", "mod",
                "type", "method", "field",
                "true", "false", "null",
                "rule", "constraint"};
        return keywords.find(candidate) != keywords.end();
    }

    TokenType Lexer::keyword_type(const std::string& candidate) {
        switch (candidate[0]) {
            case 'b':
                if (candidate == "break") return TokenType::KEYWORD_BREAK;
                break;
            case 'c':
                if (candidate == "const") return TokenType::KEYWORD_CONST;
                if (candidate == "continue") return TokenType::KEYWORD_CONTINUE;
                if (candidate == "constraint") return TokenType::KEYWORD_CONSTRAINT;
                break;
            case 'd':
                return TokenType::KEYWORD_DO;
                break;
            case 'e':
                if (candidate == "elif") return TokenType::KEYWORD_ELIF;
                if (candidate == "else") return TokenType::KEYWORD_ELSE;
                break;
            case 'f':
                if (candidate == "for") return TokenType::KEYWORD_FOR;
                if (candidate == "func") return TokenType::KEYWORD_FUNC;
                if (candidate == "field") return TokenType::KEYWORD_FIELD;
                if (candidate == "false") return TokenType::KEYWORD_FALSE;
                break;
            case 'i':
                return TokenType::KEYWORD_IF;
                break;
            case 'l':
                if (candidate == "let") return TokenType::KEYWORD_LET;
                break;
            case 'm':
                if (candidate == "method") return TokenType::KEYWORD_METHOD;
                if (candidate == "mod") return TokenType::KEYWORD_MOD;
                break;
            case 'n':
                if (candidate == "null") return TokenType::KEYWORD_NULL;
                break;
            case 'r':
                if (candidate == "return") return TokenType::KEYWORD_RETURN;
                if (candidate == "rule") return TokenType::KEYWORD_RULE;
                break;
            case 't':
                if (candidate == "type") return TokenType::KEYWORD_TYPE;
                if (candidate == "true") return TokenType::KEYWORD_TRUE;
                break;
            case 'w':
                if (candidate == "while") return TokenType::KEYWORD_WHILE;
            case 'u':
                if (candidate == "use") return TokenType::KEYWORD_USE;
            default:
                break;
        }
        throw error::LexerError("Invalid keyword", statistics.line, statistics.column);
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

    Token Lexer::next() {
        auto token = get_next_token();
        if (token.type == TokenType::INVALID) {
            throw error::LexerError("Invalid token", statistics.line, statistics.column);
        }
        return token;
    }
}// namespace luaxc