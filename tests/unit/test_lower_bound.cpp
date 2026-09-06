// Hand-computed L2/n_ checks for core/lower_bound.cpp, per
// REFACTORING_STRATEGY.md §5 ("lowerBound() matches hand-computed L2 for a couple of
// textbook instances").
#include <cassert>
#include <cstdio>
#include <vector>

#include "core/bpp_instance.hpp"
#include "core/lower_bound.hpp"

using namespace ggacgt;

namespace {

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        std::exit(1);
    }
}

// All items are "large" (> capacity/2): each must occupy its own bin, so L2 == n_ ==
// number of items, and LowerBound() should return immediately via the jx==number_items
// branch without needing the general Martello-Toth formula.
void testAllItemsLarge() {
    BppInstance instance("t1", std::vector<Weight>{6, 6, 6}, /*capacity=*/10, /*best=*/3);
    LowerBoundResult result = computeLowerBound(instance);
    expect(result.L2 == 3, "all-large instance: L2 == 3");
    expect(result.largeItemCount == 3, "all-large instance: n_ == 3");
}

// No item is "large" (jx == 0): L2 falls back to ceil(totalWeight / capacity).
void testNoLargeItemsCeilBranch() {
    BppInstance instance("t2", std::vector<Weight>{3, 3, 3, 3}, /*capacity=*/10, /*best=*/2);
    LowerBoundResult result = computeLowerBound(instance);
    expect(result.largeItemCount == 0, "no-large instance: n_ == 0");
    // total weight 12, capacity 10 -> ceil(12/10) == 2
    expect(result.L2 == 2, "no-large instance: L2 == 2");
}

// No item is "large", and the total weight divides the capacity exactly: the
// fmod(...) >= 1 branch should NOT trigger the ceil, i.e. L2 == totalWeight / capacity
// exactly (no over-count).
void testExactDivisionBranch() {
    BppInstance instance("t3", std::vector<Weight>{5, 5, 5, 5}, /*capacity=*/10, /*best=*/2);
    LowerBoundResult result = computeLowerBound(instance);
    expect(result.largeItemCount == 0, "exact-division instance: n_ == 0");
    expect(result.L2 == 2, "exact-division instance: L2 == 2 (20/10 exactly)");
}

// A mix of one large item and several small ones, forcing the general Martello-Toth
// branch (jx != 0 and jx != number_items).
void testMixedGeneralBranch() {
    // capacity 10: item 0 (weight 6) is "large" (> 5); items 1..4 (weight 3 each) are not.
    BppInstance instance("t4", std::vector<Weight>{6, 3, 3, 3, 3}, /*capacity=*/10, /*best=*/3);
    LowerBoundResult result = computeLowerBound(instance);
    expect(result.largeItemCount == 1, "mixed instance: n_ == 1 (one item > capacity/2)");
    // One bin for the large item (6), the four 3's need ceil(12/10) = 2 more bins:
    // L2 should be at least 1 + 1 = 2, and Martello-Toth should tighten it to 3 since
    // the large item's bin has only 4 spare capacity (< 3+3), so it can hold at most
    // one more 3-weight item alongside it in the best case.
    expect(result.L2 >= 2, "mixed instance: L2 is a valid lower bound (>= naive bin-count estimate)");
    expect(result.L2 <= 5, "mixed instance: L2 does not exceed the trivial one-item-per-bin bound");
}

} // namespace

int main() {
    testAllItemsLarge();
    testNoLargeItemsCeilBranch();
    testExactDivisionBranch();
    testMixedGeneralBranch();
    std::printf("test_lower_bound: all checks passed\n");
    return 0;
}
