#pragma once
#include <string>
#include <set>

// Профиль игрока — сохраняется в файл profile.dat рядом с игрой
struct Profile {
    int coins = 0;
    int level = 1;
    int xp = 0;                 // опыт внутри текущего уровня
    int equippedSkin = 0;       // надетый скин
    std::set<int> owned = {0};  // купленные скины (0 всегда есть)
    int highScoreClassic = 0;
    int levelsUnlocked = 1;     // открыто уровней в режиме "Уровни"

    // Сколько опыта нужно до следующего уровня: 50, 75, 100, ...
    int xpForNext() const { return 50 + 25 * (level - 1); }

    // Добавить опыт, вернуть число новых уровней
    int addXP(int amount);
    void addCoins(int c) { coins += c; }
    bool own(int id) const { return owned.count(id) > 0; }

    void load(const std::string& path = "profile.dat");
    void save(const std::string& path = "profile.dat") const;
};
