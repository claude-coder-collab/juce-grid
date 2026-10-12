#include "Examples.h"

namespace
{

class EditableTracks final : public juce::Component
{
public:
    EditableTracks()
    {
        jgrid::GridModel model;
        model.rowCount = [this] { return static_cast<int>(names.size()); };
        model.colCount = [] { return 2; };
        model.colName = [](int col) { return col == 0 ? "Name" : "Gain dB"; };
        model.getValue = [this](int row, int col) -> juce::var {
            const auto index = static_cast<std::size_t>(row);
            return col == 0 ? juce::var(names[index]) : juce::var(gains[index]);
        };

        model.isEditable = [](int /*row*/, int col) { return col == 1; };

        model.validate = [](int /*row*/, int /*col*/, const juce::var& value) {
            const auto text = value.toString().trim();
            if (!text.containsOnly("-.0123456789") || text.isEmpty())
                return juce::Result::fail("Gain must be a number");
            if (text.getDoubleValue() > 24.0)
                return juce::Result::fail("Gain cannot exceed 24 dB");
            return juce::Result::ok();
        };

        model.setValue = [this](int row, int col, const juce::var& value) {
            gains[static_cast<std::size_t>(row)] = value.toString().getDoubleValue();
            notifier.changed(jgrid::CellsChanged{ .firstRow = row, .numRows = 1, .firstCol = col, .numCols = 1 });
            return true;
        };

        model.notifier = &notifier;
        grid.setModel(model);
        addAndMakeVisible(grid);
    }

    void resized() override { grid.setBounds(getLocalBounds()); }

private:
    std::vector<juce::String> names{ "Kick", "Snare", "Bass" };
    std::vector<double> gains{ -3.0, -6.5, 0.0 };
    jgrid::GridModelNotifier notifier;
    jgrid::Grid grid;
};

} // namespace

std::unique_ptr<juce::Component> createEditableExample() { return std::make_unique<EditableTracks>(); }
