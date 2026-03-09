#pragma once
#include <QByteArray>
#include <QString>

namespace Zyri {

// Creates and resolves macOS security-scoped bookmarks so that the app can
// reopen user-selected files after a restart under App Sandbox restrictions.
//
// Usage:
//   1. After a successful QFileDialog::getOpenFileName() call:
//        QByteArray bm = BookmarkManager::createBookmark(filePath);
//        PreferencesStore::instance().storeBookmark(filePath, bm);
//
//   2. Before reopening a recent file:
//        QString resolved = BookmarkManager::resolveBookmark(bm, &stale);
//        if (stale) { /* re-request access */ }
class BookmarkManager {
public:
    // Create a security-scoped bookmark for the given path.
    // Returns an empty QByteArray on failure or on non-macOS builds.
    static QByteArray createBookmark(const QString& filePath);

    // Resolve a previously created bookmark back to a file path.
    // Sets *stale=true if the bookmark needs to be recreated.
    static QString resolveBookmark(const QByteArray& bookmark, bool* stale = nullptr);

    // Start accessing a security-scoped URL. Must be paired with stopAccessing().
    static bool startAccessing(const QString& filePath);
    static void stopAccessing(const QString& filePath);
};

} // namespace Zyri
