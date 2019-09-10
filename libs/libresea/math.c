#include <resea/math.h>

/// Computes log2(x). You should make sure that x is a power of two.
unsigned long long ulllog2(unsigned long long x) {
    unsigned long long ans = 0;
    while (x) {
        x >>= 1;
        ans++;
    }

    return ans;
}

// Checks if x is a power of two.
bool is_power_of_two(unsigned long long x) {
    // This implementation simply checks if popcnt(x) <= 1.
    if (x == 0) {
        return true;
    }

    while (x != 1) {
        if (x % 2 != 0) {
            return false;
        }

        x >>= 1;
    }

    return true;
}
