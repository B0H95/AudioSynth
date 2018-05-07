#pragma once

#include <vector>
#include <tuple>
#include "audio_generators.hh"
#include "audio_config.hh"

namespace bzzt {

struct audio_pipeline {
    using generator_handle = std::tuple<audio_generator_type, unsigned int>;
    using buffer_handle    = unsigned int;

    audio_pipeline  (audio_config const& config);
    audio_pipeline  (audio_pipeline const& other) = delete;
    audio_pipeline  (audio_pipeline&& other) = delete;
    audio_pipeline& operator= (audio_pipeline const& other) = delete;
    audio_pipeline& operator= (audio_pipeline&& other) = delete;
    ~audio_pipeline ();

    generator_handle add_generator_front  (audio_generator_type type);
    generator_handle add_generator_before (audio_generator_type type, generator_handle ghandle);
    generator_handle add_generator_after  (audio_generator_type type, generator_handle ghandle);
    generator_handle add_generator_back   (audio_generator_type type);

    void move_generator_front  (generator_handle handle);
    void move_generator_before (generator_handle handle, generator_handle other);
    void move_generator_after  (generator_handle handle, generator_handle other);
    void move_generator_back   (generator_handle handle);

    void delete_generator(generator_handle handle);

    buffer_handle add_buffer();

    std::vector<float> const& get_buffer(buffer_handle handle) const;
    void set_buffer(buffer_handle handle, std::vector<float> const& new_contents);
    void reset_all_buffers();

    void set_generator_input        (generator_handle ghandle, unsigned int input_id,   buffer_handle bhandle);
    void set_generator_output       (generator_handle ghandle, unsigned int output_id,  buffer_handle bhandle);
    void set_generator_parameter    (generator_handle ghandle, unsigned int parameter,  float value);

    void remove_buffer(buffer_handle handle);

    void execute();

    unsigned int        get_length() const;
    audio_config const& get_audio_config() const;

private:
    struct impl;
    impl* internal;
};

}
