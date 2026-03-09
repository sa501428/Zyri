#pragma once
#include "ToolBase.h"
#include <QObject>
#include <memory>
#include <unordered_map>

namespace Zyri {

enum class ToolId {
    Pencil, Brush, Eraser,
    FillBucket, Eyedropper,
    Line, Rect, Ellipse, RoundedRect, Polygon,
    Text,
    Move, Crop, Transform,
    RectSelect, EllipseSelect, Lasso, MagicWand,
};

class ToolRegistry : public QObject {
    Q_OBJECT
public:
    explicit ToolRegistry(QObject* parent = nullptr);

    void registerTool(ToolId id, std::unique_ptr<ToolBase> tool);

    [[nodiscard]] ToolBase* tool(ToolId id) const;
    [[nodiscard]] ToolBase* activeTool() const { return m_activeTool; }
    [[nodiscard]] ToolId    activeToolId() const { return m_activeId; }

    void setActiveTool(ToolId id, DocumentController* controller);

signals:
    void activeToolChanged(ToolId id);

private:
    std::unordered_map<int, std::unique_ptr<ToolBase>> m_tools;
    ToolBase* m_activeTool = nullptr;
    ToolId    m_activeId   = ToolId::Brush;
};
} // namespace Zyri
