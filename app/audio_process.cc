#include "audio_process.hh"
#include <algorithm>
#include <queue>
#include <tuple>
#include <mutex>
#include <limits>
#include <soundio/soundio.h>
#include <iterator>
#include "startup_parameters.hh"

namespace bzzt {

namespace {

auto audio_process_instance_count {0};
auto channel_read_pos {static_cast<unsigned int>(std::numeric_limits<unsigned int>::max())};
std::vector<float> empty_buffer;

}

struct audio_process::impl {
    impl() :
        audio_instance{nullptr},
        audio_device{nullptr},
        audio_stream{nullptr},
        config{256, 44100},
        pipeline{config},
        incoming_configuration_callbacks{},
        incoming_cleanup_callbacks{},
        config_lock{},
        buffer_left{},
        buffer_right{},
        buffer_left_valid{false},
        buffer_right_valid{false} {}

    SoundIo* audio_instance;
    SoundIoDevice* audio_device;
    SoundIoOutStream* audio_stream;
    audio_config config;
    audio_pipeline pipeline;
    std::queue<std::tuple<void*, void (*)(audio_process::configurer&, void*)>> incoming_configuration_callbacks;
    std::queue<std::tuple<void*, void (*)(void*)>> incoming_cleanup_callbacks;
    std::mutex config_lock;
    audio_pipeline::buffer_handle buffer_left;
    audio_pipeline::buffer_handle buffer_right;
    bool buffer_left_valid;
    bool buffer_right_valid;

    void init() {
        if (!global_audio_enabled()) {
            return;
        }

        std::fill_n(std::back_inserter(empty_buffer), config.buffer_size, 0.0f);

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
        pipeline.reset_all_buffers();

        pipeline.execute();

        while (!config_lock.try_lock()) {}
        configurer _configurer {this};

        // TODO: Call cleanup callback in a different thread.
        while (incoming_configuration_callbacks.size() > 0) {
            auto& configuration_data {incoming_configuration_callbacks.front()};
            auto& cleanup_data {incoming_cleanup_callbacks.front()};
            std::get<1>(configuration_data)(_configurer, std::get<0>(configuration_data));
            std::get<1>(cleanup_data)(std::get<0>(cleanup_data));
            incoming_configuration_callbacks.pop();
            incoming_cleanup_callbacks.pop();
        }

        config_lock.unlock();
    }

    std::vector<float> const& get_channel(unsigned int index) const {
        if (index == 0 && buffer_left_valid) {
            return pipeline.get_buffer(buffer_left);
        } else if (index == 1 && buffer_right_valid) {
            return pipeline.get_buffer(buffer_right);
        } else {
            return empty_buffer;
        }
    }

    audio_config const& get_audio_config() const {
        return config;
    }

private:
    static void audio_callback(SoundIoOutStream* stream, int frame_count_min, int frame_count_max) {
        (void) frame_count_max;
        auto renderer {static_cast<audio_process::impl*>(stream->userdata)};

        auto const& config {renderer->get_audio_config()};

        // Set initial channel read position on first time running this function.
        if (channel_read_pos == std::numeric_limits<unsigned int>::max()) {
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

audio_process::configurer::configurer(audio_process::impl* internals) : audio_process_internals{internals} {}

void audio_process::configurer::set_left_channel_buffer(audio_pipeline::buffer_handle bhandle) {
    audio_process_internals->buffer_left = bhandle;
    audio_process_internals->buffer_left_valid = true;
}

void audio_process::configurer::set_right_channel_buffer(audio_pipeline::buffer_handle bhandle) {
    audio_process_internals->buffer_right = bhandle;
    audio_process_internals->buffer_right_valid = true;
}

audio_pipeline& audio_process::configurer::get_pipeline() const {
    return audio_process_internals->pipeline;
}

audio_process::audio_process() : internal{new impl} {
    audio_process_instance_count += 1;
    if (audio_process_instance_count > 1) {
        throw audio_exception {"audio_process constructor failed, an instance has already been created"};
    }
    internal->init();
}

audio_process::audio_process(audio_process&& other) : internal{other.internal} {
    other.internal = nullptr;
}

audio_process& audio_process::operator=(audio_process&& other) {
    internal = other.internal;
    other.internal = nullptr;
    return *this;
}

audio_process::~audio_process() {
    if (internal) {
        internal->suicide();
        delete internal;
    }
}

void audio_process::configure(void* payload, void (*configure_callback)(audio_process::configurer& process_configurer, void* payload), void (*cleanup_callback)(void* payload)) {
    while (!internal->config_lock.try_lock()) {}
    internal->incoming_configuration_callbacks.push({payload, configure_callback});
    internal->incoming_cleanup_callbacks.push({payload, cleanup_callback});
    internal->config_lock.unlock();
}

}