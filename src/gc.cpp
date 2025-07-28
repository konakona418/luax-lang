#include "gc.hpp"

#include <cassert>

namespace luaxc {
    GarbageCollector::~GarbageCollector() {
        for (auto* object: gc_objects) {
            if (!object->no_collect) {
                delete object;
            }
        }
    }

    void GarbageCollector::collect() {
        for (auto* object: gc_objects) {
            object->marked = false;
        }

        mark_from_roots();

        statistics.last_object_count = gc_objects.size();
        statistics.alloc_count = 0;

        sweep();
    }

    bool GarbageCollector::should_run_gc() {
        if (!enabled) {
            return false;
        }

        if (statistics.alloc_count >= config.allocation_threshold) {
            return true;
        }

        if (gc_objects.size() >= statistics.last_object_count * config.growth_factor) {
            return true;
        }

        if (statistics.bytes_allocated >= config.memory_threshold) {
            return true;
        }

        return false;
    }

    void GarbageCollector::mark_from_roots() {
        for (auto& value: *op_stack) {
            if (value.is_gc_object()) {
                mark_object(value.get_inner_value<GCObject*>());
            }
        }

        for (auto& frame: *stack_frame) {
            for (auto& [_, value]: frame.variables) {
                if (value.is_gc_object()) {
                    mark_object(value.get_inner_value<GCObject*>());
                }
            }
        }
    }

    void GarbageCollector::mark_object(GCObject* object) {
        if (object == nullptr) {
            return;
        }

        if (object->marked) {
            return;
        }

        object->marked = true;
        for (auto* child: object->get_referenced_objects()) {
            mark_object(child);
        }
    }

    void GarbageCollector::sweep() {
        std::unordered_set<GCObject*> to_sweep;
        for (auto* object: gc_objects) {
            if (!object->marked && !object->no_collect) {
                to_sweep.insert(object);

                auto object_size = object->get_object_size();
                assert(statistics.bytes_allocated >= object_size);
                // !!
                statistics.bytes_allocated -= object_size;
            }
        }

        for (auto* object: to_sweep) {
            delete object;
            gc_objects.erase(object);
        }
    }
}// namespace luaxc