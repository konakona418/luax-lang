#pragma once

#include <unordered_set>

#include "value.hpp"


namespace luaxc {
    class GarbageCollector {
    public:
        GarbageCollector() = default;
        GarbageCollector(std::vector<PrimValue>* op_stack,
                         std::vector<StackFrame>* stack_frame)
            : op_stack(op_stack), stack_frame(stack_frame) {}

        void init(std::vector<PrimValue>* op_stack,
                  std::vector<StackFrame>* stack_frame) {
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
        }

        void regist(GCObject* object) {
            gc_objects.insert(object);
            statistics.alloc_count++;
            statistics.bytes_allocated += object->get_object_size();
        }

        void collect();

        void set_gc_enabled(bool enabled) { this->enabled = true; }

        bool is_gc_enabled() const { return enabled; }

    private:
        std::unordered_set<GCObject*> gc_objects;
        std::vector<PrimValue>* op_stack = nullptr;
        std::vector<StackFrame>* stack_frame = nullptr;

        bool enabled = false;

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