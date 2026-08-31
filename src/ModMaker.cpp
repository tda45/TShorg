#include "ModMaker.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <filesystem>

namespace fs = std::filesystem;

void ModMaker::SetConsoleColorRed() {
    // Windows Konsolunda "Gece Kırmızısı" için Koyu Kırmızı / Parlak Kırmızı tonu
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
}

void ModMaker::ResetConsoleColor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void ModMaker::PrintBanner() {
    SetConsoleColorRed();
    std::cout << R"(
  _____ ____  _                   __  __           _   __  __      _        
 |_   _/ ___|| |__   ___  _ __ __|  \/  | __ _ | | _|  \/  | ___| | _____ 
   | | \___ \| '_ \ / _ \| '__/ _` | |\/| |/ _` | |/ / |\/| |/ _ \ |/ / _ \
   | |  ___) | | | | (_) | | | (_| | |  | | (_| |   <| |  | |  __/   <  __/
   |_| |____/|_| |_|\___/|_|  \__,_|_|  |_|\__,_|_|\_\_|  |_|\___|_|\_\___|
)";
    std::cout << "  ===================================================================\n";
    std::cout << "             TShorg Mod & Template Creator v1.0 [Gece Kırmızısı]\n";
    std::cout << "  ===================================================================\n\n";
    ResetConsoleColor();
}

void ModMaker::ShowSpinner(const std::string& taskName, int durationMs) {
    SetConsoleColorRed();
    const char spinner[] = {'|', '/', '-', '\\'};
    int steps = durationMs / 100;
    
    for (int i = 0; i < steps; ++i) {
        std::cout << "\r[" << spinner[i % 4] << "] " << taskName << " işleniyor..." << stdflush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\r[OK] " << taskName << " tamamlandı!          \n";
    ResetConsoleColor();
}

void ModMaker::ShowProgressBar(const std::string& taskName, int durationMs) {
    SetConsoleColorRed();
    int totalBars = 30;
    int stepTime = durationMs / totalBars;

    std::cout << taskName << ":\n[";
    for (int i = 0; i <= totalBars; ++i) {
        std::cout << "\r[";
        for (int j = 0; j < i; ++j) std::cout << "█";
        for (int j = i; j < totalBars; ++j) std::cout << " ";
        int percent = (i * 100) / totalBars;
        std::cout << "] " << percent << "%" << stdflush;
        std::this_thread::sleep_for(std::chrono::milliseconds(stepTime));
    }
    std::cout << "\n\n";
    ResetConsoleColor();
}

bool ModMaker::GenerateTemplate(const std::string& templateType, const std::string& outputDir) {
    fs::create_directories(outputDir);

    if (templateType == "batch") {
        // Çoklu Oyun / Program Başlatıcı Modu
        std::ofstream file(outputDir + "/game_launcher.tsgs");
        file << "[Destination]\n\"C:\\Games\\MyGame\\game.exe\"\n\n";
        file << "[Params]\n\"-fullscreen -high\"\n\n";
        file << "[Icon]\n\"C:\\Games\\MyGame\\icon.ico\"\n\n";
        file << "[Admin]\ntrue\n";
        file.close();

        // Mod Bilgi Notu
        std::ofstream info(outputDir + "/README_MOD.txt");
        info << "[TShorg Game Launcher Mod Template]\n";
        info << "Bu mod şablonu oyunları yüksek öncelikle ve yönetici izniyle başlatmak için optimize edilmiştir.\n";
        info.close();
    } 
    else if (templateType == "hook") {
        // Script / Pre-run Hook Modu
        std::ofstream script(outputDir + "/pre_run.bat");
        script << "@echo off\n";
        script << "echo [TShorg Mod] Uygulama baslatilmadan once temizlik yapiliyor...\n";
        script << "timeout /t 2 > nul\n";
        script.close();

        std::ofstream file(outputDir + "/modded_app.tsgs");
        file << "[Destination]\n\"%APPDATA%\\MyCustomApp\\app.exe\"\n\n";
        file << "[Params]\n\"--modded-mode\"\n\n";
        file << "[Icon]\n\"\"\n\n";
        file << "[Admin]\nfalse\n";
        file.close();
    }
    else {
        // Varsayılan Temel Şablon
        std::ofstream file(outputDir + "/custom_mod.tsgs");
        file << "[Destination]\n\"C:\\Windows\\notepad.exe\"\n\n";
        file << "[Params]\n\"\"\n\n";
        file << "[Icon]\n\"\"\n\n";
        file << "[Admin]\nfalse\n";
        file.close();
    }

    return true;
}

void ModMaker::RunInteractiveCLI() {
    system("cls"); // Konsolu Temizle
    PrintBanner();

    SetConsoleColorRed();
    std::cout << ">> Hangi mod şablonunu oluşturmak istiyorsunuz?\n";
    ResetConsoleColor();
    std::cout << "  [1] Game Launcher Mod (Oyun / Yüksek Performans Şablonu)\n";
    std::cout << "  [2] Script Hook Mod (Çalıştırma Öncesi .bat / C++ Hazırlık Şablonu)\n";
    std::cout << "  [3] Basic Custom Mod (Temel TSGS Şablonu)\n\n";

    std::cout << "Seçiminiz (1-3): ";
    int choice = 1;
    std::cin >> choice;

    std::string type = "basic";
    if (choice == 1) type = "batch";
    else if (choice == 2) type = "hook";

    std::cout << "\nModun kaydedileceği klasör adı: ";
    std::string folderName;
    std::cin >> folderName;

    std::cout << "\n";
    ShowSpinner("Mod Dizin Yapısı Hazırlanıyor", 1200);
    ShowProgressBar("Şablon Dosyaları ve Yapılandırma Yazılıyor", 1800);

    if (GenerateTemplate(type, folderName)) {
        SetConsoleColorRed();
        std::cout << "=========================================================\n";
        std::cout << " [BAŞARILI] Mod Şablonu '" << folderName << "' Klasörüne Yapılandırıldı!\n";
        std::cout << "=========================================================\n";
        ResetConsoleColor();
        std::cout << "\n[Bilgi Notu]: Oluşturulan .tsgs ve mod yapılandırma dosyalarını\n";
        std::cout << "kendi projenize göre düzenleyip TShorg ile doğrudan dağıtabilirsiniz.\n\n";
    }
}