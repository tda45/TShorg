#include "TSGS.h"
#include <fstream>
#include <windows.h>
#include <shellapi.h>
#include <filesystem>

namespace fs = std::filesystem;

std::string TSGS::CleanPath(const std::string& path) {
    if (path.empty()) return "";
    std::string cleaned = path;
    if (cleaned.front() == '"' && cleaned.back() == '"' && cleaned.length() >= 2) {
        cleaned = cleaned.substr(1, cleaned.length() - 2);
    }
    return cleaned;
}

// Windows Çevre Değişkenlerini Çözme (%APPDATA%, %USERPROFILE% vb.)
std::string TSGS::ExpandEnvVars(const std::string& path) {
    if (path.find('%') == std::string::npos) return path;

    char buffer[MAX_PATH];
    DWORD ret = ExpandEnvironmentStringsA(path.c_str(), buffer, MAX_PATH);
    if (ret > 0 && ret <= MAX_PATH) {
        return std::string(buffer);
    }
    return path;
}

// Göreceli Yolları ve Çevre Değişkenlerini Tam Yola Çevirme
std::string TSGS::ResolvePath(const std::string& path) {
    std::string cleaned = CleanPath(path);
    if (cleaned.empty()) return "";

    std::string expanded = ExpandEnvVars(cleaned);
    
    try {
        fs::path p(expanded);
        if (p.is_relative()) {
            // TShorg'un çalıştığı kök dizine göre göreceli yolu çöz
            p = fs::absolute(p);
        }
        return p.string();
    } catch (...) {
        return expanded;
    }
}

bool TSGS::Save(const std::string& filepath, const TShorgShortcut& shortcut) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "[Destination]\n";
    file << "\"" << CleanPath(shortcut.destination) << "\"\n\n";

    file << "[Params]\n";
    file << "\"" << shortcut.params << "\"\n\n";

    file << "[Icon]\n";
    file << "\"" << CleanPath(shortcut.iconPath) << "\"\n\n";

    file << "[Admin]\n";
    file << (shortcut.runAsAdmin ? "true" : "false") << "\n";

    return true;
}

bool TSGS::Load(const std::string& filepath, TShorgShortcut& shortcut) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string line;
    std::string currentSection = "";

    shortcut.destination.clear();
    shortcut.params.clear();
    shortcut.iconPath.clear();
    shortcut.runAsAdmin = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line == "[Destination]") currentSection = "Destination";
        else if (line == "[Params]") currentSection = "Params";
        else if (line == "[Icon]") currentSection = "Icon";
        else if (line == "[Admin]") currentSection = "Admin";
        else {
            if (currentSection == "Admin") {
                shortcut.runAsAdmin = (line == "true" || line == "1");
            } else {
                size_t firstQuote = line.find('"');
                size_t lastQuote = line.rfind('"');
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
                    std::string val = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    if (currentSection == "Destination") shortcut.destination = val;
                    else if (currentSection == "Params") shortcut.params = val;
                    else if (currentSection == "Icon") shortcut.iconPath = val;
                }
            }
        }
    }
    return true;
}

bool TSGS::Run(const TShorgShortcut& shortcut) {
    // Dinamik yol ve çevre değişkeni çözümü
    std::string resolvedDest = ResolvePath(shortcut.destination);
    if (resolvedDest.empty()) return false;

    std::string resolvedParams = ExpandEnvVars(shortcut.params);

    // Hedef Exe'nin çalışma dizinini tespit et
    std::string workingDir = "";
    size_t lastSlash = resolvedDest.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        workingDir = resolvedDest.substr(0, lastSlash);
    }

    SHELLEXECUTEINFOA ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = shortcut.runAsAdmin ? "runas" : "open";
    ShExecInfo.lpFile = resolvedDest.c_str();
    ShExecInfo.lpParameters = resolvedParams.empty() ? NULL : resolvedParams.c_str();
    ShExecInfo.lpDirectory = workingDir.empty() ? NULL : workingDir.c_str();
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;

    return ShellExecuteExA(&ShExecInfo);
}

// .tsgs dosya uzantısı tescili
bool TSGS::RegisterFileAssociation(const std::string& appPath) {
    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\.tsgs", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "", 0, REG_SZ, (const BYTE*)"TShorgFile", 11);
        RegCloseKey(hKey);
    } else return false;

    std::string command = "\"" + appPath + "\" \"%1\"";
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\TShorgFile\\shell\\open\\command", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "", 0, REG_SZ, (const BYTE*)command.c_str(), (DWORD)command.length() + 1);
        RegCloseKey(hKey);
    } else return false;

    return true;
}

// Windows Gezgini Sağ Tık Menüsü Entegrasyonu (.exe dosyaları için)
bool TSGS::RegisterContextMenu(const std::string& appPath) {
    HKEY hKey;
    std::string regPath = "Software\\Classes\\exefile\\shell\\TShorgCreate";
    
    if (RegCreateKeyExA(HKEY_CURRENT_USER, regPath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        const char* menuName = "TShorg ile Kısayol Oluştur";
        RegSetValueExA(hKey, "", 0, REG_SZ, (const BYTE*)menuName, (DWORD)strlen(menuName) + 1);
        // Sağ tık menüsünde ikon olarak TShorg'un kendi exe ikonunu göster
        RegSetValueExA(hKey, "Icon", 0, REG_SZ, (const BYTE*)appPath.c_str(), (DWORD)appPath.length() + 1);
        RegCloseKey(hKey);
    } else return false;

    std::string cmdPath = regPath + "\\command";
    std::string command = "\"" + appPath + "\" --create \"%1\"";
    if (RegCreateKeyExA(HKEY_CURRENT_USER, cmdPath.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "", 0, REG_SZ, (const BYTE*)command.c_str(), (DWORD)command.length() + 1);
        RegCloseKey(hKey);
    } else return false;

    return true;
}

bool TSGS::UnregisterContextMenu() {
    std::string cmdPath = "Software\\Classes\\exefile\\shell\\TShorgCreate\\command";
    std::string mainPath = "Software\\Classes\\exefile\\shell\\TShorgCreate";
    
    RegDeleteKeyA(HKEY_CURRENT_USER, cmdPath.c_str());
    return RegDeleteKeyA(HKEY_CURRENT_USER, mainPath.c_str()) == ERROR_SUCCESS;
}