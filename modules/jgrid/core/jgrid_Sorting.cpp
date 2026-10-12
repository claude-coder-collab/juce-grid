#include "jgrid_Sorting.h"

#include <algorithm>
#include <numeric>
#include <utility>

namespace jgrid
{

namespace
{

bool isNumeric(const juce::var& value) { return value.isInt() || value.isInt64() || value.isDouble() || value.isBool(); }

} // namespace

std::weak_ordering compareValues(const juce::var& a, const juce::var& b)
{
    if (a.isVoid() || b.isVoid())
        return static_cast<int>(b.isVoid()) <=> static_cast<int>(a.isVoid());
    if (isNumeric(a) && isNumeric(b))
        return std::weak_order(static_cast<double>(a), static_cast<double>(b));
    const auto result = a.toString().compareNatural(b.toString(), false);
    return result <=> 0;
}

std::vector<int> sortedOrder(int rowCount, const std::function<std::weak_ordering(int, int)>& compare, bool descending)
{
    std::vector<int> order(static_cast<std::size_t>(std::max(0, rowCount)));
    std::ranges::iota(order, 0);
    std::ranges::stable_sort(order, [&](int a, int b) { return descending ? compare(b, a) < 0 : compare(a, b) < 0; });
    return order;
}

bool isIdentityOrder(const std::vector<int>& order)
{
    for (auto i = 0; std::cmp_less(i, order.size()); ++i)
        if (order[static_cast<std::size_t>(i)] != i)
            return false;
    return true;
}

} // namespace jgrid
