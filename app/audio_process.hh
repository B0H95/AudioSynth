#pragma once

#include <exception>
#include <string>
#include "audio_config.hh"
#include "audio_pipeline.hh"

namespace bzzt {

struct audio_exception : public std::exception {
    audio_exception(std::string const& what) :
        message("Audio exception: " + what) {}

    virtual char const* what() const noexcept {
        return message.data();
    }

private:
    std::string message;
};

struct audio_process {
private:
    struct impl;
    impl* internal;

public:
    struct configurer {
        void set_left_channel_buffer(audio_pipeline::buffer_handle bhandle);
        void set_right_channel_buffer(audio_pipeline::buffer_handle bhahdle);
        audio_pipeline& get_pipeline() const;

    private:
        friend struct audio_process;
        configurer(impl* internals);
        impl* audio_process_internals;
    };

    audio_process();
    audio_process(audio_process const& other) = delete;
    audio_process(audio_process&& other);
    audio_process& operator=(audio_process const& other) = delete;
    audio_process& operator=(audio_process&& other);
    ~audio_process();

    void configure(void* payload, void (*configure_callback)(configurer& process_configurer, void* payload), void (*cleanup_callback)(void* payload));

    // TODO: soundio_wait_events(audio_instance), what do?
};

}