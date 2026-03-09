#include "ColorProfile.h"
#include <lcms2.h>

namespace Zyri {

ColorProfile::ColorProfile() = default;

ColorProfile::~ColorProfile()
{
    if (m_transform) {
        cmsDeleteTransform(m_transform);
        m_transform = nullptr;
    }
}

ColorProfile::ColorProfile(ColorProfile&& o) noexcept
    : m_transform(o.m_transform), m_iccData(std::move(o.m_iccData))
{
    o.m_transform = nullptr;
}

ColorProfile& ColorProfile::operator=(ColorProfile&& o) noexcept
{
    if (this != &o) {
        if (m_transform) cmsDeleteTransform(m_transform);
        m_transform   = o.m_transform;
        m_iccData     = std::move(o.m_iccData);
        o.m_transform = nullptr;
    }
    return *this;
}

ColorProfile ColorProfile::sRGB()
{
    ColorProfile p;
    cmsHPROFILE src = cmsCreate_sRGBProfile();
    cmsHPROFILE dst = cmsCreate_sRGBProfile(); // identity for sRGB working space
    p.m_transform = cmsCreateTransform(src, TYPE_BGRA_8, dst, TYPE_BGRA_8,
                                        INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(src);
    cmsCloseProfile(dst);
    return p;
}

ColorProfile ColorProfile::fromICC(const QByteArray& iccData)
{
    ColorProfile p;
    p.m_iccData = iccData;

    cmsHPROFILE src = cmsOpenProfileFromMem(iccData.constData(),
                                             static_cast<cmsUInt32Number>(iccData.size()));
    if (!src) return p;

    cmsHPROFILE dst = cmsCreate_sRGBProfile();
    p.m_transform = cmsCreateTransform(src, TYPE_BGRA_8, dst, TYPE_BGRA_8,
                                        INTENT_PERCEPTUAL, 0);
    cmsCloseProfile(src);
    cmsCloseProfile(dst);
    return p;
}

void ColorProfile::applyToScanline(uint32_t* pixels, int count) const
{
    if (!m_transform || !pixels || count <= 0) return;
    cmsDoTransform(m_transform, pixels, pixels, static_cast<cmsUInt32Number>(count));
}

} // namespace Zyri
