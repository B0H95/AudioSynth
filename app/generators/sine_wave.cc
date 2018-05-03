#include "audio_generators.hh"

#include <math.h>

namespace bzzt {

namespace {

struct sine_wave_generator {
    float state;
    float frequency;
    float volume;
};

void sine_wave_generator_render (std::vector<float> const& input, std::vector<float>& output, void* generator, audio_config const& config) {
    auto gen {static_cast<sine_wave_generator*>(generator)};
    for (auto i {static_cast<unsigned int>(0)}; i < config.buffer_size; ++i) {
        auto sine_state {gen->state * 3.1415926535 * 2};
        auto sine_position {sin(sine_state)};
        output[i] += input[i] + sine_position * gen->volume;
        gen->state += gen->frequency / static_cast<float>(config.sample_rate);
        gen->state -= (gen->state >= 1.0f) * 1.0f;
    }
}

bool sine_wave_generator_init(void* generator) {
    auto gen {static_cast<sine_wave_generator*>(generator)};
    gen->state = 0.0f;
    gen->frequency = 440.0f;
    gen->volume = 1.0f;
    return true;
}

void sine_wave_generator_deinit(void* generator) {
    (void)generator;
}

unsigned int sine_wave_generator_size() {
    return sizeof(sine_wave_generator);
}

void sine_wave_generator_set_parameter(void* generator, unsigned int parameter, float value) {
    auto gen {static_cast<sine_wave_generator*>(generator)};
    switch (parameter) {
        case 0:
            gen->volume = value;
            break;
        case 1:
            gen->frequency = value;
            break;
        default:
            break;
    }
}

const char* sine_wave_generator_id() {
    return "sine";
}

const auto sine_wave_generator_interface {audio_generator_interface{
    sine_wave_generator_render,
    sine_wave_generator_init,
    sine_wave_generator_deinit,
    sine_wave_generator_size,
    sine_wave_generator_set_parameter,
    sine_wave_generator_id
}};

}

template<>
audio_generator_interface const& get_audio_generator_interface_impl<audio_generator_type::SINE_WAVE>() {
    return sine_wave_generator_interface;
}

}