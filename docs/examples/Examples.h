#pragma once

#include <jgrid/jgrid.h>

#include <memory>

std::unique_ptr<juce::Component> createReadOnlyExample();
std::unique_ptr<juce::Component> createEditableExample();
std::unique_ptr<juce::Component> createSortingExample();
std::unique_ptr<juce::Component> createRowsAndUndoExample();
std::unique_ptr<juce::Component> createMultipleViewsExample();
std::unique_ptr<juce::Component> createMenusAndColoursExample();
