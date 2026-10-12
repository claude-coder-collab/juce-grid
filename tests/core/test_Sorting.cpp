#include <jgrid/core/jgrid_Sorting.h>

#include <catch2/catch_test_macros.hpp>

#include <compare>

using jgrid::compareValues;

TEST_CASE("Numbers compare numerically", "[sorting]")
{
    CHECK(std::is_lt(compareValues(9, 10)));
    CHECK(std::is_gt(compareValues(2.5, 2)));
    CHECK(std::is_eq(compareValues(juce::var(static_cast<juce::int64>(5)), 5.0)));
}

TEST_CASE("Text compares naturally and ignores case", "[sorting]")
{
    CHECK(std::is_lt(compareValues("item 9", "item 10")));
    CHECK(std::is_lt(compareValues("apple", "Banana")));
    CHECK(std::is_eq(compareValues("ABC", "abc")));
    CHECK(std::is_gt(compareValues("10", 9)));
}

TEST_CASE("Void values sort first", "[sorting]")
{
    CHECK(std::is_lt(compareValues({}, 0)));
    CHECK(std::is_gt(compareValues("", juce::var())));
    CHECK(std::is_eq(compareValues({}, {})));
}

TEST_CASE("Sorting is stable in both directions", "[sorting]")
{
    const std::vector keys{ 2, 1, 2, 1, 3 };
    const auto compare = [&](int a, int b) { return std::weak_order(keys.at(static_cast<std::size_t>(a)), keys.at(static_cast<std::size_t>(b))); };
    CHECK(jgrid::sortedOrder(5, compare, false) == std::vector{ 1, 3, 0, 2, 4 });
    CHECK(jgrid::sortedOrder(5, compare, true) == std::vector{ 4, 0, 2, 1, 3 });
    CHECK(jgrid::sortedOrder(0, compare, false).empty());
}

TEST_CASE("Identity orders are recognised", "[sorting]")
{
    CHECK(jgrid::isIdentityOrder({ 0, 1, 2 }));
    CHECK(jgrid::isIdentityOrder({}));
    CHECK_FALSE(jgrid::isIdentityOrder({ 1, 0 }));
}
