#include <common.h>
#include <mram.h>
#include <defs.h>
#include <stdbool.h>

extern node_t intersect_seq_buf_thresh(node_t (*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size) {
    node_t *a_buf = buf[0];
    node_t *b_buf = buf[1];

    node_t i = 0, j = 0;
    node_t ans = 0;
    node_t found = INVALID_NODE;


    if (((uint64_t)a) & 4) {
        a--;
        i = 1;
        a_size++;
    }
    if (((uint64_t)b) & 4) {
        b--;
        j = 1;
        b_size++;
    }


    mram_read(a, a_buf, ALIGN8(a_size << SIZE_NODE_T_LOG));
    mram_read(b, b_buf, ALIGN8(b_size << SIZE_NODE_T_LOG));


    while (i < a_size && j < b_size) {
        if (a_buf[i] == b_buf[j]) {
            if (ans == 0) {
                found = a_buf[i]; 
            }
            ans++;
            if (ans > 1) return INVALID_NODE; 
            i++;
            j++;
        } else if (a_buf[i] < b_buf[j]) {
            i++;
        } else {
            j++;
        }
    }

    return (ans == 1) ? found : INVALID_NODE;
}

extern bool no_intersect_seq_buf(node_t (*buf)[BUF_SIZE],
                                 node_t __mram_ptr *a, node_t a_size,
                                 node_t __mram_ptr *b, node_t b_size) {
    node_t *a_buf = buf[0];
    node_t *b_buf = buf[1];

    node_t i = 0, j = 0;


    if (((uint64_t)a) & 4) {
        a--;
        i = 1;
        a_size++;
    }
    if (((uint64_t)b) & 4) {
        b--;
        j = 1;
        b_size++;
    }

    mram_read(a, a_buf, ALIGN8(a_size << SIZE_NODE_T_LOG));
    mram_read(b, b_buf, ALIGN8(b_size << SIZE_NODE_T_LOG));


    while (i < a_size && j < b_size) {
        if (a_buf[i] == b_buf[j]) {
            return false; 
        } else if (a_buf[i] < b_buf[j]) {
            i++;
        } else {
            j++;
        }
    }

    return true; 
}

extern node_t intersect_buf_lite(node_t (*buf)[BUF_SIZE], node_t __mram_ptr *a, node_t a_size, node_t __mram_ptr *b, node_t b_size, node_t __mram_ptr *c) {
    
    node_t *a_buf = buf[0];
    node_t *b_buf = buf[1];
    node_t *c_buf = buf[2];
    node_t i = 0, j = 0, k = 0, ans = 0;
    if (((uint64_t)a) & 4) {
        a--;
        i = 1;
        a_size++;
    }
    if (((uint64_t)b) & 4) {
        b--;
        j = 1;
        b_size++;
    }
    mram_read(a, a_buf, ALIGN8(MIN(a_size, BUF_SIZE) << SIZE_NODE_T_LOG));
    mram_read(b, b_buf, ALIGN8(MIN(b_size, BUF_SIZE) << SIZE_NODE_T_LOG));

    while (i < a_size && j < b_size) {
        if (a_buf[i] == b_buf[j]) {
            c_buf[k++] = a_buf[i];
            ans++;
            i++;
            j++;
        }
        else if (a_buf[i] < b_buf[j]) {
            i++;
        }
        else {
            j++;
        }
    }
    if (k) mram_write(c_buf, c, ALIGN8(k << SIZE_NODE_T_LOG));
    return ans;
}

