#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <cstdint>

// Стиль отрисовки блока
enum class SkinStyle {
    Solid,    // классика (бесплатно)
    Flat,     // простой матовый
    Pixel,    // ретро-фаска
    Gradient, // вертикальный градиент
    Gloss,    // глянец
    Neon,     // неоновое свечение
    Rainbow,  // радужный перелив
    Crystal,  // кристалл с гранями
    Gold,     // золото
    Galaxy    // космос со звёздами
};

struct Skin {
    int id;
    std::string name;
    int price;         // цена в монетах
    SkinStyle style;
    std::string desc;  // короткое описание
};

const std::vector<Skin>& allSkins();
const Skin& skinById(int id);

// Рисует один блок клетки в стиле скина.
// (gx, gy) — координаты в сетке (для узоров/радуги), time — время анимации.
void drawBlock(sf::RenderTarget& target, float x, float y, float size,
               sf::Color base, SkinStyle style, float time,
               int gx, int gy, std::uint8_t alpha = 255);
