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

#if defined(H2H)
    __mram_noinit edge_t  h2h_offset[IDX_CAPACITY]; // 
    __mram_noinit edge_t  h2h_buffer[H2H_CAPACITY]; // size < 24M
    __host edge_t  h2h_total_size;           // 
#else 
    __mram_noinit bit_t bitmap[BITMAP_ROW][BITMAP_COL]; 
#endif

__host uint64_t root_num;
__mram_noinit node_t roots[DPU_ROOT_NUM];   // 1M
__mram_noinit uint64_t ans[DPU_ROOT_NUM];   // 2M
__mram_noinit uint64_t cycle_ct[DPU_ROOT_NUM]; 

// buffer
node_t buf[NR_TASKLETS][3][BUF_SIZE];  // 6K
__mram_noinit node_t mram_buf[NR_TASKLETS << 2][BUF_SIZE];  // <=16M

// synchronization
BARRIER_INIT(co_barrier, NR_TASKLETS);

// intersection
extern node_t intersect_seq_buf_thresh(node_t(*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size);
extern bool no_intersect_seq_buf(node_t (*buf)[BUF_SIZE],node_t __mram_ptr *a, node_t a_size,node_t __mram_ptr *b, node_t b_size);
extern node_t intersect_buf_lite(node_t (*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size, node_t __mram_ptr *c);

static inline edge_t get_degree(edge_t e) {
    return e2v_idx[e+1] - e2v_idx[e];
}

#if defined(H2H)
    // 宏保持不变，但在逻辑上 k 代表度数
    #define H2H_GET_BLOCK(e)        (&h2h_buffer[h2h_offset[(e)]])
    #define H2H_GET_START(block, k) ((block)[(k)-1])
    #define H2H_GET_END(block, k)   ((block)[(k)])

    static inline bool h2h_is_adjacent(edge_t u, edge_t v) {
        __mram_ptr edge_t *block = H2H_GET_BLOCK(u);
        edge_t v_deg = get_degree(v); // 获取 v 的度数

        // 如果度数超出了索引范围，说明不可能在索引里 (或者你需要回退到全扫描，视构建逻辑而定)
        if (v_deg > MAX_EDGE_SIZE) return false; 

        // 直接定位到该度数的区间
        edge_t start = H2H_GET_START(block, v_deg);
        edge_t end   = H2H_GET_END(block, v_deg);
        
        // 只需扫描这一个小区间
        for (edge_t i = start; i < end; i++) {
            if (block[i] == v) return true;
        }
        return false;
    }

#else
    extern bool is_adjacent(edge_t a, edge_t b)
    {
        uint32_t word_idx = b >> 6;     // b / 64
        uint32_t bit_pos  = b & 63;     // b % 64
        return (bitmap[a][word_idx] >> bit_pos) & 1ULL;
    }
#endif