#include "Game.hpp"
#include "Constants.hpp"
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

// ─── Construtor ───────────────────────────────────────────────────────────────
Game::Game() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    m_window.create(sf::VideoMode({WINDOW_W, WINDOW_H}), WINDOW_TITLE,
                    sf::Style::Close | sf::Style::Titlebar);
    m_window.setFramerateLimit(TARGET_FPS);

    m_gameView = sf::View(sf::FloatRect({0, 0}, {WINDOW_W, WINDOW_H - HUD_HEIGHT}));
    m_gameView.setViewport(
        sf::FloatRect(
            sf::Vector2f(0.f, 0.f),
            sf::Vector2f(
                1.f,
                static_cast<float>(WINDOW_H - HUD_HEIGHT) /
                static_cast<float>(WINDOW_H)
            )
        )
    );
    m_hudView = sf::View(sf::FloatRect({0, 0}, {WINDOW_W, WINDOW_H}));

    // Textura placeholder colorida caso o arquivo não exista
    if (!m_sheet.loadFromFile("assets/textures/tiles.png")) {
        sf::Image img;
        img.resize({SHEET_COLS * TILE_SIZE, 4 * TILE_SIZE}, sf::Color(50, 50, 50));
        sf::Color colors[] = {
            {80,80,80},{120,80,40},{160,120,40},{200,160,60},{180,40,40},
            {40,180,40},{40,40,180},{200,200,40},{40,200,200},{200,40,40},
            {200,200,200},{40,200,40},{200,40,200},{200,180,100},{80,80,80},
            {80,80,80},{255,20,20},{255,220,20},{200,200,200},{100,100,255},{255,215,0}
        };
        for (int i = 0; i < SHEET_COLS * 4 && i < 21; ++i) {
            int cx = (i % SHEET_COLS) * TILE_SIZE, cy = (i / SHEET_COLS) * TILE_SIZE;
            for (int py = cy+2; py < cy+TILE_SIZE-2; ++py)
                for (int px = cx+2; px < cx+TILE_SIZE-2; ++px)
                    img.setPixel(
                        sf::Vector2u(
                            static_cast<unsigned>(px),
                            static_cast<unsigned>(py)
                        ),
                        colors[i]
                    );
        }
        m_sheet.loadFromImage(img);
    }
    m_font.openFromFile("assets/fonts/font.ttf");   // falha silenciosa: textos somem
    m_sprite.setTexture(m_sheet);

    m_mapMgr.loadLevel(m_map, 1);

    m_player.stats = {20, 20, 5, 2, 10, 200, 0, 10, 1, 0};
    m_player.gridPos = m_map.playerStart;
    m_player.pos     = MapManager::toPixel(m_player.gridPos);
    m_player.texIndex = TEX_PLAYER;
    m_mapMgr.updateFOV(m_map, m_player.gridPos);
}

void Game::run() {
    sf::Clock clock;
    while (m_window.isOpen()) {
        float dt = clock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}

// ─── Eventos ──────────────────────────────────────────────────────────────────
void Game::processEvents() {
    while (auto ev = m_window.pollEvent())
    {
        if (ev->is<sf::Event::Closed>())
            m_window.close();

        if (!ev->getIf<sf::Event::KeyPressed>()) continue;
        
        auto keyCode = ev->getIf<sf::Event::KeyPressed>()->code;

        if (m_state == GameState::MainMenu) {
            if (keyCode == sf::Keyboard::Key::Up)    m_menuSel = (m_menuSel + 2) % 3;
            if (keyCode == sf::Keyboard::Key::Down)  m_menuSel = (m_menuSel + 1) % 3;
            if (keyCode == sf::Keyboard::Key::Enter || keyCode == sf::Keyboard::Key::Space) {
                if (m_menuSel == 0) m_state = GameState::Playing;
                if (m_menuSel == 2) m_window.close();
            }
        }
        if (m_state == GameState::LevelUp) {
            if (keyCode == sf::Keyboard::Key::Up)   m_attrChoice = (m_attrChoice + 4) % 5;
            if (keyCode == sf::Keyboard::Key::Down) m_attrChoice = (m_attrChoice + 1) % 5;
            if (keyCode == sf::Keyboard::Key::Enter) applyAttrPoint(m_attrChoice);
        }
        if (m_state == GameState::BattleScreen) {
            if (keyCode == sf::Keyboard::Key::A) resolveBattle(true);
            if (keyCode == sf::Keyboard::Key::F) resolveBattle(false);
        }
        if (m_state == GameState::GameOver || m_state == GameState::Victory)
            if (keyCode == sf::Keyboard::Key::Enter) { m_state = GameState::MainMenu; m_menuSel = 0; }

        if (m_state == GameState::Playing) {
            if (keyCode == sf::Keyboard::Key::P) usePotion();
            if (keyCode == sf::Keyboard::Key::E) interactNPC();
        }
    }
}

// ─── Update ───────────────────────────────────────────────────────────────────
void Game::update(float dt) {
    if (m_msgTimer > 0.f) m_msgTimer -= dt;
    if (m_state == GameState::Playing) updatePlaying(dt);
}

void Game::updatePlaying(float dt) {
    if (!m_player.alive) { m_state = GameState::GameOver; return; }
    m_moveTimer -= dt;
    if (m_moveTimer <= 0.f) handlePlayerInput();
    updateEnemies(dt);
    updateCamera();
}

// ─── Input do jogador ─────────────────────────────────────────────────────────
void Game::handlePlayerInput() {
    namespace K = sf::Keyboard;
    int dx = 0, dy = 0;
    if (K::isKeyPressed(K::Key::W) || K::isKeyPressed(K::Key::Up))    dy = -1;
    if (K::isKeyPressed(K::Key::S) || K::isKeyPressed(K::Key::Down))  dy =  1;
    if (K::isKeyPressed(K::Key::A) || K::isKeyPressed(K::Key::Left))  dx = -1;
    if (K::isKeyPressed(K::Key::D) || K::isKeyPressed(K::Key::Right)) dx =  1;
    if (dx || dy) { tryMove(dx, dy); m_moveTimer = MOVE_DELAY; }
}

void Game::tryMove(int dx, int dy) {
    int nx = m_player.gridPos.x + dx;
    int ny = m_player.gridPos.y + dy;

    // Verifica inimigo na célula destino → batalha por turno
    for (auto& e : m_map.enemies)
        if (e.alive && e.gridPos == sf::Vector2i{nx, ny}) { startBattle(e); return; }

    // Porta trancada: consome uma chave do inventário
    if (m_mapMgr.inBounds(m_map, nx, ny)) {
        auto& tile = m_map.tiles[ny][nx];
        if (tile.type == TileType::Door && tile.solid) {
            auto it = std::find_if(m_player.inventory.begin(), m_player.inventory.end(),
                [](const Item& i){ return i.type == ItemType::Key && i.uses != 0; });
            if (it == m_player.inventory.end()) {
                m_msg = "Precisa de uma chave!"; m_msgTimer = 2.f; return;
            }
            if (--it->uses == 0) m_player.inventory.erase(it);
            tile = {TileType::Floor, false, tile.visible, tile.explored, TEX_DOOR_OPEN};
            m_msg = "Porta aberta!"; m_msgTimer = 2.f;
        }
    }

    if (m_mapMgr.isSolid(m_map, nx, ny)) return;

    m_player.gridPos = {nx, ny};
    m_player.pos     = MapManager::toPixel(m_player.gridPos);
    m_player.moves++;

    // Verifica escadas
    if (m_map.tiles[ny][nx].type == TileType::StairsDown) {
        if (m_map.dungeonLevel < MAX_LEVELS) { nextLevel(); return; }
        bool bossDead = true;
        for (auto& e : m_map.enemies)
            if (e.type == EnemyType::Boss && e.alive) { bossDead = false; break; }
        if (bossDead) m_state = GameState::Victory;
        else { m_msg = "Derrote o boss primeiro!"; m_msgTimer = 2.f; }
    }

    pickupItems();
    checkTraps();
    m_mapMgr.updateFOV(m_map, m_player.gridPos);
}

// ─── Itens / Armadilhas / NPC ─────────────────────────────────────────────────
void Game::pickupItems() {
    for (auto& item : m_map.items) {
        if (!item.active || item.gridPos != m_player.gridPos) continue;
        item.active = false;
        m_player.inventory.push_back(item);
        m_player.score += 10;
        m_msg = "Coletou: " + item.name; m_msgTimer = 2.f;
    }
}

void Game::checkTraps() {
    for (auto& trap : m_map.traps) {
        if (trap.triggered || trap.gridPos != m_player.gridPos) continue;
        trap.triggered = trap.visible = true;
        int dmg = std::max(1, trap.damage - m_player.stats.armor);
        m_player.stats.hp -= dmg;
        m_player.score    -= 5;
        m_msg = "Armadilha! -" + std::to_string(dmg) + " HP"; m_msgTimer = 2.f;
        if (m_player.stats.hp <= 0) { m_player.alive = false; m_state = GameState::GameOver; }
    }
}

void Game::interactNPC() {
    for (auto& npc : m_map.npcs) {
        int dist = std::abs(npc.gridPos.x - m_player.gridPos.x)
                 + std::abs(npc.gridPos.y - m_player.gridPos.y);
        if (dist > 1) continue;
        m_msg = npc.name + ": " + npc.dialogue; m_msgTimer = 4.f;
        npc.talked = true;
    }
}

void Game::usePotion() {
    for (auto& item : m_player.inventory) {
        if (item.type != ItemType::HealthPotion || item.uses == 0) continue;
        m_player.stats.hp = std::min(m_player.stats.hpMax, m_player.stats.hp + item.value);
        if (--item.uses == 0) item.active = false;
        m_msg = "Usou pocao! +" + std::to_string(item.value) + " HP"; m_msgTimer = 2.f;
        return;
    }
    m_msg = "Sem pocoes!"; m_msgTimer = 2.f;
}

// ─── Inimigos ─────────────────────────────────────────────────────────────────
void Game::updateEnemies(float dt) {
    for (auto& e : m_map.enemies) {
        if (!e.alive) continue;
        e.moveTimer -= dt;
        if (e.moveTimer <= 0.f) { enemyMove(e); e.moveTimer = e.moveDelay; }
    }
}

void Game::enemyMove(Enemy& e) {
    int dist = std::abs(e.gridPos.x - m_player.gridPos.x)
             + std::abs(e.gridPos.y - m_player.gridPos.y);

    if (dist <= FOV_RADIUS && m_map.tiles[e.gridPos.y][e.gridPos.x].visible)
        e.aggro = true;

    int dx = 0, dy = 0;
    if (e.aggro) {
        int ddx = m_player.gridPos.x - e.gridPos.x;
        int ddy = m_player.gridPos.y - e.gridPos.y;
        if (std::abs(ddx) >= std::abs(ddy)) dx = ddx > 0 ? 1 : -1;
        else                                dy = ddy > 0 ? 1 : -1;
    } else {
        static const int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
        auto r = dirs[std::rand() % 4];
        dx = r[0]; dy = r[1];
    }

    int nx = e.gridPos.x + dx, ny = e.gridPos.y + dy;

    // Ataca o jogador se for colidir com ele
    if (sf::Vector2i{nx, ny} == m_player.gridPos) {
        int dmg = calcDamage(e.stats, m_player.stats);
        m_player.stats.hp -= dmg;
        m_player.score    -= 2;
        m_msg = (e.type == EnemyType::Boss ? "Boss" : "Inimigo")
              + std::string(" atacou! -") + std::to_string(dmg) + " HP";
        m_msgTimer = 1.5f;
        if (m_player.stats.hp <= 0) { m_player.alive = false; m_state = GameState::GameOver; }
        return;
    }

    if (m_mapMgr.isSolid(m_map, nx, ny)) return;
    for (auto& other : m_map.enemies)
        if (&other != &e && other.alive && other.gridPos == sf::Vector2i{nx, ny}) return;

    e.gridPos = {nx, ny};
    e.pos     = MapManager::toPixel(e.gridPos);
}

// ─── Combate ──────────────────────────────────────────────────────────────────
void Game::startBattle(Enemy& e) {
    m_battleEnemy = &e;
    const char* names[] = {"Goblin", "Esqueleto", "Troll", "BOSS"};
    m_battleLog = "Combate com " + std::string(names[static_cast<int>(e.type)]);
    m_state = GameState::BattleScreen;
}

void Game::resolveBattle(bool playerAttacks) {
    if (!m_battleEnemy || !m_battleEnemy->alive) { m_state = GameState::Playing; return; }

    if (playerAttacks) {
        int dmg = calcDamage(m_player.stats, m_battleEnemy->stats);
        m_battleEnemy->stats.hp -= dmg;
        m_battleLog = "Voce causou " + std::to_string(dmg) + " dano!";
        if (m_battleEnemy->stats.hp <= 0) {
            m_battleEnemy->alive = false;
            grantXP(m_battleEnemy->xpReward);
            m_player.score += m_battleEnemy->xpReward * 5;
            m_battleLog += " Inimigo derrotado!";
            m_battleEnemy = nullptr;
            m_state = GameState::Playing;
            return;
        }
    }

    // Contra-ataque do inimigo
    if (m_battleEnemy) {
        int dmg = calcDamage(m_battleEnemy->stats, m_player.stats);
        m_player.stats.hp -= dmg;
        m_battleLog += "\nInimigo causou " + std::to_string(dmg) + " dano!";
        if (m_player.stats.hp <= 0) { m_player.alive = false; m_state = GameState::GameOver; }
    }
}

// Esquiva → crítico → dano base
int Game::calcDamage(const Stats& atk, const Stats& def) {
    if (std::rand() % 100 < def.dodge) return 0;
    int dmg = std::max(1, atk.str - def.armor);
    if (std::rand() % 100 < BASE_CRIT) dmg *= 2;
    return dmg;
}

// ─── Progressão ───────────────────────────────────────────────────────────────
void Game::grantXP(int amount) {
    m_player.stats.xp += amount;
    if (m_player.stats.xp >= m_player.stats.xpNext) doLevelUp();
}

void Game::doLevelUp() {
    m_player.stats.level++;
    m_player.stats.xp    -= m_player.stats.xpNext;
    m_player.stats.xpNext = static_cast<int>(m_player.stats.xpNext * 1.5f);
    m_player.stats.attrPts++;
    m_player.score += 50;
    m_attrChoice = 0;
    m_state = GameState::LevelUp;
}

void Game::applyAttrPoint(int attr) {
    if (m_player.stats.attrPts <= 0) return;
    m_player.stats.attrPts--;
    auto& s = m_player.stats;
    if (attr == 0) { s.hpMax += 5; s.hp = std::min(s.hp + 5, s.hpMax); }
    if (attr == 1) s.str   += 2;
    if (attr == 2) s.armor += 1;
    if (attr == 3) s.dodge += 5;
    if (attr == 4) s.speed += 20;
    if (s.attrPts == 0) m_state = GameState::Playing;
}

// ─── Próximo nível ────────────────────────────────────────────────────────────
void Game::nextLevel() {
    m_mapMgr.loadLevel(m_map, m_map.dungeonLevel + 1);
    m_player.gridPos = m_map.playerStart;
    m_player.pos     = MapManager::toPixel(m_player.gridPos);
    m_player.score  += 100;
    m_mapMgr.updateFOV(m_map, m_player.gridPos);
    m_msg = "Nivel " + std::to_string(m_map.dungeonLevel) + "!"; m_msgTimer = 3.f;
}

// ─── Câmera ───────────────────────────────────────────────────────────────────
void Game::updateCamera() {
    float viewW = static_cast<float>(WINDOW_W);
    float viewH = static_cast<float>(WINDOW_H - HUD_HEIGHT);
    float cx = m_player.pos.x + TILE_SIZE / 2.f;
    float cy = m_player.pos.y + TILE_SIZE / 2.f;
    cx = std::clamp(cx, viewW / 2.f, m_map.width  * (float)TILE_SIZE - viewW / 2.f);
    cy = std::clamp(cy, viewH / 2.f, m_map.height * (float)TILE_SIZE - viewH / 2.f);
    m_gameView.setCenter({cx, cy});
}

// ─── Render ───────────────────────────────────────────────────────────────────
void Game::render() {
    m_window.clear(sf::Color(20, 20, 20));
    switch (m_state) {
        case GameState::MainMenu:     drawMenu();     break;
        case GameState::Playing:      drawPlaying();  break;
        case GameState::BattleScreen: drawBattle();   break;
        case GameState::LevelUp:      drawLevelUp();  break;
        case GameState::GameOver:     drawGameOver(); break;
        case GameState::Victory: {
            m_window.setView(m_hudView);
            drawText("VOCE VENCEU!", 64, sf::Color::Yellow, 380, 250);
            drawText("Pontuacao: " + std::to_string(m_player.score), 36, sf::Color::White, 500, 340);
            drawText("Enter para voltar ao menu", 28, {200,200,200}, 440, 420);
        } break;
    }
    m_window.display();
}

// ─── Helper: texto inline ─────────────────────────────────────────────────────
// Centraliza a criação de sf::Text para evitar repetição em cada drawXxx()
void Game::drawText(const std::string& str, int size, sf::Color c, float x, float y) {
    sf::Text t(m_font);
    t.setString(str);
    t.setCharacterSize(static_cast<unsigned>(size));
    t.setFillColor(c);
    t.setPosition({x, y});
    m_window.draw(t);
}

// ─── drawMenu ─────────────────────────────────────────────────────────────────
void Game::drawMenu() {
    m_window.setView(m_hudView);
    drawText("ROGUELIKE", 80, {220,150,30}, 430, 120);
    drawText("UNIVALI - Algoritmos e Programacao II", 22, {180,180,180}, 350, 220);

    const char* opts[] = {"Jogar", "Como Jogar", "Sair"};
    for (int i = 0; i < 3; ++i) {
        sf::Color c = (i == m_menuSel) ? sf::Color::Yellow : sf::Color::White;
        drawText((i == m_menuSel ? "> " : "  ") + std::string(opts[i]), 40, c, 530, 320.f + i*60);
    }

    if (m_menuSel == 1) {
        drawText("WASD / Setas: Mover",                              26, sf::Color::Cyan,      350, 550);
        drawText("P: Usar pocao   E: Falar com NPC",                 26, sf::Color::Cyan,      350, 590);
        drawText("Batalha: A = Atacar   F = Defender",               26, sf::Color::Cyan,      350, 630);
        drawText("Pontuacao: itens +10, kills +XP, armadilhas -5",   22, {180,255,180}, 310, 670);
    }
}

// ─── drawPlaying ──────────────────────────────────────────────────────────────
void Game::drawPlaying() {
    m_window.setView(m_gameView);
    m_mapMgr.draw(m_window, m_map, m_sheet);

    for (auto& item : m_map.items)
        if (item.active && m_map.tiles[item.gridPos.y][item.gridPos.x].visible)
            drawEntity(item.texIndex, MapManager::toPixel(item.gridPos));

    for (auto& npc : m_map.npcs)
        if (m_map.tiles[npc.gridPos.y][npc.gridPos.x].visible)
            drawEntity(npc.texIndex, MapManager::toPixel(npc.gridPos));

    for (auto& trap : m_map.traps)
        if (trap.visible && m_map.tiles[trap.gridPos.y][trap.gridPos.x].visible)
            drawEntity(TEX_TRAP, MapManager::toPixel(trap.gridPos));

    for (auto& e : m_map.enemies)
        if (e.alive && m_map.tiles[e.gridPos.y][e.gridPos.x].visible)
            drawEntity(e.texIndex, e.pos);

    drawEntity(m_player.texIndex, m_player.pos);
    drawHUD();

    if (m_msgTimer > 0.f) {
        m_window.setView(m_hudView);
        drawText(m_msg, 24, sf::Color::Yellow, 20, static_cast<float>(WINDOW_H - HUD_HEIGHT - 40));
    }
}

// ─── drawHUD ──────────────────────────────────────────────────────────────────
void Game::drawHUD() {
    m_window.setView(m_hudView);
    float hy = static_cast<float>(WINDOW_H - HUD_HEIGHT);

    sf::RectangleShape bg({static_cast<float>(WINDOW_W), static_cast<float>(HUD_HEIGHT)});
    bg.setFillColor({20, 20, 40}); bg.setPosition({0, hy}); m_window.draw(bg);

    auto& s = m_player.stats;
    std::ostringstream ss;
    ss << "Nivel: " << s.level
       << "   HP: " << s.hp << "/" << s.hpMax
       << "   Str: " << s.str << "   Armor: " << s.armor
       << "   XP: " << s.xp << "/" << s.xpNext
       << "   Pontuacao: " << m_player.score
       << "   Dungeon: " << m_map.dungeonLevel;
    drawText(ss.str(), 20, sf::Color::White, 10, hy + 10);

    // Barra de vida
    auto bar = [&](float bx, float by, float w, float h, float pct,
                    sf::Color bg_, sf::Color fg_) {
        sf::RectangleShape back({w, h}), fill({w * pct, h});
        back.setFillColor(bg_); back.setPosition({bx, by}); m_window.draw(back);
        fill.setFillColor(fg_); fill.setPosition({bx, by}); m_window.draw(fill);
    };
    bar(10,  hy+36, 300, 18, static_cast<float>(s.hp) / s.hpMax, {80,0,0},  {200,40,40});
    bar(320, hy+40, 300, 10, static_cast<float>(s.xp) / s.xpNext,{0,0,80},  {40,40,220});
}

// ─── drawBattle ───────────────────────────────────────────────────────────────
void Game::drawBattle() {
    drawPlaying();
    m_window.setView(m_hudView);

    sf::RectangleShape panel({600, 300});
    panel.setFillColor({10,10,40,230}); panel.setOutlineColor(sf::Color::Yellow);
    panel.setOutlineThickness(2); panel.setPosition({340, 200});
    m_window.draw(panel);

    drawText("== BATALHA ==", 30, sf::Color::Yellow, 530, 215);
    drawText(m_battleLog,     22, sf::Color::White,  360, 270);

    if (m_battleEnemy) {
        drawText("Inimigo HP: " + std::to_string(m_battleEnemy->stats.hp)
               + "/" + std::to_string(m_battleEnemy->stats.hpMax), 22, sf::Color::Red,   360, 310);
        drawText("Seu HP: " + std::to_string(m_player.stats.hp)
               + "/" + std::to_string(m_player.stats.hpMax),       22, sf::Color::Green, 360, 340);
    }
    drawText("[A] Atacar     [F] Defender (contra-ataque reduzido)", 22, sf::Color::Cyan, 360, 420);
}

// ─── drawLevelUp ──────────────────────────────────────────────────────────────
void Game::drawLevelUp() {
    drawPlaying();
    m_window.setView(m_hudView);

    sf::RectangleShape panel({500, 380});
    panel.setFillColor({20,20,60,240}); panel.setOutlineColor({200,200,40});
    panel.setOutlineThickness(2); panel.setPosition({390, 160});
    m_window.draw(panel);

    drawText("SUBIU DE NIVEL! Lv " + std::to_string(m_player.stats.level),
             32, sf::Color::Yellow, 430, 175);
    drawText("Escolha um atributo para melhorar:", 24, sf::Color::White, 410, 225);

    const char* attrs[] = {"Vida Maxima  (+5 HP)", "Forca        (+2 STR)",
                           "Armadura     (+1 ARM)", "Esquiva      (+5%)", "Velocidade   (+20)"};
    for (int i = 0; i < 5; ++i) {
        bool sel = (i == m_attrChoice);
        drawText((sel ? "> " : "  ") + std::string(attrs[i]),
                 26, sel ? sf::Color::Yellow : sf::Color::White, 420, 270.f + i*44);
    }
    drawText("[Enter] Confirmar   [Cima/Baixo] Navegar", 20, sf::Color::Cyan, 400, 500);
}

// ─── drawGameOver ─────────────────────────────────────────────────────────────
void Game::drawGameOver() {
    m_window.setView(m_hudView);
    sf::RectangleShape bg({static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)});
    bg.setFillColor({10,0,0}); m_window.draw(bg);
    drawText("GAME OVER",                                             80, sf::Color::Red,    420, 220);
    drawText("Pontuacao final: " + std::to_string(m_player.score),   36, sf::Color::White,  470, 340);
    drawText("Nivel da dungeon: " + std::to_string(m_map.dungeonLevel),28,{200,200,200},    490, 400);
    drawText("Enter para voltar ao menu",                             28, {200,200,200},     440, 470);
}

// ─── Helpers de render ────────────────────────────────────────────────────────
void Game::drawEntity(int texIndex, sf::Vector2f pos) {
    int col = texIndex % SHEET_COLS, row = texIndex / SHEET_COLS;
    m_sprite.setTextureRect(
        sf::IntRect(
            sf::Vector2i(col * TILE_SIZE, row * TILE_SIZE),
            sf::Vector2i(TILE_SIZE, TILE_SIZE)
        )
    );
    m_sprite.setPosition(pos);
    m_sprite.setColor(sf::Color::White);
    m_window.draw(m_sprite);
}
