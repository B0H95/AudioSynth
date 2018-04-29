#pragma once

#include <exception>
#include <string>
#include "audio_pipeline.hh"
#include "audio_config.hh"

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
    audio_process();
    audio_process(audio_process const& other) = delete;
    audio_process(audio_process&& other) = delete;
    audio_process& operator=(audio_process const& other) = delete;
    audio_process& operator=(audio_process&& other) = delete;
    ~audio_process();

    // TODO: soundio_wait_events(audio_instance), what do?

private:
    struct impl;
    impl* internal;
};

}