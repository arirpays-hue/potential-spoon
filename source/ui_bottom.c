#include "game.h"
#include <string.h>

#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

// פונט ביטמאפ עברי קריא (8x8) לתצוגת כל השמות והעקיצות ב-RTL!
static const u8 hebFont[32][8] = {
    {0,0,0,0,0,0,0,0}, // 0: רווח
    {0x88,0x90,0x50,0x20,0x50,0x48,0x88,0}, // 1: א
    {0x78,0x08,0x08,0x08,0x08,0x08,0xFC,0}, // 2: ב
    {0x78,0x08,0x08,0x18,0x28,0x48,0x88,0}, // 3: ג
    {0xFC,0x08,0x08,0x08,0x08,0x08,0x08,0}, // 4: ד
    {0x78,0x08,0x08,0x48,0x48,0x48,0x48,0}, // 5: ה
    {0x30,0x10,0x10,0x10,0x10,0x10,0x10,0}, // 6: ו
    {0x78,0x10,0x20,0x20,0x20,0x20,0x20,0}, // 7: ז
    {0x78,0x48,0x48,0x48,0x48,0x48,0x48,0}, // 8: ח
    {0x48,0x58,0x48,0x48,0x48,0x48,0x30,0}, // 9: ט
    {0x30,0x10,0x10,0x00,0x00,0x00,0x00,0}, // 10: י
    {0x78,0x08,0x08,0x08,0x08,0x08,0x08,0x08}, // 11: ך
    {0x70,0x08,0x08,0x08,0x08,0x08,0x70,0}, // 12: כ
    {0x40,0x40,0x78,0x08,0x08,0x10,0x60,0}, // 13: ל
    {0x78,0x48,0x48,0x48,0x48,0x48,0x78,0}, // 14: ם
    {0x70,0x88,0xA8,0xA8,0x08,0x08,0x78,0}, // 15: מ
    {0x30,0x10,0x10,0x10,0x10,0x10,0x10,0x10}, // 16: ן
    {0x30,0x10,0x10,0x10,0x10,0x10,0x70,0}, // 17: נ
    {0x78,0x84,0x84,0x84,0x84,0x84,0x78,0}, // 18: ס
    {0x48,0x48,0x48,0x48,0x28,0x18,0x70,0}, // 19: ע
    {0x78,0x48,0x68,0x08,0x08,0x08,0x08,0x08}, // 20: ף
    {0x78,0x48,0x68,0x08,0x08,0x48,0x78,0}, // 21: פ
    {0x48,0x48,0x28,0x18,0x10,0x10,0x10,0x10}, // 22: ץ
    {0x48,0x48,0x28,0x10,0x10,0x10,0x78,0}, // 23: צ
    {0x78,0x08,0x48,0x50,0x40,0x40,0x40,0x40}, // 24: ק
    {0x78,0x08,0x08,0x08,0x08,0x08,0x08,0}, // 25: ר
    {0x94,0x94,0x94,0xB4,0x84,0x48,0x30,0}, // 26: ש
    {0x78,0x48,0x48,0x48,0x48,0x48,0xC8,0}, // 27: ת
    {0x18,0x18,0x18,0x18,0x00,0x18,0x18,0}, // 28: !
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0}, // 29: -
    {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0}, // 30: :
    {0x20,0x20,0x00,0x00,0x00,0x00,0x00,0}  // 31: '
};

static const u8 numFont[10][8] = {
    {0x3C,0x42,0x46,0x4A,0x52,0x62,0x3C,0}, // 0
    {0x10,0x30,0x10,0x10,0x10,0x10,0x38,0}, // 1
    {0x3C,0x42,0x02,0x0C,0x30,0x40,0x7E,0}, // 2
    {0x3C,0x42,0x02,0x1C,0x02,0x42,0x3C,0}, // 3
    {0x08,0x18,0x28,0x48,0x7E,0x08,0x08,0}, // 4
    {0x7E,0x40,0x7C,0x02,0x02,0x42,0x3C,0}, // 5
    {0x3C,0x40,0x7C,0x42,0x42,0x42,0x3C,0}, // 6
    {0x7E,0x02,0x04,0x08,0x10,0x20,0x20,0}, // 7
    {0x3C,0x42,0x42,0x3C,0x42,0x42,0x3C,0}, // 8
    {0x3C,0x42,0x42,0x3E,0x02,0x42,0x3C,0}  // 9
};

// אפקט טשטוש ופיקסליזציה אמיתי (Box-Blur + Dimming) לדמויות בחנות עד שמגיעים לרמה הנדרשת!
static void blurRectArea(u16* buf, int x, int y, int w, int h) {
    for (int py = y; py < y + h; py += 4) {
        for (int px = x; px < x + w; px += 4) {
            if (px < 0 || py < 0 || px + 3 >= SCREEN_W || py + 3 >= SCREEN_H) continue;
            u16 c = buf[(py + 1) * SCREEN_W + (px + 1)];
            int r = (c & 31) >> 1;
            int g = ((c >> 5) & 31) >> 1;
            int b = ((c >> 10) & 31) >> 1;
            u16 blurred = 0x8000 | (b << 10) | (g << 5) | r;
            for (int dy = 0; dy < 4; dy++) {
                for (int dx = 0; dx < 4; dx++) {
                    buf[(py + dy) * SCREEN_W + (px + dx)] = blurred;
                }
            }
        }
    }
}

static void drawGlyph8(u16* buf, int x, int y, const u8* rows, u16 color, int scale) {
    for (int r = 0; r < 8; r++) {
        u8 b = rows[r];
        for (int c = 0; c < 8; c++) {
            if (b & (1 << (7 - c))) {
                drawRect(buf, x + c * scale, y + r * scale, scale, scale, color);
            }
        }
    }
}

void drawNumber(u16* buf, int x, int y, int num, u16 color, int scale) {
    if (num == 0) {
        drawGlyph8(buf, x, y, numFont[0], color, scale);
        return;
    }
    int digits[10];
    int cnt = 0;
    while (num > 0 && cnt < 10) {
        digits[cnt++] = num % 10;
        num /= 10;
    }
    for (int i = cnt - 1; i >= 0; i--) {
        drawGlyph8(buf, x, y, numFont[digits[i]], color, scale);
        x += 7 * scale;
    }
}

// פענוח UTF-8 של מחרוזת עברית וציור מימין לשמאל!
void drawHebrewString(u16* buf, int centerX, int y, const char* utf8Str, u16 color, int scale) {
    u8 glyphs[80];
    int count = 0;
    const unsigned char* p = (const unsigned char*)utf8Str;
    while (*p && count < 78) {
        if (*p == 0xD7 && *(p + 1) >= 0x90 && *(p + 1) <= 0xAA) {
            glyphs[count++] = (*(p + 1) - 0x90) + 1;
            p += 2;
        } else if (*p == 0xD7 && *(p + 1) == 0xB3) {
            glyphs[count++] = 31; // גרש
            p += 2;
        } else if (*p == ' ') {
            glyphs[count++] = 0;
            p++;
        } else if (*p == '!') {
            glyphs[count++] = 28;
            p++;
        } else if (*p == '-') {
            glyphs[count++] = 29;
            p++;
        } else if (*p == ':') {
            glyphs[count++] = 30;
            p++;
        } else {
            p++;
        }
    }
    int totalW = count * 7 * scale;
    int curX = centerX + totalW / 2;
    for (int i = 0; i < count; i++) {
        curX -= 7 * scale;
        if (glyphs[i] > 0 && glyphs[i] < 32) {
            drawGlyph8(buf, curX, y, hebFont[glyphs[i]], color, scale);
        }
    }
}

// ציור משפט ארוך עם שבירת שורות אוטומטית (לעקיצות ה-Roasts!)
static void drawHebrewMultiline(u16* buf, int centerX, int y, const char* str, u16 color) {
    char line1[90] = {0}, line2[90] = {0};
    int len = strlen(str);
    if (len <= 42) {
        drawHebrewString(buf, centerX, y, str, color, 1);
        return;
    }
    int split = len / 2;
    while (split < len && str[split] != ' ') split++;
    if (split >= len) {
        drawHebrewString(buf, centerX, y, str, color, 1);
    } else {
        memcpy(line1, str, split);
        line1[split] = '\0';
        strcpy(line2, str + split + 1);
        drawHebrewString(buf, centerX, y, line1, color, 1);
        drawHebrewString(buf, centerX, y + 14, line2, color, 1);
    }
}

void renderBottomScreen(u16* buf) {
    if (gameState == STATE_CUSTOMIZE || gameState == STATE_CUSTOM_KEYBOARD) {
        renderCustomizerScreen(buf);
        return;
    }

    // רקע כהה מודרני למסך המגע
    for (int y = 0; y < SCREEN_H; y++) {
        u16 rowCol = C16(15, 22 + y / 12, 42 + y / 8);
        u16* row = &buf[y * SCREEN_W];
        for (int x = 0; x < SCREEN_W; x++) row[x] = rowCol;
    }

    u16 white  = C16(255, 255, 255);
    u16 yellow = C16(241, 196, 15);
    u16 green  = C16(46, 204, 113);
    u16 orange = C16(255, 107, 107);
    u16 blue   = C16(52, 152, 219);
    u16 purple = C16(155, 89, 182);

    if (gameState == STATE_START) {
        drawHebrewString(buf, 128, 12, "הרפתקאות הקופץ", yellow, 2);
        drawHebrewString(buf, 128, 34, getActiveMissionDesc(), C16(120, 240, 250), 1);

        // כפתור שחק עכשיו
        drawRect(buf, 48, 52, 160, 30, orange);
        drawHebrewString(buf, 128, 60, "שחק עכשיו", white, 2);

        // כפתור חנות הדמויות
        drawRect(buf, 48, 88, 160, 30, yellow);
        drawHebrewString(buf, 128, 96, "חנות הדמויות", C16(25, 25, 25), 2);

        // כפתור הרכבה אישית מטורפת
        drawRect(buf, 48, 124, 160, 28, purple);
        drawHebrewString(buf, 128, 134, "הרכבה אישית!", white, 1);

        // כפתור פרטים ושיא
        drawRect(buf, 68, 158, 120, 26, blue);
        drawHebrewString(buf, 128, 167, "פרטים ושיא", white, 1);

    } else if (gameState == STATE_PLAYING || gameState == STATE_PAUSED || gameState == STATE_COUNTDOWN) {
        drawHebrewString(buf, 128, 10, getActiveMissionDesc(), yellow, 1);

        // הילה דינמית שמשנה צבע כשמזוהה מסלול התנגשות עם אויב או באיסוף מטבע!
        u16 ringCol = C16(30, 45, 70);
        if (currentFaceExpr == EXPR_SCARED) ringCol = C16(120, 30, 40);
        else if (currentFaceExpr == EXPR_HAPPY) ringCol = C16(30, 95, 60);

        drawCircle(buf, 128, 70, 30, ringCol);
        drawCharacterSpriteEx(buf, selectedCharIdx, 128, 72, 120, playerAnimState, currentFaceExpr, 0, frames);
        const char* activeName = (selectedCharIdx == CUSTOM_CHAR_IDX) ? customChar.nameHeb : CHARACTERS[selectedCharIdx].nameHeb;
        drawHebrewString(buf, 128, 106, activeName, green, 1);

        // הצגת הודעת האזהרה הספציפית לאויב (רק כשיש סכנת התנגשות אמיתית!)
        if (currentFaceExpr == EXPR_SCARED && currentWarningText != NULL) {
            drawHebrewString(buf, 128, 120, currentWarningText, orange, 1);
        } else if (currentFaceExpr == EXPR_HAPPY) {
            drawHebrewString(buf, 128, 120, "איזה יופי!", yellow, 1);
        }

        // כפתור השהיה / המשך
        drawRect(buf, 68, 136, 120, 36, (gameState == STATE_PAUSED) ? green : blue);
        drawHebrewString(buf, 128, 146, (gameState == STATE_PAUSED) ? "המשך משחק" : "השהה משחק", white, 2);

    } else if (gameState == STATE_GAMEOVER) {
        drawHebrewString(buf, 128, 12, "נפסלת!", orange, 2);

        // תיבת העקיצה ממנוע העקיצות הענק!
        drawRect(buf, 10, 38, 236, 40, C16(28, 38, 60));
        if (currentRoastText) {
            drawHebrewMultiline(buf, 128, 46, currentRoastText, C16(129, 236, 236));
        }

        // הצגת ניקוד ושיא
        drawHebrewString(buf, 165, 86, "ניקוד:", white, 1);
        drawNumber(buf, 95, 86, score / 10, yellow, 1);

        // כפתור נסה שוב
        drawRect(buf, 48, 106, 160, 32, orange);
        drawHebrewString(buf, 128, 114, "נסה שוב", white, 2);

        // כפתור חנות
        drawRect(buf, 48, 146, 160, 32, yellow);
        drawHebrewString(buf, 128, 154, "חנות הדמויות", C16(25, 25, 25), 2);

    } else if (gameState == STATE_SHOP) {
        // כותרת חנות וכמות מטבעות
        drawHebrewString(buf, 175, 6, "חנות הדמויות", yellow, 1);
        drawCircle(buf, 22, 10, 5, yellow);
        drawNumber(buf, 32, 6, coinsCount, yellow, 1);

        // רשת של 6 דמויות בעמוד (כולל הדמות בהרכבה אישית וטשטוש לדמויות נעולות לפי רמה!)
        int startIdx = shopPage * 6;
        for (int i = 0; i < 6; i++) {
            int cIdx = startIdx + i;
            if (cIdx >= TOTAL_SHOP_CHARS) break;

            int col = i % 3;
            int row = i / 3;
            int bx = 12 + col * 78;
            int by = 22 + row * 64;

            bool owned = unlockedChars[cIdx];
            bool sel   = (selectedCharIdx == cIdx);
            int reqBiome = getRequiredBiomeForChar(cIdx);
            bool isBlurred = (bestBiomeIdx < reqBiome);

            u16 cardCol = sel ? C16(30, 90, 55) : (owned ? C16(35, 50, 80) : C16(25, 30, 45));
            drawRect(buf, bx, by, 72, 58, cardCol);
            if (sel) drawRect(buf, bx, by, 72, 3, green);

            enum FaceExpr shopExpr = sel ? EXPR_HAPPY : EXPR_NORMAL;
            drawCharacterSpriteEx(buf, cIdx, bx + 36, by + 22, 100, CHAR_ANIM_WALK, shopExpr, 0, frames);

            if (isBlurred) {
                // הפעלת טשטוש על הדמות עד שהשחקן מגיע לרמה/ביומה הנדרשת!
                blurRectArea(buf, bx + 8, by + 2, 56, 34);
                drawHebrewString(buf, bx + 36, by + 37, "מטושטש!", orange, 1);
                drawHebrewString(buf, bx + 46, by + 48, "רמה:", yellow, 1);
                drawNumber(buf, bx + 12, by + 48, reqBiome + 1, yellow, 1);
            } else if (cIdx == CUSTOM_CHAR_IDX) {
                drawHebrewString(buf, bx + 36, by + 37, customChar.nameHeb, yellow, 1);
                drawHebrewString(buf, bx + 36, by + 48, sel ? "נבחר" : (customChar.created ? "בחר" : "הרכב!"), green, 1);
            } else {
                drawHebrewString(buf, bx + 36, by + 37, CHARACTERS[cIdx].nameHeb, white, 1);
                if (sel) {
                    drawHebrewString(buf, bx + 36, by + 48, "נבחר", green, 1);
                } else if (owned) {
                    drawHebrewString(buf, bx + 36, by + 48, "בחר", blue, 1);
                } else {
                    drawNumber(buf, bx + 22, by + 48, CHARACTERS[cIdx].price, yellow, 1);
                }
            }
        }

        // כפתורי דפדוף וחזרה בתחתית החנות
        drawRect(buf, 12, 154, 55, 30, blue);
        drawHebrewString(buf, 39, 165, "הקודם", white, 1);

        drawRect(buf, 75, 154, 106, 30, orange);
        drawHebrewString(buf, 128, 165, "חזור למשחק", white, 1);

        drawRect(buf, 189, 154, 55, 30, blue);
        drawHebrewString(buf, 216, 165, "הבא", white, 1);

    } else if (gameState == STATE_DETAILS) {
        // מסך פרטים וטבלת כל השיאים שנשמרים לתמיד!
        drawHebrewString(buf, 128, 10, "הרפתקאות הקופץ", yellow, 2);
        drawHebrewString(buf, 128, 30, "שיאים שנשמרו לתמיד:", green, 1);

        // מסגרת טבלת 3 השיאים הגדולים
        drawRect(buf, 18, 44, 220, 62, C16(25, 35, 58));

        drawHebrewString(buf, 175, 50, "מקום ראשון:", yellow, 1);
        drawNumber(buf, 65, 50, topScores[0], yellow, 1);

        drawHebrewString(buf, 175, 68, "מקום שני:", C16(200, 210, 225), 1);
        drawNumber(buf, 65, 68, topScores[1], C16(200, 210, 225), 1);

        drawHebrewString(buf, 175, 86, "מקום שלישי:", C16(215, 145, 75), 1);
        drawNumber(buf, 65, 86, topScores[2], C16(215, 145, 75), 1);

        // הצגת שיא הביומה הרחוקה ביותר ומספר המשחקים ששוחקו
        drawHebrewString(buf, 175, 114, "שיא עולם:", white, 1);
        drawHebrewString(buf, 75,  114, BIOMES[bestBiomeIdx].nameHeb, green, 1);

        drawHebrewString(buf, 175, 132, "משחקים:", white, 1);
        drawNumber(buf, 65, 132, totalGamesPlayed, blue, 1);

        // כפתור סגירה
        drawRect(buf, 68, 154, 120, 28, orange);
        drawHebrewString(buf, 128, 160, "סגור", white, 2);
    }
}
 
