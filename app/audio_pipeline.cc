#include "audio_pipeline.hh"

#include <map>
#include <vector>
#include <algorithm>
#include <limits.h>

namespace bzzt {

namespace {

struct pipeline_step {
    audio_generator_render_func render_func;

    audio_generator_type generator_type;
    unsigned int state_index;

    unsigned int input_buffer_index;
    unsigned int output_buffer_index;
};

}

struct audio_pipeline::impl {
    audio_config config;

    std::vector<pipeline_step> pipeline;
    std::map<audio_generator_type, std::vector<char>> state_lists;
    std::map<audio_generator_type, std::vector<bool>> state_lists_usage;

    std::vector<std::vector<float>> buffers;
    std::vector<bool> buffer_used;

    audio_pipeline::generator_handle add_generator(audio_generator_type type, unsigned int position) {
        if (state_lists.find(type) == state_lists.end()) {
            state_lists[type] = std::vector<char>{};
            state_lists[type].reserve(8192);

            state_lists_usage[type] = std::vector<bool>{};
            state_lists_usage[type].reserve(8192);
        }

        auto& state_list {state_lists[type]};
        auto& state_list_usage {state_lists_usage[type]};

        auto const& generator {get_audio_generator_interface(type)};
        auto generator_size = generator.size();

        auto free_position {std::find(std::begin(state_list_usage), std::end(state_list_usage), false)};
        if (free_position != std::end(state_list_usage)) {
            auto dist {std::distance(std::begin(state_list_usage), free_position)};
            auto state_pos {dist * generator_size};
            auto state_ptr {static_cast<void*>(&state_list[state_pos])};
            generator.init(state_ptr);
            if (!generator.init(state_ptr)) {
                return {type, UINT_MAX};
            }
            *free_position = true;
            auto new_step {pipeline_step {generator.render, type, static_cast<unsigned int>(state_pos), UINT_MAX, UINT_MAX}};
            pipeline.insert(std::begin(pipeline) + position, new_step);
            return {type, state_pos};
        }

        auto initial_state_list_size {state_list.size()};
        for (auto i {static_cast<unsigned int>(0)}; i < generator_size; ++i) {
            state_list.push_back(static_cast<char>(0));
        }
        state_list_usage.push_back(true);

        auto state_ptr {static_cast<void*>(&state_list[initial_state_list_size])};
        if (!generator.init(state_ptr)) {
            state_list_usage.back() = false;
            return {type, UINT_MAX};
        }

        auto new_step {pipeline_step {generator.render, type, static_cast<unsigned int>(initial_state_list_size), UINT_MAX, UINT_MAX}};
        pipeline.insert(std::begin(pipeline) + position, new_step);
        return {type, initial_state_list_size};
    }

    unsigned int get_generator_position(audio_pipeline::generator_handle ghandle) {
        auto iter {std::find_if(std::begin(pipeline), std::end(pipeline), [&](pipeline_step const& step) {
            return step.generator_type == std::get<0>(ghandle) && step.state_index == std::get<1>(ghandle);
        })};
        return iter != std::end(pipeline) ? std::distance(std::begin(pipeline), iter) : UINT_MAX;
    }

    void move_generator_to_position(audio_pipeline::generator_handle handle, unsigned int pos) {
        auto handle_pos {get_generator_position(handle)};
        auto step {pipeline[handle_pos]};
        pipeline.erase(std::begin(pipeline) + handle_pos);
        if (pos > pipeline.size()) {
            pos = pipeline.size();
        }
        pipeline.insert(std::begin(pipeline) + pos, step);
    }
};

audio_pipeline::audio_pipeline(audio_config const& config) : internal{new audio_pipeline::impl} {
    internal->config = config;
    internal->pipeline.reserve(1024);
}

audio_pipeline::~audio_pipeline() {
    delete internal;
}

audio_pipeline::generator_handle audio_pipeline::add_generator_front(audio_generator_type type) {
    return internal->add_generator(type, 0);
}

audio_pipeline::generator_handle audio_pipeline::add_generator_before(audio_generator_type type, audio_pipeline::generator_handle ghandle) {
    return internal->add_generator(type, internal->get_generator_position(ghandle));
}

audio_pipeline::generator_handle audio_pipeline::add_generator_after(audio_generator_type type, audio_pipeline::generator_handle ghandle) {
    return internal->add_generator(type, internal->get_generator_position(ghandle) + 1);
}

audio_pipeline::generator_handle audio_pipeline::add_generator_back(audio_generator_type type) {
    return internal->add_generator(type, internal->pipeline.size());
}

void audio_pipeline::move_generator_front(audio_pipeline::generator_handle handle) {
    internal->move_generator_to_position(handle, 0);
}

void audio_pipeline::move_generator_before(audio_pipeline::generator_handle handle, audio_pipeline::generator_handle other) {
    internal->move_generator_to_position(handle, internal->get_generator_position(other));
}

void audio_pipeline::move_generator_after(audio_pipeline::generator_handle handle, audio_pipeline::generator_handle other) {
    internal->move_generator_to_position(handle, internal->get_generator_position(other) + 1);
}

void audio_pipeline::move_generator_back(audio_pipeline::generator_handle handle) {
    internal->move_generator_to_position(handle, internal->pipeline.size());
}

void audio_pipeline::delete_generator(audio_pipeline::generator_handle handle) {
    auto generator_type {std::get<0>(handle)};
    auto const& generator {get_audio_generator_interface(generator_type)};
    auto generator_size {generator.size()};
    auto generator_index {std::get<1>(handle)};
    internal->state_lists_usage[generator_type][generator_index / generator_size] = false;
    auto state_ptr {static_cast<void*>(&internal->state_lists[generator_type][generator_index])};
    generator.deinit(state_ptr);
    auto new_end {std::remove_if(std::begin(internal->pipeline), std::end(internal->pipeline), [&](pipeline_step const& step) {
        return step.generator_type == generator_type && step.state_index == generator_index;
    })};
    internal->pipeline.erase(new_end, std::end(internal->pipeline));
}

audio_pipeline::buffer_handle audio_pipeline::add_buffer() {
    for (auto i {static_cast<unsigned int>(0)}; i < internal->buffer_used.size(); ++i) {
        if (!internal->buffer_used[i]) {
            internal->buffer_used[i] = true;
            return i;
        }
    }
    internal->buffers.push_back(std::vector<float>{});
    internal->buffers.back().resize(internal->config.buffer_size);
    internal->buffer_used.push_back(true);
    return internal->buffers.size() - 1;
}

std::vector<float> const& audio_pipeline::get_buffer(audio_pipeline::buffer_handle handle) const {
    return internal->buffers[handle];
}

void audio_pipeline::set_buffer(buffer_handle handle, std::vector<float> const& new_contents) {
    auto& buffer {internal->buffers[handle]};
    auto buffer_size {buffer.size()};
    if (buffer_size != new_contents.size()) {
        return;
    }
    for (auto i {static_cast<unsigned int>(0)}; i < buffer_size; ++i) {
        buffer[i] = new_contents[i];
    }
}

void audio_pipeline::set_generator_input_output (audio_pipeline::generator_handle ghandle, audio_pipeline::buffer_handle bhandle_in, audio_pipeline::buffer_handle bhandle_out) {
    set_generator_input(ghandle, bhandle_in);
    set_generator_output(ghandle, bhandle_out);
}

void audio_pipeline::set_generator_input(audio_pipeline::generator_handle ghandle, audio_pipeline::buffer_handle bhandle) {
    for (auto& step : internal->pipeline) {
        if (step.state_index == std::get<1>(ghandle) && step.generator_type == std::get<0>(ghandle)) {
            step.input_buffer_index = bhandle;
            return;
        }
    }
}

void audio_pipeline::set_generator_output(audio_pipeline::generator_handle ghandle, audio_pipeline::buffer_handle bhandle) {
    for (auto& step : internal->pipeline) {
        if (step.state_index == std::get<1>(ghandle) && step.generator_type == std::get<0>(ghandle)) {
            step.output_buffer_index = bhandle;
            return;
        }
    }
}

void audio_pipeline::set_generator_parameter(audio_pipeline::generator_handle ghandle, unsigned int parameter, float value) {
    auto const& generator {get_audio_generator_interface(std::get<0>(ghandle))};
    auto state_ptr = static_cast<void*>(&(internal->state_lists[std::get<0>(ghandle)][std::get<1>(ghandle)]));
    generator.set_parameter(state_ptr, parameter, value);
}

void audio_pipeline::remove_buffer(audio_pipeline::buffer_handle handle) {
    internal->buffer_used[handle] = false;
    for (auto& step : internal->pipeline) {
        if (step.input_buffer_index == handle) {
            step.input_buffer_index = UINT_MAX;
        }
        if (step.output_buffer_index == handle) {
            step.output_buffer_index = UINT_MAX;
        }
    }
}

void audio_pipeline::execute() {
    for (auto& step : internal->pipeline) {
        auto& input_buffer {internal->buffers[step.input_buffer_index]};
        auto& output_buffer {internal->buffers[step.output_buffer_index]};
        auto state {static_cast<void*>(&internal->state_lists[step.generator_type][step.state_index])};
        step.render_func(input_buffer, output_buffer, state, internal->config);
    }
}

unsigned int audio_pipeline::get_length() const {
    return internal->pipeline.size();
}

audio_config const& audio_pipeline::get_audio_config() const {
    return internal->config;
}

}