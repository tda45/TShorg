#ifndef MODMAKER_H
#define MODMAKER_H

#include <string>
#include <vector>

class ModMaker {
public:
    // CLI Modlama Arayüzü
    static void RunInteractiveCLI();
    static bool GenerateTemplate(const std::string& templateType, const std::string& outputDir);

private:
    // Konsol Animasyon ve Görsel Yardımcıları
    static void SetConsoleColorRed();
    static void ResetConsoleColor();
    static void PrintBanner();
    static void ShowProgressBar(const std::string& taskName, int durationMs);
    static void ShowSpinner(const std::string& taskName, int durationMs);
};

#endif // MODMAKER_H