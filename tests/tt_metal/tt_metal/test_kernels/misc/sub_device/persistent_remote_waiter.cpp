// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>

#include "dataflow_api.h"

void kernel_main() {
    uint32_t sem_addr = get_arg_val<uint32_t>(0);
    uint32_t num_inc = get_arg_val<uint32_t>(1);
    uint32_t send_sync_core_x = get_arg_val<uint32_t>(2);
    uint32_t send_sync_core_y = get_arg_val<uint32_t>(3);
    uint32_t recv_sync_core_x = get_arg_val<uint32_t>(4);
    uint32_t recv_sync_core_y = get_arg_val<uint32_t>(5);
    uint32_t local_read_addr = get_arg_val<uint32_t>(6);

    volatile tt_l1_ptr uint32_t* local_read_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(local_read_addr);

    uint64_t noc_remote_send_sem_addr = get_noc_addr(send_sync_core_x, send_sync_core_y, sem_addr);
    noc_semaphore_inc(noc_remote_send_sem_addr, 1);

    uint64_t noc_remote_recv_sem_addr = get_noc_addr(recv_sync_core_x, recv_sync_core_y, sem_addr);
    volatile tt_l1_ptr uint32_t* sem = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(sem_addr);
    uint32_t num_read = 0;
    do {
        noc_async_read(noc_remote_recv_sem_addr, local_read_addr, 4);
        noc_async_read_barrier();
        invalidate_l1_cache();
        num_read = *local_read_ptr;
    } while (num_read != num_inc);

    noc_semaphore_inc(noc_remote_recv_sem_addr, -num_inc);
    noc_async_atomic_barrier();
}
