#include "value.hpp"

#include <vector>

namespace luaxc {

    class IRRuntime;

    using Functions = std::vector<std::pair<StringObject*, PrimValue>>;

    class NativeLib {
    public:
        virtual Functions load(IRRuntime& runtime) = 0;

        ~NativeLib() = default;
    };

    class IO : public NativeLib {
    public:
        Functions load(IRRuntime& runtime) override;
    };

    inline IO& io() {
        static IO io;
        return io;
    }

    class Typing : public NativeLib {
    public:
        Functions load(IRRuntime& runtime) override;
    };

    inline Typing& typing() {
        static Typing typing;
        return typing;
    }

    class Runtime : public NativeLib {
    public:
        Functions load(IRRuntime& runtime) override;
    };

    inline Runtime& runtime() {
        static Runtime runtime;
        return runtime;
    }

    class Constraints : public NativeLib {
    public:
        Functions load(IRRuntime& runtime) override;
    };

    inline Constraints& constraints() {
        static Constraints constraints;
        return constraints;
    }

    class Strings : public NativeLib {
    public:
        Functions load(IRRuntime& runtime) override;
    };

    inline Strings& strings() {
        static Strings strings;
        return strings;
    }

}// namespace luaxc