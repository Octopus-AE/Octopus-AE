#include <dpu_mine.h>


#define CHECK_ADJ_HYBRID(u, u_ptr, u_deg, v, v_ptr, v_deg) \
    ( (e_cnt <= BITMAP_ROW) ? \
      is_adjacent(u, v) : \
      !no_intersect_seq_buf(tasklet_buf, u_ptr, u_deg, v_ptr, v_deg) )



static ans_t __hg_dfs_3_4(sysname_t tasklet_id, node_t root, node_t second_root, node_t r1) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t second_begin = e2v_idx[second_root];
    // edge_t second_deg = 4; // 已知

#if defined(H2H)

    __mram_ptr edge_t *block = H2H_GET_BLOCK(root);
    
    edge_t start = H2H_GET_START(block, 5);
    edge_t end   = H2H_GET_END(block, 5);

    for (edge_t i = start; i < end; i++) {
        edge_t e = block[i]; // e is Third candidate

        if (h2h_is_adjacent(second_root, e)) continue;

        edge_t third_begin = e2v_idx[e];
        edge_t root_begin  = e2v_idx[root];
        
        node_t r2 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], 3, &e2v[third_begin], 5);
        if(r2 == INVALID_NODE || r2 == r1) continue;
        ans++;
    }

#else
    // ===   ===
    edge_t root_begin = e2v_idx[root];
    // edge_t root_deg = 3;

    edge_t begin = deg2e[5][0];
    edge_t end   = deg2e[5][1];

    for (edge_t e = begin; e < end; e++) {
        edge_t third_begin = e2v_idx[e];
        edge_t third_deg   = 5;

        // 1.  Root(3) -- Third(5)
        bool is_connected_root = CHECK_ADJ_HYBRID(root, &e2v[root_begin], 3, 
                                                  e, &e2v[third_begin], 5);
        if (!is_connected_root) continue;

        // 2.  Second(4) -/- Third(5)
        bool is_connected_second = CHECK_ADJ_HYBRID(second_root, &e2v[second_begin], 4, 
                                                    e, &e2v[third_begin], 5);
        if (is_connected_second) continue;

        node_t r2 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], 3, &e2v[third_begin], 5);
        if(r2 == INVALID_NODE || r2 == r1) continue;
        ans++;
    }
#endif

    return ans;
}

// ============================================================
// DFS Step 1: Find Second Edge (deg=4)
// Topology: Root(3) -- Second(4)
// ============================================================
static ans_t __hg_dfs_3(sysname_t tasklet_id, node_t root) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t root_begin = e2v_idx[root];
    // edge_t root_deg = 3;

#if defined(H2H)
    // === H2H ===
    __mram_ptr edge_t *block = H2H_GET_BLOCK(root);
    
    edge_t start = H2H_GET_START(block, 4);
    edge_t end   = H2H_GET_END(block, 4);

    for (edge_t i = start; i < end; i++) {
        edge_t e = block[i]; // e is Second candidate

        edge_t second_begin = e2v_idx[e];
        
        node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], 3, &e2v[second_begin], 4);
        
        if(r1 == INVALID_NODE) continue;
        ans += __hg_dfs_3_4(tasklet_id, root, e, r1);
    }

#else
    // ===  ===
    edge_t begin = deg2e[4][0];
    edge_t end   = deg2e[4][1];

    for (edge_t e = begin; e < end; e++) {
        edge_t second_begin = e2v_idx[e];
        edge_t second_deg   = 4;

        // 1.  Root(3) -- Second(4)
        bool is_connected = CHECK_ADJ_HYBRID(root, &e2v[root_begin], 3, 
                                             e, &e2v[second_begin], 4);
        if (!is_connected) continue;

        node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], 3, &e2v[second_begin], 4);
        if(r1 == INVALID_NODE) continue;
        ans += __hg_dfs_3_4(tasklet_id, root, e, r1);
    }
#endif

    return ans;
}

// ============================================================
// Kernel Entry: hyp3_4_5
// ============================================================
extern void hyp3_4_5(sysname_t tasklet_id) {
    static ans_t partial_ans[NR_TASKLETS];
    static uint64_t partial_cycle[NR_TASKLETS];
    static perfcounter_cycles cycles[NR_TASKLETS];

    node_t i = 0;

    while (i < root_num) {
        node_t root = roots[i]; 
        
        node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
        edge_t root_begin = e2v_idx[root];
        // edge_t root_deg = 3; 

#ifndef H2H
        edge_t begin = deg2e[4][0];
        edge_t end   = deg2e[4][1];
        if(end - begin <= 16) break;
#endif

        timer_start(&cycles[tasklet_id]);
        barrier_wait(&co_barrier);
        partial_ans[tasklet_id] = 0;

#if defined(H2H)
        // === H2H ===
        __mram_ptr edge_t *block = H2H_GET_BLOCK(root);
        
        edge_t start = H2H_GET_START(block, 4);
        edge_t end   = H2H_GET_END(block, 4);

        for (edge_t idx = start + tasklet_id; idx < end; idx += NR_TASKLETS) {
            edge_t e = block[idx];
            
            edge_t second_begin = e2v_idx[e];
            
            node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], 3, &e2v[second_begin], 4);
            
            if(r1 == INVALID_NODE) continue;
            partial_ans[tasklet_id] += __hg_dfs_3_4(tasklet_id, root, e, r1);
        }

#else
        // ===  ===
        for (edge_t e = begin + tasklet_id; e < end; e += NR_TASKLETS) {
            edge_t second_begin = e2v_idx[e];
            edge_t second_deg   = 4;

            // 
            bool is_connected = CHECK_ADJ_HYBRID(root, &e2v[root_begin], 3, 
                                                 e, &e2v[second_begin], 4);
            if (!is_connected) continue;

            node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], 3, &e2v[second_begin], 4);
            if(r1 == INVALID_NODE) continue;
            partial_ans[tasklet_id] += __hg_dfs_3_4(tasklet_id, root, e, r1);
        }
#endif

        partial_cycle[tasklet_id] = timer_stop(&cycles[tasklet_id]);
        barrier_wait(&co_barrier);

        if (tasklet_id == 0) {
            ans_t total_ans = 0;
            uint64_t total_cycle = 0;
            for (uint32_t j = 0; j < NR_TASKLETS; j++) {
                total_ans += partial_ans[j];
                total_cycle += partial_cycle[j];
            }
            ans[i] = total_ans;
            cycle_ct[i] = total_cycle;
        }
        i++;
    }

    // 
    for (i += tasklet_id; i < root_num; i += NR_TASKLETS) {
        node_t root = roots[i];
        
        timer_start(&cycles[tasklet_id]);
        ans[i] = __hg_dfs_3(tasklet_id, root);
        cycle_ct[i] = timer_stop(&cycles[tasklet_id]); 
    }
}