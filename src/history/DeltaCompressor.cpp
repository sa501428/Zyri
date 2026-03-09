#include "DeltaCompressor.h"
#include <QByteArray>
#include <zlib.h>

namespace Zyri {

static QByteArray zlibDeflate(const QByteArray& data)
{
    uLongf bound = compressBound(static_cast<uLong>(data.size()));
    QByteArray out(static_cast<int>(bound), '\0');
    uLongf outSize = bound;
    int ret = compress2(reinterpret_cast<Bytef*>(out.data()),
                        &outSize,
                        reinterpret_cast<const Bytef*>(data.constData()),
                        static_cast<uLong>(data.size()),
                        Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) return {};
    out.resize(static_cast<int>(outSize));
    return out;
}

static QByteArray zlibInflate(const QByteArray& data, int expectedSize)
{
    QByteArray out(expectedSize, '\0');
    uLongf outSize = static_cast<uLongf>(expectedSize);
    int ret = uncompress(reinterpret_cast<Bytef*>(out.data()),
                         &outSize,
                         reinterpret_cast<const Bytef*>(data.constData()),
                         static_cast<uLong>(data.size()));
    if (ret != Z_OK) return {};
    out.resize(static_cast<int>(outSize));
    return out;
}

QByteArray DeltaCompressor::compress(const QByteArray& before, const QByteArray& after)
{
    Q_ASSERT(before.size() == after.size());
    const int n = before.size();

    // XOR the two buffers — mostly zeros for sparse edits.
    QByteArray xorBuf(n, '\0');
    const auto* b = reinterpret_cast<const uint8_t*>(before.constData());
    const auto* a = reinterpret_cast<const uint8_t*>(after.constData());
    auto*       x = reinterpret_cast<uint8_t*>(xorBuf.data());
    for (int i = 0; i < n; ++i)
        x[i] = b[i] ^ a[i];

    return zlibDeflate(xorBuf);
}

QByteArray DeltaCompressor::apply(const QByteArray& before, const QByteArray& delta)
{
    const int n = before.size();
    QByteArray xorBuf = zlibInflate(delta, n);
    if (xorBuf.isEmpty()) return before; // inflate failed — safe fallback

    QByteArray after(n, '\0');
    const auto* b = reinterpret_cast<const uint8_t*>(before.constData());
    const auto* x = reinterpret_cast<const uint8_t*>(xorBuf.constData());
    auto*       a = reinterpret_cast<uint8_t*>(after.data());
    for (int i = 0; i < n; ++i)
        a[i] = b[i] ^ x[i];

    return after;
}

} // namespace Zyri
