#pragma once
#include <vector>
#include "Constants.h"

// Игровое поле 10x20. В ячейке хранится тип фигуры (0..6) или -1 (пусто).
class Board {
public:
    int grid[ROWS][COLS];

    Board();
    void reset();

    // Можно ли разместить фигуру type/rot в позиции (px, py)
    bool valid(int type, int rot, int px, int py) const;
    // Зафиксировать фигуру на поле
    void lock(int type, int rot, int px, int py);
    // Список полностью заполненных рядов
    std::vector<int> fullRows() const;
    // Удалить ряды и сдвинуть всё сверху вниз
    void removeRows(const std::vector<int>& rows);
};
