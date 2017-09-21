#pragma once

#include <ppl.h>

namespace TaskManager {
    void Initialize();
    void Shutdown();

    template <typename Func>
    void parallel_for(size_t Begin, size_t End, const Func& func)
    {
        concurrency::parallel_for(Begin, End, func);
    }
}