#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include "Board.h"
#include "SaveData.h"
#include "Skins.h"

// Экраны игры
enum class State { Menu, ModeSelect, Playing, Paused, GameOver, LevelComplete, Shop, Profile };
// Режимы игры
enum class Mode { Classic, Levels };

// Частица для эффектов очистки линий
struct Particle {
    sf::Vector2f pos, vel;
    sf::Color color;
    float life, maxLife, size;
};

// Всплывающий текст ("+50 XP", "+20 монет")
struct Popup {
    sf::Vector2f pos;
    std::string text;
    sf::Color color;
    float life;
};

class Game {
public:
    Game();
    void run();

private:
    sf::RenderWindow window;
    sf::Font font;
    Board board;
    Profile profile;
    std::mt19937 rng;

    State state = State::Menu;
    Mode mode = Mode::Classic;

    // --- Текущая фигура ---
    int curType, curRot, curX, curY, nextType;
    // --- Прогресс партии ---
    int score, lines, gameLevel, targetScore;
    float gravity, gravityTimer;
    int softDropHeld = 0;

    // --- Управление (авто-повтор влево/вправо) ---
    int heldDir = 0;
    float dasTimer = 0.f;

    // --- Анимации ---
    std::vector<Particle> particles;
    std::vector<Popup> popups;
    std::vector<int> clearingRows;
    float clearTimer = 0.f;
    float animTime = 0.f;
    float shake = 0.f;
    int levelsGainedFlash = 0;
    float flashTimer = 0.f;

    // --- Навигация по меню ---
    int menuIndex = 0;
    int shopIndex = 0;

    // ---- Методы ----
    void newGame(Mode m, int startLevel);
    void spawnPiece();
    void hardDrop();
    void lockPiece();
    void checkClears();
    void finishClear();
    int ghostY() const;

    void addReward(int cleared);
    void handleEvents();
    void update(float dt);
    void render();

    // Отрисовка экранов
    void drawBackground();
    void drawPlayfield();
    void drawSidePanels();
    void drawMenu();
    void drawModeSelect();
    void drawShop();
    void drawProfile();
    void drawPause();
    void drawGameOver();
    void drawLevelComplete();

    // Утилиты рисования
    void text(const std::string& s, float x, float y, unsigned size,
              sf::Color col, bool center = false, bool bold = false);
    void panel(float x, float y, float w, float h, sf::Color col, float radius = 12.f);
    bool button(const std::string& label, float x, float y, float w, float h,
                bool selected);
    void spawnBurst(int row);
    void addPopup(const std::string& s, sf::Color col);
};
