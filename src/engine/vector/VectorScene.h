#pragma once
#include <QPainter>
#include <memory>
#include <vector>

namespace Zyri {
class VectorObject;

class VectorScene {
public:
    VectorScene() = default;
    ~VectorScene();

    void addObject(std::unique_ptr<VectorObject> obj);
    void removeObject(int index);

    [[nodiscard]] int           count()        const { return static_cast<int>(m_objects.size()); }
    [[nodiscard]] VectorObject* objectAt(int i)      { return m_objects[i].get(); }

    // Paint all objects onto the given painter (used by PDFExporter for vector passthrough).
    void paintTo(QPainter& painter) const;

private:
    // std::vector works with incomplete types as long as destructor is out-of-line.
    std::vector<std::unique_ptr<VectorObject>> m_objects;
};
} // namespace Zyri
