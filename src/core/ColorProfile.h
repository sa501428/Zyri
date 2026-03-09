#pragma once

#include <QByteArray>
#include <memory>

namespace Zyri {

// Thin wrapper around an lcms2 color transform.
// Used for converting between the document's working color profile and sRGB
// on PNG/JPEG import and export.
class ColorProfile
{
public:
    static ColorProfile sRGB();
    static ColorProfile fromICC(const QByteArray& iccData);

    ColorProfile();
    ~ColorProfile();

    ColorProfile(ColorProfile&&) noexcept;
    ColorProfile& operator=(ColorProfile&&) noexcept;

    ColorProfile(const ColorProfile&) = delete;
    ColorProfile& operator=(const ColorProfile&) = delete;

    [[nodiscard]] bool        isValid()  const { return m_transform != nullptr; }
    [[nodiscard]] QByteArray  iccData()  const { return m_iccData; }

    // Apply the stored transform to a premultiplied ARGB32 scanline (in-place).
    void applyToScanline(uint32_t* pixels, int count) const;

private:
    // Stored as void* to match lcms2's own cmsHTRANSFORM typedef without
    // pulling the lcms2 header into every translation unit that includes this file.
    void*      m_transform = nullptr;
    QByteArray m_iccData;
};

} // namespace Zyri
