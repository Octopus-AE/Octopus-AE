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



void build_h2h_index(Hypergraph *hg) {
    printf("Building H2H index (Indexed by Neighbor Degree, Fixed Header)...\n");
    
    hg->h2h_total_size = 0;
    

    edge_t *temp_buckets[MAX_EDGE_SIZE + 1]; 
    edge_t bucket_counts[MAX_EDGE_SIZE + 1];
    

    edge_t max_neighbors = hg->e_cnt; 
    for(int i=0; i <= MAX_EDGE_SIZE; i++) {
        temp_buckets[i] = (edge_t*)malloc(max_neighbors * sizeof(edge_t));
    }

    for (edge_t i = 0; i < hg->e_cnt; i++) {

        memset(bucket_counts, 0, sizeof(bucket_counts));

     
        edge_t start_i = hg->e2v_idx[i];
        edge_t end_i   = hg->e2v_idx[i+1];
        
       
        for (edge_t j = 0; j < hg->e_cnt; j++) {
            if (i == j) continue;


            bool is_intersect = false;
            edge_t start_j = hg->e2v_idx[j];
            edge_t end_j   = hg->e2v_idx[j+1];
            
           
            edge_t ia = start_i, ib = start_j;
            while (ia < end_i && ib < end_j) {
                if (hg->e2v[ia] == hg->e2v[ib]) { is_intersect = true; break; }
                else if (hg->e2v[ia] < hg->e2v[ib]) ia++;
                else ib++;
            }

            if (is_intersect) {
                
                edge_t deg_j = end_j - start_j;

               
                if (deg_j > 0 && deg_j <= MAX_EDGE_SIZE) {
                    temp_buckets[deg_j][bucket_counts[deg_j]++] = j;
                }
            }
        }

 
        edge_t header_size = MAX_EDGE_SIZE + 1; 
        edge_t data_size = 0;
        for(int d=1; d <= MAX_EDGE_SIZE; d++) data_size += bucket_counts[d];

      
        if (hg->h2h_total_size + header_size + data_size >= H2H_CAPACITY) {
            fprintf(stderr, ANSI_COLOR_RED "Error: H2H buffer overflow at edge %u! (Current: %u, Cap: %u)\n" ANSI_COLOR_RESET, 
                    i, hg->h2h_total_size + header_size + data_size, H2H_CAPACITY);
            fprintf(stderr, ANSI_COLOR_RED "H2H optimization cannot be enabled with current capacity. Please Disable H2H\n" ANSI_COLOR_RESET);
            exit(1); 
        }

        
        hg->h2h_offset[i] = hg->h2h_total_size;
        edge_t *block_base = &hg->h2h_buffer[hg->h2h_total_size];


        edge_t current_relative_pos = header_size;
        for (int d = 1; d <= MAX_EDGE_SIZE; d++) {
            block_base[d-1] = current_relative_pos; 
            current_relative_pos += bucket_counts[d];
        }
        block_base[MAX_EDGE_SIZE] = current_relative_pos; 

        for (int d = 1; d <= MAX_EDGE_SIZE; d++) {
            edge_t count = bucket_counts[d];
            if (count > 0) {
                edge_t start_pos = block_base[d-1];
                memcpy(&block_base[start_pos], temp_buckets[d], count * sizeof(edge_t));
            }
        }

        hg->h2h_total_size += (header_size + data_size);
    }

    
    for(int i=0; i <= MAX_EDGE_SIZE; i++) free(temp_buckets[i]);
    
    printf("H2H Index built (By Degree). Total size: %u\n", hg->h2h_total_size);
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

    hg->deg2e[last_deg][1] = e_cnt;
    printf("Hypergraph loaded: %u edges, %u nodes in e2v\n", e_cnt, hg->e2v_size);

#if defined(H2H)
    build_h2h_index(hg);
#endif

    if (e_cnt > BITMAP_ROW) {
        printf(ANSI_COLOR_RED "Skip bitmap init: edge count %u > max row %u\n" ANSI_COLOR_RESET,
               e_cnt, BITMAP_ROW);
        return 0;
    }

    printf("Initializing adjacency bitmap (%u × %u)...\n", e_cnt, e_cnt);
    memset(hg->bitmap, 0, sizeof(hg->bitmap));


    for (edge_t i = 0; i < e_cnt; i++) {
        edge_t start_i = hg->e2v_idx[i];
        edge_t end_i   = hg->e2v_idx[i+1];

        for (edge_t j = i + 1; j < e_cnt; j++) {
            edge_t start_j = hg->e2v_idx[j];
            edge_t end_j   = hg->e2v_idx[j+1];


            node_t *a = &hg->e2v[start_i];
            node_t *b = &hg->e2v[start_j];
            edge_t ia = 0, ib = 0;
            int intersect = 0;

            while (ia < end_i - start_i && ib < end_j - start_j) {
                if (a[ia] == b[ib]) { intersect = 1; break; }
                if (a[ia] < b[ib]) ia++; else ib++;
            }

            if (intersect) {
                hg->bitmap[i][j / (sizeof(bit_t)*8)] |= ((bit_t)1 << (j % (sizeof(bit_t)*8)));
                hg->bitmap[j][i / (sizeof(bit_t)*8)] |= ((bit_t)1 << (i % (sizeof(bit_t)*8)));
            }
        }
    }

    printf(ANSI_COLOR_GREEN "Adjacency bitmap initialized.\n" ANSI_COLOR_RESET);
    


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


    for (edge_t e = start; e < end; e++) {
        unsigned dpu = (e - start) % NR_DPUS;
        hg->roots[dpu][hg->root_num[dpu]++] = (node_t)e;  
    }

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


    edge_t start3 = hg->deg2e[5][0], end3 = hg->deg2e[5][1];

    edge_t start7 = hg->deg2e[4][0], end7 = hg->deg2e[4][1];

    edge_t start12 = hg->deg2e[3][0], end12 = hg->deg2e[3][1];

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

    clock_t start, end;
    double cpu_time_used;

    start = clock(); 
    
    load_hypergraph(hg);
    //print_hypergraph(hg, 10);   
    //test_deg2e(hg, 5);           

    // edge_t total = test_pattern_count_buf_print(hg, 256);
    // printf("Total matches of the pattern (BUF_SIZE=%u): %u\n", 256, total);
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
    size_t size_deg2e   = (size_t)MAX_EDGE_SIZE * 2 * sizeof(edge_t);   

    
    size_t total = size_e2v_idx + size_e2v;

    double total_MB  = (double)total / 1000.0 / 1000.0;
    printf(" TOTAL   = %.3f MB\n", total_MB);
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


#if defined(H2H)

    if (hg->h2h_total_size >= H2H_CAPACITY) {
        printf(ANSI_COLOR_RED "ERROR: Host H2H buffer overflow detected during build! (Size >= %u)\n", H2H_CAPACITY);
        printf("Transmission Aborted: Data is incomplete.\n" ANSI_COLOR_RESET);
        return; 
    }

   
    size_t size_h2h_offset = (size_t)(hg->e_cnt + 1) * sizeof(edge_t);
    size_t size_h2h_buffer = (size_t)(hg->h2h_total_size) * sizeof(edge_t);

    printf("H2H Index Size: %.2f MB (Offset) + %.2f MB (Buffer). Transferring...\n",
           (double)size_h2h_offset/1024.0/1024.0, 
           (double)size_h2h_buffer/1024.0/1024.0);

    // 3. 执行传输
    DPU_FOREACH(set, dpu, each_dpu) { 
        DPU_ASSERT(dpu_prepare_xfer(dpu, hg->h2h_offset)); 
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "h2h_offset", 0, ALIGN8(size_h2h_offset), DPU_XFER_DEFAULT));

    DPU_FOREACH(set, dpu, each_dpu) { 
        DPU_ASSERT(dpu_prepare_xfer(dpu, hg->h2h_buffer)); 
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "h2h_buffer", 0, ALIGN8(size_h2h_buffer), DPU_XFER_DEFAULT));

    DPU_FOREACH(set, dpu, each_dpu) { 
        DPU_ASSERT(dpu_prepare_xfer(dpu, &hg->h2h_total_size)); 
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "h2h_total_size", 0, sizeof(edge_t), DPU_XFER_DEFAULT));

#else
   
    if (hg->e_cnt <= BITMAP_ROW) {
        size_t size_bitmap_bytes = 0;
        size_t bitmap_words = (size_t)BITMAP_COL * (size_t)hg->e_cnt;
        size_bitmap_bytes = bitmap_words * sizeof(bit_t);

        DPU_FOREACH(set, dpu, each_dpu) {
            DPU_ASSERT(dpu_prepare_xfer(dpu, &hg->bitmap[0][0]));
        }
        DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "bitmap", 0, ALIGN8(size_bitmap_bytes), DPU_XFER_DEFAULT));
    }
#endif

    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &hg->e_cnt));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "e_cnt", 0, sizeof(edge_t), DPU_XFER_DEFAULT));

    DPU_FOREACH(set, dpu, each_dpu) {
        DPU_ASSERT(dpu_prepare_xfer(dpu, &hg->e2v_size));
    }
    DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "e2v_size", 0, sizeof(edge_t), DPU_XFER_DEFAULT));

    printf("Data transfer complete: e_cnt=%u e2v_size=%u max_root_num=%u\n",
           (unsigned)hg->e_cnt, (unsigned)hg->e2v_size, (unsigned)max_root_num);

    end = clock(); 

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC; 
    printf("Time used: %f seconds\n", cpu_time_used);
    }
}
