#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

void PrintUsage() {
    std::cout << "TShorg Mod Compiler v1.1\n";
    std::cout << "Kullanim:\n";
    std::cout << "  TShorgModCompiler --init <mod_klasor_adi>   : Yeni mod sablonu olusturur.\n";
    std::cout << "  TShorgModCompiler --build [--x64|--x86] <klasor_yolu> : Modu .tsgm olarak derler.\n";
}

bool CreateTemplate(const std::string& modFolder) {
    fs::path targetPath(modFolder);
    if (fs::exists(targetPath)) {
        std::cerr << "[Hata] Klasor zaten mevcut: " << modFolder << "\n";
        return false;
    }

    // Klasor hiyerarsisini olustur
    fs::create_directories(targetPath / "assets");
    fs::create_directories(targetPath / "scripts");

    // 1. mod.json Şablonu
    std::ofstream jsonFile(targetPath / "mod.json");
    if (jsonFile.is_open()) {
        jsonFile << "{\n"
                 << "  \"id\": \"com.tda." << modFolder << "\",\n"
                 << "  \"name\": \"" << modFolder << " Mod\",\n"
                 << "  \"version\": \"1.0.0\",\n"
                 << "  \"author\": \"Gelistirici\",\n"
                 << "  \"description\": \"TShorg mod aciklamasi buraya gelecek.\",\n"
                 << "  \"entry_script\": \"scripts/main.bat\",\n"
                 << "  \"icon\": \"assets/icon.ico\"\n"
                 << "}\n";
        jsonFile.close();
    }

    // 2. Örnek Script Dosyası (scripts/main.bat)
    std::ofstream scriptFile(targetPath / "scripts" / "main.bat");
    if (scriptFile.is_open()) {
        scriptFile << "@echo off\n";
        scriptFile << "echo TShorg Mod Calisiyor: " << modFolder << "\n";
        scriptFile.close();
    }

    // 3. Beni Oku (README.txt)
    std::ofstream readmeFile(targetPath / "README.txt");
    if (readmeFile.is_open()) {
        readmeFile << "TShorg Mod Klasoru\n";
        readmeFile << " Derlemek icin:\n";
        readmeFile << " TShorgModCompiler --build --x64 \"" << modFolder << "\"\n";
        readmeFile.close();
    }

    std::cout << "[SUCCESS] Mod sablonu olusturuldu: " << fs::absolute(targetPath).string() << "\n";
    return true;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        PrintUsage();
        return 1;
    }

    std::string arg1 = argv[1];

    // --init Parametresi
    if (arg1 == "--init") {
        std::string modName = argv[2];
        return CreateTemplate(modName) ? 0 : 1;
    }

    // --build Parametresi
    std::string targetDir = "";
    std::string arch = "x64";
    bool buildFlag = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--build") {
            buildFlag = true;
        } else if (arg == "--x64") {
            arch = "x64";
        } else if (arg == "--x86") {
            arch = "x86";
        } else if (fs::is_directory(arg)) {
            targetDir = arg;
        }
    }

    if (!buildFlag || targetDir.empty()) {
        std::cerr << "[Hata] Gecersiz argumanlar veya klasor bulunamadi.\n";
        PrintUsage();
        return 1;
    }

    fs::path modPath(targetDir);
    std::string modName = modPath.filename().string();
    fs::path outputFile = modPath.parent_path() / (modName + ".tsgm");

    std::cout << "[+] Mod derleniyor: " << modName << " (" << arch << ")...\n";

    std::ofstream outFile(outputFile, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "[Hata] Hedef .tsgm dosyasi olusturulamadi!\n";
        return 1;
    }

    const char magic[4] = {'T', 'S', 'G', 'M'};
    outFile.write(magic, 4);
    uint8_t archCode = (arch == "x64") ? 64 : 32;
    outFile.write(reinterpret_cast<const char*>(&archCode), sizeof(archCode));

    size_t fileCount = 0;
    for (const auto& entry : fs::recursive_directory_iterator(modPath)) {
        if (entry.is_regular_file()) {
            std::string relativePath = fs::relative(entry.path(), modPath).string();
            uint32_t pathLen = static_cast<uint32_t>(relativePath.size());
            uint64_t fileSize = fs::file_size(entry.path());

            outFile.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
            outFile.write(relativePath.data(), pathLen);
            outFile.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));

            std::ifstream inFile(entry.path(), std::ios::binary);
            outFile << inFile.rdbuf();
            fileCount++;
        }
    }

    outFile.close();
    std::cout << "[SUCCESS] Mod derlendi: " << outputFile.string() << " (" << fileCount << " dosya paketlendi)\n";
    return 0;
}