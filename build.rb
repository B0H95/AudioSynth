compile_command = %x{clang++ -std=c++17 -Wall -Wextra -pedantic -Iapp/ app/*.cc -ltcc -ldl -lglfw -lsoundio -lGL -lGLU -lGLEW -o build/audiosynth}
puts compile_command
