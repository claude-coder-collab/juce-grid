#include "jgrid_Clipboard.h"

#include <algorithm>
#include <set>
#include <utility>

namespace jgrid
{

namespace
{

juce::String quoted(const juce::String& value)
{
    if (!value.containsAnyOf("\t\n\r\""))
        return value;
    return "\"" + value.replace("\"", "\"\"") + "\"";
}

} // namespace

juce::String encodeTsv(const TextTable& table)
{
    juce::String text;
    for (const auto& row : table)
    {
        for (std::size_t col = 0; col < row.size(); ++col)
        {
            if (col > 0)
                text << "\t";
            text << quoted(row[col]);
        }
        text << "\n";
    }
    return text;
}

TextTable decodeTsv(const juce::String& text)
{
    TextTable table;
    if (text.isEmpty())
        return table;

    std::vector<juce::String> row;
    juce::String field;
    auto inQuotes = false;
    auto fieldStarted = false;
    const auto endRow = [&] {
        row.push_back(field);
        table.push_back(std::move(row));
        row = {};
        field.clear();
        fieldStarted = false;
    };

    for (auto p = text.getCharPointer(); !p.isEmpty();)
    {
        const auto c = p.getAndAdvance();
        if (inQuotes)
        {
            if (c != '"')
            {
                field << juce::String::charToString(c);
            }
            else if (*p == '"')
            {
                field << '"';
                ++p;
            }
            else
            {
                inQuotes = false;
            }
            continue;
        }
        if (c == '"' && !fieldStarted)
        {
            inQuotes = true;
            fieldStarted = true;
        }
        else if (c == '\t')
        {
            row.push_back(field);
            field.clear();
            fieldStarted = false;
        }
        else if (c == '\r' || c == '\n')
        {
            if (c == '\r' && *p == '\n')
                ++p;
            endRow();
        }
        else
        {
            field << juce::String::charToString(c);
            fieldStarted = true;
        }
    }
    if (fieldStarted || !row.empty() || inQuotes)
        endRow();
    return table;
}

std::vector<CellWrite> planPaste(const TextTable& table,
    const Selection& selection,
    Cell cursor,
    const Axis& rows,
    const Axis& cols,
    const std::function<bool(Cell)>& isWritable)
{
    std::vector<CellWrite> writes;
    if (table.empty())
        return writes;

    const auto writable = [&](Cell cell) { return !isWritable || isWritable(cell); };

    if (table.size() == 1 && table.front().size() == 1 && !selection.isSingleCell())
    {
        for (const auto cell : selectedCells(selection, isWritable))
            if (rows.isVisible(cell.row) && cols.isVisible(cell.col))
                writes.push_back({ .cell = cell, .text = table.front().front() });
        return writes;
    }

    auto row = rows.step(cursor.row, 0);
    for (const auto& values : table)
    {
        if (row < 0)
            break;
        auto col = cols.step(cursor.col, 0);
        for (const auto& value : values)
        {
            if (col < 0)
                break;
            const Cell cell{ .row = row, .col = col };
            if (writable(cell))
                writes.push_back({ .cell = cell, .text = value });
            col = cols.nextVisible(col, true);
        }
        row = rows.nextVisible(row, true);
    }
    return writes;
}

CellRange copyRange(const Selection& selection) { return selection.getRanges().empty() ? CellRange{} : selection.getRanges().back(); }

std::vector<Cell> selectedCells(const Selection& selection, const std::function<bool(Cell)>& isWritable)
{
    std::set<std::pair<int, int>> seen;
    std::vector<Cell> cells;
    for (const auto& range : selection.getRanges())
    {
        for (auto row = range.rows.begin; row < range.rows.end; ++row)
        {
            for (auto col = range.cols.begin; col < range.cols.end; ++col)
            {
                const Cell cell{ .row = row, .col = col };
                if ((!isWritable || isWritable(cell)) && seen.emplace(row, col).second)
                    cells.push_back(cell);
            }
        }
    }
    std::ranges::sort(cells, [](Cell a, Cell b) { return std::pair(a.row, a.col) < std::pair(b.row, b.col); });
    return cells;
}

} // namespace jgrid
