#include "Examples.h"

namespace
{

class SortableTracks final : public juce::Component
{
public:
    SortableTracks()
    {
        jgrid::GridModel model;
        model.rowCount = [this] { return order.size(); };
        model.colCount = [] { return 2; };
        model.colName = [](int col) { return col == 0 ? "Name" : "Track"; };
        model.getValue = [this](int row, int col) -> juce::var {
            const auto stored = static_cast<std::size_t>(order.toStorage(row));
            return col == 0 ? juce::var(names[stored]) : juce::var(numbers[stored]);
        };

        model.reorderRows = [this](const std::vector<int>& newOrder) {
            order.reorder(newOrder);
            notifier.changed(jgrid::RowsReordered{ .order = newOrder });
        };

        model.compare = [this](int col, int rowA, int rowB) -> std::weak_ordering {
            const auto a = static_cast<std::size_t>(order.toStorage(rowA));
            const auto b = static_cast<std::size_t>(order.toStorage(rowB));
            if (col == 0)
                return names[a].compare(names[b]) <=> 0;
            return numbers[a] <=> numbers[b];
        };

        model.notifier = &notifier;
        grid.setModel(model);
        addAndMakeVisible(grid);
    }

    void sortByNumber() { grid.sortByColumn(1, false); }

    void resized() override { grid.setBounds(getLocalBounds()); }

private:
    std::vector<juce::String> names{ "snare", "Kick", "bass" };
    std::vector<int> numbers{ 3, 1, 2 };
    jgrid::RowOrder order{ 3 };
    jgrid::GridModelNotifier notifier;
    jgrid::Grid grid;
};

} // namespace

std::unique_ptr<juce::Component> createSortingExample()
{
    auto example = std::make_unique<SortableTracks>();
    example->sortByNumber();
    return example;
}
