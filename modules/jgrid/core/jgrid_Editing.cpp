#include "jgrid_Editing.h"

#include <cmath>
#include <numbers>

namespace jgrid
{

juce::String couldNotSetValueMessage() { return "Could not set value"; }

CommitOutcome commitValue(const GridModel& model, Cell cell, const juce::var& value)
{
    if (model.validate)
    {
        const auto result = model.validate(cell.row, cell.col, value);
        if (result.failed())
            return { .kind = CommitOutcome::Kind::invalid, .message = result.getErrorMessage() };
    }
    if (!model.setValue || !model.setValue(cell.row, cell.col, value))
        return { .kind = CommitOutcome::Kind::rejected, .message = couldNotSetValueMessage() };
    return {};
}

bool isTypedText(const juce::String& text)
{
    if (text.isEmpty())
        return false;
    for (const auto c : text) // NOLINT(readability-use-anyofallof)
        if (c < 0x20 || c == 0x7F || (c >= 0xF700 && c <= 0xF8FF))
            return false;
    return true;
}

int shakeOffset(double elapsedSeconds)
{
    if (elapsedSeconds <= 0.0 || elapsedSeconds >= shakeDurationSeconds)
        return 0;
    constexpr auto amplitude = 6.0;
    constexpr auto cycles = 3.0;
    const auto progress = elapsedSeconds / shakeDurationSeconds;
    return static_cast<int>(std::lround(amplitude * (1.0 - progress) * std::sin(2.0 * std::numbers::pi * cycles * progress)));
}

} // namespace jgrid
