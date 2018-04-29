#pragma once

#include <string>
#include <exception>

namespace bzzt {

struct window_exception : public std::exception {
    window_exception(std::string const& what) :
        message("Window exception: " + what) {}

    virtual char const* what() const noexcept {
        return message.data();
    }

private:
    std::string message;
};

struct window {
    window();
    window(window const& other) = delete;
    window(window&& other) = delete;
    window& operator=(window const& other) = delete;
    window& operator=(window&& other) = delete;
    ~window();

    void update_screen();
    bool should_close() const;
    void handle_incoming_events();
    void close();

private:
    struct impl;
    impl* internal;
};

}