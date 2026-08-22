#include "ui/DisplayManager.h"
#include "ui/DisplaySupport.h"
#include "services/LuaEngine.h"
#include "services/StorageManager.h"
#include <dirent.h>
#include <sys/stat.h>

using namespace DisplayUi;

void DisplayManager::renderLuaScriptsScreen() {
    if (!luaScriptCount && luaRunStatus.length() == 0) {
        luaScriptCount = luaEngine.listScripts(luaScripts, LUA_UI_MAX_SCRIPTS);
        luaScriptSelection = 0;
    }
    drawModernHeader("LUA SCRIPTS", SPECTRUM_ACCENT);
    tft.fillRect(0, 15, 160, 90, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.setCursor(5, 19); tft.print("SD:/RFSuite/scripts");

    if (!luaEngine.isReady()) {
        tft.setTextColor(SPECTRUM_CRITICAL, ST77XX_BLACK);
        tft.setCursor(8, 45); tft.print("SD CARD NOT READY");
    } else if (!luaScriptCount) {
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setCursor(12, 43); tft.print("NO .LUA FILES");
        tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
        tft.setCursor(7, 58); tft.print("A TO REFRESH");
    } else {
        const int first = max(0, static_cast<int>(luaScriptSelection) - 2);
        for (int row = 0; row < 5 && first + row < static_cast<int>(luaScriptCount); ++row) {
            const int index = first + row;
            const int y = 32 + row * 13;
            const bool selected = index == static_cast<int>(luaScriptSelection);
            if (selected) tft.fillRoundRect(3, y - 2, 154, 12, 2, SPECTRUM_CARD_BG);
            tft.setTextColor(selected ? SPECTRUM_ACCENT : ST77XX_WHITE,
                             selected ? SPECTRUM_CARD_BG : ST77XX_BLACK);
            tft.setCursor(7, y); tft.print(selected ? "> " : "  ");
            String label = luaScripts[index];
            if (label.length() > 20) label = label.substring(0, 19) + "~";
            tft.print(label);
        }
    }
    if (luaRunStatus.length()) {
        String status = luaRunStatus;
        if (status.length() > 25) status = status.substring(0, 24) + "~";
        tft.setTextColor(status.startsWith("ERROR") ? SPECTRUM_CRITICAL : SPECTRUM_LOW,
                         ST77XX_BLACK);
        tft.setCursor(4, 94); tft.print(status);
    }
    drawModernFooter("B BACK", "UP/DN", "A RUN");
}

void DisplayManager::loadFileExplorerDirectory() {
    fileEntryCount = 0;
    fileSelection = 0;
    if (!storageManager.usingSd()) return;

    // SD.begin() registers the Arduino SD filesystem at /sd in ESP-IDF's VFS.
    // Enumerate through POSIX instead of File::openNextFile(): some FAT cards
    // expose valid entries with d_type == DT_UNKNOWN, which the Arduino FS
    // iterator silently skips.
    const String vfsPath = String("/sd") + filePath;
    DIR* dir = opendir(vfsPath.c_str());
    if (!dir) {
        fileStatus = "CANNOT OPEN FOLDER";
        Serial.printf("File explorer: opendir failed for %s\n", vfsPath.c_str());
        return;
    }

    for (dirent* entry = readdir(dir);
         entry && fileEntryCount < FILE_UI_MAX_ENTRIES;
         entry = readdir(dir)) {
        const String name(entry->d_name);
        if (!name.length() || name == "." || name == "..") continue;

        String entryPath = vfsPath;
        if (!entryPath.endsWith("/")) entryPath += "/";
        entryPath += name;
        struct stat info {};
        const bool hasInfo = stat(entryPath.c_str(), &info) == 0;
        fileNames[fileEntryCount] = name;
        fileDirectories[fileEntryCount] = entry->d_type == DT_DIR ||
            (hasInfo && S_ISDIR(info.st_mode));
        fileSizes[fileEntryCount] = fileDirectories[fileEntryCount] || !hasInfo
            ? 0 : static_cast<uint32_t>(info.st_size);
        Serial.printf("File explorer: %s %s%s\n",
                      fileDirectories[fileEntryCount] ? "DIR " : "FILE",
                      name.c_str(), hasInfo ? "" : " (no metadata)");
        ++fileEntryCount;
    }
    closedir(dir);
    fileStatus = fileEntryCount == FILE_UI_MAX_ENTRIES ? "SHOWING FIRST 32" : "";
    Serial.printf("File explorer: %u entries in %s\n",
                  static_cast<unsigned>(fileEntryCount), filePath.c_str());

    // Directories first, then alphabetical order.
    for (size_t i = 0; i < fileEntryCount; ++i) {
        for (size_t j = i + 1; j < fileEntryCount; ++j) {
            const bool swapNeeded =
                (fileDirectories[j] && !fileDirectories[i]) ||
                (fileDirectories[j] == fileDirectories[i] &&
                 fileNames[j].compareTo(fileNames[i]) < 0);
            if (!swapNeeded) continue;
            String swapName = fileNames[i]; fileNames[i] = fileNames[j]; fileNames[j] = swapName;
            const uint32_t swapSize = fileSizes[i]; fileSizes[i] = fileSizes[j]; fileSizes[j] = swapSize;
            const bool swapDir = fileDirectories[i]; fileDirectories[i] = fileDirectories[j]; fileDirectories[j] = swapDir;
        }
    }
}

void DisplayManager::renderFileExplorerScreen() {
    if (!fileEntryCount && fileStatus.length() == 0) loadFileExplorerDirectory();
    drawModernHeader("SD FILE EXPLORER", SPECTRUM_ACCENT);
    tft.fillRect(0, 15, 160, 90, ST77XX_BLACK);
    tft.setTextSize(1);
    String shownPath = String("SD:") + filePath;
    if (shownPath.length() > 25) shownPath = "~" + shownPath.substring(shownPath.length() - 24);
    tft.setCursor(4, 18);
    tft.setTextColor(ST77XX_GRAY, ST77XX_BLACK);
    tft.print(shownPath);

    if (!storageManager.usingSd()) {
        tft.setCursor(17, 48);
        tft.setTextColor(SPECTRUM_CRITICAL, ST77XX_BLACK);
        tft.print("SD CARD NOT READY");
    } else if (!fileEntryCount) {
        tft.setCursor(35, 48);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.print(fileStatus.length() ? fileStatus : "EMPTY FOLDER");
    } else {
        const int first = max(0, static_cast<int>(fileSelection) - 2);
        for (int row = 0; row < 5 && first + row < static_cast<int>(fileEntryCount); ++row) {
            const int index = first + row;
            const int y = 31 + row * 13;
            const bool selected = index == static_cast<int>(fileSelection);
            if (selected) tft.fillRoundRect(3, y - 2, 154, 12, 2, SPECTRUM_CARD_BG);
            tft.setCursor(6, y);
            tft.setTextColor(selected ? SPECTRUM_ACCENT : ST77XX_WHITE,
                             selected ? SPECTRUM_CARD_BG : ST77XX_BLACK);
            tft.print(selected ? ">" : " ");
            tft.print(fileDirectories[index] ? "[D] " : "[F] ");
            String label = fileNames[index];
            if (label.length() > 20) label = label.substring(0, 19) + "~";
            tft.print(label);
        }
    }
    if (fileStatus.length() && fileEntryCount) {
        tft.fillRect(0, 94, 160, 10, ST77XX_BLACK);
        tft.setCursor(5, 95); tft.setTextColor(SPECTRUM_LOW, ST77XX_BLACK);
        tft.print(fileStatus);
    }
    drawModernFooter("B UP/BACK", "UP/DN", "A OPEN");
}
