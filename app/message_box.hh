#pragma once

#include <string>
#include <queue>

namespace bzzt {

struct message_box {
    void push_error(std::string const& msg);
    std::string pop();
    unsigned int length() const;

private:
    std::queue<std::string> messages;
};

}