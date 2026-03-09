#include "MacOSAppearance.h"

#ifdef Q_OS_MACOS
#import <AppKit/AppKit.h>
#endif

namespace Zyri {

MacOSAppearance::MacOSAppearance(QObject* parent) : QObject(parent)
{
#ifdef Q_OS_MACOS
    // macOS posts AppleInterfaceThemeChangedNotification to the distributed
    // notification center when the user switches dark/light mode.
    MacOSAppearance* cppSelf = this;
    id observer = [[NSDistributedNotificationCenter defaultCenter]
        addObserverForName:@"AppleInterfaceThemeChangedNotification"
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification*) {
        emit cppSelf->appearanceChanged(currentAppearance());
    }];
    // Retain the observer token using plain __bridge cast (no ARC needed).
    m_observer = (void*)CFBridgingRetain(observer);
#endif
}

MacOSAppearance::~MacOSAppearance()
{
#ifdef Q_OS_MACOS
    if (m_observer) {
        id observer = (id)CFBridgingRelease(m_observer);
        [[NSDistributedNotificationCenter defaultCenter] removeObserver:observer];
        m_observer = nullptr;
    }
#endif
}

MacOSAppearance::Appearance MacOSAppearance::currentAppearance()
{
#ifdef Q_OS_MACOS
    NSAppearanceName name = [[NSApp effectiveAppearance]
        bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua,
                                             NSAppearanceNameDarkAqua]];
    if ([name isEqualToString:NSAppearanceNameDarkAqua])
        return Appearance::Dark;
#endif
    return Appearance::Light;
}

} // namespace Zyri
