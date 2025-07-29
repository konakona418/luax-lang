#include "ir.hpp"

#include <stack>

#define LUAXC_REPL_ENV_VERSION "0.1.0"

#define __LUAXC_REPL_IS_LEFT_BRACKET(c) (c == '(' || c == '{' || c == '[')
#define __LUAXC_REPL_IS_RIGHT_BRACKET(c) (c == ')' || c == '}' || c == ']')

#define LUAXC_REPL_THROW_ERROR(message) throw error::ReplEnvException(message)

#define __LUAXC_REPL_COLORS_RESET "\033[0m"
#define __LUAXC_REPL_COLORS_RED "\033[31m"
#define __LUAXC_REPL_COLORS_GREEN "\033[32m"
#define __LUAXC_REPL_COLORS_YELLOW "\033[33m"
#define __LUAXC_REPL_COLORS_BLUE "\033[34m"
#define __LUAXC_REPL_COLORS_MAGENTA "\033[35m"
#define __LUAXC_REPL_COLORS_CYAN "\033[36m"


namespace luaxc {
    namespace error {
        class ReplEnvException : public std::exception {
        public:
            std::string message;
            explicit ReplEnvException(std::string message) {
                this->message = "ReplEnv: " + message;
            }

            const char* what() const noexcept override {
                return message.c_str();
            }
        };
    }// namespace error

    class ReplEnv {
    public:
        ReplEnv();

        ~ReplEnv();

        void run();

        void dispatch_internal_command(const std::string& line);

        void addline(const std::string& line);

        bool can_execute() const;

        void add_one_bracket(char left);

        void remove_one_bracket(char right);

        std::string readline();

        void writeline(const std::string& line);

        void write_rainbow_line(const std::string& line);

    private:
        std::stack<char> bracket_stack;
        std::string buffer;

        IRRuntime runtime;

        size_t input_count = 0;
        size_t output_count = 0;

        std::string trim(const std::string& str);

        std::string cvt_bytes_to_string(size_t bytes);

        void show_help_message();

        void show_stack_info();

        void show_byte_code();

        void show_gc_info();
    };
}// namespace luaxc