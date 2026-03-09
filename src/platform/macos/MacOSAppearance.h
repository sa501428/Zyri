#pragma once
#include <QObject>

namespace Zyri {

// Detects the current macOS system appearance (dark/light) and emits a signal
// when it changes at runtime (e.g. user switches in System Settings).
class MacOSAppearance : public QObject {
    Q_OBJECT
public:
    enum class Appearance { Light, Dark };

    explicit MacOSAppearance(QObject* parent = nullptr);
    ~MacOSAppearance() override;

    [[nodiscard]] static Appearance currentAppearance();

signals:
    void appearanceChanged(Appearance appearance);

private:
    void* m_observer = nullptr; // NSObject* observer token (opaque)
};

} // namespace Zyri
