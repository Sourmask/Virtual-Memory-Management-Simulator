#include "vmm.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_FRAMES 3
#define MAX_FRAMES 8

static const char *default_references =
    "7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1";

static void print_usage(const char *program)
{
    printf("Usage: %s [--algorithm FIFO|LRU|OPT] [--frames 1-8] "
           "[--refs \"page page ...\"]\n", program);
    puts("Simulates virtual-memory page replacement.");
    puts("Example:");
    printf("  %s --algorithm LRU --frames 3 "
           "--refs \"7 0 1 2 0 3 0 4 2 3 0 3 2 1 2 0 1 7 0 1\"\n",
           program);
}

static bool parse_size(const char *text, size_t minimum, size_t maximum,
                       size_t *value)
{
    char *end;
    unsigned long parsed;

    errno = 0;
    parsed = strtoul(text, &end, 10);
    if (errno != 0 || text[0] == '\0' || *end != '\0' ||
        parsed < minimum || parsed > maximum) {
        return false;
    }
    *value = (size_t)parsed;
    return true;
}

static bool append_reference(VMMPage **references, size_t *count,
                             size_t *capacity, VMMPage page)
{
    VMMPage *resized;
    size_t new_capacity;

    if (*count == *capacity) {
        if (*capacity > (size_t)-1 / 2 / sizeof(**references)) {
            return false;
        }
        new_capacity = *capacity == 0 ? 16 : *capacity * 2;
        resized = realloc(*references, new_capacity * sizeof(**references));
        if (resized == NULL) {
            return false;
        }
        *references = resized;
        *capacity = new_capacity;
    }
    (*references)[(*count)++] = page;
    return true;
}

static bool parse_references(const char *text, VMMPage **references,
                             size_t *count)
{
    const char *cursor = text;
    size_t capacity = 0;

    *references = NULL;
    *count = 0;
    while (*cursor != '\0') {
        char *end;
        long long page;

        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' ||
               *cursor == '\r' || *cursor == ',') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        if (*cursor == '-') {
            free(*references);
            *references = NULL;
            *count = 0;
            return false;
        }
        errno = 0;
        page = strtoll(cursor, &end, 10);
        if (errno != 0 || end == cursor || page < 0 ||
            (*end != '\0' && *end != ' ' && *end != '\t' &&
             *end != '\n' && *end != '\r' && *end != ',')) {
            free(*references);
            *references = NULL;
            *count = 0;
            return false;
        }
        if (!append_reference(references, count, &capacity, page)) {
            free(*references);
            *references = NULL;
            *count = 0;
            return false;
        }
        cursor = end;
    }
    return true;
}

static void print_frame_state(const VMMSimulator *simulator)
{
    size_t i;

    printf("  Frames:");
    for (i = 0; i < simulator->frame_count; ++i) {
        if (simulator->occupied[i]) {
            printf(" [%zu:%lld]", i, simulator->frames[i]);
        } else {
            printf(" [%zu:-]", i);
        }
    }
    putchar('\n');
}

int main(int argc, char **argv)
{
    VMMAlgorithm algorithm = VMM_FIFO;
    size_t frame_count = DEFAULT_FRAMES;
    const char *reference_text = default_references;
    VMMPage *references = NULL;
    size_t reference_count = 0;
    size_t i;
    VMMSimulator simulator;
    VMMEvent event;

    for (i = 1; i < (size_t)argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "--algorithm") == 0 && i + 1 < (size_t)argc) {
            if (!vmm_parse_algorithm(argv[++i], &algorithm)) {
                fprintf(stderr, "Error: algorithm must be FIFO, LRU, or OPT.\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < (size_t)argc) {
            if (!parse_size(argv[++i], 1, MAX_FRAMES, &frame_count)) {
                fprintf(stderr, "Error: frame count must be between 1 and %d.\n",
                        MAX_FRAMES);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "--refs") == 0 && i + 1 < (size_t)argc) {
            reference_text = argv[++i];
        } else {
            fprintf(stderr, "Error: invalid or incomplete argument '%s'.\n",
                    argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!parse_references(reference_text, &references, &reference_count)) {
        fprintf(stderr, "Error: references must be non-negative integers "
                        "separated by spaces or commas.\n");
        return EXIT_FAILURE;
    }
    if (!vmm_init(&simulator, references, reference_count, frame_count, algorithm)) {
        fprintf(stderr, "Error: unable to initialize the simulator.\n");
        free(references);
        return EXIT_FAILURE;
    }

    printf("Algorithm: %s | Frames: %zu | References: %zu\n\n",
           vmm_algorithm_name(algorithm), frame_count, reference_count);
    while (vmm_step(&simulator, &event)) {
        if (event.hit) {
            printf("Step %zu: page %lld -> HIT in frame %zu\n",
                   event.step, event.page, event.frame);
        } else if (event.has_evicted) {
            printf("Step %zu: page %lld -> FAULT, evict page %lld from frame %zu\n",
                   event.step, event.page, event.evicted, event.frame);
        } else {
            printf("Step %zu: page %lld -> FAULT, loaded into free frame %zu\n",
                   event.step, event.page, event.frame);
        }
        print_frame_state(&simulator);
    }

    printf("\nSummary: %zu accesses, %zu hits, %zu faults, %.2f%% hit ratio\n",
           simulator.hits + simulator.faults, simulator.hits, simulator.faults,
           vmm_hit_ratio(&simulator) * 100.0);

    vmm_destroy(&simulator);
    free(references);
    return EXIT_SUCCESS;
}
