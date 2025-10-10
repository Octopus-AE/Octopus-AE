#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>  // for debug


#define DATA_DIR "./data/"
#define DATA_NAME "senate"
// #define DATA_NAME "school"
#define DATA_PATH DATA_DIR DATA_NAME ".txt"

#ifndef NR_DPUS
#warning "No NR_DPUS defined, fall back to 1."
#define NR_DPUS 1
#endif
#ifndef NR_TASKLETS
#warning "No NR_TASKLETS defined, fall back to 1."
#define NR_TASKLETS 1
#endif
#ifndef DPU_BINARY
#warning "No DPU_BINARY defined, fall back to bin/dpu."
#define DPU_BINARY "bin/dpu" 
#endif

#if defined(CLIQUE3)
#define KERNEL_FUNC clique3
#define PATTERN_NAME "clique3"
#endif


#define IDX_CAPACITY   (1024 * 1024)   // 
#define E2V_CAPACITY   (7 * 1024 * 1024)  // 
#define ADJ_CAPACITY  (2 * 1024 * 1024)
#define MAX_EDGE_SIZE  256                // 
#define MAX_EDGES      (1024 * 1024)      //

#define node_t uint32_t
#define edge_t uint32_t
#define ans_t uint64_t
#define SIZE_NODE_T_LOG 2
#define SIZE_EDGE_PTR_LOG 2
#define INVALID_NODE ((node_t)(-1))
#define DPU_ROOT_NUM ((1<<20)/sizeof(node_t))
#define BUF_SIZE 32
#define MRAM_BUF_SIZE 1024

// #define BRANCH_LEVEL_THRESHOLD 16


typedef struct {
    edge_t e;     
    edge_t cnt;   
} AdjPair;        

typedef struct {
    edge_t  e2v_idx[IDX_CAPACITY];  
    node_t  e2v[E2V_CAPACITY];  
    edge_t  e_cnt;              
    edge_t  e2v_size;           


    edge_t  deg2e[MAX_EDGE_SIZE][2];


    edge_t   adj_idx[IDX_CAPACITY]; // 
    AdjPair  adj_e2e[ADJ_CAPACITY]; // 
    edge_t   adj_size;              //

    uint64_t root_num[NR_DPUS];  // number of search roots allocated to dpu
    node_t *roots[NR_DPUS];
} Hypergraph;


#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define ALIGN2(x) ALIGN(x, 2)
#define ALIGN4(x) ALIGN(x, 4)
#define ALIGN8(x) ALIGN(x, 8)
#define ALIGN16(x) ALIGN(x, 16)
#define ALIGN_LOWER(x, a) ((x) & ~((a)-1))
#define ALIGN2_LOWER(x) ALIGN_LOWER(x, 2)
#define ALIGN4_LOWER(x) ALIGN_LOWER(x, 4)
#define ALIGN8_LOWER(x) ALIGN_LOWER(x, 8)
#define ALIGN16_LOWER(x) ALIGN_LOWER(x, 16)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#endif // COMMON_H