#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <dpu.h>
#include <dpu_types.h>
#include <time.h>   


extern Hypergraph *hg;

typedef struct {
    node_t nodes[MAX_EDGE_SIZE];
    edge_t size;
} EdgeTmp;

static EdgeTmp edges[MAX_EDGES];


static int cmp_node(const void *a, const void *b) {
    node_t na = *(const node_t*)a;
    node_t nb = *(const node_t*)b;
    return (na > nb) - (na < nb);
}


static int cmp_edge_tmp(const void *a, const void *b) {
    const EdgeTmp *ea = (const EdgeTmp*)a;
    const EdgeTmp *eb = (const EdgeTmp*)b;
    if (ea->size != eb->size) return (ea->size > eb->size) - (ea->size < eb->size);
    for (edge_t i = 0; i < ea->size; i++) {
        if (ea->nodes[i] != eb->nodes[i])
            return (ea->nodes[i] > eb->nodes[i]) - (ea->nodes[i] < eb->nodes[i]);
    }
    return 0;
}


int load_hypergraph(Hypergraph *hg) {
    FILE *fp = fopen(DATA_PATH, "rb");
    if (!fp) { perror("fopen"); return -1; }

    edge_t e_cnt = 0;
    static char line[1024];

    while (fgets(line, sizeof(line), fp)) {
        if (e_cnt >= MAX_EDGES) {
            fprintf(stderr, "too many edges!\n");
            break;
        }


        for (char *p = line; *p; p++) if (*p == ',') *p = ' ';

        node_t buf[MAX_EDGE_SIZE];
        edge_t n_cnt = 0;

        char *tok = strtok(line, " \t\r\n");
        while (tok && n_cnt < MAX_EDGE_SIZE) {
            long long val = atoll(tok);
            if (val < 0) { tok = strtok(NULL, " \t\r\n"); continue; }
            buf[n_cnt++] = (node_t)val;
            tok = strtok(NULL, " \t\r\n");
        }
        if (n_cnt == 0) continue;


        qsort(buf, n_cnt, sizeof(node_t), cmp_node);
        edge_t unique_cnt = 0;
        for (edge_t i = 0; i < n_cnt; i++)
            if (unique_cnt == 0 || buf[i] != edges[e_cnt].nodes[unique_cnt-1])
                edges[e_cnt].nodes[unique_cnt++] = buf[i];

        edges[e_cnt].size = unique_cnt;
        e_cnt++;
    }
    fclose(fp);
    

    qsort(edges, e_cnt, sizeof(EdgeTmp), cmp_edge_tmp);


    edge_t new_cnt = 0;
    for (edge_t i = 0; i < e_cnt; i++) {
        if (new_cnt == 0 || cmp_edge_tmp(&edges[i], &edges[new_cnt-1]) != 0) {
            if (new_cnt != i) edges[new_cnt] = edges[i];
            new_cnt++;
        }
    }
    e_cnt = new_cnt;


    hg->e_cnt = e_cnt;
    hg->e2v_size = 0;
    hg->e2v_idx[0] = 0;

    for (edge_t i = 0; i < e_cnt; i++) {
        if (hg->e2v_size + edges[i].size >= E2V_CAPACITY) {
            fprintf(stderr, "e2v overflow!\n");
            break;
        }
        memcpy(&hg->e2v[hg->e2v_size], edges[i].nodes, edges[i].size * sizeof(node_t));
        hg->e2v_size += edges[i].size;
        hg->e2v_idx[i+1] = hg->e2v_size;
    }


    edge_t last_deg = 0;
    for (edge_t i = 0; i < e_cnt; i++) {
        edge_t deg = hg->e2v_idx[i+1] - hg->e2v_idx[i];


        if (deg != last_deg) {
            if (last_deg > 0) {
                hg->deg2e[last_deg][1] = i;  
            }
            hg->deg2e[deg][0] = i;          
            last_deg = deg;
        }
    }
    printf("WARNING: adj pool full (%u)\n", hg->e2v_size);

    hg->deg2e[last_deg][1] = e_cnt;


    // hg->adj_size = 0;
    // for (edge_t e = 0; e < hg->e_cnt; e++) {
    //     hg->adj_idx[e] = hg->adj_size;
    //     for (edge_t f = e + 1; f < hg->e_cnt; f++) {
    //         
    //         edge_t i = hg->e2v_idx[e], i_end = hg->e2v_idx[e+1];
    //         edge_t j = hg->e2v_idx[f], j_end = hg->e2v_idx[f+1];
    //         edge_t cnt = 0;
    //         while (i < i_end && j < j_end) {
    //             if (hg->e2v[i] == hg->e2v[j]) { cnt++; i++; j++; }
    //             else if (hg->e2v[i] < hg->e2v[j]) i++;
    //             else j++;
    //         }
    //         if (cnt > 0) {
    //             if (hg->adj_size + 2 > ADJ_CAPACITY) {
    //                 printf("WARNING: adj pool full (%u)\n", hg->adj_size);
    //                 return;
    //             }
    //            
    //             hg->adj_e2e[hg->adj_size].e   = f;
    //             hg->adj_e2e[hg->adj_size].cnt = cnt;
    //             hg->adj_size++;

    //            
    //             hg->adj_e2e[hg->adj_size].e   = e;
    //             hg->adj_e2e[hg->adj_size].cnt = cnt;
    //             hg->adj_size++;
    //         }
    //     }
    // }
    // hg->adj_idx[hg->e_cnt] = hg->adj_size;

    return 0;
}


void print_hypergraph(Hypergraph *hg, int limit) {
    printf("Total edges = %u\n", hg->e_cnt);
    for (edge_t e = 0; e < hg->e_cnt && e < (edge_t)limit; e++) {
        printf("E%u: ", e);
        for (edge_t i = hg->e2v_idx[e]; i < hg->e2v_idx[e+1]; i++) {
            printf("%u ", hg->e2v[i]);
        }
        printf("\n");
    }
}


void test_deg2e(Hypergraph *hg, edge_t degree) {
    edge_t start = hg->deg2e[degree][0];
    edge_t end   = hg->deg2e[degree][1];

    printf("Edges with degree %u: [%u, %u)\n", degree, start, end);
    // for (edge_t e = start; e < end; e++) {
    //     printf("E%u: ", e);
    //     for (edge_t i = hg->e2v_idx[e]; i < hg->e2v_idx[e+1]; i++)
    //         printf("%u ", hg->e2v[i]);
    //     printf("\n");
    // }
}

edge_t test_hyperedge_intersection(const Hypergraph* hg, edge_t e1, edge_t e2,
                              node_t* buffer, edge_t buf_size)
{
    edge_t i = hg->e2v_idx[e1];
    edge_t i_end = hg->e2v_idx[e1+1];
    edge_t j = hg->e2v_idx[e2];
    edge_t j_end = hg->e2v_idx[e2+1];
    edge_t k = 0;

    while (i < i_end && j < j_end && k < buf_size) {
        node_t n1 = hg->e2v[i];
        node_t n2 = hg->e2v[j];

        if (n1 == n2) {
            buffer[k++] = n1;
            i++; j++;
        } else if (n1 < n2) {
            i++;
        } else {
            j++;
        }
    }
    return k; 
}


void alloc_tasks(Hypergraph *hg) {
    edge_t start = hg->deg2e[3][0];
    edge_t end   = hg->deg2e[3][1];


    for (unsigned d = 0; d < NR_DPUS; d++) {
        hg->root_num[d] = 0;
        hg->roots[d] = (node_t*)malloc(sizeof(node_t) * DPU_ROOT_NUM);
    }

    // round-robin
    for (edge_t e = start; e < end; e++) {
        unsigned dpu = (e - start) % NR_DPUS;
        hg->roots[dpu][hg->root_num[dpu]++] = (node_t)e;  
    }

    // 
    // for (unsigned d = 0; d < NR_DPUS; d++) {
    //     printf("DPU %u: root_num = %llu\n", d,
    //            (unsigned long long)hg->root_num[d]);
    // }
}

edge_t test_pattern_count_buf(Hypergraph* hg, edge_t buf_size) {
    edge_t count = 0;
    node_t buffer[buf_size];  


    edge_t start3 = hg->deg2e[3][0], end3 = hg->deg2e[3][1];

    edge_t start7 = hg->deg2e[7][0], end7 = hg->deg2e[7][1];

    edge_t start12 = hg->deg2e[12][0], end12 = hg->deg2e[12][1];

    for (edge_t e3 = start3; e3 < end3; e3++) {
        for (edge_t e7 = start7; e7 < end7; e7++) {

            edge_t inter37 = test_hyperedge_intersection(hg, e3, e7, buffer, buf_size);
            if (inter37 != 0) continue;

            for (edge_t e12 = start12; e12 < end12; e12++) {

                edge_t inter312 = test_hyperedge_intersection(hg, e3, e12, buffer, buf_size);
                if (inter312 != 1) continue;
                node_t inter_node_312 = buffer[0];

                edge_t inter712 = test_hyperedge_intersection(hg, e7, e12, buffer, buf_size);
                if (inter712 != 1) continue;
                node_t inter_node_712 = buffer[0];


                if (inter_node_312 == inter_node_712) continue;


                count++;
            }
        }
    }

    return count;
}

edge_t test_pattern_count_buf_print(Hypergraph* hg, edge_t buf_size) {
    edge_t count = 0;
    node_t buffer[buf_size];  


    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }


    struct {
        edge_t e3, e7, e12;
    } samples[3];
    int sample_num = 0;


    edge_t start3 = hg->deg2e[3][0], end3 = hg->deg2e[3][1];

    edge_t start7 = hg->deg2e[7][0], end7 = hg->deg2e[7][1];

    edge_t start12 = hg->deg2e[12][0], end12 = hg->deg2e[12][1];

    for (edge_t e3 = start3; e3 < end3; e3++) {
        for (edge_t e7 = start7; e7 < end7; e7++) {

            edge_t inter37 = test_hyperedge_intersection(hg, e3, e7, buffer, buf_size);
            if (inter37 != 0) continue;

            for (edge_t e12 = start12; e12 < end12; e12++) {

                edge_t inter312 = test_hyperedge_intersection(hg, e3, e12, buffer, buf_size);
                if (inter312 != 1) continue;
                node_t inter_node_312 = buffer[0];


                edge_t inter712 = test_hyperedge_intersection(hg, e7, e12, buffer, buf_size);
                if (inter712 != 1) continue;
                node_t inter_node_712 = buffer[0];


                if (inter_node_312 == inter_node_712) continue;


                count++;


                if (sample_num < 3) {
                    samples[sample_num].e3 = e3;
                    samples[sample_num].e7 = e7;
                    samples[sample_num].e12 = e12;
                    sample_num++;
                } else {
                    int r = rand() % count;
                    if (r < 3) {
                        samples[r].e3 = e3;
                        samples[r].e7 = e7;
                        samples[r].e12 = e12;
                    }
                }
            }
        }
    }

    printf("Total matches of the pattern (BUF_SIZE=%u): %u\n", buf_size, count);


    for (int i = 0; i < sample_num; i++) {
        printf("Sample %d: E%u (deg=3), E%u (deg=7), E%u (deg=12)\n",
               i+1, samples[i].e3, samples[i].e7, samples[i].e12);
        printf("  E%u: ", samples[i].e3);
        for (edge_t j = hg->e2v_idx[samples[i].e3]; j < hg->e2v_idx[samples[i].e3+1]; j++)
            printf("%u ", hg->e2v[j]);
        printf("\n  E%u: ", samples[i].e7);
        for (edge_t j = hg->e2v_idx[samples[i].e7]; j < hg->e2v_idx[samples[i].e7+1]; j++)
            printf("%u ", hg->e2v[j]);
        printf("\n  E%u: ", samples[i].e12);
        for (edge_t j = hg->e2v_idx[samples[i].e12]; j < hg->e2v_idx[samples[i].e12+1]; j++)
            printf("%u ", hg->e2v[j]);
        printf("\n");
    }

    return count;
}

void data_transfer(struct dpu_set_t set, Hypergraph *hg)
{
    //read data
    load_hypergraph(hg);
    //print_hypergraph(hg, 10);  
    test_deg2e(hg, 3);           

    edge_t total = test_pattern_count_buf_print(hg, 256);
    printf("Total matches of the pattern (BUF_SIZE=%u): %u\n", 256, total);
    // node_t common_nodes[256]; 
    // edge_t cnt = test_hyperedge_intersection(hg, 0, 1, common_nodes, 256);
    // printf("Intersection size = %u\n", cnt);
    // for (edge_t i = 0; i < cnt; i++)
    //     printf("%u ", common_nodes[i]);
    // printf("\n");
 
    //alloc task
    alloc_tasks(hg);

    //transfer data
    {

    size_t size_e2v_idx = (size_t)(hg->e_cnt + 1) * sizeof(edge_t);   
    size_t size_e2v     = (size_t)(hg->e2v_size) * sizeof(node_t);     
    size_t size_adj_idx = (size_t)(hg->e_cnt + 1) * sizeof(edge_t);   
    size_t size_adj_e2e = (size_t)(hg->adj_size) * sizeof(AdjPair);    
    size_t size_deg2e   = (size_t)MAX_EDGE_SIZE * 2 * sizeof(edge_t);  

    struct dpu_set_t dpu;
    uint32_t each_dpu;
    node_t max_root_num = 0;

    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &hg->root_num[each_dpu]));
        if (hg->root_num[each_dpu] > max_root_num)
            max_root_num = (node_t)hg->root_num[each_dpu];
    }

    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "root_num", 0, sizeof(uint64_t), DPU_XFER_DEFAULT));


    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, hg->roots[each_dpu]));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "roots", 0, ALIGN8(max_root_num * sizeof(node_t)), DPU_XFER_DEFAULT));
    
    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, hg->e2v_idx));   // e2v_idx base
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "e2v_idx", 0, ALIGN8(size_e2v_idx), DPU_XFER_DEFAULT));

    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, hg->e2v));      // e2v nodes
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "e2v", 0, ALIGN8(size_e2v), DPU_XFER_DEFAULT));

    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, hg->deg2e));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "deg2e", 0, ALIGN8(size_deg2e), DPU_XFER_DEFAULT));

    // DPU_FOREACH(set, dpu, each_dpu) {
    //     DPU_ASSERT(dpu_prepare_xfer(dpu, hg->adj_idx));
    // }
    // DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "adj_idx", 0, ALIGN8(size_adj_idx), DPU_XFER_DEFAULT));

    // if (hg->adj_size > 0) {
    //     DPU_FOREACH(set, dpu, each_dpu) {
    //         DPU_ASSERT(dpu_prepare_xfer(dpu, hg->adj_e2e));
    //     }
    //     DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "adj_e2e", 0, ALIGN8(size_adj_e2e), DPU_XFER_DEFAULT));
    // }

    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &hg->e_cnt));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "e_cnt", 0, sizeof(edge_t), DPU_XFER_DEFAULT));

    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &hg->e2v_size));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "e2v_size", 0, sizeof(edge_t), DPU_XFER_DEFAULT));

    // DPU_FOREACH(set, dpu, each_dpu) {
    //     DPU_ASSERT(dpu_prepare_xfer(dpu, &hg->adj_size));
    // }
    // DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "adj_size", 0, sizeof(edge_t), DPU_XFER_DEFAULT));

    printf("Data transfer complete: e_cnt=%u e2v_size=%u adj_size=%u max_root_num=%u\n",
           (unsigned)hg->e_cnt, (unsigned)hg->e2v_size, (unsigned)hg->adj_size, (unsigned)max_root_num);
    }
}
