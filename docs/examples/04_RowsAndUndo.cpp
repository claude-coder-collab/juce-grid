#include "Examples.h"

#include <ranges>

namespace
{

class NotesTable final : public juce::Component
{
public:
    NotesTable()
    {
        jgrid::GridModel model;
        model.rowCount = [this] { return static_cast<int>(notes.size()); };
        model.colCount = [] { return 1; };
        model.colName = [](int) { return juce::String("Note"); };
        model.getValue = [this](int row, int) { return juce::var(notes[static_cast<std::size_t>(row)]); };

        model.setValue = [this](int row, int col, const juce::var& value) {
            remember();
            notes[static_cast<std::size_t>(row)] = value.toString();
            notifier.changed(jgrid::CellsChanged{ .firstRow = row, .numRows = 1, .firstCol = col, .numCols = 1 });
            return true;
        };

        model.insertRows = [this](int at, const jgrid::RowData& rows) {
            remember();
            auto position = notes.begin() + at;
            for (const auto& row : rows)
                position = notes.insert(position, row.front().toString()) + 1;
            notifier.changed(jgrid::RowsInserted{ .at = at, .count = static_cast<int>(rows.size()) });
            return true;
        };

        model.removeRows = [this](const std::vector<int>& rows) {
            remember();
            for (const auto row : std::views::reverse(rows))
                notes.erase(notes.begin() + row);
            notifier.changed(jgrid::RowsRemoved{ .rows = rows });
            return true;
        };

        model.beginBatch = [this] {
            remember();
            inBatch = true;
        };
        model.endBatch = [this] { inBatch = false; };

        model.undo = [this] {
            if (history.empty())
                return;
            notes = history.back();
            history.pop_back();
            notifier.changed(jgrid::Reset{});
        };

        model.notifier = &notifier;
        grid.setModel(model);
        addAndMakeVisible(grid);
    }

    void resized() override { grid.setBounds(getLocalBounds()); }

private:
    void remember()
    {
        if (!inBatch)
            history.push_back(notes);
    }

    std::vector<juce::String> notes{ "Comp vocals", "Check timing", "Bounce stems" };
    std::vector<std::vector<juce::String>> history;
    bool inBatch = false;
    jgrid::GridModelNotifier notifier;
    jgrid::Grid grid;
};

} // namespace

std::unique_ptr<juce::Component> createRowsAndUndoExample() { return std::make_unique<NotesTable>(); }
