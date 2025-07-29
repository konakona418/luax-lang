#include "repl.hpp"

#include <iomanip>
#include <iostream>

#define __LUAXC_REPL_PRINT_ERR(message) std::cout << __LUAXC_REPL_COLORS_RED << message << __LUAXC_REPL_COLORS_RESET << std::endl;

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

    ReplEnv::~ReplEnv() {
        if (!runtime.is_generator_present()) {
            return;
        }

        assert(runtime.get_generator().end_module_compilation() == 0);
    }

    void ReplEnv::run() {
        write_rainbow_line("LuaX REPL " + std::string(LUAXC_REPL_ENV_VERSION));

        std::cout << "Type " << __LUAXC_REPL_COLORS_MAGENTA << "'/quit'" << __LUAXC_REPL_COLORS_RESET << " to quit;" << std::endl;
        std::cout << "Type " << __LUAXC_REPL_COLORS_MAGENTA << "'/help'" << __LUAXC_REPL_COLORS_RESET << " for help." << std::endl;

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

            if (line.front() == '/') {
                dispatch_internal_command(line);
                continue;
            }

            try {
                addline(line);
            } catch (const error::ReplEnvException& e) {
                __LUAXC_REPL_PRINT_ERR("Error: " << e.message)
                continue;
            }

            if (can_execute()) {
                input_count++;

                try {
                    runtime.eval(buffer);
                } catch (const IRRuntime::EvaluationException& e) {
                    __LUAXC_REPL_PRINT_ERR("Error: " << e.what() << "; Stack dumped.")
                    show_stack_info();

                    runtime.get_interpreter().set_byte_code(e.cached_interpreter_code);
                    runtime.get_interpreter().load_snapshot(e.cached_interpreter_state);
                }
                buffer.clear();
            }
        }
    }

    void ReplEnv::dispatch_internal_command(const std::string& line) {
        if (line == "/help") {
            show_help_message();
            return;
        }

        if (line == "/stack") {
            show_stack_info();
            return;
        }

        if (line == "/bytecode") {
            show_byte_code();
            return;
        }

        if (line == "/gcstats") {
            show_gc_info();
            return;
        }

        __LUAXC_REPL_PRINT_ERR("Unknown command: " << line);
    }

    void ReplEnv::show_help_message() {
        std::cout << "LuaXC REPL Help" << std::endl;
        std::cout << "  /help - Show this help message" << std::endl;
        std::cout << "  /stack - Show current stack frames" << std::endl;
        std::cout << "  /bytecode - Show current bytecode" << std::endl;
        std::cout << "  /gcstats - Show current garbage collector stats" << std::endl;
    }

    void ReplEnv::show_stack_info() {
        constexpr size_t max_stack_trace_depth = 16;

        if (!runtime.is_interpreter_present()) {
            __LUAXC_REPL_PRINT_ERR("Interpreter not present")
            return;
        }

        std::cout << "Stack Info:" << std::endl;
        std::cout << "-- Operand Stack (from most recent to last):" << std::endl;

        auto* op_stack = runtime.get_interpreter().get_op_stack_ptr();
        size_t i = 0;
        for (auto it = op_stack->rbegin(); it != op_stack->rend(); ++it) {
            std::cout << "   " << i << ": " << it->to_string() << std::endl;
            i++;
        }

        if (op_stack->empty()) {
            std::cout << "   <empty>" << std::endl;
        }

        std::cout << "-- Stack Frame (from most recent to last):" << std::endl;
        auto* stack_frames = runtime.get_interpreter().get_stack_frames_ptr();
        i = 0;
        for (auto it = stack_frames->rbegin(); it != stack_frames->rend(); ++it) {
            if (i > max_stack_trace_depth) {
                std::cout << "   ....<" << stack_frames->size() - max_stack_trace_depth << " frames truncated>" << std::endl;
                break;
            }

            std::cout << "   " << "Frame #" << i << ":" << std::endl;
            for (auto& var: it->variables) {
                auto name = var.first->contained_string();

                if (name.find("__builtin_") != std::string::npos) {
                    continue;
                }

                std::cout << "      " << "| " << name << " = " << var.second.to_string() << std::endl;
            }

            if (it->variables.empty()) {
                std::cout << "      " << "| <empty>" << std::endl;
            }

            i++;
        }
    }

    void ReplEnv::show_byte_code() {
        std::cout << dump_bytecode(runtime.get_byte_code()) << std::endl;
    }

    std::string ReplEnv::cvt_bytes_to_string(size_t bytes) {
        std::stringstream ss;
        if (bytes < 1024) {
            ss << bytes << " B";
        } else if (bytes < 1024 * 1024) {
            ss << std::fixed << std::setprecision(2) << (bytes / 1024.0) << " KB";
        } else {
            ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
        }
        return ss.str();
    }

    void ReplEnv::show_gc_info() {
        auto stats = runtime.get_gc().dump_stats();

        std::cout << "GC Stats:" << std::endl
                  << (stats.running
                              ? (__LUAXC_REPL_COLORS_GREEN "Running")
                              : (__LUAXC_REPL_COLORS_RED "Stopped"))
                  << __LUAXC_REPL_COLORS_RESET << std::endl
                  << "--  Max Heap Size: " << cvt_bytes_to_string(stats.max_heap_size) << std::endl
                  << "--  Heap Size: " << cvt_bytes_to_string(stats.heap_size) << std::endl
                  << "--  Objects: " << stats.object_count << std::endl;
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
            LUAXC_REPL_THROW_ERROR("Unmatched right bracket");
        }

        char left = bracket_stack.top();
        if (left == '(' && right == ')') {
            bracket_stack.pop();
        } else if (left == '{' && right == '}') {
            bracket_stack.pop();
        } else if (left == '[' && right == ']') {
            bracket_stack.pop();
        } else {
            LUAXC_REPL_THROW_ERROR("Unmatched bracket");
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

    void ReplEnv::write_rainbow_line(const std::string& line) {
        for (size_t i = 0; i < line.size(); ++i) {
            double frequency = 0.3;
            int r = static_cast<int>(std::sin(frequency * i + 0) * 127 + 128);
            int g = static_cast<int>(std::sin(frequency * i + 2) * 127 + 128);
            int b = static_cast<int>(std::sin(frequency * i + 4) * 127 + 128);

            std::printf("\033[38;2;%d;%d;%dm%c", r, g, b, line[i]);
        }

        std::cout << __LUAXC_REPL_COLORS_RESET << std::endl;
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