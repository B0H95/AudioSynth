#ifndef IO_AUDIO_H
#define IO_AUDIO_H

#include <stdbool.h>

bool start_audio_output(void callback(float*, unsigned int, void*), void* data);

void stop_audio_output();

#endif
