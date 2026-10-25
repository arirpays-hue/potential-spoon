#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))
#define SAVE_MAGIC 0x4A4D503A // חתימת זיהוי מעודכנת לשמירת הדמות בהרכבה אישית והשיאים!

enum GameState gameState = STATE_START;
Player player;
int score = 0;
int highScore = 0;
int topScores[3] = {0, 0, 0};
int bestBiomeIdx = 0;
int totalGamesPlayed = 0;
int coinsCount = 0; // מתחילים מ-0 מטבעות כדי שלא יהיה ניתן לקנות דמויות על הסבב הראשון!
int selectedCharIdx = 0;
bool unlockedChars[TOTAL_SHOP_CHARS] = {false};
int currentBiomeIdx = 0;
int frames = 0;
float currentSpeed = 3.2f;
float worldScrollX = 0.0f; // נעצר לחלוטין כאשר המשחק לא ב-STATE_PLAYING!
int shopPage = 0;
int countdownVal = 3;
static int countdownFrames = 0;
int bubbleTimer = 0;
const char* bubbleText = NULL;
const char* currentRoastText = NULL;
const char* currentWarningText = NULL; // הודעת האזהרה המדויקת לאויב שבמסלול התנגשות!
enum ObsType lastDeathObs = OBS_ROCK;

// משתני מצב ל-4 האנימציות של הדמות, הבעות הפנים ומעבר הביומות ההדרגתי
enum CharAnimState playerAnimState = CHAR_ANIM_WALK;
enum FaceExpr currentFaceExpr = EXPR_NORMAL;
int happyFaceTimer = 0;
int deathAnimTimer = 0;
float smoothSkyTop[3] = {135.0f, 206.0f, 235.0f};
float smoothSkyBot[3] = {224.0f, 255.0f, 255.0f};
float smoothGround[3] = {34.0f,  139.0f, 34.0f};

Obstacle obstacles[6];
Coin coins[6];
BgElement bgElements[6];
EnvParticle envParticles[25];

static u16 topBuffer[SCREEN_W * SCREEN_H];
static u16 bottomBuffer[SCREEN_W * SCREEN_H];

// מבנה הנתונים שנשמר לתמיד בזיכרון הקונסולה/כרטיס (כולל הדמות בהרכבה אישית!)
typedef struct {
    u32 magic;
    int highScore;
    int topScores[3];
    int bestBiomeIdx;
    int totalGamesPlayed;
    int coinsCount;
    int selectedCharIdx;
    u8 unlocked[TOTAL_SHOP_CHARS];
    CustomCharData savedCustomChar;
    u32 savedCustomRewards;
} SaveFileData;

// חישוב הרמה/הביומה הנדרשת להסרת הטשטוש מעל דמויות מתקדמות בחנות!
int getRequiredBiomeForChar(int cIdx) {
    if (cIdx < 6 || cIdx == CUSTOM_CHAR_IDX) return 0;
    if (cIdx % 3 == 0) {
        int req = (cIdx / 4) + 1;
        if (req > 20) req = 20;
        return req;
    }
    return 0;
}

// שמירה בטוחה וישירה לתמיד לזיכרון ה-SRAM של ה-DS (כולל הדמות שיצרת!)
void savePermanentData(void) {
    SaveFileData data;
    memset(&data, 0, sizeof(SaveFileData));
    data.magic = SAVE_MAGIC;
    data.highScore = highScore;
    data.topScores[0] = topScores[0];
    data.topScores[1] = topScores[1];
    data.topScores[2] = topScores[2];
    data.bestBiomeIdx = bestBiomeIdx;
    data.totalGamesPlayed = totalGamesPlayed;
    data.coinsCount = coinsCount;
    data.selectedCharIdx = selectedCharIdx;
    for (int i = 0; i < TOTAL_SHOP_CHARS; i++) {
        data.unlocked[i] = unlockedChars[i] ? 1 : 0;
    }
    data.savedCustomChar = customChar;
    data.savedCustomRewards = unlockedCustomRewards;

    sysSetCartOwner(BUS_OWNER_ARM9);
    volatile u8* sram = (volatile u8*)0x0A000000;
    const u8* src = (const u8*)&data;
    for (unsigned int i = 0; i < sizeof(SaveFileData); i++) {
        sram[i] = src[i];
    }
}

// טעינה בטוחה ומידית של כל השיאים, הדמויות והדמות האישית בעת הדלקת המשחק
void loadPermanentData(void) {
    SaveFileData data;
    memset(&data, 0, sizeof(SaveFileData));

    sysSetCartOwner(BUS_OWNER_ARM9);
    volatile u8* sram = (volatile u8*)0x0A000000;
    u8* dst = (u8*)&data;
    for (unsigned int i = 0; i < sizeof(SaveFileData); i++) {
        dst[i] = sram[i];
    }

    if (data.magic == SAVE_MAGIC) {
        highScore = (data.highScore >= 0) ? data.highScore : 0;
        topScores[0] = (data.topScores[0] >= 0) ? data.topScores[0] : 0;
        topScores[1] = (data.topScores[1] >= 0) ? data.topScores[1] : 0;
        topScores[2] = (data.topScores[2] >= 0) ? data.topScores[2] : 0;
        bestBiomeIdx = (data.bestBiomeIdx >= 0 && data.bestBiomeIdx < NUM_BIOMES) ? data.bestBiomeIdx : 0;
        totalGamesPlayed = (data.totalGamesPlayed >= 0) ? data.totalGamesPlayed : 0;
        coinsCount = (data.coinsCount >= 0) ? data.coinsCount : 0;
        selectedCharIdx = (data.selectedCharIdx >= 0 && data.selectedCharIdx < TOTAL_SHOP_CHARS) ? data.selectedCharIdx : 0;
        for (int i = 0; i < TOTAL_SHOP_CHARS; i++) {
            unlockedChars[i] = (data.unlocked[i] != 0);
        }
        unlockedChars[0] = true;
        customChar = data.savedCustomChar;
        unlockedCustomRewards = data.savedCustomRewards;
        if (customChar.created) unlockedChars[CUSTOM_CHAR_IDX] = true;
    }
}

// עדכון טבלת 3 השיאים הגדולים, בדיקת פרסי שיאים ושמירה מידית לתמיד!
static void recordNewScore(int finalSc, int biomeReached) {
    totalGamesPlayed++;
    bool brokeRecord = (finalSc > highScore);
    if (biomeReached > bestBiomeIdx) {
        bestBiomeIdx = biomeReached;
    }
    if (finalSc > highScore) {
        highScore = finalSc;
    }
    if (finalSc > topScores[0]) {
        topScores[2] = topScores[1];
        topScores[1] = topScores[0];
        topScores[0] = finalSc;
    } else if (finalSc > topScores[1] && finalSc != topScores[0]) {
        topScores[2] = topScores[1];
        topScores[1] = finalSc;
    } else if (finalSc > topScores[2] && finalSc != topScores[1] && finalSc != topScores[0]) {
        topScores[2] = finalSc;
    }
    checkMissionAndRecordUnlocks(finalSc, biomeReached, brokeRecord);
    savePermanentData();
}

static void showBubble(const char* txt) {
    bubbleText = txt;
    bubbleTimer = 140;
}

static void resetGame(void) {
    score = 0;
    frames = 0;
    currentBiomeIdx = 0;
    currentSpeed = 3.2f;
    worldScrollX = 0.0f;
    player.x = 32.0f;
    player.y = GROUND_Y - 24;
    player.dy = 0.0f;
    player.w = 24;
    player.h = 24;
    player.grounded = true;
    player.angle = 0;
    player.jumpBuffer = 0;
    player.coyoteTimer = 0;

    playerAnimState = CHAR_ANIM_WALK;
    currentFaceExpr = EXPR_NORMAL;
    currentWarningText = NULL;
    happyFaceTimer = 0;
    deathAnimTimer = 0;

    pickNewTempMission(); // הגרלת משימה זמנית חדשה לסיבוב זה!

    for (int c = 0; c < 3; c++) {
        smoothSkyTop[c] = (float)BIOMES[0].skyTop[c];
        smoothSkyBot[c] = (float)BIOMES[0].skyBot[c];
        smoothGround[c] = (float)BIOMES[0].ground[c];
    }

    for (int i = 0; i < 6; i++) {
        obstacles[i].active = false;
        obstacles[i].hitAnimTimer = 0;
        coins[i].active = false;
        bgElements[i].active = true;
        bgElements[i].x = i * 50 + (rand() % 20);
        bgElements[i].y = 25 + (rand() % 30);
        bgElements[i].isCloud = (i % 2 == 0);
        bgElements[i].type = BIOMES[0].scenery;
    }
    for (int i = 0; i < 25; i++) envParticles[i].life = 0;
    showBubble(getActiveMissionDesc());
}

static void triggerJump(void) {
    player.jumpBuffer = 8;
}

// סימולציית חיזוי מסלול מדויקת: בודקת אם השחקן והאויב באמת הולכים להתנגש אם השחקן לא ישנה מסלול!
static bool willPlayerCollideWithObstacle(const Obstacle* o, float grav) {
    int pad = 5;
    float simY  = player.y;
    float simDY = player.dy;
    float simOx = o->x;

    if (simOx + o->w - pad <= player.x + pad) return false;
    float maxLookahead = 55.0f + currentSpeed * 12.0f;
    if (simOx - (player.x + player.w) > maxLookahead) return false;

    for (int step = 0; step < 32; step++) {
        if (player.x + pad < simOx + o->w - pad &&
            player.x + player.w - pad > simOx + pad &&
            simY + pad < o->y + o->h - pad &&
            simY + player.h - pad > o->y + pad) {
            return true;
        }

        simOx -= currentSpeed;
        simDY += grav;
        simY  += simDY;
        if (simY + player.h >= GROUND_Y) {
            simY = GROUND_Y - player.h;
            simDY = 0.0f;
        }

        if (simOx + o->w < player.x) break;
    }
    return false;
}

static void updateGamePhysics(void) {
    frames++;
    score++;
    int dispScore = score / 10;

    if (bubbleTimer > 0) bubbleTimer--;
    if (happyFaceTimer > 0) happyFaceTimer--;

    // עדכון ביומה לפי ניקוד (27 עולמות!)
    int newBiome = 0;
    for (int i = NUM_BIOMES - 1; i >= 0; i--) {
        if (dispScore >= BIOMES[i].threshold) {
            newBiome = i;
            break;
        }
    }
    if (newBiome != currentBiomeIdx) {
        currentBiomeIdx = newBiome;
        if (currentBiomeIdx > bestBiomeIdx) {
            bestBiomeIdx = currentBiomeIdx;
            savePermanentData();
        }
        showBubble(BIOMES[currentBiomeIdx].nameHeb);
    }

    // בדיקה בזמן אמת אם השחקן השלים את המשימה הזמנית (למשל הגיע לביומת הממתקים וזכה בראש עוגייה!)
    checkMissionAndRecordUnlocks(dispScore, currentBiomeIdx, (dispScore > highScore));

    // מעבר הדרגתי ומציאותי בין צבעי הביומות (ללא מעבר "בבום"!)
    const BiomeData* targetB = &BIOMES[currentBiomeIdx];
    for (int c = 0; c < 3; c++) {
        smoothSkyTop[c] += ((float)targetB->skyTop[c] - smoothSkyTop[c]) * 0.025f;
        smoothSkyBot[c] += ((float)targetB->skyBot[c] - smoothSkyBot[c]) * 0.025f;
        smoothGround[c] += ((float)targetB->ground[c] - smoothGround[c]) * 0.025f;
    }

    // האצה הדרגתית ומאתגרת ככל שמתקדמים בניקוד ובביומות (דורש אינסטינקט חד אך תמיד אפשרי!)
    float speedBonus = (dispScore < 1500) ? (dispScore * 0.0011f) : (1.65f + (dispScore - 1500) * 0.00035f);
    float biomeBoost = currentBiomeIdx * 0.04f;
    currentSpeed = 3.2f + speedBonus + biomeBoost;
    if (currentSpeed > 6.4f) currentSpeed = 6.4f;

    // קידום מרחק הקרקע אך ורק כשמשחקים בפועל!
    worldScrollX += currentSpeed;

    // פיזיקת קפיצה אדפטיבית
    float speedRatio = currentSpeed / 3.2f;
    enum AnimStyle style = (selectedCharIdx == CUSTOM_CHAR_IDX) ? STYLE_NORMAL : CHARACTERS[selectedCharIdx].style;
    float grav = 0.45f * (0.85f + 0.15f * speedRatio);
    float jumpForce = -7.8f * (0.92f + 0.08f * speedRatio);

    if (style == STYLE_FLOAT) {
        grav = 0.32f * (0.85f + 0.15f * speedRatio);
        jumpForce = -6.9f * (0.92f + 0.08f * speedRatio);
    } else if (style == STYLE_HEAVY) {
        grav = 0.58f * (0.85f + 0.15f * speedRatio);
        jumpForce = -8.8f * (0.92f + 0.08f * speedRatio);
    } else if (style == STYLE_BOUNCE) {
        jumpForce = -8.2f * (0.92f + 0.08f * speedRatio);
    }

    if (player.jumpBuffer > 0) player.jumpBuffer--;
    if (player.grounded) player.coyoteTimer = 6;
    else if (player.coyoteTimer > 0) player.coyoteTimer--;

    if (player.jumpBuffer > 0 && player.coyoteTimer > 0) {
        player.dy = jumpForce;
        player.grounded = false;
        player.jumpBuffer = 0;
        player.coyoteTimer = 0;
    }

    int held = keysHeld();
    if (!(held & (KEY_A | KEY_UP | KEY_B | KEY_TOUCH)) && player.dy < -3.0f) {
        player.dy = -3.0f;
    }

    player.dy += grav;
    player.y += player.dy;

    if (player.y + player.h >= GROUND_Y) {
        player.y = GROUND_Y - player.h;
        player.dy = 0.0f;
        player.grounded = true;
        player.angle = 0;
        playerAnimState = CHAR_ANIM_WALK;
    } else {
        player.grounded = false;
        player.angle += (style == STYLE_SPIN) ? 16 : 4;
        playerAnimState = CHAR_ANIM_JUMP;
    }

    // עדכון תפאורת רקע
    for (int i = 0; i < 6; i++) {
        bgElements[i].x -= currentSpeed * 0.25f;
        if (bgElements[i].x < -40) {
            bgElements[i].x = SCREEN_W + (rand() % 30);
            bgElements[i].isCloud = (rand() % 3 == 0);
            bgElements[i].type = BIOMES[currentBiomeIdx].scenery;
            bgElements[i].y = 22 + (rand() % 35);
        }
    }

    // יצירת מכשולים עם מרווח בטיחות מובטח
    float rightmostObsX = -100.0f;
    for (int i = 0; i < 6; i++) {
        if (obstacles[i].active && obstacles[i].x > rightmostObsX) {
            rightmostObsX = obstacles[i].x;
        }
    }

    float minSafeGap = 86.0f + currentSpeed * 15.5f + (rand() % 28);

    if (SCREEN_W - rightmostObsX >= minSafeGap) {
        for (int i = 0; i < 6; i++) {
            if (!obstacles[i].active) {
                obstacles[i].active = true;
                obstacles[i].hitAnimTimer = 0;
                obstacles[i].x = SCREEN_W + 10 + (rand() % 18);
                bool isAir = (rand() % 10 >= 7);
                obstacles[i].type = isAir ? BIOMES[currentBiomeIdx].airObs : BIOMES[currentBiomeIdx].groundObs;
                obstacles[i].w = 24;
                obstacles[i].h = isAir ? 18 : 26;
                obstacles[i].y = isAir ? (GROUND_Y - 48 - (rand() % 28)) : (GROUND_Y - 26);
                break;
            }
        }

        if (rand() % 10 >= 4) {
            for (int i = 0; i < 6; i++) {
                if (!coins[i].active) {
                    coins[i].active = true;
                    coins[i].x = SCREEN_W + 42 + (rand() % 35);
                    int r = rand() % 10;
                    if (r >= 9) {
                        coins[i].value = 20;
                        coins[i].color = C16(0, 255, 255);
                        coins[i].y = GROUND_Y - 68;
                    } else if (r >= 6) {
                        coins[i].value = 15;
                        coins[i].color = C16(255, 215, 0);
                        coins[i].y = GROUND_Y - 45;
                    } else {
                        coins[i].value = 10;
                        coins[i].color = C16(215, 135, 50);
                        coins[i].y = GROUND_Y - 20;
                    }
                    break;
                }
            }
        }
    }

    // זיהוי מדויק: האם השחקן נמצא במסלול התנגשות ישיר עם אחד האויבים?
    bool collisionCourse = false;
    float closestThreatX = 9999.0f;
    enum ObsType threatType = OBS_ROCK;

    for (int i = 0; i < 6; i++) {
        if (obstacles[i].active) {
            obstacles[i].x -= currentSpeed;

            if (willPlayerCollideWithObstacle(&obstacles[i], grav)) {
                if (obstacles[i].x < closestThreatX) {
                    closestThreatX = obstacles[i].x;
                    threatType = obstacles[i].type;
                    collisionCourse = true;
                }
            }

            int pad = 5;
            if (player.x + pad < obstacles[i].x + obstacles[i].w - pad &&
                player.x + player.w - pad > obstacles[i].x + pad &&
                player.y + pad < obstacles[i].y + obstacles[i].h - pad &&
                player.y + player.h - pad > obstacles[i].y + pad) {
           
                lastDeathObs = obstacles[i].type;
                obstacles[i].hitAnimTimer = 1;
                deathAnimTimer = 1;

                if (lastDeathObs >= OBS_BIRD_START) {
                    playerAnimState = CHAR_ANIM_HIT_AIR;
                } else {
                    playerAnimState = CHAR_ANIM_HIT_GROUND;
                }
                currentFaceExpr = EXPR_HURT;

                int finalSc = score / 10;
                recordNewScore(finalSc, currentBiomeIdx);

                int roastIdx = (selectedCharIdx == CUSTOM_CHAR_IDX) ? 0 : selectedCharIdx;
                currentRoastText = getRoastForDeath(lastDeathObs, finalSc, roastIdx, currentBiomeIdx);
                gameState = STATE_GAMEOVER;
                return;
            }
            if (obstacles[i].x < -30) obstacles[i].active = false;
        }
    }

    // תנועת מטבעות ואיסוף
    for (int i = 0; i < 6; i++) {
        if (coins[i].active) {
            coins[i].x -= currentSpeed;
            if (player.x < coins[i].x + 12 && player.x + player.w > coins[i].x &&
                player.y < coins[i].y + 12 && player.y + player.h > coins[i].y) {
                coinsCount += coins[i].value;
                happyFaceTimer = 45;
                if (coins[i].value == 20) showBubble("יהלום!");
                coins[i].active = false;
                savePermanentData();
            } else if (coins[i].x < -20) {
                coins[i].active = false;
            }
        }
    }

    // עדכון הבעת הפנים הדינמית של הדמות והודעת האזהרה הספציפית לאויב במסך התחתון!
    if (collisionCourse) {
        currentFaceExpr = EXPR_SCARED;
        currentWarningText = getWarningForObstacle(threatType);
    } else if (happyFaceTimer > 0) {
        currentFaceExpr = EXPR_HAPPY;
        currentWarningText = NULL;
    } else {
        currentFaceExpr = EXPR_NORMAL;
        currentWarningText = NULL;
    }

    // חלקיקי מזג אוויר לפי הביומה
    if (frames % 4 == 0 && BIOMES[currentBiomeIdx].partType != PART_NONE) {
        int idx = rand() % 25;
        envParticles[idx].x = SCREEN_W;
        envParticles[idx].y = rand() % GROUND_Y;
        envParticles[idx].vx = -1.8f;
        envParticles[idx].vy = 0.6f;
        envParticles[idx].size = 2;
        envParticles[idx].life = 80;
        envParticles[idx].color = BIOMES[currentBiomeIdx].obsColor;
    }
    for (int i = 0; i < 25; i++) {
        if (envParticles[i].life > 0) {
            envParticles[i].x += envParticles[i].vx;
            envParticles[i].y += envParticles[i].vy;
            envParticles[i].life--;
        }
    }
}

int main(void) {
    // 1. אתחול מיידי של שני המסכים כדי למנוע מסך לבן בכניסה!
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    int bgTop = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* vramTop = bgGetGfxPtr(bgTop);

    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    int bgSub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* vramBottom = bgGetGfxPtr(bgSub);

    srand(time(NULL));
    unlockedChars[0] = true; // דמות ברירת מחדל פתוחה
    initDefaultCustomChar();
    loadPermanentData();     // טעינה בטוחה מה-SRAM ללא fopen!

    resetGame();
    gameState = STATE_START;

    while (1) {
        scanKeys();
        int down = keysDown();

        if (gameState == STATE_PLAYING) {
            if (down & (KEY_A | KEY_B | KEY_UP)) triggerJump();
            if (down & KEY_START) gameState = STATE_PAUSED;
            updateGamePhysics();

        } else if (gameState == STATE_COUNTDOWN) {
            countdownFrames++;
            if (countdownFrames >= 45) {
                countdownFrames = 0;
                countdownVal--;
                if (countdownVal <= 0) gameState = STATE_PLAYING;
            }
        } else {
            frames++;
            if (gameState == STATE_GAMEOVER) {
                deathAnimTimer++;
                for (int i = 0; i < 6; i++) {
                    if (obstacles[i].active && obstacles[i].hitAnimTimer > 0) {
                        obstacles[i].hitAnimTimer++;
                    }
                }
            }
            if ((down & KEY_A) && (gameState == STATE_START || gameState == STATE_GAMEOVER)) {
                resetGame();
                gameState = STATE_PLAYING;
            }
        }

        // זיהוי נגיעות במסך התחתון
        if (down & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);
            int tx = touch.px, ty = touch.py;

            if (gameState == STATE_START) {
                if (tx >= 48 && tx <= 208 && ty >= 52 && ty <= 82) {
                    resetGame();
                    gameState = STATE_PLAYING;
                } else if (tx >= 48 && tx <= 208 && ty >= 88 && ty <= 118) {
                    gameState = STATE_SHOP;
                } else if (tx >= 48 && tx <= 208 && ty >= 124 && ty <= 152) {
                    gameState = STATE_CUSTOMIZE;
                } else if (tx >= 68 && tx <= 188 && ty >= 158 && ty <= 184) {
                    gameState = STATE_DETAILS;
                }
            } else if (gameState == STATE_CUSTOMIZE || gameState == STATE_CUSTOM_KEYBOARD) {
                handleCustomizerTouch(tx, ty);
            } else if (gameState == STATE_PLAYING) {
                if (tx >= 68 && tx <= 188 && ty >= 134 && ty <= 170) {
                    gameState = STATE_PAUSED;
                } else {
                    triggerJump();
                }
            } else if (gameState == STATE_PAUSED) {
                if (tx >= 68 && tx <= 188 && ty >= 134 && ty <= 170) {
                    countdownVal = 3;
                    countdownFrames = 0;
                    gameState = STATE_COUNTDOWN;
                }
            } else if (gameState == STATE_GAMEOVER) {
                if (tx >= 48 && tx <= 208 && ty >= 106 && ty <= 138) {
                    resetGame();
                    gameState = STATE_PLAYING;
                } else if (tx >= 48 && tx <= 208 && ty >= 146 && ty <= 178) {
                    gameState = STATE_SHOP;
                }
            } else if (gameState == STATE_SHOP) {
                // בחירה או קניית דמות מתוך ה-6 שבעמוד הנוכחי (כולל בדיקת טשטוש רמה ודמות אישית!)
                for (int i = 0; i < 6; i++) {
                    int cIdx = shopPage * 6 + i;
                    if (cIdx >= TOTAL_SHOP_CHARS) break;
                    int bx = 12 + (i % 3) * 78;
                    int by = 22 + (i / 3) * 64;
                    if (tx >= bx && tx <= bx + 72 && ty >= by && ty <= by + 58) {
                        if (cIdx == CUSTOM_CHAR_IDX) {
                            if (customChar.created) {
                                selectedCharIdx = CUSTOM_CHAR_IDX;
                                savePermanentData();
                            } else {
                                gameState = STATE_CUSTOMIZE;
                            }
                        } else {
                            int reqBiome = getRequiredBiomeForChar(cIdx);
                            if (bestBiomeIdx >= reqBiome) { // ניתן לקנות או לבחור רק אם הוסר הטשטוש!
                                if (unlockedChars[cIdx]) {
                                    selectedCharIdx = cIdx;
                                    savePermanentData();
                                } else if (coinsCount >= CHARACTERS[cIdx].price) {
                                    coinsCount -= CHARACTERS[cIdx].price;
                                    unlockedChars[cIdx] = true;
                                    selectedCharIdx = cIdx;
                                    savePermanentData();
                                }
                            }
                        }
                    }
                }
                // כפתורי ניווט בחנות
                if (ty >= 154 && ty <= 184) {
                    if (tx >= 12 && tx <= 67 && shopPage > 0) shopPage--;
                    else if (tx >= 75 && tx <= 181) gameState = STATE_START;
                    else if (tx >= 189 && tx <= 244 && (shopPage + 1) * 6 < TOTAL_SHOP_CHARS) shopPage++;
                }
            } else if (gameState == STATE_DETAILS) {
                if (tx >= 68 && tx <= 188 && ty >= 154 && ty <= 184) {
                    gameState = STATE_START;
                }
            }
        }

        // רינדור שני המסכים, ניקוי מטמון והעברה מהירה ב-DMA ל-VRAM
        renderTopScreen(topBuffer);
        renderBottomScreen(bottomBuffer);

        DC_FlushRange(topBuffer, SCREEN_W * SCREEN_H * 2);
        DC_FlushRange(bottomBuffer, SCREEN_W * SCREEN_H * 2);

        swiWaitForVBlank();
        dmaCopy(topBuffer, vramTop, SCREEN_W * SCREEN_H * 2);
        dmaCopy(bottomBuffer, vramBottom, SCREEN_W * SCREEN_H * 2);
    }
    return 0;
}
 
