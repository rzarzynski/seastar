/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright (C) 2016 ScyllaDB.
 */

#pragma once
#include <atomic>

namespace seastar {

namespace internal {

struct preemption_monitor {
    // We preempt when head != tail
    // This happens to match the Linux aio completion ring, so we can have the
    // kernel preempt a task by queuing a completion event to an io_context.
    std::atomic<uint32_t> head;
    std::atomic<uint32_t> tail;
};

} // namespace internal

extern __thread const internal::preemption_monitor* g_need_preempt;

namespace internal {

#ifndef SEASTAR_DEBUG
inline bool _need_preempt() {
    // prevent compiler from eliminating loads in a loop
    std::atomic_signal_fence(std::memory_order_seq_cst);
    auto np = g_need_preempt;
    // We aren't reading anything from the ring, so we don't need
    // any barriers.
    auto head = np->head.load(std::memory_order_relaxed);
    auto tail = np->tail.load(std::memory_order_relaxed);
    // Possible optimization: read head and tail in a single 64-bit load,
    // and find a funky way to compare the two 32-bit halves.
    return __builtin_expect(head != tail, false);
}
#endif // not SEASTAR_DEBUG

} // namespace internal

} // namespace seastar

#ifndef SEASTAR_DEBUG
#define need_preempt() __builtin_expect(::seastar::internal::_need_preempt(), false)
#else
#define need_preempt() true
#endif // not SEASTAR_DEBUG
