#pragma once

namespace bzzt {

using audio_generator_run_func = void (*)(
    float*       inputs,
    float*       outputs,
    void*        generator,
    unsigned int sample_rate
);

using audio_generator_init_func = unsigned int (*)(
    void* generator
);

using audio_generator_deinit_func = void (*)(
    void* generator
);

using audio_generator_id_func = const char* (*)();

using audio_generator_size_func = unsigned int (*)();

using audio_generator_input_count_func = unsigned int (*)();

using audio_generator_output_count_func = unsigned int (*)();

struct audio_generator_interface {
    audio_generator_run_func run;
    audio_generator_init_func init;
    audio_generator_deinit_func deinit;
    audio_generator_id_func id;
    audio_generator_size_func size;
    audio_generator_input_count_func input_count;
    audio_generator_output_count_func output_count;
};

}