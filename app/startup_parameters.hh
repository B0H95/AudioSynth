#pragma once

#include <string>

namespace bzzt {

void consume_command_line_arguments(int argc, char** argv);

bool global_audio_enabled();
std::string get_pipeline_configuration_filename();

}