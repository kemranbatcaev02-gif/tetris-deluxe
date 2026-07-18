#include "SaveData.h"
#include <fstream>
#include <sstream>

int Profile::addXP(int amount) {
    xp += amount;
    int gained = 0;
    while (xp >= xpForNext()) {
        xp -= xpForNext();
        level++;
        gained++;
    }
    return gained;
}

void Profile::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) return; // нет файла — стартуем с нуля
    std::string key;
    while (f >> key) {
        if (key == "coins") f >> coins;
        else if (key == "level") f >> level;
        else if (key == "xp") f >> xp;
        else if (key == "equipped") f >> equippedSkin;
        else if (key == "highscore") f >> highScoreClassic;
        else if (key == "unlocked") f >> levelsUnlocked;
        else if (key == "owned") {
            std::string line;
            std::getline(f, line);
            std::istringstream ss(line);
            owned.clear(); owned.insert(0);
            int id;
            while (ss >> id) owned.insert(id);
        }
    }
    if (level < 1) level = 1;
    if (levelsUnlocked < 1) levelsUnlocked = 1;
}

void Profile::save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return;
    f << "coins " << coins << "\n";
    f << "level " << level << "\n";
    f << "xp " << xp << "\n";
    f << "equipped " << equippedSkin << "\n";
    f << "highscore " << highScoreClassic << "\n";
    f << "unlocked " << levelsUnlocked << "\n";
    f << "owned";
    for (int id : owned) f << " " << id;
    f << "\n";
}
