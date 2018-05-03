#include "audio_generators.hh"

#include <map>
#include <functional>
#include <cstring>

namespace bzzt {

namespace {

std::map<audio_generator_type, std::function<audio_generator_interface const&()>> interfaces;

template<audio_generator_type Type>
void populate_interface_map() {
    interfaces[Type] = get_audio_generator_interface_impl<Type>;
    populate_interface_map<static_cast<audio_generator_type>(static_cast<unsigned int>(Type) + 1)>();
}

template<>
void populate_interface_map<audio_generator_type::SIZE>() {}

void populate_interfaces() {
    if (interfaces.size() == 0) {
        populate_interface_map<audio_generator_type::ADD>(); // NOTE: Always make sure that the audio generator type is the first one in the enum.
    }
}

}

audio_generator_interface const& get_audio_generator_interface(audio_generator_type type) {
    populate_interfaces();
    return interfaces[type]();
}

audio_generator_type get_audio_generator_type_by_id(const char* id) {
    populate_interfaces();
    for (auto const& interface: interfaces) {
        if (!strcmp(id, interface.second().id())) {
            return interface.first;
        }
    }
    return audio_generator_type::INVALID;
}

}