#include "Game.h"
#include "Tetromino.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

using Key = sf::Keyboard::Key;

// ---- Тайминги ----
static constexpr float CLEAR_DUR = 0.35f;  // длительность анимации очистки
static constexpr float DAS_DELAY = 0.16f;  // задержка до авто-повтора
static constexpr float DAS_REPEAT = 0.045f;// период авто-повтора

// ---- Геометрия кнопок меню ----
static constexpr float BTN_W = 320.f, BTN_H = 58.f, BTN_GAP = 18.f;
static const float BTN_X = (WINDOW_W - BTN_W) / 2.f;

static float gravityForLevel(int l) {
    return std::max(0.05f, 0.80f - (l - 1) * 0.065f);
}
static int targetForLevel(int n) {
    return 500 * n + 100 * n * (n - 1); // 500, 1200, 2100, 3200, ...
}

// Скруглённый прямоугольник для панелей
static void roundRect(sf::RenderTarget& t, float x, float y, float w, float h,
                      float r, sf::Color col, sf::Color outline = sf::Color::Transparent,
                      float outThick = 0.f) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    sf::RectangleShape rh({w - 2 * r, h});
    rh.setPosition({x + r, y}); rh.setFillColor(col); t.draw(rh);
    sf::RectangleShape rv({w, h - 2 * r});
    rv.setPosition({x, y + r}); rv.setFillColor(col); t.draw(rv);
    float p[4][2] = {{x + r, y + r}, {x + w - r, y + r},
                     {x + r, y + h - r}, {x + w - r, y + h - r}};
    for (auto& pt : p) {
        sf::CircleShape c(r); c.setOrigin({r, r});
        c.setPosition({pt[0], pt[1]}); c.setFillColor(col); t.draw(c);
    }
    if (outThick > 0.f) {
        sf::RectangleShape o({w, h}); o.setPosition({x, y});
        o.setFillColor(sf::Color::Transparent);
        o.setOutlineThickness(-outThick); o.setOutlineColor(outline);
        t.draw(o);
    }
}

// Какая кнопка вертикального меню под курсором (или -1)
static int menuHit(sf::RenderWindow& w, float firstY, int count) {
    auto m = sf::Mouse::getPosition(w);
    for (int i = 0; i < count; i++) {
        float y = firstY + i * (BTN_H + BTN_GAP);
        if (m.x >= BTN_X && m.x <= BTN_X + BTN_W && m.y >= y && m.y <= y + BTN_H)
            return i;
    }
    return -1;
}

Game::Game()
    : window(sf::VideoMode(sf::Vector2u(WINDOW_W, WINDOW_H)), "Tetris Deluxe",
             sf::Style::Titlebar | sf::Style::Close),
      rng(std::random_device{}()) {
    window.setFramerateLimit(60);
    loadFont(font); // если шрифт не найден — игра всё равно запустится
    profile.load();
    nextType = std::uniform_int_distribution<int>(0, 6)(rng);
}

void Game::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f; // защита от скачков
        handleEvents();
        update(dt);
        render();
    }
}

// ---------- Игровая логика ----------

void Game::newGame(Mode m, int startLevel) {
    mode = m;
    board.reset();
    score = 0; lines = 0;
    gameLevel = startLevel;
    gravity = gravityForLevel(mode == Mode::Levels ? gameLevel : 1);
    gravityTimer = 0.f;
    targetScore = (mode == Mode::Levels) ? targetForLevel(gameLevel) : 0;
    particles.clear(); popups.clear(); clearingRows.clear();
    clearTimer = 0.f; heldDir = 0; softDropHeld = 0;
    nextType = std::uniform_int_distribution<int>(0, 6)(rng);
    spawnPiece();
    state = State::Playing;
}

void Game::spawnPiece() {
    curType = nextType;
    nextType = std::uniform_int_distribution<int>(0, 6)(rng);
    curRot = 0; curX = 3; curY = 0;
    if (!board.valid(curType, curRot, curX, curY)) {
        if (mode == Mode::Classic && score > profile.highScoreClassic)
            profile.highScoreClassic = score;
        profile.save();
        state = State::GameOver;
    }
}

int Game::ghostY() const {
    int y = curY;
    while (board.valid(curType, curRot, curX, y + 1)) y++;
    return y;
}

void Game::hardDrop() {
    while (board.valid(curType, curRot, curX, curY + 1)) {
        curY++; score += 2;
    }
    lockPiece();
}

void Game::lockPiece() {
    board.lock(curType, curRot, curX, curY);
    clearingRows = board.fullRows();
    if (clearingRows.empty()) {
        spawnPiece();
    } else {
        clearTimer = CLEAR_DUR;
        shake = 8.f;
        for (int r : clearingRows) spawnBurst(r);
    }
}

void Game::finishClear() {
    int n = (int)clearingRows.size();
    board.removeRows(clearingRows);
    clearingRows.clear();
    clearTimer = 0.f;
    addReward(n);
    if (mode == Mode::Classic)
        gravity = gravityForLevel(1 + lines / 10);
    if (mode == Mode::Levels && score >= targetScore) {
        int bonusC = gameLevel * 50, bonusX = gameLevel * 30;
        profile.addCoins(bonusC);
        int lv = profile.addXP(bonusX);
        if (gameLevel + 1 > profile.levelsUnlocked)
            profile.levelsUnlocked = gameLevel + 1;
        profile.save();
        if (lv > 0) { levelsGainedFlash = lv; flashTimer = 2.f; }
        state = State::LevelComplete;
        return;
    }
    spawnPiece();
}

void Game::addReward(int cleared) {
    static const int lineScore[5] = {0, 100, 300, 500, 800};
    int lvl = (mode == Mode::Levels) ? gameLevel : (1 + lines / 10);
    score += lineScore[cleared] * lvl;
    lines += cleared;

    int coinGain = cleared * 4 + (cleared == 4 ? 10 : 0);
    int xpGain = cleared * 8 + (cleared == 4 ? 12 : 0);
    profile.addCoins(coinGain);
    int lv = profile.addXP(xpGain);

    addPopup("+" + std::to_string(coinGain) + " монет", sf::Color(255, 210, 60));
    addPopup("+" + std::to_string(xpGain) + " XP", sf::Color(120, 220, 255));
    if (lv > 0) {
        addPopup("Новый уровень!", sf::Color(180, 255, 140));
        levelsGainedFlash = lv; flashTimer = 2.f;
    }
}

void Game::spawnBurst(int row) {
    std::uniform_real_distribution<float> ang(0, 6.283f), sp(40, 260);
    for (int c = 0; c < COLS; c++) {
        int type = board.grid[row][c];
        sf::Color col = (type >= 0) ? PIECE_COLORS[type] : sf::Color::White;
        for (int k = 0; k < 3; k++) {
            float a = ang(rng), v = sp(rng);
            Particle p;
            p.pos = {BOARD_X + c * CELL + CELL / 2.f, BOARD_Y + row * CELL + CELL / 2.f};
            p.vel = {std::cos(a) * v, std::sin(a) * v - 80};
            p.color = col;
            p.maxLife = p.life = 0.5f + (k * 0.1f);
            p.size = 3 + (k % 3);
            particles.push_back(p);
        }
    }
}

void Game::addPopup(const std::string& s, sf::Color col) {
    Popup p;
    p.pos = {BOARD_X + BOARD_W / 2.f, BOARD_Y + 120.f + popups.size() * 26.f};
    p.text = s; p.color = col; p.life = 1.4f;
    popups.push_back(p);
}

// ---------- Ввод ----------

void Game::handleEvents() {
    while (const std::optional<sf::Event> ev = window.pollEvent()) {
        if (ev->is<sf::Event::Closed>()) { profile.save(); window.close(); }

        if (const auto* kp = ev->getIf<sf::Event::KeyPressed>()) {
            Key k = kp->code;
            switch (state) {
            case State::Menu:
                if (k == Key::Up)   menuIndex = (menuIndex + 3) % 4;
                if (k == Key::Down) menuIndex = (menuIndex + 1) % 4;
                if (k == Key::Enter || k == Key::Space) {
                    if (menuIndex == 0) { state = State::ModeSelect; menuIndex = 0; }
                    else if (menuIndex == 1) { state = State::Shop; shopIndex = 0; }
                    else if (menuIndex == 2) { state = State::Profile; }
                    else { profile.save(); window.close(); }
                }
                if (k == Key::Escape) { profile.save(); window.close(); }
                break;

            case State::ModeSelect:
                if (k == Key::Up)   menuIndex = (menuIndex + 2) % 3;
                if (k == Key::Down) menuIndex = (menuIndex + 1) % 3;
                if (k == Key::Enter || k == Key::Space) {
                    if (menuIndex == 0) newGame(Mode::Classic, 1);
                    else if (menuIndex == 1) newGame(Mode::Levels, 1);
                    else { state = State::Menu; menuIndex = 0; }
                }
                if (k == Key::Escape) { state = State::Menu; menuIndex = 0; }
                break;

            case State::Playing:
                if (clearTimer <= 0) {
                    if (k == Key::Left || k == Key::A) {
                        heldDir = -1; dasTimer = DAS_DELAY;
                        if (board.valid(curType, curRot, curX - 1, curY)) curX--;
                    }
                    if (k == Key::Right || k == Key::D) {
                        heldDir = 1; dasTimer = DAS_DELAY;
                        if (board.valid(curType, curRot, curX + 1, curY)) curX++;
                    }
                    if (k == Key::Up || k == Key::W || k == Key::X) {
                        int nr = (curRot + 1) & 3;
                        int kick[5] = {0, -1, 1, -2, 2};
                        for (int kk : kick)
                            if (board.valid(curType, nr, curX + kk, curY)) { curRot = nr; curX += kk; break; }
                    }
                    if (k == Key::Z || k == Key::LControl) {
                        int nr = (curRot + 3) & 3;
                        int kick[5] = {0, -1, 1, -2, 2};
                        for (int kk : kick)
                            if (board.valid(curType, nr, curX + kk, curY)) { curRot = nr; curX += kk; break; }
                    }
                    if (k == Key::Space) hardDrop();
                }
                if (k == Key::Escape || k == Key::P) state = State::Paused;
                break;

            case State::Paused:
                if (k == Key::Escape || k == Key::P) state = State::Playing;
                if (k == Key::M) { profile.save(); state = State::Menu; menuIndex = 0; }
                break;

            case State::GameOver:
                if (k == Key::Enter) {
                    if (mode == Mode::Classic) newGame(Mode::Classic, 1);
                    else newGame(Mode::Levels, gameLevel);
                }
                if (k == Key::Escape || k == Key::M) { state = State::Menu; menuIndex = 0; }
                break;

            case State::LevelComplete:
                if (k == Key::Enter) newGame(Mode::Levels, gameLevel + 1);
                if (k == Key::Escape || k == Key::M) { state = State::Menu; menuIndex = 0; }
                break;

            case State::Shop: {
                int cols = 2, count = (int)allSkins().size();
                if (k == Key::Left)  shopIndex = (shopIndex + count - 1) % count;
                if (k == Key::Right) shopIndex = (shopIndex + 1) % count;
                if (k == Key::Up)    shopIndex = (shopIndex + count - cols) % count;
                if (k == Key::Down)  shopIndex = (shopIndex + cols) % count;
                if (k == Key::Enter || k == Key::Space) {
                    const Skin& sk = allSkins()[shopIndex];
                    if (!profile.own(sk.id)) {
                        if (profile.coins >= sk.price) {
                            profile.coins -= sk.price;
                            profile.owned.insert(sk.id);
                            profile.equippedSkin = sk.id;
                            profile.save();
                        }
                    } else {
                        profile.equippedSkin = sk.id;
                        profile.save();
                    }
                }
                if (k == Key::Escape) { state = State::Menu; menuIndex = 0; }
                break;
            }

            case State::Profile:
                if (k == Key::Escape || k == Key::Enter) { state = State::Menu; menuIndex = 0; }
                break;
            }
        }

        if (const auto* kr = ev->getIf<sf::Event::KeyReleased>()) {
            Key k = kr->code;
            if ((k == Key::Left || k == Key::A) && heldDir == -1) heldDir = 0;
            if ((k == Key::Right || k == Key::D) && heldDir == 1) heldDir = 0;
        }

        // Мышь для вертикальных меню
        if (state == State::Menu) {
            if (ev->is<sf::Event::MouseMoved>()) {
                int h = menuHit(window, 300.f, 4);
                if (h >= 0) menuIndex = h;
            }
            if (ev->is<sf::Event::MouseButtonReleased>()) {
                int h = menuHit(window, 300.f, 4);
                if (h >= 0) {
                    menuIndex = h;
                    if (h == 0) { state = State::ModeSelect; menuIndex = 0; }
                    else if (h == 1) { state = State::Shop; shopIndex = 0; }
                    else if (h == 2) state = State::Profile;
                    else { profile.save(); window.close(); }
                }
            }
        } else if (state == State::ModeSelect) {
            if (ev->is<sf::Event::MouseMoved>()) {
                int h = menuHit(window, 320.f, 3);
                if (h >= 0) menuIndex = h;
            }
            if (ev->is<sf::Event::MouseButtonReleased>()) {
                int h = menuHit(window, 320.f, 3);
                if (h == 0) newGame(Mode::Classic, 1);
                else if (h == 1) newGame(Mode::Levels, 1);
                else if (h == 2) { state = State::Menu; menuIndex = 0; }
            }
        }
    }
}

// ---------- Обновление ----------

void Game::update(float dt) {
    animTime += dt;
    if (shake > 0) shake = std::max(0.f, shake - dt * 30.f);
    if (flashTimer > 0) flashTimer -= dt;

    for (auto& p : particles) {
        p.pos += p.vel * dt;
        p.vel.y += 500 * dt;
        p.life -= dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                    [](const Particle& p){ return p.life <= 0; }), particles.end());
    for (auto& p : popups) { p.pos.y -= 34 * dt; p.life -= dt; }
    popups.erase(std::remove_if(popups.begin(), popups.end(),
                 [](const Popup& p){ return p.life <= 0; }), popups.end());

    if (state == State::Playing) {
        if (clearTimer > 0) {
            clearTimer -= dt;
            if (clearTimer <= 0) finishClear();
            return;
        }
        softDropHeld = (sf::Keyboard::isKeyPressed(Key::Down) ||
                        sf::Keyboard::isKeyPressed(Key::S)) ? 1 : 0;
        if (heldDir != 0) {
            dasTimer -= dt;
            if (dasTimer <= 0) {
                if (board.valid(curType, curRot, curX + heldDir, curY)) curX += heldDir;
                dasTimer = DAS_REPEAT;
            }
        }
        gravityTimer += dt;
        float iv = softDropHeld ? 0.04f : gravity;
        while (gravityTimer >= iv && state == State::Playing && clearTimer <= 0) {
            gravityTimer -= iv;
            if (board.valid(curType, curRot, curX, curY + 1)) {
                curY++;
                if (softDropHeld) score += 1;
            } else {
                lockPiece();
                break;
            }
        }
    }
}

// ---------- Отрисовка ----------

void Game::text(const std::string& s, float x, float y, unsigned size,
                sf::Color col, bool center, bool bold) {
    sf::Text t(font, u(s), size);
    t.setFillColor(col);
    if (bold) t.setStyle(sf::Text::Bold);
    if (center) {
        auto b = t.getLocalBounds();
        t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
    }
    t.setPosition({x, y});
    window.draw(t);
}

void Game::panel(float x, float y, float w, float h, sf::Color col, float radius) {
    roundRect(window, x, y, w, h, radius, col, sf::Color(255, 255, 255, 30), 1.5f);
}

bool Game::button(const std::string& label, float x, float y, float w, float h,
                  bool selected) {
    sf::Color base = selected ? sf::Color(70, 90, 180) : sf::Color(40, 44, 70);
    sf::Color out = selected ? sf::Color(140, 180, 255) : sf::Color(80, 84, 120);
    roundRect(window, x, y, w, h, 12.f, base, out, selected ? 3.f : 1.5f);
    text(label, x + w / 2.f, y + h / 2.f, 26, sf::Color::White, true, selected);
    return false;
}

void Game::drawBackground() {
    float t = animTime * 0.15f;
    sf::Color c1 = mix(sf::Color(18, 16, 40), sf::Color(30, 20, 55),
                       0.5f + 0.5f * std::sin(t));
    sf::Color c2 = mix(sf::Color(10, 12, 28), sf::Color(24, 14, 44),
                       0.5f + 0.5f * std::cos(t * 0.8f));
    sf::VertexArray bg(sf::PrimitiveType::TriangleFan, 4);
    bg[0].position = {0, 0};                        bg[0].color = c1;
    bg[1].position = {(float)WINDOW_W, 0};          bg[1].color = mix(c1, c2, 0.4f);
    bg[2].position = {(float)WINDOW_W, (float)WINDOW_H}; bg[2].color = c2;
    bg[3].position = {0, (float)WINDOW_H};          bg[3].color = mix(c2, c1, 0.4f);
    window.draw(bg);

    for (int i = 0; i < 9; i++) {
        float px = std::fmod(i * 127 + 40, (float)WINDOW_W);
        float py = std::fmod(animTime * (18 + i * 6) + i * 90, (float)WINDOW_H + 60) - 30;
        float sz = 20 + (i % 3) * 12;
        sf::RectangleShape r({sz, sz});
        r.setOrigin({sz / 2, sz / 2});
        r.setPosition({px, py});
        r.setFillColor(sf::Color(255, 255, 255, 8));
        r.setRotation(sf::degrees(animTime * 20 + i * 30));
        window.draw(r);
    }
}

void Game::drawPlayfield() {
    float ox = 0, oy = 0;
    if (shake > 0) {
        std::uniform_real_distribution<float> d(-shake, shake);
        ox = d(rng); oy = d(rng);
    }
    roundRect(window, BOARD_X - 6 + ox, BOARD_Y - 6 + oy, BOARD_W + 12, BOARD_H + 12,
              10.f, sf::Color(12, 12, 26), sf::Color(90, 100, 180), 2.f);

    for (int c = 0; c <= COLS; c++) {
        sf::RectangleShape line({1.f, (float)BOARD_H});
        line.setPosition({BOARD_X + c * CELL + ox, BOARD_Y + oy});
        line.setFillColor(sf::Color(255, 255, 255, 12));
        window.draw(line);
    }
    for (int r = 0; r <= ROWS; r++) {
        sf::RectangleShape line({(float)BOARD_W, 1.f});
        line.setPosition({BOARD_X + ox, BOARD_Y + r * CELL + oy});
        line.setFillColor(sf::Color(255, 255, 255, 12));
        window.draw(line);
    }

    SkinStyle style = skinById(profile.equippedSkin).style;

    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            if (board.grid[r][c] >= 0)
                drawBlock(window, BOARD_X + c * CELL + ox, BOARD_Y + r * CELL + oy,
                          CELL, PIECE_COLORS[board.grid[r][c]], style, animTime, c, r);

    if (clearTimer > 0) {
        float k = clearTimer / CLEAR_DUR;
        std::uint8_t a = (std::uint8_t)(200 * (0.4f + 0.6f * std::sin(k * 20)));
        for (int r : clearingRows) {
            sf::RectangleShape f({(float)BOARD_W, (float)CELL});
            f.setPosition({BOARD_X + ox, BOARD_Y + r * CELL + oy});
            f.setFillColor(sf::Color(255, 255, 255, a));
            window.draw(f);
        }
    }

    if ((state == State::Playing || state == State::Paused) && clearTimer <= 0) {
        int gy = ghostY();
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                if (pieceCell(curType, curRot, r, c)) {
                    sf::RectangleShape gh({(float)CELL - 4, (float)CELL - 4});
                    gh.setPosition({BOARD_X + (curX + c) * CELL + 2 + ox,
                                    BOARD_Y + (gy + r) * CELL + 2 + oy});
                    gh.setFillColor(sf::Color(255, 255, 255, 20));
                    gh.setOutlineThickness(-1.5f);
                    gh.setOutlineColor(mix(PIECE_COLORS[curType], sf::Color::White, 0.3f));
                    window.draw(gh);
                }
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                if (pieceCell(curType, curRot, r, c)) {
                    int py = curY + r;
                    if (py < 0) continue;
                    drawBlock(window, BOARD_X + (curX + c) * CELL + ox,
                              BOARD_Y + py * CELL + oy, CELL,
                              PIECE_COLORS[curType], style, animTime, curX + c, py);
                }
    }
}

void Game::drawSidePanels() {
    SkinStyle style = skinById(profile.equippedSkin).style;

    panel(20, 60, 240, 300, sf::Color(28, 30, 52));
    text(mode == Mode::Classic ? "КЛАССИКА" : "УРОВНИ", 40, 76, 22,
         sf::Color(150, 180, 255), false, true);
    text("Счёт", 40, 118, 16, sf::Color(160, 160, 190));
    text(std::to_string(score), 40, 138, 30, sf::Color::White, false, true);
    text("Линии", 40, 186, 16, sf::Color(160, 160, 190));
    text(std::to_string(lines), 40, 206, 24, sf::Color::White, false, true);

    if (mode == Mode::Levels) {
        text("Уровень " + std::to_string(gameLevel), 40, 250, 18, sf::Color(180, 220, 160));
        text("Цель: " + std::to_string(targetScore), 40, 276, 16, sf::Color(160, 160, 190));
        float prog = std::min(1.f, (float)score / targetScore);
        roundRect(window, 40, 302, 200, 16, 8, sf::Color(20, 20, 36));
        roundRect(window, 40, 302, 200 * prog, 16, 8, sf::Color(120, 220, 140));
    } else {
        text("Рекорд", 40, 250, 16, sf::Color(160, 160, 190));
        text(std::to_string(profile.highScoreClassic), 40, 270, 24,
             sf::Color(255, 210, 90), false, true);
    }

    panel(20, 380, 240, 130, sf::Color(28, 30, 52));
    text("Ур. игрока " + std::to_string(profile.level), 40, 396, 18,
         sf::Color(200, 180, 255), false, true);
    float xpProg = (float)profile.xp / profile.xpForNext();
    roundRect(window, 40, 424, 200, 14, 7, sf::Color(20, 20, 36));
    roundRect(window, 40, 424, 200 * xpProg, 14, 7, sf::Color(150, 120, 240));
    text(std::to_string(profile.xp) + " / " + std::to_string(profile.xpForNext()) + " XP",
         40, 444, 14, sf::Color(170, 170, 200));
    text("Монеты: " + std::to_string(profile.coins), 40, 474, 18,
         sf::Color(255, 210, 60), false, true);

    panel(620, 60, 240, 200, sf::Color(28, 30, 52));
    text("СЛЕДУЮЩАЯ", 740, 82, 18, sf::Color(150, 180, 255), true, true);
    float pvx = 700, pvy = 120, pcell = 26;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (pieceCell(nextType, 0, r, c))
                drawBlock(window, pvx + c * pcell, pvy + r * pcell, pcell,
                          PIECE_COLORS[nextType], style, animTime, c, r);

    panel(620, 280, 240, 230, sf::Color(28, 30, 52));
    text("УПРАВЛЕНИЕ", 740, 300, 18, sf::Color(150, 180, 255), true, true);
    const char* ctrl[] = {
        "< > / A D  — движение",
        "Вверх / W  — поворот",
        "Вниз / S   — ускорить",
        "Пробел     — сброс",
        "Esc / P    — пауза",
    };
    for (int i = 0; i < 5; i++)
        text(ctrl[i], 636, 330 + i * 30, 15, sf::Color(190, 190, 210));
}

void Game::drawMenu() {
    text("TETRIS", WINDOW_W / 2.f, 150, 90, sf::Color(120, 200, 255), true, true);
    text("DELUXE", WINDOW_W / 2.f, 220, 40, sf::Color(180, 150, 255), true, true);
    const char* items[] = {"Играть", "Магазин", "Профиль", "Выход"};
    for (int i = 0; i < 4; i++)
        button(items[i], BTN_X, 300 + i * (BTN_H + BTN_GAP), BTN_W, BTN_H, menuIndex == i);
    text("Монеты: " + std::to_string(profile.coins) + "   Ур. " + std::to_string(profile.level),
         WINDOW_W / 2.f, WINDOW_H - 40, 18, sf::Color(255, 210, 90), true);
}

void Game::drawModeSelect() {
    text("Выбор режима", WINDOW_W / 2.f, 200, 48, sf::Color(150, 200, 255), true, true);
    const char* items[] = {"Классика", "Уровни", "Назад"};
    for (int i = 0; i < 3; i++)
        button(items[i], BTN_X, 320 + i * (BTN_H + BTN_GAP), BTN_W, BTN_H, menuIndex == i);
    text("Классика: играй, пока не проиграешь. Уровни: набери счёт, чтобы пройти.",
         WINDOW_W / 2.f, 560, 16, sf::Color(170, 170, 200), true);
}

void Game::drawShop() {
    text("Магазин скинов", WINDOW_W / 2.f, 50, 40, sf::Color(150, 200, 255), true, true);
    text("Монеты: " + std::to_string(profile.coins), WINDOW_W / 2.f, 90, 22,
         sf::Color(255, 210, 60), true, true);

    const auto& skins = allSkins();
    int cols = 2;
    float cw = 380, ch = 92, gap = 20;
    float startX = (WINDOW_W - (cols * cw + gap)) / 2.f;
    float startY = 130;

    for (int i = 0; i < (int)skins.size(); i++) {
        const Skin& sk = skins[i];
        int col = i % cols, row = i / cols;
        float x = startX + col * (cw + gap);
        float y = startY + row * (ch + gap * 0.7f);
        bool sel = (i == shopIndex);
        bool owned = profile.own(sk.id);
        bool equipped = (profile.equippedSkin == sk.id);

        sf::Color bg = sel ? sf::Color(50, 60, 110) : sf::Color(30, 34, 58);
        sf::Color out = equipped ? sf::Color(120, 255, 160)
                                  : (sel ? sf::Color(150, 190, 255) : sf::Color(70, 74, 110));
        roundRect(window, x, y, cw, ch, 12, bg, out, sel || equipped ? 3.f : 1.5f);

        for (int b = 0; b < 3; b++)
            drawBlock(window, x + 16 + b * 26, y + ch / 2 - 13, 26,
                      PIECE_COLORS[(sk.id + b) % 7], sk.style, animTime, b, i);

        text(sk.name, x + 110, y + 16, 22, sf::Color::White, false, true);
        text(sk.desc, x + 110, y + 46, 14, sf::Color(180, 180, 200));

        std::string status;
        sf::Color scol;
        if (equipped)   { status = "НАДЕТ";  scol = sf::Color(120, 255, 160); }
        else if (owned) { status = "Куплено — Enter"; scol = sf::Color(180, 200, 255); }
        else            { status = std::to_string(sk.price) + " монет"; scol = sf::Color(255, 210, 60); }
        text(status, x + cw - 16, y + ch - 26, 16, scol, false, true);
    }
    text("Стрелки — выбор,  Enter — купить/надеть,  Esc — назад",
         WINDOW_W / 2.f, WINDOW_H - 30, 16, sf::Color(170, 170, 200), true);
}

void Game::drawProfile() {
    text("Профиль", WINDOW_W / 2.f, 90, 48, sf::Color(150, 200, 255), true, true);
    panel(250, 180, 400, 300, sf::Color(28, 30, 52));
    text("Уровень игрока: " + std::to_string(profile.level), 290, 210, 26,
         sf::Color::White, false, true);
    float xpProg = (float)profile.xp / profile.xpForNext();
    roundRect(window, 290, 256, 320, 20, 10, sf::Color(20, 20, 36));
    roundRect(window, 290, 256, 320 * xpProg, 20, 10, sf::Color(150, 120, 240));
    text(std::to_string(profile.xp) + " / " + std::to_string(profile.xpForNext()) + " XP",
         290, 284, 16, sf::Color(190, 190, 210));
    text("Монеты: " + std::to_string(profile.coins), 290, 322, 24,
         sf::Color(255, 210, 60), false, true);
    text("Рекорд (классика): " + std::to_string(profile.highScoreClassic), 290, 360, 20,
         sf::Color(200, 220, 255));
    text("Открыто уровней: " + std::to_string(profile.levelsUnlocked), 290, 392, 20,
         sf::Color(200, 220, 255));
    text("Куплено скинов: " + std::to_string((int)profile.owned.size()) + " / " +
         std::to_string((int)allSkins().size()), 290, 424, 20, sf::Color(200, 220, 255));
    text("Esc — назад", WINDOW_W / 2.f, WINDOW_H - 40, 18, sf::Color(170, 170, 200), true);
}

void Game::drawPause() {
    sf::RectangleShape ov({(float)WINDOW_W, (float)WINDOW_H});
    ov.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(ov);
    text("ПАУЗА", WINDOW_W / 2.f, 300, 60, sf::Color::White, true, true);
    text("Esc / P — продолжить", WINDOW_W / 2.f, 380, 22, sf::Color(200, 200, 220), true);
    text("M — в меню", WINDOW_W / 2.f, 415, 22, sf::Color(200, 200, 220), true);
}

void Game::drawGameOver() {
    sf::RectangleShape ov({(float)WINDOW_W, (float)WINDOW_H});
    ov.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(ov);
    text("Игра окончена", WINDOW_W / 2.f, 260, 54, sf::Color(255, 120, 120), true, true);
    text("Счёт: " + std::to_string(score), WINDOW_W / 2.f, 330, 30, sf::Color::White, true, true);
    if (mode == Mode::Classic)
        text("Рекорд: " + std::to_string(profile.highScoreClassic), WINDOW_W / 2.f, 370, 22,
             sf::Color(255, 210, 90), true);
    text("Enter — заново    Esc — в меню", WINDOW_W / 2.f, 440, 22,
         sf::Color(200, 200, 220), true);
}

void Game::drawLevelComplete() {
    sf::RectangleShape ov({(float)WINDOW_W, (float)WINDOW_H});
    ov.setFillColor(sf::Color(0, 0, 0, 170));
    window.draw(ov);
    text("Уровень " + std::to_string(gameLevel) + " пройден!", WINDOW_W / 2.f, 250, 46,
         sf::Color(140, 255, 160), true, true);
    text("Награда: +" + std::to_string(gameLevel * 50) + " монет,  +" +
         std::to_string(gameLevel * 30) + " XP", WINDOW_W / 2.f, 320, 24,
         sf::Color(255, 220, 120), true);
    text("Enter — следующий уровень (" + std::to_string(gameLevel + 1) + ")",
         WINDOW_W / 2.f, 400, 22, sf::Color(200, 220, 255), true);
    text("Esc — в меню", WINDOW_W / 2.f, 435, 22, sf::Color(200, 200, 220), true);
}

void Game::render() {
    window.clear();
    drawBackground();

    switch (state) {
    case State::Menu:       drawMenu(); break;
    case State::ModeSelect: drawModeSelect(); break;
    case State::Shop:       drawShop(); break;
    case State::Profile:    drawProfile(); break;
    default:
        drawPlayfield();
        drawSidePanels();
        for (auto& p : particles) {
            std::uint8_t a = (std::uint8_t)(255 * (p.life / p.maxLife));
            sf::RectangleShape r({p.size, p.size});
            r.setPosition(p.pos);
            sf::Color c = p.color; c.a = a;
            r.setFillColor(c);
            window.draw(r);
        }
        for (auto& p : popups) {
            sf::Color c = p.color; c.a = (std::uint8_t)(255 * std::min(1.f, p.life));
            text(p.text, p.pos.x, p.pos.y, 22, c, true, true);
        }
        if (state == State::Paused)        drawPause();
        if (state == State::GameOver)      drawGameOver();
        if (state == State::LevelComplete) drawLevelComplete();
        break;
    }
    window.display();
}
