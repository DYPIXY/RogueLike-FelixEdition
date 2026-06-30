#pragma once
#include <SFML/Graphics.hpp>
#include "Types.hpp"
#include "MapManager.hpp"

// Classe principal: loop de jogo, estados, subsistemas.
class Game {
public:
    Game();
    void run();

private:
    GameState m_state = GameState::MainMenu;
    GameMap m_map;
    MapManager m_mapMgr;
    Player m_player;
    Enemy* m_battleEnemy;
    std::string m_battleLog;
    int m_attrChoice = 0;
    int m_menuSel = 0;
    std::string m_msg;
    float m_msgTimer = 0.f;
    float m_moveTimer = 0.f;
    float MOVE_DELAY = 0.15f;



    // Loop principal
    void processEvents();
    void update(float dt);
    void render();

    // Atualização por estado
    void updatePlaying(float dt);

    // Renderização por estado
    void drawMenu();
    void drawPlaying();
    void drawHUD();
    void drawBattle();
    void drawLevelUp();
    void drawGameOver();

    // Jogador
    void handlePlayerInput();
    void tryMove(int dx, int dy);
    void pickupItems();
    void checkTraps();
    void interactNPC();
    void usePotion();

    // Inimigos
    void updateEnemies(float dt);
    void enemyMove(Enemy& e);

    // Combate
    void startBattle(Enemy& e);
    void resolveBattle(bool playerAttacks);
    int  calcDamage(const Stats& atk, const Stats& def);

    // Progressão
    void grantXP(int amount);
    void doLevelUp();
    void applyAttrPoint(int attr);  // 0=hp 1=str 2=armor 3=dodge 4=speed

    // Utilitários
    void nextLevel();
    void drawEntity(int texIndex, sf::Vector2f pos);
    void updateCamera();
    // Atalho para criar e desenhar sf::Text inline
    void drawText(const std::string& s, int size, sf::Color c, float x, float y);
};
