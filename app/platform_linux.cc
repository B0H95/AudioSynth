#include "startup_parameters.hh"
#include "window.hh"
#include "audio_process.hh"
#include "storage.hh"
#include <memory>

int main(int argc, char** argv) {
    bzzt::consume_command_line_arguments(argc, argv);

    auto audio_process {std::make_unique<bzzt::audio_process>()};

    auto pipeline_config_filename {bzzt::get_pipeline_configuration_filename()};
    if (pipeline_config_filename.size() > 0) {
        audio_process = bzzt::load_pipeline_from_file(std::move(audio_process), pipeline_config_filename);
    }

    auto window {bzzt::window{}};

    while (!window.should_close()) {
        // glClear(GL_COLOR_BUFFER_BIT);
        window.update_screen();
        window.handle_incoming_events();
    }

    return 0;
}