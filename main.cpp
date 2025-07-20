#include "ir.hpp"

#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    luaxc::IRRuntime runtime;

    std::string input_file = "test.lx";
    std::fstream input_file_stream(input_file);
    std::string input_file_contents = std::string(std::istreambuf_iterator<char>(input_file_stream),
                                                  std::istreambuf_iterator<char>());
    runtime.compile(input_file_contents);

    auto byte_code = luaxc::dump_bytecode(runtime.get_byte_code());
    std::cout << byte_code << std::endl;

    runtime.run();

    return 0;
}
