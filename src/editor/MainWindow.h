#pragma once
#include "Canvas.h"
#include <QMainWindow>
#include <QPointer>
#include <QSet>
#include <atomic>
#include <memory>
class QTabWidget;
class QListWidget;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QPushButton;
class QLineEdit;
class QFontComboBox;
class QTimer;
class QLockFile;
class QSettings;
class QCheckBox;
namespace Zyri {
class MainWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(QString storageRoot = {}, bool recovery = true);
    ~MainWindow() override;
    Canvas* current() const;
    Canvas* addDocument(Document document, QString path = {}, bool saved = false);
    void openPath(QString path);
    void recover();

  signals:
    void closeAccepted();

  protected:
    void closeEvent(QCloseEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;

  private:
    QTabWidget* m_tabs;
    QListWidget* m_layers = nullptr;
    QDoubleSpinBox *m_size, *m_hardness, *m_opacity, *m_flow, *m_spacing, *m_layerOpacity,
        *m_strokeWidth, *m_objectOpacity, *m_fontSize;
    QSpinBox* m_tolerance;
    QComboBox *m_blend, *m_theme;
    QFontComboBox* m_font;
    QPushButton *m_foreground, *m_background, *m_fill, *m_stroke;
    QLineEdit* m_hex;
    QCheckBox *m_locked, *m_pressure, *m_sampleVisible;
    QAction *m_undo, *m_redo, *m_save;
    QTimer* m_autosave;
    bool m_refreshing = false, m_closing = false;
    bool m_closeInProgress = false, m_dialogActive = false, m_closeAccepted = false;
    QString m_storage, m_instance;
    std::unique_ptr<QLockFile> m_lock;
    std::unique_ptr<QSettings> m_settings;
    QSet<QString> m_busy;
    QSet<QUuid> m_saveBusy;
    QMap<QUuid, std::shared_ptr<std::atomic_bool>> m_operations;
    QSet<QUuid> m_recoveryBusy;
    QMap<QUuid, QUuid> m_recoveredStates;
    QMap<QUuid, QStringList> m_recoverySources;
    void buildMenus();
    void buildPanels();
    void refresh();
    void newDocument();
    void save(Canvas* canvas, bool as = false, bool closeAfter = false);
    void closeTab(int index);
    void exportCurrent();
    void preferences();
    void autosave();
    void discardRecovery(Session* session);
    void runOperation(const QString& label, std::function<void(Session&)> operation);
    void setTheme(int index);
    QAction* action(QMenu* menu, QString text, QKeySequence shortcut,
                    std::function<void()> callback);
};
} // namespace Zyri
