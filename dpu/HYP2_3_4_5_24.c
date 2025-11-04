#include <dpu_mine.h>


static ans_t __hg_dfs_3_5_4_2(sysname_t tasklet_id,node_t root, node_t second_root,node_t third_root ,node_t fourth_root) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t root_begin = e2v_idx[root];
    edge_t root_end   = e2v_idx[root + 1];
    edge_t root_deg   = root_end - root_begin;

    edge_t second_begin = e2v_idx[second_root];
    edge_t second_end   = e2v_idx[second_root + 1];
    edge_t second_deg   = second_end - second_begin;

    edge_t third_begin = e2v_idx[third_root];
    edge_t third_end   = e2v_idx[third_root + 1];
    edge_t third_deg   = third_end - third_begin;

    edge_t fourth_begin = e2v_idx[fourth_root];
    edge_t fourth_end   = e2v_idx[fourth_root + 1];
    edge_t fourth_deg   = fourth_end - fourth_begin;

    edge_t begin = deg2e[24][0];
    edge_t end   = deg2e[24][1];

    for (edge_t e = begin; e < end; e++) {

        edge_t fifth_begin = e2v_idx[e];
        edge_t fifth_end   = e2v_idx[e + 1];
        edge_t fifth_deg   = fifth_begin - fifth_end;
        
        if(!no_intersect_seq_buf(tasklet_buf, &e2v[root_begin], root_deg , &e2v[fifth_begin], fifth_deg ))continue;
        if(!no_intersect_seq_buf(tasklet_buf, &e2v[fourth_begin], fourth_deg , &e2v[fifth_begin], fifth_deg ))continue;

        node_t s1 = intersect_buf_lite(tasklet_buf, &e2v[second_begin], second_deg, &e2v[fifth_begin], fifth_deg,mram_buf[tasklet_id+NR_TASKLETS]);
        if(s1 != 2 )continue;
        node_t s2 = intersect_buf_lite(tasklet_buf, &e2v[third_begin], third_deg, &e2v[fifth_begin], fifth_deg,mram_buf[tasklet_id+NR_TASKLETS*2]);
        if(s2 != 2 )continue;
        node_t s3 = intersect_buf_lite(tasklet_buf, mram_buf[tasklet_id+NR_TASKLETS], 2, mram_buf[tasklet_id+NR_TASKLETS*2] , 2,mram_buf[tasklet_id+NR_TASKLETS*3]);
        if(s3 != 2 )continue;
        ans ++;
    }
    return ans;
}

static ans_t __hg_dfs_3_5_4(sysname_t tasklet_id,node_t root, node_t second_root,node_t third_root ,node_t r1) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t root_begin = e2v_idx[root];
    edge_t root_end   = e2v_idx[root + 1];
    edge_t root_deg   = root_end - root_begin;

    edge_t second_begin = e2v_idx[second_root];
    edge_t second_end   = e2v_idx[second_root + 1];
    edge_t second_deg   = second_end - second_begin;

    edge_t third_begin = e2v_idx[third_root];
    edge_t third_end   = e2v_idx[third_root + 1];
    edge_t third_deg   = third_end - third_begin;

    edge_t begin = deg2e[2][0];
    edge_t end   = deg2e[2][1];

    for (edge_t e = begin; e < end; e++) {

        edge_t fourth_begin = e2v_idx[e];
        edge_t fourth_end   = e2v_idx[e + 1];
        edge_t fourth_deg   = fourth_end - fourth_begin;
        
        if(!no_intersect_seq_buf(tasklet_buf, &e2v[root_begin], root_deg , &e2v[fourth_begin], fourth_deg ))continue;
        if(!no_intersect_seq_buf(tasklet_buf, &e2v[second_begin], second_deg , &e2v[fourth_begin], fourth_deg ))continue;
        node_t r2 = intersect_seq_buf_thresh(tasklet_buf, &e2v[third_begin], third_deg , &e2v[fourth_begin], fourth_deg );
        if(r2 == INVALID_NODE )continue;
        if(r2 == mram_buf[tasklet_id][0] || r2 == mram_buf[tasklet_id][1])continue;
        ans += __hg_dfs_3_5_4_2(tasklet_id,root,second_root,third_root,e);
    }
    return ans;
}

static ans_t __hg_dfs_3_5(sysname_t tasklet_id,node_t root, node_t second_root,node_t r1) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t root_begin = e2v_idx[root];
    edge_t root_end   = e2v_idx[root + 1];
    edge_t root_deg   = root_end - root_begin;

    edge_t second_begin = e2v_idx[second_root];
    edge_t second_end   = e2v_idx[second_root + 1];
    edge_t second_deg   = second_end - second_begin;

    edge_t begin = deg2e[4][0];
    edge_t end   = deg2e[4][1];

    for (edge_t e = begin; e < end; e++) {

        edge_t third_begin = e2v_idx[e];
        edge_t third_end   = e2v_idx[e + 1];
        edge_t third_deg   = third_end - third_begin;
        
        if(!no_intersect_seq_buf(tasklet_buf, &e2v[root_begin], root_deg , &e2v[third_begin], third_deg ))continue;
        node_t s1 = intersect_buf_lite(tasklet_buf, &e2v[second_begin], second_deg, &e2v[third_begin], third_deg,mram_buf[tasklet_id]);
        if(s1 != 2 )continue;
        if(r1 == mram_buf[tasklet_id][0] || r1 == mram_buf[tasklet_id][1])continue;

        ans += __hg_dfs_3_5_4(tasklet_id,root,second_root,e,r1);
        
    }
    return ans;
}

static ans_t __hg_dfs_3(sysname_t tasklet_id, node_t root) {
    node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
    ans_t ans = 0;

    edge_t root_begin = e2v_idx[root];
    edge_t root_end   = e2v_idx[root + 1];
    edge_t root_deg   = root_end - root_begin;

    edge_t begin = deg2e[5][0];
    edge_t end   = deg2e[5][1];

    for (edge_t e = begin; e < end; e++) {
        edge_t second_begin = e2v_idx[e];
        edge_t second_end   = e2v_idx[e + 1];
        edge_t second_deg   = second_end - second_begin;

        node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], root_deg , &e2v[second_begin], second_deg );
        if(r1 == INVALID_NODE )continue;
        ans += __hg_dfs_3_5(tasklet_id,root,e,r1);
    }

    return ans;
}

extern void hyp2_3_4_5_24(sysname_t tasklet_id) { 
    static ans_t partial_ans[NR_TASKLETS];
    static uint64_t partial_cycle[NR_TASKLETS];
    static perfcounter_cycles cycles[NR_TASKLETS];

    node_t i = 0;

    while (i < root_num) {
        node_t root = roots[i]; 
        
        node_t(*tasklet_buf)[BUF_SIZE] = buf[tasklet_id];
        edge_t root_begin = e2v_idx[root];
        edge_t root_end   = e2v_idx[root + 1];
        edge_t root_deg   = root_end - root_begin;

        edge_t begin = deg2e[5][0];
        edge_t end   = deg2e[5][1];

        if(end - begin <= 16)break;

        timer_start(&cycles[tasklet_id]);
        barrier_wait(&co_barrier);
        partial_ans[tasklet_id] = 0;

        for (edge_t e = begin + tasklet_id; e < end; e += NR_TASKLETS) {
            edge_t second_begin = e2v_idx[e];
            edge_t second_end   = e2v_idx[e + 1];
            edge_t second_deg   = second_end - second_begin;

            node_t r1 = intersect_seq_buf_thresh(tasklet_buf, &e2v[root_begin], root_deg , &e2v[second_begin], second_deg );
            if(r1 == INVALID_NODE )continue;
             partial_ans[tasklet_id] += __hg_dfs_3_5(tasklet_id,root,e,r1);
        }

        partial_cycle[tasklet_id] = timer_stop(&cycles[tasklet_id]);

        barrier_wait(&co_barrier);

        if (tasklet_id == 0) {
            ans_t total_ans = 0;
            uint64_t total_cycle = 0;
            for (uint32_t j = 0; j < NR_TASKLETS; j++) {
                total_ans += partial_ans[j];
                total_cycle += partial_cycle[j];
            }
            ans[i] = total_ans;  // intended DMA
            cycle_ct[i] = total_cycle;  // intended DMA
        }
        i++;
    }
 
    for (i += tasklet_id; i < root_num; i += NR_TASKLETS) {
        node_t root = roots[i];  // intended DMA
        
        timer_start(&cycles[tasklet_id]);

        ans[i] = __hg_dfs_3(tasklet_id, root);  // intended DMA

        cycle_ct[i] = timer_stop(&cycles[tasklet_id]); 
    
    }
}