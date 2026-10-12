#include "Examples.h"

namespace
{

class SessionViews final : public juce::Component
{
public:
    SessionViews()
    {
        source.rowCount = [this] { return static_cast<int>(rows.size()); };
        source.colCount = [] { return 3; };
        source.colName = [](int col) { return juce::StringArray{ "Name", "Instrument", "Track" }[col]; };
        source.getValue = [this](int row, int col) { return rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)]; };
        source.setValue = [this](int row, int col, const juce::var& value) {
            rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = value;
            notifier.changed(jgrid::CellsChanged{ .firstRow = row, .numRows = 1, .firstCol = col, .numCols = 1 });
            return true;
        };
        source.notifier = &notifier;

        everything.setModel(allColumns.bind());
        namesOnly.setModel(nameColumns.bind());
        addAndMakeVisible(everything);
        addAndMakeVisible(namesOnly);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        everything.setBounds(area.removeFromTop(area.getHeight() / 2));
        namesOnly.setBounds(area);
    }

private:
    std::vector<std::vector<juce::var>> rows{ { "Alex", "Piano", 1 }, { "Sam", "Bass", 2 } };
    jgrid::GridModelNotifier notifier;
    jgrid::DataSource source;
    jgrid::ViewModel allColumns{ source };
    jgrid::ViewModel nameColumns{ source, { 0, 1 } };
    jgrid::Grid everything;
    jgrid::Grid namesOnly;
};

} // namespace

std::unique_ptr<juce::Component> createMultipleViewsExample() { return std::make_unique<SessionViews>(); }
