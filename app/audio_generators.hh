#pragma once

#include <vector>
#include <string>
#include "audio_config.hh"

namespace bzzt {

enum struct audio_generator_type : unsigned int {
    ADD,
    SINE_WAVE,
    SQUARE_WAVE,
    SIZE,
    INVALID
};

using audio_generator_render_func = void (*)(
    std::vector<float> const& input,
    std::vector<float>&       output,
    void*                     generator,
    audio_config const&       config
);

using audio_generator_init_func = bool (*)(
    void* generator
);

using audio_generator_deinit_func = void (*)(
    void* generator
);

using audio_generator_size_func = unsigned int (*)();

using audio_generator_set_parameter_func = void (*)(
    void* generator,
    unsigned int parameter,
    float value
);

using audio_generator_id_func = const char* (*)();

struct audio_generator_interface {
    audio_generator_render_func        render;
    audio_generator_init_func          init;
    audio_generator_deinit_func        deinit;
    audio_generator_size_func          size;
    audio_generator_set_parameter_func set_parameter;
    audio_generator_id_func            id;
};

template<audio_generator_type Type>
audio_generator_interface const& get_audio_generator_interface_impl() {
    static auto default_interface {audio_generator_interface{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}};
    return default_interface;
}

audio_generator_interface const& get_audio_generator_interface(audio_generator_type type);

audio_generator_type get_audio_generator_type_by_id(const char* id);

}