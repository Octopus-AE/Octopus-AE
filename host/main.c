#include <common.h>
#include <timer.h>
#include <assert.h>
#include <stdio.h>
#include <dpu.h>

extern void data_transfer(struct dpu_set_t set, Hypergraph *hg);

Hypergraph *hg;
ans_t result[IDX_CAPACITY];
uint64_t cycle_ct[IDX_CAPACITY];
uint64_t cycle_ct_dpu[NR_DPUS][NR_TASKLETS];
Timer timer;

int main() {
    printf("NR_DPUS: %u, NR_TASKLETS: %u, DPU_BINARY: %s, PATTERN: %s\n", NR_DPUS, NR_TASKLETS, DPU_BINARY, PATTERN_NAME);

    struct dpu_set_t set, dpu;
    DPU_ASSERT(dpu_alloc(NR_DPUS, NULL, &set));

    // task allocation and data partition
    printf("Selecting graph: %s\n", DATA_PATH);

    hg = malloc(sizeof(Hypergraph));
    data_transfer(set, hg);

    // run it on CPU to get the answer
    // run it on DPU
    start(&timer, 0, 0);
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    stop(&timer, 0);
    printf("DPU ");
    print(&timer, 0, 1);

    // collect answer and cycle count
    bool fine = true;
    bool finished, failed;
    uint32_t each_dpu;

    ans_t total_ans = 0;
    uint64_t total_cycle_ct = 0;
    
    DPU_FOREACH(set, dpu, each_dpu) {
        // check status
        DPU_ASSERT(dpu_status(dpu, &finished, &failed));
        if (failed) {
            printf("DPU: %u failed\n", each_dpu);
            fine = false;
            break;
        }

        // collect answer
        uint64_t *dpu_ans = (uint64_t *)malloc(ALIGN8(hg->root_num[each_dpu] * sizeof(uint64_t)));
        DPU_ASSERT(dpu_copy_from(dpu, "ans", 0, dpu_ans, ALIGN8(hg->root_num[each_dpu] * sizeof(uint64_t))));
        for (node_t k = 0; k < hg->root_num[each_dpu]; k++) {
            node_t cur_root = hg->roots[each_dpu][k];
            result[cur_root] = dpu_ans[k];
            total_ans += dpu_ans[k];
        }
        free(dpu_ans);

        //cycyle test

        uint64_t *dpu_cycle_ct = (uint64_t *)malloc(ALIGN8(hg->root_num[each_dpu] * sizeof(uint64_t)));
        DPU_ASSERT(dpu_copy_from(dpu, "cycle_ct", 0, dpu_cycle_ct, ALIGN8(hg->root_num[each_dpu] * sizeof(uint64_t))));
        for (node_t k = 0, cur_thread = 0; k < hg->root_num[each_dpu]; k++) {
            node_t cur_root = hg->roots[each_dpu][k];
            cycle_ct[cur_root] = dpu_cycle_ct[k];
            cycle_ct_dpu[each_dpu][cur_thread] += dpu_cycle_ct[k];
            cur_thread = (cur_thread + 1) % NR_TASKLETS;
            total_cycle_ct += dpu_cycle_ct[k];

        }

        free(dpu_cycle_ct);

    }
    printf("DPU ans: %lu\n", total_ans);

        // find the maximum per-DPU per-thread cycles (the "bottleneck")
    uint64_t max_cycle = 0;
    for (uint32_t dpu_id = 0; dpu_id < NR_DPUS; dpu_id++) {
        for (uint32_t t = 0; t < NR_TASKLETS; t++) {
            if (cycle_ct_dpu[dpu_id][t] > max_cycle) {
                max_cycle = cycle_ct_dpu[dpu_id][t];
            }
        }
    }

    // output to file
    FILE *fp = fopen("dpu_cycle_report.txt", "w");
    if (!fp) {
        perror("fopen");
    } else {
        for (uint32_t dpu_id = 0; dpu_id < NR_DPUS; dpu_id++) {
            for (uint32_t t = 0; t < NR_TASKLETS; t++) {
                fprintf(fp, "DPU %u, Thread %u, Cycles: %llu\n",
                        dpu_id, t, (unsigned long long)cycle_ct_dpu[dpu_id][t]);
            }
        }
        fprintf(fp, "Bottleneck (max cycle): %llu\n", (unsigned long long)max_cycle);
        fclose(fp);
        printf("Cycle report written to dpu_cycle_report.txt\n");
    }

    // output result to file
    if (fine) printf(ANSI_COLOR_GREEN "All fine\n" ANSI_COLOR_RESET);
    else printf(ANSI_COLOR_RED "Some failed\n" ANSI_COLOR_RESET);

    free(hg);
    DPU_ASSERT(dpu_free(set));
    return 0;
}