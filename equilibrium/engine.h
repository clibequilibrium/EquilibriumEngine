#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <cr.h>
#include "equilibrium_defines.h"

typedef struct engine_t {
    void *world;
    bool  running;
} engine_t;

CR_EXPORT
engine_t engine_init(int32_t num_threads, bool enable_rest);

CR_EXPORT
bool engine_update(engine_t *engine);

#endif