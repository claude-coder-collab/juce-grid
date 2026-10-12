#pragma once

#include <juce_core/juce_core.h>

#include <compare>
#include <functional>
#include <vector>

namespace jgrid
{

//==============================================================================
/** The Grid's default comparison of two cell values, used when a model has no compare callback: void values first, numbers numerically
    when both are numeric, otherwise natural, case-insensitive order of their text ("9" before "10"). */
[[nodiscard]] std::weak_ordering compareValues(const juce::var& a, const juce::var& b);

/** Returns the stable sort of rows 0..rowCount-1 as order[newIndex] = oldIndex. Descending order
    reverses the comparison but keeps equal rows in their current relative order. */
[[nodiscard]] std::vector<int> sortedOrder(int rowCount, const std::function<std::weak_ordering(int, int)>& compare, bool descending);

/** True if order is 0, 1, 2, ... */
[[nodiscard]] bool isIdentityOrder(const std::vector<int>& order);

} // namespace jgrid
