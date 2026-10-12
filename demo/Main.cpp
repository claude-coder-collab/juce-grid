#include "DemoModel.h"

#include <jgrid/jgrid.h>

namespace
{

void applyScheme(jgrid::Grid& grid, int scheme)
{
    using Ids = jgrid::Grid::ColourIds;
    constexpr std::array ids{ Ids::backgroundColourId, Ids::textColourId, Ids::headerBackgroundColourId, Ids::headerTextColourId, Ids::gridLineColourId, Ids::cursorColourId, Ids::selectionColourId };
    for (const auto id : ids)
        grid.removeColour(id);
    if (scheme == 2)
    {
        grid.setColour(Ids::backgroundColourId, juce::Colours::black);
        grid.setColour(Ids::textColourId, juce::Colours::white);
        grid.setColour(Ids::headerBackgroundColourId, juce::Colours::black);
        grid.setColour(Ids::headerTextColourId, juce::Colours::yellow);
        grid.setColour(Ids::gridLineColourId, juce::Colours::white);
        grid.setColour(Ids::cursorColourId, juce::Colours::yellow);
        grid.setColour(Ids::selectionColourId, juce::Colours::yellow.withAlpha(0.35f));
    }
}

class MainComponent final : public juce::Component
{
public:
    MainComponent()
    {
        grid.setModel(mainView.bind());
        grid.setTitle("Session tracks");
        secondGrid.setModel(sideView.bind());
        secondGrid.setTitle("Session tracks, second view");

        secondView.onClick = [this] {
            secondGrid.setVisible(secondView.getToggleState());
            resized();
        };

        rowHeaders.setToggleState(true, juce::dontSendNotification);
        rowHeaders.onClick = [this] { grid.setRowHeaderVisible(rowHeaders.getToggleState()); };
        columnHeaders.setToggleState(true, juce::dontSendNotification);
        columnHeaders.onClick = [this] { grid.setColumnHeaderVisible(columnHeaders.getToggleState()); };

        scheme.addItem("Look and feel colours", 1);
        scheme.addItem("High contrast", 2);
        scheme.setSelectedId(1, juce::dontSendNotification);
        scheme.setTitle("Colour scheme");
        scheme.onChange = [this] {
            applyScheme(grid, scheme.getSelectedId());
            applyScheme(secondGrid, scheme.getSelectedId());
        };

        status.setText("Right-click or Shift+F10 for menus; drag header edges to resize, double-click to autofit; Cmd+Z undoes.", juce::dontSendNotification);

        for (auto* component : std::initializer_list<juce::Component*>{ &rowHeaders, &columnHeaders, &scheme, &secondView, &status, &grid })
            addAndMakeVisible(component);
        addChildComponent(secondGrid);
        setSize(900, 600);
    }

    void paint(juce::Graphics& g) override { g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId)); }

    void resized() override
    {
        auto area = getLocalBounds().reduced(8);
        auto toolbar = area.removeFromTop(28);
        rowHeaders.setBounds(toolbar.removeFromLeft(130));
        columnHeaders.setBounds(toolbar.removeFromLeft(150));
        scheme.setBounds(toolbar.removeFromLeft(200));
        toolbar.removeFromLeft(16);
        secondView.setBounds(toolbar.removeFromLeft(130));
        status.setBounds(area.removeFromBottom(24));
        area.removeFromTop(8);
        if (secondGrid.isVisible())
        {
            secondGrid.setBounds(area.removeFromBottom(area.getHeight() / 2));
            area.removeFromBottom(8);
        }
        grid.setBounds(area);
    }

    void showScenario(const juce::String& name)
    {
        if (name == "editing")
        {
            grid.setCursor({ .row = 2, .col = 3 });
            grid.startEditingWithText("abc");
            grid.commitEdit();
        }
        else if (name == "selection")
        {
            grid.setCursor({ .row = 1, .col = 1 });
            grid.extendSelection({ .row = 4, .col = 2 });
            grid.cut();
        }
        else if (name == "views")
        {
            secondView.setToggleState(true, juce::sendNotificationSync);
            grid.sortByColumn(3, true);
            secondGrid.sortByColumn(0, false);
            secondGrid.selectColumns(1, 1);
        }
        else if (name == "layout")
        {
            grid.setColumnsHidden({ 5 }, true);
            grid.setColumnWidth(7, 180);
            grid.setRowHeight(2, 40);
            grid.selectRows(2, 3);
        }
        else if (name == "contrast")
        {
            scheme.setSelectedId(2, juce::sendNotificationSync);
            grid.setCursor({ .row = 2, .col = 1 });
            grid.extendSelection({ .row = 4, .col = 3 });
        }
        else
        {
            grid.setCursor({ .row = 2, .col = 1 });
        }
    }

    void visibilityChanged() override
    {
        if (isShowing())
            grid.grabKeyboardFocus();
    }

private:
    DemoModel model{ 500 };
    jgrid::ViewModel mainView{ model.getSource() };
    jgrid::ViewModel sideView{ model.getSource(), { 1, 2, 3 } };
    jgrid::Grid grid;
    jgrid::Grid secondGrid;
    juce::ToggleButton rowHeaders{ "Row headers" };
    juce::ToggleButton columnHeaders{ "Column headers" };
    juce::ComboBox scheme;
    juce::ToggleButton secondView{ "Second view" };
    juce::Label status;
};

void writeSnapshot(const juce::File& file, const juce::String& scenario)
{
    MainComponent component;
    component.setSize(900, 600);
    component.showScenario(scenario);
    const auto image = component.createComponentSnapshot(component.getLocalBounds(), true, 1.5f);
    file.deleteFile();
    juce::FileOutputStream stream(file);
    juce::PNGImageFormat().writeImageToStream(image, stream);
}

class MainWindow final : public juce::DocumentWindow
{
public:
    explicit MainWindow(const juce::String& name) : DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId), allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);
        setResizable(true, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class DemoApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override
    {
        const auto args = juce::StringArray::fromTokens(commandLine, true);
        if (const auto index = args.indexOf("--snapshot"); index >= 0 && index + 1 < args.size())
        {
            writeSnapshot(juce::File(args[index + 1].unquoted()), index + 2 < args.size() ? args[index + 2] : juce::String());
            quit();
            return;
        }
        window = std::make_unique<MainWindow>(getApplicationName());
    }
    void shutdown() override { window.reset(); }
    void systemRequestedQuit() override { quit(); }

private:
    std::unique_ptr<MainWindow> window;
};

} // namespace

START_JUCE_APPLICATION(DemoApplication)
