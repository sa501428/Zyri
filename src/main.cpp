#include "editor/Application.h"
#include "editor/MainWindow.h"
#include "editor/ProjectIO.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QTextStream>
#include <QThreadPool>
#include <cmath>

int main(int argc, char** argv) {
    Zyri::Application app(argc, argv);
    app.setApplicationName("Zyri");
    app.setOrganizationName("Zyri");
    app.setOrganizationDomain("zyri.app");
    app.setApplicationVersion("0.2.0");
    app.setWindowIcon(QIcon(":/icons/logo.png"));
    QThreadPool::globalInstance()->setMaxThreadCount(2);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Projects or images to open");
    QCommandLineOption demo(
        "render-demo", "Generate a mixed-document fixture without opening a window", "directory");
    parser.addOption(demo);
    parser.process(app);
    if (parser.isSet(demo)) {
        QString dir = parser.value(demo);
        QDir().mkpath(dir);
        Zyri::Document document;
        document.size = {768, 512};
        document.layers.emplace_back();
        Zyri::Session session(document);
        session.foreground = QColor("#327bea");
        session.brush.size = 64;
        session.beginStroke({70, 200});
        for (int i = 0; i < 60; ++i)
            session.strokeTo({70 + i * 9., 200 + std::sin(i * .12) * 80});
        session.endStroke();
        Zyri::Object shape;
        shape.kind = Zyri::ObjectKind::RoundedRectangle;
        shape.rect = {260, 300, 280, 120};
        shape.fill = QColor("#f6ac35");
        session.addObject(shape);
        Zyri::Object text;
        text.kind = Zyri::ObjectKind::Text;
        text.text = "Zyri";
        text.rect = {50, 30, 400, 100};
        text.fill = QColor("#202a40");
        text.font.setPointSize(48);
        session.addObject(text);
        QString error = Zyri::saveProject(session.document(), dir + "/demo.zyri");
        if (error.isEmpty())
            error = Zyri::exportImage(session.document(), dir + "/demo.png");
        if (error.isEmpty())
            error = Zyri::exportSvg(session.document(), dir + "/demo.svg");
        if (error.isEmpty())
            error = Zyri::exportImage(session.document(), dir + "/demo.pdf");
        if (!error.isEmpty()) {
            QTextStream(stderr) << error << Qt::endl;
            return 1;
        }
        return 0;
    }
    Zyri::MainWindow window;
    app.requestClose = [&window] { window.close(); };
    QObject::connect(&window, &Zyri::MainWindow::closeAccepted, &app, [&app] {
        app.quitApproved = true;
        QMetaObject::invokeMethod(&app, &QCoreApplication::quit, Qt::QueuedConnection);
    });
    app.openFile = [&window](QString path) { window.openPath(path); };
    for (const auto& path : app.pendingFiles)
        window.openPath(path);
    for (const auto& path : parser.positionalArguments())
        window.openPath(path);
    if (parser.positionalArguments().isEmpty()) {
        Zyri::Document document;
        document.layers.emplace_back();
        window.addDocument(document, {}, true);
    }
    window.show();
    int result = app.exec();
    app.openFile = {};
    app.requestClose = {};
    return result;
}
