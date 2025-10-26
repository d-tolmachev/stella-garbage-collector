#ifndef GARBAGE_COLLECTOR_H
#define GARBAGE_COLLECTOR_H

#include <deque>
#include <memory>

#include "runtime.h"

namespace stella {

    class garbage_collector {
    public:
        garbage_collector() noexcept;

        garbage_collector(const garbage_collector& other) = delete;

        garbage_collector(garbage_collector&& other) noexcept;

        garbage_collector& operator=(const garbage_collector& rhs) = delete;

        garbage_collector& operator=(garbage_collector&& rhs) noexcept;

        ~garbage_collector() = default;

        [[nodiscard]] bool inited() const noexcept;

        void init();

        void* allocate(size_t size);

        void read_barrier(stella_object* object, size_t field_index);

        void write_barrier() noexcept;

        void push_root(void** object);

        void pop_root(void** object);

        void print_allocation_statistics() const noexcept;

        void print_state() const noexcept;

        void print_roots() const noexcept;

        void swap(garbage_collector& other) noexcept;

    private:
        constexpr static size_t REGION_SIZE = MAX_ALLOC_SIZE - MAX_ALLOC_SIZE % alignof(stella_object);
        constexpr static size_t RECORDS_TO_FORWARD = 16;
        std::unique_ptr<std::byte[]> heap_;
        std::deque<void**> roots_;
        std::byte* from_space_;
        std::byte* to_space_;
        std::byte* scan_;
        std::byte* next_;
        std::byte* limit_;
        size_t total_allocated_bytes_cnt_;
        size_t total_allocated_objects_cnt_;
        size_t current_allocated_bytes_cnt_;
        size_t current_allocated_objects_cnt_;
        size_t total_cycles_cnt_;
        size_t maximum_resident_bytes_cnt_;
        size_t maximum_resident_objects_cnt_;
        size_t reads_cnt_;
        size_t writes_cnt_;
        size_t read_barrier_triggers_cnt_;
        constexpr static size_t write_barrier_triggers_cnt_ = 0;

        static bool points_to(stella_object* p, std::byte* space) noexcept;

        void collect();

        void incremental_forward();

        stella_object* forward(stella_object* p);

        void chase(stella_object* p);
    };

}

#endif
