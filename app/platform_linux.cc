#include "startup_parameters.hh"
#include "window.hh"
#include "audio_process.hh"

int main(int argc, char** argv) {
    bzzt::consume_command_line_arguments(argc, argv);

    auto audio_process {bzzt::audio_process{}};
    auto window {bzzt::window{}};

    while (!window.should_close()) {
        // glClear(GL_COLOR_BUFFER_BIT);
        window.update_screen();
        window.handle_incoming_events();
    }

    return 0;
}