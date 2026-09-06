#include "lower_bound.hpp"

#include <cmath>

namespace ggacgt {

LowerBoundResult computeLowerBound(const BppInstance& instance) {
    const auto& W = instance.weights();
    const auto& OW = instance.descendingWeightOrder();
    const long numberItems = instance.numberOfItems();
    // long double throughout, matching the original's `sjx`/`sj2`/`sj3` types --
    // these feed fmod()/ceil() comparisons against exact integer boundaries where
    // precision loss could flip a `>= 1` check and silently change L2 by one bin.
    const long double capacity = static_cast<long double>(instance.capacity());

    long jx = 0;
    while (jx < numberItems && W[static_cast<std::size_t>(OW[static_cast<std::size_t>(jx)])] >
                                    instance.capacity() / 2)
        ++jx;

    LowerBoundResult result{};
    result.largeItemCount = jx;

    if (jx == numberItems) {
        result.L2 = jx;
        return result;
    }

    auto w = [&](long rank) { return W[static_cast<std::size_t>(OW[static_cast<std::size_t>(rank)])]; };

    if (jx == 0) {
        long double total = static_cast<long double>(instance.totalWeight());
        result.L2 = (std::fmod(total, capacity) >= 1)
                        ? static_cast<long>(std::ceil(total / capacity))
                        : static_cast<long>(total / capacity);
        return result;
    }

    long cj12 = jx;
    long double sjx = 0;
    for (long i = jx; i < numberItems; ++i) sjx += static_cast<long double>(w(i));

    long jp = jx;
    for (long i = 0; i < jx; ++i) {
        if (w(i) <= instance.capacity() - w(jx)) {
            jp = i;
            break;
        }
    }

    long cj2 = jx - jp;
    long double sj2 = 0;
    for (long i = jp; i <= jx - 1; ++i) sj2 += static_cast<long double>(w(i));

    long jpp = jx;
    long double sj3 = static_cast<long double>(w(jpp));
    // Original: `ordered_weight[number_items] = number_items; weight[number_items] = 0;`
    // then an unguarded `while(weight[ordered_weight[jpp+1]] == weight[ordered_weight[jpp]])`.
    // See lower_bound.hpp doc comment: replaced with an explicit bounds check.
    while (jpp + 1 < numberItems && w(jpp + 1) == w(jpp)) {
        ++jpp;
        sj3 += static_cast<long double>(w(jpp));
    }

    long L2 = cj12;
    long aux2 = 0;

    do {
        long aux1 = (std::fmod(sj3 + sj2, capacity) >= 1)
                        ? static_cast<long>(std::ceil((sj3 + sj2) / capacity - cj2))
                        : static_cast<long>((sj3 + sj2) / capacity - cj2);
        if (L2 < cj12 + aux1) L2 = cj12 + aux1;

        ++jpp;
        if (jpp < numberItems) {
            sj3 += static_cast<long double>(w(jpp));
            while (jpp + 1 < numberItems && w(jpp + 1) == w(jpp)) {
                ++jpp;
                sj3 += static_cast<long double>(w(jpp));
            }
            while (jp > 0 && w(jp - 1) <= instance.capacity() - w(jpp)) {
                --jp;
                ++cj2;
                sj2 += static_cast<long double>(w(jp));
            }
        }

        aux2 = (std::fmod(sjx + sj2, capacity) >= 1)
                   ? static_cast<long>(std::ceil((sjx + sj2) / capacity - cj2))
                   : static_cast<long>((sjx + sj2) / capacity - cj2);
    } while (jpp <= numberItems || (cj12 + aux2) > L2);

    result.L2 = L2;
    return result;
}

} // namespace ggacgt
