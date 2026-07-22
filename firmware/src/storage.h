#pragma once
#include "config.h"
#include "lora_mesh.h"
#include <vector>

namespace Storage {
    void             init();
    void             save(const Msg& msg);
    void             tick();              // call from loop() — flushes to flash if dirty
    std::vector<Msg> get(int limit = MSG_MAX);
    void             clear();
    size_t           usedBytes();
    size_t           totalBytes();
}
