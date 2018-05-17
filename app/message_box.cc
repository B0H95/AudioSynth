#include <iostream>
#include "message_box.hh"

namespace bzzt {

void message_box::push_error(std::string const& msg) {
    messages.push("ERROR: " + msg);
}

std::string message_box::pop() {
    auto msg {messages.front()};
    messages.pop();
    return msg;
}

unsigned int message_box::length() const {
    return messages.size();
}

}