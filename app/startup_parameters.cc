#include "startup_parameters.hh"

#include <vector>
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

std::string get_pipeline_configuration_filename() {
    static std::string param {"--pipeline-config="};
    for (auto const& arg : command_line_arguments) {
        auto index {arg.find(param)};
        if (index != std::string::npos && index == 0 && arg.size() > param.size()) {
            auto filename_length {arg.size() - param.size()};
            auto filename_pos {param.size()};
            return arg.substr(filename_pos, filename_length);
        }
    }
    return "";
}

}