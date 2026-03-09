#pragma once
#include <QObject>
namespace Zyri {
class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject* p = nullptr) : QObject(p) {}
};
} // namespace Zyri
