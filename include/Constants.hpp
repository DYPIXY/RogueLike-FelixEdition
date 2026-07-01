#pragma once

constexpr int  WINDOW_W    = 1280;
constexpr int  WINDOW_H    = 720;
constexpr int  TARGET_FPS  = 60;
constexpr char WINDOW_TITLE[] = "Roguelike - UNIVALI";

constexpr int  TILE_SIZE   = 32;
constexpr int  MAP_W       = 40;
constexpr int  MAP_H       = 30;
constexpr int  HUD_HEIGHT  = 60;
constexpr int  FOV_RADIUS  = 7;
constexpr int  MAX_LEVELS  = 5;
constexpr int  BASE_CRIT   = 10;   // % chance de acerto crítico
constexpr int  SHEET_COLS  = 8;

// Tiles (linha 0 do sprite sheet)
constexpr int TEX_FLOOR       = 0;
constexpr int TEX_WALL        = 1;
constexpr int TEX_DOOR_CLOSED = 2;
constexpr int TEX_DOOR_OPEN   = 3;
constexpr int TEX_TRAP        = 4;
constexpr int TEX_STAIRS_DOWN = 5;
constexpr int TEX_STAIRS_UP   = 6;
constexpr int TEX_CHEST       = 7;

// Entidades (linha 1)
constexpr int TEX_PLAYER   = 8;
constexpr int TEX_GOBLIN   = 9;
constexpr int TEX_SKELETON = 10;
constexpr int TEX_TROLL    = 11;
constexpr int TEX_BOSS     = 12;
constexpr int TEX_NPC      = 13;

// Itens (linha 2)
constexpr int TEX_POTION = 16;
constexpr int TEX_KEY    = 17;
constexpr int TEX_WEAPON = 18;
constexpr int TEX_SHIELD = 19;
constexpr int TEX_GOLD   = 20;
constexpr int TEX_LONG_SWORD = 21;
