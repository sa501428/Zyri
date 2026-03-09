#include <QApplication>
#include <QIcon>
#include <QMainWindow>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QSurfaceFormat>

int main(int argc, char* argv[])
{
    // Request OpenGL 3.3 Core for the canvas viewport.
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(0); // no MSAA on the canvas — we handle AA in shaders
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Zyri"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setOrganizationName(QStringLiteral("Zyri"));
    app.setOrganizationDomain(QStringLiteral("zyri.app"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/logo.png")));

    // ── Main window ───────────────────────────────────────────────────────────
    QMainWindow win;
    win.setWindowTitle(QStringLiteral("Zyri"));
    win.resize(1400, 900);

    // Placeholder central widget — replaced by CanvasViewport in Phase 2.
    auto* central = new QWidget(&win);
    auto* layout  = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* placeholder = new QLabel(
        QStringLiteral("<center><h2>Zyri</h2>"
                       "<p style='color:#888'>Canvas will appear here in Phase 2.</p></center>"),
        central);
    placeholder->setAlignment(Qt::AlignCenter);
    layout->addWidget(placeholder);
    win.setCentralWidget(central);

    // ── Menu bar ──────────────────────────────────────────────────────────────
    auto* fileMenu = win.menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("&New"),       QKeySequence::New);
    fileMenu->addAction(QStringLiteral("&Open…"),     QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("&Save"),      QKeySequence::Save);
    fileMenu->addAction(QStringLiteral("Save &As…"),  QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("&Quit"),      QKeySequence::Quit,
                        &app, &QApplication::quit);

    auto* editMenu = win.menuBar()->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(QStringLiteral("&Undo"),  QKeySequence::Undo);
    editMenu->addAction(QStringLiteral("&Redo"),  QKeySequence::Redo);

    // ── Status bar ────────────────────────────────────────────────────────────
    win.statusBar()->showMessage(QStringLiteral("Ready"));

    win.show();
    return app.exec();
}
