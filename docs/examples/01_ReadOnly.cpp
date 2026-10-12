#include "Examples.h"

namespace
{

struct Track
{
    juce::String name;
    juce::String instrument;
    int number = 0;
};

class TrackList final : public juce::Component
{
public:
    TrackList()
    {
        jgrid::GridModel model;
        model.rowCount = [this] { return static_cast<int>(tracks.size()); };
        model.colCount = [] { return 3; };
        model.getValue = [this](int row, int col) -> juce::var {
            const auto& track = tracks[static_cast<std::size_t>(row)];
            switch (col)
            {
                case 0:
                    return track.name;
                case 1:
                    return track.instrument;
                default:
                    return track.number;
            }
        };
        model.colName = [](int col) { return juce::StringArray{ "Name", "Instrument", "Track" }[col]; };

        grid.setModel(model);
        addAndMakeVisible(grid);
    }

    void resized() override { grid.setBounds(getLocalBounds()); }

private:
    std::vector<Track> tracks{ { .name = "Alex", .instrument = "Piano", .number = 1 },
        { .name = "Sam", .instrument = "Bass", .number = 2 },
        { .name = "Robin", .instrument = "Vocal", .number = 3 } };
    jgrid::Grid grid;
};

} // namespace

std::unique_ptr<juce::Component> createReadOnlyExample() { return std::make_unique<TrackList>(); }
