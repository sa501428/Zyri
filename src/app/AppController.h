#pragma once
#include <QObject>
namespace Zyri {
class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* p = nullptr) : QObject(p) {}
};
} // namespace Zyri
