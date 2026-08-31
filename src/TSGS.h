#ifndef TSGS_H
#define TSGS_H

#include <string>

struct TShorgShortcut {
    std::string destination;
    std::string params;
    std::string iconPath;
    bool runAsAdmin = false;
};

class TSGS {
public:
    static bool Save(const std::string& filepath, const TShorgShortcut& shortcut);
    static bool Load(const std::string& filepath, TShorgShortcut& shortcut);
    static bool Run(const TShorgShortcut& shortcut);
    
    // Windows Registry işlemleri
    static bool RegisterFileAssociation(const std::string& appPath);
    static bool RegisterContextMenu(const std::string& appPath);
    static bool UnregisterContextMenu();

    // Yol ve Çevre Değişkeni Yönetimi
    static std::string CleanPath(const std::string& path);
    static std::string ExpandEnvVars(const std::string& path);
    static std::string ResolvePath(const std::string& path);
};

#endif // TSGS_H