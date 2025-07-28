#include "repl.hpp"

#include <iostream>

namespace luaxc {

    ReplEnv::ReplEnv() {
        runtime.handlers.pop_stack_handler = [this](IRPrimValue value) {
            if (value.is_unit()) {
                return;
            }

            if (value.is_string()) {
                writeline(static_cast<StringObject*>(value.get_inner_value<GCObject*>())->contained_string());
                return;
            }

            writeline(value.to_string());
        };
    }

    void ReplEnv::run() {
        std::cout << "LuaX REPL " << LUAXC_REPL_ENV_VERSION << std::endl;
        std::cout << "Type '/quit' to quit" << std::endl;

        std::string line;
        while (true) {
            line = readline();

            if (line.empty()) {
                continue;
            }

            line = trim(line);

            if (line == "/quit") {
                break;
            }

            addline(line);

            if (can_execute()) {
                input_count++;

                try {
                    runtime.eval(buffer);
                } catch (const std::exception& e) {
                    std::cout << __LUAXC_REPL_COLORS_RED << "Error: " << e.what() << __LUAXC_REPL_COLORS_RESET << std::endl;
                }
                buffer.clear();
            }
        }
    }

    void ReplEnv::addline(const std::string& line) {
        for (auto& c: line) {
            if (__LUAXC_REPL_IS_LEFT_BRACKET(c)) {
                add_one_bracket(c);
            } else if (__LUAXC_REPL_IS_RIGHT_BRACKET(c)) {
                remove_one_bracket(c);
            }
        }

        buffer += line;
    }

    bool ReplEnv::can_execute() const {
        return bracket_stack.empty() && !buffer.empty() &&
               (buffer.back() == ';' || buffer.back() == '}');
    }

    void ReplEnv::add_one_bracket(char left) {
        bracket_stack.push(left);
    }

    void ReplEnv::remove_one_bracket(char right) {
        if (bracket_stack.empty()) {
            LUAXC_GC_THROW_ERROR("Unmatched right bracket");
        }

        char left = bracket_stack.top();
        if (left == '(' && right == ')') {
            bracket_stack.pop();
        } else if (left == '{' && right == '}') {
            bracket_stack.pop();
        } else if (left == '[' && right == ']') {
            bracket_stack.pop();
        } else {
            LUAXC_GC_THROW_ERROR("Unmatched bracket");
        }
    }

    std::string ReplEnv::readline() {
        std::string line;

        if (buffer.empty()) {
            std::cout << __LUAXC_REPL_COLORS_CYAN << "In[" << input_count << "]: " << __LUAXC_REPL_COLORS_RESET;
        } else {
            std::string indent(std::to_string(input_count).size(), ' ');
            std::cout << __LUAXC_REPL_COLORS_CYAN << indent << "....: " << __LUAXC_REPL_COLORS_RESET;
        }

        std::getline(std::cin, line);

        return line;
    }

    void ReplEnv::writeline(const std::string& line) {
        std::cout << __LUAXC_REPL_COLORS_GREEN << "Out[" << output_count++ << "]: " << __LUAXC_REPL_COLORS_RESET << line << std::endl;
    }

    std::string ReplEnv::trim(const std::string& str) {
        const std::string whitespace = " \t\n\r\f\v";

        size_t first = str.find_first_not_of(whitespace);
        if (first == std::string::npos) {
            return "";
        }

        size_t last = str.find_last_not_of(whitespace);
        return str.substr(first, last - first + 1);
    }
}// namespace luaxc