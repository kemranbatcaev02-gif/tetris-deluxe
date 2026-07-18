#include "Skins.h"
#include "Constants.h"
#include <cmath>
#include <cstdint>

// Каталог скинов: от дешёвых и простых к дорогим и красивым
const std::vector<Skin>& allSkins() {
    static const std::vector<Skin> skins = {
        {0, "Классика",  0,   SkinStyle::Solid,    "Стандартные блоки"},
        {1, "Матовый",   30,  SkinStyle::Flat,     "Простой и чистый"},
        {2, "Пиксель",   80,  SkinStyle::Pixel,    "Ретро-фаска 8-бит"},
        {3, "Градиент",  140, SkinStyle::Gradient, "Плавный переход цвета"},
        {4, "Глянец",    220, SkinStyle::Gloss,    "Блестящая поверхность"},
        {5, "Неон",      320, SkinStyle::Neon,     "Светящийся контур"},
        {6, "Радуга",    480, SkinStyle::Rainbow,  "Переливается цветами"},
        {7, "Кристалл",  650, SkinStyle::Crystal,  "Гранёный самоцвет"},
        {8, "Золото",    850, SkinStyle::Gold,     "Металлический блеск"},
        {9, "Космос",    1200,SkinStyle::Galaxy,   "Звёзды и туманности"},
    };
    return skins;
}

const Skin& skinById(int id) {
    for (auto& s : allSkins())
        if (s.id == id) return s;
    return allSkins()[0];
}

// ---- Вспомогательные функции отрисовки ----

// Скруглённый прямоугольник (два прямоугольника + 4 круга в углах)
static void roundedRect(sf::RenderTarget& t, float x, float y, float w, float h,
                        float r, sf::Color col) {
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
}

// Заливка четырёхугольника градиентом (4 вершины, веер треугольников)
static void gradQuad(sf::RenderTarget& t,
                     sf::Vector2f a, sf::Color ca, sf::Vector2f b, sf::Color cb,
                     sf::Vector2f c, sf::Color cc, sf::Vector2f d, sf::Color cd) {
    sf::VertexArray va(sf::PrimitiveType::TriangleFan, 4);
    va[0].position = a; va[0].color = ca;
    va[1].position = b; va[1].color = cb;
    va[2].position = c; va[2].color = cc;
    va[3].position = d; va[3].color = cd;
    t.draw(va);
}

// Псевдослучайное значение 0..1 по координатам (для звёзд)
static float hash01(int a, int b) {
    unsigned h = (unsigned)(a * 73856093) ^ (unsigned)(b * 19349663);
    h = (h ^ (h >> 13)) * 1274126177u;
    return (h & 0xFFFF) / 65535.f;
}

void drawBlock(sf::RenderTarget& target, float x, float y, float size,
               sf::Color base, SkinStyle style, float time,
               int gx, int gy, std::uint8_t alpha) {
    float pad = 1.f;
    float s = size - pad * 2;
    float bx = x + pad, by = y + pad;
    auto A = [&](sf::Color c) { c.a = (std::uint8_t)(c.a * (alpha / 255.f)); return c; };

    switch (style) {
    case SkinStyle::Solid: {
        sf::RectangleShape r({s, s}); r.setPosition({bx, by});
        r.setFillColor(A(base));
        r.setOutlineThickness(-2.f);
        r.setOutlineColor(A(mix(base, sf::Color::Black, 0.4f)));
        target.draw(r);
        sf::RectangleShape hl({s, s * 0.3f}); hl.setPosition({bx, by});
        hl.setFillColor(A(sf::Color(255, 255, 255, 55))); target.draw(hl);
        break;
    }
    case SkinStyle::Flat: {
        sf::RectangleShape r({s, s}); r.setPosition({bx, by});
        r.setFillColor(A(mix(base, sf::Color(30, 30, 40), 0.18f)));
        r.setOutlineThickness(-1.f);
        r.setOutlineColor(A(sf::Color(0, 0, 0, 90)));
        target.draw(r);
        break;
    }
    case SkinStyle::Pixel: {
        sf::RectangleShape r({s, s}); r.setPosition({bx, by});
        r.setFillColor(A(base)); target.draw(r);
        float b = size * 0.16f;
        sf::Color light = mix(base, sf::Color::White, 0.45f);
        sf::Color dark = mix(base, sf::Color::Black, 0.5f);
        sf::RectangleShape top({s, b}); top.setPosition({bx, by});
        top.setFillColor(A(light)); target.draw(top);
        sf::RectangleShape left({b, s}); left.setPosition({bx, by});
        left.setFillColor(A(light)); target.draw(left);
        sf::RectangleShape bot({s, b}); bot.setPosition({bx, by + s - b});
        bot.setFillColor(A(dark)); target.draw(bot);
        sf::RectangleShape right({b, s}); right.setPosition({bx + s - b, by});
        right.setFillColor(A(dark)); target.draw(right);
        break;
    }
    case SkinStyle::Gradient: {
        sf::Color top = mix(base, sf::Color::White, 0.4f);
        sf::Color bot = mix(base, sf::Color::Black, 0.45f);
        gradQuad(target, {bx, by}, A(top), {bx + s, by}, A(top),
                 {bx + s, by + s}, A(bot), {bx, by + s}, A(bot));
        break;
    }
    case SkinStyle::Gloss: {
        sf::Color top = mix(base, sf::Color::White, 0.3f);
        sf::Color bot = mix(base, sf::Color::Black, 0.4f);
        gradQuad(target, {bx, by}, A(top), {bx + s, by}, A(top),
                 {bx + s, by + s}, A(bot), {bx, by + s}, A(bot));
        sf::CircleShape g(s * 0.5f); g.setScale({1.f, 0.5f});
        g.setOrigin({s * 0.5f, s * 0.5f});
        g.setPosition({bx + s * 0.5f, by + s * 0.28f});
        g.setFillColor(A(sf::Color(255, 255, 255, 90))); target.draw(g);
        break;
    }
    case SkinStyle::Neon: {
        roundedRect(target, bx, by, s, s, s * 0.22f,
                    A(mix(base, sf::Color::Black, 0.75f)));
        sf::RectangleShape glow({s, s}); glow.setPosition({bx, by});
        glow.setFillColor(sf::Color::Transparent);
        glow.setOutlineThickness(-2.5f);
        sf::Color bright = mix(base, sf::Color::White, 0.35f);
        glow.setOutlineColor(A(bright));
        target.draw(glow);
        sf::RectangleShape inner({s * 0.5f, s * 0.5f});
        inner.setPosition({bx + s * 0.25f, by + s * 0.25f});
        inner.setFillColor(A(sf::Color(bright.r, bright.g, bright.b, 60)));
        target.draw(inner);
        break;
    }
    case SkinStyle::Rainbow: {
        float hue = std::fmod(gx * 24.f + gy * 12.f + time * 70.f, 360.f);
        sf::Color c1 = hsv(hue, 0.85f, 1.0f);
        sf::Color c2 = hsv(hue + 40.f, 0.9f, 0.75f);
        gradQuad(target, {bx, by}, A(c1), {bx + s, by}, A(hsv(hue + 20, 0.85f, 1.f)),
                 {bx + s, by + s}, A(c2), {bx, by + s}, A(c2));
        sf::RectangleShape hl({s, s * 0.25f}); hl.setPosition({bx, by});
        hl.setFillColor(A(sf::Color(255, 255, 255, 70))); target.draw(hl);
        break;
    }
    case SkinStyle::Crystal: {
        sf::Color c = mix(base, sf::Color(120, 180, 255), 0.25f);
        roundedRect(target, bx, by, s, s, s * 0.18f, A(mix(c, sf::Color::Black, 0.3f)));
        sf::VertexArray tl(sf::PrimitiveType::Triangles, 3);
        tl[0].position = {bx, by}; tl[1].position = {bx + s, by};
        tl[2].position = {bx + s * 0.5f, by + s * 0.5f};
        for (int i = 0; i < 3; i++) tl[i].color = A(mix(c, sf::Color::White, 0.55f));
        target.draw(tl);
        sf::VertexArray br(sf::PrimitiveType::Triangles, 3);
        br[0].position = {bx + s, by + s}; br[1].position = {bx, by + s};
        br[2].position = {bx + s * 0.5f, by + s * 0.5f};
        for (int i = 0; i < 3; i++) br[i].color = A(mix(c, sf::Color::Black, 0.35f));
        target.draw(br);
        sf::CircleShape sp(s * 0.09f);
        sp.setPosition({bx + s * 0.62f, by + s * 0.18f});
        sp.setFillColor(A(sf::Color(255, 255, 255, 200))); target.draw(sp);
        break;
    }
    case SkinStyle::Gold: {
        sf::Color top(255, 240, 170), bot(150, 100, 20);
        gradQuad(target, {bx, by}, A(top), {bx + s, by}, A(sf::Color(230, 190, 90)),
                 {bx + s, by + s}, A(bot), {bx, by + s}, A(mix(bot, top, 0.2f)));
        sf::VertexArray shine(sf::PrimitiveType::TriangleFan, 4);
        shine[0].position = {bx, by + s * 0.25f};
        shine[1].position = {bx + s * 0.25f, by};
        shine[2].position = {bx + s * 0.55f, by};
        shine[3].position = {bx, by + s * 0.55f};
        for (int i = 0; i < 4; i++) shine[i].color = A(sf::Color(255, 255, 240, 130));
        target.draw(shine);
        sf::RectangleShape fr({s, s}); fr.setPosition({bx, by});
        fr.setFillColor(sf::Color::Transparent); fr.setOutlineThickness(-1.5f);
        fr.setOutlineColor(A(sf::Color(120, 80, 10))); target.draw(fr);
        break;
    }
    case SkinStyle::Galaxy: {
        roundedRect(target, bx, by, s, s, s * 0.2f, A(sf::Color(20, 12, 45)));
        sf::CircleShape neb(s * 0.4f);
        neb.setOrigin({s * 0.4f, s * 0.4f});
        neb.setPosition({bx + s * 0.35f, by + s * 0.4f});
        neb.setFillColor(A(sf::Color(90, 40, 140, 120))); target.draw(neb);
        for (int i = 0; i < 4; i++) {
            float sx = hash01(gx * 4 + i, gy);
            float sy = hash01(gy * 4 + i, gx);
            float tw = 0.5f + 0.5f * std::sin(time * 3.f + i + gx);
            sf::CircleShape star(1.4f);
            star.setPosition({bx + sx * s, by + sy * s});
            star.setFillColor(A(sf::Color(255, 255, 255, (std::uint8_t)(150 + 100 * tw))));
            target.draw(star);
        }
        break;
    }
    }
}
