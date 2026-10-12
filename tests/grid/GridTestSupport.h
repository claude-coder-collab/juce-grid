#pragma once

#include <jgrid/jgrid.h>

#include <functional>
#include <memory>
#include <ranges>
#include <vector>

namespace jgrid::test
{

struct TableData
{
    int rows = 0;
    int cols = 0;
    bool named = false;
};

inline GridModel makeModel(const std::shared_ptr<TableData>& data)
{
    GridModel model;
    model.rowCount = [data] { return data->rows; };
    model.colCount = [data] { return data->cols; };
    model.getValue = [](int row, int col) { return juce::var("r" + juce::String(row) + "c" + juce::String(col)); };
    if (data->named)
        model.colName = [](int col) { return "Column " + juce::String(col); };
    return model;
}

struct EditableTable
{
    std::vector<std::vector<juce::String>> cells;
    jgrid::GridModelNotifier notifier;
    int sets = 0;
    int batchesBegun = 0;
    int batchesEnded = 0;
    int undos = 0;
    int redos = 0;
    bool acceptSets = true;
    std::function<bool(int, int)> rejectSet;
    std::function<juce::Result(int, int, const juce::var&)> validate;
    std::function<bool(int, int)> isEditable;

    EditableTable(int rows, int cols)
    {
        for (auto row = 0; row < rows; ++row)
        {
            cells.emplace_back();
            for (auto col = 0; col < cols; ++col)
                cells.back().push_back("r" + juce::String(row) + "c" + juce::String(col));
        }
    }

    [[nodiscard]] const juce::String& at(int row, int col) const { return cells.at(static_cast<std::size_t>(row)).at(static_cast<std::size_t>(col)); }
};

inline GridModel makeEditableModel(const std::shared_ptr<EditableTable>& table)
{
    GridModel model;
    model.rowCount = [table] { return static_cast<int>(table->cells.size()); };
    model.colCount = [table] { return table->cells.empty() ? 0 : static_cast<int>(table->cells.front().size()); };
    model.getValue = [table](int row, int col) { return juce::var(table->at(row, col)); };
    model.setValue = [table](int row, int col, const juce::var& value) {
        ++table->sets;
        if (!table->acceptSets || (table->rejectSet && table->rejectSet(row, col)))
            return false;
        table->cells.at(static_cast<std::size_t>(row)).at(static_cast<std::size_t>(col)) = value.toString();
        table->notifier.changed(CellsChanged{ .firstRow = row, .numRows = 1, .firstCol = col, .numCols = 1 });
        return true;
    };
    model.validate = [table](int row, int col, const juce::var& value) {
        return table->validate ? table->validate(row, col, value) : juce::Result::ok();
    };
    model.isEditable = [table](int row, int col) { return !table->isEditable || table->isEditable(row, col); };
    model.notifier = &table->notifier;
    model.reorderRows = [table](const std::vector<int>& order) {
        std::vector<std::vector<juce::String>> reordered;
        reordered.reserve(order.size());
        for (const auto oldRow : order)
            reordered.push_back(table->cells.at(static_cast<std::size_t>(oldRow)));
        table->cells = std::move(reordered);
        table->notifier.changed(RowsReordered{ .order = order });
    };
    model.insertRows = [table](int at, const RowData& rows) {
        auto position = table->cells.begin() + at;
        for (const auto& values : rows)
        {
            std::vector<juce::String> texts;
            texts.reserve(values.size());
            for (const auto& value : values)
                texts.push_back(value.toString());
            position = table->cells.insert(position, texts) + 1;
        }
        table->notifier.changed(RowsInserted{ .at = at, .count = static_cast<int>(rows.size()) });
        return true;
    };
    model.removeRows = [table](const std::vector<int>& rows) {
        for (const auto row : std::views::reverse(rows))
            table->cells.erase(table->cells.begin() + row);
        table->notifier.changed(RowsRemoved{ .rows = rows });
        return true;
    };
    model.undo = [table] { ++table->undos; };
    model.redo = [table] { ++table->redos; };
    model.beginBatch = [table] { ++table->batchesBegun; };
    model.endBatch = [table] { ++table->batchesEnded; };
    return model;
}

inline juce::KeyPress key(int code, juce::ModifierKeys mods = {})
{
    return { code, mods, 0 };
}

} // namespace jgrid::test
