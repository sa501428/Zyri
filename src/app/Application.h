#pragma once
#include <QObject>
namespace Zyri {
class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject* p = nullptr) : QObject(p) {}
};
} // namespace Zyri
