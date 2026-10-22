#include "vmm.h"

#include <stddef.h>

static VMMSimulator simulator;
static VMMEvent last_event;
static bool initialized;

int vmm_web_init(const VMMPage *references, size_t reference_count,
                 size_t frame_count, int algorithm)
{
    if (initialized) {
        vmm_destroy(&simulator);
        initialized = false;
    }

    if (algorithm < VMM_FIFO || algorithm > VMM_OPT ||
        !vmm_init(&simulator, references, reference_count, frame_count,
                  (VMMAlgorithm)algorithm)) {
        return 0;
    }
    initialized = true;
    return 1;
}

void vmm_web_destroy(void)
{
    if (initialized) {
        vmm_destroy(&simulator);
        initialized = false;
    }
}

int vmm_web_step(void)
{
    return initialized && vmm_step(&simulator, &last_event);
}

int vmm_web_done(void)
{
    return !initialized || vmm_done(&simulator);
}

size_t vmm_web_position(void)
{
    return initialized ? simulator.position : 0;
}

size_t vmm_web_hits(void)
{
    return initialized ? simulator.hits : 0;
}

size_t vmm_web_faults(void)
{
    return initialized ? simulator.faults : 0;
}

double vmm_web_hit_ratio(void)
{
    return initialized ? vmm_hit_ratio(&simulator) : 0.0;
}

double vmm_web_event_page(void)
{
    return (double)last_event.page;
}

int vmm_web_event_hit(void)
{
    return last_event.hit;
}

size_t vmm_web_event_frame(void)
{
    return last_event.frame;
}

int vmm_web_event_has_evicted(void)
{
    return last_event.has_evicted;
}

double vmm_web_event_evicted(void)
{
    return (double)last_event.evicted;
}

size_t vmm_web_frame_count(void)
{
    return initialized ? simulator.frame_count : 0;
}

int vmm_web_frame_occupied(size_t frame)
{
    return initialized && frame < simulator.frame_count &&
           simulator.occupied[frame];
}

double vmm_web_frame_page(size_t frame)
{
    return vmm_web_frame_occupied(frame)
               ? (double)simulator.frames[frame]
               : 0.0;
}

size_t vmm_web_frame_loaded_at(size_t frame)
{
    return initialized && frame < simulator.frame_count
               ? simulator.loaded_at[frame]
               : 0;
}

size_t vmm_web_frame_used_at(size_t frame)
{
    return initialized && frame < simulator.frame_count
               ? simulator.used_at[frame]
               : 0;
}
