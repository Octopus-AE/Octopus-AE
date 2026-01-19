#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>  // for debug


#define DATA_DIR "./data/"
#if defined(SB)
#define DATA_NAME "senate"
#elif defined(CP)
#define DATA_NAME "primary_school"
#elif defined(TC)
#define DATA_NAME "trivago"
#elif defined(WT)
#define DATA_NAME "wt"
#elif defined(HB)
#define DATA_NAME "hb"
#elif defined(HS)
#define DATA_NAME "high_school"

#else
#warning "No graph selected, fall back to SB."
#define DATA_NAME "senate"
#endif

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

#if defined(HYP3_7_12)
#define KERNEL_FUNC hyp3_7_12
#define PATTERN_NAME "hyp3_7_12"
#elif defined(HYP3_4_5)
#define KERNEL_FUNC hyp3_4_5
#define PATTERN_NAME "hyp3_4_5"
#elif defined(HYP2_3_7_13)
#define KERNEL_FUNC hyp2_3_7_13
#define PATTERN_NAME "hyp2_3_7_13"
#elif defined(HYP2_3_4_6)
#define KERNEL_FUNC hyp2_3_4_6
#define PATTERN_NAME "hyp2_3_4_6"
#elif defined(HYP2_3_4_5_24)
#define KERNEL_FUNC hyp2_3_4_5_24
#define PATTERN_NAME "hyp2_3_4_5_24"
#else
#warning "No kernel function selected, fall back to HYP3_7_12."
#define KERNEL_FUNC hyp3_7_12
#define PATTERN_NAME "hyp3_7_12"
#endif



#define IDX_CAPACITY   (1024 * 1024)   
#define E2V_CAPACITY   (7 * 1024 * 1024) 
#define ADJ_CAPACITY  (2 * 1024 * 1024)
#define MAX_EDGE_SIZE  32                
#define MAX_EDGES      (1024 * 1024)    


#define bit_t uint64_t
#define BITMAP_ROW (14144) 
#define BITMAP_COL (BITMAP_ROW /64)

#define node_t uint32_t
#define edge_t uint32_t
#define ans_t uint64_t
#define SIZE_NODE_T_LOG 2
#define SIZE_EDGE_PTR_LOG 2
#define INVALID_NODE ((node_t)(-1))
#define DPU_ROOT_NUM ((1<<18)/sizeof(node_t))
#define BUF_SIZE 32
#define MRAM_BUF_SIZE 1024

//#define H2H
#define H2H_CAPACITY   (6 * 1024 * 1024)
// #define BRANCH_LEVEL_THRESHOLD 16


typedef struct {
    edge_t e;     // 
    edge_t cnt;   // 
} AdjPair;        // 

typedef struct {
    edge_t  e2v_idx[IDX_CAPACITY];  // 
    node_t  e2v[E2V_CAPACITY];  // 
    edge_t  e_cnt;              // 
    edge_t  e2v_size;           // e2v 


    edge_t  deg2e[MAX_EDGE_SIZE][2];

    //
    edge_t  h2h_offset[IDX_CAPACITY]; // CSR 
    edge_t  h2h_buffer[H2H_CAPACITY]; //  size < 24M
    edge_t  h2h_total_size;           // buffer 

    //
    bit_t bitmap[BITMAP_ROW][BITMAP_COL]; 

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