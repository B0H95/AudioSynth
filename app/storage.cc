#include "storage.hh"

#include <vector>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <memory>
#include <map>
#include "parsers.hh"
#include "audio_pipeline.hh"

namespace bzzt {

namespace {

struct generator_section_step {
    std::string generator_code;
};

struct pipeline_step_input_parameter {
    bool is_buffer;
    union {
        unsigned int buffer_id;
        float value;
    };
};

struct pipeline_step_output_parameter {
    unsigned int buffer_id;
};

struct pipeline_section_step {
    std::string generator_type;
    std::vector<pipeline_step_input_parameter> input_parameters;
    std::vector<pipeline_step_output_parameter> output_parameters;
};

struct output_section_step {
    std::string channel_name;
    unsigned int buffer_id;
};

struct pipeline_config_payload {
    std::vector<generator_section_step> generator_section;
    std::vector<pipeline_section_step> pipeline_section;
    std::vector<output_section_step> output_section;
    std::map<unsigned int, audio_pipeline::buffer_handle> buffer_id_to_handle;
    std::map<std::string, audio_pipeline::generator_type_handle> generator_type_id_to_impl;
};

const auto FILE_MAX_BYTES {static_cast<unsigned int>(1024*1024)};
const auto RESERVATION_SIZE {static_cast<unsigned int>(1024*8)};

std::string get_raw_file_contents(std::string const& path) {
    auto file {std::ifstream{path}};
    if (file.fail()) {
        return "";
    }

    file.seekg(0, file.end);
    auto file_length {file.tellg()};
    file.seekg(0, file.beg);
    if (file_length > FILE_MAX_BYTES) {
        return "";
    }

    auto raw {std::string{}};
    raw.reserve(RESERVATION_SIZE);
    raw.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

    return raw;
}

bool section_exists(std::vector<std::string> const& parsed_sections, std::string const& section) {
    for (unsigned int i {0}; i < parsed_sections.size(); i += 2) {
        if (parsed_sections[i] == section && parsed_sections.size() > i + 1) {
            return true;
        }
    }
    return false;
}

std::string const& get_section(std::vector<std::string> const& parsed_sections, std::string const& section) {
    for (unsigned int i {0}; i < parsed_sections.size(); i += 2) {
        if (parsed_sections[i] == section && parsed_sections.size() > i + 1) {
            return parsed_sections[i + 1];
        }
    }
    return parsed_sections[0];
}

}

std::unique_ptr<audio_process> load_pipeline_from_file(std::unique_ptr<audio_process> aprocess, std::string const& path, message_box& msg_box) {
    auto file_contents {get_raw_file_contents(path)};
    if (file_contents.size() == 0) {
        msg_box.push_error("Pipeline config file " + path + " could not be loaded");
        return aprocess;
    }

    auto sections {parse_sections(file_contents)};
    auto bad_sections {false};
    if (!section_exists(sections, "pipeline")) {
        msg_box.push_error("Pipeline section not specified in " + path);
        bad_sections = true;
    }
    if (!section_exists(sections, "output")) {
        msg_box.push_error("Output section not specified in " + path);
        bad_sections = true;
    }
    if (!section_exists(sections, "generators")) {
        msg_box.push_error("Generator section not specified in " + path);
        bad_sections = true;
    }
    if (bad_sections) {
        return aprocess;
    }

    auto const& generator_section_raw {get_section(sections, "generators")};
    auto const& pipeline_section_raw  {get_section(sections, "pipeline")};
    auto const& output_section_raw    {get_section(sections, "output")};

    auto generator_lines {parse_lines(generator_section_raw)};
    auto pipeline_lines  {parse_lines(pipeline_section_raw)};
    auto output_lines    {parse_lines(output_section_raw)};

    // =====================================================================
    // ======================= Parse file contents =========================
    // =====================================================================
    auto payload {new pipeline_config_payload};
    std::for_each(std::begin(generator_lines), std::end(generator_lines), [&](std::string const& line) {
        if (line.size() <= 2 || line[0] != '"' || line[line.size() - 1] != '"') {
            msg_box.push_error("Generator filenames need to be enclosed in double-quotes");
            return;
        }
        auto filename {line.substr(1, line.size() - 2)};
        auto code {get_raw_file_contents(filename)};
        if (code.size() == 0) {
            msg_box.push_error("Generator file " + filename + " could not be loaded");
            return;
        }
        payload->generator_section.push_back({code});
    });

    auto current_line {0};
    std::for_each(std::begin(pipeline_lines), std::end(pipeline_lines), [&](std::string const& line) {
        ++current_line;
        auto splited_line {parse_whitespace_separated_values(line)};

        // Line format: <generator type> <input parameters> <output parameters>.
        if (splited_line.size() != 3) {
            msg_box.push_error("At pipeline step " + std::to_string(current_line) + ": Required parameters not specified (generator name, inputs and outputs)");
            return;
        }

        auto& generator_type        {splited_line[0]};
        auto& input_parameters_raw  {splited_line[1]};
        auto& output_parameters_raw {splited_line[2]};

        // These two values should be surrounded by parentheses, so check size accordingly.
        auto bad_inputs_outputs {false};
        if (input_parameters_raw.size() < 3 || input_parameters_raw[0] != '(' || input_parameters_raw[input_parameters_raw.size() - 1] != ')') {
            msg_box.push_error("At pipeline step " + std::to_string(current_line) + ": Inputs need to be surrounded by parentheses");
            bad_inputs_outputs = true;
        }
        if (output_parameters_raw.size() < 3 || output_parameters_raw[0] != '(' || output_parameters_raw[output_parameters_raw.size() - 1] != ')') {
            msg_box.push_error("At pipeline step " + std::to_string(current_line) + ": Outputs need to be surrounded by parentheses");
            bad_inputs_outputs = true;
        }
        if (bad_inputs_outputs) {
            return;
        }

        // Remove parentheses.
        input_parameters_raw  = input_parameters_raw.substr  (1, input_parameters_raw.size()  - 2);
        output_parameters_raw = output_parameters_raw.substr (1, output_parameters_raw.size() - 2);

        payload->pipeline_section.push_back({});
        payload->pipeline_section.back().generator_type = generator_type;

        auto input_parameter_list_raw {parse_whitespace_separated_values(input_parameters_raw)};
        auto& input_parameter_list {payload->pipeline_section.back().input_parameters};
        std::transform(std::begin(input_parameter_list_raw), std::end(input_parameter_list_raw), std::back_inserter(input_parameter_list), [](std::string const& input_parameter_raw){
            pipeline_step_input_parameter param;
            if (input_parameter_raw[0] == '#' && input_parameter_raw.size() >= 2) {
                param.is_buffer = true;
                param.buffer_id = parse_unsigned_int(input_parameter_raw.substr(1));
            } else {
                param.is_buffer = false;
                param.value = parse_float(input_parameter_raw);
            }
            return param;
        });

        auto output_buffer_list_raw {parse_whitespace_separated_values(output_parameters_raw)};
        auto& output_buffer_list {payload->pipeline_section.back().output_parameters};
        std::for_each(std::begin(output_buffer_list_raw), std::end(output_buffer_list_raw), [&](std::string const& output_parameter_raw){
            pipeline_step_output_parameter param;
            if (output_parameter_raw[0] == '#' && output_parameter_raw.size() >= 2) {
                param.buffer_id = parse_unsigned_int(output_parameter_raw.substr(1));
                output_buffer_list.push_back(param);
            }
        });
    });

    std::for_each(std::begin(output_lines), std::end(output_lines), [&](std::string const& line) {
        auto splited_line {parse_whitespace_separated_values(line)};

        // Line format: <channel name> <buffer id>
        if (splited_line.size() != 2) {
            msg_box.push_error("Outputs need to be defined by a channel name and a buffer id");
            return;
        }

        auto& channel_name  {splited_line[0]};
        auto& buffer_id_raw {splited_line[1]};

        auto buffer_id {parse_unsigned_int(buffer_id_raw)};

        payload->output_section.push_back({channel_name, buffer_id});
    });

    if (msg_box.length() > 0) {
        delete payload;
        return aprocess;
    }

    std::for_each(std::begin(payload->pipeline_section), std::end(payload->pipeline_section), [&](pipeline_section_step const& step){
        for (auto const& param: step.input_parameters) {
            if (param.is_buffer) {
                payload->buffer_id_to_handle[param.buffer_id] = {};
            }
        }
        for (auto const& param: step.output_parameters) {
            payload->buffer_id_to_handle[param.buffer_id] = {};
        }
    });

    // =====================================================================
    // ======================== Transform pipeline =========================
    // =====================================================================
    aprocess->configure(static_cast<void*>(payload), [](audio_process::configurer& process_configurer, void* p){
        auto payload {static_cast<pipeline_config_payload*>(p)};
        auto& pipeline {process_configurer.get_pipeline()};

        for (auto& id_to_handle : payload->buffer_id_to_handle) {
            id_to_handle.second = pipeline.add_buffer();
        }

        for (auto const& generator_type : payload->generator_section) {
            auto handle {pipeline.add_generator_type(generator_type.generator_code)};
            if (pipeline.generator_type_is_valid(handle)) {
                auto const& generator_interface {pipeline.get_generator_interface(handle)};
                auto type_id {generator_interface.id()};
                payload->generator_type_id_to_impl[type_id] = handle;
            }
        }

        for (auto const& step : payload->pipeline_section) {
            auto it {payload->generator_type_id_to_impl.find(step.generator_type)};
            if (it == payload->generator_type_id_to_impl.end()) {
                continue;
            }
            auto generator_type {it->second};

            auto const& generator_interface {pipeline.get_generator_interface(it->second)};
            if (step.input_parameters.size() != generator_interface.input_count() || step.output_parameters.size() != generator_interface.output_count()) {
                continue;
            }

            auto generator = pipeline.add_generator_back(generator_type);

            for (unsigned int i {0}; i < step.input_parameters.size(); ++i) {
                auto input_param {step.input_parameters[i]};
                if (input_param.is_buffer) {
                    auto buffer_handle {payload->buffer_id_to_handle[input_param.buffer_id]};
                    pipeline.set_generator_input_buffer(generator, i, buffer_handle);
                } else {
                    pipeline.set_generator_input_value(generator, i, input_param.value);
                }
            }

            for (unsigned int i {0}; i < step.output_parameters.size(); ++i) {
                auto output_param {step.output_parameters[i]};
                auto buffer_handle {payload->buffer_id_to_handle[output_param.buffer_id]};
                pipeline.set_generator_output_buffer(generator, i, buffer_handle);
            }
        }

        for (auto const& step: payload->output_section) {
            if (payload->buffer_id_to_handle.find(step.buffer_id) == payload->buffer_id_to_handle.end()) {
                continue;
            }

            if (step.channel_name == "left") {
                process_configurer.set_left_channel_buffer(payload->buffer_id_to_handle[step.buffer_id]);
            } else if (step.channel_name == "right") {
                process_configurer.set_right_channel_buffer(payload->buffer_id_to_handle[step.buffer_id]);
            }
        }
    }, [](void *p){
        auto payload {static_cast<pipeline_config_payload*>(p)};
        delete payload;
    });

    return aprocess;
}

}