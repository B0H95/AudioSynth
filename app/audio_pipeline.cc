#include "audio_pipeline.hh"

#include <map>
#include <vector>
#include <array>
#include <algorithm>
#include <limits.h>
#include <libtcc.h>
#include "audio_generator_interface.hh"

namespace bzzt {

namespace {

const unsigned int MAX_INPUT_PARAMETERS {8};
const unsigned int MAX_OUTPUT_PARAMETERS {8};

struct pipeline_step {
    audio_generator_run_func render_func;

    audio_pipeline::generator_type_handle generator_type;
    unsigned int state_index;

    unsigned int inputs;
    unsigned int outputs;
};

struct audio_generator_impl {
    audio_generator_impl(std::string const& code) {
        TCCState* tcc_state {tcc_new()};
        if (!tcc_state) {
            return;
        }
        tcc_set_output_type(tcc_state, TCC_OUTPUT_MEMORY);
        if (tcc_compile_string(tcc_state, code.c_str()) == -1) {
            tcc_delete(tcc_state);
            return;
        }
        auto memory_size {tcc_relocate(tcc_state, nullptr)};
        if (!memory_size) {
            tcc_delete(tcc_state);
            return;
        }
        build_memory = static_cast<void*>(new char[memory_size]);
        if (!build_memory) {
            tcc_delete(tcc_state);
            return;
        }
        tcc_relocate(tcc_state, build_memory);

        generator_impl.run          = (audio_generator_run_func)          (tcc_get_symbol(tcc_state, "run"));
        generator_impl.init         = (audio_generator_init_func)         (tcc_get_symbol(tcc_state, "init"));
        generator_impl.deinit       = (audio_generator_deinit_func)       (tcc_get_symbol(tcc_state, "deinit"));
        generator_impl.id           = (audio_generator_id_func)           (tcc_get_symbol(tcc_state, "id"));
        generator_impl.size         = (audio_generator_size_func)         (tcc_get_symbol(tcc_state, "size"));
        generator_impl.input_count  = (audio_generator_input_count_func)  (tcc_get_symbol(tcc_state, "input_count"));
        generator_impl.output_count = (audio_generator_output_count_func) (tcc_get_symbol(tcc_state, "output_count"));

        tcc_delete(tcc_state);
    }

    audio_generator_impl(audio_generator_impl const& other) = delete;
    audio_generator_impl& operator=(audio_generator_impl const& other) = delete;

    audio_generator_impl(audio_generator_impl&& other) {
        build_memory = other.build_memory;
        generator_impl = other.generator_impl;
        other.build_memory = nullptr;
    }

    audio_generator_impl& operator=(audio_generator_impl&& other) {
        build_memory = other.build_memory;
        generator_impl = other.generator_impl;
        other.build_memory = nullptr;
        return *this;
    }

    ~audio_generator_impl() {
        if (build_memory) {
            delete[] static_cast<char*>(build_memory);
        }
    }

    bool is_valid() const {
        auto const& x {generator_impl};
        return x.run && x.init && x.deinit && x.id && x.size && x.input_count && x.output_count;
    }

    void* build_memory {nullptr};
    audio_generator_interface generator_impl {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

struct parameter_type {
    bool is_buffer;
    union {
        unsigned int buffer_id;
        float value;
    };
};

}

struct audio_pipeline::impl {
    audio_config config;

    std::vector<pipeline_step> pipeline;
    std::array<std::vector<parameter_type>, MAX_INPUT_PARAMETERS> pipeline_inputs;
    std::array<std::vector<parameter_type>, MAX_OUTPUT_PARAMETERS> pipeline_outputs;

    std::map<audio_pipeline::generator_type_handle, std::vector<char>> state_lists;
    std::map<audio_pipeline::generator_type_handle, std::vector<bool>> state_lists_usage;

    std::vector<std::vector<float>> buffers;
    std::vector<bool> buffer_used;

    std::vector<audio_generator_impl> generator_impls;

    audio_pipeline::generator_type_handle add_generator_type(std::string const& generator_code) {
        audio_generator_impl impl {generator_code};
        if (!impl.is_valid()) {
            return UINT_MAX;
        }
        generator_impls.push_back(std::move(impl));
        return generator_impls.size() - 1;
    }

    audio_pipeline::generator_handle add_generator(audio_pipeline::generator_type_handle type, unsigned int position) {
        if (state_lists.find(type) == state_lists.end()) {
            state_lists[type] = std::vector<char>{};
            state_lists[type].reserve(8192);

            state_lists_usage[type] = std::vector<bool>{};
            state_lists_usage[type].reserve(8192);
        }

        parameter_type new_param;
        new_param.is_buffer = false;
        new_param.value = 0.0f;

        auto& state_list {state_lists[type]};
        auto& state_list_usage {state_lists_usage[type]};

        auto const& generator {generator_impls[type].generator_impl};
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
            auto new_step {pipeline_step {generator.run, type, static_cast<unsigned int>(state_pos), generator.input_count(), generator.output_count()}};
            pipeline.insert(std::begin(pipeline) + position, new_step);
            for (auto& input_buffer_list : pipeline_inputs) {
                input_buffer_list.insert(std::begin(input_buffer_list) + position, new_param);
            }
            for (auto& output_buffer_list : pipeline_outputs) {
                output_buffer_list.insert(std::begin(output_buffer_list) + position, new_param);
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

        auto new_step {pipeline_step {generator.run, type, static_cast<unsigned int>(initial_state_list_size), generator.input_count(), generator.output_count()}};
        pipeline.insert(std::begin(pipeline) + position, new_step);
        for (auto& input_buffer_list : pipeline_inputs) {
            input_buffer_list.insert(std::begin(input_buffer_list) + position, new_param);
        }
        for (auto& output_buffer_list : pipeline_outputs) {
            output_buffer_list.insert(std::begin(output_buffer_list) + position, new_param);
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

        for (auto& inputs : pipeline_inputs) {
            auto param {inputs[handle_pos]};
            inputs.erase(std::begin(inputs) + handle_pos);
            inputs.insert(std::begin(inputs) + pos, param);
        }
        for (auto& outputs : pipeline_inputs) {
            auto param {outputs[handle_pos]};
            outputs.erase(std::begin(outputs) + handle_pos);
            outputs.insert(std::begin(outputs) + pos, param);
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

audio_pipeline::generator_type_handle audio_pipeline::add_generator_type(std::string const& generator_code) {
    return internal->add_generator_type(generator_code);
}

bool audio_pipeline::generator_type_is_valid(audio_pipeline::generator_type_handle handle) const {
    return handle != UINT_MAX;
}

std::string audio_pipeline::get_generator_id(audio_pipeline::generator_type_handle handle) const {
    return internal->generator_impls[handle].generator_impl.id();
}

audio_pipeline::generator_handle audio_pipeline::add_generator_front(audio_pipeline::generator_type_handle type) {
    return internal->add_generator(type, 0);
}

audio_pipeline::generator_handle audio_pipeline::add_generator_before(audio_pipeline::generator_type_handle type, audio_pipeline::generator_handle ghandle) {
    return internal->add_generator(type, internal->get_generator_position(ghandle));
}

audio_pipeline::generator_handle audio_pipeline::add_generator_after(audio_pipeline::generator_type_handle type, audio_pipeline::generator_handle ghandle) {
    return internal->add_generator(type, internal->get_generator_position(ghandle) + 1);
}

audio_pipeline::generator_handle audio_pipeline::add_generator_back(audio_pipeline::generator_type_handle type) {
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
    auto const& generator {internal->generator_impls[generator_type].generator_impl};
    auto generator_size {generator.size()};
    auto generator_index {std::get<1>(handle)};
    internal->state_lists_usage[generator_type][generator_index / generator_size] = false;
    auto state_ptr {static_cast<void*>(&internal->state_lists[generator_type][generator_index])};
    generator.deinit(state_ptr);
    auto new_end {std::remove_if(std::begin(internal->pipeline), std::end(internal->pipeline), [&](pipeline_step const& step) {
        return step.generator_type == generator_type && step.state_index == generator_index;
    })};
    internal->pipeline.erase(new_end, std::end(internal->pipeline));
    for (auto& inputs : internal->pipeline_inputs) {
        inputs.erase(std::begin(inputs) + generator_position);
    }
    for (auto& outputs : internal->pipeline_outputs) {
        outputs.erase(std::begin(outputs) + generator_position);
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

void audio_pipeline::set_generator_input_value(audio_pipeline::generator_handle ghandle, unsigned int input_id, float value) {
    if (input_id >= MAX_INPUT_PARAMETERS) {
        return;
    }
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        auto& step {internal->pipeline[i]};
        if (step.state_index == std::get<1>(ghandle) && step.generator_type == std::get<0>(ghandle)) {
            auto& param {internal->pipeline_inputs[input_id][i]};
            param.value = value;
            param.is_buffer = false;
            return;
        }
    }
}

void audio_pipeline::set_generator_input_buffer(audio_pipeline::generator_handle ghandle, unsigned int input_id, audio_pipeline::buffer_handle bhandle) {
    if (input_id >= MAX_INPUT_PARAMETERS) {
        return;
    }
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        auto& step {internal->pipeline[i]};
        if (step.state_index == std::get<1>(ghandle) && step.generator_type == std::get<0>(ghandle)) {
            auto& param {internal->pipeline_inputs[input_id][i]};
            param.buffer_id = bhandle;
            param.is_buffer = true;
            return;
        }
    }
}

void audio_pipeline::set_generator_output_value(audio_pipeline::generator_handle ghandle, unsigned int output_id, float value) {
    if (output_id >= MAX_OUTPUT_PARAMETERS) {
        return;
    }
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        auto& step {internal->pipeline[i]};
        if (step.state_index == std::get<1>(ghandle) && step.generator_type == std::get<0>(ghandle)) {
            auto& param {internal->pipeline_outputs[output_id][i]};
            param.value = value;
            param.is_buffer = false;
            return;
        }
    }
}

void audio_pipeline::set_generator_output_buffer(audio_pipeline::generator_handle ghandle, unsigned int output_id, audio_pipeline::buffer_handle bhandle) {
    if (output_id >= MAX_OUTPUT_PARAMETERS) {
        return;
    }
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        auto& step {internal->pipeline[i]};
        if (step.state_index == std::get<1>(ghandle) && step.generator_type == std::get<0>(ghandle)) {
            auto& param {internal->pipeline_outputs[output_id][i]};
            param.buffer_id = bhandle;
            param.is_buffer = true;
            return;
        }
    }
}

void audio_pipeline::remove_buffer(audio_pipeline::buffer_handle handle) {
    internal->buffer_used[handle] = false;
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        for (auto& inputs : internal->pipeline_inputs) {
            auto& param {inputs[i]};
            if (param.is_buffer && param.buffer_id == handle) {
                param.is_buffer = false;
                param.value = 0.0f;
            }
        }
        for (auto& outputs : internal->pipeline_outputs) {
            auto& param {outputs[i]};
            if (param.is_buffer && param.buffer_id == handle) {
                param.is_buffer = false;
                param.value = 0.0f;
            }
        }
    }
}

void audio_pipeline::execute() {
    float inputs[MAX_INPUT_PARAMETERS];
    float outputs[MAX_OUTPUT_PARAMETERS];
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {

        auto& step {internal->pipeline[i]};
        auto state {static_cast<void*>(&internal->state_lists[step.generator_type][step.state_index])};

        for (unsigned int sample_id {0}; sample_id < internal->config.buffer_size; ++sample_id) {
            for (unsigned int in {0}; in < step.inputs; ++in) {
                auto& inparam {internal->pipeline_inputs[in][i]};
                if (inparam.is_buffer) {
                    inputs[in] = internal->buffers[inparam.buffer_id][sample_id];
                } else {
                    inputs[in] = inparam.value;
                }
            }

            step.render_func(inputs, outputs, state, internal->config.sample_rate);

            for (unsigned int out {0}; out < step.outputs; ++out) {
                auto& outparam {internal->pipeline_outputs[out][i]};
                if (outparam.is_buffer) {
                    internal->buffers[outparam.buffer_id][sample_id] = outputs[out];
                } else {
                    // TODO: It does not make sense to not write to a buffer. Please fix this.
                }
            }
        }
    }
}

unsigned int audio_pipeline::get_length() const {
    return internal->pipeline.size();
}

audio_config const& audio_pipeline::get_audio_config() const {
    return internal->config;
}

}