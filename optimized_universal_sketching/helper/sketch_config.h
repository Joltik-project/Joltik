/* parameters related to universal sketching data structure */
#define HEAP_REDUCE_RATE 1
#define PRIME 39916801
#define TOPK_SIZE 100
#define EXTRA_LAYER 0
#define MICE_LAYER 0
#define ELEPHANT_LAYER 8
#define USE_MODEL 0
#define CS_ROW_NO 5
#define CS_LVLS 8
#define EXTRA_SKETCH_NUM 0
#define CS_COL_NO 1425
#define SKETCH_REDUCE_RATE 2
#define SAMPLE_RATE 2
typedef uint16_t my_key;
typedef int16_t my_int;

/* parameters related to compressing universal sketching data structure */
#define BIT_SIZE_LVL_1 4
#define BIT_SIZE_LVL_2 8
#define BIT_SIZE_LVL_3 12
#define BIT_SIZE_LVL_4 16
#define PREFIX_LVL_1 "00"
#define PREFIX_LVL_2 "01"
#define PREFIX_LVL_3 "10"
#define PREFIX_LVL_4 "11"

/* whether to deploy computation optimization as indicated in paper */
#define ONLY_UPDATE_LAST_LAYER_COUNT_SKETCH 1