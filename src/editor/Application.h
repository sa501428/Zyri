#pragma once
#include <QApplication>
#include <QFileOpenEvent>
#include <functional>

namespace Zyri {
// Native Quit events must use the same asynchronous close flow as the window.
class Application : public QApplication {
  public:
    Application(int& argc, char** argv) : QApplication(argc, argv) {
        setQuitOnLastWindowClosed(false);
    }
    std::function<void(QString)> openFile;
    std::function<void()> requestClose;
    QStringList pendingFiles;
    bool quitApproved = false;

  protected:
    bool event(QEvent* event) override {
        if (event->type() == QEvent::Quit && !quitApproved) {
            event->ignore();
            if (requestClose)
                requestClose();
            return true;
        }
        if (event->type() == QEvent::FileOpen) {
            const auto path = static_cast<QFileOpenEvent*>(event)->file();
            if (!path.isEmpty()) {
                if (openFile)
                    openFile(path);
                else
                    pendingFiles.append(path);
            }
            return true;
        }
        return QApplication::event(event);
    }
};
} // namespace Zyri
