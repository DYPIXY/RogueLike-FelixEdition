#pragma once
#include <SFML/Graphics.hpp>
#include "Types.hpp"
#include "Constants.hpp"

class MapManager {
public:
    void loadLevel(GameMap& map, int level);
    void updateFOV(GameMap& map, sf::Vector2i playerGrid);
    void draw(sf::RenderTarget& target, const GameMap& map, const sf::Texture& sheet);

    bool isSolid  (const GameMap& map, int gx, int gy) const;
    bool inBounds (const GameMap& map, int gx, int gy) const;

    static sf::Vector2f toPixel(sf::Vector2i grid);
    static sf::Vector2i toGrid (sf::Vector2f px);

private:
    void buildDemoLevel(GameMap& map);
    void castRay(GameMap& map, sf::Vector2i origin, sf::Vector2i target, int radius);
};
