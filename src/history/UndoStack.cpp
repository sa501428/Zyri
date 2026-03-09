#include "UndoStack.h"

namespace Zyri {

UndoStack::UndoStack(int capacity, QObject* parent)
    : QObject(parent)
    , m_capacity(capacity)
{
    m_commands.reserve(static_cast<size_t>(capacity));
}

void UndoStack::push(std::unique_ptr<CommandBase> cmd)
{
    // Discard any redo history above the current cursor.
    if (m_cursor < static_cast<int>(m_commands.size()))
        m_commands.erase(m_commands.begin() + m_cursor, m_commands.end());

    // Try to merge with the previous command.
    if (m_cursor > 0 && m_commands[m_cursor - 1]->mergeWith(cmd.get())) {
        // Merged — no new entry needed; cursor stays the same.
        emit indexChanged();
        return;
    }

    // Enforce capacity limit (ring: drop the oldest entry).
    if (static_cast<int>(m_commands.size()) >= m_capacity) {
        m_commands.erase(m_commands.begin());
        if (m_cursor > 0) --m_cursor;
    }

    m_commands.push_back(std::move(cmd));
    m_cursor = static_cast<int>(m_commands.size());
    emit indexChanged();
}

void UndoStack::undo()
{
    if (!canUndo()) return;
    --m_cursor;
    m_commands[m_cursor]->undo();
    emit indexChanged();
}

void UndoStack::redo()
{
    if (!canRedo()) return;
    m_commands[m_cursor]->execute();
    ++m_cursor;
    emit indexChanged();
}

bool UndoStack::canUndo() const
{
    return m_cursor > 0;
}

bool UndoStack::canRedo() const
{
    return m_cursor < static_cast<int>(m_commands.size());
}

QString UndoStack::undoText() const
{
    if (!canUndo()) return {};
    return m_commands[m_cursor - 1]->description();
}

QString UndoStack::redoText() const
{
    if (!canRedo()) return {};
    return m_commands[m_cursor]->description();
}

void UndoStack::clear()
{
    m_commands.clear();
    m_cursor = 0;
    emit indexChanged();
}

} // namespace Zyri
