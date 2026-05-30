/* XMRig
 * Copyright (c) 2018-2019 tevador     <tevador@gmail.com>
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "crypto/rx/RxDataset.h"
#include "backend/cpu/Cpu.h"
#include "base/io/log/Log.h"
#include "base/io/log/Tags.h"
#include "base/kernel/Platform.h"
#include "crypto/common/VirtualMemory.h"
#include "crypto/randomx/randomx.h"
#include "crypto/rx/RxAlgo.h"
#include "crypto/rx/RxCache.h"

#if defined(__cpp_lib_parallel_algorithm) && __cpp_lib_parallel_algorithm >= 201603L
#  include <execution>
#  include <numeric>
#  define RXS_HAS_PAR_EXEC 1
#else
#  define RXS_HAS_PAR_EXEC 0
#endif

#include <algorithm>
#include <cassert>
#include <cstring>
#include <latch>
#include <limits>
#include <span>
#include <thread>
#include <version>
#include <uv.h>
#include <vector>


namespace rxs {

namespace {

#if defined(__cpp_lib_jthread) && __cpp_lib_jthread >= 201911L
using ThreadType = std::jthread;
struct ThreadJoiner {
    explicit ThreadJoiner(std::vector<ThreadType>&) noexcept {}
};
#else
using ThreadType = std::thread;
struct ThreadJoiner {
    std::vector<ThreadType>& threads;
    ~ThreadJoiner() {
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
    }
};
#endif

constexpr uint32_t MAX_DATASET_THREADS = 256u;

[[nodiscard]] inline uint32_t optimalThreadCount(uint32_t maxAllowed) noexcept
{
    const uint32_t hw = std::max(1u,
        static_cast<uint32_t>(std::thread::hardware_concurrency()));
    return std::min(hw, maxAllowed);
}

[[nodiscard]] constexpr uint32_t alignDown(uint32_t count, uint32_t align) noexcept
{
    assert(align > 0);
    return count - (count % align);
}

void init_dataset_slice(randomx_dataset *dataset,
                         randomx_cache   *cache,
                         uint32_t         startItem,
                         uint32_t         itemCount,
                         int              priority) noexcept
{
    Platform::setThreadPriority(priority);

    if (itemCount == 0) [[unlikely]] return;

    if (Cpu::info()->hasAVX2()) {
        const uint32_t aligned = alignDown(itemCount, 5u);
        if (aligned != itemCount) {
            if (itemCount >= 5u) {
                if (aligned > 0) {
                    randomx_init_dataset(dataset, cache, startItem, aligned);
                }
                randomx_init_dataset(dataset, cache, startItem + itemCount - 5u, 5u);
            } else {
                randomx_init_dataset(dataset, cache, startItem, itemCount);
            }
            return;
        }
    }
#ifdef RXS_RISCV
    else {
        const uint32_t aligned = alignDown(itemCount, 4u);
        if (aligned != itemCount) {
            if (itemCount >= 4u) {
                if (aligned > 0) {
                    randomx_init_dataset(dataset, cache, startItem, aligned);
                }
                randomx_init_dataset(dataset, cache, startItem + itemCount - 4u, 4u);
            } else {
                randomx_init_dataset(dataset, cache, startItem, itemCount);
            }
            return;
        }
    }
#endif


    randomx_init_dataset(dataset, cache, startItem, itemCount);
}

} // anonymous namespace

RxDataset::RxDataset(bool hugePages, bool oneGbPages, bool cache,
                     RxConfig::Mode mode, uint32_t node)
    : m_mode(mode)
    , m_node(node)
{
    allocate(hugePages, oneGbPages);

    if (isOneGbPages()) {
        m_cache = new RxCache(m_memory->raw() + VirtualMemory::align(maxSize()));
        return;
    }

    if (cache) {
        m_cache = new RxCache(hugePages, node);
    }
}


RxDataset::RxDataset(RxCache *cache)
    : m_node(0)
    , m_cache(cache)
{}


RxDataset::~RxDataset()
{
    randomx_release_dataset(m_dataset);

    delete m_cache;
    delete m_memory;
}


bool RxDataset::init(const Buffer &seed, uint32_t numThreads, int priority)
{
    if (!m_cache || !m_cache->get()) {
        return false;
    }

    m_cache->init(seed);

    if (!get()) {
        return true;
    }

    const uint32_t safeThreads = std::min(
        std::max(numThreads, 1u),
        MAX_DATASET_THREADS
    );

    const uint64_t datasetItemCount = randomx_dataset_item_count();

    if (safeThreads <= 1) {
        init_dataset_slice(m_dataset, m_cache->get(), 0u,
                           static_cast<uint32_t>(datasetItemCount), priority);
        return true;
    }

    [[assume(safeThreads >= 2)]];

    std::latch done(static_cast<std::ptrdiff_t>(safeThreads));

    struct Slice { uint32_t start; uint32_t count; };
    std::vector<Slice> slices;
    slices.reserve(safeThreads);
    for (uint32_t i = 0; i < safeThreads; ++i) {
        const uint64_t a64 = (datasetItemCount * uint64_t{i})       / safeThreads;
        const uint64_t b64 = (datasetItemCount * (uint64_t{i} + 1u)) / safeThreads;
        slices.push_back({
            static_cast<uint32_t>(a64),
            static_cast<uint32_t>(b64 - a64)
        });
    }

    std::vector<ThreadType> threads;
    ThreadJoiner joiner{threads};
    threads.reserve(safeThreads);
    for (const auto &[start, count] : slices) {
        threads.emplace_back([this, start, count, priority, &done]() noexcept {
            init_dataset_slice(m_dataset, m_cache->get(), start, count, priority);
            done.count_down();
        });
    }

    done.wait();
    return true;
}


bool RxDataset::isHugePages() const
{
    return m_memory && m_memory->isHugePages();
}


bool RxDataset::isOneGbPages() const
{
    return m_memory && m_memory->isOneGbPages();
}


HugePagesInfo RxDataset::hugePages(bool includeCache) const
{
    auto pages = m_memory ? m_memory->hugePages() : HugePagesInfo();

    if (includeCache && m_cache) {
        pages += m_cache->hugePages();
    }

    return pages;
}


size_t RxDataset::size(bool includeCache) const
{
    size_t total = 0;

    if (m_dataset) {
        total += maxSize();
    }

    if (includeCache && m_cache) {
        total += RxCache::maxSize();
    }

    return total;
}


uint8_t *RxDataset::tryAllocateScrathpad()
{
    auto *p = static_cast<uint8_t *>(raw());
    if (!p) [[unlikely]] {
        return nullptr;
    }

    const size_t offset =
        m_scratchpadOffset.fetch_add(RANDOMX_SCRATCHPAD_L3_MAX_SIZE,
                                     std::memory_order_relaxed);

    constexpr size_t SCRATCH = RANDOMX_SCRATCHPAD_L3_MAX_SIZE;
    const bool exhausted =
        (m_scratchpadLimit < SCRATCH) ||
        (offset > m_scratchpadLimit - SCRATCH);

    if (exhausted) [[unlikely]] {
        return nullptr;
    }

    return p + offset;
}


void *RxDataset::raw() const
{
    return m_dataset ? randomx_get_dataset_memory(m_dataset) : nullptr;
}

std::span<const uint8_t> RxDataset::rawSpan() const noexcept
{
    const auto *p = static_cast<const uint8_t *>(raw());
    if (!p) [[unlikely]] return {};
    return { p, maxSize() };
}

void RxDataset::parallelCopy(uint8_t *dst, const uint8_t *src, size_t n)
{
    if (n == 0) [[unlikely]] return;

    const uint32_t numThreads = optimalThreadCount(8u);

    if (numThreads <= 1) {
        std::memcpy(dst, src, n);
        return;
    }

    const size_t chunkSize = (n + numThreads - 1) / numThreads;

#if RXS_HAS_PAR_EXEC
    std::vector<uint32_t> indices(numThreads);
    std::iota(indices.begin(), indices.end(), 0u);

    std::for_each(std::execution::par_unseq,
                  indices.begin(), indices.end(),
                  [dst, src, n, chunkSize](uint32_t i) noexcept {
                      const size_t offset = size_t{i} * chunkSize;
                      if (offset >= n) return;
                      std::memcpy(dst + offset,
                                  src + offset,
                                  std::min(chunkSize, n - offset));
                  });
#else
    std::latch done(static_cast<std::ptrdiff_t>(numThreads));

    std::vector<ThreadType> threads;
    ThreadJoiner joiner{threads};
    threads.reserve(numThreads);

    for (uint32_t i = 0; i < numThreads; ++i) {
        const size_t offset = size_t{i} * chunkSize;
        if (offset >= n) {
            done.count_down();
            continue;
        }
        const size_t bytes = std::min(chunkSize, n - offset);
        threads.emplace_back([dst, src, offset, bytes, &done]() noexcept {
            std::memcpy(dst + offset, src + offset, bytes);
            done.count_down();
        });
    }

    done.wait();
#endif
}

void RxDataset::setRaw(const void *rawSrc)
{
    if (!m_dataset) [[unlikely]] return;

    if (!rawSrc) [[unlikely]] return;

    auto *dst = static_cast<uint8_t *>(randomx_get_dataset_memory(m_dataset));
    const auto *src = static_cast<const uint8_t *>(rawSrc);

    if (dst == src) [[unlikely]] return;

    try {
        RxDataset::parallelCopy(dst, src, maxSize());
    } catch (...) {
        std::memcpy(dst, src, maxSize());
    }
}

void RxDataset::allocate(bool hugePages, bool oneGbPages)
{
    if (m_mode == RxConfig::LightMode) {
        LOG_ERR(CLEAR "%s" RED_BOLD_S "fast RandomX mode disabled by config",
                Tags::randomx());
        return;
    }

    if (m_mode == RxConfig::AutoMode &&
        uv_get_total_memory() < (maxSize() + RxCache::maxSize())) {
        LOG_ERR(CLEAR "%s" RED_BOLD_S "not enough memory for RandomX dataset",
                Tags::randomx());
        return;
    }

    m_memory = new VirtualMemory(maxSize(), hugePages, oneGbPages, false,
                                  m_node, VirtualMemory::kDefaultHugePageSize);

    if (m_memory->isOneGbPages()) {
        m_scratchpadOffset = maxSize() + RANDOMX_CACHE_MAX_SIZE;
        m_scratchpadLimit = m_memory->capacity();
    }

    uint8_t *memRaw = m_memory->raw();
    if (!memRaw) [[unlikely]] {
        LOG_ERR(CLEAR "%s" RED_BOLD_S "VirtualMemory allocation returned null pointer",
                Tags::randomx());
        return;
    }

    m_dataset = randomx_create_dataset(memRaw);

#ifdef RXS_OS_LINUX
    if (oneGbPages && !isOneGbPages()) {
        LOG_ERR(CLEAR "%s" RED_BOLD_S "failed to allocate RandomX dataset using 1GB pages",
                Tags::randomx());
    }
#endif
}

} // namespace rxs
