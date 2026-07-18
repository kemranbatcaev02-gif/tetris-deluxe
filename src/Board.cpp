#include "Board.h"
#include "Tetromino.h"

Board::Board() { reset(); }

void Board::reset() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            grid[r][c] = -1;
}

bool Board::valid(int type, int rot, int px, int py) const {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            if (!pieceCell(type, rot, r, c)) continue;
            int x = px + c, y = py + r;
            if (x < 0 || x >= COLS || y >= ROWS) return false; // за стенами/полом
            if (y >= 0 && grid[y][x] != -1) return false;      // столкновение
        }
    return true;
}

void Board::lock(int type, int rot, int px, int py) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            if (!pieceCell(type, rot, r, c)) continue;
            int x = px + c, y = py + r;
            if (y >= 0 && y < ROWS && x >= 0 && x < COLS) grid[y][x] = type;
        }
}

std::vector<int> Board::fullRows() const {
    std::vector<int> rows;
    for (int r = 0; r < ROWS; r++) {
        bool full = true;
        for (int c = 0; c < COLS; c++)
            if (grid[r][c] == -1) { full = false; break; }
        if (full) rows.push_back(r);
    }
    return rows;
}

void Board::removeRows(const std::vector<int>& rows) {
    // Удаляем по одному ряду сверху вниз, сдвигая всё, что выше
    for (int rr : rows) {
        for (int r = rr; r > 0; r--)
            for (int c = 0; c < COLS; c++)
                grid[r][c] = grid[r - 1][c];
        for (int c = 0; c < COLS; c++) grid[0][c] = -1;
    }
}
