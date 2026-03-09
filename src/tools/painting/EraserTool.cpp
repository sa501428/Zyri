#include "EraserTool.h"
#include "../../core/DocumentController.h"
#include "../../core/Document.h"

namespace Zyri {

void EraserTool::pointerDown(const QPointF& pos, qreal pressure)
{
    if (!m_controller || !m_engine) return;
    auto* doc = m_controller->document();
    if (!doc || doc->layers()->count() == 0) return;
    auto* layer = doc->layers()->layerAt(0);
    m_engine->beginStroke(layer->id(), Qt::black /* color unused for erase */,
                           preset(), /*eraser=*/true);
    m_engine->addPoint(pos, static_cast<float>(pressure));
}

} // namespace Zyri
