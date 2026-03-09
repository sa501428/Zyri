#pragma once

#include <QString>

namespace Zyri {

// Abstract base for all undoable commands (Command pattern).
//
// Commands are pushed to the UndoStack after they have been executed.
// The stack calls undo() to reverse a command and execute() to redo it.
//
// mergeWith() allows consecutive compatible commands (e.g., brush stroke
// segments) to be coalesced into a single undo step.
class CommandBase
{
public:
    virtual ~CommandBase() = default;

    // Apply (or re-apply) the command to the document.
    virtual void execute() = 0;

    // Reverse the effect of execute().
    virtual void undo() = 0;

    // Optional: attempt to merge 'next' into this command.
    // Return true if the merge succeeded (next will be discarded).
    // Default implementation returns false (no merging).
    virtual bool mergeWith(const CommandBase* /*next*/) { return false; }

    // Human-readable description for UI (e.g. Edit menu, history panel).
    [[nodiscard]] virtual QString description() const = 0;
};

} // namespace Zyri
