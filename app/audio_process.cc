#include "audio_process.hh"
#include <algorithm>
#include <soundio/soundio.h>
#include "startup_parameters.hh"

namespace bzzt {

namespace {

auto left_channel {std::vector<float>{}};
auto right_channel {std::vector<float>{}};
auto channel_read_pos {static_cast<unsigned int>(0)};

}

struct audio_process::impl {
    impl() :
        audio_instance{nullptr},
        audio_device{nullptr},
        audio_stream{nullptr},
        config{256, 44100},
        pipeline{config} {}

    SoundIo* audio_instance;
    SoundIoDevice* audio_device;
    SoundIoOutStream* audio_stream;
    audio_config config;
    audio_pipeline pipeline;

    void init() {
        if (!global_audio_enabled()) {
            return;
        }

        create_example_audio_configuration();

        audio_instance = soundio_create();
        if (!audio_instance) {
            suicide_violently("soundio_create failed");
        }
        auto error {soundio_connect_backend(audio_instance, SoundIoBackendAlsa)};
        if (error) {
            suicide_violently("soundio_connect failed, " + std::string{ soundio_strerror(error) });
        }
        soundio_flush_events(audio_instance);
        auto device_index {soundio_default_output_device_index(audio_instance)};
        if (device_index < 0) {
            suicide_violently("soundio_default_output_device_index failed, no output device found");
        }
        audio_device = soundio_get_output_device(audio_instance, device_index);
        if (!audio_device) {
            suicide_violently("soundio_get_output_device failed");
        }
        audio_stream = soundio_outstream_create(audio_device);
        if (!audio_stream) {
            suicide_violently("soundio_outstream_create failed");
        }
        audio_stream->format = SoundIoFormatFloat32NE;
        audio_stream->write_callback = audio_callback;
        audio_stream->sample_rate = config.sample_rate;
        audio_stream->userdata = static_cast<void*>(this);
        error = soundio_outstream_open(audio_stream);
        if (error) {
            suicide_violently("soundio_outstream_open failed, " + std::string{ soundio_strerror(error) });
        }
        if (audio_stream->layout_error) {
            suicide_violently("soundio_outstream_open failed, unable to set channel layout");
        }
        error = soundio_outstream_start(audio_stream);
        if (error) {
            suicide_violently("soundio_outstream_start failed, " + std::string{ soundio_strerror(error) });
        }
    }

    void suicide() {
        if (audio_stream) {
            soundio_outstream_destroy(audio_stream);
            audio_stream = nullptr;
        }
        if (audio_device) {
            soundio_device_unref(audio_device);
            audio_device = nullptr;
        }
        if (audio_instance) {
            soundio_destroy(audio_instance);
            audio_instance = nullptr;
        }
    }

    void suicide_violently(std::string const& message) {
        suicide();
        throw audio_exception {message};
    }

    void render() {
        pipeline.set_buffer(buffer_left, zero_buffer);
        pipeline.set_buffer(buffer_right, zero_buffer);

        pipeline.execute();
    }

    std::vector<float> const& get_channel(unsigned int index) const {
        if (index == 0) {
            return pipeline.get_buffer(buffer_left);
        } else {
            return pipeline.get_buffer(buffer_right);
        }
    }

    audio_config const& get_audio_config() const {
        return config;
    }

    // Stuff used for example audio output.

    std::vector<float> zero_buffer {};
    audio_pipeline::buffer_handle buffer_left {};
    audio_pipeline::buffer_handle buffer_right {};

    audio_pipeline::generator_handle gen1;
    audio_pipeline::generator_handle gen2;
    audio_pipeline::generator_handle gen3;
    audio_pipeline::generator_handle gen4;

    void create_example_audio_configuration() {
        // Guard against calling this function multiple times.
        static auto called {false};
        if (called) {
            return;
        }
        called = true;

        zero_buffer.resize(config.buffer_size);
        for (auto& sample : zero_buffer) {
            sample = 0.0f;
        }

        buffer_left = pipeline.add_buffer();
        buffer_right = pipeline.add_buffer();

        gen1 = pipeline.add_generator_back(bzzt::audio_generator_type::SINE_WAVE);
        gen2 = pipeline.add_generator_back(bzzt::audio_generator_type::SINE_WAVE);
        gen3 = pipeline.add_generator_back(bzzt::audio_generator_type::SINE_WAVE);
        gen4 = pipeline.add_generator_back(bzzt::audio_generator_type::SINE_WAVE);

        pipeline.set_generator_parameter(gen1, 1, 440.0f);
        pipeline.set_generator_parameter(gen2, 1, 880.0f);
        pipeline.set_generator_parameter(gen3, 1, 440.0f);
        pipeline.set_generator_parameter(gen4, 1, 880.0f);

        pipeline.set_generator_parameter(gen1, 0, 0.25f);
        pipeline.set_generator_parameter(gen2, 0, 0.25f);
        pipeline.set_generator_parameter(gen3, 0, 0.25f);
        pipeline.set_generator_parameter(gen4, 0, 0.25f);

        pipeline.set_generator_input_output(gen1, buffer_left, buffer_left);
        pipeline.set_generator_input_output(gen2, buffer_left, buffer_left);
        pipeline.set_generator_input_output(gen3, buffer_right, buffer_right);
        pipeline.set_generator_input_output(gen4, buffer_right, buffer_right);
    }

private:
    static void audio_callback(SoundIoOutStream* stream, int frame_count_min, int frame_count_max) {
        (void) frame_count_max;
        auto renderer {static_cast<audio_process::impl*>(stream->userdata)};

        auto const& config {renderer->get_audio_config()};
        if (left_channel.size() != config.buffer_size || right_channel.size() != config.buffer_size) {
            left_channel.resize(config.buffer_size);
            right_channel.resize(config.buffer_size);
            std::fill(std::begin(left_channel), std::end(left_channel), 0.0f);
            std::fill(std::begin(right_channel), std::end(right_channel), 0.0f);
            channel_read_pos = config.buffer_size;
        }

        auto layout {&stream->layout};
        auto areas {static_cast<SoundIoChannelArea*>(nullptr)};
        auto frames_rendered {0};

        while (frames_rendered <= frame_count_min) {
            auto frame_count {static_cast<int>(config.buffer_size)};
            if (soundio_outstream_begin_write(stream, &areas, &frame_count)) {
                throw audio_exception {"soundio_outstream_begin_write failed"};
            }
            for (auto i {0}; i < frame_count; ++i) {
                if (channel_read_pos == config.buffer_size) {
                    channel_read_pos = 0;
                    renderer->render();
                }
                for (auto c {0}; c < layout->channel_count; ++c) {
                    auto ptr {(float*)(areas[c].ptr + areas[c].step * i)};
                    *ptr = renderer->get_channel(c)[channel_read_pos];
                }
                channel_read_pos += 1;
            }
            if (soundio_outstream_end_write(stream)) {
                throw audio_exception {"soundio_outstream_end_write failed"};
            }
            frames_rendered += frame_count;
        }
    }
};

audio_process::audio_process() : internal{new impl} {
    internal->init();
}

audio_process::~audio_process() {
    internal->suicide();
    delete internal;
}

}