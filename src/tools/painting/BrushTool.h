#pragma once
#include "../ToolBase.h"
#include "../../engine/raster/PaintEngine.h"
#include "../../engine/raster/BrushPreset.h"

namespace Zyri {
class BrushTool : public ToolBase {
    Q_OBJECT
public:
    explicit BrushTool(QObject* parent = nullptr);
    [[nodiscard]] QString name()   const override { return QStringLiteral("Brush"); }
    [[nodiscard]] QCursor cursor() const override { return Qt::CrossCursor; }
    void activate(DocumentController* c) override;
    void pointerDown(const QPointF& pos, qreal pressure) override;
    void pointerMove(const QPointF& pos, qreal pressure) override;
    void pointerUp(const QPointF& pos) override;
    BrushPreset& preset() { return m_preset; }
protected:
    BrushPreset  m_preset;
    PaintEngine* m_engine = nullptr;
};
} // namespace Zyri
