#include "game.h"
#include <stdlib.h>
#include <time.h>

#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

enum GameState gameState = STATE_START;
Player player;
int score = 0;
int highScore = 0;
int coinsCount = 250; // מטבעות התחלתיים כדי שתוכל לנסות דמויות מיד!
int selectedCharIdx = 0;
bool unlockedChars[NUM_CHARS] = {false};
int currentBiomeIdx = 0;
int frames = 0;
float currentSpeed = 3.2f;
int shopPage = 0;
int countdownVal = 3;
static int countdownFrames = 0;
int bubbleTimer = 0;
const char* bubbleText = NULL;
const char* currentRoastText = NULL;
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

static void showBubble(const char* txt) {
    bubbleText = txt;
    bubbleTimer = 140;
}

static void resetGame(void) {
    score = 0;
    frames = 0;
    currentBiomeIdx = 0;
    currentSpeed = 3.2f;
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
    happyFaceTimer = 0;
    deathAnimTimer = 0;

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
    showBubble("יאללה קופצים!");
}

static void triggerJump(void) {
    player.jumpBuffer = 8;
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
        showBubble(BIOMES[currentBiomeIdx].nameHeb);
    }

    // מעבר הדרגתי ומציאותי בין צבעי הביומות (ללא מעבר "בבום"!)
    const BiomeData* targetB = &BIOMES[currentBiomeIdx];
    for (int c = 0; c < 3; c++) {
        smoothSkyTop[c] += ((float)targetB->skyTop[c] - smoothSkyTop[c]) * 0.025f;
        smoothSkyBot[c] += ((float)targetB->skyBot[c] - smoothSkyBot[c]) * 0.025f;
        smoothGround[c] += ((float)targetB->ground[c] - smoothGround[c]) * 0.025f;
    }

    currentSpeed = 3.2f + (dispScore * 0.004f);
    if (currentSpeed > 7.0f) currentSpeed = 7.0f;

    // פיזיקת קפיצה מותאמת לפי הדמות שנבחרה בחנות!
    enum AnimStyle style = CHARACTERS[selectedCharIdx].style;
    float grav = 0.45f;
    float jumpForce = -7.8f;

    if (style == STYLE_FLOAT) {
        grav = 0.30f;      // חייזר / אסטרונאוט / מלאך מרחפים לאט באוויר!
        jumpForce = -6.8f;
    } else if (style == STYLE_HEAVY) {
        grav = 0.58f;      // אביר / קינג קונג / גודזילה קופצים חזק ונוחתים כבד!
        jumpForce = -8.8f;
    } else if (style == STYLE_BOUNCE) {
        jumpForce = -8.2f; // המבורגר / סליים / חיות קופצניות!
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

    // קיצור קפיצה אם עוזבים את הכפתור/מגע
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

    // יצירת מכשולים ומטבעות
    int spawnInterval = (int)(180.0f / currentSpeed);
    if (spawnInterval < 28) spawnInterval = 28;

    if (frames % spawnInterval == 0) {
        for (int i = 0; i < 6; i++) {
            if (!obstacles[i].active) {
                obstacles[i].active = true;
                obstacles[i].hitAnimTimer = 0;
                obstacles[i].x = SCREEN_W + 10;
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
                    coins[i].x = SCREEN_W + 35 + (rand() % 35);
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

    // בדיקה אם אויב או מכשול קרובים לשחקן (עבור הבעת הפנים המפוחדת במסך התחתון!)
    bool dangerNear = false;

    // תנועת מכשולים ובדיקת התנגשות
    for (int i = 0; i < 6; i++) {
        if (obstacles[i].active) {
            obstacles[i].x -= currentSpeed;

            float dist = obstacles[i].x - (player.x + player.w);
            if (dist > -10.0f && dist < 78.0f) {
                dangerNear = true;
            }

            int pad = 5;
            if (player.x + pad < obstacles[i].x + obstacles[i].w - pad &&
                player.x + player.w - pad > obstacles[i].x + pad &&
                player.y + pad < obstacles[i].y + obstacles[i].h - pad &&
                player.y + player.h - pad > obstacles[i].y + pad) {
               
                lastDeathObs = obstacles[i].type;
                obstacles[i].hitAnimTimer = 1; // מפעיל אנימציית תקיפה לאויב הפוגע!
                deathAnimTimer = 1;

                // בחירת אנימציית הפגיעה הייחודית לדמות: אויב מעופף לעומת אויב קרקע!
                if (lastDeathObs >= OBS_BIRD_START) {
                    playerAnimState = CHAR_ANIM_HIT_AIR;
                } else {
                    playerAnimState = CHAR_ANIM_HIT_GROUND;
                }
                currentFaceExpr = EXPR_HURT;

                int finalSc = score / 10;
                if (finalSc > highScore) highScore = finalSc;
                // שליפת עקיצה מותאמת אישית ממאגר ה-C הענק!
                currentRoastText = getRoastForDeath(lastDeathObs, finalSc, selectedCharIdx, currentBiomeIdx);
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
                happyFaceTimer = 45; // הבעת פנים שמחה בדמות שבמסך התחתון!
                if (coins[i].value == 20) showBubble("יהלום!");
                coins[i].active = false;
            } else if (coins[i].x < -20) {
                coins[i].active = false;
            }
        }
    }

    // עדכון הבעת הפנים הדינמית של הדמות
    if (dangerNear) {
        currentFaceExpr = EXPR_SCARED;
    } else if (happyFaceTimer > 0) {
        currentFaceExpr = EXPR_HAPPY;
    } else {
        currentFaceExpr = EXPR_NORMAL;
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
    srand(time(NULL));
    unlockedChars[0] = true; // דמות ברירת מחדל פתוחה

    // אתחול שני המסכים של ה-DS במצב 16-bit Bitmap לחופש גרפי מקסימלי!
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    int bgTop = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* vramTop = bgGetGfxPtr(bgTop);

    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    int bgSub = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* vramBottom = bgGetGfxPtr(bgSub);

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
                if (tx >= 48 && tx <= 208 && ty >= 64 && ty <= 96) {
                    resetGame();
                    gameState = STATE_PLAYING;
                } else if (tx >= 48 && tx <= 208 && ty >= 106 && ty <= 138) {
                    gameState = STATE_SHOP;
                } else if (tx >= 68 && tx <= 188 && ty >= 148 && ty <= 174) {
                    gameState = STATE_DETAILS;
                }
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
                // בחירה או קניית דמות מתוך ה-6 שבעמוד הנוכחי
                for (int i = 0; i < 6; i++) {
                    int cIdx = shopPage * 6 + i;
                    if (cIdx >= NUM_CHARS) break;
                    int bx = 12 + (i % 3) * 78;
                    int by = 22 + (i / 3) * 64;
                    if (tx >= bx && tx <= bx + 72 && ty >= by && ty <= by + 58) {
                        if (unlockedChars[cIdx]) {
                            selectedCharIdx = cIdx;
                        } else if (coinsCount >= CHARACTERS[cIdx].price) {
                            coinsCount -= CHARACTERS[cIdx].price;
                            unlockedChars[cIdx] = true;
                            selectedCharIdx = cIdx;
                        }
                    }
                }
                // כפתורי ניווט בחנות
                if (ty >= 154 && ty <= 184) {
                    if (tx >= 12 && tx <= 67 && shopPage > 0) shopPage--;
                    else if (tx >= 75 && tx <= 181) gameState = STATE_START;
                    else if (tx >= 189 && tx <= 244 && (shopPage + 1) * 6 < NUM_CHARS) shopPage++;
                }
            } else if (gameState == STATE_DETAILS) {
                if (tx >= 68 && tx <= 188 && ty >= 142 && ty <= 174) {
                    gameState = STATE_START;
                }
            }
        }

        // רינדור שני המסכים והעברה מהירה ב-DMA ל-VRAM
        renderTopScreen(topBuffer);
        renderBottomScreen(bottomBuffer);

        swiWaitForVBlank();
        dmaCopy(topBuffer, vramTop, SCREEN_W * SCREEN_H * 2);
        dmaCopy(bottomBuffer, vramBottom, SCREEN_W * SCREEN_H * 2);
    }
    return 0;
}
 
