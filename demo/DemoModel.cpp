#include "DemoModel.h"

#include <algorithm>
#include <array>
#include <ranges>

namespace
{

constexpr int idColumn = 0;
constexpr int nameColumn = 1;
constexpr int trackColumn = 3;
constexpr int gainColumn = 4;
constexpr int panColumn = 5;
constexpr int takeColumn = 6;

constexpr std::array columnNames{ "ID", "Name", "Instrument", "Track", "Gain dB", "Pan", "Take", "Notes" };
constexpr std::array instruments{ "Vocal", "Guitar", "Bass", "Kick", "Snare", "Piano", "Strings", "Synth" };
constexpr std::array players{ "Alex", "Sam", "Jordan", "Robin", "Morgan", "Casey", "Taylor", "Jamie" };
constexpr std::array notes{ "", "Comp later", "Check timing", "Noisy intro", "Keep", "Double-tracked" };

template <typename Array>
juce::String pick(juce::Random& random, const Array& values)
{
    return values.at(static_cast<std::size_t>(random.nextInt(static_cast<int>(values.size()))));
}

juce::Result integerInRange(const juce::String& text, int low, int high, const juce::String& what)
{
    const auto trimmed = text.trim();
    const auto digits = trimmed.startsWithChar('-') ? trimmed.substring(1) : trimmed;
    if (digits.isEmpty() || !digits.containsOnly("0123456789"))
        return juce::Result::fail(what + " must be a whole number");
    const auto value = trimmed.getIntValue();
    if (value < low || value > high)
        return juce::Result::fail(what + " must be between " + juce::String(low) + " and " + juce::String(high));
    return juce::Result::ok();
}

} // namespace

DemoModel::DemoModel(int rowCount)
{
    juce::Random random{ 42 };
    for (auto row = 0; row < rowCount; ++row)
    {
        rows.push_back({ row + 1,
            pick(random, players),
            pick(random, instruments),
            random.nextInt({ 1, 65 }),
            juce::roundToInt((random.nextDouble() * 240.0) - 180.0) / 10.0,
            random.nextInt({ -100, 101 }),
            random.nextInt({ 1, 13 }),
            pick(random, notes) });
    }

    source.rowCount = [this] { return getRowCount(); };
    source.colCount = [] { return getColCount(); };
    source.getValue = [this](int row, int col) { return getValue(row, col); };
    source.colName = [](int col) { return getColName(col); };
    source.isEditable = [](int, int col) { return isEditable(col); };
    source.validate = [](int, int col, const juce::var& value) { return validate(col, value); };
    source.setValue = [this](int row, int col, const juce::var& value) { return setValue(row, col, value); };
    source.insertRows = [this](int at, const jgrid::RowData& data) { return insertRows(at, data); };
    source.removeRows = [this](const std::vector<int>& indices) { return removeRows(indices); };
    source.undo = [this] { undo(); };
    source.redo = [this] { redo(); };
    source.beginBatch = [this] {
        snapshot();
        inBatch = true;
    };
    source.endBatch = [this] { inBatch = false; };
    source.notifier = &notifier;
    nextId = rowCount + 1;
}

jgrid::DataSource& DemoModel::getSource() { return source; }

int DemoModel::getRowCount() const { return static_cast<int>(rows.size()); }

int DemoModel::getColCount() { return static_cast<int>(columnNames.size()); }

juce::var DemoModel::getValue(int row, int col) const
{
    return rows.at(static_cast<std::size_t>(row)).at(static_cast<std::size_t>(col));
}

juce::String DemoModel::getColName(int col) { return columnNames.at(static_cast<std::size_t>(col)); }

bool DemoModel::isEditable(int col) { return col != idColumn; }

juce::Result DemoModel::validate(int col, const juce::var& value)
{
    const auto text = value.toString();
    switch (col)
    {
        case nameColumn:
            return text.trim().isEmpty() ? juce::Result::fail("Name cannot be empty") : juce::Result::ok();
        case trackColumn:
            return integerInRange(text, 1, 64, "Track");
        case gainColumn:
            if (!text.trim().containsOnly("-.0123456789") || text.trim().isEmpty())
                return juce::Result::fail("Gain must be a number");
            return text.getDoubleValue() > 24.0 ? juce::Result::fail("Gain cannot exceed 24 dB") : juce::Result::ok();
        case panColumn:
            return integerInRange(text, -100, 100, "Pan");
        case takeColumn:
            return integerInRange(text, 1, 999, "Take");
        default:
            return juce::Result::ok();
    }
}

juce::var DemoModel::convert(int col, const juce::var& value)
{
    auto text = value.toString().trim();
    switch (col)
    {
        case trackColumn:
        case panColumn:
        case takeColumn:
            return text.getIntValue();
        case gainColumn:
            return text.getDoubleValue();
        default:
            return text;
    }
}

void DemoModel::snapshot()
{
    if (inBatch)
        return;
    constexpr std::size_t maxUndoSteps = 100;
    undoStack.push_back(rows);
    if (undoStack.size() > maxUndoSteps)
        undoStack.erase(undoStack.begin());
    redoStack.clear();
}

bool DemoModel::setValue(int row, int col, const juce::var& value)
{
    snapshot();
    rows.at(static_cast<std::size_t>(row)).at(static_cast<std::size_t>(col)) = convert(col, value);
    notifier.changed(jgrid::CellsChanged{ .firstRow = row, .numRows = 1, .firstCol = col, .numCols = 1 });
    return true;
}

bool DemoModel::insertRows(int at, const jgrid::RowData& data)
{
    snapshot();
    auto position = rows.begin() + std::clamp(at, 0, getRowCount());
    for (const auto& values : data)
    {
        std::vector<juce::var> row;
        row.reserve(static_cast<std::size_t>(getColCount()));
        for (auto col = 0; col < getColCount(); ++col)
            row.push_back(col == idColumn ? juce::var(nextId++) : convert(col, values.at(static_cast<std::size_t>(col))));
        position = rows.insert(position, std::move(row)) + 1;
    }
    notifier.changed(jgrid::RowsInserted{ .at = at, .count = static_cast<int>(data.size()) });
    return true;
}

bool DemoModel::removeRows(const std::vector<int>& indices)
{
    snapshot();
    for (const auto index : std::views::reverse(indices))
        rows.erase(rows.begin() + index);
    notifier.changed(jgrid::RowsRemoved{ .rows = indices });
    return true;
}

void DemoModel::undo()
{
    if (undoStack.empty())
        return;
    redoStack.push_back(std::move(rows));
    rows = std::move(undoStack.back());
    undoStack.pop_back();
    notifier.changed(jgrid::Reset{});
}

void DemoModel::redo()
{
    if (redoStack.empty())
        return;
    undoStack.push_back(std::move(rows));
    rows = std::move(redoStack.back());
    redoStack.pop_back();
    notifier.changed(jgrid::Reset{});
}
