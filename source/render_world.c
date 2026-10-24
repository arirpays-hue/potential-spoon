#include "game.h"
#include <stdlib.h>

#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

void drawRect(u16* buf, int x, int y, int w, int h, u16 col) {
    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCREEN_W) ? SCREEN_W : x + w;
    int y1 = (y + h > SCREEN_H) ? SCREEN_H : y + h;
    for (int py = y0; py < y1; py++) {
        u16* row = &buf[py * SCREEN_W];
        for (int px = x0; px < x1; px++) {
            row[px] = col;
        }
    }
}

void drawCircle(u16* buf, int cx, int cy, int r, u16 col) {
    int r2 = r * r;
    for (int dy = -r; dy <= r; dy++) {
        int py = cy + dy;
        if (py < 0 || py >= SCREEN_H) continue;
        for (int dx = -r; dx <= r; dx++) {
            int px = cx + dx;
            if (px < 0 || px >= SCREEN_W) continue;
            if (dx * dx + dy * dy <= r2) {
                buf[py * SCREEN_W + px] = col;
            }
        }
    }
}

void drawLine(u16* buf, int x0, int y0, int x1, int y1, u16 col) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        if (x0 >= 0 && x0 < SCREEN_W && y0 >= 0 && y0 < SCREEN_H) {
            buf[y0 * SCREEN_W + x0] = col;
            if (x0 + 1 < SCREEN_W) buf[y0 * SCREEN_W + x0 + 1] = col;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// ציור תפאורת הרקע האמיתית לפי כל ביומה!
static void drawSceneryElement(u16* buf, BgElement* el) {
    int x = (int)el->x;
    int gy = GROUND_Y;
    if (el->isCloud) {
        drawCircle(buf, x, (int)el->y, 12, C16(245, 250, 255));
        drawCircle(buf, x + 10, (int)el->y + 2, 10, C16(245, 250, 255));
        drawCircle(buf, x - 10, (int)el->y + 3, 9, C16(245, 250, 255));
        return;
    }

    switch (el->type) {
        case SCN_CANDY: // רקע סוכריות בביומת הממתקים!
            drawRect(buf, x + 12, gy - 45, 4, 45, C16(255, 255, 255)); // מקל לבן
            drawCircle(buf, x + 14, gy - 50, 14, C16(255, 40, 150));   // סוכרייה ורודה ענקית
            drawCircle(buf, x + 14, gy - 50, 9,  C16(255, 255, 255));  // ספירלה לבנה
            drawCircle(buf, x + 14, gy - 50, 5,  C16(0, 230, 255));    // לב תכלת
            break;
        case SCN_PYRAMID: // פירמידה מצרית במדבר!
            for (int r = 0; r < 45; r++) {
                drawRect(buf, x + r, gy - r, 90 - r * 2, 1, C16(225, 165, 65));
            }
            break;
        case SCN_PALM: // עץ דקל טרופי בג'ונגל ובחוף!
            drawRect(buf, x + 14, gy - 42, 5, 42, C16(130, 80, 40));
            drawCircle(buf, x + 6,  gy - 42, 8, C16(30, 160, 50));
            drawCircle(buf, x + 24, gy - 42, 8, C16(30, 160, 50));
            drawCircle(buf, x + 16, gy - 48, 9, C16(40, 190, 60));
            break;
        case SCN_VOLCANO: // הר געש מתפרץ עם לבה!
            for (int r = 0; r < 42; r++) {
                u16 c = (r > 32) ? C16(255, 70, 0) : C16(65, 35, 35);
                drawRect(buf, x + r/2, gy - r, 60 - r, 1, c);
            }
            break;
        case SCN_PLANET: // כוכב לכת עם טבעות בחלל!
            drawCircle(buf, x + 20, gy - 75, 14, C16(180, 100, 240));
            drawLine(buf, x, gy - 70, x + 40, gy - 80, C16(240, 220, 130));
            break;
        case SCN_SKYSCRAPER: // גורד שחקים מואר בניאון בסייבר!
            drawRect(buf, x, gy - 70, 28, 70, C16(25, 15, 55));
            for (int wy = gy - 64; wy < gy - 8; wy += 10) {
                drawRect(buf, x + 4,  wy, 6, 5, C16(0, 255, 255));
                drawRect(buf, x + 16, wy, 6, 5, C16(255, 0, 180));
            }
            break;
        case SCN_GIANT_SHROOM: // פטריית ענק בביומת הפטריות!
            drawRect(buf, x + 12, gy - 38, 8, 38, C16(240, 230, 210));
            drawCircle(buf, x + 16, gy - 40, 16, C16(220, 40, 180));
            drawCircle(buf, x + 10, gy - 45, 3, C16(255, 255, 255));
            drawCircle(buf, x + 22, gy - 42, 3, C16(255, 255, 255));
            break;
        case SCN_CRYSTAL: // גבישי קריסטל סגולים זוהרים!
            for (int r = 0; r < 45; r++) {
                drawRect(buf, x + r/3, gy - r, 24 - (r/2), 1, C16(190, 60, 255));
            }
            break;
        case SCN_RAINBOW: // קשת בענן צבעונית!
            drawRect(buf, x, gy - 65, 45, 4, C16(255, 0, 0));
            drawRect(buf, x, gy - 61, 45, 4, C16(255, 165, 0));
            drawRect(buf, x, gy - 57, 45, 4, C16(255, 255, 0));
            drawRect(buf, x, gy - 53, 45, 4, C16(0, 220, 80));
            drawRect(buf, x, gy - 49, 45, 4, C16(0, 160, 255));
            break;
        case SCN_SAKURA: // עץ דובדבן ורוד בפריחה!
            drawRect(buf, x + 12, gy - 36, 6, 36, C16(90, 50, 40));
            drawCircle(buf, x + 15, gy - 44, 16, C16(255, 160, 200));
            break;
        default: // עץ יער / עמוד עתיק / מגדל קוביות
            drawRect(buf, x + 12, gy - 36, 6, 36, C16(100, 65, 45));
            drawCircle(buf, x + 15, gy - 44, 15, C16(34, 150, 45));
            break;
    }
}

// ציור כל האויבים והמכשולים בהתאם לשם הביומה שלהם!
static void drawObstacleReal(u16* buf, Obstacle* o, u16 biomeObsCol, int frameCnt) {
    int x = (int)o->x;
    int y = (int)o->y;
    u16 black = C16(20, 20, 20);
    u16 white = C16(250, 250, 250);
    u16 red   = C16(240, 30, 30);

    // אויבים מעופפים (ציפורים, עטלפים, רחפנים, רוחות, דגים, מלאכים)
    if (o->type >= OBS_BIRD_START) {
        int wingY = ((frameCnt / 4) % 2 == 0) ? -6 : 5;
        u16 bodyCol = C16(60, 60, 70);
        if (o->type == OBS_PARROT)     bodyCol = C16(255, 50, 50);
        if (o->type == OBS_FIRE_BIRD)  bodyCol = C16(255, 140, 0);
        if (o->type == OBS_GHOST || o->type == OBS_GHAST || o->type == OBS_ANGEL) bodyCol = white;
        if (o->type == OBS_ALIEN_BIRD || o->type == OBS_DRONE) bodyCol = C16(0, 240, 180);
        if (o->type == OBS_NYAN)       bodyCol = C16(255, 105, 180);

        drawRect(buf, x + 4, y + 6, 18, 9, bodyCol);
        drawLine(buf, x + 10, y + 6, x + 14, y + 6 + wingY, biomeObsCol);
        drawRect(buf, x + 2, y + 8, 3, 3, red); // עין אויב מעופף
        return;
    }

    // אויבים קרקעיים ייחודיים לכל ביומה!
    switch (o->type) {
        case OBS_CACTUS: // קקטוס מדברי עם זרועות!
            drawRect(buf, x + 8, y, 8, 26, C16(40, 160, 70));
            drawRect(buf, x + 2, y + 6, 6, 4, C16(40, 160, 70));
            drawRect(buf, x + 2, y + 2, 4, 6, C16(40, 160, 70));
            drawRect(buf, x + 16, y + 10, 6, 4, C16(40, 160, 70));
            drawRect(buf, x + 18, y + 5, 4, 7, C16(40, 160, 70));
            break;
        case OBS_LOLLIPOP: // סוכרייה על מקל אויבת בביומת הממתקים!
            drawRect(buf, x + 10, y + 10, 4, 16, white);
            drawCircle(buf, x + 12, y + 8, 9, C16(255, 20, 147));
            drawCircle(buf, x + 12, y + 8, 5, white);
            drawCircle(buf, x + 12, y + 8, 2, C16(255, 220, 0));
            break;
        case OBS_SNAKE: // נחש ג'ונגל מפותל עם לשון אדומה!
            drawCircle(buf, x + 6,  y + 18, 5, C16(50, 180, 40));
            drawCircle(buf, x + 14, y + 20, 5, C16(40, 150, 30));
            drawCircle(buf, x + 8,  y + 10, 5, C16(60, 200, 50));
            drawRect(buf, x, y + 9, 4, 2, red); // לשון
            drawRect(buf, x + 6, y + 8, 2, 2, black);
            break;
        case OBS_MAGMA: // כדור לבה בוער בהר געש וגיהנום!
            drawCircle(buf, x + 12, y + 14, 11, C16(220, 40, 0));
            drawCircle(buf, x + 12, y + 14, 7,  C16(255, 160, 0));
            drawRect(buf, x + 7,  y + 10, 3, 3, C16(255, 255, 0));
            drawRect(buf, x + 14, y + 10, 3, 3, C16(255, 255, 0));
            break;
        case OBS_PENGUIN: // פינגווין חמוד בקרחון!
            drawCircle(buf, x + 12, y + 14, 10, black);
            drawCircle(buf, x + 12, y + 15, 6,  white);
            drawRect(buf, x + 6, y + 6, 4, 3, C16(255, 150, 0)); // מקור כתום
            drawRect(buf, x + 9, y + 4, 2, 2, white);
            break;
        case OBS_RED_SHROOM: // פטרייה אדומה מנוקדת!
            drawRect(buf, x + 8, y + 12, 8, 14, C16(245, 235, 210));
            drawCircle(buf, x + 12, y + 10, 10, red);
            drawCircle(buf, x + 8,  y + 7, 2, white);
            drawCircle(buf, x + 15, y + 9, 2, white);
            break;
        case OBS_GRAVE: // מצבה עם צלב בבית הקברות!
            drawRect(buf, x + 4, y + 4, 16, 22, C16(130, 135, 140));
            drawCircle(buf, x + 12, y + 6, 8, C16(130, 135, 140));
            drawRect(buf, x + 11, y + 5, 2, 10, C16(70, 70, 75));
            drawRect(buf, x + 8,  y + 8, 8, 2,  C16(70, 70, 75));
            break;
        case OBS_PUMPKIN: // דלעת ליל כל הקדושים בסתיו!
            drawCircle(buf, x + 12, y + 14, 10, C16(255, 130, 0));
            drawRect(buf, x + 10, y + 1, 4, 4, C16(40, 140, 40));
            drawRect(buf, x + 7,  y + 10, 3, 3, C16(255, 240, 0));
            drawRect(buf, x + 14, y + 10, 3, 3, C16(255, 240, 0));
            break;
        case OBS_BARREL: // חבית פסולת רדיואקטיבית נוזלת ברעל!
            drawRect(buf, x + 4, y + 2, 16, 24, C16(50, 75, 60));
            drawRect(buf, x + 4, y + 5, 16, 4,  C16(120, 255, 0));
            drawCircle(buf, x + 12, y + 15, 4, C16(255, 230, 0));
            break;
        case OBS_CRYSTAL_SPIKE: // קוצי קריסטל חדים!
            for (int r = 0; r < 24; r++) {
                drawRect(buf, x + r/2, y + 24 - r, 24 - r, 1, C16(200, 50, 255));
            }
            break;
        case OBS_BARRIER: // מחסום לייזר סייבר!
            drawRect(buf, x + 2,  y, 4, 26, C16(80, 80, 90));
            drawRect(buf, x + 18, y, 4, 26, C16(80, 80, 90));
            drawRect(buf, x + 6,  y + 4, 12, 4, C16(0, 255, 120));
            drawRect(buf, x + 6,  y + 14, 12, 4, C16(255, 0, 120));
            break;
        default: // סלעים, גלגלי שיניים, קוביות לגו, תיבות זהב
            drawRect(buf, x + 2, y + 4, 20, 22, biomeObsCol);
            drawRect(buf, x + 5, y + 7, 14, 4, white);
            break;
    }
}

extern Obstacle obstacles[];
extern Coin coins[];
extern BgElement bgElements[];
extern EnvParticle envParticles[];

void renderTopScreen(u16* buf) {
    const BiomeData* b = &BIOMES[currentBiomeIdx];

    // 1. שמיים עם גרדיאנט צבעוני מדויק לפי הביומה הנוכחית
    for (int y = 0; y < GROUND_Y; y++) {
        int r = b->skyTop[0] + ((b->skyBot[0] - b->skyTop[0]) * y) / GROUND_Y;
        int g = b->skyTop[1] + ((b->skyBot[1] - b->skyTop[1]) * y) / GROUND_Y;
        int bl = b->skyTop[2] + ((b->skyBot[2] - b->skyTop[2]) * y) / GROUND_Y;
        u16 rowCol = rgb8To16(r, g, bl);
        u16* row = &buf[y * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++) row[x] = rowCol;
    }

    // 2. שמש או ירח וכוכבים בביומות לילה
    if (b->isNight) {
        drawCircle(buf, 220, 32, 12, C16(245, 245, 255));
        for (int i = 0; i < 20; i++) {
            int sx = (i * 37 + frames / 3) % SCREEN_W;
            int sy = (i * 19) % (GROUND_Y - 30);
            buf[sy * SCREEN_W + sx] = C16(255, 255, 255);
        }
    } else {
        drawCircle(buf, 220, 35, 18, C16(255, 245, 160));
    }

    // 3. אלמנטי תפאורה ברקע
    for (int i = 0; i < 6; i++) {
        if (bgElements[i].active) {
            drawSceneryElement(buf, &bgElements[i]);
        }
    }

    // 4. אדמה מדורגת לפי צבע הביומה
    for (int y = GROUND_Y; y < SCREEN_H; y++) {
        int fade = (y - GROUND_Y) * 2;
        int r = (b->ground[0] > fade) ? b->ground[0] - fade : 0;
        int g = (b->ground[1] > fade) ? b->ground[1] - fade : 0;
        int bl = (b->ground[2] > fade) ? b->ground[2] - fade : 0;
        u16 gCol = rgb8To16(r, g, bl);
        u16* row = &buf[y * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++) row[x] = gCol;
    }

    // 5. צעיף לבן מתנופף מאחורי השחקן
    int px = (int)player.x + 12;
    int py = (int)player.y + 12;
    for (int i = 1; i < 6; i++) {
        int wave = ((frames / 3 + i) % 2 == 0) ? -2 : 2;
        drawRect(buf, px - i * 5, py + wave, 5, 3, C16(255, 255, 255));
    }

    // 6. השחקן עם העיצוב והאנימציה הייחודית שלו
    drawCharacterSprite(buf, selectedCharIdx, (int)player.x + 12, (int)player.y + 12, 100, (gameState == STATE_PLAYING), player.grounded, player.angle, frames);

    // 7. אויבים ומכשולים
    for (int i = 0; i < 6; i++) {
        if (obstacles[i].active) {
            drawObstacleReal(buf, &obstacles[i], b->obsColor, frames);
        }
    }

    // 8. מטבעות ויהלומים
    for (int i = 0; i < 6; i++) {
        if (coins[i].active) {
            drawCircle(buf, (int)coins[i].x + 6, (int)coins[i].y + 6, 6, coins[i].color);
            drawCircle(buf, (int)coins[i].x + 6, (int)coins[i].y + 6, 4, C16(255, 255, 255));
        }
    }

    // 9. חלקיקי מזג אוויר ואווירה
    for (int i = 0; i < 25; i++) {
        if (envParticles[i].life > 0) {
            drawRect(buf, (int)envParticles[i].x, (int)envParticles[i].y, envParticles[i].size, envParticles[i].size, envParticles[i].color);
        }
    }

    // 10. תצוגת עלית (HUD): שם הביומה, ניקוד ומטבעות
    drawRect(buf, 4, 4, 85, 16, C16(20, 25, 35));
    drawHebrewString(buf, 46, 8, b->nameHeb, C16(255, 255, 255), 1);

    drawRect(buf, 95, 4, 72, 16, C16(20, 25, 35));
    drawNumber(buf, 102, 8, score / 10, C16(255, 255, 255), 1);

    drawRect(buf, 173, 4, 78, 16, C16(20, 25, 35));
    drawCircle(buf, 182, 12, 4, C16(255, 215, 0));
    drawNumber(buf, 192, 8, coinsCount, C16(255, 215, 0), 1);

    // 11. בועת הודעה או ספירה לאחור
    if (bubbleTimer > 0 && bubbleText != NULL) {
        drawRect(buf, 58, 36, 140, 18, C16(255, 255, 255));
        drawHebrewString(buf, 128, 41, bubbleText, C16(20, 20, 20), 1);
    }
    if (gameState == STATE_COUNTDOWN) {
        drawCircle(buf, 128, 96, 22, C16(20, 20, 30));
        drawNumber(buf, 122, 88, countdownVal, C16(255, 255, 255), 2);
    }
}
