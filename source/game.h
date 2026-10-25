#ifndef GAME_H
#define GAME_H

#include <nds.h>
#include <stdbool.h>

#define SCREEN_W 256
#define SCREEN_H 192
#define GROUND_Y 152
#define NUM_BIOMES 27
#define NUM_CHARS 99

enum GameState {
    STATE_START, STATE_PLAYING, STATE_PAUSED, STATE_COUNTDOWN, STATE_GAMEOVER, STATE_SHOP, STATE_DETAILS
};

enum AnimStyle {
    STYLE_NORMAL = 0, STYLE_SPIN, STYLE_FLOAT, STYLE_BOUNCE, STYLE_BLOCKY, STYLE_WOBBLE, STYLE_HEAVY, STYLE_FLAP
};

enum CharShape {
    SHAPE_HUMAN = 0, SHAPE_ANIMAL_EARS, SHAPE_MINECRAFT, SHAPE_BURGER, SHAPE_PIZZA, SHAPE_SUSHI,
    SHAPE_DONUT, SHAPE_BANANA, SHAPE_SLIME, SHAPE_ROBOT, SHAPE_WINGED, SHAPE_MONSTER, SHAPE_ONE_EYE
};

enum ObsType {
    OBS_ROCK = 0, OBS_CACTUS, OBS_SNAKE, OBS_LOLLIPOP, OBS_MAGMA, OBS_DRONE, OBS_PENGUIN, OBS_SHELL,
    OBS_BARRIER, OBS_STUMP, OBS_RED_SHROOM, OBS_CLOUD_ROCK, OBS_LANTERN, OBS_CRYSTAL_SPIKE, OBS_GEAR,
    OBS_GRAVE, OBS_GOLD_POT, OBS_PIXEL_BLOCK, OBS_SANDCASTLE, OBS_PUMPKIN, OBS_BARREL, OBS_RUIN, OBS_ROVER,
    OBS_BRICK, OBS_GLOWING_ROCK,
    OBS_BIRD_START = 30, OBS_BIRD_NORMAL = 30, OBS_VULTURE, OBS_PARROT, OBS_FIRE_BIRD, OBS_ALIEN_BIRD,
    OBS_FISH, OBS_MOSQUITO, OBS_SPORE_BAT, OBS_ANGEL, OBS_GHAST, OBS_BUTTERFLY, OBS_BAT, OBS_MECH_BIRD,
    OBS_GHOST, OBS_NYAN, OBS_PIXEL_BIRD, OBS_SEAGULL, OBS_CROW, OBS_MUTANT_FLY, OBS_EAGLE, OBS_PLANE, OBS_ANGLER
};

enum SceneryType {
    SCN_TREE = 0, SCN_PYRAMID, SCN_PALM, SCN_CANDY, SCN_VOLCANO, SCN_PLANET, SCN_ICE, SCN_CORAL,
    SCN_SKYSCRAPER, SCN_ACACIA, SCN_WILLOW, SCN_GIANT_SHROOM, SCN_PILLAR, SCN_QUARTZ_PILLAR, SCN_SAKURA,
    SCN_CRYSTAL, SCN_PIPES, SCN_DEAD_TREE, SCN_RAINBOW, SCN_PIXEL_TREE, SCN_ORANGE_TREE, SCN_VENT,
    SCN_COLUMN, SCN_RED_ROCK, SCN_BLOCK_TOWER, SCN_DEEP_VENTS
};

enum ParticleType {
    PART_NONE = 0, PART_LEAVES, PART_SAND, PART_FIREFLY, PART_SPARKLE, PART_SMOKE, PART_STAR, PART_SNOW,
    PART_BUBBLES, PART_RAIN, PART_ASH, PART_PETALS, PART_STEAM, PART_PIXELS, PART_CONFETTI, PART_GLOW
};

// הבעות פנים ומצבי אנימציה
enum FaceExpr { FACE_NORMAL = 0, FACE_SCARED, FACE_HAPPY, FACE_HURT };
enum PlayerAnimState { ANIM_RUN = 0, ANIM_JUMP, ANIM_HIT_GROUND, ANIM_HIT_AIR };

typedef struct {
    int threshold; const char* nameHeb; u8 skyTop[3]; u8 skyBot[3]; u8 ground[3]; u16 obsColor;
    enum ObsType groundObs; enum ObsType airObs; enum SceneryType scenery; enum ParticleType partType; bool isNight;
} BiomeData;

typedef struct {
    const char* id; const char* nameHeb; int price; u16 mainColor; u16 subColor;
    enum CharShape shape; enum AnimStyle style; u8 specialAcc;
} CharData;

typedef struct {
    float x, y, dy; int w, h; bool grounded; int angle; int jumpBuffer; int coyoteTimer;
    bool isDead; int deathTimer; enum PlayerAnimState animState;
} Player;

typedef struct {
    float x, y; int w, h; enum ObsType type; bool active; bool isAttacking;
} Obstacle;

typedef struct {
    float x, y; int value; u16 color; bool active;
} Coin;

typedef struct {
    float x, y; int w, h; enum SceneryType type; bool isCloud; bool active;
} BgElement;

typedef struct {
    float x, y, vx, vy; u16 color; int size; int life;
} EnvParticle;

// משתנים גלובליים
extern enum GameState gameState;
extern Player player;
extern int score, highScore, coinsCount, selectedCharIdx, currentBiomeIdx, frames, shopPage, countdownVal, bubbleTimer;
extern float currentSpeed;
extern bool unlockedChars[NUM_CHARS];
extern const char* bubbleText;
extern const char* currentRoastText;
extern enum ObsType lastDeathObs;
extern enum FaceExpr faceExpression;
extern float smoothSkyTop[3], smoothSkyBot[3], smoothGround[3];

extern const BiomeData BIOMES[NUM_BIOMES];
extern const CharData CHARACTERS[NUM_CHARS];

// פונקציות
u16 rgb8To16(int r, int g, int b);
void drawRect(u16* buf, int x, int y, int w, int h, u16 col);
void drawCircle(u16* buf, int cx, int cy, int r, u16 col);
void drawLine(u16* buf, int x0, int y0, int x1, int y1, u16 col);
void renderTopScreen(u16* buf);
void drawCharacterSprite(u16* buf, int charIdx, int cx, int cy, int scalePct, bool isMoving, bool grounded, int angle, int frameCnt, enum FaceExpr face, enum PlayerAnimState anim);
const char* getRoastForDeath(enum ObsType obs, int finalScore, int charIdx, int biomeIdx);
void renderBottomScreen(u16* buf);
void drawHebrewString(u16* buf, int centerX, int y, const char* utf8Str, u16 color, int scale);
void drawNumber(u16* buf, int x, int y, int num, u16 color, int scale);

#endif
