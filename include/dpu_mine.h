#include <common.h>
#include <cyclecount.h>
#include <string.h>
#include <mram.h>
#include <defs.h>
#include <assert.h>
#include <barrier.h>

// transferred data
__mram_noinit edge_t e2v_idx[IDX_CAPACITY];   // 4M
__mram_noinit node_t e2v[E2V_CAPACITY];    // 28M
__host edge_t e_cnt;
__host edge_t e2v_size;
__mram_noinit edge_t deg2e[MAX_EDGE_SIZE][2];
__mram_noinit edge_t adj_idx[IDX_CAPACITY]; // 
__mram_noinit AdjPair adj_e2e[ADJ_CAPACITY]; //
__host edge_t adj_size;        


__host uint64_t root_num;
__mram_noinit node_t roots[DPU_ROOT_NUM];   // 1M
__mram_noinit uint64_t ans[DPU_ROOT_NUM];   // 2M
__mram_noinit uint64_t cycle_ct[DPU_ROOT_NUM]; 

// buffer
node_t buf[NR_TASKLETS][3][BUF_SIZE];  // 6K
//__mram_noinit node_t mram_buf[NR_TASKLETS << 2][MRAM_BUF_SIZE];  // <=16M

// synchronization
BARRIER_INIT(co_barrier, NR_TASKLETS);

// intersection
extern node_t intersect_seq_buf_thresh(node_t(*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size);
extern bool no_intersect_seq_buf(node_t (*buf)[BUF_SIZE],node_t __mram_ptr *a, node_t a_size,node_t __mram_ptr *b, node_t b_size);
