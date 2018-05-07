#include "audio_pipeline.hh"

#include <map>
#include <vector>
#include <array>
#include <algorithm>
#include <limits.h>

namespace bzzt {

namespace {

const unsigned int MAX_INPUT_BUFFERS {8};
const unsigned int MAX_OUTPUT_BUFFERS {8};

struct pipeline_step {
    audio_generator_render_func render_func;

    audio_generator_type generator_type;
    unsigned int state_index;

    unsigned int inputs;
    unsigned int outputs;
};

}

struct audio_pipeline::impl {
    audio_config config;

    std::vector<pipeline_step> pipeline;
    std::array<std::vector<unsigned int>, MAX_INPUT_BUFFERS> pipeline_input_buffers;
    std::array<std::vector<unsigned int>, MAX_OUTPUT_BUFFERS> pipeline_output_buffers;

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
        auto generator_size = generator.get_properties().size;

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
            auto new_step {pipeline_step {generator.render, type, static_cast<unsigned int>(state_pos), generator.get_properties().inputs, generator.get_properties().outputs}};
            pipeline.insert(std::begin(pipeline) + position, new_step);
            for (auto& input_buffer_list : pipeline_input_buffers) {
                input_buffer_list.insert(std::begin(input_buffer_list) + position, UINT_MAX);
            }
            for (auto& output_buffer_list : pipeline_output_buffers) {
                output_buffer_list.insert(std::begin(output_buffer_list) + position, UINT_MAX);
            }
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

        auto new_step {pipeline_step {generator.render, type, static_cast<unsigned int>(initial_state_list_size), generator.get_properties().inputs, generator.get_properties().outputs}};
        pipeline.insert(std::begin(pipeline) + position, new_step);
        for (auto& input_buffer_list : pipeline_input_buffers) {
            input_buffer_list.insert(std::begin(input_buffer_list) + position, UINT_MAX);
        }
        for (auto& output_buffer_list : pipeline_output_buffers) {
            output_buffer_list.insert(std::begin(output_buffer_list) + position, UINT_MAX);
        }
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

        for (auto& input_buffer_list : pipeline_input_buffers) {
            auto buffer_id {input_buffer_list[handle_pos]};
            input_buffer_list.erase(std::begin(input_buffer_list) + handle_pos);
            input_buffer_list.insert(std::begin(input_buffer_list) + pos, buffer_id);
        }
        for (auto& output_buffer_list : pipeline_input_buffers) {
            auto buffer_id {output_buffer_list[handle_pos]};
            output_buffer_list.erase(std::begin(output_buffer_list) + handle_pos);
            output_buffer_list.insert(std::begin(output_buffer_list) + pos, buffer_id);
        }
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
    auto generator_position {internal->get_generator_position(handle)};
    auto generator_type {std::get<0>(handle)};
    auto const& generator {get_audio_generator_interface(generator_type)};
    auto generator_size {generator.get_properties().size};
    auto generator_index {std::get<1>(handle)};
    internal->state_lists_usage[generator_type][generator_index / generator_size] = false;
    auto state_ptr {static_cast<void*>(&internal->state_lists[generator_type][generator_index])};
    generator.deinit(state_ptr);
    auto new_end {std::remove_if(std::begin(internal->pipeline), std::end(internal->pipeline), [&](pipeline_step const& step) {
        return step.generator_type == generator_type && step.state_index == generator_index;
    })};
    internal->pipeline.erase(new_end, std::end(internal->pipeline));
    for (auto& input_buffer_list : internal->pipeline_input_buffers) {
        input_buffer_list.erase(std::begin(input_buffer_list) + generator_position);
    }
    for (auto& output_buffer_list : internal->pipeline_output_buffers) {
        output_buffer_list.erase(std::begin(output_buffer_list) + generator_position);
    }
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

void audio_pipeline::reset_all_buffers() {
    for (auto& buffer : internal->buffers) {
        for (unsigned int i {0}; i < internal->config.buffer_size; ++i) {
            buffer[i] = 0.0f;
        }
    }
}

void audio_pipeline::set_generator_input(audio_pipeline::generator_handle ghandle, unsigned int input_id, audio_pipeline::buffer_handle bhandle) {
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        auto& step {internal->pipeline[i]};
        if (step.state_index == std::get<1>(ghandle) && step.generator_type == std::get<0>(ghandle)) {
            internal->pipeline_input_buffers[input_id][i] = bhandle;
            return;
        }
    }
}

void audio_pipeline::set_generator_output(audio_pipeline::generator_handle ghandle, unsigned int output_id, audio_pipeline::buffer_handle bhandle) {
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        auto& step {internal->pipeline[i]};
        if (step.state_index == std::get<1>(ghandle) && step.generator_type == std::get<0>(ghandle)) {
            internal->pipeline_output_buffers[output_id][i] = bhandle;
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
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        for (auto& input_buffer_list : internal->pipeline_input_buffers) {
            if (input_buffer_list[i] == handle) {
                input_buffer_list[i] = UINT_MAX;
            }
        }
        for (auto& output_buffer_list : internal->pipeline_output_buffers) {
            if (output_buffer_list[i] == handle) {
                output_buffer_list[i] = UINT_MAX;
            }
        }
    }
}

void audio_pipeline::execute() {
    float* input_buffer_list[MAX_INPUT_BUFFERS];
    float* output_buffer_list[MAX_OUTPUT_BUFFERS];
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        auto& step {internal->pipeline[i]};
        input_buffer_list[0] = internal->buffers[internal->pipeline_input_buffers[0][i]].data();
        output_buffer_list[0] = internal->buffers[internal->pipeline_output_buffers[0][i]].data();
        for (unsigned int j {1}; j < step.inputs; ++j) {
            input_buffer_list[j] = internal->buffers[internal->pipeline_input_buffers[j][i]].data();
        }
        for (unsigned int j {1}; j < step.outputs; ++j) {
            output_buffer_list[j] = internal->buffers[internal->pipeline_output_buffers[j][i]].data();
        }
        auto state {static_cast<void*>(&internal->state_lists[step.generator_type][step.state_index])};
        step.render_func(input_buffer_list, output_buffer_list, state, internal->config);
    }
}

unsigned int audio_pipeline::get_length() const {
    return internal->pipeline.size();
}

audio_config const& audio_pipeline::get_audio_config() const {
    return internal->config;
}

}