#pragma once

#include <vector>
#include <tuple>
#include <string>
#include "audio_config.hh"
#include "audio_generator_interface.hh"

namespace bzzt {

struct audio_pipeline {
    using generator_type_handle = unsigned int;
    using generator_handle      = std::tuple<generator_type_handle, unsigned int>;
    using buffer_handle         = unsigned int;

    audio_pipeline  (audio_config const& config);
    audio_pipeline  (audio_pipeline const& other) = delete;
    audio_pipeline  (audio_pipeline&& other) = delete;
    audio_pipeline& operator= (audio_pipeline const& other) = delete;
    audio_pipeline& operator= (audio_pipeline&& other) = delete;
    ~audio_pipeline ();

    generator_type_handle add_generator_type(std::string const& generator_code);
    bool generator_type_is_valid(generator_type_handle handle) const;
    audio_generator_interface const& get_generator_interface(generator_type_handle handle) const;

    generator_handle add_generator_front  (generator_type_handle type);
    generator_handle add_generator_before (generator_type_handle type, generator_handle ghandle);
    generator_handle add_generator_after  (generator_type_handle type, generator_handle ghandle);
    generator_handle add_generator_back   (generator_type_handle type);

    void move_generator_front  (generator_handle handle);
    void move_generator_before (generator_handle handle, generator_handle other);
    void move_generator_after  (generator_handle handle, generator_handle other);
    void move_generator_back   (generator_handle handle);

    void delete_generator(generator_handle handle);

    buffer_handle add_buffer();

    std::vector<float> const& get_buffer(buffer_handle handle) const;
    void set_buffer(buffer_handle handle, std::vector<float> const& new_contents);

    void set_generator_input_value   (generator_handle ghandle, unsigned int input_id,  float value);
    void set_generator_input_buffer  (generator_handle ghandle, unsigned int input_id,  buffer_handle bhandle);
    void set_generator_output_buffer (generator_handle ghandle, unsigned int output_id, buffer_handle bhandle);

    void remove_buffer(buffer_handle handle);

    void execute();

    unsigned int        get_length() const;
    audio_config const& get_audio_config() const;

private:
    struct impl;
    impl* internal;
};

}
