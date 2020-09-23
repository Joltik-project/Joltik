/*
 * univmon_offline.c contains functions for optimized universal sketching offline algorithm. 
 * When you received sketch summaries from sensors, use offline_estimate_* functions to estimate
 * metrics of interests. 
 */ 

#include <string.h>
#include "univmon.h"

#define MAX_LINE 8192

void init_seed_pair_to_zero(seedPair* current_pair) {
    current_pair->seed1 = 0;
    current_pair->seed2 = 0;
}

void init_single_sketch_copy(singleSketchCopy* current_copy, int is_mice_layer,
                             uint32_t current_layer_sketch_length,
                             uint32_t current_layer_heap_size) {
    int i;
    init_seed_pair_to_zero(&(current_copy->seed_sketch));
    initMinHeap(&(current_copy->topKs), current_layer_heap_size);
    if (!is_mice_layer) {
        for (i = 0; i < CS_ROW_NO; i++) {
            init_seed_pair_to_zero(&(current_copy->seed_pos[i]));
            init_seed_pair_to_zero(&(current_copy->seed_filter[i]));
            current_copy->sketch_counter[i] =
                calloc(current_layer_sketch_length, sizeof(my_int));
        }
    }
}

void init_univmon(univSketch* univ) {
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
}

void load_univmon_from_file(univSketch* univ) {
    FILE* fp = fopen(ONLINE_FILE, "r");
    char current_line[MAX_LINE];
    int layer_num = 0;
    int copy_num = 0;
    singleSketchLayer* current_layer = &(univ->all_layers[layer_num]);
    singleSketchCopy* current_copy = &(current_layer->all_copy[copy_num]);

    while (fgets(current_line, MAX_LINE, fp) != NULL) {
        if (strstr(current_line, "layer")) {
            sscanf(current_line, "layer %d copy %d", &layer_num, &copy_num);
            current_layer = &(univ->all_layers[layer_num]);
            current_copy = &(current_layer->all_copy[copy_num]);
        }

        if (strstr(current_line, "seed_sketch")) {
            sscanf(current_line, "seed_sketch seed1 %u seed2 %u",
                   &(current_copy->seed_sketch.seed1),
                   &(current_copy->seed_sketch.seed2));
        }

        if (strstr(current_line, "heap_elem")) {
            uint32_t key;
            int i, count;
            sscanf(current_line, "heap_elem %d key %u count %d", &i, &key,
                   &count);
            insertNode(&(current_copy->topKs), key, count);
        }

        if (strstr(current_line, "seed_pos")) {
            int i;
            uint32_t seed1, seed2;
            sscanf(current_line, "seed_pos %d seed1 %u seed2 %u", &i, &seed1,
                   &seed2);
            current_copy->seed_pos[i].seed1 = seed1;
            current_copy->seed_pos[i].seed2 = seed2;
        }

        if (strstr(current_line, "seed_filter")) {
            int i;
            uint32_t seed1, seed2;
            sscanf(current_line, "seed_filter %d seed1 %u seed2 %u", &i, &seed1,
                   &seed2);
            current_copy->seed_filter[i].seed1 = seed1;
            current_copy->seed_filter[i].seed2 = seed2;
        }

        if (strstr(current_line, "sketch_counter")) {
            int i;
            char current_line_counter[MAX_LINE];
            sscanf(current_line, "sketch_counter %d %s", &i,
                   current_line_counter);
            for (int j = 0; j < current_layer->current_layer_sketch_length;
                 j++) {
                int temp;
                sscanf(current_line_counter, "%d,%s", &temp,
                       current_line_counter);
                current_copy->sketch_counter[i][j] = temp;
            }
        }
    }
    fclose(fp);
}

/* print functions */
void print_seed_pair(seedPair* current_pair, FILE* fp) {
    fprintf(fp, "seed1 %d seed2 %d\n", current_pair->seed1,
            current_pair->seed2);
}

void print_heap(minHeap* current_heap, FILE* fp) {
    int i;
    for (i = 0; i < current_heap->size; i++) {
        fprintf(fp, "heap_elem %d key %d count %d\n", i,
                current_heap->elem[i].key, current_heap->elem[i].count);
    }
}

void swap_my_int(my_int* a, my_int* b) {
    my_int temp = *b;
    *b = *a;
    *a = temp;
}

void swap_my_key(my_key* a, my_key* b) {
    my_key temp = *b;
    *b = *a;
    *a = temp;
}

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

void print_univmon(univSketch* univ) {
    FILE* fp = fopen(OFFLINE_FILE, "w");
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

void print_single_sketch_copy_counter_heap(
    singleSketchCopy* current_copy, FILE* fp, int is_mice_layer,
    uint32_t current_layer_sketch_length) {
    // fprintf(fp, "seed_sketch ");
    // print_seed_pair(&(current_copy->seed_sketch), fp);
    print_heap(&(current_copy->topKs), fp);
    // print_sorted_heap(&(current_copy->topKs), fp);
    if (!is_mice_layer) {
        int i, j;
        // for (i = 0; i < CS_ROW_NO; i++) {
        //     fprintf(fp, "seed_pos %d ", i);
        //     print_seed_pair(&(current_copy->seed_pos[i]), fp);
        // }
        // for (i = 0; i < CS_ROW_NO; i++) {
        //     fprintf(fp, "seed_filter %d ", i);
        //     print_seed_pair(&(current_copy->seed_filter[i]), fp);
        // }
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

void print_univmon_counter_heap(univSketch* univ) {
    FILE* fp = fopen(OFFLINE_COUNTER_FILE, "w");
    int i, j;
    for (i = 0; i < CS_LVLS; i++) {
        singleSketchLayer* current_layer = &(univ->all_layers[i]);
        if (!(current_layer->is_extra_layer || current_layer->is_mice_layer)) {
            fprintf(fp, "layer %d copy 0\n", i);
            print_single_sketch_copy_counter_heap(
                &(current_layer->all_copy[0]), fp, current_layer->is_mice_layer,
                current_layer->current_layer_sketch_length);
        } else {
            for (j = 0; j < 1 + EXTRA_SKETCH_NUM; j++) {
                fprintf(fp, "layer %d copy %d\n", i, j);
                print_single_sketch_copy_counter_heap(
                    &(current_layer->all_copy[j]), fp,
                    current_layer->is_mice_layer,
                    current_layer->current_layer_sketch_length);
            }
        }
    }
}

/* offline functions */
void quick_sort(long double* estimation, int first_index, int last_index) {
    // declaring index variables
    int pivotIndex, index_a, index_b;
    long double temp;

    if (first_index < last_index) {
        // assigning first element index as pivot element
        pivotIndex = first_index;
        index_a = first_index;
        index_b = last_index;

        // Sorting in Ascending order with quick sort
        while (index_a < index_b) {
            while (estimation[index_a] <= estimation[pivotIndex] &&
                   index_a < last_index) {
                index_a++;
            }
            while (estimation[index_b] > estimation[pivotIndex]) {
                index_b--;
            }

            if (index_a < index_b) {
                // Swapping operation
                temp = estimation[index_a];
                estimation[index_a] = estimation[index_b];
                estimation[index_b] = temp;
            }
        }

        // At the end of first iteration, swap pivot element with index_b
        // element
        temp = estimation[pivotIndex];
        estimation[pivotIndex] = estimation[index_b];
        estimation[index_b] = temp;

        // Recursive call for quick sort, with partitioning
        quick_sort(estimation, first_index, index_b - 1);
        quick_sort(estimation, index_b + 1, last_index);
    }
}

long double calculate_entropy_for_one_copy(univSketch* univ, int copy_num) {
    long double y_bottom = 0.0;
    long double y_bottom_num = 0.0;
    singleSketchLayer* current_layer;
    singleSketchCopy* current_copy;
    minHeap* current_heap;
    minHeap* next_layer_heap;

    int layer_to_calc = CS_LVLS;

    /* calculate bottom layer */
    current_layer = &(univ->all_layers[layer_to_calc - 1]);
    current_copy = &(current_layer->all_copy[copy_num]);
    current_heap = &(current_copy->topKs);
    for (int i = 0; i < (*current_heap).size; ++i) {
        long double w = (long double)(*current_heap).elem[i].count;
        if (w > 0.0) {
            y_bottom += w * log2l(w);
            y_bottom_num += w;
        }
    }
    next_layer_heap = current_heap;

    long double y_1 = 0.0, y_2 = 0.0;
    long double y_1_num = 0.0, y_2_num = 0.0;

    y_2 = y_bottom;
    y_2_num = y_bottom_num;

    for (int i = layer_to_calc - 2; i >= 0; i--) {
        current_layer = &(univ->all_layers[i]);
        if (current_layer->is_extra_layer) {
            current_copy = &(current_layer->all_copy[copy_num]);
        } else {
            current_copy = &(current_layer->all_copy[0]);
        }
        current_heap = &(current_copy->topKs);

        long double indSum = 0.0;
        long double indSum_num = 0.0;
        for (int j = 0; j < (*current_heap).size; j++) {
            long double w_tmp = (long double)(*current_heap).elem[j].count;
            if (w_tmp > 0.0) {
                long double hash = 0.0;
                for (int k = 0; k < (*next_layer_heap).size; k++) {
                    if ((*current_heap).elem[j].key ==
                        (*next_layer_heap).elem[k].key) {
                        hash = 1.0;
                    }
                }
                indSum += (1.0 - SAMPLE_RATE * hash) * w_tmp * log2l(w_tmp);
                indSum_num += (1.0 - SAMPLE_RATE * hash) * w_tmp;
            }
        }

        y_1 = SAMPLE_RATE * y_2 + indSum;
        y_2 = y_1;
        y_1_num = SAMPLE_RATE * y_2_num + indSum_num;
        y_2_num = y_1_num;
        next_layer_heap = current_heap;
    }

    // printf("total_num: %Lf\n", y_1_num);
    long double entropy =
        log2l((long double)y_1_num) - (y_1 / (long double)y_1_num);
    return entropy;
}

long double offline_estimate_entropy(univSketch* univ) {
    long double offline_estimate_entropy;
    long double entropy_estimation[1 + EXTRA_SKETCH_NUM];
    int total_copy_num = 1 + EXTRA_SKETCH_NUM;
    int i;

    for (i = 0; i < total_copy_num; i++) {
        entropy_estimation[i] = calculate_entropy_for_one_copy(univ, i);
        // printf("copy %d, entropy estimation %Lf\n", i,
        // entropy_estimation[i]);
    }
    quick_sort(entropy_estimation, 0, total_copy_num - 1);

    // printf("after sort:\n");
    // for (i = 0; i < total_copy_num; i++) {
    //     printf("copy %d, entropy estimation %Lf\n", i,
    //     entropy_estimation[i]);
    // }
    if (total_copy_num % 2 == 0) {
        offline_estimate_entropy =
            (entropy_estimation[total_copy_num / 2] +
             entropy_estimation[total_copy_num / 2 - 1]) /
            2;
    } else {
        offline_estimate_entropy = entropy_estimation[total_copy_num / 2];
    }

    return offline_estimate_entropy;
}

long double calculate_f2_for_one_copy(univSketch* univ, int copy_num) {
    long double y_bottom = 0.0;
    singleSketchLayer* current_layer;
    singleSketchCopy* current_copy;
    minHeap* current_heap;
    minHeap* next_layer_heap;

    int layer_to_calc = CS_LVLS;

    /* calculate bottom layer */
    current_layer = &(univ->all_layers[layer_to_calc - 1]);
    current_copy = &(current_layer->all_copy[copy_num]);
    current_heap = &(current_copy->topKs);
    for (int i = 0; i < (*current_heap).size; ++i) {
        long double w = (long double)(*current_heap).elem[i].count;
        if (w > 0.0) {
            y_bottom += w * w;
        }
    }
    next_layer_heap = current_heap;

    long double y_1 = 0.0, y_2 = 0.0;

    y_2 = y_bottom;

    for (int i = layer_to_calc - 2; i >= 0; i--) {
        current_layer = &(univ->all_layers[i]);
        if (current_layer->is_extra_layer) {
            current_copy = &(current_layer->all_copy[copy_num]);
            printf("copy_num: %d\n", copy_num);
        } else {
            current_copy = &(current_layer->all_copy[0]);
        }
        current_heap = &(current_copy->topKs);

        long double indSum = 0.0;
        for (int j = 0; j < (*current_heap).size; j++) {
            long double w_tmp = (long double)(*current_heap).elem[j].count;
            if (w_tmp > 0.0) {
                long double hash = 0.0;
                for (int k = 0; k < (*next_layer_heap).size; k++) {
                    if ((*current_heap).elem[j].key ==
                        (*next_layer_heap).elem[k].key) {
                        hash = 1.0;
                    }
                }
                indSum += (1.0 - SAMPLE_RATE * hash) * w_tmp * w_tmp;
            }
        }

        y_1 = SAMPLE_RATE * y_2 + indSum;
        y_2 = y_1;
        next_layer_heap = current_heap;
    }

    long double f2 = y_1;

    return f2;
}

long double offline_estimate_f2(univSketch* univ) {
    long double offline_estimate_f2;
    long double f2_estimation[1 + EXTRA_SKETCH_NUM];
    int total_copy_num = 1 + EXTRA_SKETCH_NUM;
    int i;

    for (i = 0; i < total_copy_num; i++) {
        f2_estimation[i] = calculate_f2_for_one_copy(univ, i);
        printf("copy %d, f2 estimation %Lf\n", i, f2_estimation[i]);
    }
    quick_sort(f2_estimation, 0, total_copy_num - 1);

    printf("after sort:\n");
    for (i = 0; i < total_copy_num; i++) {
        printf("copy %d, f2 estimation %Lf\n", i, f2_estimation[i]);
    }
    if (total_copy_num % 2 == 0) {
        offline_estimate_f2 = (f2_estimation[total_copy_num / 2] +
                               f2_estimation[total_copy_num / 2 - 1]) /
                              2;
    } else {
        offline_estimate_f2 = f2_estimation[total_copy_num / 2];
    }

    return offline_estimate_f2;
}

long double calculate_cardinality_for_one_copy(univSketch* univ, int copy_num) {
    long double y_bottom = 0.0;
    singleSketchLayer* current_layer;
    singleSketchCopy* current_copy;
    minHeap* current_heap;
    minHeap* next_layer_heap;

    int layer_to_calc = CS_LVLS;

    /* calculate bottom layer */
    current_layer = &(univ->all_layers[layer_to_calc - 1]);
    current_copy = &(current_layer->all_copy[copy_num]);
    current_heap = &(current_copy->topKs);
    for (int i = 0; i < (*current_heap).size; ++i) {
        long double w = (long double)(*current_heap).elem[i].count;
        if (w > 0.0) {
            y_bottom += 1;
        }
    }
    next_layer_heap = current_heap;

    long double y_1 = 0.0, y_2 = 0.0;

    y_2 = y_bottom;

    for (int i = layer_to_calc - 2; i >= 0; i--) {
        current_layer = &(univ->all_layers[i]);
        if (current_layer->is_extra_layer) {
            current_copy = &(current_layer->all_copy[copy_num]);
        } else {
            current_copy = &(current_layer->all_copy[0]);
        }
        current_heap = &(current_copy->topKs);

        long double indSum = 0.0;
        for (int j = 0; j < (*current_heap).size; j++) {
            long double w_tmp = (long double)(*current_heap).elem[j].count;
            if (w_tmp > 0.0) {
                long double hash = 0.0;
                for (int k = 0; k < (*next_layer_heap).size; k++) {
                    if ((*current_heap).elem[j].key ==
                        (*next_layer_heap).elem[k].key) {
                        hash = 1.0;
                    }
                }
                indSum += (1.0 - SAMPLE_RATE * hash) * 1;
            }
        }

        y_1 = SAMPLE_RATE * y_2 + indSum;
        y_2 = y_1;
        next_layer_heap = current_heap;
    }

    long double cardinality = y_1;

    return cardinality;
}

long double offline_estimate_cardinality(univSketch* univ) {
    long double offline_estimate_cardinality;
    long double cardinality_estimation[1 + EXTRA_SKETCH_NUM];
    int total_copy_num = 1 + EXTRA_SKETCH_NUM;
    int i;

    for (i = 0; i < total_copy_num; i++) {
        cardinality_estimation[i] = calculate_cardinality_for_one_copy(univ, i);
        // printf("copy %d, f2 estimation %Lf\n", i, cardinality_estimation[i]);
    }
    quick_sort(cardinality_estimation, 0, total_copy_num - 1);

    // printf("after sort:\n");
    // for (i = 0; i < total_copy_num; i++) {
    //     printf("copy %d, f2 estimation %Lf\n", i, cardinality_estimation[i]);
    // }
    if (total_copy_num % 2 == 0) {
        offline_estimate_cardinality =
            (cardinality_estimation[total_copy_num / 2] +
             cardinality_estimation[total_copy_num / 2 - 1]) /
            2;
    } else {
        offline_estimate_cardinality =
            cardinality_estimation[total_copy_num / 2];
    }

    return offline_estimate_cardinality;
}

/* print offline result to file */
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

void print_offline_result_to_file(univSketch* univ) {
    FILE* fp = fopen(OFFLINE_RESULT, "w");
    minHeap* current_heap = &(univ->all_layers[0].all_copy[0].topKs);
    print_sorted_heap(current_heap, fp);
    fprintf(fp, "real_cardinality %Lf\n", offline_estimate_cardinality(univ));
    fprintf(fp, "real_entropy %Lf\n", offline_estimate_entropy(univ));
    fprintf(fp, "real_f2 %Lf\n", offline_estimate_f2(univ));
}

int main() {
    univSketch my_univmon;
    univSketch* univ = &my_univmon;
    init_univmon(univ);
    load_univmon_from_file(univ);
    print_univmon(univ);
    print_offline_result_to_file(univ);
}