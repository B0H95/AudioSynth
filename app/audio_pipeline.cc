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

const audio_pipeline::generator_type_handle INVALID_GENERATOR_TYPE_HANDLE {UINT_MAX};
const audio_pipeline::generator_handle      INVALID_GENERATOR_HANDLE      {INVALID_GENERATOR_TYPE_HANDLE, UINT_MAX};

audio_pipeline::generator_type_handle get_generator_type(audio_pipeline::generator_handle const& handle) {
    return std::get<0>(handle);
};

unsigned int get_generator_state_index (audio_pipeline::generator_handle const& handle) {
    return std::get<1>(handle);
}

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

    bool valid() const {
        auto const& x {generator_impl};
        return x.run && x.init && x.deinit && x.id && x.size && x.input_count && x.output_count;
    }

    void* build_memory {nullptr};
    audio_generator_interface generator_impl {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
};

struct generator_input_param {
    generator_input_param() : is_buffer{false} {
        value = 0.0f;
    }

    bool is_buffer;
    union {
        unsigned int buffer_id;
        float value;
    };
};

struct generator_output_param {
    generator_output_param() : buffer_id{UINT_MAX} {}

    unsigned int buffer_id;
};

}

struct audio_pipeline::impl {
    audio_config audio_conf;

    std::vector<pipeline_step> pipeline;
    std::array<std::vector<generator_input_param>,  MAX_INPUT_PARAMETERS>  pipeline_inputs;
    std::array<std::vector<generator_output_param>, MAX_OUTPUT_PARAMETERS> pipeline_outputs;

    std::map<audio_pipeline::generator_type_handle, std::vector<char>> generator_states;
    std::map<audio_pipeline::generator_type_handle, std::vector<bool>> generator_states_occupied;

    std::vector<std::vector<float>> buffers;
    std::vector<bool> buffers_occupied;

    std::vector<audio_generator_impl> generator_implementations;

    audio_pipeline::generator_type_handle add_generator_type(std::string const& generator_code) {
        audio_generator_impl impl {generator_code};
        if (!impl.valid()) {
            return INVALID_GENERATOR_TYPE_HANDLE;
        }
        generator_implementations.push_back(std::move(impl));
        return generator_implementations.size() - 1;
    }

    void init_generator_states_for_type(audio_pipeline::generator_type_handle type) {
        generator_states[type] = std::vector<char>{};
        generator_states[type].reserve(8192);

        generator_states_occupied[type] = std::vector<bool>{};
        generator_states_occupied[type].reserve(8192);
    }

    bool generator_states_inited_for_type(audio_pipeline::generator_type_handle type) const {
        return generator_states.find(type) != generator_states.end();
    }

    audio_pipeline::generator_handle add_generator(audio_pipeline::generator_type_handle type, unsigned int position) {
        if (!generator_states_inited_for_type(type)) {
            init_generator_states_for_type(type);
        }

        auto& states {generator_states[type]};
        auto& states_occupied {generator_states_occupied[type]};
        unsigned int state_position;

        auto const& generator_interface {generator_implementations[type].generator_impl};

        auto vacant_position_iter {std::find(std::begin(states_occupied), std::end(states_occupied), false)};
        if (vacant_position_iter != std::end(states_occupied)) {
            auto vacant_position {std::distance(std::begin(states_occupied), vacant_position_iter)};
            state_position = static_cast<unsigned int>(vacant_position * generator_interface.size());
            auto state_pointer {static_cast<void*>(&states[state_position])};
            if (!generator_interface.init(state_pointer)) {
                return INVALID_GENERATOR_HANDLE;
            }
            *vacant_position_iter = true;
        } else {
            state_position = states.size();
            for (unsigned int i {0}; i < generator_interface.size(); ++i) {
                states.push_back(static_cast<char>(0));
            }
            states_occupied.push_back(true);

            auto state_pointer {static_cast<void*>(&states[state_position])};
            if (!generator_interface.init(state_pointer)) {
                states_occupied.back() = false;
                return INVALID_GENERATOR_HANDLE;
            }
        }

        auto new_pipeline_step {pipeline_step {generator_interface.run, type, state_position, generator_interface.input_count(), generator_interface.output_count()}};
        pipeline.insert(std::begin(pipeline) + position, new_pipeline_step);

        for (auto& inputs : pipeline_inputs) {
            inputs.insert(std::begin(inputs) + position, generator_input_param{});
        }
        for (auto& outputs : pipeline_outputs) {
            outputs.insert(std::begin(outputs) + position, generator_output_param{});
        }

        return {type, state_position};
    }

    unsigned int get_generator_position(audio_pipeline::generator_handle ghandle) {
        auto found_generator_iter {std::find_if(std::begin(pipeline), std::end(pipeline), [&](pipeline_step const& step) {
            return step.generator_type == get_generator_type(ghandle) && step.state_index == get_generator_state_index(ghandle);
        })};
        return found_generator_iter != std::end(pipeline) ? std::distance(std::begin(pipeline), found_generator_iter) : UINT_MAX;
    }

    void move_generator_to_position(audio_pipeline::generator_handle handle, unsigned int position) {
        auto generator_position {get_generator_position(handle)};
        auto step {pipeline[generator_position]};
        pipeline.erase(std::begin(pipeline) + generator_position);
        if (position > pipeline.size()) {
            position = pipeline.size();
        }
        pipeline.insert(std::begin(pipeline) + position, step);

        for (auto& inputs : pipeline_inputs) {
            auto input_param {inputs[generator_position]};
            inputs.erase(std::begin(inputs) + generator_position);
            inputs.insert(std::begin(inputs) + position, input_param);
        }
        for (auto& outputs : pipeline_outputs) {
            auto output_param {outputs[generator_position]};
            outputs.erase(std::begin(outputs) + generator_position);
            outputs.insert(std::begin(outputs) + position, output_param);
        }
    }
};

audio_pipeline::audio_pipeline(audio_config const& config) : internal{new audio_pipeline::impl} {
    internal->audio_conf = config;
    internal->pipeline.reserve(1024);
}

audio_pipeline::~audio_pipeline() {
    delete internal;
}

audio_pipeline::generator_type_handle audio_pipeline::add_generator_type(std::string const& generator_code) {
    return internal->add_generator_type(generator_code);
}

bool audio_pipeline::generator_type_is_valid(audio_pipeline::generator_type_handle handle) const {
    return handle != INVALID_GENERATOR_TYPE_HANDLE;
}

audio_generator_interface const& audio_pipeline::get_generator_interface(audio_pipeline::generator_type_handle handle) const {
    return internal->generator_implementations[handle].generator_impl;
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
    auto generator_type {get_generator_type(handle)};
    auto const& generator_interface {internal->generator_implementations[generator_type].generator_impl};
    auto generator_state_index {get_generator_state_index(handle)};

    internal->generator_states_occupied[generator_type][generator_state_index / generator_interface.size()] = false;

    auto state_pointer {static_cast<void*>(&internal->generator_states[generator_type][generator_state_index])};
    generator_interface.deinit(state_pointer);

    auto pipeline_new_end_iter {std::remove_if(std::begin(internal->pipeline), std::end(internal->pipeline), [&](pipeline_step const& step) {
        return step.generator_type == generator_type && step.state_index == generator_state_index;
    })};
    internal->pipeline.erase(pipeline_new_end_iter, std::end(internal->pipeline));

    for (auto& inputs : internal->pipeline_inputs) {
        inputs.erase(std::begin(inputs) + generator_position);
    }
    for (auto& outputs : internal->pipeline_outputs) {
        outputs.erase(std::begin(outputs) + generator_position);
    }
}

audio_pipeline::buffer_handle audio_pipeline::add_buffer() {
    for (unsigned int i {0}; i < internal->buffers_occupied.size(); ++i) {
        if (!internal->buffers_occupied[i]) {
            internal->buffers_occupied[i] = true;
            return i;
        }
    }
    internal->buffers.push_back(std::vector<float>{});
    internal->buffers.back().resize(internal->audio_conf.buffer_size);
    internal->buffers_occupied.push_back(true);
    return internal->buffers.size() - 1;
}

std::vector<float> const& audio_pipeline::get_buffer(audio_pipeline::buffer_handle handle) const {
    return internal->buffers[handle];
}

void audio_pipeline::set_buffer(buffer_handle handle, std::vector<float> const& new_contents) {
    auto& buffer {internal->buffers[handle]};
    if (buffer.size() != new_contents.size()) {
        return;
    }
    for (unsigned int i {0}; i < buffer.size(); ++i) {
        buffer[i] = new_contents[i];
    }
}

void audio_pipeline::set_generator_input_value(audio_pipeline::generator_handle ghandle, unsigned int input_id, float value) {
    if (input_id >= MAX_INPUT_PARAMETERS) {
        return;
    }
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {
        auto& step {internal->pipeline[i]};
        if (step.state_index == get_generator_state_index(ghandle) && step.generator_type == get_generator_type(ghandle)) {
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
        if (step.state_index == get_generator_state_index(ghandle) && step.generator_type == get_generator_type(ghandle)) {
            auto& param {internal->pipeline_inputs[input_id][i]};
            param.buffer_id = bhandle;
            param.is_buffer = true;
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
        if (step.state_index == get_generator_state_index(ghandle) && step.generator_type == get_generator_type(ghandle)) {
            auto& param {internal->pipeline_outputs[output_id][i]};
            param.buffer_id = bhandle;
            return;
        }
    }
}

void audio_pipeline::delete_buffer(audio_pipeline::buffer_handle handle) {
    internal->buffers_occupied[handle] = false;
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
            param.buffer_id = UINT_MAX;
        }
    }
}

void audio_pipeline::execute() {
    float inputs[MAX_INPUT_PARAMETERS];
    float outputs[MAX_OUTPUT_PARAMETERS];
    for (unsigned int i {0}; i < internal->pipeline.size(); ++i) {

        auto& step {internal->pipeline[i]};
        auto state {static_cast<void*>(&internal->generator_states[step.generator_type][step.state_index])};

        for (unsigned int sample_id {0}; sample_id < internal->audio_conf.buffer_size; ++sample_id) {
            for (unsigned int in {0}; in < step.inputs; ++in) {
                auto& inparam {internal->pipeline_inputs[in][i]};
                if (inparam.is_buffer) {
                    inputs[in] = internal->buffers[inparam.buffer_id][sample_id];
                } else {
                    inputs[in] = inparam.value;
                }
            }

            step.render_func(inputs, outputs, state, internal->audio_conf.sample_rate);

            for (unsigned int out {0}; out < step.outputs; ++out) {
                auto& outparam {internal->pipeline_outputs[out][i]};
                internal->buffers[outparam.buffer_id][sample_id] = outputs[out];
            }
        }
    }
}

unsigned int audio_pipeline::get_length() const {
    return internal->pipeline.size();
}

audio_config const& audio_pipeline::get_audio_config() const {
    return internal->audio_conf;
}

}