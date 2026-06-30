#pragma once
#include "Constants.hpp"
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

//enums
enum class TileType  { Floor, Wall, Door, Trap, StairsDown, StairsUp, Chest };
enum class ItemType  { HealthPotion, Key, Weapon, Shield, PowerUp };
enum class EnemyType { Goblin, Skeleton, Troll, Boss };
enum class GameState { MainMenu, Playing, BattleScreen, LevelUp, GameOver, Victory };

//structs
struct Tile {
    TileType type     = TileType::Floor;
    bool     solid    = false;
    bool     visible  = false;  // dentro do FOV atual
    bool     explored = false;  // já visitado (desenha escuro)
    int      texIndex = 0;
};

struct Stats {
    int hp = 20, hpMax = 20;
    int str = 5, armor = 2, dodge = 5;
    int speed  = 200;           // px/s (jogador) ou tiles/s (inimigos)
    int xp = 0, xpNext = 10;
    int level = 1, attrPts = 0; // pontos a distribuir ao subir de nível
};

struct Item {
    ItemType     type    = ItemType::HealthPotion;
    std::string  name    = "Potion";
    int          value   = 10;  // cura, bônus de dano…
    int          uses    = 1;   // -1 = infinito
    bool         active  = true;
    sf::Vector2i gridPos = {0, 0};
    int          texIndex = 0;
};

struct Player {
    Stats             stats;
    sf::Vector2f      pos;
    sf::Vector2i      gridPos;
    int               gold = 0, score = 0, moves = 0;
    bool              alive    = true;
    int               texIndex = 0;
    std::vector<Item> inventory;
};

struct Enemy {
    EnemyType    type      = EnemyType::Goblin;
    Stats        stats;
    sf::Vector2f pos;
    sf::Vector2i gridPos;
    bool         alive     = true;
    bool         aggro     = false;   // está perseguindo?
    float        moveTimer = 0.f;
    float        moveDelay = 1.0f;   // segundos entre passos
    int          texIndex  = 0;
    int          xpReward  = 5;
};

struct NPC {
    std::string  name, dialogue;
    sf::Vector2i gridPos;
    bool         talked   = false;
    int          texIndex = 0;
};

struct Trap {
    sf::Vector2i gridPos;
    int          damage    = 5;
    bool         triggered = false;
    bool         visible   = false;
};

struct GameMap {
    int   width = MAP_W, height = MAP_H;
    int   dungeonLevel = 1;
    sf::Vector2i playerStart = {2, 2};
    sf::Vector2i stairsPos   = {38, 28};
    std::vector<std::vector<Tile>> tiles;   // tiles[y][x]
    std::vector<Item>  items;
    std::vector<Enemy> enemies;
    std::vector<NPC>   npcs;
    std::vector<Trap>  traps;
};



//globals
static sf::Texture m_sheet;
static sf::Font m_font;
static sf::Sprite m_sprite = sf::Sprite(m_sheet);
static sf::RenderWindow m_window;
static sf::View m_gameView;    // câmera que segue o jogador
static sf::View m_hudView;     // fixa para HUD e menus
