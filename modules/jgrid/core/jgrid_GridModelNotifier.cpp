#include "jgrid_GridModelNotifier.h"

namespace jgrid
{

GridModelNotifier::~GridModelNotifier() // NOLINT(bugprone-exception-escape)
{
    listeners.call([this](Listener& listener) { listener.notifierDestroyed(*this); });
}

void GridModelNotifier::changed(const Change& change)
{
    listeners.call([&change](Listener& listener) { listener.modelChanged(change); });
}

void GridModelNotifier::addListener(Listener* listener) { listeners.add(listener); }

void GridModelNotifier::removeListener(Listener* listener) { listeners.remove(listener); }

int GridModelNotifier::getNumListeners() const { return listeners.size(); }

} // namespace jgrid
