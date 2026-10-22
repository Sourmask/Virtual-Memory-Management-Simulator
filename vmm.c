#include "vmm.h"

#include <stdlib.h>
#include <string.h>

const char *vmm_algorithm_name(VMMAlgorithm algorithm)
{
    switch (algorithm) {
    case VMM_FIFO:
        return "FIFO";
    case VMM_LRU:
        return "LRU";
    case VMM_OPT:
        return "OPT";
    default:
        return "UNKNOWN";
    }
}

bool vmm_parse_algorithm(const char *name, VMMAlgorithm *algorithm)
{
    if (name == NULL || algorithm == NULL) {
        return false;
    }
    if (strcmp(name, "FIFO") == 0) {
        *algorithm = VMM_FIFO;
    } else if (strcmp(name, "LRU") == 0) {
        *algorithm = VMM_LRU;
    } else if (strcmp(name, "OPT") == 0) {
        *algorithm = VMM_OPT;
    } else {
        return false;
    }
    return true;
}

bool vmm_init(VMMSimulator *simulator, const VMMPage *references,
              size_t reference_count, size_t frame_count,
              VMMAlgorithm algorithm)
{
    if (simulator == NULL) {
        return false;
    }
    memset(simulator, 0, sizeof(*simulator));
    if ((reference_count > 0 && references == NULL) || frame_count == 0 ||
        (algorithm != VMM_FIFO && algorithm != VMM_LRU &&
         algorithm != VMM_OPT)) {
        return false;
    }
    if (frame_count > (size_t)-1 / sizeof(*simulator->frames) ||
        frame_count > (size_t)-1 / sizeof(*simulator->loaded_at)) {
        return false;
    }

    simulator->frames = malloc(frame_count * sizeof(*simulator->frames));
    simulator->occupied = calloc(frame_count, sizeof(*simulator->occupied));
    simulator->loaded_at = calloc(frame_count, sizeof(*simulator->loaded_at));
    simulator->used_at = calloc(frame_count, sizeof(*simulator->used_at));
    if (simulator->frames == NULL || simulator->occupied == NULL ||
        simulator->loaded_at == NULL || simulator->used_at == NULL) {
        vmm_destroy(simulator);
        return false;
    }

    simulator->references = references;
    simulator->reference_count = reference_count;
    simulator->algorithm = algorithm;
    simulator->frame_count = frame_count;
    return true;
}

void vmm_destroy(VMMSimulator *simulator)
{
    if (simulator == NULL) {
        return;
    }
    free(simulator->frames);
    free(simulator->occupied);
    free(simulator->loaded_at);
    free(simulator->used_at);
    memset(simulator, 0, sizeof(*simulator));
}

bool vmm_done(const VMMSimulator *simulator)
{
    return simulator == NULL || simulator->position >= simulator->reference_count;
}

double vmm_hit_ratio(const VMMSimulator *simulator)
{
    size_t accesses;

    if (simulator == NULL) {
        return 0.0;
    }
    accesses = simulator->hits + simulator->faults;
    return accesses == 0 ? 0.0 : (double)simulator->hits / (double)accesses;
}

static size_t vmm_pick_victim(const VMMSimulator *simulator)
{
    size_t i;
    size_t victim = 0;

    if (simulator->algorithm == VMM_FIFO) {
        for (i = 1; i < simulator->frame_count; ++i) {
            if (simulator->loaded_at[i] < simulator->loaded_at[victim]) {
                victim = i;
            }
        }
        return victim;
    }

    if (simulator->algorithm == VMM_LRU) {
        for (i = 1; i < simulator->frame_count; ++i) {
            if (simulator->used_at[i] < simulator->used_at[victim]) {
                victim = i;
            }
        }
        return victim;
    }

    {
        size_t farthest = 0;
        bool farthest_is_never_used = false;

        for (i = 0; i < simulator->frame_count; ++i) {
            size_t next;
            for (next = simulator->position + 1;
                 next < simulator->reference_count; ++next) {
                if (simulator->references[next] == simulator->frames[i]) {
                    break;
                }
            }
            if (next == simulator->reference_count) {
                if (!farthest_is_never_used) {
                    victim = i;
                    farthest_is_never_used = true;
                }
            } else if (!farthest_is_never_used &&
                       (i == 0 || next > farthest)) {
                victim = i;
                farthest = next;
            }
        }
    }
    return victim;
}

bool vmm_step(VMMSimulator *simulator, VMMEvent *event)
{
    size_t frame;
    size_t i;
    VMMPage page;

    if (simulator == NULL || event == NULL || vmm_done(simulator)) {
        return false;
    }

    page = simulator->references[simulator->position];
    event->step = simulator->position + 1;
    event->page = page;
    event->hit = false;
    event->frame = 0;
    event->has_evicted = false;
    event->evicted = 0;

    for (i = 0; i < simulator->frame_count; ++i) {
        if (simulator->occupied[i] && simulator->frames[i] == page) {
            frame = i;
            event->hit = true;
            break;
        }
    }

    if (event->hit) {
        ++simulator->hits;
        simulator->used_at[frame] = event->step;
    } else {
        ++simulator->faults;
        for (frame = 0; frame < simulator->frame_count; ++frame) {
            if (!simulator->occupied[frame]) {
                break;
            }
        }
        if (frame == simulator->frame_count) {
            frame = vmm_pick_victim(simulator);
            event->has_evicted = true;
            event->evicted = simulator->frames[frame];
        }
        simulator->frames[frame] = page;
        simulator->occupied[frame] = true;
        simulator->loaded_at[frame] = event->step;
        simulator->used_at[frame] = event->step;
    }

    event->frame = frame;
    ++simulator->position;
    return true;
}
