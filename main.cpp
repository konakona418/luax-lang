#include "ir.hpp"

#include <fstream>

#define LUAXC_COMPILER_VERSION "0.1.0"

class ArgParser {
public:
    ArgParser(int argc, char** argv) : argc(argc), argv(argv), idx(0) {}

    void parse() {
        advance("Usage: luax <file>; Expected an input file!");
        output.file = get_current_arg();
        while (idx < argc - 1) {
            advance("");
            if (get_current_arg() == "-i") {
                advance("Usage: luax <file> -i <include path>; Expected an include path!");
                output.has_included_path = true;
                output.included_path = get_current_arg();
                continue;
            }
            if (get_current_arg() == "-d") {
                advance("Usage: luax <file> -d <dump file>; Expected a dump file!");
                output.dump_bytecode = true;
                output.dump_bytecode_file = get_current_arg();
                continue;
            }
        }
    }

    struct {
        std::string file;
        bool has_included_path = false;
        std::string included_path;
        bool dump_bytecode = false;
        std::string dump_bytecode_file;
    } output;

private:
    int argc;
    char** argv;

    int idx;

    void advance(const std::string& expected) {
        idx++;
        if (idx > argc - 1) {
            throw std::runtime_error(expected);
        }
    }

    std::string get_current_arg() const { return argv[idx]; }
};

int main(int argc, char** argv) {
#ifdef LUAXC_COMPILER_DEBUG

    luaxc::IRRuntime runtime;

    std::string input_file = "test.lx";
    std::fstream input_file_stream(input_file);
    std::string input_file_contents = std::string(std::istreambuf_iterator<char>(input_file_stream),
                                                  std::istreambuf_iterator<char>());
    input_file_stream.close();

    runtime.compile(input_file_contents);

    runtime.run();

#else
    ArgParser parser(argc, argv);
    parser.parse();

    luaxc::IRRuntime runtime;
    if (parser.output.has_included_path) {
        runtime.get_runtime_context().import_path = parser.output.included_path;
    }

    std::string input_file = parser.output.file;
    std::fstream input_file_stream(input_file);
    std::string input_file_contents = std::string(std::istreambuf_iterator<char>(input_file_stream),
                                                  std::istreambuf_iterator<char>());
    input_file_stream.close();

    runtime.compile(input_file_contents);

    if (parser.output.dump_bytecode) {
        auto byte_code = luaxc::dump_bytecode(runtime.get_byte_code());
        std::string dump_file_name = input_file + ".dump";
        std::ofstream dump_file(dump_file_name);
        dump_file << byte_code;
        dump_file.close();
    }

    runtime.run();
#endif

    return 0;
}
