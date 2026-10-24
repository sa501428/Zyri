#include "MainWindow.h"
#include "ProjectIO.h"
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLockFile>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QSvgRenderer>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QUuid>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <algorithm>

namespace Zyri {
MainWindow::MainWindow(QString root, bool recovery)
    : m_storage(root.isEmpty()
                    ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                    : std::move(root)) {
    setObjectName("ZyriMainWindow");
    setWindowTitle("Zyri");
    resize(1440, 920);
    setAcceptDrops(true);
    QDir().mkpath(m_storage);
    m_settings = std::make_unique<QSettings>(m_storage + "/settings.ini", QSettings::IniFormat);
    m_instance = m_storage + "/recovery/" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDir().mkpath(m_instance);
    m_lock = std::make_unique<QLockFile>(m_instance + "/instance.lock");
    m_lock->setStaleLockTime(0);
    m_lock->tryLock();
    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setTabsClosable(true);
    setCentralWidget(m_tabs);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this] { refresh(); });
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    buildMenus();
    buildPanels();
    restoreGeometry(m_settings->value("geometry").toByteArray());
    restoreState(m_settings->value("layout").toByteArray());
    setTheme(m_settings->value("theme", 0).toInt());
    m_autosave = new QTimer(this);
    m_autosave->setObjectName("RecoveryTimer");
    int autosaveSeconds = std::clamp(m_settings->value("autosaveSeconds", 30).toInt(), 0, 300);
    m_autosave->setInterval(std::max(1, autosaveSeconds) * 1000);
    connect(m_autosave, &QTimer::timeout, this, &MainWindow::autosave);
    if (autosaveSeconds > 0)
        m_autosave->start();
    if (recovery)
        QTimer::singleShot(0, this, &MainWindow::recover);
    statusBar()->showMessage(tr("Ready — all documents stay on this computer"));
    refresh();
}
MainWindow::~MainWindow() = default;
Canvas* MainWindow::current() const {
    return qobject_cast<Canvas*>(m_tabs->currentWidget());
}
QAction* MainWindow::action(QMenu* menu, QString text, QKeySequence key,
                            std::function<void()> callback) {
    auto a = menu->addAction(text);
    a->setObjectName(menu->title() + "/" + text);
    a->setShortcut(QKeySequence(
        m_settings->value("shortcuts/" + a->objectName(), key.toString(QKeySequence::PortableText))
            .toString()));
    connect(a, &QAction::triggered, this, std::move(callback));
    return a;
}
void MainWindow::buildMenus() {
    auto file = menuBar()->addMenu(tr("&File"));
    action(file, tr("&New…"), QKeySequence::New, [this] { newDocument(); });
    action(file, tr("&Open…"), QKeySequence::Open, [this] {
        QString path = QFileDialog::getOpenFileName(
            this, tr("Open"), {},
            tr("Images and projects (*.zyri *.png *.jpg *.jpeg *.svg *.pdf);;All files (*)"));
        if (!path.isEmpty())
            openPath(path);
    });
    auto recent = file->addMenu(tr("Open recent"));
    connect(recent, &QMenu::aboutToShow, this, [this, recent] {
        recent->clear();
        for (const auto& path : m_settings->value("recent").toStringList())
            connect(recent->addAction(QFileInfo(path).fileName()), &QAction::triggered, this,
                    [this, path] { openPath(path); });
    });
    m_save = action(file, tr("&Save"), QKeySequence::Save, [this] { save(current()); });
    action(file, tr("Save &As…"), QKeySequence::SaveAs, [this] { save(current(), true); });
    action(file, tr("&Export…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E),
           [this] { exportCurrent(); });
    action(file, tr("&Close document"), QKeySequence::Close,
           [this] { closeTab(m_tabs->currentIndex()); });
    file->addSeparator();
    action(file, tr("Quit"), QKeySequence::Quit, [this] { close(); });
    auto edit = menuBar()->addMenu(tr("&Edit"));
    m_undo = action(edit, tr("Undo"), QKeySequence::Undo, [this] {
        if (auto c = current())
            c->session()->undo();
    });
    m_redo = action(edit, tr("Redo"), QKeySequence::Redo, [this] {
        if (auto c = current())
            c->session()->redo();
    });
    edit->addSeparator();
    action(edit, tr("Copy"), QKeySequence::Copy, [this] {
        if (auto c = current())
            QApplication::clipboard()->setImage(c->session()->copyPixels());
    });
    action(edit, tr("Copy merged"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C), [this] {
        if (auto c = current())
            QApplication::clipboard()->setImage(c->session()->copyPixels(true));
    });
    action(edit, tr("Cut"), QKeySequence::Cut, [this] {
        if (auto c = current()) {
            QApplication::clipboard()->setImage(c->session()->copyPixels());
            c->session()->clearPixels();
        }
    });
    action(edit, tr("Paste as layer"), QKeySequence::Paste, [this] {
        if (auto c = current())
            c->session()->paste(QApplication::clipboard()->image());
    });
    action(edit, tr("New from clipboard"), {}, [this] {
        QImage image = QApplication::clipboard()->image();
        if (!validSize(image.size()))
            return;
        Document d;
        d.size = image.size();
        Layer l;
        replaceRaster(l, image);
        d.layers.push_back(l);
        addDocument(d);
    });
    action(edit, tr("Delete pixels or object"), {}, [this] {
        if (auto c = current())
            c->session()->clearPixels();
    });
    edit->addSeparator();
    auto preferencesAction =
        action(edit, tr("Preferences…"), QKeySequence(Qt::CTRL | Qt::Key_Comma),
               [this] { preferences(); });
    preferencesAction->setMenuRole(QAction::PreferencesRole);
    auto select = menuBar()->addMenu(tr("&Select"));
    action(select, tr("All"), QKeySequence::SelectAll, [this] {
        if (auto c = current()) {
            QPainterPath p;
            p.addRect(QRectF(QPointF(), c->session()->document().size));
            c->session()->select(p);
        }
    });
    action(select, tr("Deselect"), QKeySequence(Qt::CTRL | Qt::Key_D), [this] {
        if (auto c = current())
            c->session()->deselect();
    });
    action(select, tr("Invert"), {}, [this] {
        if (auto c = current()) {
            QPainterPath all;
            all.addRect(QRectF(QPointF(), c->session()->document().size));
            c->session()->select(
                c->session()->hasSelection ? all.subtracted(c->session()->selection) : all);
        }
    });
    auto layer = menuBar()->addMenu(tr("&Layer"));
    action(layer, tr("Add raster layer"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [this] {
        if (auto c = current())
            c->session()->addLayer(LayerKind::Raster);
    });
    action(layer, tr("Add vector layer"), {}, [this] {
        if (auto c = current())
            c->session()->addLayer(LayerKind::Vector);
    });
    action(layer, tr("Duplicate layer"), {}, [this] {
        if (auto c = current())
            c->session()->duplicateLayer();
    });
    action(layer, tr("Delete layer"), {}, [this] {
        if (auto c = current())
            c->session()->deleteLayer();
    });
    action(layer, tr("Rasterize layer"), {}, [this] {
        if (auto c = current())
            c->session()->rasterize();
    });
    auto image = menuBar()->addMenu(tr("&Image"));
    auto resizeDialog = [this](bool resample) {
        auto c = current();
        if (!c)
            return;
        c->cancelInteraction();
        QDialog dialog(this);
        dialog.setWindowTitle(resample ? tr("Resize image") : tr("Resize canvas"));
        QFormLayout layout(&dialog);
        QSpinBox width, height, x, y;
        width.setRange(1, MaxDimension);
        height.setRange(1, MaxDimension);
        width.setValue(c->session()->document().size.width());
        height.setValue(c->session()->document().size.height());
        x.setRange(-MaxDimension, MaxDimension);
        y.setRange(-MaxDimension, MaxDimension);
        layout.addRow(tr("Width (px)"), &width);
        layout.addRow(tr("Height (px)"), &height);
        if (!resample) {
            layout.addRow(tr("Content offset X"), &x);
            layout.addRow(tr("Content offset Y"), &y);
        }
        QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout.addRow(&buttons);
        connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() == QDialog::Accepted) {
            QSize size(width.value(), height.value());
            QPoint offset(x.value(), y.value());
            runOperation(resample ? tr("Resize image") : tr("Resize canvas"),
                         [=](Session& s) { s.resize(size, resample, offset); });
        }
    };
    action(image, tr("Resize image…"), {}, [resizeDialog] { resizeDialog(true); });
    action(image, tr("Resize canvas…"), {}, [resizeDialog] { resizeDialog(false); });
    action(image, tr("Resolution / PPI…"), {}, [this] {
        if (auto c = current()) {
            bool ok;
            int ppi = QInputDialog::getInt(this, tr("Resolution"),
                                           tr("Pixels per inch (changes physical export size)"),
                                           c->session()->document().ppi, 1, 2400, 1, &ok);
            if (ok)
                c->session()->apply(tr("Resolution"), [=](Document& d) { d.ppi = ppi; });
        }
    });
    auto transform = menuBar()->addMenu(tr("&Transform"));
    auto affine = [this](double degrees, double sx, double sy) {
        if (auto c = current()) {
            QPointF center = c->session()->object()
                                 ? c->session()->object()->bounds().center()
                                 : QRectF(QPointF(), c->session()->document().size).center();
            QTransform t;
            t.translate(center.x(), center.y());
            t.rotate(degrees);
            t.scale(sx, sy);
            t.translate(-center.x(), -center.y());
            runOperation(tr("Transform"), [t](Session& s) { s.transformActive(t); });
        }
    };
    action(transform, tr("Flip horizontal"), {}, [affine] { affine(0, -1, 1); });
    action(transform, tr("Flip vertical"), {}, [affine] { affine(0, 1, -1); });
    action(transform, tr("Rotate 90°"), {}, [affine] { affine(90, 1, 1); });
    action(transform, tr("Rotate…"), {}, [this, affine] {
        bool ok;
        double angle =
            QInputDialog::getDouble(this, tr("Rotate"), tr("Degrees"), 0, -360, 360, 1, &ok);
        if (ok)
            affine(angle, 1, 1);
    });
    action(transform, tr("Scale…"), {}, [this, affine] {
        bool ok;
        double percent =
            QInputDialog::getDouble(this, tr("Scale"), tr("Percent"), 100, 1, 1000, 1, &ok);
        if (ok)
            affine(0, percent / 100, percent / 100);
    });
    auto viewMenu = menuBar()->addMenu(tr("&View"));
    action(viewMenu, tr("Fit canvas"), QKeySequence(Qt::CTRL | Qt::Key_0), [this] {
        if (auto c = current())
            c->fit();
    });
    action(viewMenu, tr("100%"), QKeySequence(Qt::CTRL | Qt::Key_1), [this] {
        if (auto c = current())
            c->zoom(1 / c->view.zoom);
    });
    action(viewMenu, tr("Actual display pixels"), {}, [this] {
        if (auto c = current())
            c->zoom(1 / (c->view.zoom * c->devicePixelRatioF()));
    });
    action(viewMenu, tr("Zoom in"), QKeySequence::ZoomIn, [this] {
        if (auto c = current())
            c->zoom(1.25);
    });
    action(viewMenu, tr("Zoom out"), QKeySequence::ZoomOut, [this] {
        if (auto c = current())
            c->zoom(.8);
    });
    action(viewMenu, tr("Rotate view left"), {}, [this] {
        if (auto c = current())
            c->rotate(-15);
    });
    action(viewMenu, tr("Rotate view right"), {}, [this] {
        if (auto c = current())
            c->rotate(15);
    });
    action(viewMenu, tr("Reset view rotation"), {}, [this] {
        if (auto c = current()) {
            c->view.rotation = 0;
            c->update();
        }
    });
    auto help = menuBar()->addMenu(tr("&Help"));
    action(help, tr("Tool guide"), {}, [this] {
        QMessageBox::information(
            this, tr("Tool guide"),
            tr("Space or middle mouse: pan. Ctrl/Cmd + wheel or pinch: zoom.\nSelection: Shift "
               "adds, Alt subtracts, Shift+Alt intersects.\nMove: drag content; drag object "
               "corners to scale.\nPen: click nodes, drag for curves, Enter or right click to "
               "finish.\nNodes: drag points/handles, double click to append, Delete to "
               "remove.\nDouble click a text object to edit. Escape cancels a gesture.\nRaster "
               "tools require an unlocked raster layer. Shapes create a vector layer when "
               "needed.\nSVG supports simple geometry; complex input offers raster import. PDF "
               "export is flattened."));
    });
    action(help, tr("About Zyri"), {}, [this] {
        QMessageBox::about(this, tr("Zyri"),
                           tr("Zyri 0.2\nLocal painting and vector illustration\nC++20 / Qt "
                              "6\nDevelopment build"));
    });
    auto tools = addToolBar(tr("Tools"));
    tools->setObjectName("Tools");
    addToolBar(Qt::LeftToolBarArea, tools);
    tools->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tools->setIconSize(QSize(24, 24));
    auto group = new QActionGroup(this);
    const std::vector<std::pair<QString, Tool>> list = {
        {tr("Brush"), Tool::Brush},
        {tr("Pencil"), Tool::Pencil},
        {tr("Eraser"), Tool::Eraser},
        {tr("Fill"), Tool::Fill},
        {tr("Picker"), Tool::Picker},
        {tr("Rectangle select"), Tool::RectangleSelect},
        {tr("Ellipse select"), Tool::EllipseSelect},
        {tr("Lasso"), Tool::Lasso},
        {tr("Move"), Tool::Move},
        {tr("Rectangle"), Tool::Rectangle},
        {tr("Ellipse"), Tool::Ellipse},
        {tr("Rounded rect"), Tool::RoundedRectangle},
        {tr("Line"), Tool::Line},
        {tr("Polygon"), Tool::Polygon},
        {tr("Pen path"), Tool::Path},
        {tr("Nodes"), Tool::Node},
        {tr("Text"), Tool::Text},
        {tr("Crop"), Tool::Crop}};
    const QMap<int, QString> icons = {{int(Tool::Brush), "tool-brush"},
                                      {int(Tool::Pencil), "tool-pencil"},
                                      {int(Tool::Eraser), "tool-eraser"},
                                      {int(Tool::Fill), "tool-fill"},
                                      {int(Tool::Picker), "tool-eyedropper"},
                                      {int(Tool::RectangleSelect), "tool-rect-select"},
                                      {int(Tool::EllipseSelect), "tool-ellipse-select"},
                                      {int(Tool::Lasso), "tool-lasso"},
                                      {int(Tool::Move), "tool-move"},
                                      {int(Tool::Rectangle), "tool-rectangle"},
                                      {int(Tool::Ellipse), "tool-ellipse"},
                                      {int(Tool::RoundedRectangle), "tool-rectangle"},
                                      {int(Tool::Line), "tool-line"},
                                      {int(Tool::Polygon), "tool-polygon"},
                                      {int(Tool::Path), "vector-anchor-add"},
                                      {int(Tool::Node), "tool-transform"},
                                      {int(Tool::Text), "tool-text"},
                                      {int(Tool::Crop), "tool-crop"}};
    for (const auto& [label, tool] : list) {
        auto a = tools->addAction(label);
        a->setToolTip(label);
        a->setProperty("iconResource", ":/icons/" + icons.value(int(tool)) + ".svg");
        a->setCheckable(true);
        a->setData(int(tool));
        group->addAction(a);
        if (tool == Tool::Brush)
            a->setChecked(true);
        connect(a, &QAction::triggered, this, [this, tool] {
            if (auto c = current())
                c->setTool(tool);
        });
    }
    connect(m_tabs, &QTabWidget::currentChanged, this, [this, group] {
        if (auto c = current())
            for (auto a : group->actions())
                a->setChecked(a->data().toInt() == int(c->tool));
    });
    auto common = addToolBar(tr("Document"));
    common->setObjectName("DocumentToolbar");
    common->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    auto newAction = file->actions().value(0);
    auto openAction = file->actions().value(1);
    newAction->setProperty("iconResource", ":/icons/doc-new.svg");
    openAction->setProperty("iconResource", ":/icons/doc-open.svg");
    m_save->setProperty("iconResource", ":/icons/doc-save-disk.svg");
    m_undo->setProperty("iconResource", ":/icons/ui-undo.svg");
    m_redo->setProperty("iconResource", ":/icons/ui-redo.svg");
    common->addAction(newAction);
    common->addAction(openAction);
    common->addAction(m_save);
    common->addSeparator();
    common->addAction(m_undo);
    common->addAction(m_redo);
}
void MainWindow::buildPanels() {
    auto dock = [this](QString title, QString name) {
        auto d = new QDockWidget(title, this);
        d->setObjectName(name);
        auto body = new QWidget(d);
        d->setWidget(body);
        addDockWidget(Qt::RightDockWidgetArea, d);
        return body;
    };
    QWidget* options = dock(tr("Tool options"), "ToolOptions");
    auto form = new QFormLayout(options);
    auto number = [&](QString label, double low, double high, double value, int decimals = 2) {
        auto spin = new QDoubleSpinBox(options);
        spin->setRange(low, high);
        spin->setDecimals(decimals);
        spin->setValue(value);
        form->addRow(label, spin);
        return spin;
    };
    m_size = number(tr("Size (px)"), 1, 1024, 20, 1);
    m_hardness = number(tr("Hardness"), 0, 1, .8);
    m_opacity = number(tr("Stroke opacity"), 0, 1, 1);
    m_flow = number(tr("Dab flow"), 0, 1, 1);
    m_spacing = number(tr("Spacing"), .01, 1, .15);
    for (auto spin : {m_size, m_hardness, m_opacity, m_flow, m_spacing})
        connect(spin, &QDoubleSpinBox::valueChanged, this, [this] {
            if (m_refreshing)
                return;
            if (auto c = current()) {
                auto& b = c->session()->brush;
                b.size = m_size->value();
                b.hardness = m_hardness->value();
                b.opacity = m_opacity->value();
                b.flow = m_flow->value();
                b.spacing = m_spacing->value();
            }
        });
    auto pressure = m_pressure = new QCheckBox(tr("Pressure changes size"), options);
    pressure->setChecked(true);
    form->addRow(pressure);
    connect(pressure, &QCheckBox::toggled, this, [this](bool value) {
        if (!m_refreshing)
            if (auto c = current())
                c->session()->brush.pressure = value;
    });
    m_tolerance = new QSpinBox(options);
    m_tolerance->setRange(0, 255);
    m_tolerance->setValue(20);
    form->addRow(tr("Fill tolerance"), m_tolerance);
    connect(m_tolerance, &QSpinBox::valueChanged, this, [this](int value) {
        if (auto c = current())
            c->fillTolerance = value;
    });
    auto visible = m_sampleVisible = new QCheckBox(tr("Fill samples visible layers"), options);
    form->addRow(visible);
    connect(visible, &QCheckBox::toggled, this, [this](bool value) {
        if (!m_refreshing)
            if (auto c = current())
                c->sampleVisible = value;
    });
    QWidget* color = dock(tr("Color"), "Colors");
    auto colors = new QFormLayout(color);
    m_foreground = new QPushButton(tr("Foreground"), color);
    m_background = new QPushButton(tr("Background / stroke"), color);
    colors->addRow(m_foreground);
    colors->addRow(m_background);
    m_hex = new QLineEdit(color);
    colors->addRow(tr("RGBA hex"), m_hex);
    auto chooseColor = [this](bool foreground) {
        if (auto c = current()) {
            QColor value = QColorDialog::getColor(
                foreground ? c->session()->foreground : c->session()->background, this,
                tr("Choose color"), QColorDialog::ShowAlphaChannel);
            if (value.isValid()) {
                if (foreground)
                    c->session()->foreground = value;
                else
                    c->session()->background = value;
                refresh();
            }
        }
    };
    connect(m_foreground, &QPushButton::clicked, this, [chooseColor] { chooseColor(true); });
    connect(m_background, &QPushButton::clicked, this, [chooseColor] { chooseColor(false); });
    connect(m_hex, &QLineEdit::editingFinished, this, [this] {
        if (auto c = current()) {
            QString text = m_hex->text().trimmed();
            if (text.startsWith('#'))
                text.remove(0, 1);
            QColor color;
            if (text.size() == 8) {
                bool ok;
                quint32 rgba = text.toUInt(&ok, 16);
                if (ok)
                    color = QColor((rgba >> 24) & 255, (rgba >> 16) & 255, (rgba >> 8) & 255,
                                   rgba & 255);
            } else
                color = QColor('#' + text);
            if (color.isValid())
                c->session()->foreground = color;
            refresh();
        }
    });
    auto swatches = new QHBoxLayout;
    for (QColor colorValue :
         {QColor(Qt::black), QColor(Qt::white), QColor("#ef4444"), QColor("#f59e0b"),
          QColor("#22c55e"), QColor("#3b82f6"), QColor("#a855f7")}) {
        auto b = new QPushButton(color);
        b->setFixedSize(23, 23);
        b->setStyleSheet("background:" + colorValue.name());
        connect(b, &QPushButton::clicked, this, [this, colorValue] {
            if (auto c = current())
                c->session()->foreground = colorValue;
            refresh();
        });
        swatches->addWidget(b);
    }
    colors->addRow(swatches);
    QWidget* layers = dock(tr("Layers"), "Layers");
    auto layerLayout = new QVBoxLayout(layers);
    m_layers = new QListWidget(layers);
    m_layers->setMinimumHeight(140);
    layerLayout->addWidget(m_layers);
    auto row = new QHBoxLayout;
    layerLayout->addLayout(row);
    auto button = [&](QString text, std::function<void()> op) {
        auto b = new QPushButton(text, layers);
        row->addWidget(b);
        connect(b, &QPushButton::clicked, this, std::move(op));
    };
    button("+", [this] {
        if (auto c = current())
            c->session()->addLayer(LayerKind::Raster);
    });
    button(tr("Vector"), [this] {
        if (auto c = current())
            c->session()->addLayer(LayerKind::Vector);
    });
    button("−", [this] {
        if (auto c = current())
            c->session()->deleteLayer();
    });
    button("↑", [this] {
        if (auto c = current())
            c->session()->reorderLayer(1);
    });
    button("↓", [this] {
        if (auto c = current())
            c->session()->reorderLayer(-1);
    });
    auto properties = new QFormLayout;
    layerLayout->addLayout(properties);
    m_layerOpacity = new QDoubleSpinBox(layers);
    m_layerOpacity->setRange(0, 1);
    m_layerOpacity->setSingleStep(.05);
    properties->addRow(tr("Layer opacity"), m_layerOpacity);
    m_blend = new QComboBox(layers);
    m_blend->addItems({"Normal", "Multiply", "Screen", "Overlay"});
    properties->addRow(tr("Blend"), m_blend);
    auto locked = m_locked = new QCheckBox(tr("Lock layer"), layers);
    properties->addRow(locked);
    connect(m_layers, &QListWidget::currentRowChanged, this, [this, locked](int index) {
        if (m_refreshing)
            return;
        if (auto c = current()) {
            c->cancelInteraction();
            c->session()->activeLayer = int(c->session()->document().layers.size()) - 1 - index;
            c->session()->selectedObject = {};
            QSignalBlocker blocker(locked);
            locked->setChecked(
                c->session()->document().layers[size_t(c->session()->activeLayer)].locked);
            refresh();
        }
    });
    connect(m_layers, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
        if (m_refreshing)
            return;
        if (auto c = current()) {
            int i = int(c->session()->document().layers.size()) - 1 - m_layers->row(item);
            bool visible = item->checkState() == Qt::Checked;
            c->session()->apply(tr("Layer visibility"),
                                [=](Document& d) { d.layers[size_t(i)].visible = visible; });
        }
    });
    connect(m_layers, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        if (auto c = current()) {
            bool ok;
            QString name = QInputDialog::getText(
                this, tr("Rename layer"), tr("Name"), QLineEdit::Normal,
                c->session()->document().layers[size_t(c->session()->activeLayer)].name, &ok);
            if (ok)
                c->session()->setLayerProperty(tr("Rename layer"),
                                               [=](Layer& l) { l.name = name; });
        }
    });
    connect(m_layerOpacity, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (!m_refreshing)
            if (auto c = current())
                c->session()->setLayerProperty(tr("Layer opacity"),
                                               [=](Layer& l) { l.opacity = value; });
    });
    connect(m_blend, &QComboBox::currentIndexChanged, this, [this](int value) {
        if (!m_refreshing)
            if (auto c = current())
                c->session()->setLayerProperty(tr("Blend mode"),
                                               [=](Layer& l) { l.blend = Blend(value); });
    });
    connect(locked, &QCheckBox::toggled, this, [this](bool value) {
        if (!m_refreshing)
            if (auto c = current())
                c->session()->setLayerProperty(tr("Layer lock"),
                                               [=](Layer& l) { l.locked = value; });
    });
    QWidget* objects = dock(tr("Object properties"), "ObjectProperties");
    auto obj = new QFormLayout(objects);
    m_fill = new QPushButton(tr("Fill"), objects);
    m_stroke = new QPushButton(tr("Stroke"), objects);
    obj->addRow(m_fill, m_stroke);
    auto objectColor = [this](bool fill) {
        if (auto c = current())
            if (auto o = c->session()->object()) {
                QColor value =
                    QColorDialog::getColor(fill ? o->fill : o->stroke, this, tr("Object color"),
                                           QColorDialog::ShowAlphaChannel);
                if (value.isValid())
                    c->session()->editObject(tr("Object color"), [=](Object& object) {
                        if (fill)
                            object.fill = value;
                        else
                            object.stroke = value;
                    });
            }
    };
    connect(m_fill, &QPushButton::clicked, this, [objectColor] { objectColor(true); });
    connect(m_stroke, &QPushButton::clicked, this, [objectColor] { objectColor(false); });
    m_strokeWidth = new QDoubleSpinBox(objects);
    m_strokeWidth->setRange(0, 4096);
    obj->addRow(tr("Stroke width"), m_strokeWidth);
    m_objectOpacity = new QDoubleSpinBox(objects);
    m_objectOpacity->setRange(0, 1);
    m_objectOpacity->setSingleStep(.05);
    obj->addRow(tr("Object opacity"), m_objectOpacity);
    m_font = new QFontComboBox(objects);
    obj->addRow(tr("Font"), m_font);
    m_fontSize = new QDoubleSpinBox(objects);
    m_fontSize->setRange(1, 512);
    obj->addRow(tr("Font size (pt)"), m_fontSize);
    connect(m_strokeWidth, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (!m_refreshing)
            if (auto c = current())
                c->session()->editObject(tr("Stroke width"),
                                         [=](Object& o) { o.strokeWidth = value; });
    });
    connect(m_objectOpacity, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (!m_refreshing)
            if (auto c = current())
                c->session()->editObject(tr("Object opacity"),
                                         [=](Object& o) { o.opacity = value; });
    });
    connect(m_font, &QFontComboBox::currentFontChanged, this, [this](QFont font) {
        if (!m_refreshing)
            if (auto c = current())
                c->session()->editObject(tr("Font"),
                                         [=](Object& o) { o.font.setFamily(font.family()); });
    });
    connect(m_fontSize, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        if (!m_refreshing)
            if (auto c = current())
                c->session()->editObject(tr("Font size"),
                                         [=](Object& o) { o.font.setPointSizeF(value); });
    });
    auto fontStyle = new QPushButton(tr("Font style…"), objects);
    obj->addRow(fontStyle);
    connect(fontStyle, &QPushButton::clicked, this, [this] {
        if (auto c = current())
            if (auto o = c->session()->object()) {
                bool ok;
                QFont font = QFontDialog::getFont(&ok, o->font, this, tr("Text font"));
                if (ok)
                    c->session()->editObject(tr("Font style"),
                                             [=](Object& object) { object.font = font; });
            }
    });
    auto alignment = new QComboBox(objects);
    alignment->addItems({tr("Left"), tr("Center"), tr("Right")});
    obj->addRow(tr("Text alignment"), alignment);
    connect(alignment, &QComboBox::activated, this, [this](int index) {
        if (auto c = current())
            c->session()->editObject(tr("Text alignment"), [=](Object& o) {
                o.alignment = index == 0   ? Qt::AlignLeft
                              : index == 1 ? Qt::AlignHCenter
                                           : Qt::AlignRight;
            });
    });
    auto editText = new QPushButton(tr("Edit text"), objects);
    obj->addRow(editText);
    connect(editText, &QPushButton::clicked, this, [this] {
        if (auto c = current())
            c->editText();
    });
    auto closePath = new QPushButton(tr("Close path"), objects);
    obj->addRow(closePath);
    connect(closePath, &QPushButton::clicked, this, [this] {
        if (auto c = current())
            c->session()->editObject(tr("Close path"), [](Object& o) { o.path.closeSubpath(); });
    });
    auto order = new QHBoxLayout;
    for (bool front : {false, true}) {
        auto b = new QPushButton(front ? tr("Bring to front") : tr("Send to back"), objects);
        order->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, front] {
            if (auto c = current()) {
                QUuid id = c->session()->selectedObject;
                int layer = c->session()->activeLayer;
                c->session()->apply(tr("Object order"), [=](Document& d) {
                    auto& v = d.layers[size_t(layer)].objects;
                    auto it = std::find_if(v.begin(), v.end(),
                                           [&](const Object& o) { return o.id == id; });
                    if (it == v.end())
                        return;
                    Object o = *it;
                    v.erase(it);
                    if (front)
                        v.push_back(o);
                    else
                        v.insert(v.begin(), o);
                });
            }
        });
    }
    obj->addRow(order);
    m_theme = new QComboBox(objects);
    m_theme->addItems({tr("System theme"), tr("Light"), tr("Dark")});
    obj->addRow(tr("Appearance"), m_theme);
    connect(m_theme, &QComboBox::currentIndexChanged, this, &MainWindow::setTheme);
    auto toolDock = findChild<QDockWidget*>("ToolOptions");
    auto objectDock = findChild<QDockWidget*>("ObjectProperties");
    auto layersDock = findChild<QDockWidget*>("Layers");
    auto colorsDock = findChild<QDockWidget*>("Colors");
    tabifyDockWidget(toolDock, objectDock);
    tabifyDockWidget(layersDock, colorsDock);
    toolDock->raise();
    layersDock->raise();
}
void MainWindow::setTheme(int index) {
    QPalette palette = QApplication::style()->standardPalette();
    if (index == 2) {
        palette.setColor(QPalette::Window, QColor("#25272c"));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor("#191b20"));
        palette.setColor(QPalette::AlternateBase, QColor("#30333a"));
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor("#353841"));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::Highlight, QColor("#387de3"));
        palette.setColor(QPalette::HighlightedText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#81858f"));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#81858f"));
    }
    qApp->setPalette(palette);
    for (auto action : findChildren<QAction*>()) {
        QString path = action->property("iconResource").toString();
        if (path.isEmpty())
            continue;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        QByteArray svg = file.readAll();
        svg.replace("stroke=\"black\"",
                    ("stroke=\"" + palette.color(QPalette::ButtonText).name() + "\"").toUtf8());
        QSvgRenderer renderer(svg);
        QImage image(48, 48, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderer.render(&painter);
        painter.end();
        image.setDevicePixelRatio(2);
        action->setIcon(QPixmap::fromImage(image));
    }

    m_settings->setValue("theme", index);
    if (m_theme->currentIndex() != index) {
        QSignalBlocker block(m_theme);
        m_theme->setCurrentIndex(index);
    }
}
void MainWindow::refresh() {
    if (!m_layers)
        return;
    m_refreshing = true;
    auto c = current();
    Session* s = c ? c->session() : nullptr;
    m_undo->setEnabled(s && s->canUndo());
    m_redo->setEnabled(s && s->canRedo());
    m_save->setEnabled(s && !m_saveBusy.contains(s->sessionId));
    m_undo->setText(s && s->canUndo() ? tr("Undo %1").arg(s->undoLabel()) : tr("Undo"));
    m_redo->setText(s && s->canRedo() ? tr("Redo %1").arg(s->redoLabel()) : tr("Redo"));
    m_layers->clear();
    if (s) {
        s->activeLayer = std::clamp(s->activeLayer, 0, int(s->document().layers.size()) - 1);
        const auto& d = s->document();
        for (int i = int(d.layers.size()) - 1; i >= 0; --i) {
            const auto& l = d.layers[size_t(i)];
            auto item = new QListWidgetItem(
                (l.kind == LayerKind::Vector ? QStringLiteral("◇  ") : QStringLiteral("▦  ")) +
                    l.name + (l.locked ? QStringLiteral("  🔒") : QString()),
                m_layers);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(l.visible ? Qt::Checked : Qt::Unchecked);
        }
        m_layers->setCurrentRow(int(d.layers.size()) - 1 - s->activeLayer);
        const auto& l = d.layers[size_t(s->activeLayer)];
        m_layerOpacity->setValue(l.opacity);
        m_blend->setCurrentIndex(int(l.blend));
        m_locked->setChecked(l.locked);
        m_pressure->setChecked(s->brush.pressure);
        m_sampleVisible->setChecked(c->sampleVisible);
        m_size->setValue(s->brush.size);
        m_hardness->setValue(s->brush.hardness);
        m_opacity->setValue(s->brush.opacity);
        m_flow->setValue(s->brush.flow);
        m_spacing->setValue(s->brush.spacing);
        m_tolerance->setValue(c->fillTolerance);
        auto paintButton = [](QPushButton* b, QColor color) {
            b->setStyleSheet(
                QString("background-color:%1;color:%2;padding:5px;")
                    .arg(color.name(), color.lightness() > 130 ? "#151515" : "#ffffff"));
        };
        paintButton(m_foreground, s->foreground);
        paintButton(m_background, s->background);
        m_hex->setText(QString("%1%2%3%4")
                           .arg(s->foreground.red(), 2, 16, QChar('0'))
                           .arg(s->foreground.green(), 2, 16, QChar('0'))
                           .arg(s->foreground.blue(), 2, 16, QChar('0'))
                           .arg(s->foreground.alpha(), 2, 16, QChar('0'))
                           .toUpper());
        if (auto object = s->object()) {
            paintButton(m_fill, object->fill);
            paintButton(m_stroke, object->stroke);
            m_strokeWidth->setValue(object->strokeWidth);
            m_objectOpacity->setValue(object->opacity);
            m_font->setCurrentFont(object->font);
            m_fontSize->setValue(object->font.pointSizeF());
        }
        QString name = s->filePath.isEmpty() ? tr("Untitled") : QFileInfo(s->filePath).fileName();
        setWindowTitle(name + (s->dirty() ? " *" : "") + " — Zyri");
    } else
        setWindowTitle("Zyri");
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto canvas = qobject_cast<Canvas*>(m_tabs->widget(i));
        auto session = canvas->session();
        QString title =
            session->filePath.isEmpty() ? tr("Untitled") : QFileInfo(session->filePath).fileName();
        m_tabs->setTabText(i, title + (session->dirty() ? " *" : ""));
    }
    for (QWidget* widget : std::initializer_list<QWidget*>{m_fill, m_stroke, m_strokeWidth,
                                                           m_objectOpacity, m_font, m_fontSize})
        widget->setEnabled(s && s->object());
    m_refreshing = false;
}
Canvas* MainWindow::addDocument(Document document, QString path, bool saved) {
    auto session = new Session(std::move(document));
    session->historyLimit = std::clamp(m_settings->value("historyEntries", 100).toInt(), 1, 1000);
    session->historyBudget =
        qint64(std::clamp(m_settings->value("historyMemoryMiB", 256).toInt(), 16, 2048)) * 1024 *
        1024;
    session->spillBudget =
        qint64(std::clamp(m_settings->value("historyDiskMiB", 2048).toInt(), 64, 8192)) * 1024 *
        1024;
    session->filePath = std::move(path);
    if (!saved)
        session->markUnsaved();
    auto canvas = new Canvas(session);
    session->setParent(canvas);
    m_tabs->addTab(canvas, tr("Untitled"));
    m_tabs->setCurrentWidget(canvas);
    connect(session, &Session::historyChanged, this, &MainWindow::refresh);
    connect(session, &Session::changed, this, [this, canvas](QRect) {
        if (!canvas->session()->editing())
            refresh();
    });
    connect(session, &Session::errorOccurred, this,
            [this](QString message) { statusBar()->showMessage(message, 12000); });
    connect(canvas, &Canvas::positionChanged, this, [this, canvas](QPointF position, double zoom) {
        QSize s = canvas->session()->document().size;
        statusBar()->showMessage(QString("%1 × %2 px   |   %3%   |   X %4  Y %5")
                                     .arg(s.width())
                                     .arg(s.height())
                                     .arg(qRound(zoom * 100))
                                     .arg(qRound(position.x()))
                                     .arg(qRound(position.y())));
    });
    connect(canvas, &Canvas::cancelJobRequested, this, [this, session] {
        auto job = m_operations.value(session->sessionId);
        if (job)
            job->store(true);
    });
    connect(canvas, &Canvas::toolChanged, this, [this](Tool tool) {
        if (auto toolbar = findChild<QToolBar*>("Tools"))
            for (auto a : toolbar->actions())
                a->setChecked(a->data().toInt() == int(tool));
    });
    connect(canvas, &Canvas::fillRequested, this,
            [this, canvas](QPoint p, int tolerance, bool visible) {
                if (canvas != current())
                    return;
                runOperation(tr("Fill"), [=](Session& s) { s.fill(p, tolerance, visible); });
            });
    refresh();
    return canvas;
}
void MainWindow::newDocument() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("New document"));
    QFormLayout layout(&dialog);
    QSpinBox width, height, ppi;
    width.setRange(1, MaxDimension);
    height.setRange(1, MaxDimension);
    ppi.setRange(1, 2400);
    width.setValue(1920);
    height.setValue(1080);
    ppi.setValue(96);
    QComboBox background;
    background.addItems({tr("Transparent"), tr("White"), tr("Black")});
    layout.addRow(tr("Width (px)"), &width);
    layout.addRow(tr("Height (px)"), &height);
    layout.addRow(tr("PPI"), &ppi);
    layout.addRow(tr("Background"), &background);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    Document d;
    d.size = {width.value(), height.value()};
    d.ppi = ppi.value();
    if (!validSize(d.size)) {
        QMessageBox::warning(this, tr("New document"),
                             tr("Dimensions exceed the supported pixel count."));
        return;
    }
    Layer l;
    l.name = tr("Background");
    if (background.currentIndex()) {
        QImage image(d.size, QImage::Format_ARGB32_Premultiplied);
        image.fill(background.currentIndex() == 1 ? Qt::white : Qt::black);
        replaceRaster(l, image);
    }
    d.layers.push_back(l);
    addDocument(d);
}
void MainWindow::openPath(QString path) {
    int page = 0, ppi = 144;
#ifdef ZYRI_PDF
    if (QFileInfo(path).suffix().compare("pdf", Qt::CaseInsensitive) == 0) {
        bool ok;
        page = QInputDialog::getInt(this, tr("PDF import"), tr("Page number (raster import)"), 1, 1,
                                    100000, 1, &ok) -
               1;
        if (!ok)
            return;
        ppi = QInputDialog::getInt(this, tr("PDF import"), tr("Raster resolution (PPI)"), 144, 36,
                                   1200, 1, &ok);
        if (!ok)
            return;
    }
#endif
    statusBar()->showMessage(tr("Opening %1…").arg(QFileInfo(path).fileName()));
    auto watcher = new QFutureWatcher<LoadResult>(this);
    connect(watcher, &QFutureWatcher<LoadResult>::finished, this, [this, watcher, path] {
        auto result = watcher->result();
        watcher->deleteLater();
        if (!result) {
            QMessageBox::warning(this, tr("Open failed"), result.error);
            return;
        }
        if (!result.warning.isEmpty() &&
            QMessageBox::question(this, tr("Import as raster?"),
                                  result.warning + tr("\nContinue?")) != QMessageBox::Yes)
            return;
        bool native = QFileInfo(path).suffix().compare("zyri", Qt::CaseInsensitive) == 0;
        addDocument(std::move(result.document), native ? path : QString(), native);
        QStringList recent = m_settings->value("recent").toStringList();
        recent.removeAll(path);
        recent.prepend(path);
        while (recent.size() > 12)
            recent.removeLast();
        m_settings->setValue("recent", recent);
        statusBar()->showMessage(tr("Opened %1").arg(QFileInfo(path).fileName()), 4000);
    });
    watcher->setFuture(
        QtConcurrent::run([path, page, ppi] { return importImage(path, page, ppi); }));
}
void MainWindow::save(Canvas* canvas, bool as, bool closeAfter) {
    if (!canvas)
        return;
    canvas->cancelInteraction();
    Session* session = canvas->session();
    if (m_saveBusy.contains(session->sessionId))
        return;
    QString path = session->filePath;
    if (as || path.isEmpty()) {
        QScopedValueRollback<bool> dialogGuard(m_dialogActive, true);
        path = QFileDialog::getSaveFileName(this, tr("Save editable project"),
                                            path.isEmpty() ? QStringLiteral("Untitled.zyri") : path,
                                            tr("Zyri project (*.zyri)"));
    }
    if (path.isEmpty()) {
        m_closing = false;
        return;
    }
    if (QFileInfo(path).suffix().compare("zyri", Qt::CaseInsensitive) != 0)
        path += ".zyri";
    path = QFileInfo(path).absoluteFilePath();
    if (m_busy.contains(path)) {
        statusBar()->showMessage(tr("A write to this file is already in progress."), 5000);
        return;
    }
    Document snapshot = session->document();
    QUuid state = session->stateId();
    m_busy.insert(path);
    m_saveBusy.insert(session->sessionId);
    refresh();
    statusBar()->showMessage(tr("Saving…"));
    QPointer<Canvas> target = canvas;
    auto watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this,
            [this, watcher, target, state, path, closeAfter, id = session->sessionId] {
                QString error = watcher->result();
                watcher->deleteLater();
                m_busy.remove(path);
                m_saveBusy.remove(id);
                if (!error.isEmpty()) {
                    m_closing = false;
                    QScopedValueRollback<bool> dialogGuard(m_dialogActive, true);
                    QMessageBox::warning(this, tr("Save failed"), error);
                } else if (target) {
                    auto s = target->session();
                    s->filePath = path;
                    s->markSaved(state);
                    if (!s->dirty())
                        discardRecovery(s);
                    statusBar()->showMessage(tr("Saved %1").arg(QFileInfo(path).fileName()), 4000);
                    if (closeAfter && !s->dirty())
                        closeTab(m_tabs->indexOf(target));
                }
                refresh();
                if (m_closing && error.isEmpty())
                    close();
            });
    watcher->setFuture(QtConcurrent::run([snapshot, path] { return saveProject(snapshot, path); }));
}
void MainWindow::closeTab(int index) {
    if (m_dialogActive || index < 0 || index >= m_tabs->count())
        return;
    auto canvas = qobject_cast<Canvas*>(m_tabs->widget(index));
    canvas->cancelInteraction();
    if (canvas->session()->dirty()) {
        if (m_saveBusy.contains(canvas->session()->sessionId))
            return;
        int result;
        {
            QScopedValueRollback<bool> dialogGuard(m_dialogActive, true);
            result = QMessageBox::warning(
                this, tr("Unsaved changes"), tr("Save changes to this document?"),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        }
        if (result != QMessageBox::Save && result != QMessageBox::Discard) {
            m_closing = false;
            return;
        }
        if (result == QMessageBox::Save) {
            save(canvas, false, true);
            return;
        }
    }
    discardRecovery(canvas->session());
    m_tabs->removeTab(index);
    canvas->deleteLater();
    refresh();
}
void MainWindow::closeEvent(QCloseEvent* event) {
    // Ignore before entering any dialog's nested event loop. A native Quit or
    // completed worker may request another close while the decision is pending.
    event->ignore();
    if (m_closeInProgress || m_dialogActive)
        return;
    QScopedValueRollback<bool> closeGuard(m_closeInProgress, true);
    m_closing = true;
    for (auto token : m_operations)
        token->store(true);
    for (int i = m_tabs->count() - 1; i >= 0; --i) {
        auto canvas = qobject_cast<Canvas*>(m_tabs->widget(i));
        if (canvas->session()->dirty()) {
            int count = m_tabs->count();
            closeTab(i);
            if (m_tabs->count() == count) {
                event->ignore();
                return;
            }
        }
    }
    if (!m_busy.isEmpty() || !m_recoveryBusy.isEmpty() || !m_operations.isEmpty()) {
        statusBar()->showMessage(tr("Waiting for file writes to finish…"));
        event->ignore();
        return;
    }
    m_settings->setValue("geometry", saveGeometry());
    m_settings->setValue("layout", saveState());
    event->accept();
    if (!m_closeAccepted) {
        m_closeAccepted = true;
        emit closeAccepted();
    }
}
void MainWindow::runOperation(const QString& label, std::function<void(Session&)> operation) {
    auto canvas = current();
    if (!canvas)
        return;
    canvas->cancelInteraction();
    auto s = canvas->session();
    auto id = s->sessionId;
    if (m_operations.contains(id)) {
        statusBar()->showMessage(tr("An operation is running; Escape cancels it."), 4000);
        return;
    }
    Document snapshot = s->document();
    quint64 revision = s->revision();
    int active = s->activeLayer;
    QUuid selected = s->selectedObject;
    QPainterPath selection = s->selection;
    bool hasSelection = s->hasSelection;
    QColor foreground = s->foreground;
    QPointer<Session> target = s;
    auto cancellation = std::make_shared<std::atomic_bool>(false);
    m_operations[id] = cancellation;
    struct Result {
        Document document;
        QPainterPath selection;
        bool hasSelection;
        QString error;
    };
    statusBar()->showMessage(label + tr("… Escape cancels"));
    auto watcher = new QFutureWatcher<Result>(this);
    connect(watcher, &QFutureWatcher<Result>::finished, this,
            [this, watcher, target, revision, label, id, cancellation, active, selected] {
                auto result = watcher->result();
                watcher->deleteLater();
                m_operations.remove(id);
                QTimer::singleShot(0, this, [this] {
                    if (m_closing)
                        close();
                });
                if (!target || cancellation->load())
                    return;
                if (!result.error.isEmpty()) {
                    QMessageBox::warning(this, label, result.error);
                    return;
                }
                if (target->revision() != revision || target->editing() ||
                    target->activeLayer != active || target->selectedObject != selected) {
                    statusBar()->showMessage(
                        tr("The document changed; the old operation result was discarded."), 6000);
                    return;
                }
                target->apply(label, [&](Document& d) { d = std::move(result.document); });
                target->selection = result.selection;
                target->hasSelection = result.hasSelection;
                emit target->changed({});
                statusBar()->showMessage(label + tr(" complete"), 3000);
            });
    watcher->setFuture(QtConcurrent::run(
        [snapshot, active, selected, selection, hasSelection, foreground, operation, cancellation] {
            Session temporary(snapshot);
            temporary.activeLayer = active;
            temporary.selectedObject = selected;
            temporary.selection = selection;
            temporary.hasSelection = hasSelection;
            temporary.foreground = foreground;
            temporary.cancelRequested = [cancellation] { return cancellation->load(); };
            QString error;
            QObject::connect(
                &temporary, &Session::errorOccurred, &temporary,
                [&](QString text) { error = text; }, Qt::DirectConnection);
            if (!cancellation->load())
                operation(temporary);
            return Result{temporary.document(), temporary.selection, temporary.hasSelection, error};
        }));
}
void MainWindow::preferences() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Preferences"));
    dialog.resize(560, 600);
    QVBoxLayout layout(&dialog);
    QFormLayout form;
    layout.addLayout(&form);
    QSpinBox entries, memory, disk, autosave;
    entries.setRange(1, 1000);
    entries.setValue(m_settings->value("historyEntries", 100).toInt());
    memory.setRange(16, 2048);
    memory.setValue(m_settings->value("historyMemoryMiB", 256).toInt());
    disk.setRange(64, 8192);
    disk.setValue(m_settings->value("historyDiskMiB", 2048).toInt());
    autosave.setRange(0, 300);
    autosave.setValue(m_settings->value("autosaveSeconds", 30).toInt());
    form.addRow(tr("Undo entry limit"), &entries);
    form.addRow(tr("Resident history (MiB)"), &memory);
    form.addRow(tr("History disk budget (MiB)"), &disk);
    form.addRow(tr("Recovery interval (seconds, 0 disables)"), &autosave);
    layout.addWidget(new QLabel(tr("Menu shortcuts")));
    QTableWidget shortcuts;
    shortcuts.setColumnCount(2);
    shortcuts.setHorizontalHeaderLabels({tr("Command"), tr("Shortcut")});
    QList<QAction*> commands;
    for (auto a : findChildren<QAction*>())
        if (!a->objectName().isEmpty())
            commands << a;
    shortcuts.setRowCount(commands.size());
    for (int i = 0; i < commands.size(); ++i) {
        auto item = new QTableWidgetItem(commands[i]->text().remove('&'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        shortcuts.setItem(i, 0, item);
        shortcuts.setCellWidget(i, 1, new QKeySequenceEdit(commands[i]->shortcut()));
    }
    shortcuts.resizeColumnsToContents();
    layout.addWidget(&shortcuts);
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addWidget(&buttons);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        QSet<QString> assigned;
        for (int i = 0; i < commands.size(); ++i) {
            auto editor = qobject_cast<QKeySequenceEdit*>(shortcuts.cellWidget(i, 1));
            QString sequence = editor->keySequence().toString(QKeySequence::PortableText);
            if (!sequence.isEmpty() && assigned.contains(sequence)) {
                QMessageBox::warning(
                    &dialog, tr("Shortcut conflict"),
                    tr("Two commands use %1. Choose distinct shortcuts.").arg(sequence));
                return;
            }
            if (!sequence.isEmpty())
                assigned.insert(sequence);
        }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted)
        return;
    for (int i = 0; i < commands.size(); ++i) {
        auto sequence = qobject_cast<QKeySequenceEdit*>(shortcuts.cellWidget(i, 1))->keySequence();
        commands[i]->setShortcut(sequence);
        m_settings->setValue("shortcuts/" + commands[i]->objectName(),
                             sequence.toString(QKeySequence::PortableText));
    }
    m_settings->setValue("historyEntries", entries.value());
    m_settings->setValue("historyMemoryMiB", memory.value());
    m_settings->setValue("historyDiskMiB", disk.value());
    m_settings->setValue("autosaveSeconds", autosave.value());
    if (autosave.value() > 0)
        m_autosave->start(autosave.value() * 1000);
    else
        m_autosave->stop();
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto session = qobject_cast<Canvas*>(m_tabs->widget(i))->session();
        session->historyLimit = entries.value();
        session->historyBudget = qint64(memory.value()) * 1024 * 1024;
        session->spillBudget = qint64(disk.value()) * 1024 * 1024;
    }
}
void MainWindow::exportCurrent() {
    auto canvas = current();
    if (!canvas)
        return;
    canvas->cancelInteraction();
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Export"));
    QFormLayout layout(&dialog);
    QDoubleSpinBox scale;
    scale.setRange(.1, 8);
    scale.setValue(1);
    QSpinBox quality;
    quality.setRange(1, 100);
    quality.setValue(92);
    QPushButton matte(tr("White matte (JPEG)"));
    QColor matteColor = Qt::white;
    connect(&matte, &QPushButton::clicked, &dialog, [&] {
        QColor value = QColorDialog::getColor(matteColor, &dialog, tr("JPEG background"));
        if (value.isValid()) {
            matteColor = value;
            matte.setText(value.name());
        }
    });
    layout.addRow(tr("Scale (PNG/JPEG/PDF raster)"), &scale);
    layout.addRow(tr("JPEG quality"), &quality);
    layout.addRow(&matte);
    layout.addRow(new QLabel(tr("PDF/SVG retain simple vectors; blend modes flatten the scene.")));
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout.addRow(&buttons);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    QString path =
        QFileDialog::getSaveFileName(this, tr("Export"), "Untitled.png",
                                     tr("PNG (*.png);;JPEG (*.jpg);;SVG (*.svg);;PDF (*.pdf)"));
    if (path.isEmpty())
        return;
    path = QFileInfo(path).absoluteFilePath();
    if (m_busy.contains(path))
        return;
    Document snapshot = canvas->session()->document();
    m_busy.insert(path);
    auto watcher = new QFutureWatcher<QString>(this);
    double factor = scale.value();
    int q = quality.value();
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, path] {
        QString error = watcher->result();
        watcher->deleteLater();
        m_busy.remove(path);
        if (!error.isEmpty())
            QMessageBox::warning(this, tr("Export failed"), error);
        else
            statusBar()->showMessage(tr("Exported %1").arg(QFileInfo(path).fileName()), 5000);
        if (m_closing)
            close();
    });
    watcher->setFuture(QtConcurrent::run([snapshot, path, factor, matteColor, q] {
        return exportImage(snapshot, path, factor, matteColor, q);
    }));
}
void MainWindow::autosave() {
    if (!m_lock->isLocked())
        return;
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto canvas = qobject_cast<Canvas*>(m_tabs->widget(i));
        auto s = canvas->session();
        QUuid id = s->sessionId, state = s->stateId();
        if (!s->dirty() || s->editing() || m_recoveryBusy.contains(id) ||
            m_recoveredStates.value(id) == state)
            continue;
        Document snapshot = s->document();
        QString prefix = id.toString(QUuid::WithoutBraces);
        QString path = m_instance + "/" + prefix + "_" +
                       QString::number(QDateTime::currentMSecsSinceEpoch()) + ".zyri";
        m_recoveryBusy.insert(id);
        QPointer<Session> target = s;
        auto watcher = new QFutureWatcher<QString>(this);
        connect(watcher, &QFutureWatcher<QString>::finished, this,
                [this, watcher, target, path, prefix, state, id] {
                    QString error = watcher->result();
                    watcher->deleteLater();
                    m_recoveryBusy.remove(id);
                    QTimer::singleShot(0, this, [this] {
                        if (m_closing)
                            close();
                    });
                    if (!target || !target->dirty() ||
                        m_tabs->indexOf(qobject_cast<Canvas*>(target->parent())) < 0) {
                        QFile::remove(path);
                        return;
                    }
                    if (!error.isEmpty()) {
                        statusBar()->showMessage(tr("Recovery save failed: %1").arg(error), 15000);
                        return;
                    }
                    m_recoveredStates[id] = state;
                    QDir dir(m_instance);
                    QStringList files = dir.entryList({prefix + "_*.zyri"}, QDir::Files,
                                                      QDir::Name | QDir::Reversed);
                    for (int n = 2; n < files.size(); ++n)
                        dir.remove(files[n]);
                });
        watcher->setFuture(
            QtConcurrent::run([snapshot, path] { return saveRecoveryGeneration(snapshot, path); }));
    }
}
void MainWindow::discardRecovery(Session* session) {
    QUuid id = session->sessionId;
    QString prefix = id.toString(QUuid::WithoutBraces);
    QDir dir(m_instance);
    for (auto file : dir.entryList({prefix + "_*.zyri"}, QDir::Files))
        dir.remove(file);
    for (const auto& path : m_recoverySources.take(id))
        QFile::remove(path);
    m_recoveredStates.remove(id);
}
void MainWindow::recover() {
    QDir root(m_storage + "/recovery");
    for (const auto& name : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString directory = root.filePath(name);
        if (directory == m_instance)
            continue;
        QLockFile lock(directory + "/instance.lock");
        lock.setStaleLockTime(0);
        if (!lock.tryLock())
            continue;
        QDir dir(directory);
        QMap<QString, QStringList> groups;
        for (auto file : dir.entryList({"*.zyri"}, QDir::Files, QDir::Name | QDir::Reversed))
            groups[file.section('_', 0, 0)] << dir.filePath(file);
        for (const auto& paths : groups) {
            LoadResult result;
            QString chosen;
            for (const auto& path : paths) {
                result = loadProject(path);
                if (result) {
                    chosen = path;
                    break;
                }
            }
            if (chosen.isEmpty())
                continue;
            auto answer = QMessageBox::question(
                this, tr("Recover document"),
                tr("A locally recovered document from %1 is available.\nYes: recover as unsaved "
                   "work.\nNo: discard these recovery files.\nCancel: keep them for later.")
                    .arg(QFileInfo(chosen).lastModified().toString()),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (answer == QMessageBox::Yes) {
                auto restored = addDocument(std::move(result.document));
                m_recoverySources[restored->session()->sessionId] = paths;
            } else if (answer == QMessageBox::No)
                for (const auto& path : paths)
                    QFile::remove(path);
        }
    }
}
void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}
void MainWindow::dropEvent(QDropEvent* e) {
    for (const auto& url : e->mimeData()->urls())
        if (url.isLocalFile())
            openPath(url.toLocalFile());
    e->acceptProposedAction();
}
} // namespace Zyri
