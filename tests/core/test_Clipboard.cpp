#include <jgrid/core/jgrid_Clipboard.h>

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::CellWrite;
using jgrid::Selection;
using jgrid::TextTable;

namespace
{

jgrid::Axis axis(int count, const std::vector<int>& hidden = {})
{
    jgrid::Axis result{ 10 };
    result.setCount(count);
    for (const auto index : hidden)
        result.setHidden(index, true);
    return result;
}

} // namespace

TEST_CASE("Tables encode as tab-separated lines", "[clipboard]")
{
    CHECK(jgrid::encodeTsv({ { "a", "b" }, { "1", "" } }) == "a\tb\n1\t\n");
    CHECK(jgrid::encodeTsv({}).isEmpty());
}

TEST_CASE("Special values are quoted Excel-style", "[clipboard]")
{
    CHECK(jgrid::encodeTsv({ { "say \"hi\"", "two\nlines", "tab\there" } }) == "\"say \"\"hi\"\"\"\t\"two\nlines\"\t\"tab\there\"\n");
}

TEST_CASE("Decoding reverses encoding", "[clipboard]")
{
    const TextTable table{ { "a", "say \"hi\"", "" }, { "two\nlines", "x\ty", "3" } };
    CHECK(jgrid::decodeTsv(jgrid::encodeTsv(table)) == table);
}

TEST_CASE("Decoding accepts other apps' line ends and missing trailing newline", "[clipboard]")
{
    CHECK(jgrid::decodeTsv("a\tb\r\nc\td") == TextTable{ { "a", "b" }, { "c", "d" } });
    CHECK(jgrid::decodeTsv("a\rb\r") == TextTable{ { "a" }, { "b" } });
    CHECK(jgrid::decodeTsv("single") == TextTable{ { "single" } });
    CHECK(jgrid::decodeTsv("a\t") == TextTable{ { "a", "" } });
    CHECK(jgrid::decodeTsv("\n") == TextTable{ { "" } });
    CHECK(jgrid::decodeTsv({}).empty());
}

TEST_CASE("A quote inside an unquoted value is kept", "[clipboard]")
{
    CHECK(jgrid::decodeTsv("5\" screen") == TextTable{ { "5\" screen" } });
}

TEST_CASE("Paste lands at the cursor and is clipped", "[clipboard]")
{
    Selection selection;
    selection.collapseTo(Cell{ .row = 1, .col = 1 });
    const auto writes = jgrid::planPaste({ { "a", "b", "c" }, { "d", "e", "f" } }, selection, { .row = 1, .col = 1 }, axis(3), axis(3), {});
    CHECK(writes
        == std::vector<CellWrite>{ { .cell = { .row = 1, .col = 1 }, .text = "a" },
            { .cell = { .row = 1, .col = 2 }, .text = "b" },
            { .cell = { .row = 2, .col = 1 }, .text = "d" },
            { .cell = { .row = 2, .col = 2 }, .text = "e" } });
}

TEST_CASE("A single value fills a multi-cell selection", "[clipboard]")
{
    Selection selection;
    selection.collapseTo(Cell{});
    selection.extendTo({ .row = 1, .col = 1 });
    const auto writes = jgrid::planPaste({ { "x" } }, selection, {}, axis(5), axis(5), {});
    CHECK(writes.size() == 4);
    CHECK(std::ranges::all_of(writes, [](const CellWrite& write) { return write.text == "x"; }));
}

TEST_CASE("Paste skips cells that cannot be written", "[clipboard]")
{
    Selection selection;
    selection.collapseTo(Cell{});
    const auto writes = jgrid::planPaste({ { "a", "b" } }, selection, {}, axis(5), axis(5), [](Cell cell) { return cell.col != 0; });
    CHECK(writes == std::vector<CellWrite>{ { .cell = { .row = 0, .col = 1 }, .text = "b" } });
}

TEST_CASE("Selected cells are listed once in reading order", "[clipboard]")
{
    Selection selection;
    selection.collapseTo(Cell{ .row = 1, .col = 0 });
    selection.extendTo({ .row = 1, .col = 1 });
    selection.addCell({ .row = 0, .col = 1 });
    selection.extendTo({ .row = 1, .col = 1 });
    CHECK(jgrid::selectedCells(selection, {}) == std::vector<Cell>{ { .row = 0, .col = 1 }, { .row = 1, .col = 0 }, { .row = 1, .col = 1 } });
    CHECK(jgrid::copyRange(selection) == jgrid::CellRange::spanning({ .row = 0, .col = 1 }, { .row = 1, .col = 1 }));
}

TEST_CASE("Paste skips hidden rows and columns", "[clipboard]")
{
    Selection selection;
    selection.collapseTo(Cell{});
    const auto writes = jgrid::planPaste({ { "a", "b" }, { "c", "d" } }, selection, {}, axis(4, { 1 }), axis(4, { 0, 2 }), {});
    CHECK(writes
        == std::vector<CellWrite>{ { .cell = { .row = 0, .col = 1 }, .text = "a" },
            { .cell = { .row = 0, .col = 3 }, .text = "b" },
            { .cell = { .row = 2, .col = 1 }, .text = "c" },
            { .cell = { .row = 2, .col = 3 }, .text = "d" } });
}

TEST_CASE("A fill skips hidden cells", "[clipboard]")
{
    Selection selection;
    selection.collapseTo(Cell{});
    selection.extendTo({ .row = 2, .col = 0 });
    const auto writes = jgrid::planPaste({ { "x" } }, selection, {}, axis(3, { 1 }), axis(1), {});
    CHECK(writes.size() == 2);
}
