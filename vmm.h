#ifndef VMM_H
#define VMM_H

#include <stdbool.h>
#include <stddef.h>

typedef long long VMMPage;

typedef enum {
    VMM_FIFO,
    VMM_LRU,
    VMM_OPT
} VMMAlgorithm;

typedef struct {
    size_t step;
    VMMPage page;
    bool hit;
    size_t frame;
    bool has_evicted;
    VMMPage evicted;
} VMMEvent;

typedef struct {
    const VMMPage *references;
    size_t reference_count;
    VMMAlgorithm algorithm;
    VMMPage *frames;
    bool *occupied;
    size_t *loaded_at;
    size_t *used_at;
    size_t frame_count;
    size_t position;
    size_t hits;
    size_t faults;
} VMMSimulator;

const char *vmm_algorithm_name(VMMAlgorithm algorithm);
bool vmm_parse_algorithm(const char *name, VMMAlgorithm *algorithm);
bool vmm_init(VMMSimulator *simulator, const VMMPage *references,
              size_t reference_count, size_t frame_count,
              VMMAlgorithm algorithm);
void vmm_destroy(VMMSimulator *simulator);
bool vmm_done(const VMMSimulator *simulator);
double vmm_hit_ratio(const VMMSimulator *simulator);
bool vmm_step(VMMSimulator *simulator, VMMEvent *event);

#endif
