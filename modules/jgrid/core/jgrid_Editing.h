#pragma once

#include "jgrid_GridModel.h"
#include "jgrid_Navigation.h"

namespace jgrid
{

/** The result of trying to store an edited value. */
struct CommitOutcome
{
    enum class Kind {
        committed,
        invalid,
        rejected
    };

    Kind kind = Kind::committed;
    juce::String message;
};

/** The message shown when setValue fails. */
[[nodiscard]] juce::String couldNotSetValueMessage();

/** Validates a value and, if valid, stores it with setValue. */
[[nodiscard]] CommitOutcome commitValue(const GridModel& model, Cell cell, const juce::var& value);

/** Whether text typed into a cell should open its editor: true unless it is empty or contains
    control characters or the private-use characters macOS uses for function keys. */
[[nodiscard]] bool isTypedText(const juce::String& text);

/** The macOS character code of the F2 key, which can arrive as text when the Grid accepts text input. */
inline constexpr juce::juce_wchar macF2Character = 0xF705;

/** The horizontal offset of the error shake animation after the given time, 0 once it ends. */
[[nodiscard]] int shakeOffset(double elapsedSeconds);

/** How long the error shake animation lasts. */
inline constexpr double shakeDurationSeconds = 0.35;

} // namespace jgrid
