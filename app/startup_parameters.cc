#include "startup_parameters.hh"

#include <vector>
#include <string>
#include <algorithm>

namespace bzzt {

namespace {

std::vector<std::string> command_line_arguments;

}

void consume_command_line_arguments(int argc, char** argv) {
    command_line_arguments = std::vector<std::string>{argv + 1, argv + argc};
}

bool global_audio_enabled() {
    auto end {std::end(command_line_arguments)};
    return std::find(std::begin(command_line_arguments), end, "--no-audio") == end;
}

}