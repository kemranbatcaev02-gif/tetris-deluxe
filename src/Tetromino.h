#pragma once
#include <SFML/Graphics.hpp>
#include <array>

// 7 фигур, у каждой 4 состояния поворота, закодированы битами в сетке 4x4.
// Клетка (row, col) занята, если (mask & (0x8000 >> (row*4 + col))) != 0.
inline const std::array<std::array<unsigned short, 4>, 7> TETROMINOES = {{
    {0x0F00, 0x2222, 0x00F0, 0x4444}, // I
    {0x6600, 0x6600, 0x6600, 0x6600}, // O
    {0x4E00, 0x4640, 0x0E40, 0x4C40}, // T
    {0x6C00, 0x4620, 0x06C0, 0x8C40}, // S
    {0xC600, 0x2640, 0x0C60, 0x4C80}, // Z
    {0x8E00, 0x6440, 0x0E20, 0x44C0}, // J
    {0x2E00, 0x4460, 0x0E80, 0xC440}, // L
}};

// Базовые цвета фигур
inline const std::array<sf::Color, 7> PIECE_COLORS = {{
    sf::Color(0, 240, 240),   // I - голубой
    sf::Color(240, 216, 0),   // O - жёлтый
    sf::Color(176, 0, 240),   // T - фиолетовый
    sf::Color(0, 224, 72),    // S - зелёный
    sf::Color(240, 32, 48),   // Z - красный
    sf::Color(0, 96, 240),    // J - синий
    sf::Color(240, 144, 0),   // L - оранжевый
}};

// Занята ли клетка (r, c) у фигуры type в повороте rot
inline bool pieceCell(int type, int rot, int r, int c) {
    unsigned short m = TETROMINOES[type][rot & 3];
    int i = r * 4 + c;
    return (m & (0x8000 >> i)) != 0;
}
