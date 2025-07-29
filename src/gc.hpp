#pragma once

#include <list>
#include <unordered_set>

#include "value.hpp"


namespace luaxc {
    class GarbageCollector {
    public:
        class GCGuard {
        public:
            GCGuard(GarbageCollector* gc) {
                this->gc = gc;
                gc->increase_guard_semaphore();

                was_gc_enabled = gc->is_gc_enabled();
                gc->set_gc_enabled(false);
            }

            ~GCGuard() {
                gc->set_gc_enabled(was_gc_enabled);
                gc->decrease_guard_semaphore();
            }

        private:
            GarbageCollector* gc;
            bool was_gc_enabled;
        };

        GarbageCollector() = default;
        GarbageCollector(std::vector<PrimValue>* op_stack,
                         std::list<StackFrame>* stack_frame)
            : op_stack(op_stack), stack_frame(stack_frame) {}

        void init(std::vector<PrimValue>* op_stack,
                  std::list<StackFrame>* stack_frame) {
            this->op_stack = op_stack;
            this->stack_frame = stack_frame;
        }

        ~GarbageCollector();

        template<typename ObjectType, typename... Args, typename = std::enable_if_t<std::is_base_of_v<GCObject, ObjectType>>>
        ObjectType* allocate(Args&&... args) {
            if (statistics.bytes_allocated > config.max_heap_size) {
                throw std::runtime_error("Heap memory overflow");
            }

            if (should_run_gc()) {
                collect();
            }

            ObjectType* object = new ObjectType(std::forward<Args>(args)...);
            gc_objects.insert(object);

            size_t size = object->get_object_size();
            statistics.alloc_count++;
            statistics.bytes_allocated += size;

            return object;
        }

        void regist_no_collect(GCObject* object) {
            object->no_collect = true;
            gc_objects.insert(object);
            //statistics.bytes_allocated += object->get_object_size();
        }

        void regist(GCObject* object) {
            if (statistics.bytes_allocated > config.max_heap_size) {
                throw std::runtime_error("Heap memory overflow");
            }

            gc_objects.insert(object);
            statistics.alloc_count++;
            statistics.bytes_allocated += object->get_object_size();
        }

        void collect();

        GCGuard guard() { return GCGuard{this}; }

        void set_gc_enabled(bool enabled) { this->enabled = enabled; }

        bool is_gc_enabled() const { return enabled; }

        void set_max_heap_size(size_t size) { config.max_heap_size = size; }

        size_t get_max_heap_size() const { return config.max_heap_size; }

        void increase_guard_semaphore() { ++guard_semaphore; }

        void decrease_guard_semaphore() {
            --guard_semaphore;

            if (guard_semaphore == 0 && should_run_gc()) {
                collect();
            }
        }

        struct DumpedStats {
            size_t heap_size = 0;
            size_t max_heap_size = 0;

            size_t object_count = 0;

            bool running = false;
        };

        DumpedStats dump_stats() const {
            return {statistics.bytes_allocated,
                    config.max_heap_size,
                    gc_objects.size(),
                    enabled};
        }

    private:
        std::unordered_set<GCObject*> gc_objects;
        std::vector<PrimValue>* op_stack = nullptr;
        std::list<StackFrame>* stack_frame = nullptr;

        bool enabled = false;
        size_t guard_semaphore = 0;

        struct {
            size_t alloc_count = 0;
            size_t last_object_count = 0;
            size_t bytes_allocated = 0;
        } statistics;

        struct {
            size_t allocation_threshold = 64;
            double growth_factor = 2.0;
            size_t memory_threshold = 1024 * 1024 * 1;// 1 MB

            size_t max_heap_size = 1024 * 1024 * 64;// 64 MB
        } config;

        bool should_run_gc();

        void mark_from_roots();

        void mark_object(GCObject* object);

        void sweep();
    };

}// namespace luaxc