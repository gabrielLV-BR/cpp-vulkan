#pragma once

namespace MathUtils {
    template<typename T>
    T clamp(T val, T min, T max) {
        if(val < min) return min;
        if(val > max) return max;
        return val;
    }
}