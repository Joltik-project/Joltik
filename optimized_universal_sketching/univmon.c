/*
 * univmon.c contains functions for optimized universal sketching algorithm. When integrating 
 * this file with sensors, simply use "univmon_processing" to get the sketch summary, and then
 * use "univmon_compressing" to get compressed sketch summary for transmission.
 */ 

#include "univmon.h"

/*
 * helper function to initialize seed pair for sketch data structure
 */
void init_seed_pair(seedPair* current_pair) {
    current_pair->seed1 = rand() % PRIME;
    current_pair->seed2 = rand() % PRIME;
}

/*
 * helper function to initialize a single sketch copy
 */
void init_single_sketch_copy(singleSketchCopy* current_copy, int is_mice_layer,
                             uint32_t current_layer_sketch_length,
                             uint32_t current_layer_heap_size) {
    int i;
    init_seed_pair(&(current_copy->seed_sketch));
    initMinHeap(&(current_copy->topKs), current_layer_heap_size);
    if (!is_mice_layer) {
        for (i = 0; i < CS_ROW_NO; i++) {
            init_seed_pair(&(current_copy->seed_pos[i]));
            init_seed_pair(&(current_copy->seed_filter[i]));
            current_copy->sketch_counter[i] =
                calloc(current_layer_sketch_length, sizeof(my_int));
        }
    }
}

/*
 * helper function to initialize the entire univmon data structure
 */
void init_univmon(univSketch* univ) {
    srand((unsigned)time(NULL));
    int i, j;
    for (i = 0; i < CS_LVLS; i++) {
        singleSketchLayer* current_layer = &(univ->all_layers[i]);

        if (i < ELEPHANT_LAYER) {
            current_layer->is_extra_layer = 0;
            current_layer->is_mice_layer = 0;
        } else if (i >= ELEPHANT_LAYER && i < ELEPHANT_LAYER + EXTRA_LAYER) {
            current_layer->is_extra_layer = 1;
            current_layer->is_mice_layer = 0;
        } else {
            current_layer->is_extra_layer = 0;
            current_layer->is_mice_layer = 1;
        }

        if (!current_layer->is_mice_layer) {
            current_layer->current_layer_sketch_length =
                (uint32_t)(CS_COL_NO / pow(SKETCH_REDUCE_RATE, i));
        } else {
            current_layer->current_layer_sketch_length = 0;
        }

        current_layer->current_layer_heap_size =
            (uint32_t)(TOPK_SIZE / pow(HEAP_REDUCE_RATE, i));

        if (current_layer->is_extra_layer || current_layer->is_mice_layer) {
            current_layer->all_copy =
                calloc(1 + EXTRA_SKETCH_NUM, sizeof(singleSketchCopy));
            for (j = 0; j < 1 + EXTRA_SKETCH_NUM; j++) {
                init_single_sketch_copy(
                    &(current_layer->all_copy[j]), current_layer->is_mice_layer,
                    current_layer->current_layer_sketch_length,
                    current_layer->current_layer_heap_size);
            }
        } else {
            current_layer->all_copy = calloc(1, sizeof(singleSketchCopy));
            init_single_sketch_copy(&(current_layer->all_copy[0]),
                                    current_layer->is_mice_layer,
                                    current_layer->current_layer_sketch_length,
                                    current_layer->current_layer_heap_size);
        }
    }

    /* calculate total sketch memory usage of universal sketching data structure */
    printf("total memory usage: ");
    int total_num = 0;
    for (i = 0; i < CS_LVLS; i++) {
        int current_layer_num = 0;
        singleSketchLayer* current_layer = &(univ->all_layers[i]);
        if (current_layer->is_mice_layer) {
            current_layer_num = (1 + EXTRA_SKETCH_NUM) *
                                (current_layer->current_layer_heap_size *
                                 (sizeof(my_key) + sizeof(my_int)));
            total_num += current_layer_num;
        } else if (current_layer->is_extra_layer) {
            current_layer_num =
                (1 + EXTRA_SKETCH_NUM) *
                (current_layer->current_layer_heap_size *
                     (sizeof(my_key) + sizeof(my_int)) +
                 CS_ROW_NO * current_layer->current_layer_sketch_length *
                     sizeof(my_int));
            total_num += current_layer_num;
        } else {
            current_layer_num = current_layer->current_layer_heap_size *
                                    (sizeof(my_key) + sizeof(my_int)) +
                                CS_ROW_NO *
                                    current_layer->current_layer_sketch_length *
                                    sizeof(my_int);
            total_num += current_layer_num;
        }
    }
    univ->size = total_num;
    printf("total_num: %d\n", total_num);
}

/*
 * helper function to print a seed pair
 */
void print_seed_pair(seedPair* current_pair, FILE* fp) {
    fprintf(fp, "seed1 %d seed2 %d\n", current_pair->seed1,
            current_pair->seed2);
}

/*
 * helper function to print heap
 */
void print_heap(minHeap* current_heap, FILE* fp) {
    int i;
    for (i = 0; i < current_heap->size; i++) {
        fprintf(fp, "heap_elem %d key %d count %d\n", i,
                current_heap->elem[i].key, current_heap->elem[i].count);
    }
}

/*
 * helper function to swap two integers
 */
void swap_my_int(my_int* a, my_int* b) {
    my_int temp = *b;
    *b = *a;
    *a = temp;
}

/*
 * helper function to swap two keys
 */
void swap_my_key(my_key* a, my_key* b) {
    my_key temp = *b;
    *b = *a;
    *a = temp;
}

/*
 * helper function to print sorted heap
 */
void print_sorted_heap(minHeap* current_heap, FILE* fp) {
    my_int freq[TOPK_SIZE];
    my_key item[TOPK_SIZE];
    int k;
    for (k = 0; k < current_heap->size; k++) {
        item[k] = current_heap->elem[k].key;
        freq[k] = current_heap->elem[k].count;
    }

    /* do bubble sort */
    int i = 0;
    int j;
    while (i < current_heap->size) {
        j = 0;
        while (j < i) {
            if (freq[j] < freq[i]) {
                swap_my_int(&freq[j], &freq[i]);
                swap_my_key(&item[j], &item[i]);
            }
            j++;
        }
        i++;
    }

    for (i = 0; i < current_heap->size; i++) {
        fprintf(fp, "key %d count %d\n", item[i], freq[i]);
    }
}

/*
 * helper function to print a single sketch copy
 */
void print_single_sketch_copy(singleSketchCopy* current_copy, FILE* fp,
                              int is_mice_layer,
                              uint32_t current_layer_sketch_length) {
    fprintf(fp, "seed_sketch ");
    print_seed_pair(&(current_copy->seed_sketch), fp);
    print_heap(&(current_copy->topKs), fp);
    // print_sorted_heap(&(current_copy->topKs), fp);
    if (!is_mice_layer) {
        int i, j;
        for (i = 0; i < CS_ROW_NO; i++) {
            fprintf(fp, "seed_pos %d ", i);
            print_seed_pair(&(current_copy->seed_pos[i]), fp);
        }
        for (i = 0; i < CS_ROW_NO; i++) {
            fprintf(fp, "seed_filter %d ", i);
            print_seed_pair(&(current_copy->seed_filter[i]), fp);
        }
        for (i = 0; i < CS_ROW_NO; i++) {
            fprintf(fp, "sketch_counter %d ", i);
            for (j = 0; j < current_layer_sketch_length; j++) {
                fprintf(fp, "%d,", current_copy->sketch_counter[i][j]);
            }
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "\n");
}

/*
 * helper function to print the entire univmon data structure
 */
void print_univmon(univSketch* univ) {
    FILE* fp = fopen(ONLINE_FILE, "w");
    int i, j;
    for (i = 0; i < CS_LVLS; i++) {
        singleSketchLayer* current_layer = &(univ->all_layers[i]);
        if (!(current_layer->is_extra_layer || current_layer->is_mice_layer)) {
            fprintf(fp, "layer %d copy 0\n", i);
            print_single_sketch_copy(
                &(current_layer->all_copy[0]), fp, current_layer->is_mice_layer,
                current_layer->current_layer_sketch_length);
        } else {
            for (j = 0; j < 1 + EXTRA_SKETCH_NUM; j++) {
                fprintf(fp, "layer %d copy %d\n", i, j);
                print_single_sketch_copy(
                    &(current_layer->all_copy[j]), fp,
                    current_layer->is_mice_layer,
                    current_layer->current_layer_sketch_length);
            }
        }
    }
}

/*
 * Hash function. 
 */
uint32_t simple_hash(seedPair current_pair, uint32_t key) {
    return ((current_pair.seed1 * key + current_pair.seed2) % PRIME);
}

/*
 * Update sketch counter in a single line in each layer.
 */
my_int insert_sketch_counter_single_line(singleSketchCopy* current_copy,
                                         uint32_t current_layer_sketch_length,
                                         int line_num, uint32_t key) {
    uint32_t pos = simple_hash(current_copy->seed_pos[line_num], key) %
                   current_layer_sketch_length;
    int f2_filter = simple_hash(current_copy->seed_filter[line_num], key) % 2;
    current_copy->sketch_counter[line_num][pos] += (1 - 2 * f2_filter);

    my_int current_value;
    if ((1 - 2 * f2_filter) > 0) {
        current_value = current_copy->sketch_counter[line_num][pos];
    } else {
        current_value = -current_copy->sketch_counter[line_num][pos];
    }
    return current_value;
}

/*
 * Helper function to do quick sort.
 */
void quick_sort(my_int* values, my_int first_index, my_int last_index) {
    // declaring index variables
    my_int pivotIndex, temp, index_a, index_b;

    if (first_index < last_index) {
        // assigning first element index as pivot element
        pivotIndex = first_index;
        index_a = first_index;
        index_b = last_index;

        // Sorting in Ascending order with quick sort
        while (index_a < index_b) {
            while (values[index_a] <= values[pivotIndex] &&
                   index_a < last_index) {
                index_a++;
            }
            while (values[index_b] > values[pivotIndex]) {
                index_b--;
            }

            if (index_a < index_b) {
                // Swapping operation
                temp = values[index_a];
                values[index_a] = values[index_b];
                values[index_b] = temp;
            }
        }

        // At the end of first iteration, swap pivot element with index_b
        // element
        temp = values[pivotIndex];
        values[pivotIndex] = values[index_b];
        values[index_b] = temp;

        // Recursive call for quick sort, with partitioning
        quick_sort(values, first_index, index_b - 1);
        quick_sort(values, index_b + 1, last_index);
    }
}

/*
 * helper function to find median value.
 */
my_int find_median_value(my_int* values) {
    my_int median;
    quick_sort(values, 0, CS_ROW_NO - 1);
    if (CS_ROW_NO % 2 == 0) {
        median = (values[CS_ROW_NO / 2] + values[CS_ROW_NO / 2 - 1]) / 2;
    } else {
        median = values[CS_ROW_NO / 2];
    }
    median = (median <= 0) ? 0 : median;

    return median;
}

/*
 * Update sketch counter in a sketch layer.
 */
my_int adjust_counter_for_single_elephant_copy(
    singleSketchCopy* current_copy, uint32_t current_layer_sketch_length,
    my_key key) {
    my_int values[CS_ROW_NO];
    int line_num;
    for (line_num = 0; line_num < CS_ROW_NO; line_num++) {
        values[line_num] = insert_sketch_counter_single_line(
            current_copy, current_layer_sketch_length, line_num, key);
    }
    my_int median = find_median_value(values);
    return median;
}

/*
 * Update heap in a sketch layer.
 */
void adjust_heap_for_single_elephant_copy(singleSketchCopy* current_copy,
                                          my_int median, my_key key,
                                          uint32_t current_layer_heap_size) {
    minHeap* current_heap = &(current_copy->topKs);
    int found = find(current_heap, key);
    if (found != (-1)) {
        (*current_heap).elem[found].count++;
        // (*current_heap).elem[found].count = median;
        for (int i = ((*current_heap).size - 1) / 2; i >= 0; i--) {
            heapify(current_heap, i);
        }
    } else {
        if ((*current_heap).size < current_layer_heap_size) {
            insertNode(current_heap, key, median);
        } else if (median > (*current_heap).elem[0].count) {
            deleteNode(current_heap);
            insertNode(current_heap, key, median);
        }
    }
}

/*
 * Update heap in a sketch layer.
 */
my_int adjust_heap_for_single_mice_copy(singleSketchCopy* current_copy,
                                        my_key key,
                                        uint32_t current_layer_heap_size) {
    minHeap* current_heap = &(current_copy->topKs);
    int count = 0;
    int found = find(current_heap, key);
    if (found != (-1)) {
        (*current_heap).elem[found].count++;
        count = (*current_heap).elem[found].count;
        for (int i = ((*current_heap).size - 1) / 2; i >= 0; i--) {
            heapify(current_heap, i);
        }
    } else if ((*current_heap).size < current_layer_heap_size) {
        insertNode(current_heap, key, 1);
        count = 1;
    }
    return count;
}

/*
 * Update the entire sketch data structure.
 */
void update_bottom_up(univSketch* univ, int bottom_layer_num, int up_layer_num,
                      int copy_num, my_key key) {
    singleSketchLayer* current_layer;
    singleSketchCopy* current_copy;

    /* update bottom layer: counter and heap */
    current_layer = &(univ->all_layers[bottom_layer_num]);
    current_copy = &(current_layer->all_copy[copy_num]);
    my_int median;
    if (!(current_layer->is_mice_layer)) {
        median = adjust_counter_for_single_elephant_copy(
            current_copy, current_layer->current_layer_sketch_length, key);
        adjust_heap_for_single_elephant_copy(
            current_copy, median, key, current_layer->current_layer_heap_size);
    } else {
        median = adjust_heap_for_single_mice_copy(
            current_copy, key, current_layer->current_layer_heap_size);
    }

    /* update all layers except bottom layer: heap */
    int i;
    for (i = up_layer_num; i < bottom_layer_num; i++) {
        current_layer = &(univ->all_layers[i]);
        current_copy = &(current_layer->all_copy[copy_num]);
        if (ONLY_UPDATE_LAST_LAYER_COUNT_SKETCH) {
            adjust_heap_for_single_elephant_copy(
                current_copy, median, key,
                current_layer->current_layer_heap_size);
        } else {
            median = adjust_counter_for_single_elephant_copy(
                current_copy, current_layer->current_layer_sketch_length, key);
            adjust_heap_for_single_elephant_copy(
                current_copy, median, key,
                current_layer->current_layer_heap_size);
        }
    }
}

/*
 * Decide which layer will a key be sampled in.
 */
int could_enter_current_copy(singleSketchCopy* current_copy, uint32_t key) {
    if (simple_hash(current_copy->seed_sketch, key) % SAMPLE_RATE == 0) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Find the last possible layer for each key.
 */
int find_bottom_layer_num(univSketch* univ, int start_num, int end_num,
                          int copy_num, uint32_t key) {
    singleSketchLayer* current_layer;
    singleSketchCopy* current_copy;
    int bottom_layer_num = start_num;
    int i;
    for (i = start_num + 1; i < end_num; i++) {
        current_layer = &(univ->all_layers[i]);
        current_copy = &(current_layer->all_copy[copy_num]);
        if (could_enter_current_copy(current_copy, key)) {
            bottom_layer_num = i;
        } else {
            break;
        }
    }
    return bottom_layer_num;
}

/*
 * Process a new sensed value in universal sketching data structure.
 */
void univmon_processing(univSketch* univ, uint32_t key) {
    int start_num, end_num, copy_num, bottom_layer_num, up_layer_num;
    /* update copy 0 */
    start_num = 0;
    end_num = CS_LVLS;
    copy_num = 0;
    bottom_layer_num =
        find_bottom_layer_num(univ, start_num, end_num, copy_num, key);
    // count[bottom_layer_num] += 1;
    up_layer_num = 0;
    update_bottom_up(univ, bottom_layer_num, up_layer_num, copy_num, key);

    /* update other copies */
    if (bottom_layer_num >= ELEPHANT_LAYER - 1) {
        start_num = ELEPHANT_LAYER - 1;
        end_num = CS_LVLS;
        up_layer_num = ELEPHANT_LAYER;
        int i;
        for (i = 1; i < 1 + EXTRA_SKETCH_NUM; i++) {
            copy_num = i;
            bottom_layer_num =
                find_bottom_layer_num(univ, start_num, end_num, copy_num, key);
            if (bottom_layer_num >= up_layer_num) {
                update_bottom_up(univ, bottom_layer_num, up_layer_num, copy_num,
                                 key);
            }
        }
    }
}

/* main function used for simulation on computer, simply change INPUT_FILE to your sensor datasets.
 * When you integrate universal sketching algorithm on your sensor, use "univmon_processing" as interface
 * to do universal sketching data structure update for each new sensed value, and use "univmon_compressing"
 * to compress the universal sketching data structure. */
int main() {
    univSketch my_univmon;
    univSketch* univ = &my_univmon;
    init_univmon(univ);

    int i;

    FILE* fp = fopen(INPUT_FILE, "r");
    for (i = 0; i < INPUT_NUM; i++) {
        int temp;
        fscanf(fp, "%d ", &temp);
        univmon_processing(univ, (uint32_t)temp);
    }
}