#pragma once

#include "jgrid_Grid.h"

#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace jgrid
{

inline constexpr int cellPadding = 4;

/** The process-wide cut: either a cell range or whole rows of one Grid, and the clipboard text
    it put there. */
struct CutMark
{
    juce::Component::SafePointer<Grid> source;
    CellRange range;
    std::vector<int> rows;
    juce::String text;
};

std::optional<CutMark>& currentCutMark();

[[nodiscard]] inline juce::Font cellFont() { return juce::FontOptions{ 14.0f }; }

inline void postAnnouncement(const juce::String& message)
{
    juce::AccessibilityHandler::postAnnouncement(message, juce::AccessibilityHandler::AnnouncementPriority::high);
}

class Grid::Body final : public juce::Component
{
public:
    explicit Body(Grid& owner);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    Grid& grid;
};

class Grid::CellEditor final : public juce::TextEditor
{
public:
    explicit CellEditor(Grid& owner);

    bool keyPressed(const juce::KeyPress& key) override;
    void focusLost(FocusChangeType cause) override;
    void insertTextAtCaret(const juce::String& textToInsert) override;

private:
    Grid& grid;
};

class Grid::ErrorPopup final : public juce::Component, private juce::Timer
{
public:
    explicit ErrorPopup(Grid& owner);

    void show(const juce::String& text, juce::Rectangle<int> cellBounds);
    void reposition(juce::Rectangle<int> cellBounds);
    void dismiss();
    [[nodiscard]] juce::String getMessage() const;

    void paint(juce::Graphics& g) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    void timerCallback() override;

    Grid& grid;
    juce::String message;
};

class Grid::Shaker final : private juce::Timer
{
public:
    explicit Shaker(Grid& owner);

    void start();
    void stop();
    [[nodiscard]] int getOffset() const;

private:
    void timerCallback() override;
    [[nodiscard]] double getElapsedSeconds() const;

    Grid& grid;
    double startMilliseconds = 0.0;
};

class Grid::ColumnHeader final : public juce::Component
{
public:
    explicit ColumnHeader(Grid& owner);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    Grid& grid;
};

class Grid::RowHeader final : public juce::Component
{
public:
    explicit RowHeader(Grid& owner);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    Grid& grid;
};

/** Screen reader support. Nothing is created until a client first queries the table; from then
    on, invisible row components with one cell child per column are kept for the visible rows and
    the cursor row, and the column header holds one invisible child per column. */
class Grid::Accessibility
{
public:
    explicit Accessibility(Grid& owner);
    ~Accessibility();

    Accessibility(const Accessibility&) = delete;
    Accessibility& operator=(const Accessibility&) = delete;
    Accessibility(Accessibility&&) = delete;
    Accessibility& operator=(Accessibility&&) = delete;

    [[nodiscard]] std::unique_ptr<juce::AccessibilityHandler> createTableHandler();

    void activate();
    [[nodiscard]] bool isActive() const;

    /** Rebuilds every accessible component, after the model's shape changes. */
    void rebuild();

    /** Rebuilds only if the column count or header visibility changed, otherwise syncs, so that
        components a screen reader is using survive row changes and sorting. */
    void refresh();

    /** Matches the accessible components to the visible rows and current geometry. */
    void sync();

    /** Gives accessibility focus to the cursor cell while the Grid has keyboard focus. */
    void focusCursorCell();

    /** Tells screen readers that the text of the given cells changed. */
    void cellsChanged(const CellsChanged& change);

    /** Tells screen readers that the column header titles changed. */
    void headersChanged();

    /** Tells screen readers that the rows or columns changed in number. */
    void tableStructureChanged();

    /** Tells screen readers which whole rows are selected, if that changed. */
    void rowSelectionChanged(const std::vector<int>& selectedRows);

    [[nodiscard]] juce::AccessibilityHandler* getRowHandler(int row);
    [[nodiscard]] juce::AccessibilityHandler* getCellHandler(int row, int col);
    [[nodiscard]] juce::AccessibilityHandler* getHeaderHandler();
    [[nodiscard]] std::optional<int> findRow(const juce::AccessibilityHandler& handler) const;
    [[nodiscard]] std::optional<int> findCol(const juce::AccessibilityHandler& handler) const;

private:
    class RowComponent;
    class CellComponent;
    class HeaderCellComponent;
    class TableInterface;

    [[nodiscard]] IndexRange getRowsToKeep() const;

    Grid& grid;
    bool active = false;
    std::map<int, std::unique_ptr<RowComponent>> rowComponents;
    std::vector<std::unique_ptr<HeaderCellComponent>> headerCells;
    std::vector<int> lastSelectedRows;
};

} // namespace jgrid
