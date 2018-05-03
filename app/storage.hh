#pragma once

#include <string>
#include <memory>
#include "audio_process.hh"

namespace bzzt {

std::unique_ptr<audio_process> load_pipeline_from_file(std::unique_ptr<audio_process> aprocess, std::string const& path);

}