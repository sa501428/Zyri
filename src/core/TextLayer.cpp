#include "TextLayer.h"

namespace Zyri {

TextLayer::TextLayer(const QString& name)
    : Layer(LayerType::Text, name.isEmpty() ? QStringLiteral("Text Layer") : name)
{
}

} // namespace Zyri
