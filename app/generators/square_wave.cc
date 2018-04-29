#include "audio_generators.hh"

namespace bzzt {

namespace {

struct square_wave_generator {
    float state;
    float frequency;
    float volume;
    float phase;
};

void square_wave_generator_render (std::vector<float> const& input, std::vector<float>& output, void* generator, audio_config const& config) {
    auto gen {static_cast<square_wave_generator*>(generator)};
    for (auto i {static_cast<unsigned int>(0)}; i < config.buffer_size; ++i) {
        auto square_position {-1.0f + (gen->state > (gen->phase + 1.0f) / 2.0f) * 2.0f};
        output[i] += input[i] + square_position * gen->volume;
        gen->state += gen->frequency / static_cast<float>(config.sample_rate);
        gen->state -= (gen->state >= 1.0f) * 1.0f;
    }
}

bool square_wave_generator_init(void* generator) {
    auto gen {static_cast<square_wave_generator*>(generator)};
    gen->state = 0.0f;
    gen->frequency = 440.0f;
    gen->volume = 1.0f;
    gen->phase = 0.0f;
    return true;
}

void square_wave_generator_deinit(void* generator) {
    (void)generator;
}

unsigned int square_wave_generator_size() {
    return sizeof(square_wave_generator);
}

void square_wave_generator_set_parameter(void* generator, unsigned int parameter, float value) {
    auto gen {static_cast<square_wave_generator*>(generator)};
    switch (parameter) {
        case 0:
            gen->volume = value;
            break;
        case 1:
            gen->frequency = value;
            break;
        case 2:
            gen->phase = value;
            break;
        default:
            break;
    }
}

const auto square_wave_generator_interface {audio_generator_interface{
    square_wave_generator_render,
    square_wave_generator_init,
    square_wave_generator_deinit,
    square_wave_generator_size,
    square_wave_generator_set_parameter
}};

}

template<>
audio_generator_interface const& get_audio_generator_interface_impl<audio_generator_type::SQUARE_WAVE>() {
    return square_wave_generator_interface;
}

}