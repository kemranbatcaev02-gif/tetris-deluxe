#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdint>

// ---- Размеры игрового поля ----
inline constexpr int COLS = 10;
inline constexpr int ROWS = 20;
inline constexpr int CELL = 32;

// ---- Окно ----
inline constexpr int WINDOW_W = 900;
inline constexpr int WINDOW_H = 740;

// ---- Расположение поля ----
inline constexpr int BOARD_X = 280;
inline constexpr int BOARD_Y = 60;
inline constexpr int BOARD_W = COLS * CELL; // 320
inline constexpr int BOARD_H = ROWS * CELL; // 640

// UTF-8 std::string -> sf::String (чтобы кириллица рисовалась корректно на любом компиляторе)
inline sf::String u(const std::string& s) {
    return sf::String::fromUtf8(s.begin(), s.end());
}

// Плавное смешивание двух цветов (t от 0 до 1)
inline sf::Color mix(sf::Color a, sf::Color b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    return sf::Color(
        (std::uint8_t)(a.r + (b.r - a.r) * t),
        (std::uint8_t)(a.g + (b.g - a.g) * t),
        (std::uint8_t)(a.b + (b.b - a.b) * t),
        (std::uint8_t)(a.a + (b.a - a.a) * t));
}

// HSV -> RGB для радужных скинов
inline sf::Color hsv(float h, float s, float v, std::uint8_t a = 255) {
    h = std::fmod(h, 360.f); if (h < 0) h += 360.f;
    float c = v * s;
    float x = c * (1 - std::fabs(std::fmod(h / 60.f, 2.f) - 1));
    float m = v - c;
    float r = 0, g = 0, b = 0;
    if (h < 60)      { r = c; g = x; }
    else if (h < 120){ r = x; g = c; }
    else if (h < 180){ g = c; b = x; }
    else if (h < 240){ g = x; b = c; }
    else if (h < 300){ r = x; b = c; }
    else             { r = c; b = x; }
    return sf::Color((std::uint8_t)((r + m) * 255), (std::uint8_t)((g + m) * 255),
                     (std::uint8_t)((b + m) * 255), a);
}

// Загрузка шрифта с несколькими запасными путями (сначала assets, затем системные)
inline bool loadFont(sf::Font& font) {
    const char* paths[] = {
        "assets/fonts/main.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    };
    for (auto p : paths)
        if (font.openFromFile(p)) return true;
    return false;
}
