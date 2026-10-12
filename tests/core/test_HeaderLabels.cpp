#include <jgrid/core/jgrid_HeaderLabels.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Column letters follow spreadsheet order", "[labels]")
{
    CHECK(jgrid::columnLetters(0) == "A");
    CHECK(jgrid::columnLetters(25) == "Z");
    CHECK(jgrid::columnLetters(26) == "AA");
    CHECK(jgrid::columnLetters(27) == "AB");
    CHECK(jgrid::columnLetters(51) == "AZ");
    CHECK(jgrid::columnLetters(52) == "BA");
    CHECK(jgrid::columnLetters(701) == "ZZ");
    CHECK(jgrid::columnLetters(702) == "AAA");
    CHECK(jgrid::columnLetters(-1).isEmpty());
}

TEST_CASE("Row numbers are one-based", "[labels]")
{
    CHECK(jgrid::rowNumber(0) == "1");
    CHECK(jgrid::rowNumber(99) == "100");
}
