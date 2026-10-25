#include "game.h"
#include <string.h>
#include <stdlib.h>

#undef C16
#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

CustomCharData customChar;
u32 unlockedCustomRewards = 0; // ביטים של פריטים מיוחדים שנפתחו במשימות ושבירת שיאים
int currentMissionId = 0;
bool currentMissionDone = false;

// 14 קטגוריות מלאות לבחירה בעורך הדמויות!
static int activeCategory = 0;
static u8 kbGlyphs[10] = {1, 10, 26, 10, 0}; // ברירת מחדל: "אישי"
static int kbLen = 4;

static const char* CAT_NAMES[CUSTOM_PARTS_COUNT] = {
    "פנים וראש",      // 0
    "זקן ושפם",       // 1
    "עיניים",         // 2
    "גוף ושריון",     // 3
    "ידיים ונשק",     // 4
    "רגליים",         // 5
    "נעליים",         // 6
    "כובעים",         // 7
    "גבות",           // 8
    "קמירות והבעה",   // 9
    "אפים",           // 10
    "פיות וחיוכים",   // 11
    "שיער ותסרוקת",   // 12
    "גלימה וכנפיים"   // 13
};

// משימות זמניות למשחק אחד שמעניקות חלקי הרכבה אגדיים!
static const char* MISSION_DESCS[6] = {
    "משימה: הגע לממתקים לראש עוגיה!",
    "משימה: הגע למדבר לכובע פרעה!",
    "משימה: הגע להר געש לשריון לבה!",
    "משימה: הגע לחלל לקסדת חלל!",
    "משימה: צבור 500 נק׳ לזקן קוסם!",
    "משימה: שבור שיא לנעלי סילון!"
};

// טבלת UTF-8 לאותיות עבריות (1..27) עבור המקלדת
static const unsigned char HEB_UTF8_MAP[28][2] = {
    {0, 0},
    {0xD7, 0x90}, {0xD7, 0x91}, {0xD7, 0x92}, {0xD7, 0x93}, {0xD7, 0x94},
    {0xD7, 0x95}, {0xD7, 0x96}, {0xD7, 0x97}, {0xD7, 0x98}, {0xD7, 0x99},
    {0xD7, 0x9A}, {0xD7, 0x9B}, {0xD7, 0x9C}, {0xD7, 0x9D}, {0xD7, 0x9E},
    {0xD7, 0x9F}, {0xD7, 0xA0}, {0xD7, 0xA1}, {0xD7, 0xA2}, {0xD7, 0xA3},
    {0xD7, 0xA4}, {0xD7, 0xA5}, {0xD7, 0xA6}, {0xD7, 0xA7}, {0xD7, 0xA8},
    {0xD7, 0xA9}, {0xD7, 0xAA}
};

static const char* HEB_LETTER_LABELS[28] = {
    " ", "א","ב","ג","ד","ה","ו","ז","ח","ט",
    "י","ך","כ","ל","ם","מ","ן","נ","ס","ע",
    "ף","פ","ץ","צ","ק","ר","ש","ת"
};

// פלטת 64 צבעים עשירה ומגוונת לכל איבר בהרכבה האישית
u16 getCustomPaletteColor(int colIdx) {
    int idx = colIdx & 63;
    int shade = (idx / 16); // 4 דרגות בהירות לכל גוון!
    int boost = (shade == 1) ? 30 : ((shade == 2) ? -30 : ((shade == 3) ? -60 : 0));
    int r = 200, g = 200, b = 200;
    switch (idx % 16) {
        case 0:  r = 250; g = 202; b = 150; break; // עור בהיר
        case 1:  r = 210; g = 140; b = 85;  break; // עוגייה / עור שזוף
        case 2:  r = 235; g = 45;  b = 45;  break; // אדום בוהק
        case 3:  r = 46;  g = 204; b = 113; break; // ירוק אמרלד
        case 4:  r = 52;  g = 152; b = 219; break; // כחול שמיים
        case 5:  r = 241; g = 196; b = 15;  break; // זהב מלכותי
        case 6:  r = 155; g = 89;  b = 182; break; // סגול קסם
        case 7:  r = 0;   g = 235; b = 255; break; // טורקיז ניאון
        case 8:  r = 255; g = 120; b = 30;  break; // כתום לבה
        case 9:  r = 255; g = 90;  b = 180; break; // ורוד ממתק
        case 10: r = 245; g = 245; b = 250; break; // לבן שלג
        case 11: r = 40;  g = 45;  b = 58;  break; // שחור פחם
        case 12: r = 140; g = 90;  b = 45;  break; // חום שוקולד
        case 13: r = 185; g = 195; b = 210; break; // כסף מתכתי
        case 14: r = 118; g = 255; b = 3;   break; // ירוק האקר
        default: r = 90;  g = 30;  b = 120; break; // סגול כהה
    }
    r += boost; g += boost; b += boost;
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return C16(r, g, b);
}

static void syncGlyphsToUtf8Name(void) {
    int pos = 0;
    for (int i = 0; i < kbLen && pos < 20; i++) {
        u8 g = kbGlyphs[i];
        if (g >= 1 && g <= 27) {
            customChar.nameHeb[pos++] = (char)HEB_UTF8_MAP[g][0];
            customChar.nameHeb[pos++] = (char)HEB_UTF8_MAP[g][1];
        } else if (g == 0) {
            customChar.nameHeb[pos++] = ' ';
        }
    }
    if (pos == 0) {
        strcpy(customChar.nameHeb, "אישי");
    } else {
        customChar.nameHeb[pos] = '\0';
    }
}

void initDefaultCustomChar(void) {
    if (customChar.nameHeb[0] == '\0') {
        strcpy(customChar.nameHeb, "אישי");
        customChar.partStyle[0]  = 0; customChar.partColor[0]  = 0;  // 0: פנים
        customChar.partStyle[1]  = 0; customChar.partColor[1]  = 12; // 1: זקן (0=ללא)
        customChar.partStyle[2]  = 0; customChar.partColor[2]  = 4;  // 2: עיניים
        customChar.partStyle[3]  = 1; customChar.partColor[3]  = 2;  // 3: גוף
        customChar.partStyle[4]  = 1; customChar.partColor[4]  = 0;  // 4: ידיים
        customChar.partStyle[5]  = 1; customChar.partColor[5]  = 4;  // 5: רגליים
        customChar.partStyle[6]  = 1; customChar.partColor[6]  = 12; // 6: נעליים
        customChar.partStyle[7]  = 0; customChar.partColor[7]  = 5;  // 7: כובע (0=ללא כובע, מציג שיער!)
        customChar.partStyle[8]  = 1; customChar.partColor[8]  = 11; // 8: גבות
        customChar.partStyle[9]  = 0; customChar.partColor[9]  = 2;  // 9: קמירות גבות ופה
        customChar.partStyle[10] = 1; customChar.partColor[10] = 1;  // 10: אפים
        customChar.partStyle[11] = 1; customChar.partColor[11] = 2;  // 11: פיות
        customChar.partStyle[12] = 2; customChar.partColor[12] = 12; // 12: שיער ותסרוקת
        customChar.partStyle[13] = 0; customChar.partColor[13] = 2;  // 13: גלימה וכנפיים
        customChar.created = false;
    }
}

const char* getActiveMissionDesc(void) {
    return MISSION_DESCS[currentMissionId % 6];
}

void pickNewTempMission(void) {
    currentMissionId = rand() % 6;
    currentMissionDone = false;
}

void checkMissionAndRecordUnlocks(int dispScore, int biomeIdx, bool brokeRecord) {
    if (!currentMissionDone) {
        bool completed = false;
        if (currentMissionId == 0 && biomeIdx >= 3) completed = true; // הגעה לממתקים -> ראש עוגייה!
        if (currentMissionId == 1 && biomeIdx >= 1) completed = true; // הגעה למדבר -> כובע פרעה!
        if (currentMissionId == 2 && biomeIdx >= 4) completed = true; // הגעה להר געש -> שריון לבה!
        if (currentMissionId == 3 && biomeIdx >= 5) completed = true; // הגעה לחלל -> קסדת חלל!
        if (currentMissionId == 4 && dispScore >= 500) completed = true; // 500 נקודות -> זקן קוסם!
        if (currentMissionId == 5 && brokeRecord && dispScore >= 100) completed = true; // שבירת שיא -> נעלי סילון!

        if (completed) {
            currentMissionDone = true;
            unlockedCustomRewards |= (1 << currentMissionId);
            if (currentMissionId == 0) {
                customChar.partStyle[0] = 199;
                customChar.partColor[0] = 1;
                bubbleText = "נפתח ראש איש עוגיה!";
            } else {
                bubbleText = "משימה הושלמה! פרס נפתח!";
            }
            bubbleTimer = 160;
            savePermanentData();
        }
    }

    if (brokeRecord && dispScore >= 150) {
        unlockedCustomRewards |= (1 << 6);
        savePermanentData();
    }
}

// ציור הדמות בהרכבה אישית בתמיכה מלאה בכל סקאלה (100%=רגיל, 200%=בינוני, 300%=תקריב זום אוטומטי לפנים ולקמירות!)
void drawCustomCharacterSprite(u16* buf, int cx, int cy, int scalePct, enum CharAnimState animState, enum FaceExpr faceExpr, int frameCnt) {
    int s = (scalePct >= 250) ? 3 : ((scalePct >= 120) ? 2 : 1);
    u16 black = C16(18, 18, 24);
    u16 white = C16(252, 252, 255);
    u16 gold  = C16(255, 215, 0);
    u16 red   = C16(235, 40, 40);

    bool isMoving = (gameState == STATE_PLAYING || gameState == STATE_SHOP || (gameState == STATE_CUSTOMIZE && s <= 2));
    int phase = isMoving ? ((frameCnt / 3) % 4) : 0;
    int legL = (phase == 0) ? -4 : ((phase == 2) ? 4 : 0);
    int legR = -legL;
    int bobY = (isMoving && (phase % 2 == 0)) ? -1 : 0;
    if (animState == CHAR_ANIM_JUMP) { bobY = -3; legL = -4; legR = 4; }
    else if (animState == CHAR_ANIM_HIT_GROUND || animState == CHAR_ANIM_HIT_AIR) { bobY = 2; faceExpr = EXPR_HURT; }

    int x = cx;
    int y = cy + bobY * s;

    u16 faceCol  = getCustomPaletteColor(customChar.partColor[0]);
    u16 beardCol = getCustomPaletteColor(customChar.partColor[1]);
    u16 eyeCol   = getCustomPaletteColor(customChar.partColor[2]);
    u16 bodyCol  = getCustomPaletteColor(customChar.partColor[3]);
    u16 handCol  = getCustomPaletteColor(customChar.partColor[4]);
    u16 legCol   = getCustomPaletteColor(customChar.partColor[5]);
    u16 shoeCol  = getCustomPaletteColor(customChar.partColor[6]);
    u16 hatCol   = getCustomPaletteColor(customChar.partColor[7]);
    u16 browCol  = getCustomPaletteColor(customChar.partColor[8]);
    u16 noseCol  = getCustomPaletteColor(customChar.partColor[10]);
    u16 mouthCol = getCustomPaletteColor(customChar.partColor[11]);
    u16 hairCol  = getCustomPaletteColor(customChar.partColor[12]);
    u16 backCol  = getCustomPaletteColor(customChar.partColor[13]);

    int faceSt  = customChar.partStyle[0];
    int beardSt = customChar.partStyle[1];
    int eyeSt   = customChar.partStyle[2];
    int bodySt  = customChar.partStyle[3];
    int handSt  = customChar.partStyle[4];
    int legSt   = customChar.partStyle[5];
    int shoeSt  = customChar.partStyle[6];
    int hatSt   = customChar.partStyle[7];
    int browSt  = customChar.partStyle[8];
    int curveSt = customChar.partStyle[9];  // קמירות גבות ופה!
    int noseSt  = customChar.partStyle[10]; // אפים!
    int mouthSt = customChar.partStyle[11]; // פיות!
    int hairSt  = customChar.partStyle[12]; // שיער ותסרוקת!
    int backSt  = customChar.partStyle[13]; // גלימה/כנפיים/אביזר גב!

    // 0. גלימות, כנפיים ואביזרי גב מותאמים אישית (קטגוריה 13)
    if (backSt > 0 || bodySt % 5 == 4) {
        int bMode = (backSt > 0) ? (backSt % 4) : 1;
        if (bMode == 1) { // גלימת מלכות/גיבור
            drawRect(buf, x - 11 * s, y - 5 * s, 22 * s, 16 * s, backCol);
        } else if (bMode == 2) { // כנפי מלאך/דרקון פרוסות!
            drawCircle(buf, x - 12 * s, y - 3 * s, 6 * s, backCol);
            drawCircle(buf, x + 12 * s, y - 3 * s, 6 * s, backCol);
        } else if (bMode == 3) { // תרמיל סילון (Jetpack)
            drawRect(buf, x - 9 * s, y - 4 * s, 18 * s, 9 * s, C16(160, 170, 185));
            drawCircle(buf, x - 8 * s, y + 6 * s, 2 * s, gold);
            drawCircle(buf, x + 8 * s, y + 6 * s, 2 * s, gold);
        }
    }

    // 1. רגליים מותאמות אישית
    int legW = (legSt % 3 == 0) ? 6 : 5;
    drawRect(buf, x - (6 - legL / 2) * s, y + 5 * s, legW * s, 7 * s, black);
    drawRect(buf, x - (5 - legL / 2) * s, y + 5 * s, (legW - 2) * s, 6 * s, legCol);
    drawRect(buf, x + (1 + legR / 2) * s, y + 5 * s, legW * s, 7 * s, black);
    drawRect(buf, x + (2 + legR / 2) * s, y + 5 * s, (legW - 2) * s, 6 * s, legCol);
    if (legSt % 2 == 1) {
        drawRect(buf, x - (5 - legL / 2) * s, y + 8 * s, (legW - 2) * s, 1 * s, white);
        drawRect(buf, x + (2 + legR / 2) * s, y + 8 * s, (legW - 2) * s, 1 * s, white);
    }

    // 2. נעליים מותאמות אישית
    int shoeW = 6 + (shoeSt % 3);
    drawRect(buf, x - (7 - legL / 2) * s, y + 11 * s, shoeW * s, 4 * s, black);
    drawRect(buf, x - (6 - legL / 2) * s, y + 12 * s, (shoeW - 2) * s, 2 * s, shoeCol);
    drawRect(buf, x + (1 + legR / 2) * s, y + 11 * s, shoeW * s, 4 * s, black);
    drawRect(buf, x + (2 + legR / 2) * s, y + 12 * s, (shoeW - 2) * s, 2 * s, shoeCol);
    if (shoeSt % 4 == 3) {
        drawRect(buf, x - (6 - legL / 2) * s, y + 15 * s, 3 * s, 2 * s, gold);
        drawRect(buf, x + (2 + legR / 2) * s, y + 15 * s, 3 * s, 2 * s, red);
    }

    // 3. גוף ושריון מותאמים אישית
    drawRect(buf, x - 7 * s, y - 4 * s, 14 * s, 10 * s, black);
    drawRect(buf, x - 6 * s, y - 3 * s, 12 * s, 8 * s, bodyCol);
    if (bodySt % 3 == 1) {
        drawRect(buf, x - 6 * s, y + 2 * s, 12 * s, 2 * s, gold);
    } else if (bodySt % 3 == 2) {
        drawCircle(buf, x, y, 3 * s, white);
    }
    if (bodySt >= 100) {
        drawCircle(buf, x - 7 * s, y - 3 * s, 3 * s, gold);
        drawCircle(buf, x + 7 * s, y - 3 * s, 3 * s, gold);
    }

    // 4. ידיים ונשקים
    drawRect(buf, x - 11 * s, y - 3 * s, 4 * s, 7 * s, black);
    drawRect(buf, x - 10 * s, y - 2 * s, 2 * s, 5 * s, bodyCol);
    drawRect(buf, x + 7 * s,  y - 3 * s, 4 * s, 7 * s, black);
    drawRect(buf, x + 8 * s,  y - 2 * s, 2 * s, 5 * s, bodyCol);
    drawCircle(buf, x - 9 * s, y + 4 * s, 2 * s, handCol);
    drawCircle(buf, x + 9 * s, y + 4 * s, 2 * s, handCol);
    if (handSt % 4 == 1) {
        drawLine(buf, x + 9 * s, y + 3 * s, x + 15 * s, y - 10 * s, C16(0, 255, 255));
    } else if (handSt % 4 == 2) {
        drawCircle(buf, x - 11 * s, y + 2 * s, 4 * s, gold);
    } else if (handSt % 4 == 3) {
        drawLine(buf, x + 11 * s, y + 10 * s, x + 11 * s, y - 12 * s, gold);
        drawCircle(buf, x + 11 * s, y - 13 * s, 3 * s, red);
    }

    // 5. פנים וראש
    int hx = x;
    int hy = y - 10 * s;

    // אם יש תסרוקת שיער ארוכה/אפרו מאחורי הראש (כשללא כובע או מציץ)
    if (hatSt == 0 && hairSt > 0 && (hairSt % 6 == 4)) {
        drawCircle(buf, hx, hy - 2 * s, 10 * s, hairCol);
    }

    if (faceSt == 199) {
        // ראש איש עוגייה (Gingerbread Cookie Head)
        drawCircle(buf, hx, hy, 9 * s, C16(90, 45, 15));
        drawCircle(buf, hx, hy, 8 * s, C16(195, 125, 60));
        drawCircle(buf, hx - 5 * s, hy - 5 * s, 2 * s, white);
        drawCircle(buf, hx,         hy - 6 * s, 2 * s, C16(255, 105, 180));
        drawCircle(buf, hx + 5 * s, hy - 5 * s, 2 * s, white);
    } else {
        int headR = 7 + (faceSt % 2);
        drawCircle(buf, hx, hy, (headR + 1) * s, black);
        drawCircle(buf, hx, hy, headR * s, faceCol);
        if (faceSt % 5 == 2) { // אוזני אלף/חיה
            drawCircle(buf, hx - 8 * s, hy - 1 * s, 2 * s, faceCol);
            drawCircle(buf, hx + 8 * s, hy - 1 * s, 2 * s, faceCol);
        } else if (faceSt % 5 == 4) { // צלקת קרב על הלחי
            drawLine(buf, hx - 5 * s, hy - 3 * s, hx - 4 * s, hy + 2 * s, red);
        }
    }

    // 6. עיניים ומשקפיים
    if (faceExpr == EXPR_SCARED) {
        drawRect(buf, hx - 5 * s, hy - 2 * s, 4 * s, 4 * s, white);
        drawRect(buf, hx + 2 * s, hy - 2 * s, 4 * s, 4 * s, white);
        drawRect(buf, hx - 4 * s, hy - 1 * s, 2 * s, 2 * s, black);
        drawRect(buf, hx + 3 * s, hy - 1 * s, 2 * s, 2 * s, black);
    } else if (faceExpr == EXPR_HURT) {
        drawLine(buf, hx - 5 * s, hy - 2 * s, hx - 2 * s, hy + 1 * s, black);
        drawLine(buf, hx - 2 * s, hy - 2 * s, hx - 5 * s, hy + 1 * s, black);
        drawLine(buf, hx + 2 * s, hy - 2 * s, hx + 5 * s, hy + 1 * s, black);
        drawLine(buf, hx + 5 * s, hy - 2 * s, hx + 2 * s, hy + 1 * s, black);
    } else {
        if (eyeSt % 5 == 1) { // משקפי שמש / סייבר
            drawRect(buf, hx - 6 * s, hy - 2 * s, 12 * s, 3 * s, eyeCol);
            drawRect(buf, hx - 6 * s, hy - 3 * s, 12 * s, 1 * s, black);
        } else if (eyeSt % 5 == 2) { // עין קיקלופ ענקית
            drawCircle(buf, hx, hy - 1 * s, 3 * s, white);
            drawCircle(buf, hx, hy - 1 * s, 1 * s, eyeCol);
        } else if (eyeSt % 5 == 3) { // עיניים מנצנצות עם ריסים ארוכים
            drawRect(buf, hx - 5 * s, hy - 2 * s, 3 * s, 3 * s, white);
            drawRect(buf, hx + 2 * s, hy - 2 * s, 3 * s, 3 * s, white);
            drawRect(buf, hx - 4 * s, hy - 1 * s, 2 * s, 2 * s, eyeCol);
            drawRect(buf, hx + 3 * s, hy - 1 * s, 2 * s, 2 * s, eyeCol);
            drawRect(buf, hx - 5 * s, hy - 2 * s, 1 * s, 1 * s, white);
            drawRect(buf, hx + 2 * s, hy - 2 * s, 1 * s, 1 * s, white);
        } else {
            drawRect(buf, hx - 5 * s, hy - 2 * s, 3 * s, 3 * s, white);
            drawRect(buf, hx + 2 * s, hy - 2 * s, 3 * s, 3 * s, white);
            drawRect(buf, hx - 4 * s, hy - 1 * s, 2 * s, 2 * s, eyeCol);
            drawRect(buf, hx + 3 * s, hy - 1 * s, 2 * s, 2 * s, eyeCol);
        }
    }

    // 7. גבות וקמירות הגבות (קטגוריות 8 ו-9!)
    int browCurve = curveSt % 5;
    int bthick = (browSt % 3 == 2) ? (2 * s) : (1 * s);
    int byL_out = hy - 4 * s, byL_in = hy - 4 * s;
    int byR_in  = hy - 4 * s, byR_out = hy - 4 * s;

    if (browCurve == 1)      { byL_out += s; byR_out += s; } // גבות קמורות שמחות
    else if (browCurve == 2) { byL_in  += s; byR_in  += s; byL_out -= s; byR_out -= s; } // גבות זועמות אלכסוניות
    else if (browCurve == 3) { byL_out += s; byR_out += s; byL_in  -= s; byR_in  -= s; } // גבות עצובות/מופתעות
    else if (browCurve == 4) { byL_out -= s; byL_in  -= s; byR_in  += s; }               // גבה אחת מורמת בסטייל!

    if (browSt % 4 == 3) { // חד-גבה מחוברת!
        drawLine(buf, hx - 6 * s, byL_out, hx, byL_in, browCol);
        drawLine(buf, hx, byR_in, hx + 6 * s, byR_out, browCol);
    } else {
        drawLine(buf, hx - 6 * s, byL_out, hx - 2 * s, byL_in, browCol);
        drawLine(buf, hx + 2 * s, byR_in,  hx + 6 * s, byR_out, browCol);
        if (bthick > s) {
            drawLine(buf, hx - 6 * s, byL_out - 1, hx - 2 * s, byL_in - 1, browCol);
            drawLine(buf, hx + 2 * s, byR_in - 1,  hx + 6 * s, byR_out - 1, browCol);
        }
    }

    // 8. אפים מותאמים אישית (קטגוריה 10: קטן, כפתור, ליצן עגול, מחודד, רחב, משולש!)
    int nMode = noseSt % 6;
    if (nMode == 1) { // אף כפתור חמוד
        drawRect(buf, hx - 1 * s, hy, 2 * s, 2 * s, noseCol);
    } else if (nMode == 2) { // אף ליצן עגול ובולט!
        drawCircle(buf, hx, hy + 1 * s, 2 * s, noseCol);
    } else if (nMode == 3) { // אף ארוך/מחודד
        drawLine(buf, hx, hy - 1 * s, hx + 2 * s, hy + 2 * s, noseCol);
    } else if (nMode == 4) { // אף רחב
        drawRect(buf, hx - 2 * s, hy + 1 * s, 4 * s, 2 * s, noseCol);
    } else if (nMode == 5) { // חוטם חיה משולש
        drawRect(buf, hx - 2 * s, hy, 4 * s, 2 * s, white);
        drawRect(buf, hx - 1 * s, hy, 2 * s, 1 * s, noseCol);
    }

    // 9. פיות, חיוכים וקמירות הפה (קטגוריות 11 ו-9!)
    int mMode = mouthSt % 7;
    int mouthCurve = (curveSt / 5) % 3; // 0=חיוך למעלה, 1=ישר, 2=קמור למטה
    int my = hy + 4 * s;

    if (faceExpr == EXPR_SCARED) {
        drawCircle(buf, hx, my, 2 * s, black);
        drawCircle(buf, hx, my, 1 * s, red);
    } else if (mMode == 0) { // חיוך מותאם לפי הקמירות!
        drawRect(buf, hx - 3 * s, my, 6 * s, 1 * s, mouthCol);
        if (mouthCurve == 0) { // פינות חיוך עולות למעלה
            drawRect(buf, hx - 4 * s, my - 1 * s, 1 * s, 1 * s, mouthCol);
            drawRect(buf, hx + 3 * s, my - 1 * s, 1 * s, 1 * s, mouthCol);
        } else if (mouthCurve == 2) { // פינות פה יורדות למטה
            drawRect(buf, hx - 4 * s, my + 1 * s, 1 * s, 1 * s, mouthCol);
            drawRect(buf, hx + 3 * s, my + 1 * s, 1 * s, 1 * s, mouthCol);
        }
    } else if (mMode == 1) { // חיוך רחב עם שיניים לבנות בולטות!
        drawRect(buf, hx - 4 * s, my - 1 * s, 8 * s, 3 * s, black);
        drawRect(buf, hx - 3 * s, my - 1 * s, 6 * s, 1 * s, white);
        drawRect(buf, hx - 2 * s, my + 1 * s, 4 * s, 1 * s, mouthCol);
    } else if (mMode == 2) { // פה עם ניבים של ערפד/מפלצת!
        drawRect(buf, hx - 4 * s, my, 8 * s, 2 * s, mouthCol);
        drawRect(buf, hx - 3 * s, my + 1 * s, 1 * s, 2 * s, white);
        drawRect(buf, hx + 2 * s, my + 1 * s, 1 * s, 2 * s, white);
    } else if (mMode == 3) { // פה שורק / מופתע עגול ('O')
        drawCircle(buf, hx, my, 2 * s, black);
        drawCircle(buf, hx, my, 1 * s, mouthCol);
    } else if (mMode == 4) { // פה עם לשון בחוץ!
        drawRect(buf, hx - 3 * s, my, 6 * s, 1 * s, black);
        drawRect(buf, hx - 1 * s, my + 1 * s, 3 * s, 2 * s, C16(255, 100, 140));
    } else if (mMode == 5) { // חיוך חתולי גלי (:3)
        drawLine(buf, hx - 3 * s, my, hx, my + 1 * s, mouthCol);
        drawLine(buf, hx, my + 1 * s, hx + 3 * s, my, mouthCol);
    } else { // חיוך צדדי מתוחכם (Smirk)
        drawLine(buf, hx - 3 * s, my + 1 * s, hx + 3 * s, my - 1 * s, mouthCol);
    }

    // 10. זקן ושפם (קטגוריה 1)
    if (beardSt > 0) {
        int bType = beardSt % 4;
        if (bType == 1) { // שפם אצילי
            drawRect(buf, hx - 5 * s, hy + 2 * s, 10 * s, 2 * s, beardCol);
        } else if (bType == 2) { // זקן צרפתי
            drawRect(buf, hx - 3 * s, hy + 5 * s, 6 * s, 3 * s, beardCol);
        } else if (bType == 3) { // זקן קוסם ענק
            drawCircle(buf, hx, hy + 6 * s, 5 * s, beardCol);
            drawCircle(buf, hx, hy + 10 * s, 3 * s, beardCol);
        } else { // זקן מלא
            drawRect(buf, hx - 6 * s, hy + 3 * s, 12 * s, 5 * s, beardCol);
        }
    }

    // 11. שיער ותסרוקות במידה שאין כובע (קטגוריה 12!) או כובעים (קטגוריה 7)
    if (hatSt == 0) {
        if (hairSt > 0) {
            int hrMode = hairSt % 6;
            if (hrMode == 1) { // קוצים בסגנון אנימה / סופר-סאייה!
                drawRect(buf, hx - 7 * s, hy - 9 * s, 14 * s, 3 * s, hairCol);
                drawCircle(buf, hx - 5 * s, hy - 11 * s, 2 * s, hairCol);
                drawCircle(buf, hx,         hy - 12 * s, 3 * s, hairCol);
                drawCircle(buf, hx + 5 * s, hy - 11 * s, 2 * s, hairCol);
            } else if (hrMode == 2) { // מוהוק פאנק גבוה במרכז הראש!
                drawRect(buf, hx - 2 * s, hy - 14 * s, 4 * s, 7 * s, hairCol);
                drawRect(buf, hx - 1 * s, hy - 15 * s, 2 * s, 2 * s, gold);
            } else if (hrMode == 3) { // שיער גולש ארוך בצדדים + פוני!
                drawRect(buf, hx - 8 * s, hy - 9 * s, 16 * s, 3 * s, hairCol);
                drawRect(buf, hx - 9 * s, hy - 7 * s, 3 * s, 10 * s, hairCol);
                drawRect(buf, hx + 6 * s, hy - 7 * s, 3 * s, 10 * s, hairCol);
            } else if (hrMode == 4) { // תסרוקת אפרו נפוחה
                drawCircle(buf, hx, hy - 8 * s, 6 * s, hairCol);
                drawCircle(buf, hx - 5 * s, hy - 6 * s, 4 * s, hairCol);
                drawCircle(buf, hx + 5 * s, hy - 6 * s, 4 * s, hairCol);
            } else if (hrMode == 5) { // קוקו סמוראי / נינג'ה למעלה
                drawRect(buf, hx - 7 * s, hy - 9 * s, 14 * s, 3 * s, hairCol);
                drawCircle(buf, hx, hy - 12 * s, 3 * s, hairCol);
            } else { // בלורית אלגנטית עם שביל בצד
                drawRect(buf, hx - 7 * s, hy - 9 * s, 14 * s, 3 * s, hairCol);
                drawCircle(buf, hx + 3 * s, hy - 6 * s, 2 * s, hairCol);
            }
        }
    } else {
        int hType = hatSt % 5;
        if (hType == 1) { // כתר מלכותי משובץ
            drawRect(buf, hx - 7 * s, hy - 10 * s, 14 * s, 4 * s, hatCol);
            drawCircle(buf, hx - 5 * s, hy - 11 * s, 2 * s, gold);
            drawCircle(buf, hx,         hy - 12 * s, 2 * s, red);
            drawCircle(buf, hx + 5 * s, hy - 11 * s, 2 * s, gold);
        } else if (hType == 2) { // מצנפת קוסם גבוהה
            drawRect(buf, hx - 10 * s, hy - 8 * s, 20 * s, 3 * s, hatCol);
            drawCircle(buf, hx, hy - 11 * s, 5 * s, hatCol);
            drawCircle(buf, hx, hy - 15 * s, 3 * s, gold);
        } else if (hType == 3) { // קסדת קרניים / ויקינג
            drawRect(buf, hx - 7 * s, hy - 10 * s, 14 * s, 4 * s, hatCol);
            drawCircle(buf, hx - 8 * s, hy - 11 * s, 3 * s, white);
            drawCircle(buf, hx + 8 * s, hy - 11 * s, 3 * s, white);
        } else if (hType == 4) { // הילת מלאך מרחפת
            drawRect(buf, hx - 6 * s, hy - 13 * s, 12 * s, 3 * s, hatCol);
        } else { // כובע מצחייה ספורטיבי
            drawRect(buf, hx - 7 * s, hy - 10 * s, 14 * s, 4 * s, hatCol);
            drawRect(buf, hx - 2 * s, hy - 7 * s, 11 * s, 2 * s, hatCol);
        }
    }
}

// בדיקה אם הקטגוריה הנוכחית היא איבר קטן/פנימי כדי להפעיל זום מצלמה אוטומטי!
static bool isSmallFacePartCategory(int cat) {
    return (cat == 0 || cat == 1 || cat == 2 || cat == 7 ||
            cat == 8 || cat == 9 || cat == 10 || cat == 11 || cat == 12);
}

// רינדור מסך מעבדת ההרכבה האישית (כולל מצלמת זום אוטומטית!) ומסך המקלדת העברית
void renderCustomizerScreen(u16* buf) {
    for (int y = 0; y < SCREEN_H; y++) {
        u16 rowCol = C16(18, 25 + y / 10, 48 + y / 7);
        u16* row = &buf[y * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++) row[x] = rowCol;
    }

    u16 white  = C16(255, 255, 255);
    u16 yellow = C16(241, 196, 15);
    u16 green  = C16(46, 204, 113);
    u16 orange = C16(255, 107, 107);
    u16 blue   = C16(52, 152, 219);
    u16 purple = C16(142, 68, 173);

    if (gameState == STATE_CUSTOM_KEYBOARD) {
        drawHebrewString(buf, 128, 8, "כתוב את שם הדמות:", yellow, 1);

        drawRect(buf, 48, 24, 160, 24, C16(10, 15, 30));
        drawRect(buf, 48, 24, 160, 2, green);
        syncGlyphsToUtf8Name();
        drawHebrewString(buf, 128, 30, customChar.nameHeb, white, 2);

        for (int i = 1; i <= 27; i++) {
            int idx = i - 1;
            int r = idx / 9;
            int c = idx % 9;
            int kx = 224 - c * 26;
            int ky = 54 + r * 28;
            drawRect(buf, kx, ky, 23, 24, C16(35, 55, 90));
            drawHebrewString(buf, kx + 11, ky + 8, HEB_LETTER_LABELS[i], white, 1);
        }

        drawRect(buf, 12, 144, 65, 34, orange);
        drawHebrewString(buf, 44, 157, "מחק אות", white, 1);

        drawRect(buf, 84, 144, 65, 34, blue);
        drawHebrewString(buf, 116, 157, "רווח", white, 1);

        drawRect(buf, 156, 144, 88, 34, green);
        drawHebrewString(buf, 200, 157, "שמור לחנות!", C16(15, 20, 15), 1);
        return;
    }

    drawHebrewString(buf, 128, 5, "הרכבה אישית מטורפת!", yellow, 1);

    // חלון המצלמה החכמה בצד ימין: מתקרבת אוטומטית (זום פי 3) כשעורכים חלקים קטנים!
    bool zoomFace = isSmallFacePartCategory(activeCategory);
    drawCircle(buf, 204, 70, 38, zoomFace ? C16(42, 65, 105) : C16(30, 48, 78));

    if (zoomFace) {
        // מצלמת תקריב אוטומטית (Scale 300% ממוקדת באזור הראש/פנים/גבות/פה/שיער!)
        drawCustomCharacterSprite(buf, 204, 98, 300, CHAR_ANIM_WALK, EXPR_NORMAL, frames);
        drawHebrewString(buf, 204, 22, "זום פנים!", C16(120, 245, 255), 1);
    } else if (activeCategory == 5 || activeCategory == 6) {
        // מיקוד מצלמה על פלג הגוף התחתון והנעליים!
        drawCustomCharacterSprite(buf, 204, 56, 200, CHAR_ANIM_WALK, EXPR_NORMAL, frames);
    } else {
        // תצוגת גוף מלאה
        drawCustomCharacterSprite(buf, 204, 74, 150, CHAR_ANIM_WALK, EXPR_NORMAL, frames);
    }
    drawHebrewString(buf, 204, 114, customChar.nameHeb, green, 1);

    // בחירת קטגוריה (מתוך 14 קטגוריות מלאות!)
    drawRect(buf, 8, 22, 24, 22, blue);
    drawHebrewString(buf, 20, 29, "-", white, 1);

    drawRect(buf, 35, 22, 96, 22, C16(30, 45, 75));
    drawHebrewString(buf, 83, 29, CAT_NAMES[activeCategory], yellow, 1);

    drawRect(buf, 134, 22, 24, 22, blue);
    drawHebrewString(buf, 146, 29, "!", white, 1);

    // בחירת סגנון מתוך 200 אפשרויות בכל קטגוריה!
    drawHebrewString(buf, 120, 52, "סגנון:", white, 1);
    drawRect(buf, 8, 48, 26, 20, purple);
    drawHebrewString(buf, 21, 54, "-", white, 1);

    drawNumber(buf, 54, 52, customChar.partStyle[activeCategory] + 1, yellow, 1);

    drawRect(buf, 132, 48, 26, 20, purple);
    drawHebrewString(buf, 145, 54, "!", white, 1);

    // בחירת צבע מתוך 64 צבעים לכל איבר!
    drawHebrewString(buf, 120, 76, "צבע:", white, 1);
    drawRect(buf, 8, 72, 26, 20, purple);
    drawHebrewString(buf, 21, 78, "-", white, 1);

    u16 curCol = getCustomPaletteColor(customChar.partColor[activeCategory]);
    drawRect(buf, 42, 74, 34, 16, curCol);
    drawNumber(buf, 82, 76, customChar.partColor[activeCategory] + 1, white, 1);

    drawRect(buf, 132, 72, 26, 20, purple);
    drawHebrewString(buf, 145, 78, "!", white, 1);

    // כפתור ישיר למעבר מהיר בין כובע לתסרוקת שיער + כפתור הגרלת עיצוב מטורף!
    drawRect(buf, 8, 96, 72, 20, C16(45, 85, 75));
    drawHebrewString(buf, 44, 102, (customChar.partStyle[7] == 0) ? "מצב: שיער" : "מצב: כובע", green, 1);

    drawRect(buf, 84, 96, 74, 20, C16(40, 65, 95));
    drawHebrewString(buf, 121, 102, "הגרל הכל!", yellow, 1);

    // שורת קיצור דרך לקמירות גבות ופה בזמן עריכת הפנים!
    drawRect(buf, 8, 120, 150, 18, C16(55, 40, 85));
    drawHebrewString(buf, 83, 125, "שנה קמירות גבות ופה!", C16(255, 200, 255), 1);

    // הצגת המשימה הזמנית הפעילה
    drawHebrewString(buf, 128, 141, getActiveMissionDesc(), C16(130, 240, 250), 1);

    // כפתורי שמירה/שם במקלדת וחזרה
    drawRect(buf, 8, 154, 70, 30, orange);
    drawHebrewString(buf, 43, 165, "חזור", white, 1);

    drawRect(buf, 84, 154, 78, 30, blue);
    drawHebrewString(buf, 123, 165, "שם במקלדת", white, 1);

    drawRect(buf, 168, 154, 80, 30, green);
    drawHebrewString(buf, 208, 165, "שמור ושחק!", C16(15, 20, 15), 1);
}

void handleCustomizerTouch(int tx, int ty) {
    if (gameState == STATE_CUSTOM_KEYBOARD) {
        if (ty >= 54 && ty < 138) {
            int r = (ty - 54) / 28;
            for (int c = 0; c < 9; c++) {
                int kx = 224 - c * 26;
                if (tx >= kx && tx <= kx + 23) {
                    int letterIdx = r * 9 + c + 1;
                    if (letterIdx >= 1 && letterIdx <= 27 && kbLen < 9) {
                        kbGlyphs[kbLen++] = (u8)letterIdx;
                        syncGlyphsToUtf8Name();
                    }
                    return;
                }
            }
        }
        if (ty >= 144 && ty <= 178) {
            if (tx >= 12 && tx <= 77) {
                if (kbLen > 0) kbLen--;
                syncGlyphsToUtf8Name();
            } else if (tx >= 84 && tx <= 149) {
                if (kbLen < 9) kbGlyphs[kbLen++] = 0;
                syncGlyphsToUtf8Name();
            } else if (tx >= 156 && tx <= 244) {
                syncGlyphsToUtf8Name();
                customChar.created = true;
                unlockedChars[CUSTOM_CHAR_IDX] = true;
                selectedCharIdx = CUSTOM_CHAR_IDX;
                savePermanentData();
                gameState = STATE_CUSTOMIZE;
            }
        }
        return;
    }

    // מעבר בין 14 הקטגוריות (המצלמה מתקרבת ומתרחקת אוטומטית!)
    if (ty >= 22 && ty <= 44) {
        if (tx >= 8 && tx <= 35) activeCategory = (activeCategory + CUSTOM_PARTS_COUNT - 1) % CUSTOM_PARTS_COUNT;
        else if (tx >= 130 && tx <= 160) activeCategory = (activeCategory + 1) % CUSTOM_PARTS_COUNT;
    }
    // מעבר בין 200 הסגנונות של הקטגוריה
    else if (ty >= 48 && ty <= 68) {
        if (tx >= 8 && tx <= 36) {
            customChar.partStyle[activeCategory] = (customChar.partStyle[activeCategory] + 199) % 200;
        } else if (tx >= 130 && tx <= 160) {
            customChar.partStyle[activeCategory] = (customChar.partStyle[activeCategory] + 1) % 200;
        }
    }
    // מעבר בין 64 הצבעים של האיבר
    else if (ty >= 72 && ty <= 92) {
        if (tx >= 8 && tx <= 36) {
            customChar.partColor[activeCategory] = (customChar.partColor[activeCategory] + 63) % 64;
        } else if (tx >= 130 && tx <= 160) {
            customChar.partColor[activeCategory] = (customChar.partColor[activeCategory] + 1) % 64;
        }
    }
    // החלפה מהירה בין מצב שיער (ללא כובע) למצב כובע + הגרלה אקראית
    else if (ty >= 96 && ty <= 116) {
        if (tx >= 8 && tx <= 80) {
            if (customChar.partStyle[7] == 0) {
                customChar.partStyle[7] = 1;
                activeCategory = 7;
            } else {
                customChar.partStyle[7] = 0; // מסיר כובע ומציג את השיער!
                if (customChar.partStyle[12] == 0) customChar.partStyle[12] = 1;
                activeCategory = 12; // מעביר אוטומטית לעריכת השיער בזום פנים!
            }
        } else if (tx >= 84 && tx <= 158) {
            for (int i = 0; i < CUSTOM_PARTS_COUNT; i++) {
                customChar.partStyle[i] = rand() % 199;
                customChar.partColor[i] = rand() % 64;
            }
            if (rand() % 2 == 0) customChar.partStyle[7] = 0; // 50% סיכוי להציג תסרוקת שיער ללא כובע!
        }
    }
    // לחצן שינוי מהיר לקמירות הגבות והפה!
    else if (ty >= 120 && ty <= 138 && tx >= 8 && tx <= 158) {
        activeCategory = 9;
        customChar.partStyle[9] = (customChar.partStyle[9] + 1) % 15;
    }
    // כפתורים תחתונים
    else if (ty >= 154 && ty <= 186) {
        if (tx >= 8 && tx <= 78) {
            gameState = STATE_START;
        } else if (tx >= 84 && tx <= 162) {
            gameState = STATE_CUSTOM_KEYBOARD;
        } else if (tx >= 168 && tx <= 248) {
            customChar.created = true;
            unlockedChars[CUSTOM_CHAR_IDX] = true;
            selectedCharIdx = CUSTOM_CHAR_IDX;
            savePermanentData();
            gameState = STATE_SHOP;
        }
    }
}
 
