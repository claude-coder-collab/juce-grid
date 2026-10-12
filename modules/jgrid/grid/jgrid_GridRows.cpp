#include "jgrid_GridInternals.h"

#include <algorithm>
#include <set>

namespace jgrid
{

namespace
{

juce::String rowCount(std::size_t count) { return juce::String(static_cast<int>(count)) + (count == 1 ? " row" : " rows"); }

} // namespace

std::vector<int> Grid::getSelectedRows() const
{
    std::set<int> result;
    for (const auto& range : selection.getRanges())
        if (range.cols.begin <= 0 && range.cols.end >= cols.getCount() && cols.getCount() > 0)
            for (auto row = range.rows.begin; row < range.rows.end; ++row)
                if (rows.isVisible(row))
                    result.insert(row);
    return { result.begin(), result.end() };
}

std::vector<int> Grid::getSelectedColumns() const
{
    std::set<int> result;
    for (const auto& range : selection.getRanges())
        if (range.rows.begin <= 0 && range.rows.end >= rows.getCount() && rows.getCount() > 0)
            for (auto col = range.cols.begin; col < range.cols.end; ++col)
                if (cols.isVisible(col))
                    result.insert(col);
    return { result.begin(), result.end() };
}

TextTable Grid::rowsTable(const std::vector<int>& rowIndices) const
{
    TextTable table;
    for (const auto row : rowIndices)
    {
        auto& values = table.emplace_back();
        for (auto col = 0; col < cols.getCount(); ++col)
            values.push_back(getCellText({ .row = row, .col = col }));
    }
    return table;
}

bool Grid::copyRows()
{
    const auto selected = getSelectedRows();
    if (selected.empty())
        return false;
    clearCutMark();
    juce::SystemClipboard::copyTextToClipboard(encodeTsv(rowsTable(selected)));
    postAnnouncement("Copied " + rowCount(selected.size()));
    return true;
}

bool Grid::cutRows()
{
    if (!model.removeRows)
        return false;
    const auto selected = getSelectedRows();
    if (selected.empty() || !copyRows())
        return false;
    currentCutMark() = CutMark{ .source = this, .range = {}, .rows = selected, .text = juce::SystemClipboard::getTextFromClipboard() };
    body->repaint();
    postAnnouncement("Cut " + rowCount(selected.size()));
    return true;
}

RowData Grid::clipboardRows() const
{
    RowData data;
    for (const auto& values : decodeTsv(juce::SystemClipboard::getTextFromClipboard()))
    {
        auto& row = data.emplace_back();
        for (auto col = 0; col < cols.getCount(); ++col)
            row.emplace_back(std::cmp_less(col, values.size()) ? values[static_cast<std::size_t>(col)] : juce::String());
    }
    return data;
}

bool Grid::insertCopiedRows(int belowRow)
{
    if (!model.insertRows || editor)
        return false;
    const auto data = clipboardRows();
    if (data.empty())
        return false;
    const auto count = static_cast<int>(data.size());
    const auto at = std::clamp(belowRow + 1, 0, rows.getCount());

    auto& mark = currentCutMark();
    Grid* source = nullptr;
    std::vector<int> sourceRows;
    if (mark && !mark->rows.empty() && mark->source != nullptr && mark->text == juce::SystemClipboard::getTextFromClipboard()
        && mark->source->model.removeRows)
    {
        source = mark->source.getComponent();
        sourceRows = mark->rows;
    }
    clearCutMark();

    if (model.beginBatch)
        model.beginBatch();
    const juce::ScopeGuard endBatch{ [this] {
        if (model.endBatch)
            model.endBatch();
    } };

    if (!model.insertRows(at, data))
    {
        reportFailure("Could not insert rows");
        return false;
    }
    notifySelf(RowsInserted{ .at = at, .count = count });

    auto firstInserted = at;
    if (source == this)
    {
        for (auto& row : sourceRows)
            if (row >= at)
                row += count;
        if (!model.removeRows(sourceRows))
        {
            reportFailure("Could not remove the cut rows");
        }
        else
        {
            notifySelf(RowsRemoved{ .rows = sourceRows });
            firstInserted -= static_cast<int>(std::ranges::count_if(sourceRows, [at](int row) { return row < at; }));
        }
    }
    else if (source != nullptr)
    {
        if (source->model.beginBatch)
            source->model.beginBatch();
        if (source->model.removeRows(sourceRows))
            source->notifySelf(RowsRemoved{ .rows = sourceRows });
        else
            source->reportFailure("Could not remove the cut rows");
        if (source->model.endBatch)
            source->model.endBatch();
    }

    selectRows(firstInserted, firstInserted + count - 1);
    postAnnouncement("Inserted " + rowCount(data.size()));
    return true;
}

bool Grid::duplicateSelectedRows()
{
    const auto selected = getSelectedRows();
    if (selected.empty() || editor)
        return false;
    if (model.beginBatch)
        model.beginBatch();
    const juce::ScopeGuard endBatch{ [this] {
        if (model.endBatch)
            model.endBatch();
    } };

    if (model.duplicateRows)
    {
        if (!model.duplicateRows(selected))
        {
            reportFailure("Could not duplicate rows");
            return false;
        }
        postAnnouncement("Duplicated " + rowCount(selected.size()));
        return true;
    }
    if (!model.insertRows)
        return false;

    RowData data;
    for (const auto row : selected)
    {
        auto& values = data.emplace_back();
        for (auto col = 0; col < cols.getCount(); ++col)
            values.push_back(model.getValue(row, col));
    }
    const auto at = selected.back() + 1;
    if (!model.insertRows(at, data))
    {
        reportFailure("Could not duplicate rows");
        return false;
    }
    notifySelf(RowsInserted{ .at = at, .count = static_cast<int>(data.size()) });
    selectRows(at, at + static_cast<int>(data.size()) - 1);
    postAnnouncement("Duplicated " + rowCount(selected.size()));
    return true;
}

bool Grid::deleteSelectedRows()
{
    if (!model.removeRows || editor || !cursor)
        return false;
    auto doomed = getSelectedRows();
    if (doomed.empty())
        doomed.push_back(cursor->row);
    if (model.beginBatch)
        model.beginBatch();
    const juce::ScopeGuard endBatch{ [this] {
        if (model.endBatch)
            model.endBatch();
    } };
    if (!model.removeRows(doomed))
    {
        reportFailure("Could not delete rows");
        return false;
    }
    notifySelf(RowsRemoved{ .rows = doomed });
    postAnnouncement("Deleted " + rowCount(doomed.size()));
    return true;
}

bool Grid::insertBlankRows()
{
    if (!model.insertRows || editor || !cursor)
        return false;
    const auto selected = getSelectedRows();
    const auto count = std::max<std::size_t>(1, selected.size());
    const auto at = (selected.empty() ? cursor->row : selected.back()) + 1;
    const auto col = cursor->col;
    const RowData data(count, std::vector<juce::var>(static_cast<std::size_t>(cols.getCount()), juce::var(juce::String())));
    if (model.beginBatch)
        model.beginBatch();
    const juce::ScopeGuard endBatch{ [this] {
        if (model.endBatch)
            model.endBatch();
    } };
    if (!model.insertRows(at, data))
    {
        reportFailure("Could not insert rows");
        return false;
    }
    notifySelf(RowsInserted{ .at = at, .count = static_cast<int>(count) });
    if (selected.empty())
        setCursor({ .row = at, .col = col });
    else
        selectRows(at, at + static_cast<int>(count) - 1);
    postAnnouncement("Inserted " + rowCount(count));
    return true;
}

bool Grid::undo() const
{
    if (!model.undo)
        return false;
    model.undo();
    return true;
}

bool Grid::redo() const
{
    if (!model.redo)
        return false;
    model.redo();
    return true;
}

void Grid::reportFailure(const juce::String& message)
{
    if (cursor)
        showErrorAt(*cursor, message);
    postAnnouncement(message);
}

} // namespace jgrid
