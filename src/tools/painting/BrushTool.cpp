#include "BrushTool.h"
#include "../../core/DocumentController.h"
#include "../../core/Document.h"

namespace Zyri {

BrushTool::BrushTool(QObject* parent) : ToolBase(parent) {}

void BrushTool::activate(DocumentController* c)
{
    ToolBase::activate(c);
    // PaintEngine is owned by DocumentController (wired in Phase 3).
}

void BrushTool::pointerDown(const QPointF& pos, qreal pressure)
{
    if (!m_controller || !m_engine) return;
    auto* doc   = m_controller->document();
    if (!doc || doc->layers()->count() == 0) return;
    auto* layer = doc->layers()->layerAt(0); // TODO: active layer selection
    m_engine->beginStroke(layer->id(), m_controller->foregroundColor(), m_preset, false);
    m_engine->addPoint(pos, static_cast<float>(pressure));
}

void BrushTool::pointerMove(const QPointF& pos, qreal pressure)
{
    if (!m_engine || !m_engine->isActive()) return;
    m_engine->addPoint(pos, static_cast<float>(pressure));
}

void BrushTool::pointerUp(const QPointF& /*pos*/)
{
    if (!m_engine) return;
    m_engine->endStroke();
}

} // namespace Zyri
