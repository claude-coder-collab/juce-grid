#include "../core/jgrid_Editing.h"
#include "jgrid_GridInternals.h"

#include <algorithm>
#include <set>
#include <utility>

namespace jgrid
{

std::optional<CutMark>& currentCutMark()
{
    static std::optional<CutMark> mark;
    return mark;
}

namespace
{

juce::String cellCount(int count) { return juce::String(count) + (count == 1 ? " cell" : " cells"); }

} // namespace

struct Grid::WriteFailure
{
    Cell cell;
    juce::String message;
    int count = 0;
};

void Grid::copy()
{
    clearCutMark();
    const auto range = copyRange(selection);
    if (range.isEmpty())
        return;
    TextTable table;
    for (auto row = range.rows.begin; row < range.rows.end; ++row)
    {
        if (!rows.isVisible(row))
            continue;
        auto& values = table.emplace_back();
        for (auto col = range.cols.begin; col < range.cols.end; ++col)
            if (cols.isVisible(col))
                values.push_back(getCellText({ .row = row, .col = col }));
    }
    if (table.empty() || table.front().empty())
        return;
    juce::SystemClipboard::copyTextToClipboard(encodeTsv(table));
    postAnnouncement("Copied " + cellCount(range.getNumCells()));
}

void Grid::cut()
{
    const auto range = copyRange(selection);
    if (range.isEmpty())
        return;
    copy();
    currentCutMark() = CutMark{ .source = this, .range = range, .rows = {}, .text = juce::SystemClipboard::getTextFromClipboard() };
    body->repaint();
    postAnnouncement("Cut " + cellCount(range.getNumCells()));
}

bool Grid::paste()
{
    if (!cursor)
        return false;
    if (!model.setValue)
    {
        postAnnouncement("Read-only grid");
        return false;
    }
    const auto text = juce::SystemClipboard::getTextFromClipboard();
    const auto table = decodeTsv(text);
    if (table.empty())
        return false;

    const auto writable = [this](Cell cell) { return isCellWritable(cell); };
    auto writes = planPaste(table, selection, *cursor, rows, cols, writable);

    auto& mark = currentCutMark();
    Grid* source = nullptr;
    std::vector<CellWrite> clears;
    if (mark && (mark->source == nullptr || mark->text != text || !mark->rows.empty()))
        clearCutMark();
    if (mark)
    {
        source = mark->source.getComponent();
        std::set<std::pair<int, int>> written;
        if (source == this)
            for (const auto& write : writes)
                written.emplace(write.cell.row, write.cell.col);
        for (auto row = mark->range.rows.begin; row < mark->range.rows.end; ++row)
            for (auto col = mark->range.cols.begin; col < mark->range.cols.end; ++col)
                if (source->isCellWritable({ .row = row, .col = col }) && !written.contains({ row, col }))
                    clears.push_back({ .cell = { .row = row, .col = col }, .text = {} });
    }

    if (writes.empty())
        return false;
    if (const auto failure = validateWrites(writes))
    {
        reportWriteFailure(*failure);
        return false;
    }
    if (source != nullptr)
    {
        if (const auto failure = source->validateWrites(clears))
        {
            source->reportWriteFailure(*failure);
            return false;
        }
    }

    clearCutMark();
    auto ok = true;
    if (source == this)
    {
        auto all = writes;
        all.insert(all.end(), clears.begin(), clears.end());
        ok = applyWrites(all);
    }
    else
    {
        ok = applyWrites(writes);
        if (ok && source != nullptr)
            ok = source->applyWrites(clears);
    }
    if (!ok)
        return false;

    const auto isFill = table.size() == 1 && table.front().size() == 1;
    if (const auto start = cursor; writes.size() > 1 && !isFill && start)
    {
        const auto last = writes.back().cell;
        selection.collapseTo(start);
        selection.extendTo({ .row = std::max(last.row, start->row), .col = std::max(last.col, start->col) });
        selectionChanged(false);
    }
    postAnnouncement("Pasted " + cellCount(static_cast<int>(writes.size())));
    return true;
}

bool Grid::deleteSelection()
{
    if (!model.setValue)
        return false;
    std::vector<CellWrite> writes;
    for (const auto cell : selectedCells(selection, [this](Cell c) { return isCellWritable(c); }))
        writes.push_back({ .cell = cell, .text = {} });
    if (writes.empty())
        return false;
    if (const auto failure = validateWrites(writes))
    {
        reportWriteFailure(*failure);
        return false;
    }
    if (!applyWrites(writes))
        return false;
    postAnnouncement("Cleared " + cellCount(static_cast<int>(writes.size())));
    return true;
}

void Grid::clearCutMark()
{
    auto& mark = currentCutMark();
    if (!mark)
        return;
    if (auto* source = mark->source.getComponent())
        source->body->repaint();
    mark.reset();
}

std::optional<CellRange> Grid::getCutRange() const
{
    const auto& mark = currentCutMark();
    if (mark && mark->source == this && mark->rows.empty())
        return mark->range;
    return std::nullopt;
}

std::optional<Grid::WriteFailure> Grid::validateWrites(const std::vector<CellWrite>& writes) const
{
    if (!model.validate)
        return std::nullopt;
    std::optional<WriteFailure> failure;
    for (const auto& write : writes)
    {
        const auto result = model.validate(write.cell.row, write.cell.col, juce::var(write.text));
        if (result.wasOk())
            continue;
        if (!failure)
            failure = WriteFailure{ .cell = write.cell, .message = result.getErrorMessage() };
        ++failure->count;
    }
    return failure;
}

bool Grid::applyWrites(const std::vector<CellWrite>& writes)
{
    if (writes.empty())
        return true;
    if (model.beginBatch)
        model.beginBatch();
    const juce::ScopeGuard endBatch{ [this] {
        if (model.endBatch)
            model.endBatch();
    } };
    for (const auto& write : writes)
    {
        if (!model.setValue(write.cell.row, write.cell.col, juce::var(write.text)))
        {
            reportWriteFailure({ .cell = write.cell, .message = couldNotSetValueMessage(), .count = 1 });
            return false;
        }
    }
    body->repaint();
    auto firstRow = writes.front().cell.row;
    auto lastRow = firstRow;
    auto firstCol = writes.front().cell.col;
    auto lastCol = firstCol;
    for (const auto& write : writes)
    {
        firstRow = std::min(firstRow, write.cell.row);
        lastRow = std::max(lastRow, write.cell.row);
        firstCol = std::min(firstCol, write.cell.col);
        lastCol = std::max(lastCol, write.cell.col);
    }
    notifySelf(CellsChanged{ .firstRow = firstRow, .numRows = lastRow - firstRow + 1, .firstCol = firstCol, .numCols = lastCol - firstCol + 1 });
    return true;
}

void Grid::reportWriteFailure(const WriteFailure& failure)
{
    const auto message = failure.count > 1 ? failure.message + ", and " + juce::String(failure.count - 1) + " more" : failure.message;
    setCursor(failure.cell);
    showErrorAt(failure.cell, message);
    postAnnouncement(message);
}

bool Grid::isCellWritable(Cell cell) const { return rows.isVisible(cell.row) && cols.isVisible(cell.col) && model.canEdit(cell.row, cell.col); }

void Grid::paintCutMark(juce::Graphics& g) const
{
    const auto& mark = currentCutMark();
    if (!mark || mark->source != this)
        return;

    std::vector<juce::Rectangle<int>> areas;
    const auto addRows = [&](int first, int last, int left, int right) {
        areas.emplace_back(left, rows.getStart(first), right - left, rows.getEnd(last) - rows.getStart(first));
    };
    if (mark->rows.empty())
    {
        const auto& range = mark->range;
        if (range.isEmpty() || range.rows.end > rows.getCount() || range.cols.end > cols.getCount())
            return;
        addRows(range.rows.begin, range.rows.end - 1, cols.getStart(range.cols.begin), cols.getStart(range.cols.end));
    }
    else
    {
        for (std::size_t i = 0; i < mark->rows.size();)
        {
            auto j = i;
            while (j + 1 < mark->rows.size() && mark->rows[j + 1] == mark->rows[j] + 1)
                ++j;
            if (mark->rows[j] < rows.getCount())
                addRows(mark->rows[i], mark->rows[j], 0, cols.getTotalSize());
            i = j + 1;
        }
    }

    g.setColour(getGridColour(cursorColourId));
    const std::array dashes{ 4.0f, 3.0f };
    for (const auto& area : areas)
    {
        juce::Path outline;
        outline.addRectangle(area.translated(-scroll.x, -scroll.y).toFloat().reduced(1.0f));
        juce::Path dashed;
        juce::PathStrokeType(2.0f).createDashedStroke(dashed, outline, dashes.data(), static_cast<int>(dashes.size()));
        g.fillPath(dashed);
    }
}

void Grid::cutMarkTouched(const Change& change) const
{
    const auto& mark = currentCutMark();
    if (!mark || mark->source != this)
        return;
    if (const auto* cells = std::get_if<CellsChanged>(&change))
    {
        const auto rowsTouched = [&](int first, int end) { return cells->firstRow < end && cells->firstRow + cells->numRows > first; };
        auto overlaps = false;
        if (mark->rows.empty())
            overlaps = rowsTouched(mark->range.rows.begin, mark->range.rows.end) && cells->firstCol < mark->range.cols.end
                && cells->firstCol + cells->numCols > mark->range.cols.begin;
        else
            overlaps = std::ranges::any_of(mark->rows, [&](int row) { return rowsTouched(row, row + 1); });
        if (!overlaps)
            return;
    }
    clearCutMark();
}

} // namespace jgrid
