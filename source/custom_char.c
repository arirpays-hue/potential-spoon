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
    initCustomPartsDatabase();
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
    return getProceduralMissionText();
}

void pickNewTempMission(void) {
    currentMissionId++;
    generateProceduralMission();
}

void checkMissionAndRecordUnlocks(int dispScore, int biomeIdx, bool brokeRecord) {
    notifyProceduralMissionEvent(dispScore, biomeIdx, 0, 0, brokeRecord);
}

// ציור הדמות בהרכבה אישית ישירות מתוך מאגר 2,800 האלמנטים הייחודיים (custom_parts_db.c)!
void drawCustomCharacterSprite(u16* buf, int cx, int cy, int scalePct, enum CharAnimState animState, enum FaceExpr faceExpr, int frameCnt) {
    int s = (scalePct >= 250) ? 3 : ((scalePct >= 120) ? 2 : 1);

    bool isMoving = (gameState == STATE_PLAYING || gameState == STATE_SHOP || (gameState == STATE_CUSTOMIZE && s <= 2));
    int phase = isMoving ? ((frameCnt / 3) % 4) : 0;
    int legL = (phase == 0) ? -4 : ((phase == 2) ? 4 : 0);
    int legR = -legL;
    int bobY = (isMoving && (phase % 2 == 0)) ? -1 : 0;
    if (animState == CHAR_ANIM_JUMP) { bobY = -3; legL = -4; legR = 4; }
    else if (animState == CHAR_ANIM_HIT_GROUND || animState == CHAR_ANIM_HIT_AIR) { bobY = 2; faceExpr = EXPR_HURT; }

    int x  = cx;
    int y  = cy + bobY * s;
    int hx = x;
    int hy = y - 10 * s;

    // סדר ציור השכבות מתוך מסד 200 הפריטים הייחודיים לכל קטגוריה:
    // 1. גלימה/כנפיים מאחור -> 2. רגליים -> 3. נעליים -> 4. גוף -> 5. ידיים -> 6. פנים -> 7. עיניים -> 8. גבות -> 9. אף -> 10. פה -> 11. זקן -> 12. שיער/כובע
    const int drawOrder[12] = {13, 5, 6, 3, 4, 0, 2, 8, 10, 11, 1, 7};
    for (int step = 0; step < 12; step++) {
        int cat = drawOrder[step];
        if (cat == 7 && customChar.partStyle[7] == 0) {
            // אם אין כובע, מציירים את תסרוקת השיער הייחודית (קטגוריה 12)!
            u16 hairCol = getCustomPaletteColor(customChar.partColor[12]);
            renderStoredCustomPart(buf, 12, customChar.partStyle[12], hairCol, hx, hy, x, y, s, legL, legR, faceExpr);
        } else {
            u16 col = getCustomPaletteColor(customChar.partColor[cat]);
            renderStoredCustomPart(buf, cat, customChar.partStyle[cat], col, hx, hy, x, y, s, legL, legR, faceExpr);
        }
    }
}

static bool isSmallFacePartCategory(int cat) {
    return (cat == 0 || cat == 1 || cat == 2 || cat == 7 ||
            cat == 8 || cat == 9 || cat == 10 || cat == 11 || cat == 12);
}

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
        drawCustomCharacterSprite(buf, 204, 98, 300, CHAR_ANIM_WALK, EXPR_NORMAL, frames);
        drawHebrewString(buf, 204, 22, "זום פנים!", C16(120, 245, 255), 1);
    } else if (activeCategory == 5 || activeCategory == 6) {
        drawCustomCharacterSprite(buf, 204, 56, 200, CHAR_ANIM_WALK, EXPR_NORMAL, frames);
    } else {
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

    // בחירת סגנון מתוך 200 אפשרויות ייחודיות בכל קטגוריה!
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

    drawRect(buf, 8, 96, 72, 20, C16(45, 85, 75));
    drawHebrewString(buf, 44, 102, (customChar.partStyle[7] == 0) ? "מצב: שיער" : "מצב: כובע", green, 1);

    drawRect(buf, 84, 96, 74, 20, C16(40, 65, 95));
    drawHebrewString(buf, 121, 102, "הגרל הכל!", yellow, 1);

    drawRect(buf, 8, 120, 150, 18, C16(55, 40, 85));
    drawHebrewString(buf, 83, 125, "שנה קמירות גבות ופה!", C16(255, 200, 255), 1);

    drawHebrewString(buf, 128, 141, getActiveMissionDesc(), C16(130, 240, 250), 1);

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
                        playCoinSfx(false);
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
                playCharacterVoiceSfx(CUSTOM_CHAR_IDX);
                savePermanentData();
                gameState = STATE_CUSTOMIZE;
            }
        }
        return;
    }

    if (ty >= 22 && ty <= 44) {
        if (tx >= 8 && tx <= 35) activeCategory = (activeCategory + CUSTOM_PARTS_COUNT - 1) % CUSTOM_PARTS_COUNT;
        else if (tx >= 130 && tx <= 160) activeCategory = (activeCategory + 1) % CUSTOM_PARTS_COUNT;
    } else if (ty >= 48 && ty <= 68) {
        if (tx >= 8 && tx <= 36) {
            customChar.partStyle[activeCategory] = (customChar.partStyle[activeCategory] + 199) % 200;
            playCharacterVoiceSfx(CUSTOM_CHAR_IDX);
        } else if (tx >= 130 && tx <= 160) {
            customChar.partStyle[activeCategory] = (customChar.partStyle[activeCategory] + 1) % 200;
            playCharacterVoiceSfx(CUSTOM_CHAR_IDX);
        }
    } else if (ty >= 72 && ty <= 92) {
        if (tx >= 8 && tx <= 36) {
            customChar.partColor[activeCategory] = (customChar.partColor[activeCategory] + 63) % 64;
        } else if (tx >= 130 && tx <= 160) {
            customChar.partColor[activeCategory] = (customChar.partColor[activeCategory] + 1) % 64;
        }
    } else if (ty >= 96 && ty <= 116) {
        if (tx >= 8 && tx <= 80) {
            if (customChar.partStyle[7] == 0) {
                customChar.partStyle[7] = 1;
                activeCategory = 7;
            } else {
                customChar.partStyle[7] = 0;
                if (customChar.partStyle[12] == 0) customChar.partStyle[12] = 1;
                activeCategory = 12;
            }
        } else if (tx >= 84 && tx <= 158) {
            for (int i = 0; i < CUSTOM_PARTS_COUNT; i++) {
                customChar.partStyle[i] = rand() % 199;
                customChar.partColor[i] = rand() % 64;
            }
            if (rand() % 2 == 0) customChar.partStyle[7] = 0;
            playCharacterVoiceSfx(CUSTOM_CHAR_IDX);
        }
    } else if (ty >= 120 && ty <= 138 && tx >= 8 && tx <= 158) {
        activeCategory = 9;
        customChar.partStyle[9] = (customChar.partStyle[9] + 1) % 200;
    } else if (ty >= 154 && ty <= 186) {
        if (tx >= 8 && tx <= 78) {
            gameState = STATE_START;
        } else if (tx >= 84 && tx <= 162) {
            gameState = STATE_CUSTOM_KEYBOARD;
        } else if (tx >= 168 && tx <= 248) {
            customChar.created = true;
            unlockedChars[CUSTOM_CHAR_IDX] = true;
            selectedCharIdx = CUSTOM_CHAR_IDX;
            playCharacterVoiceSfx(CUSTOM_CHAR_IDX);
            savePermanentData();
            gameState = STATE_SHOP;
        }
    }
}
 
