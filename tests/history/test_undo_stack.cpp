#include <QtTest>
#include "UndoStack.h"
#include "CommandBase.h"

using namespace Zyri;

// A simple counter command for testing.
struct CounterCommand : public CommandBase {
    int*   counter;
    int    delta;
    CounterCommand(int* c, int d) : counter(c), delta(d) {}
    void execute() override { *counter += delta; }
    void undo()    override { *counter -= delta; }
    QString description() const override { return QStringLiteral("Counter"); }
};

class TestUndoStack : public QObject
{
    Q_OBJECT
private slots:
    void pushAndUndo()
    {
        UndoStack stack;
        int v = 0;

        auto cmd = std::make_unique<CounterCommand>(&v, 1);
        cmd->execute();
        stack.push(std::move(cmd));

        QCOMPARE(v, 1);
        QVERIFY(stack.canUndo());
        stack.undo();
        QCOMPARE(v, 0);
        QVERIFY(!stack.canUndo());
        QVERIFY(stack.canRedo());
    }

    void redo()
    {
        UndoStack stack;
        int v = 0;
        auto cmd = std::make_unique<CounterCommand>(&v, 5);
        cmd->execute();
        stack.push(std::move(cmd));
        stack.undo();
        QCOMPARE(v, 0);
        stack.redo();
        QCOMPARE(v, 5);
        QVERIFY(!stack.canRedo());
    }

    void capacityLimit()
    {
        // Stack capacity = 5; push 10 commands; oldest must be discarded.
        UndoStack stack(5);
        int v = 0;
        for (int i = 0; i < 10; ++i) {
            auto cmd = std::make_unique<CounterCommand>(&v, 1);
            cmd->execute();
            stack.push(std::move(cmd));
        }
        QCOMPARE(v, 10);
        QCOMPARE(stack.count(), 5); // only 5 kept

        // Undoing all 5 remaining steps should get us to 5, not 0.
        for (int i = 0; i < 5; ++i) stack.undo();
        QCOMPARE(v, 5);
    }

    void pushClearsRedoHistory()
    {
        UndoStack stack;
        int v = 0;

        auto cmd1 = std::make_unique<CounterCommand>(&v, 1);
        cmd1->execute();
        stack.push(std::move(cmd1));

        stack.undo();
        QVERIFY(stack.canRedo());

        // Push a new command — redo history must be discarded.
        auto cmd2 = std::make_unique<CounterCommand>(&v, 7);
        cmd2->execute();
        stack.push(std::move(cmd2));

        QVERIFY(!stack.canRedo());
    }
};

QTEST_MAIN(TestUndoStack)
#include "test_undo_stack.moc"
