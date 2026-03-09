#pragma once
#include "../CommandBase.h"
#include "../../core/BlendMode.h"
#include <QUuid>
#include <QString>
#include <variant>

namespace Zyri {
class DocumentController;

// Handles opacity, blend mode, visibility, lock, and name changes.
class LayerPropertyCommand : public CommandBase {
public:
    using Value = std::variant<float, BlendMode, bool, QString>;
    enum class Property { Opacity, BlendMode, Visible, Locked, Name };

    LayerPropertyCommand(DocumentController* ctrl, const QUuid& layerId,
                          Property prop, Value oldVal, Value newVal);
    void execute() override;
    void undo()    override;
    [[nodiscard]] QString description() const override;

private:
    void apply(const Value& v);
    DocumentController* m_ctrl;
    QUuid               m_layerId;
    Property            m_prop;
    Value               m_old, m_new;
};
} // namespace Zyri
