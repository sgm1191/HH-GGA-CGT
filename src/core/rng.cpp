#include "rng.hpp"

#include <utility>

namespace ggacgt {

float Rng::next01() {
    // Ported verbatim from the original `randp()` (Schrage's algorithm): the
    // parenthesization is load-bearing (it avoids 32-bit signed overflow by never
    // forming the full product `A * ix`), so it is kept exactly as in the original.
    const int A = 16807;            // = 7**5
    const int P = 2147483647;       // = Mersenne prime (2**31)-1
    const int b15 = 32768;          // = 2**15
    const int b16 = 65536;          // = 2**16

    int ix = state_;
    int xhi = ix / b16;
    int xalo = (ix - xhi * b16) * A;
    int leftlo = xalo / b16;
    int fhi = xhi * A + leftlo;
    int k = fhi / b15;

    ix = (((xalo - leftlo * b16) - P) + (fhi - k * b15) * b16) + k;
    if (ix < 0) ix += P;

    state_ = ix;
    return static_cast<float>(ix * 4.656612875e-10);
}

int Rng::uniform(int size) {
    next01();
    return static_cast<int>(static_cast<double>(state_) /
                             (2147483647.0 / static_cast<double>(size))) +
           1;
}

int Rng::uniformRange(int i, int j) {
    next01();
    return static_cast<int>(static_cast<double>(state_) /
                             (2147483647.0 / static_cast<double>(j - i + 1))) +
           i;
}

void shuffleRange(Rng& rng, std::vector<long>& arr, long k, long n,
                   const std::vector<Weight>& weight, Weight& lighterWeight) {
    for (long i = n - 1; i >= k; --i) {
        long randomIndex = k + rng.uniform(static_cast<int>(n - k)) - 1;
        std::swap(arr[randomIndex], arr[i]);
        if (weight[arr[i]] < lighterWeight) lighterWeight = weight[arr[i]];
        if (weight[arr[randomIndex]] < lighterWeight) lighterWeight = weight[arr[randomIndex]];
    }
}

void shuffleRange(Rng& rng, std::vector<long>& arr, long k, long n) {
    for (long i = n - 1; i >= k; --i) {
        long randomIndex = k + rng.uniform(static_cast<int>(n - k)) - 1;
        std::swap(arr[randomIndex], arr[i]);
    }
}

} // namespace ggacgt
