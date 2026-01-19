#include <dpu_mine.h>

// ============================================================
// Level 3: Find Fourth Edge (deg=6)
// Topology: Second(2) -- Fourth(6)
// Constraints: 
//   1. Fourth connects to Root
//   2. Fourth connects to Third
//   3. Intersection(Second, Fourth) != r1
// ============================================================
static ans_t __hg_dfs_3_2_4(sysname_t tasklet_id, node_t root, node_t second_root, node_t third_root, node_t r1) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t second_begin = e2v_idx[second_root];
    edge_t second_end   = e2v_idx[second_root + 1];
    edge_t second_deg   = second_end - second_begin;

    // 
    edge_t root_begin = e2v_idx[root];
    edge_t root_deg   = get_degree(root); // 

    edge_t third_begin = e2v_idx[third_root];
    edge_t third_deg   = get_degree(third_root);

#if defined(H2H)
    // === H2H  ===
    __mram_ptr edge_t *block = H2H_GET_BLOCK(second_root);
    
    edge_t start = H2H_GET_START(block, 6);
    edge_t end   = H2H_GET_END(block, 6);

    for (edge_t i = start; i < end; i++) {
        edge_t e = block[i]; // e is Fourth candidate


        if (!h2h_is_adjacent(root, e)) continue;


        if (!h2h_is_adjacent(third_root, e)) continue;


        edge_t fourth_begin = e2v_idx[e];
        edge_t fourth_deg   = 6;

        node_t r2 = intersect_seq_buf_thresh(tasklet_buf, &e2v[second_begin], second_deg, &e2v[fourth_begin], fourth_deg);
        

        if (r2 == INVALID_NODE || r2 == r1) continue;
        
        ans++;
    }

#else
    // ===  ===
    edge_t begin = deg2e[6][0];
    edge_t end   = deg2e[6][1];

    for (edge_t e = begin; e < end; e++) {
        edge_t fourth_begin = e2v_idx[e];
        edge_t fourth_end   = e2v_idx[e + 1];
        edge_t fourth_deg   = fourth_end - fourth_begin;
        
        node_t r2 = intersect_seq_buf_thresh(tasklet_buf, &e2v[second_begin], second_deg, &e2v[fourth_begin], fourth_deg);
        if(r2 == INVALID_NODE || r2 == r1 ) continue;

        if(!no_intersect_seq_buf(tasklet_buf, &e2v[root_begin], root_deg , &e2v[fourth_begin], fourth_deg )) continue;
        if(!no_intersect_seq_buf(tasklet_buf, &e2v[third_begin], third_deg , &e2v[fourth_begin], fourth_deg )) continue;
        ans++;
    }
#endif

    return ans;
}

// ============================================================
// Level 2: Find Third Edge (deg=4)
// Topology: Second(2) -- Third(4)
// Constraints:
//   1. Intersection(Second, Third) == r1
//   2. Third connects to Root
// ============================================================
static ans_t __hg_dfs_3_2(sysname_t tasklet_id, node_t root, node_t second_root, node_t r1) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t second_begin = e2v_idx[second_root];
    edge_t second_end   = e2v_idx[second_root + 1];
    edge_t second_deg   = second_end - second_begin;

    edge_t root_begin = e2v_idx[root];
    edge_t root_deg   = get_degree(root);

#if defined(H2H)
    // === H2H  ===
    __mram_ptr edge_t *block = H2H_GET_BLOCK(second_root);
    
    edge_t start = H2H_GET_START(block, 4);
    edge_t end   = H2H_GET_END(block, 4);

    for (edge_t i = start; i < end; i++) {
        edge_t e = block[i]; // e is Third candidate


        edge_t third_begin = e2v_idx[e];
        edge_t third_deg   = 4;

        node_t r2 = intersect_seq_buf_thresh(tasklet_buf, &e2v[second_begin], second_deg, &e2v[third_begin], third_deg);
        

        if (r2 != r1) continue;


        if (!h2h_is_adjacent(root, e)) continue;

        ans += __hg_dfs_3_2_4(tasklet_id, root, second_root, e, r1);
    }

#else
    // ===  ===
    edge_t begin = deg2e[4][0];
    edge_t end   = deg2e[4][1];

    for (edge_t e = begin; e < end; e++) {
        edge_t third_begin = e2v_idx[e];
        edge_t third_end   = e2v_idx[e + 1];
        edge_t third_deg   = third_end - third_begin;
        
        node_t r2 = intersect_seq_buf_thresh(tasklet_buf, &e2v[second_begin], second_deg , &e2v[third_begin], third_deg );
        if(r2 != r1 ) continue;
        
        node_t r3 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], root_deg , &e2v[third_begin], third_deg );
        if(r3 == INVALID_NODE ) continue;
        
        ans += __hg_dfs_3_2_4(tasklet_id, root, second_root, e, r1);
    }
#endif

    return ans;
}

// ============================================================
// Level 1 (Serial Fallback): Find Second Edge (deg=2)
// Topology: Root -- Second(2)
// ============================================================
static ans_t __hg_dfs_3(sysname_t tasklet_id, node_t root) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t root_begin = e2v_idx[root];
    edge_t root_deg   = get_degree(root);

#if defined(H2H)
    // === H2H  ===
    __mram_ptr edge_t *block = H2H_GET_BLOCK(root);
    
    edge_t start = H2H_GET_START(block, 2);
    edge_t end   = H2H_GET_END(block, 2);

    for (edge_t i = start; i < end; i++) {
        edge_t e = block[i]; // e is Second candidate

        edge_t second_begin = e2v_idx[e];
        edge_t second_deg   = 2;

        node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], root_deg, &e2v[second_begin], second_deg);
        if (r1 == INVALID_NODE) continue;
        
        ans += __hg_dfs_3_2(tasklet_id, root, e, r1);
    }

#else
    // ===  ===
    edge_t begin = deg2e[2][0];
    edge_t end   = deg2e[2][1];

    for (edge_t e = begin; e < end; e++) {
        edge_t second_begin = e2v_idx[e];
        edge_t second_end   = e2v_idx[e + 1];
        edge_t second_deg   = second_end - second_begin;

        node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], root_deg , &e2v[second_begin], second_deg );
        if(r1 == INVALID_NODE ) continue;
        ans += __hg_dfs_3_2(tasklet_id,root,e,r1);
    }
#endif

    return ans;
}

// ============================================================
// Kernel Entry: hyp2_3_4_6
// ============================================================
extern void hyp2_3_4_6(sysname_t tasklet_id) { 
    static ans_t partial_ans[NR_TASKLETS];
    static uint64_t partial_cycle[NR_TASKLETS];
    static perfcounter_cycles cycles[NR_TASKLETS];

    node_t i = 0;

    while (i < root_num) {
        node_t root = roots[i]; 
        
        node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
        edge_t root_begin = e2v_idx[root];
        edge_t root_deg   = get_degree(root);

#ifndef H2H
        edge_t begin = deg2e[2][0];
        edge_t end   = deg2e[2][1];
        if(end - begin <= 16) break;
#endif

        timer_start(&cycles[tasklet_id]);
        barrier_wait(&co_barrier);
        partial_ans[tasklet_id] = 0;

#if defined(H2H)
        // === H2H ===
        __mram_ptr edge_t *block = H2H_GET_BLOCK(root);
        
        edge_t start = H2H_GET_START(block, 2);
        edge_t end   = H2H_GET_END(block, 2);

        // Stride access
        for (edge_t idx = start + tasklet_id; idx < end; idx += NR_TASKLETS) {
            edge_t e = block[idx];
            
            edge_t second_begin = e2v_idx[e];
            edge_t second_deg   = 2;

            node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], root_deg, &e2v[second_begin], second_deg);
            
            if (r1 == INVALID_NODE) continue;
            partial_ans[tasklet_id] += __hg_dfs_3_2(tasklet_id, root, e, r1);
        }

#else
        // === ===
        for (edge_t e = begin + tasklet_id; e < end; e += NR_TASKLETS) {
            edge_t second_begin = e2v_idx[e];
            edge_t second_end   = e2v_idx[e + 1];
            edge_t second_deg   = second_end - second_begin;

            node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], root_deg , &e2v[second_begin], second_deg );
            if(r1 == INVALID_NODE ) continue;
            partial_ans[tasklet_id] += __hg_dfs_3_2(tasklet_id,root,e,r1);
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