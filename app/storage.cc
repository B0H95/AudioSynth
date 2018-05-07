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

struct pipeline_section_step {
    std::string generator_type;
    std::vector<unsigned int> input_buffer_ids;
    std::vector<unsigned int> output_buffer_ids;
    std::vector<float> parameters;
};

struct output_section_step {
    std::string channel_name;
    unsigned int buffer_id;
};

struct pipeline_config_payload {
    std::vector<pipeline_section_step> pipeline_section;
    std::vector<output_section_step> output_section;
    std::map<unsigned int, audio_pipeline::buffer_handle> buffer_id_to_handle;
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

std::unique_ptr<audio_process> load_pipeline_from_file(std::unique_ptr<audio_process> aprocess, std::string const& path) {
    auto file_contents {get_raw_file_contents(path)};
    if (file_contents.size() == 0) {
        return aprocess;
    }

    auto sections {parse_sections(file_contents)};
    if (!section_exists(sections, "pipeline") || !section_exists(sections, "output")) {
        return aprocess;
    }

    auto const& pipeline_section_raw {get_section(sections, "pipeline")};
    auto const& output_section_raw {get_section(sections, "output")};

    auto pipeline_lines {parse_lines(pipeline_section_raw)};
    auto output_lines {parse_lines(output_section_raw)};

    // =====================================================================
    // ======================= Parse file contents =========================
    // =====================================================================
    auto payload {new pipeline_config_payload};

    std::for_each(std::begin(pipeline_lines), std::end(pipeline_lines), [&](std::string const& line) {
        auto splited_line {parse_whitespace_separated_values(line)};

        // Line format: <generator type> <input buffer id> <output buffer id> <parameters>.
        if (splited_line.size() != 4) {
            return;
        }

        auto& generator_type       {splited_line[0]};
        auto& input_buffers_raw  {splited_line[1]};
        auto& output_buffers_raw {splited_line[2]};
        auto& parameters_raw       {splited_line[3]};

        // These three values should be surrounded by parentheses, so check size accordingly.
        if (input_buffers_raw.size() < 3 || output_buffers_raw.size() < 3 || parameters_raw.size() < 3) {
            return;
        }

        // Remove parentheses.
        input_buffers_raw  = input_buffers_raw.substr  (1, input_buffers_raw.size()  - 2);
        output_buffers_raw = output_buffers_raw.substr (1, output_buffers_raw.size() - 2);
        parameters_raw     = parameters_raw.substr     (1, parameters_raw.size()     - 2);

        payload->pipeline_section.push_back({});
        payload->pipeline_section.back().generator_type = generator_type;

        auto input_buffer_list_raw {parse_whitespace_separated_values(input_buffers_raw)};
        auto& input_buffer_list {payload->pipeline_section.back().input_buffer_ids};
        std::transform(std::begin(input_buffer_list_raw), std::end(input_buffer_list_raw), std::back_inserter(input_buffer_list), [](std::string const& input_buffer_id_raw){
            return parse_float(input_buffer_id_raw);
        });

        auto output_buffer_list_raw {parse_whitespace_separated_values(output_buffers_raw)};
        auto& output_buffer_list {payload->pipeline_section.back().output_buffer_ids};
        std::transform(std::begin(output_buffer_list_raw), std::end(output_buffer_list_raw), std::back_inserter(output_buffer_list), [](std::string const& output_buffer_id_raw){
            return parse_float(output_buffer_id_raw);
        });

        auto parameter_list_raw {parse_whitespace_separated_values(parameters_raw)};
        auto& parameter_list {payload->pipeline_section.back().parameters};
        std::transform(std::begin(parameter_list_raw), std::end(parameter_list_raw), std::back_inserter(parameter_list), [](std::string const& parameter_raw){
            return parse_float(parameter_raw);
        });
    });

    std::for_each(std::begin(output_lines), std::end(output_lines), [&](std::string const& line) {
        auto splited_line {parse_whitespace_separated_values(line)};

        // Line format: <channel name> <buffer id>
        if (splited_line.size() != 2) {
            return;
        }

        auto& channel_name  {splited_line[0]};
        auto& buffer_id_raw {splited_line[1]};

        auto buffer_id {parse_unsigned_int(buffer_id_raw)};

        payload->output_section.push_back({channel_name, buffer_id});
    });

    std::for_each(std::begin(payload->pipeline_section), std::end(payload->pipeline_section), [&](pipeline_section_step const& step){
        for (auto const& id: step.input_buffer_ids) {
            payload->buffer_id_to_handle[id] = {};
        }
        for (auto const& id: step.output_buffer_ids) {
            payload->buffer_id_to_handle[id] = {};
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

        for (auto const& step : payload->pipeline_section) {
            auto generator_type {get_audio_generator_type_by_id(step.generator_type.c_str())};
            if (generator_type == audio_generator_type::INVALID) {
                continue;
            }

            auto const& generator_interface {get_audio_generator_interface(generator_type)};
            auto input_buffer_count {generator_interface.get_properties().inputs};
            auto output_buffer_count {generator_interface.get_properties().outputs};
            if (step.input_buffer_ids.size() != input_buffer_count || step.output_buffer_ids.size() != output_buffer_count) {
                continue;
            }

            auto generator = pipeline.add_generator_back(generator_type);

            for (unsigned int i {0}; i < step.input_buffer_ids.size(); ++i) {
                auto input_buffer {payload->buffer_id_to_handle[step.input_buffer_ids[i]]};
                pipeline.set_generator_input(generator, i, input_buffer);
            }

            for (unsigned int i {0}; i < step.output_buffer_ids.size(); ++i) {
                auto output_buffer {payload->buffer_id_to_handle[step.output_buffer_ids[i]]};
                pipeline.set_generator_output(generator, i, output_buffer);
            }

            for (unsigned int i {0}; i < step.parameters.size(); ++i) {
                pipeline.set_generator_parameter(generator, i, step.parameters[i]);
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