#pragma once

#include <jgrid/jgrid.h>

#include <vector>

class DemoModel
{
public:
    explicit DemoModel(int rowCount);

    [[nodiscard]] jgrid::DataSource& getSource();
    [[nodiscard]] int getRowCount() const;
    [[nodiscard]] static int getColCount();
    [[nodiscard]] juce::var getValue(int row, int col) const;
    [[nodiscard]] static juce::String getColName(int col);
    [[nodiscard]] static bool isEditable(int col);
    [[nodiscard]] static juce::Result validate(int col, const juce::var& value);
    bool setValue(int row, int col, const juce::var& value);
    bool insertRows(int at, const jgrid::RowData& data);
    bool removeRows(const std::vector<int>& indices);
    void undo();
    void redo();

private:
    using Rows = std::vector<std::vector<juce::var>>;

    [[nodiscard]] static juce::var convert(int col, const juce::var& value);
    void snapshot();

    Rows rows;
    std::vector<Rows> undoStack;
    std::vector<Rows> redoStack;
    bool inBatch = false;
    int nextId = 1;
    jgrid::GridModelNotifier notifier;
    jgrid::DataSource source;
};
