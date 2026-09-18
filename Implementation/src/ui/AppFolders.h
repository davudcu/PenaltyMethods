#pragma once
#include <mu/mu.h>
#include <mu/IAppSettings.h>
#include <fo/FileOperations.h>
#include <syst/Shell.h>
#include <td/Date.h>
#include <td/Time.h>
#include <td/String.h>
#include <cctype>
#include <cstdio>
#include <string>
#include <system_error>

// ============================================================
// AppFolders: where exports are written.
//
//   <Documents>/Penalty Methods/Exports/<date_time>_<problem>/
//
// <Documents> is <home>/Documents when it exists, otherwise the home
// folder.  The home folder comes from natID (mu::IAppSettings), paths
// use natID's fo::fs (std::filesystem) and folders are opened with
// natID's syst::Shell, so the code is the same on Windows, macOS and
// Linux.  No file dialogs are used: the natID file dialogs corrupt the
// heap on Windows (SDK issue).
// ============================================================
namespace ui::appfs
{

constexpr const char* kAppFolder = "Penalty Methods";

inline fo::fs::path toPath(const char* utf8)
{
    return fo::fs::path(std::u8string(reinterpret_cast<const char8_t*>(utf8)));
}

inline std::string toUtf8(const fo::fs::path& p)
{
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

inline fo::fs::path exportsFolder()
{
    std::error_code ec;
    const fo::fs::path home = toPath(mu::getAppSettings()->getHomeFolder().c_str());
    const fo::fs::path docs = home / "Documents";
    const fo::fs::path dir = (fo::fs::is_directory(docs, ec) ? docs : home) / kAppFolder / "Exports";
    fo::fs::create_directories(dir, ec);
    return dir;
}

// "2026-09-17_14-03-22"
inline std::string timestamp()
{
    td::Date d(true);
    td::Time t(true);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02d-%02d_%02d-%02d-%02d",
                  d.getYear(), d.getMonth(), d.getDay(), t.getHour(), t.getMinute(), t.getSecond());
    return buf;
}

// Keeps letters, digits, '-' and '.', everything else becomes '_'.
inline std::string safeName(const std::string& s)
{
    std::string out;
    for (unsigned char c : s)
        out += (std::isalnum(c) || c == '-' || c == '.') ? (char) c : '_';
    while (out.find("__") != std::string::npos) out.replace(out.find("__"), 2, "_");
    return out;
}

// New folder <Exports>/<timestamp>_<label>; a suffix keeps it unique.
inline fo::fs::path createExportFolder(const std::string& label)
{
    std::error_code ec;
    const std::string base = timestamp() + "_" + safeName(label);
    fo::fs::path dir = exportsFolder() / toPath(base.c_str());
    for (int i = 2; fo::fs::exists(dir, ec); ++i)
        dir = exportsFolder() / toPath((base + "_" + std::to_string(i)).c_str());
    fo::fs::create_directories(dir, ec);
    return dir;
}

// <folder>/<name><ext>, or <name>_2<ext>, ... if taken.
inline fo::fs::path uniqueFile(const fo::fs::path& folder, const std::string& name, const char* ext)
{
    std::error_code ec;
    fo::fs::path p = folder / toPath((name + ext).c_str());
    for (int i = 2; fo::fs::exists(p, ec); ++i)
        p = folder / toPath((name + "_" + std::to_string(i) + ext).c_str());
    return p;
}

// file:// URL with percent-encoding; works for Explorer, Finder and xdg-open.
inline td::String fileUrl(const fo::fs::path& p)
{
    const std::u8string u = fo::fs::absolute(p).generic_u8string();
    std::string url = "file://";
    if (u.empty() || u[0] != u8'/') url += '/';
    static const char* hex = "0123456789ABCDEF";
    for (char8_t ch : u)
    {
        const unsigned char c = (unsigned char) ch;
        if (std::isalnum(c) || c == '/' || c == ':' || c == '-' || c == '_' || c == '.' || c == '~')
            url += (char) c;
        else
        {
            url += '%';
            url += hex[c >> 4];
            url += hex[c & 15];
        }
    }
    return td::String(url.c_str());
}

inline bool openFolder(const fo::fs::path& p)
{
    std::error_code ec;
    fo::fs::create_directories(p, ec);
    return syst::Shell::openUrl(fileUrl(p));
}

} // namespace ui::appfs
