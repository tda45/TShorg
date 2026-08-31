#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "TSGS.h"
#include <vector>
#include <string>
#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// Form Tamponları
static char destBuf[512] = "";
static char paramBuf[512] = "";
static char iconBuf[512] = "";
static char pathBuf[256] = "kisayol.tsgs";
static char modPathBuf[256] = "";
static bool adminFlag = false;
static std::string statusMsg = "";

// Log & İstatistik Veri Yapıları
struct LogEntry {
    std::string timestamp;
    std::string message;
    ImVec4 color;
};

static std::vector<LogEntry> g_LogHistory;
static uint32_t g_TotalExecutions = 0;
static uint32_t g_TotalModInstalls = 0;
static uint32_t g_TotalFailures = 0;

void AddLog(const std::string& msg, const ImVec4& color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f)) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm buf;
    localtime_s(&buf, &in_time_t);

    std::ostringstream ss;
    ss << std::put_time(&buf, "[%H:%M:%S] ");
    
    g_LogHistory.push_back({ ss.str(), msg, color });
    statusMsg = msg;
}

// Mod Yöneticisi Veri Yapıları
struct InstalledMod {
    std::string folderName;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    bool enabled;
};

static std::vector<InstalledMod> g_InstalledMods;

void ScanInstalledMods() {
    g_InstalledMods.clear();
    fs::path modsDir = fs::current_path() / "mods";

    if (!fs::exists(modsDir)) {
        fs::create_directories(modsDir);
        return;
    }

    for (const auto& entry : fs::directory_iterator(modsDir)) {
        if (entry.is_directory()) {
            std::string folderName = entry.path().filename().string();
            fs::path jsonPath = entry.path() / "mod.json";
            
            InstalledMod mod;
            mod.folderName = folderName;
            mod.enabled = (folderName.rfind(".disabled") == std::string::npos);

            // Varsayılan Etiketler
            mod.name = folderName;
            mod.version = "1.0.0";
            mod.author = "Bilinmiyor";
            mod.description = "Açıklama bulunamadı.";

            // Basit JSON / Manifest Okuma
            if (fs::exists(jsonPath)) {
                std::ifstream jFile(jsonPath);
                std::string line;
                while (std::getline(jFile, line)) {
                    if (line.find("\"name\"") != std::string::npos) {
                        size_t first = line.find(':', line.find("\"name\"")) + 1;
                        size_t start = line.find('"', first) + 1;
                        size_t end = line.find('"', start);
                        if (start != std::string::npos && end != std::string::npos)
                            mod.name = line.substr(start, end - start);
                    }
                    else if (line.find("\"version\"") != std::string::npos) {
                        size_t first = line.find(':', line.find("\"version\"")) + 1;
                        size_t start = line.find('"', first) + 1;
                        size_t end = line.find('"', start);
                        if (start != std::string::npos && end != std::string::npos)
                            mod.version = line.substr(start, end - start);
                    }
                    else if (line.find("\"author\"") != std::string::npos) {
                        size_t first = line.find(':', line.find("\"author\"")) + 1;
                        size_t start = line.find('"', first) + 1;
                        size_t end = line.find('"', start);
                        if (start != std::string::npos && end != std::string::npos)
                            mod.author = line.substr(start, end - start);
                    }
                }
            }

            g_InstalledMods.push_back(mod);
        }
    }
}

std::string OpenFileDialog(const char* filter) {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

bool InstallTSGMMod(const std::string& tsgmPath) {
    std::ifstream inFile(tsgmPath, std::ios::binary);
    if (!inFile.is_open()) return false;

    char magic[4];
    inFile.read(magic, 4);
    if (magic[0] != 'T' || magic[1] != 'S' || magic[2] != 'G' || magic[3] != 'M') return false;

    uint8_t archCode;
    inFile.read(reinterpret_cast<char*>(&archCode), sizeof(archCode));

    fs::path extractDir = fs::current_path() / "mods" / fs::path(tsgmPath).stem();
    fs::create_directories(extractDir);

    while (inFile.peek() != EOF) {
        uint32_t pathLen = 0;
        if (!inFile.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen))) break;

        std::string relPath(pathLen, '\0');
        inFile.read(&relPath[0], pathLen);

        uint64_t fileSize = 0;
        inFile.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

        fs::path outFilePath = extractDir / relPath;
        fs::create_directories(outFilePath.parent_path());

        std::ofstream outFile(outFilePath, std::ios::binary);
        std::vector<char> buffer(fileSize);
        inFile.read(buffer.data(), fileSize);
        outFile.write(buffer.data(), fileSize);
    }

    g_TotalModInstalls++;
    ScanInstalledMods();
    return true;
}

void DropCallback(GLFWwindow* window, int count, const char** paths) {
    if (count > 0) {
        std::string droppedPath = paths[0];
        if (droppedPath.length() >= 5 && droppedPath.substr(droppedPath.length() - 5) == ".tsgm") {
            if (InstallTSGMMod(droppedPath)) {
                AddLog("Mod başarıyla kuruldu: " + fs::path(droppedPath).filename().string(), ImVec4(0.2f, 0.9f, 0.3f, 1.0f));
            } else {
                g_TotalFailures++;
                AddLog("Hata: Geçersiz .tsgm mod dosyası!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
            }
        }
        else if (droppedPath.length() >= 5 && droppedPath.substr(droppedPath.length() - 5) == ".tsgs") {
            TShorgShortcut sc;
            if (TSGS::Load(droppedPath, sc)) {
                strncpy_s(destBuf, sc.destination.c_str(), sizeof(destBuf) - 1);
                strncpy_s(paramBuf, sc.params.c_str(), sizeof(paramBuf) - 1);
                strncpy_s(iconBuf, sc.iconPath.c_str(), sizeof(iconBuf) - 1);
                adminFlag = sc.runAsAdmin;
                strncpy_s(pathBuf, droppedPath.c_str(), sizeof(pathBuf) - 1);
                AddLog("Sürüklenen .tsgs dosyası yüklendi: " + droppedPath, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
            }
        } else {
            strncpy_s(destBuf, droppedPath.c_str(), sizeof(destBuf) - 1);
            AddLog("Hedef dosya yolu aktarıldı: " + droppedPath);
        }
    }
}

void ApplyNightRedTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;

    colors[ImGuiCol_Text]                  = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.08f, 0.08f, 0.09f, 0.96f);
    colors[ImGuiCol_Header]                = ImVec4(0.55f, 0.10f, 0.15f, 0.55f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.70f, 0.12f, 0.18f, 0.80f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.85f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.50f, 0.08f, 0.12f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.70f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.85f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.70f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.90f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.40f, 0.06f, 0.10f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.30f, 0.06f, 0.09f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.70f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.60f, 0.10f, 0.15f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.45f, 0.10f, 0.14f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.70f, 0.12f, 0.18f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.85f, 0.15f, 0.20f, 1.00f);
}

int main(int argc, char** argv) {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string currentExePath(exePath);

    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--create" && argc > 2) {
            strncpy_s(destBuf, argv[2], sizeof(destBuf) - 1);
            statusMsg = "Sağ tık menüsünden hedef aktarıldı.";
        }
        else if (arg1 == "--register-context") {
            TSGS::RegisterContextMenu(currentExePath);
            return 0;
        }
        else if (arg1 == "--unregister-context") {
            TSGS::UnregisterContextMenu();
            return 0;
        }
        else if (arg1.length() >= 5 && arg1.substr(arg1.length() - 5) == ".tsgs") {
            TShorgShortcut shortcut;
            if (TSGS::Load(arg1, shortcut)) {
                TSGS::Run(shortcut);
            }
            return 0;
        }
    }

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(720, 620, "TShorg - Kısayol & Mod Yöneticisi", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetDropCallback(window, DropCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ApplyNightRedTheme();

    static const ImWchar turkish_glyph_ranges[] = {
        0x0020, 0x00FF, // Temel Latin + Latin 1 Supplement
        0x0100, 0x017F, // Latin Extended-A (Türkçe harfler: Ğ, ğ, İ, ı, Ş, ş)
        0,
    };

    std::string fontPath = "font/custom.ttf";
    if (!std::filesystem::exists(fontPath)) {
        fontPath = "C:\\Windows\\Fonts\\segoeui.ttf";
    }

    if (std::filesystem::exists(fontPath)) {
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 17.0f, NULL, turkish_glyph_ranges);
    } else {
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ScanInstalledMods();
    AddLog("TShorg Başlatıldı. Sistem hazır.", ImVec4(0.4f, 0.9f, 0.4f, 1.0f));

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("TShorg Dashboard", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

        ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.30f, 1.0f), "TShorg Kısayol & Mod Yöneticisi");
        ImGui::SameLine(ImGui::GetWindowWidth() - 180);
        ImGui::TextDisabled("v1.0");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("MainTabBar")) {
            
            // ================= SEKMELER 1 & 2: KISAYOL & MOD YÜKLEYİCİ =================
            if (ImGui::BeginTabItem("Kısayol & Mod Ekle")) {
                ImGui::Spacing();
                
                // 1. Hedef Exe
                ImGui::Text("Hedef Program (Appdata kısayolu yani yüzdeli olan veya Göreceli Yol Destekli):");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.0f);
                ImGui::InputText("##dest", destBuf, sizeof(destBuf));
                ImGui::SameLine();
                if (ImGui::Button("Gözat...##exe", ImVec2(80, 0))) {
                    std::string selected = OpenFileDialog("Executable (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");
                    if (!selected.empty()) strncpy_s(destBuf, selected.c_str(), sizeof(destBuf) - 1);
                }

                ImGui::Spacing();

                // 2. Parametreler
                ImGui::Text("Parametreler (örneğin: -windowed /config %USERPROFILE%\\conf.ini):");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##params", paramBuf, sizeof(paramBuf));

                ImGui::Spacing();

                // 3. İkon Yolu
                ImGui::Text("Özel İkon Yolu (.ico / .exe):");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 90.0f);
                ImGui::InputText("##icon", iconBuf, sizeof(iconBuf));
                ImGui::SameLine();
                if (ImGui::Button("Gözat...##icon", ImVec2(80, 0))) {
                    std::string selected = OpenFileDialog("Icon / Exe (*.ico;*.exe)\0*.ico;*.exe\0All Files (*.*)\0*.*\0");
                    if (!selected.empty()) strncpy_s(iconBuf, selected.c_str(), sizeof(iconBuf) - 1);
                }

                ImGui::Spacing();

                // 4. Yönetici İzni Flag'i
                ImGui::Checkbox("Yönetici Olarak Çalıştır (Run as Admin)", &adminFlag);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // 5. Kayıt Yolu
                ImGui::Text("Kayıt Dosya Adı (.tsgs):");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText("##path", pathBuf, sizeof(pathBuf));

                ImGui::Spacing();

                // Ana İşlem Butonları
                if (ImGui::Button("Oluştur (.tsgs Kaydet)", ImVec2(180, 32))) {
                    TShorgShortcut sc{ destBuf, paramBuf, iconBuf, adminFlag };
                    if (TSGS::Save(pathBuf, sc)) {
                        AddLog("Başarıyla kaydedildi: " + std::string(pathBuf), ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
                    } else {
                        g_TotalFailures++;
                        AddLog("Hata: Dosya yazılamadı!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button(".tsgs Yükle", ImVec2(110, 32))) {
                    TShorgShortcut sc;
                    if (TSGS::Load(pathBuf, sc)) {
                        strncpy_s(destBuf, sc.destination.c_str(), sizeof(destBuf) - 1);
                        strncpy_s(paramBuf, sc.params.c_str(), sizeof(paramBuf) - 1);
                        strncpy_s(iconBuf, sc.iconPath.c_str(), sizeof(iconBuf) - 1);
                        adminFlag = sc.runAsAdmin;
                        AddLog("Dosya başarıyla yüklendi: " + std::string(pathBuf), ImVec4(0.3f, 0.8f, 1.0f, 1.0f));
                    } else {
                        g_TotalFailures++;
                        AddLog("Hata: Dosya okunamadı!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Şimdi Çalıştır", ImVec2(110, 32))) {
                    TShorgShortcut sc{ destBuf, paramBuf, iconBuf, adminFlag };
                    if (TSGS::Run(sc)) {
                        g_TotalExecutions++;
                        AddLog("Uygulama tetiklendi: " + std::string(destBuf), ImVec4(0.2f, 0.9f, 0.4f, 1.0f));
                    } else {
                        g_TotalFailures++;
                        AddLog("Hata: Uygulama başlatılamadı!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.30f, 1.0f), "Mod Paket Kurucu (.tsgm):");
                ImGui::Spacing();

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
                ImGui::InputText("##modpath", modPathBuf, sizeof(modPathBuf));
                ImGui::SameLine();
                if (ImGui::Button("Mod Seç...", ImVec2(110, 0))) {
                    std::string selectedMod = OpenFileDialog("TShorg Mod Package (*.tsgm)\0*.tsgm\0All Files (*.*)\0*.*\0");
                    if (!selectedMod.empty()) strncpy_s(modPathBuf, selectedMod.c_str(), sizeof(modPathBuf) - 1);
                }

                if (ImGui::Button("Modu Yükle & Kur (.tsgm)", ImVec2(200, 28))) {
                    if (strlen(modPathBuf) > 0 && InstallTSGMMod(modPathBuf)) {
                        AddLog("Mod başarıyla eklendi ve çıkarıldı!", ImVec4(0.2f, 0.9f, 0.3f, 1.0f));
                    } else {
                        g_TotalFailures++;
                        AddLog("Hata: Mod dosyası okunamadı veya geçersiz!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextDisabled("Windows Sistem Entegrasyonları:");
                ImGui::Spacing();

                if (ImGui::Button(".tsgs Dosya Tescili", ImVec2(150, 26))) {
                    if (TSGS::RegisterFileAssociation(currentExePath)) {
                        AddLog(".tsgs uzantısı Windows Registy'ye işlendi.");
                    } else {
                        g_TotalFailures++;
                        AddLog("Hata: Registry yazma başarısız!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Sağ Tık Menüsü Ekle", ImVec2(160, 26))) {
                    if (TSGS::RegisterContextMenu(currentExePath)) {
                        AddLog("Sağ tık menüsüne 'TShorg ile Kısayol Oluştur' eklendi.");
                    } else {
                        g_TotalFailures++;
                        AddLog("Hata: Registry yazma başarısız!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Sağ Tık Kaldır", ImVec2(120, 26))) {
                    if (TSGS::UnregisterContextMenu()) {
                        AddLog("Sağ tık menüsü temizlendi.");
                    } else {
                        g_TotalFailures++;
                        AddLog("Hata: Registry silinemedi!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                    }
                }

                ImGui::EndTabItem();
            }

            // ================= ÖZELLİK 2: YÜKLÜ MODLARI YÖNETME SEKMESİ =================
            if (ImGui::BeginTabItem("Yüklü Modlar (Mod Manager)")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.40f, 1.0f), "Kurulu Modlar ve Eklentiler");
                ImGui::SameLine(ImGui::GetWindowWidth() - 140);
                if (ImGui::Button("Yenile / Tara", ImVec2(120, 24))) {
                    ScanInstalledMods();
                    AddLog("Mod klasörü yeniden tarandı.");
                }
                ImGui::Separator();
                ImGui::Spacing();

                if (g_InstalledMods.empty()) {
                    ImGui::TextDisabled("Henüz yüklü bir mod bulunamadı. (mods/ klasörü boş)");
                } else {
                    for (size_t i = 0; i < g_InstalledMods.size(); ++i) {
                        auto& mod = g_InstalledMods[i];
                        ImGui::PushID(static_cast<int>(i));

                        bool state = mod.enabled;
                        if (ImGui::Checkbox("##modstate", &state)) {
                            fs::path oldPath = fs::current_path() / "mods" / mod.folderName;
                            std::string newFolderName = mod.folderName;
                            
                            if (state) {
                                // Enable: remove .disabled
                                size_t pos = newFolderName.rfind(".disabled");
                                if (pos != std::string::npos) newFolderName.erase(pos);
                            } else {
                                // Disable: add .disabled
                                newFolderName += ".disabled";
                            }
                            
                            fs::path newPath = fs::current_path() / "mods" / newFolderName;
                            try {
                                fs::rename(oldPath, newPath);
                                mod.folderName = newFolderName;
                                mod.enabled = state;
                                AddLog("Mod durumu değiştirildi: " + mod.name + (state ? " [Aktif]" : " [Pasif]"));
                            } catch (...) {
                                AddLog("Hata: Mod klasör adı değiştirilemedi!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                            }
                        }

                        ImGui::SameLine();
                        ImGui::TextColored(mod.enabled ? ImVec4(0.95f, 0.95f, 0.95f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                                           "%s (v%s)", mod.name.c_str(), mod.version.c_str());
                        
                        ImGui::SameLine(ImGui::GetWindowWidth() - 100);
                        if (ImGui::Button("Sil (Uninstall)")) {
                            fs::path modPath = fs::current_path() / "mods" / mod.folderName;
                            try {
                                fs::remove_all(modPath);
                                AddLog("Mod tamamen kaldırıldı: " + mod.name, ImVec4(0.95f, 0.35f, 0.40f, 1.0f));
                                ScanInstalledMods();
                                ImGui::PopID();
                                break;
                            } catch (...) {
                                AddLog("Hata: Mod klasörü silinemedi!", ImVec4(0.95f, 0.25f, 0.30f, 1.0f));
                            }
                        }

                        ImGui::TextDisabled("  Yazar: %s | Klasör: %s", mod.author.c_str(), mod.folderName.c_str());
                        ImGui::Separator();
                        ImGui::PopID();
                    }
                }

                ImGui::EndTabItem();
            }

            // ================= ÖZELLİK 4: GELİŞMİŞ LOG & İSTATİSTİK PANELİ =================
            if (ImGui::BeginTabItem("İstatistikler & Log")) {
                ImGui::Spacing();
                
                // İstatistik Rozetleri Paneli
                ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.40f, 1.0f), "Sistem İstatistikleri");
                ImGui::Separator();
                
                ImGui::Columns(4, "StatColumns", false);
                
                ImGui::Text("Çalıştırma");
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "%u", g_TotalExecutions);
                ImGui::NextColumn();

                ImGui::Text("Kurulan Mod");
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.3f, 1.0f), "%u", g_TotalModInstalls);
                ImGui::NextColumn();

                ImGui::Text("Aktif Mod Sayısı");
                size_t activeCount = 0;
                for (const auto& m : g_InstalledMods) if (m.enabled) activeCount++;
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "%zu", activeCount);
                ImGui::NextColumn();

                ImGui::Text("Hata Sayısı");
                ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.30f, 1.0f), "%u", g_TotalFailures);
                ImGui::Columns(1);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.40f, 1.0f), "Canlı Sistem Günlüğü (Logs)");
                ImGui::SameLine(ImGui::GetWindowWidth() - 110);
                if (ImGui::Button("Temizle", ImVec2(90, 22))) {
                    g_LogHistory.clear();
                }

                ImGui::BeginChild("LogScrollArea", ImVec2(0, -30), true, ImGuiWindowFlags_HorizontalScrollbar);
                for (const auto& log : g_LogHistory) {
                    ImGui::TextDisabled("%s", log.timestamp.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(log.color, "%s", log.message.c_str());
                }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // Alt Durum Çubuğu
        if (!statusMsg.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.40f, 1.0f), "Son Durum: %s", statusMsg.c_str());
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.06f, 0.06f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}