#pragma once

#include "CommandBase.h"
#include <QObject>
#include <memory>
#include <vector>

namespace Zyri {

// Ring-buffer undo/redo stack.
// The stack owns its commands. Once the capacity limit is reached the oldest
// entry is discarded to keep memory bounded.
class UndoStack : public QObject
{
    Q_OBJECT
public:
    static constexpr int kDefaultCapacity = 100;

    explicit UndoStack(int capacity = kDefaultCapacity, QObject* parent = nullptr);

    // Push a new command. The command must already have been executed.
    // If the command can be merged with the current top it will be merged
    // rather than pushed as a separate entry.
    void push(std::unique_ptr<CommandBase> cmd);

    // Undo the top command (moves cursor back by one).
    void undo();

    // Redo the next command after the cursor.
    void redo();

    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    [[nodiscard]] QString undoText() const;
    [[nodiscard]] QString redoText() const;

    // Discard all history.
    void clear();

    [[nodiscard]] int capacity() const { return m_capacity; }
    [[nodiscard]] int count()    const { return static_cast<int>(m_commands.size()); }

signals:
    void indexChanged();

private:
    int m_capacity;
    // Commands in chronological order. m_cursor points one past the last
    // executed command (i.e. the next redo target).
    std::vector<std::unique_ptr<CommandBase>> m_commands;
    int m_cursor = 0;
};

} // namespace Zyri
