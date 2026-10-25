#include "game.h"
#include <stdlib.h>

// תיקון מוחלט של צבעי ה-DS (פורמט ABGR של החומרה!)
#undef C16
#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

u16 rgb8To16_fixed(int r, int g, int b) {
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return C16(r, g, b);
}

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

// מלבן עם מסגרת שחורה (Pixel-Art Outline)
static void drawFramedRect(u16* buf, int x, int y, int w, int h, u16 fillCol, u16 borderCol) {
    drawRect(buf, x, y, w, h, borderCol);
    if (w > 2 && h > 2) {
        drawRect(buf, x + 1, y + 1, w - 2, h - 2, fillCol);
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

// עיגול עם קו מתאר והצללה פנימית (נפח תלת-ממדי לפיקסל ארט!)
static void drawShadedCircle(u16* buf, int cx, int cy, int r, u16 baseCol, u16 lightCol, u16 darkCol, u16 outlineCol) {
    int rOut2 = (r + 1) * (r + 1);
    int rIn2  = r * r;
    for (int dy = -(r + 1); dy <= (r + 1); dy++) {
        int py = cy + dy;
        if (py < 0 || py >= SCREEN_H) continue;
        for (int dx = -(r + 1); dx <= (r + 1); dx++) {
            int px = cx + dx;
            if (px < 0 || px >= SCREEN_W) continue;
            int d2 = dx * dx + dy * dy;
            if (d2 <= rIn2) {
                if (dx + dy < -r / 2) buf[py * SCREEN_W + px] = lightCol;
                else if (dx + dy > r / 2) buf[py * SCREEN_W + px] = darkCol;
                else buf[py * SCREEN_W + px] = baseCol;
            } else if (d2 <= rOut2) {
                buf[py * SCREEN_W + px] = outlineCol;
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

// ציור עננים מפורטים עם נפח והצללה תחתונה
static void drawDetailedCloud(u16* buf, int x, int y) {
    u16 outline = C16(140, 165, 195);
    u16 shadow  = C16(205, 225, 245);
    u16 white   = C16(252, 254, 255);
    drawCircle(buf, x - 12, y + 4, 10, outline);
    drawCircle(buf, x + 12, y + 4, 11, outline);
    drawCircle(buf, x,      y - 2, 14, outline);
    drawRect(buf, x - 14, y + 6, 28, 7, shadow);
    drawCircle(buf, x - 12, y + 3, 9, white);
    drawCircle(buf, x + 12, y + 3, 10, white);
    drawCircle(buf, x,      y - 2, 13, white);
    drawRect(buf, x - 12, y + 5, 24, 5, shadow);
}

// ציור שכבת הרים וגבעות רחוקות באופק עם צבעים שעוברים בהדרגתיות!
static void drawDistantHills(u16* buf, int scrollOffset) {
    int r  = ((int)smoothSkyBot[0] + (int)smoothGround[0]) / 2;
    int g  = ((int)smoothSkyBot[1] + (int)smoothGround[1]) / 2;
    int bl = ((int)smoothSkyBot[2] + (int)smoothGround[2]) / 2;
    u16 hillCol   = rgb8To16_fixed(r - 20, g - 20, bl - 20);
    u16 hillLight = rgb8To16_fixed(r + 10, g + 10, bl + 10);

    for (int x = 0; x < SCREEN_W; x++) {
        int wx = (x + scrollOffset) % 128;
        int distFromPeak = abs(wx - 64);
        int hillH = 34 - (distFromPeak * distFromPeak) / 120;
        if (hillH > 0) {
            int topY = GROUND_Y - hillH;
            drawRect(buf, x, topY, 1, hillH, hillCol);
            if (wx < 64 && topY >= 0 && topY < SCREEN_H) {
                buf[topY * SCREEN_W + x] = hillLight;
            }
        }
    }
}

// ציור תפאורת רקע ברזולוציה גבוהה לכל 27 הביומות!
static void drawSceneryElement(u16* buf, BgElement* el) {
    int x = (int)el->x;
    int gy = GROUND_Y;
    u16 black = C16(20, 20, 25);

    if (el->isCloud) {
        drawDetailedCloud(buf, x, (int)el->y);
        return;
    }

    switch (el->type) {
        case SCN_CANDY: { // רקע סוכריות ענקיות, וופלים ומקלות סבא בביומת הממתקים!
            drawFramedRect(buf, x + 13, gy - 52, 6, 52, C16(255, 255, 255), C16(180, 180, 200));
            for (int py = gy - 48; py < gy - 4; py += 8) {
                drawRect(buf, x + 14, py, 4, 3, C16(255, 80, 160));
            }
            drawFramedRect(buf, x + 8, gy - 36, 16, 5, C16(0, 220, 255), black);
            drawShadedCircle(buf, x + 16, gy - 54, 17, C16(255, 30, 140), C16(255, 130, 200), C16(190, 10, 90), black);
            drawCircle(buf, x + 16, gy - 54, 13, C16(255, 250, 255));
            drawCircle(buf, x + 16, gy - 54, 9,  C16(0, 230, 255));
            drawCircle(buf, x + 16, gy - 54, 6,  C16(255, 230, 0));
            drawCircle(buf, x + 16, gy - 54, 3,  C16(255, 30, 140));
            drawCircle(buf, x + 9, gy - 61, 2, C16(255, 255, 255));
            break;
        }
        case SCN_PYRAMID: { // פירמידה מצרית עם צד מואר, צד מוצל ושורות אבנים!
            for (int r = 0; r < 52; r++) {
                int y = gy - 1 - r;
                int halfW = 52 - r;
                drawRect(buf, x + 52 - halfW, y, halfW, 1, (r % 6 == 0) ? C16(190, 135, 50) : C16(245, 195, 85));
                drawRect(buf, x + 52, y, halfW, 1, (r % 6 == 0) ? C16(140, 95, 30) : C16(185, 130, 50));
            }
            drawFramedRect(buf, x + 46, gy - 14, 12, 14, C16(25, 15, 10), C16(120, 80, 25));
            break;
        }
        case SCN_PALM: { // עץ דקל טרופי מפורט עם גזע טבעות, אגוזי קוקוס וכפות דקל קמורות!
            for (int seg = 0; seg < 7; seg++) {
                int sy = gy - 8 - seg * 7;
                int sx = x + 14 + (seg / 2);
                drawFramedRect(buf, sx, sy, 8, 8, C16(150, 95, 45), C16(80, 45, 20));
                drawRect(buf, sx + 1, sy + 1, 3, 6, C16(185, 125, 65));
            }
            int topX = x + 19, topY = gy - 54;
            drawShadedCircle(buf, topX - 3, topY + 3, 4, C16(90, 55, 25), C16(130, 80, 40), C16(50, 30, 10), black);
            drawShadedCircle(buf, topX + 4, topY + 3, 4, C16(90, 55, 25), C16(130, 80, 40), C16(50, 30, 10), black);
            drawShadedCircle(buf, topX - 14, topY - 2, 9, C16(35, 175, 55), C16(90, 225, 95), C16(20, 120, 35), black);
            drawShadedCircle(buf, topX + 14, topY - 2, 9, C16(35, 175, 55), C16(90, 225, 95), C16(20, 120, 35), black);
            drawShadedCircle(buf, topX,      topY - 8, 11, C16(45, 195, 65), C16(110, 240, 110), C16(25, 140, 40), black);
            break;
        }
        case SCN_VOLCANO: { // הר געש אימתני עם לוע בוער ונחלי לבה זוהרים!
            for (int r = 0; r < 50; r++) {
                int y = gy - 1 - r;
                int w = 76 - r;
                int startX = x + r / 2;
                u16 rockCol = (r > 38) ? C16(255, 80, 0) : ((r % 5 == 0) ? C16(45, 25, 25) : C16(65, 40, 40));
                drawRect(buf, startX, y, w, 1, rockCol);
                drawRect(buf, x + 36 + (r % 3), y, 5, 1, (r % 2 == 0) ? C16(255, 200, 0) : C16(255, 60, 0));
            }
            drawCircle(buf, x + 38, gy - 52, 8, C16(255, 120, 0));
            drawCircle(buf, x + 38, gy - 52, 4, C16(255, 240, 80));
            break;
        }
        case SCN_PLANET: { // כוכב לכת בסגנון שבתאי עם מכתשים וטבעות כפולות!
            drawShadedCircle(buf, x + 22, gy - 82, 16, C16(160, 80, 230), C16(210, 140, 255), C16(95, 35, 150), black);
            drawCircle(buf, x + 17, gy - 86, 3, C16(120, 50, 180));
            drawCircle(buf, x + 26, gy - 78, 4, C16(120, 50, 180));
            drawLine(buf, x - 2, gy - 74, x + 46, gy - 90, C16(255, 225, 120));
            drawLine(buf, x,     gy - 72, x + 44, gy - 88, C16(220, 175, 80));
            break;
        }
        case SCN_SKYSCRAPER: { // גורד שחקים סייברפאנק עם אנטנה, מסגרת ניאון וחלונות מוארים!
            drawFramedRect(buf, x, gy - 78, 34, 78, C16(20, 15, 45), C16(0, 240, 255));
            drawLine(buf, x + 17, gy - 78, x + 17, gy - 92, C16(255, 0, 160));
            drawCircle(buf, x + 17, gy - 93, 2, C16(255, 50, 50));
            for (int wy = gy - 70; wy < gy - 6; wy += 9) {
                drawRect(buf, x + 4,  wy, 7, 5, ((wy / 9) % 2 == 0) ? C16(0, 255, 255) : C16(255, 230, 0));
                drawRect(buf, x + 14, wy, 6, 5, C16(255, 0, 180));
                drawRect(buf, x + 23, wy, 7, 5, C16(0, 255, 160));
            }
            break;
        }
        case SCN_GIANT_SHROOM: { // פטריית יער קסומה ענקית עם זימים ונקודות בולטות!
            drawFramedRect(buf, x + 14, gy - 44, 12, 44, C16(245, 235, 215), black);
            drawRect(buf, x + 15, gy - 43, 4, 42, C16(255, 250, 240));
            drawShadedCircle(buf, x + 20, gy - 48, 20, C16(215, 30, 150), C16(255, 95, 200), C16(145, 15, 100), black);
            drawShadedCircle(buf, x + 12, gy - 54, 4, C16(255, 255, 255), C16(255, 255, 255), C16(210, 210, 230), black);
            drawShadedCircle(buf, x + 26, gy - 51, 5, C16(255, 255, 255), C16(255, 255, 255), C16(210, 210, 230), black);
            drawShadedCircle(buf, x + 19, gy - 42, 3, C16(255, 255, 255), C16(255, 255, 255), C16(210, 210, 230), black);
            break;
        }
        case SCN_SAKURA:
        case SCN_ORANGE_TREE:
        default: { // עץ מפורט ברזולוציה גבוהה
            u16 leafBase  = (el->type == SCN_SAKURA) ? C16(255, 145, 195) : ((el->type == SCN_ORANGE_TREE) ? C16(235, 115, 25) : C16(38, 165, 52));
            u16 leafLight = (el->type == SCN_SAKURA) ? C16(255, 205, 230) : ((el->type == SCN_ORANGE_TREE) ? C16(255, 175, 55) : C16(95, 220, 90));
            u16 leafDark  = (el->type == SCN_SAKURA) ? C16(195, 85, 135)  : ((el->type == SCN_ORANGE_TREE) ? C16(165, 65, 10)  : C16(22, 105, 32));

            drawFramedRect(buf, x + 9, gy - 6, 22, 6, C16(105, 65, 35), black);
            drawFramedRect(buf, x + 13, gy - 44, 14, 42, C16(120, 75, 42), black);
            drawRect(buf, x + 14, gy - 43, 4, 40, C16(155, 102, 60));
            drawRect(buf, x + 22, gy - 43, 4, 40, C16(82, 48, 25));
            drawRect(buf, x + 18, gy - 30, 2, 8, C16(65, 38, 18));

            drawShadedCircle(buf, x + 9,  gy - 44, 13, leafBase, leafLight, leafDark, black);
            drawShadedCircle(buf, x + 31, gy - 44, 13, leafBase, leafLight, leafDark, black);
            drawShadedCircle(buf, x + 20, gy - 40, 14, leafBase, leafLight, leafDark, black);
            drawShadedCircle(buf, x + 20, gy - 55, 15, leafBase, leafLight, leafDark, black);

            drawCircle(buf, x + 12, gy - 48, 2, C16(240, 35, 35));
            drawCircle(buf, x + 27, gy - 43, 2, C16(240, 35, 35));
            drawCircle(buf, x + 19, gy - 58, 2, C16(240, 35, 35));
            break;
        }
    }
}

// בדיקה אם האויב מוגדר כ"יצור חי" לצורך הפעלת אנימציית תקיפה כשהוא פוגע בשחקן!
static bool isLivingEnemy(enum ObsType t) {
    if (t >= OBS_BIRD_START) return true;
    return (t == OBS_SNAKE || t == OBS_PENGUIN || t == OBS_RED_SHROOM ||
            t == OBS_PUMPKIN || t == OBS_MAGMA || t == OBS_LOLLIPOP);
}

// ציור אויבים ברזולוציה גבוהה + אנימציית תקיפה לאויבים חיים שפגעו בשחקן!
static void drawObstacleReal(u16* buf, Obstacle* o, u16 biomeObsCol, int frameCnt) {
    bool attacking = (o->hitAnimTimer > 0 && isLivingEnemy(o->type));
    int atkLungeX = attacking ? -4 + ((o->hitAnimTimer / 2) % 2) * 3 : 0;
    int atkHopY   = attacking ? -(((o->hitAnimTimer / 3) % 2) * 4) : 0;

    int x = (int)o->x + atkLungeX;
    int y = (int)o->y + atkHopY;
    u16 black  = C16(18, 18, 22);
    u16 white  = C16(252, 252, 255);
    u16 red    = C16(245, 35, 35);
    u16 yellow = C16(255, 225, 25);

    // הילת תקיפה אדומה-כתומה לאויב חי בזמן פגיעה בשחקן!
    if (attacking) {
        drawCircle(buf, x + 8, y + 10, 12, yellow);
        drawCircle(buf, x + 8, y + 10, 9,  red);
    }

    // 1. אויבים מעופפים ברזולוציה גבוהה!
    if (o->type >= OBS_BIRD_START) {
        int flap = attacking ? (((o->hitAnimTimer) % 2 == 0) ? -10 : 8) : (((frameCnt / 4) % 2 == 0) ? -7 : 5);

        if (o->type == OBS_GHOST || o->type == OBS_GHAST) {
            drawShadedCircle(buf, x + 12, y + 8, attacking ? 11 : 9, white, white, C16(190, 210, 235), black);
            drawRect(buf, x + 3, y + 8, 18, 8, white);
            drawCircle(buf, x + 6,  y + 16, 3, white);
            drawCircle(buf, x + 12, y + 16, 3, white);
            drawCircle(buf, x + 18, y + 16, 3, white);
            drawCircle(buf, x + 8,  y + 8, 2, black);
            drawCircle(buf, x + 14, y + 8, 2, black);
            drawCircle(buf, x + 7,  y + 8, 1, red);
            drawCircle(buf, x + 13, y + 8, 1, red);
            if (attacking) { // פה פעור לרוח הרפאים בתקיפה!
                drawCircle(buf, x + 10, y + 13, 3, black);
            }
            return;
        }

        if (o->type == OBS_DRONE || o->type == OBS_ALIEN_BIRD || o->type == OBS_MECH_BIRD) {
            drawFramedRect(buf, x + 4, y + 6, 16, 8, C16(90, 105, 120), black);
            drawShadedCircle(buf, x + 12, y + 10, 6, C16(140, 155, 175), white, C16(75, 85, 100), black);
            drawCircle(buf, x + 9, y + 10, attacking ? 4 : 3, red);
            drawCircle(buf, x + 8, y + 9, 1, yellow);
            int bladeW = ((frameCnt / 2) % 2 == 0) ? 22 : 10;
            drawRect(buf, x + 12 - bladeW / 2, y + 1, bladeW, 2, C16(0, 240, 255));
            drawRect(buf, x + 11, y + 3, 2, 3, black);
            if (attacking) { // קרן לייזר בתקיפה!
                drawLine(buf, x + 6, y + 10, x - 8, y + 14, red);
            }
            return;
        }

        u16 bodyBase  = C16(75, 65, 90);
        u16 bodyLight = C16(120, 110, 140);
        u16 wingCol   = C16(95, 85, 115);
        if (o->type == OBS_PARROT)    { bodyBase = C16(235, 45, 45); bodyLight = C16(255, 110, 90); wingCol = C16(30, 180, 255); }
        if (o->type == OBS_FIRE_BIRD) { bodyBase = C16(255, 110, 0); bodyLight = C16(255, 220, 40); wingCol = C16(230, 35, 0); }
        if (o->type == OBS_CROW || o->type == OBS_BAT) { bodyBase = C16(40, 35, 50); bodyLight = C16(80, 75, 95); wingCol = C16(60, 40, 80); }

        drawFramedRect(buf, x + 18, y + 7, 7, 5, wingCol, black);
        drawShadedCircle(buf, x + 13, y + 10, 6, bodyBase, bodyLight, bodyBase, black);
        drawShadedCircle(buf, x + 7,  y + 8,  5, bodyBase, bodyLight, bodyBase, black);
        // מקור פעור בתקיפה או סגור בתעופה רגילה!
        if (attacking) {
            drawLine(buf, x + 3, y + 6, x - 3, y + 4, yellow);
            drawLine(buf, x + 3, y + 9, x - 3, y + 11, yellow);
        } else {
            drawFramedRect(buf, x, y + 7, 5, 4, yellow, black);
        }
        drawRect(buf, x + 5, y + 6, 2, 2, attacking ? red : white);
        drawShadedCircle(buf, x + 13, y + 9 + flap / 2, 5, wingCol, bodyLight, bodyBase, black);
        return;
    }

    // 2. אויבים קרקעיים ברזולוציה גבוהה לפי כל ביומה!
    switch (o->type) {
        case OBS_CACTUS: {
            u16 cDark = C16(22, 115, 45), cMid = C16(42, 168, 68), cLight = C16(95, 220, 105);
            drawFramedRect(buf, x + 7, y, 10, 26, cMid, black);
            drawRect(buf, x + 9, y + 2, 2, 22, cLight);
            drawRect(buf, x + 13, y + 2, 2, 22, cDark);
            drawFramedRect(buf, x + 1, y + 8, 7, 5, cMid, black);
            drawFramedRect(buf, x + 1, y + 3, 5, 8, cMid, black);
            drawFramedRect(buf, x + 16, y + 11, 7, 5, cMid, black);
            drawFramedRect(buf, x + 18, y + 5, 5, 9, cMid, black);
            drawShadedCircle(buf, x + 12, y - 2, 3, red, C16(255, 130, 130), C16(160, 15, 15), black);
            drawRect(buf, x + 9,  y + 7, 2, 3, black);
            drawRect(buf, x + 13, y + 7, 2, 3, black);
            drawRect(buf, x + 9,  y + 7, 1, 1, yellow);
            drawRect(buf, x + 13, y + 7, 1, 1, yellow);
            break;
        }
        case OBS_LOLLIPOP: {
            drawFramedRect(buf, x + 10, y + 12, 4, 14, white, black);
            drawShadedCircle(buf, x + 12, y + 9, 10, C16(255, 25, 145), C16(255, 140, 205), C16(185, 10, 95), black);
            drawCircle(buf, x + 12, y + 9, 7, white);
            drawCircle(buf, x + 12, y + 9, 4, C16(0, 225, 255));
            drawCircle(buf, x + 12, y + 9, 2, yellow);
            drawFramedRect(buf, x + 7,  y + 6, 4, 4, white, black);
            drawFramedRect(buf, x + 13, y + 6, 4, 4, white, black);
            drawRect(buf, x + 8,  y + 7, 2, 2, attacking ? red : black);
            drawRect(buf, x + 14, y + 7, 2, 2, attacking ? red : black);
            break;
        }
        case OBS_SNAKE: { // נחש קוברה מפותל שמזנק קדימה ופושט לשון וניבים בתקיפה!
            u16 sGreen = C16(48, 185, 45), sLight = C16(120, 235, 85), sDark = C16(25, 115, 25);
            drawShadedCircle(buf, x + 16, y + 20, 5, sGreen, sLight, sDark, black);
            drawShadedCircle(buf, x + 9,  y + 20, 6, sGreen, sLight, sDark, black);
            drawShadedCircle(buf, x + 10, y + 12, 6, C16(220, 180, 60), yellow, C16(150, 100, 20), black);
            drawShadedCircle(buf, x + 8,  y + 7,  6, sGreen, sLight, sDark, black);
            drawRect(buf, x + 5, y + 5, 3, 2, red);
            drawRect(buf, x + 4, y + 11, 2, attacking ? 5 : 3, white);
            int tongueLen = attacking ? 7 : 3;
            drawLine(buf, x + 2, y + 9, x - tongueLen, y + 7, red);
            drawLine(buf, x + 2, y + 9, x - tongueLen, y + 11, red);
            break;
        }
        case OBS_MAGMA: {
            drawShadedCircle(buf, x + 12, y + 14, 11, C16(215, 45, 0), C16(255, 135, 0), C16(120, 15, 0), black);
            drawShadedCircle(buf, x + 12, y + 6,  8,  C16(235, 65, 0), C16(255, 165, 20), C16(135, 20, 0), black);
            drawRect(buf, x + 7,  y + 6, 4, 3, yellow);
            drawRect(buf, x + 13, y + 6, 4, 3, yellow);
            drawRect(buf, x + 8,  y + 13, 8, attacking ? 5 : 3, yellow);
            break;
        }
        case OBS_PENGUIN: {
            drawRect(buf, x + 5,  y + 23, 5, 3, C16(255, 140, 0));
            drawRect(buf, x + 14, y + 23, 5, 3, C16(255, 140, 0));
            drawShadedCircle(buf, x + 12, y + 15, 9, C16(35, 45, 65), C16(75, 90, 120), black, black);
            drawShadedCircle(buf, x + 12, y + 7,  7, C16(35, 45, 65), C16(75, 90, 120), black, black);
            drawCircle(buf, x + 11, y + 16, 6, white);
            drawRect(buf, x + 8, y + 5, 2, 2, attacking ? red : white);
            drawFramedRect(buf, x + (attacking ? 0 : 2), y + 7, 6, 4, C16(255, 150, 0), black);
            break;
        }
        case OBS_RED_SHROOM: {
            drawFramedRect(buf, x + 6, y + 12, 12, 14, C16(245, 230, 200), black);
            drawShadedCircle(buf, x + 12, y + 9, 11, red, C16(255, 110, 110), C16(165, 15, 15), black);
            drawCircle(buf, x + 7,  y + 6, 3, white);
            drawCircle(buf, x + 16, y + 7, 3, white);
            drawCircle(buf, x + 12, y + 3, 2, white);
            drawRect(buf, x + 8,  y + 15, 2, 4, black);
            drawRect(buf, x + 14, y + 15, 2, 4, black);
            if (attacking) drawRect(buf, x + 9, y + 20, 6, 3, red);
            break;
        }
        case OBS_GRAVE: {
            drawShadedCircle(buf, x + 12, y + 8, 9, C16(135, 142, 150), C16(185, 192, 200), C16(85, 90, 98), black);
            drawFramedRect(buf, x + 3, y + 8, 18, 18, C16(135, 142, 150), black);
            drawRect(buf, x + 4, y + 9, 3, 15, C16(180, 188, 195));
            drawRect(buf, x + 11, y + 5, 2, 11, C16(55, 60, 68));
            drawRect(buf, x + 8,  y + 8, 8, 2,  C16(55, 60, 68));
            drawRect(buf, x + 2, y + 23, 20, 3, C16(45, 130, 50));
            break;
        }
        case OBS_PUMPKIN: {
            drawFramedRect(buf, x + 10, y, 4, 5, C16(40, 135, 45), black);
            drawShadedCircle(buf, x + 8,  y + 14, 9, C16(235, 110, 0), C16(255, 165, 40), C16(165, 65, 0), black);
            drawShadedCircle(buf, x + 16, y + 14, 9, C16(235, 110, 0), C16(255, 165, 40), C16(165, 65, 0), black);
            drawShadedCircle(buf, x + 12, y + 14, 9, C16(255, 130, 10), C16(255, 185, 60), C16(180, 75, 0), black);
            drawRect(buf, x + 6,  y + 10, 4, 4, attacking ? red : yellow);
            drawRect(buf, x + 14, y + 10, 4, 4, attacking ? red : yellow);
            drawRect(buf, x + 7,  y + 17, 10, attacking ? 5 : 3, yellow);
            break;
        }
        default: {
            u16 base = biomeObsCol;
            drawShadedCircle(buf, x + 12, y + 14, 11, base, white, black, black);
            drawShadedCircle(buf, x + 7,  y + 18, 7,  base, white, black, black);
            drawShadedCircle(buf, x + 17, y + 18, 7,  base, white, black, black);
            drawFramedRect(buf, x + 6,  y + 10, 5, 4, yellow, black);
            drawFramedRect(buf, x + 13, y + 10, 5, 4, yellow, black);
            drawRect(buf, x + 7,  y + 11, 2, 2, red);
            drawRect(buf, x + 14, y + 11, 2, 2, red);
            break;
        }
    }
}

extern Obstacle obstacles[];
extern Coin coins[];
extern BgElement bgElements[];
extern EnvParticle envParticles[];

void renderTopScreen(u16* buf) {
    const BiomeData* b = &BIOMES[currentBiomeIdx];
    u16 black = C16(15, 15, 20);
    u16 white = C16(255, 255, 255);

    // 1. שמיים עם מעבר הדרגתי ומציאותי בין ביומות!
    int stR = (int)smoothSkyTop[0], stG = (int)smoothSkyTop[1], stB = (int)smoothSkyTop[2];
    int sbR = (int)smoothSkyBot[0], sbG = (int)smoothSkyBot[1], sbB = (int)smoothSkyBot[2];
    int grR = (int)smoothGround[0], grG = (int)smoothGround[1], grB = (int)smoothGround[2];

    for (int y = 0; y < GROUND_Y; y++) {
        int r  = stR + ((sbR - stR) * y) / GROUND_Y;
        int g  = stG + ((sbG - stG) * y) / GROUND_Y;
        int bl = stB + ((sbB - stB) * y) / GROUND_Y;
        u16 rowCol = rgb8To16_fixed(r, g, bl);
        u16* row = &buf[y * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++) row[x] = rowCol;
    }

    // 2. שמש זהובה עם הילה או ירח עם מכתשים וכוכבים מנצנצים
    if (b->isNight) {
        drawShadedCircle(buf, 220, 34, 14, C16(240, 245, 255), white, C16(180, 195, 220), C16(100, 120, 160));
        drawCircle(buf, 215, 30, 3, C16(200, 210, 230));
        drawCircle(buf, 224, 38, 4, C16(200, 210, 230));
        for (int i = 0; i < 24; i++) {
            int sx = (i * 41 + frames / 4) % SCREEN_W;
            int sy = (i * 23) % (GROUND_Y - 40);
            buf[sy * SCREEN_W + sx] = white;
            if (i % 3 == 0 && sx + 1 < SCREEN_W && sy + 1 < SCREEN_H) {
                buf[sy * SCREEN_W + sx + 1] = C16(255, 255, 160);
            }
        }
    } else {
        drawCircle(buf, 220, 36, 22, C16(255, 240, 150));
        drawCircle(buf, 220, 36, 17, C16(255, 220, 60));
        drawCircle(buf, 220, 36, 13, C16(255, 250, 190));
    }

    // 3. שכבת הרים וגבעות רחוקים באופק (Parallax Layer) עם צבעים מתמזגים
    drawDistantHills(buf, frames / 2);

    // 4. אלמנטי תפאורה עשירים ברקע
    for (int i = 0; i < 6; i++) {
        if (bgElements[i].active) {
            drawSceneryElement(buf, &bgElements[i]);
        }
    }

    // 5. אדמה מפורטת עם מעבר צבעים הדרגתי בין ביומות!
    u16 topTurfLight = rgb8To16_fixed(grR + 45, grG + 55, grB + 35);
    u16 topTurfBase  = rgb8To16_fixed(grR + 15, grG + 25, grB + 15);
    u16 dirtDark     = rgb8To16_fixed(grR - 45, grG - 45, grB - 45);

    drawRect(buf, 0, GROUND_Y, SCREEN_W, 1, black);
    drawRect(buf, 0, GROUND_Y + 1, SCREEN_W, 3, topTurfLight);
    drawRect(buf, 0, GROUND_Y + 4, SCREEN_W, 4, topTurfBase);

    for (int y = GROUND_Y + 8; y < SCREEN_H; y++) {
        int depth = (y - (GROUND_Y + 8)) * 2;
        u16 gCol = rgb8To16_fixed(grR - 20 - depth, grG - 25 - depth, grB - 20 - depth);
        u16* row = &buf[y * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++) row[x] = gCol;
    }

    int groundScroll = (int)(frames * currentSpeed);
    for (int gx = 0; gx < SCREEN_W; gx += 8) {
        int worldX = gx + groundScroll;
        int overhang = ((worldX / 8) * 13) % 5;
        drawRect(buf, gx, GROUND_Y + 8, 6, 2 + overhang, topTurfBase);
        drawRect(buf, gx, GROUND_Y + 10 + overhang, 6, 1, dirtDark);

        if (((worldX / 8) % 3) == 0) {
            int py = GROUND_Y + 18 + (((worldX / 8) * 7) % 16);
            drawFramedRect(buf, gx, py, 6, 4, dirtDark, black);
        }
    }

    // 6. צעיף גיבור מתנופף עם מסגרת
    int px = (int)player.x + 12;
    int py = (int)player.y + 12;
    for (int i = 1; i < 6; i++) {
        int wave = ((frames / 3 + i) % 2 == 0) ? -2 : 2;
        drawFramedRect(buf, px - i * 5, py + wave - 2, 6, 5, white, C16(200, 40, 40));
    }

    // 7. ציור הדמות המשודרגת של השחקן עם 4 האנימציות והבעות הפנים!
    drawCharacterSpriteEx(buf, selectedCharIdx, (int)player.x + 12, (int)player.y + 12, 100, playerAnimState, currentFaceExpr, player.angle, frames);

    // 8. אויבים ומכשולים מפורטים (כולל אנימציית תקיפה לאויבים חיים!)
    for (int i = 0; i < 6; i++) {
        if (obstacles[i].active) {
            drawObstacleReal(buf, &obstacles[i], b->obsColor, frames);
        }
    }

    // 9. מטבעות זהב מנצנצים ויהלומים מלוטשים
    for (int i = 0; i < 6; i++) {
        if (coins[i].active) {
            int cx = (int)coins[i].x + 6;
            int cy = (int)coins[i].y + 6;
            drawShadedCircle(buf, cx, cy, 6, coins[i].color, white, C16(180, 120, 10), black);
            drawRect(buf, cx - 1, cy - 3, 2, 6, white);
        }
    }

    // 10. חלקיקי מזג אוויר
    for (int i = 0; i < 25; i++) {
        if (envParticles[i].life > 0) {
            drawFramedRect(buf, (int)envParticles[i].x, (int)envParticles[i].y, envParticles[i].size + 1, envParticles[i].size + 1, envParticles[i].color, black);
        }
    }

    // 11. תצוגת עלית (HUD) מעוצבת עם מסגרות כפולות
    drawFramedRect(buf, 4, 4, 85, 16, C16(20, 28, 45), white);
    drawHebrewString(buf, 46, 8, b->nameHeb, white, 1);

    drawFramedRect(buf, 95, 4, 72, 16, C16(20, 28, 45), white);
    drawNumber(buf, 104, 8, score / 10, white, 1);

    drawFramedRect(buf, 173, 4, 78, 16, C16(20, 28, 45), white);
    drawShadedCircle(buf, 183, 12, 5, C16(255, 215, 0), white, C16(200, 130, 0), black);
    drawNumber(buf, 193, 8, coinsCount, C16(255, 215, 0), 1);

    // 12. בועת הודעה מעוצבת
    if (bubbleTimer > 0 && bubbleText != NULL) {
        drawFramedRect(buf, 54, 34, 148, 20, white, black);
        drawHebrewString(buf, 128, 40, bubbleText, black, 1);
    }
    if (gameState == STATE_COUNTDOWN) {
        drawShadedCircle(buf, 128, 96, 22, C16(25, 35, 65), C16(60, 80, 140), black, white);
        drawNumber(buf, 122, 88, countdownVal, C16(255, 225, 0), 2);
    }
}
 
