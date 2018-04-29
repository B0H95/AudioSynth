#include "audio_generators.hh"

namespace bzzt {

namespace {

struct adder_generator {
    float volume;
};

void adder_generator_render(std::vector<float> const& input, std::vector<float>& output, void* generator, audio_config const& config) {
    auto gen {static_cast<adder_generator*>(generator)};
    for (auto i {static_cast<unsigned int>(0)}; i < config.buffer_size; ++i) {
        output[i] += input[i] * gen->volume;
    }
}

bool adder_generator_init(void* generator) {
    auto gen {static_cast<adder_generator*>(generator)};
    gen->volume = 1.0f;
    return true;
}

void adder_generator_deinit(void* generator) {
    (void)generator;
}

unsigned int adder_generator_size() {
    return sizeof(adder_generator);
}

void adder_generator_set_parameter(void* generator, unsigned int parameter, float value) {
    auto gen {static_cast<adder_generator*>(generator)};
    switch (parameter) {
        case 0:
            gen->volume = value;
            break;
        default:
            break;
    }
}

const auto adder_generator_interface {audio_generator_interface{
    adder_generator_render,
    adder_generator_init,
    adder_generator_deinit,
    adder_generator_size,
    adder_generator_set_parameter
}};

}

template<>
audio_generator_interface const& get_audio_generator_interface_impl<audio_generator_type::ADD>() {
    return adder_generator_interface;
}

}