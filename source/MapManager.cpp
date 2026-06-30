#include "../include/MapManager.hpp"
#include <cmath>
#include <cstdlib>

// ─── Carregar nível ───────────────────────────────────────────────────────────
void MapManager::loadLevel(GameMap& map, int level) {
    map.dungeonLevel = level;
    map.width  = MAP_W;
    map.height = MAP_H;

    // Começa tudo como parede não explorada
    Tile wall; wall.type = TileType::Wall; wall.solid = true; wall.texIndex = TEX_WALL;
    map.tiles.assign(map.height, std::vector<Tile>(map.width, wall));

    map.items.clear();
    map.enemies.clear();
    map.npcs.clear();
    map.traps.clear();

    buildDemoLevel(map);
}

// ─── Construção do mapa ───────────────────────────────────────────────────────
// Mapa fixo com salas e corredores.  Pode ser trocado por geração procedural.
void MapManager::buildDemoLevel(GameMap& map) {

    // Helpers para "escavar" floor num retângulo ou corredor
    auto carveRoom = [&](int x, int y, int w, int h) {
        for (int ry = y; ry < y + h && ry < map.height; ++ry)
            for (int rx = x; rx < x + w && rx < map.width; ++rx) {
                map.tiles[ry][rx] = {TileType::Floor, false, false, false, TEX_FLOOR};
            }
    };

    auto corridorH = [&](int x1, int x2, int y) {
        for (int x = std::min(x1,x2); x <= std::max(x1,x2) && x < map.width; ++x)
            map.tiles[y][x] = {TileType::Floor, false, false, false, TEX_FLOOR};
    };

    auto corridorV = [&](int x, int y1, int y2) {
        for (int y = std::min(y1,y2); y <= std::max(y1,y2) && y < map.height; ++y)
            map.tiles[y][x] = {TileType::Floor, false, false, false, TEX_FLOOR};
    };

    // Salas
    carveRoom( 1,  1, 10,  8);   // inicial
    carveRoom(15,  1, 10,  6);   // sala 2
    carveRoom(28,  1,  8,  8);   // sala 3
    carveRoom( 1, 14, 12,  8);   // sala 4
    carveRoom(16, 12, 14,  9);   // central
    carveRoom(28, 18,  8,  8);   // boss

    // Corredores
    corridorH(10, 15,  4);
    corridorH(24, 28,  4);
    corridorV( 5,  8, 14);
    corridorH(12, 16, 17);
    corridorH(29, 28, 15);
    corridorV(22,  6, 12);

    // Escadas e início
    map.playerStart = {3, 3};
    map.stairsPos   = {32, 22};
    map.tiles[22][32] = {TileType::StairsDown, false, false, false, TEX_STAIRS_DOWN};

    // Porta trancada
    map.tiles[15][12] = {TileType::Door, true, false, false, TEX_DOOR_CLOSED};

    // Armadilhas
    map.traps.push_back({{20, 15}, 8});
    map.traps.push_back({{22, 17}, 5});

    // Itens
    auto addItem = [&](ItemType t, const std::string& n, int val,
                        int uses, sf::Vector2i pos, int tex) {
        map.items.push_back({t, n, val, uses, true, pos, tex});
    };
    addItem(ItemType::HealthPotion, "Pocao de Vida",  15,  1, {6,  3}, TEX_POTION);
    addItem(ItemType::Key,          "Chave",           0,  1, {18, 3}, TEX_KEY);
    addItem(ItemType::Weapon,       "Espada Curta",    5, -1, {30, 3}, TEX_WEAPON);
    addItem(ItemType::Shield,       "Escudo",          3, -1, {20,13}, TEX_SHIELD);

    // Inimigos
    auto addEnemy = [&](EnemyType et, int gx, int gy,
                         int hp, int str, float delay, int tex, int xp) {
        Enemy e;
        e.type = et;
        e.stats.hp = e.stats.hpMax = hp;
        e.stats.str = str;
        e.gridPos = {gx, gy};
        e.pos     = toPixel(e.gridPos);
        e.moveDelay = delay;
        e.texIndex  = tex;
        e.xpReward  = xp;
        map.enemies.push_back(e);
    };
    addEnemy(EnemyType::Goblin,   18,  3, 10,  3, 1.2f, TEX_GOBLIN,    5);
    addEnemy(EnemyType::Goblin,   20,  3, 10,  3, 1.2f, TEX_GOBLIN,    5);
    addEnemy(EnemyType::Skeleton, 20, 16, 18,  5, 1.0f, TEX_SKELETON, 10);
    addEnemy(EnemyType::Troll,    20, 17, 30,  8, 1.5f, TEX_TROLL,    20);
    addEnemy(EnemyType::Boss,     31, 22, 80, 15, 0.8f, TEX_BOSS,    100);

    // NPC
    map.npcs.push_back({
        "Guarda",
        "Cuidado viajante! Monstros assustadores habitam estas masmorras.",
        {4, 5}, false, TEX_NPC
    });
}

// ─── FOV (raycasting por Bresenham) ──────────────────────────────────────────
void MapManager::updateFOV(GameMap& map, sf::Vector2i p) {
    for (auto& row : map.tiles)
        for (auto& t : row)
            t.visible = false;

    for (int dy = -FOV_RADIUS; dy <= FOV_RADIUS; ++dy)
        for (int dx = -FOV_RADIUS; dx <= FOV_RADIUS; ++dx)
            if (dx*dx + dy*dy <= FOV_RADIUS * FOV_RADIUS)
                castRay(map, p, {p.x + dx, p.y + dy}, FOV_RADIUS);
}

void MapManager::castRay(GameMap& map, sf::Vector2i origin,
                          sf::Vector2i target, int /*radius*/) {
    int x = origin.x, y = origin.y;
    int dx = std::abs(target.x - x), dy = -std::abs(target.y - y);
    int sx = x < target.x ? 1 : -1;
    int sy = y < target.y ? 1 : -1;
    int err = dx + dy;

    while (inBounds(map, x, y)) {
        map.tiles[y][x].visible  = true;
        map.tiles[y][x].explored = true;
        if (map.tiles[y][x].solid || (x == target.x && y == target.y)) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

// ─── Desenho do mapa ─────────────────────────────────────────────────────────
void MapManager::draw(sf::RenderTarget& rt, const GameMap& map,
                      const sf::Texture& sheet) {
    m_sprite.setTexture(sheet);
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x) {
            const Tile& t = map.tiles[y][x];
            if (!t.explored) continue;

            int col = t.texIndex % SHEET_COLS, row = t.texIndex / SHEET_COLS;
            m_sprite.setTextureRect(
                sf::IntRect(
                    sf::Vector2i(col * TILE_SIZE, row * TILE_SIZE),
                    sf::Vector2i(TILE_SIZE, TILE_SIZE)
                )
            );
            m_sprite.setPosition({static_cast<float>(x * TILE_SIZE),
                                 static_cast<float>(y * TILE_SIZE)});
            m_sprite.setColor(t.visible ? sf::Color::White : sf::Color(80, 80, 80));
            rt.draw(m_sprite);
        }
}

// ─── Utilitários ─────────────────────────────────────────────────────────────
bool MapManager::isSolid(const GameMap& map, int gx, int gy) const {
    return !inBounds(map, gx, gy) || map.tiles[gy][gx].solid;
}

bool MapManager::inBounds(const GameMap& map, int gx, int gy) const {
    return gx >= 0 && gx < map.width && gy >= 0 && gy < map.height;
}

sf::Vector2f MapManager::toPixel(sf::Vector2i g) {
    return {static_cast<float>(g.x * TILE_SIZE), static_cast<float>(g.y * TILE_SIZE)};
}

sf::Vector2i MapManager::toGrid(sf::Vector2f px) {
    return {static_cast<int>(px.x) / TILE_SIZE, static_cast<int>(px.y) / TILE_SIZE};
}
