#include "Examples.h"

int main()
{
    const juce::ScopedJuceInitialiser_GUI juce;
    for (const auto& create : { createReadOnlyExample,
             createEditableExample,
             createSortingExample,
             createRowsAndUndoExample,
             createMultipleViewsExample,
             createMenusAndColoursExample })
    {
        const auto example = create();
        example->setSize(600, 400);
        if (example->getNumChildComponents() == 0)
            return 1;
    }
    return 0;
}
