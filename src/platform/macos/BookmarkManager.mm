#include "BookmarkManager.h"

#ifdef Q_OS_MACOS
#import <Foundation/Foundation.h>
#endif

namespace Zyri {

QByteArray BookmarkManager::createBookmark(const QString& filePath)
{
#ifdef Q_OS_MACOS
    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:filePath.toNSString()];
        NSError* error = nil;
        NSData* data = [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
                    includingResourceValuesForKeys:nil
                                    relativeToURL:nil
                                            error:&error];
        if (error || !data) return {};
        return QByteArray(static_cast<const char*>(data.bytes),
                          static_cast<int>(data.length));
    }
#else
    Q_UNUSED(filePath)
    return {};
#endif
}

QString BookmarkManager::resolveBookmark(const QByteArray& bookmark, bool* stale)
{
#ifdef Q_OS_MACOS
    @autoreleasepool {
        if (stale) *stale = false;
        NSData* data = [NSData dataWithBytes:bookmark.constData()
                                      length:static_cast<NSUInteger>(bookmark.size())];
        BOOL isStale = NO;
        NSError* error = nil;
        NSURL* url = [NSURL URLByResolvingBookmarkData:data
                                               options:NSURLBookmarkResolutionWithSecurityScope
                                         relativeToURL:nil
                                   bookmarkDataIsStale:&isStale
                                                 error:&error];
        if (error || !url) return {};
        if (stale) *stale = (bool)isStale;
        return QString::fromNSString(url.path);
    }
#else
    Q_UNUSED(bookmark)
    if (stale) *stale = false;
    return {};
#endif
}

bool BookmarkManager::startAccessing(const QString& filePath)
{
#ifdef Q_OS_MACOS
    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:filePath.toNSString()];
        return [url startAccessingSecurityScopedResource];
    }
#else
    Q_UNUSED(filePath)
    return true;
#endif
}

void BookmarkManager::stopAccessing(const QString& filePath)
{
#ifdef Q_OS_MACOS
    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:filePath.toNSString()];
        [url stopAccessingSecurityScopedResource];
    }
#else
    Q_UNUSED(filePath)
#endif
}

} // namespace Zyri
