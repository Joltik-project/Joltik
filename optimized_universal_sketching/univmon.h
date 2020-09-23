/*
 * univmon.h defines universal sketching data structure.
 */ 

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "helper/heap.h"
#include "helper/input.h"
#include "helper/sketch_config.h"

typedef struct seedPair {
    uint32_t seed1;
    uint32_t seed2;
} seedPair;

typedef struct singleSketchCopy {
    seedPair seed_sketch;
    minHeap topKs;
    seedPair seed_pos[CS_ROW_NO];
    seedPair seed_filter[CS_ROW_NO];
    my_int* sketch_counter[CS_ROW_NO];
} singleSketchCopy;

typedef struct singleSketchLayer {
    int is_extra_layer;
    int is_mice_layer;
    uint32_t current_layer_sketch_length;
    uint32_t current_layer_heap_size;
    singleSketchCopy* all_copy;
} singleSketchLayer;

typedef struct univSketch {
    singleSketchLayer all_layers[CS_LVLS];
    uint32_t size; /* unit: byte */
} univSketch;