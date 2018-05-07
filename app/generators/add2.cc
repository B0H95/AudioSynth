#include "audio_generators.hh"

namespace bzzt {

namespace {

struct add2_generator {
    float volume;
};

const audio_generator_properties add2_properties {
    "add2",
    sizeof(add2_generator),
    2,
    1
};

void add2_generator_render(const float* const* input, float* const* output, void* generator, audio_config const& config) {
    auto gen {static_cast<add2_generator*>(generator)};
    for (auto i {static_cast<unsigned int>(0)}; i < config.buffer_size; ++i) {
        output[0][i] = (input[0][i] + input[1][i]) * gen->volume;
    }
}

bool add2_generator_init(void* generator) {
    auto gen {static_cast<add2_generator*>(generator)};
    gen->volume = 1.0f;
    return true;
}

void add2_generator_deinit(void* generator) {
    (void)generator;
}

void add2_generator_set_parameter(void* generator, unsigned int parameter, float value) {
    auto gen {static_cast<add2_generator*>(generator)};
    switch (parameter) {
        case 0:
            gen->volume = value;
            break;
        default:
            break;
    }
}

audio_generator_properties const& add2_generator_get_properties() {
    return add2_properties;
}

const auto add2_generator_interface {audio_generator_interface{
    add2_generator_render,
    add2_generator_init,
    add2_generator_deinit,
    add2_generator_set_parameter,
    add2_generator_get_properties
}};

}

template<>
audio_generator_interface const& get_audio_generator_interface_impl<audio_generator_type::ADD2>() {
    return add2_generator_interface;
}

}