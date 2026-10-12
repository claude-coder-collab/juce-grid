#pragma once

#include "jgrid_Change.h"

#include <juce_core/juce_core.h>

namespace jgrid
{

//==============================================================================
/**
    Lets a model tell every Grid viewing it what changed.

    The model owns a notifier and points GridModel::notifier at it. Each Grid bound to that model
    subscribes when bound and unsubscribes when unbound or destroyed. Several Grids can share one
    notifier, so one call reaches them all.

    Lifetime is safe in both directions: if the notifier is destroyed first, every subscribed Grid
    detaches from its model and shows as empty.

    @code
    model.setValue = [this](int row, int col, const juce::var& value) {
        store(row, col, value);
        notifier.changed(jgrid::CellsChanged{ row, 1, col, 1 });
        return true;
    };
    @endcode

    Use it on the message thread only.

    @see Change, GridModel::notifier

    @tags{GUI}
*/
class GridModelNotifier
{
public:
    /** Receives notifications. Grid and ViewModel implement this; you rarely need to. */
    class Listener
    {
    public:
        Listener() = default;
        virtual ~Listener() = default;
        /** @internal */
        Listener(const Listener&) = default;
        /** @internal */
        Listener& operator=(const Listener&) = default;
        /** @internal */
        Listener(Listener&&) = default;
        /** @internal */
        Listener& operator=(Listener&&) = default;

        /** Called for every change sent with GridModelNotifier::changed(). */
        virtual void modelChanged(const Change& change) = 0;

        /** Called when the notifier is being destroyed. A listener should stop using it. */
        virtual void notifierDestroyed(GridModelNotifier& notifier) = 0;
    };

    /** Creates a notifier with no listeners. */
    GridModelNotifier() = default;

    /** Destructor. Tells every remaining listener that the notifier is going away. */
    ~GridModelNotifier();

    GridModelNotifier(const GridModelNotifier&) = delete;
    GridModelNotifier& operator=(const GridModelNotifier&) = delete;
    GridModelNotifier(GridModelNotifier&&) = delete;
    GridModelNotifier& operator=(GridModelNotifier&&) = delete;

    /** Sends a change to every listener. */
    void changed(const Change& change);

    /** Adds a listener. Grids do this themselves when bound. */
    void addListener(Listener* listener);

    /** Removes a listener. */
    void removeListener(Listener* listener);

    /** Returns the number of listeners. */
    [[nodiscard]] int getNumListeners() const;

private:
    juce::ListenerList<Listener> listeners;
};

} // namespace jgrid
