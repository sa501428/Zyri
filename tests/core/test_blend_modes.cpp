#include <QtTest>
#include "BlendMode.h"

using namespace Zyri;

class TestBlendModes : public QObject
{
    Q_OBJECT
private slots:
    void normalBlendOpaqueOnTransparent()
    {
        // Compositing opaque red over transparent should yield red.
        const uint32_t src = 0xFF'FF'00'00; // opaque red ARGB
        const uint32_t dst = 0x00'00'00'00; // transparent
        const uint32_t result = compositePixel(src, dst, BlendMode::Normal);
        QCOMPARE((result >> 24) & 0xFF, uint32_t(0xFF)); // alpha=255
        QCOMPARE((result >> 16) & 0xFF, uint32_t(0xFF)); // red
        QCOMPARE((result >>  8) & 0xFF, uint32_t(0x00)); // green
        QCOMPARE((result      ) & 0xFF, uint32_t(0x00)); // blue
    }

    void normalBlendTransparentSrcLeavesDestUnchanged()
    {
        const uint32_t src    = 0x00'FF'00'00; // transparent red (alpha=0)
        const uint32_t dst    = 0xFF'00'FF'00; // opaque green
        const uint32_t result = compositePixel(src, dst, BlendMode::Normal);
        QCOMPARE(result, dst);
    }

    void multiplyBlendWhiteIsIdentity()
    {
        // White src (255,255,255) multiplied over any dst should yield dst.
        const uint32_t src    = 0xFF'FF'FF'FF;
        const uint32_t dst    = 0xFF'7F'40'20;
        const uint32_t result = compositePixel(src, dst, BlendMode::Multiply);
        // Allow ±1 rounding difference.
        const auto dr = int((dst    >> 16) & 0xFF);
        const auto rr = int((result >> 16) & 0xFF);
        QVERIFY(std::abs(dr - rr) <= 1);
    }

    void screenBlendBlackIsIdentity()
    {
        // Black src (0,0,0) screened over any dst should yield dst.
        const uint32_t src    = 0xFF'00'00'00;
        const uint32_t dst    = 0xFF'80'40'20;
        const uint32_t result = compositePixel(src, dst, BlendMode::Screen);
        const auto dg = int((dst    >>  8) & 0xFF);
        const auto rg = int((result >>  8) & 0xFF);
        QVERIFY(std::abs(dg - rg) <= 1);
    }
};

QTEST_MAIN(TestBlendModes)
#include "test_blend_modes.moc"
