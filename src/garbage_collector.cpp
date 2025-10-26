#include "gc.h"
#include "garbage_collector.h"

#include <iomanip>
#include <iostream>
#include <new>
#include <utility>

#include "runtime.h"

namespace stella {

    garbage_collector::garbage_collector() noexcept
        : from_space_(nullptr)
        , to_space_(nullptr)
        , scan_(nullptr)
        , next_(nullptr)
        , limit_(nullptr)
        , total_allocated_bytes_cnt_(0)
        , total_allocated_objects_cnt_(0)
        , current_allocated_bytes_cnt_(0)
        , current_allocated_objects_cnt_(0)
        , total_cycles_cnt_(0)
        , maximum_resident_bytes_cnt_(0)
        , maximum_resident_objects_cnt_(0)
        , reads_cnt_(0)
        , writes_cnt_(0)
        , read_barrier_triggers_cnt_(0) {
    }

    garbage_collector::garbage_collector(garbage_collector&& other) noexcept
        : from_space_(std::exchange(other.from_space_, nullptr))
        , to_space_(std::exchange(other.to_space_, nullptr))
        , scan_(std::exchange(other.scan_, nullptr))
        , next_(std::exchange(other.next_, nullptr))
        , limit_(std::exchange(other.limit_, nullptr))
        , total_allocated_bytes_cnt_(std::exchange(other.total_allocated_bytes_cnt_, 0))
        , total_allocated_objects_cnt_(std::exchange(other.total_allocated_objects_cnt_, 0))
        , current_allocated_bytes_cnt_(std::exchange(other.current_allocated_bytes_cnt_, 0))
        , current_allocated_objects_cnt_(std::exchange(other.current_allocated_objects_cnt_, 0))
        , total_cycles_cnt_(std::exchange(other.total_cycles_cnt_, 0))
        , maximum_resident_bytes_cnt_(std::exchange(other.maximum_resident_bytes_cnt_, 0))
        , maximum_resident_objects_cnt_(std::exchange(other.maximum_resident_objects_cnt_, 0))
        , reads_cnt_(std::exchange(other.reads_cnt_, 0))
        , writes_cnt_(std::exchange(other.writes_cnt_, 0))
        , read_barrier_triggers_cnt_(std::exchange(other.read_barrier_triggers_cnt_, 0)) {
    }

    garbage_collector& garbage_collector::operator=(garbage_collector&& rhs) noexcept {
        if (this != std::addressof(rhs)) {
            garbage_collector tmp(std::move(rhs));
            swap(tmp);
        }
        return *this;
    }

    [[nodiscard]] bool garbage_collector::inited() const noexcept {
        return heap_ != nullptr;
    }

    void garbage_collector::init() {
        heap_.reset(new (std::align_val_t{alignof(stella_object)}) std::byte[2 * REGION_SIZE]);
        from_space_ = heap_.get();
        to_space_ = heap_.get() + REGION_SIZE;
        scan_ = from_space_;
        next_ = from_space_;
        limit_ = from_space_ + REGION_SIZE;
    }

    void* garbage_collector::allocate(size_t size) {
        if (next_ >= limit_ - size) {
            collect();
        } else {
            incremental_forward();
        }
        if (next_ >= limit_ - size) {
            throw std::bad_alloc();
        }
        total_allocated_bytes_cnt_ += size;
        ++total_allocated_objects_cnt_;
        current_allocated_bytes_cnt_ += size;
        ++current_allocated_objects_cnt_;
        if (current_allocated_bytes_cnt_ > maximum_resident_bytes_cnt_) {
            maximum_resident_bytes_cnt_ = current_allocated_bytes_cnt_;
            maximum_resident_objects_cnt_ = current_allocated_objects_cnt_;
        }
        limit_ -= size;
        return limit_;
    }

    void garbage_collector::read_barrier(stella_object* object, size_t field_index) {
        ++reads_cnt_;
        object->object_fields[field_index] = static_cast<void*>(forward(static_cast<stella_object*>(object->object_fields[field_index])));
    }

    void garbage_collector::write_barrier() noexcept {
        ++writes_cnt_;
    }

    void garbage_collector::push_root(void** object) {
        roots_.push_back(object);
    }

    void garbage_collector::pop_root(void** object) {
        if (roots_.back() != object) {
            throw std::invalid_argument("The argument is not at the top of the stack");
        }
        roots_.pop_back();
    }

    void garbage_collector::print_allocation_statistics() const noexcept {
        std::cout << "Total memory allocation: " << total_allocated_bytes_cnt_ << " bytes (" << total_allocated_objects_cnt_ << " objects)" << std::endl;
        std::cout << "Total GC invocation: " << total_cycles_cnt_ << " cycles" << std::endl;
        std::cout << "Maximum residency: " << maximum_resident_bytes_cnt_ << " bytes (" << maximum_resident_objects_cnt_ << " objects)" << std::endl;
        std::cout << "Total memory use: " << reads_cnt_ << " reads and " << writes_cnt_ << " writes" << std::endl;
        std::cout << "Total barriers triggering: " << read_barrier_triggers_cnt_ << " read barriers and " << write_barrier_triggers_cnt_ << " write_barriers" << std::endl;
    }

    void garbage_collector::print_state() const noexcept {
        std::cout << "Heap state:" << std::endl;
        bool first = true;
        std::cout << "From-space: " << REGION_SIZE << " bytes at " << std::hex << std::showbase << static_cast<void*>(from_space_) << std::noshowbase << std::dec << std::endl;
        stella_object* object = static_cast<stella_object*>(static_cast<void*>(from_space_));
        while (static_cast<std::byte*>(static_cast<void*>(object)) < next_) {
            if (!first) {
                std::cout << ", ";
            }
            first = false;
            std::cout << "Stella object with " << static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(object->object_header)) << " fields at " << std::hex << std::showbase << static_cast<void*>(object) << std::noshowbase << std::dec;
            object = static_cast<stella_object*>(static_cast<void*>(static_cast<std::byte*>(static_cast<void*>(object)) + sizeof(stella_object) + static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(object->object_header)) * sizeof(void*)));
        }
        object = static_cast<stella_object*>(static_cast<void*>(limit_));
        while (static_cast<std::byte*>(static_cast<void*>(object)) < from_space_ + REGION_SIZE) {
            if (!first) {
                std::cout << ", ";
            }
            first = false;
            std::cout << "Stella object with " << static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(object->object_header)) << " fields at " << std::hex << std::showbase << static_cast<void*>(object) << std::noshowbase << std::dec;
            object = static_cast<stella_object*>(static_cast<void*>(static_cast<std::byte*>(static_cast<void*>(object)) + sizeof(stella_object) + static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(object->object_header)) * sizeof(void*)));
        }
        std::cout << std::endl;
        std::cout << "To-space: " << REGION_SIZE << " bytes at " << std::hex << std::showbase << static_cast<void*>(to_space_) << std::noshowbase << std::dec << std::endl;
        std::cout << "GC variable values: " << std::hex << std::showbase << "scan = " << static_cast<void*>(scan_) << ", next = " << static_cast<void*>(next_) << ", limit = " << static_cast<void*>(limit_) << std::noshowbase << std::dec << std::endl;
        print_roots();
        std::cout << "Current memory allocation: " << current_allocated_bytes_cnt_ << " bytes (" << current_allocated_objects_cnt_ << " objects)" << std::endl;
        std::cout << "Current memory available: " << (limit_ - next_) << " bytes" << std::endl;
    }

    void garbage_collector::print_roots() const noexcept {
        bool first = true;
        std::cout << std::hex << std::showbase;
        std::cout << "Set of roots: ";
        for (void** root : roots_) {
            if (!first) {
                std::cout << ", ";
            }
            first = false;
            std::cout << *root;
        }
        std::cout << std::endl;
        std::cout << std::noshowbase << std::dec;
    }

    void garbage_collector::swap(garbage_collector& other) noexcept {
        std::swap(heap_, other.heap_);
        std::swap(roots_, other.roots_);
        std::swap(from_space_, other.from_space_);
        std::swap(to_space_, other.to_space_);
        std::swap(scan_, other.scan_);
        std::swap(next_, other.next_);
        std::swap(limit_, other.limit_);
        std::swap(total_allocated_bytes_cnt_, other.total_allocated_bytes_cnt_);
        std::swap(total_allocated_objects_cnt_, other.total_allocated_objects_cnt_);
        std::swap(current_allocated_bytes_cnt_, other.current_allocated_bytes_cnt_);
        std::swap(current_allocated_objects_cnt_, other.current_allocated_objects_cnt_);
        std::swap(total_cycles_cnt_, other.total_cycles_cnt_);
        std::swap(maximum_resident_bytes_cnt_, other.maximum_resident_bytes_cnt_);
        std::swap(maximum_resident_objects_cnt_, other.maximum_resident_objects_cnt_);
        std::swap(reads_cnt_, other.reads_cnt_);
        std::swap(writes_cnt_, other.writes_cnt_);
        std::swap(read_barrier_triggers_cnt_, other.read_barrier_triggers_cnt_);
    }

    bool garbage_collector::points_to(stella_object* p, std::byte* space) noexcept {
        return static_cast<std::byte*>(static_cast<void*>(p)) >= space && static_cast<std::byte*>(static_cast<void*>(p)) < space + REGION_SIZE;
    }

    void garbage_collector::collect() noexcept {
        current_allocated_bytes_cnt_ = 0;
        current_allocated_objects_cnt_ = 0;
        ++total_cycles_cnt_;
        std::swap(from_space_, to_space_);
        scan_ = from_space_;
        next_ = from_space_;
        limit_ = from_space_ + REGION_SIZE;
        for (void** root : roots_) {
            *root = static_cast<void*>(forward(static_cast<stella_object*>(*root)));
        }
    }

    void garbage_collector::incremental_forward() noexcept {
        size_t forwarded_records = 0;
        while (scan_ < next_ && forwarded_records < RECORDS_TO_FORWARD) {
            stella_object* object = static_cast<stella_object*>(static_cast<void*>(scan_));
            for (size_t i = 0; i < static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(object->object_header)); ++i) {
                object->object_fields[i] = static_cast<void*>(forward(static_cast<stella_object*>(object->object_fields[i])));
            }
            scan_ += sizeof(stella_object) + static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(object->object_header)) * sizeof(void*);
            ++forwarded_records;
        }
    }

    stella_object* garbage_collector::forward(stella_object* p) noexcept {
        if (points_to(p, to_space_)) {
            if (points_to(static_cast<stella_object*>(p->object_fields[0]), from_space_)) {
                return static_cast<stella_object*>(p->object_fields[0]);
            } else {
                chase(p);
                return static_cast<stella_object*>(p->object_fields[0]);
            }
        } else {
            return p;
        }
    }

    void garbage_collector::chase(stella_object* p) noexcept {
        do {
            current_allocated_bytes_cnt_ += sizeof(stella_object) + static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(p->object_header)) * sizeof(void*);
            ++current_allocated_objects_cnt_;
            stella_object* q = static_cast<stella_object*>(static_cast<void*>(next_));
            next_ += sizeof(stella_object) + static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(p->object_header)) * sizeof(void*);
            stella_object* r = nullptr;
            q->object_header = p->object_header;
            for (size_t i = 0; i < static_cast<size_t>(STELLA_OBJECT_HEADER_FIELD_COUNT(p->object_header)); ++i) {
                q->object_fields[i] = p->object_fields[i];
                if (points_to(static_cast<stella_object*>(q->object_fields[i]), to_space_) && !points_to(static_cast<stella_object*>(static_cast<stella_object*>(q->object_fields[i])->object_fields[0]), from_space_)) {
                    r = static_cast<stella_object*>(q->object_fields[i]);
                }
            }
            p->object_fields[0] = static_cast<void*>(q);
            p = r;
        } while (p);
    }

    garbage_collector gc;

}

void* gc_alloc(size_t size_in_bytes) {
    if (!stella::gc.inited()) {
        stella::gc.init();
    }
    return stella::gc.allocate(size_in_bytes);
}

void gc_read_barrier(void *object, int field_index) {
    if (!stella::gc.inited()) {
        stella::gc.init();
    }
    stella::gc.read_barrier(static_cast<stella_object*>(object), static_cast<size_t>(field_index));
}

void gc_write_barrier(void *object, int field_index, void *contents) {
    static_cast<void>(object);
    static_cast<void>(field_index);
    static_cast<void>(contents);
    if (!stella::gc.inited()) {
        stella::gc.init();
    }
    stella::gc.write_barrier();
}

void gc_push_root(void **object) {
    if (!stella::gc.inited()) {
        stella::gc.init();
    }
    stella::gc.push_root(object);
}

void gc_pop_root(void **object) {
    if (!stella::gc.inited()) {
        stella::gc.init();
    }
    stella::gc.pop_root(object);
}

void print_gc_alloc_stats() {
    if (!stella::gc.inited()) {
        stella::gc.init();
    }
    stella::gc.print_allocation_statistics();
}

void print_gc_state() {
    if (!stella::gc.inited()) {
        stella::gc.init();
    }
    stella::gc.print_state();
}

void print_gc_roots() {
    if (!stella::gc.inited()) {
        stella::gc.init();
    }
    stella::gc.print_roots();
}
