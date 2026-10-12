#include <jgrid/core/jgrid_Editing.h>

#include <catch2/catch_test_macros.hpp>

using jgrid::Cell;
using jgrid::CommitOutcome;

namespace
{

struct Recorder
{
    int sets = 0;
    juce::var lastValue;
};

jgrid::GridModel recordingModel(Recorder& recorder, bool accept)
{
    jgrid::GridModel model;
    model.setValue = [&recorder, accept](int, int, const juce::var& value) {
        ++recorder.sets;
        recorder.lastValue = value;
        return accept;
    };
    return model;
}

} // namespace

TEST_CASE("Valid values are stored", "[editing]")
{
    Recorder recorder;
    auto model = recordingModel(recorder, true);
    model.validate = [](int, int, const juce::var&) { return juce::Result::ok(); };

    const auto outcome = commitValue(model, Cell{}, juce::var("42"));
    CHECK(outcome.kind == CommitOutcome::Kind::committed);
    CHECK(recorder.sets == 1);
    CHECK(recorder.lastValue.toString() == "42");
}

TEST_CASE("Invalid values are not stored and report the model's message", "[editing]")
{
    Recorder recorder;
    auto model = recordingModel(recorder, true);
    model.validate = [](int, int, const juce::var&) { return juce::Result::fail("Must be a number"); };

    const auto outcome = commitValue(model, Cell{}, juce::var("x"));
    CHECK(outcome.kind == CommitOutcome::Kind::invalid);
    CHECK(outcome.message == "Must be a number");
    CHECK(recorder.sets == 0);
}

TEST_CASE("A failed setValue is reported as rejected", "[editing]")
{
    Recorder recorder;
    const auto model = recordingModel(recorder, false);
    const auto outcome = commitValue(model, Cell{}, juce::var("x"));
    CHECK(outcome.kind == CommitOutcome::Kind::rejected);
    CHECK(outcome.message == jgrid::couldNotSetValueMessage());

    CHECK(commitValue(jgrid::GridModel{}, Cell{}, juce::var("x")).kind == CommitOutcome::Kind::rejected);
}

TEST_CASE("Only printable text opens an editor", "[editing]")
{
    CHECK(jgrid::isTypedText("a"));
    CHECK(jgrid::isTypedText(juce::String::fromUTF8("\xc3\xa9")));
    CHECK(jgrid::isTypedText(juce::String::fromUTF8("\xe6\x97\xa5\xe6\x9c\xac")));
    CHECK(jgrid::isTypedText(" "));
    CHECK_FALSE(jgrid::isTypedText({}));
    CHECK_FALSE(jgrid::isTypedText("\t"));
    CHECK_FALSE(jgrid::isTypedText(juce::String::charToString(0x7F)));
    CHECK_FALSE(jgrid::isTypedText(juce::String::charToString(jgrid::macF2Character)));
}

TEST_CASE("Model editability combines setValue and isEditable", "[editing]")
{
    jgrid::GridModel model;
    CHECK_FALSE(model.canEdit(0, 0));
    model.setValue = [](int, int, const juce::var&) { return true; };
    CHECK(model.canEdit(0, 0));
    model.isEditable = [](int, int col) { return col != 0; };
    CHECK_FALSE(model.canEdit(0, 0));
    CHECK(model.canEdit(0, 1));
}

TEST_CASE("The shake decays to rest", "[editing]")
{
    CHECK(jgrid::shakeOffset(0.0) == 0);
    CHECK(jgrid::shakeOffset(jgrid::shakeDurationSeconds) == 0);
    CHECK(jgrid::shakeOffset(10.0) == 0);
    auto moved = false;
    for (auto step = 0; step < 35; ++step)
    {
        const auto offset = jgrid::shakeOffset(step * 0.01);
        CHECK(std::abs(offset) <= 6);
        moved = moved || offset != 0;
    }
    CHECK(moved);
}
