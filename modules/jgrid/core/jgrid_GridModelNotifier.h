#pragma once

#include "jgrid_Change.h"

#include <juce_core/juce_core.h>

namespace jgrid
{

/** Owned by a model and used to tell every attached Grid what changed.

    A Grid subscribes when it is bound to a GridModel that points at this notifier, and
    unsubscribes when it is unbound or destroyed. If the notifier is destroyed first, each
    subscribed Grid detaches from its model and shows as empty. Use it on the message thread only.
*/
class GridModelNotifier
{
public:
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        Listener(const Listener&) = default;
        Listener& operator=(const Listener&) = default;
        Listener(Listener&&) = default;
        Listener& operator=(Listener&&) = default;

        virtual void modelChanged(const Change& change) = 0;
        virtual void notifierDestroyed(GridModelNotifier& notifier) = 0;
    };

    GridModelNotifier() = default;
    ~GridModelNotifier();

    GridModelNotifier(const GridModelNotifier&) = delete;
    GridModelNotifier& operator=(const GridModelNotifier&) = delete;
    GridModelNotifier(GridModelNotifier&&) = delete;
    GridModelNotifier& operator=(GridModelNotifier&&) = delete;

    /** Sends a change to every subscribed Grid. */
    void changed(const Change& change);

    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    [[nodiscard]] int getNumListeners() const;

private:
    juce::ListenerList<Listener> listeners;
};

} // namespace jgrid
