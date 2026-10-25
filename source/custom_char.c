#include "game.h"
#include <string.h>
#include <stdlib.h>

#undef C16
#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

CustomCharData customChar;
u32 unlockedCustomRewards = 0; // ביטים של פריטים מיוחדים שנפתחו במשימות ושבירת שיאים
int currentMissionId = 0;
bool currentMissionDone = false;

static int activeCategory = 0; // 0=פנים, 1=זקן, 2=עיניים, 3=גוף, 4=ידיים, 5=רגליים, 6=נעליים, 7=כובעים
static int kbCursor = 0;
static u8 kbGlyphs[10] = {1, 10, 26, 10, 0}; // ברירת מחדל: "אישי"
static int kbLen = 4;

static const char* CAT_NAMES[8] = {
    "פנים וראש",
    "זקן ושפם",
    "עיניים",
    "גוף ושריון",
    "ידיים",
    "רגליים",
    "נעליים",
    "כובעים"
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
    {0xD7, 0x90}, // 1: א
    {0xD7, 0x91}, // 2: ב
    {0xD7, 0x92}, // 3: ג
    {0xD7, 0x93}, // 4: ד
    {0xD7, 0x94}, // 5: ה
    {0xD7, 0x95}, // 6: ו
    {0xD7, 0x96}, // 7: ז
    {0xD7, 0x97}, // 8: ח
    {0xD7, 0x98}, // 9: ט
    {0xD7, 0x99}, // 10: י
    {0xD7, 0x9A}, // 11: ך
    {0xD7, 0x9B}, // 12: כ
    {0xD7, 0x9C}, // 13: ל
    {0xD7, 0x9D}, // 14: ם
    {0xD7, 0x9E}, // 15: מ
    {0xD7, 0x9F}, // 16: ן
    {0xD7, 0xA0}, // 17: נ
    {0xD7, 0xA1}, // 18: ס
    {0xD7, 0xA2}, // 19: ע
    {0xD7, 0xA3}, // 20: ף
    {0xD7, 0xA4}, // 21: פ
    {0xD7, 0xA5}, // 22: ץ
    {0xD7, 0xA6}, // 23: צ
    {0xD7, 0xA7}, // 24: ק
    {0xD7, 0xA8}, // 25: ר
    {0xD7, 0xA9}, // 26: ש
    {0xD7, 0xAA}  // 27: ת
};

static const char* HEB_LETTER_LABELS[28] = {
    " ", "א","ב","ג","ד","ה","ו","ז","ח","ט",
    "י","ך","כ","ל","ם","מ","ן","נ","ס","ע",
    "ף","פ","ץ","צ","ק","ר","ש","ת"
};

// פלטת 64 צבעים עשירה לכל איבר בהרכבה האישית
u16 getCustomPaletteColor(int colIdx) {
    int idx = colIdx & 63;
    switch (idx % 16) {
        case 0:  return C16(250, 202, 150); // גוון עור בהיר
        case 1:  return C16(210, 140, 85);  // גוון עוגייה / עור שזוף
        case 2:  return C16(235, 45, 45);   // אדום בוהק
        case 3:  return C16(46, 204, 113);  // ירוק אמרלד
        case 4:  return C16(52, 152, 219);  // כחול שמיים
        case 5:  return C16(241, 196, 15);  // זהב מלכותי
        case 6:  return C16(155, 89, 182);  // סגול קסם
        case 7:  return C16(0, 235, 255);   // טורקיז ניאון
        case 8:  return C16(255, 120, 30);  // כתום לבה
        case 9:  return C16(255, 90, 180);  // ורוד ממתק
        case 10: return C16(245, 245, 250); // לבן שלג
        case 11: return C16(40, 45, 58);    // שחור פחם
        case 12: return C16(140, 90, 45);   // חום שוקולד
        case 13: return C16(185, 195, 210); // כסף מתכתי
        case 14: return C16(118, 255, 3);   // ירוק האקר
        default: return C16(90, 30, 120);   // סגול כהה
    }
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
        customChar.partStyle[0] = 0; customChar.partColor[0] = 0;  // פנים
        customChar.partStyle[1] = 0; customChar.partColor[1] = 12; // זקן
        customChar.partStyle[2] = 0; customChar.partColor[2] = 4;  // עיניים
        customChar.partStyle[3] = 1; customChar.partColor[3] = 2;  // גוף
        customChar.partStyle[4] = 1; customChar.partColor[4] = 0;  // ידיים
        customChar.partStyle[5] = 1; customChar.partColor[5] = 4;  // רגליים
        customChar.partStyle[6] = 1; customChar.partColor[6] = 12; // נעליים
        customChar.partStyle[7] = 1; customChar.partColor[7] = 5;  // כובע
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

// בדיקת השלמת משימה זמנית למשחק אחד או שבירת שיא ופתיחת פריטים נדירים!
void checkMissionAndRecordUnlocks(int dispScore, int biomeIdx, bool brokeRecord) {
    if (!currentMissionDone) {
        bool completed = false;
        if (currentMissionId == 0 && biomeIdx >= 3) completed = true; // הגעה לביומת הממתקים -> ראש איש עוגייה!
        if (currentMissionId == 1 && biomeIdx >= 1) completed = true; // הגעה למדבר -> כובע פרעה!
        if (currentMissionId == 2 && biomeIdx >= 4) completed = true; // הגעה להר געש -> שריון לבה!
        if (currentMissionId == 3 && biomeIdx >= 5) completed = true; // הגעה לחלל -> קסדת חלל!
        if (currentMissionId == 4 && dispScore >= 500) completed = true; // 500 נקודות -> זקן קוסם!
        if (currentMissionId == 5 && brokeRecord && dispScore >= 100) completed = true; // שבירת שיא -> נעלי סילון!

        if (completed) {
            currentMissionDone = true;
            unlockedCustomRewards |= (1 << currentMissionId);
            if (currentMissionId == 0) {
                // פותח ומצייד אוטומטית את ראש איש העוגייה המיוחד!
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

    // מתן פרסי התאמה אישית אוטומטיים בכל שבירת שיא!
    if (brokeRecord && dispScore >= 150) {
        unlockedCustomRewards |= (1 << 6);
        savePermanentData();
    }
}

// ציור הדמות בהרכבה אישית (משמש גם בחנות, גם במשחק וגם במעבדת היצירה!)
void drawCustomCharacterSprite(u16* buf, int cx, int cy, int scalePct, enum CharAnimState animState, enum FaceExpr faceExpr, int frameCnt) {
    int s = (scalePct >= 120) ? 2 : 1;
    u16 black = C16(18, 18, 24);
    u16 white = C16(252, 252, 255);
    u16 gold  = C16(255, 215, 0);
    u16 red   = C16(235, 40, 40);

    bool isMoving = (gameState == STATE_PLAYING || gameState == STATE_SHOP || gameState == STATE_CUSTOMIZE);
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

    int faceSt  = customChar.partStyle[0];
    int beardSt = customChar.partStyle[1];
    int eyeSt   = customChar.partStyle[2];
    int bodySt  = customChar.partStyle[3];
    int handSt  = customChar.partStyle[4];
    int legSt   = customChar.partStyle[5];
    int shoeSt  = customChar.partStyle[6];
    int hatSt   = customChar.partStyle[7];

    // 1. רגליים מותאמות אישית (200 וריאציות!)
    int legW = (legSt % 3 == 0) ? 6 : 5;
    drawRect(buf, x - (6 - legL / 2) * s, y + 5 * s, legW * s, 7 * s, black);
    drawRect(buf, x - (5 - legL / 2) * s, y + 5 * s, (legW - 2) * s, 6 * s, legCol);
    drawRect(buf, x + (1 + legR / 2) * s, y + 5 * s, legW * s, 7 * s, black);
    drawRect(buf, x + (2 + legR / 2) * s, y + 5 * s, (legW - 2) * s, 6 * s, legCol);
    if (legSt % 2 == 1) { // פסי שריון/קישוט על המכנסיים
        drawRect(buf, x - (5 - legL / 2) * s, y + 8 * s, (legW - 2) * s, 1 * s, white);
        drawRect(buf, x + (2 + legR / 2) * s, y + 8 * s, (legW - 2) * s, 1 * s, white);
    }

    // 2. נעליים מותאמות אישית (כולל נעלי סילון, מגפיים מכונפים וסניקרס!)
    int shoeW = 6 + (shoeSt % 3);
    drawRect(buf, x - (7 - legL / 2) * s, y + 11 * s, shoeW * s, 4 * s, black);
    drawRect(buf, x - (6 - legL / 2) * s, y + 12 * s, (shoeW - 2) * s, 2 * s, shoeCol);
    drawRect(buf, x + (1 + legR / 2) * s, y + 11 * s, shoeW * s, 4 * s, black);
    drawRect(buf, x + (2 + legR / 2) * s, y + 12 * s, (shoeW - 2) * s, 2 * s, shoeCol);
    if (shoeSt % 4 == 3) { // להבת סילון מתחת לנעליים!
        drawRect(buf, x - (6 - legL / 2) * s, y + 15 * s, 3 * s, 2 * s, gold);
        drawRect(buf, x + (2 + legR / 2) * s, y + 15 * s, 3 * s, 2 * s, red);
    }

    // 3. גוף ושריון מותאמים אישית (200 וריאציות דוגמה, חגורות, גלימות וסמלים!)
    if (bodySt % 5 == 4) { // גלימה מתנופפת מאחורי הגוף
        drawRect(buf, x - 10 * s, y - 4 * s, 20 * s, 14 * s, hatCol);
    }
    drawRect(buf, x - 7 * s, y - 4 * s, 14 * s, 10 * s, black);
    drawRect(buf, x - 6 * s, y - 3 * s, 12 * s, 8 * s, bodyCol);
    // דוגמה פנימית על החזה לפי מספר הסגנון (מאות שילובים!)
    if (bodySt % 3 == 1) {
        drawRect(buf, x - 6 * s, y + 2 * s, 12 * s, 2 * s, gold); // חגורת זהב
    } else if (bodySt % 3 == 2) {
        drawCircle(buf, x, y, 3 * s, white); // ליבת אנרגיה בחזה
    }
    if (bodySt >= 100) { // כתפיות שריון מלכותיות
        drawCircle(buf, x - 7 * s, y - 3 * s, 3 * s, gold);
        drawCircle(buf, x + 7 * s, y - 3 * s, 3 * s, gold);
    }

    // 4. ידיים, כפפות ואביזרי יד (חרב, מטה, כפפות אגרוף, צמידים)
    drawRect(buf, x - 11 * s, y - 3 * s, 4 * s, 7 * s, black);
    drawRect(buf, x - 10 * s, y - 2 * s, 2 * s, 5 * s, bodyCol);
    drawRect(buf, x + 7 * s,  y - 3 * s, 4 * s, 7 * s, black);
    drawRect(buf, x + 8 * s,  y - 2 * s, 2 * s, 5 * s, bodyCol);
    drawCircle(buf, x - 9 * s, y + 4 * s, 2 * s, handCol);
    drawCircle(buf, x + 9 * s, y + 4 * s, 2 * s, handCol);
    if (handSt % 4 == 1) { // חרב אור ביד ימין!
        drawLine(buf, x + 9 * s, y + 3 * s, x + 15 * s, y - 10 * s, C16(0, 255, 255));
    } else if (handSt % 4 == 2) { // מגן עגול ביד שמאל!
        drawCircle(buf, x - 11 * s, y + 2 * s, 4 * s, gold);
    } else if (handSt % 4 == 3) { // מטה קסמים!
        drawLine(buf, x + 11 * s, y + 10 * s, x + 11 * s, y - 12 * s, gold);
        drawCircle(buf, x + 11 * s, y - 13 * s, 3 * s, red);
    }

    // 5. פנים וראש (כולל ראש איש עוגייה מיוחד כשמשלימים את משימת הממתקים!)
    int hx = x;
    int hy = y - 10 * s;
    if (faceSt == 199) {
        // ראש איש עוגייה (Gingerbread Cookie Head) עם שוקולד צ'יפס וזיגוג סוכר לבן!
        drawCircle(buf, hx, hy, 9 * s, C16(90, 45, 15));
        drawCircle(buf, hx, hy, 8 * s, C16(195, 125, 60));
        // נטיפי זיגוג לבן-ורוד וקישוטי סוכריות על העוגייה
        drawCircle(buf, hx - 5 * s, hy - 5 * s, 2 * s, white);
        drawCircle(buf, hx,         hy - 6 * s, 2 * s, C16(255, 105, 180));
        drawCircle(buf, hx + 5 * s, hy - 5 * s, 2 * s, white);
        drawRect(buf, hx - 6 * s, hy + 2 * s, 2 * s, 2 * s, C16(70, 35, 15));
        drawRect(buf, hx + 5 * s, hy + 1 * s, 2 * s, 2 * s, C16(70, 35, 15));
    } else {
        int headR = 7 + (faceSt % 2);
        drawCircle(buf, hx, hy, (headR + 1) * s, black);
        drawCircle(buf, hx, hy, headR * s, faceCol);
        if (faceSt % 5 == 2) { // אוזני אלף/חיה מחודדות
            drawCircle(buf, hx - 8 * s, hy - 2 * s, 2 * s, faceCol);
            drawCircle(buf, hx + 8 * s, hy - 2 * s, 2 * s, faceCol);
        } else if (faceSt % 5 == 4) { // צלקת קרב או סימן שבטי על הלחי
            drawLine(buf, hx - 5 * s, hy - 3 * s, hx - 3 * s, hy + 2 * s, red);
        }
    }

    // 6. עיניים, משקפיים והבעת פנים
    if (faceExpr == EXPR_SCARED) {
        drawRect(buf, hx - 5 * s, hy - 2 * s, 4 * s, 4 * s, white);
        drawRect(buf, hx + 2 * s, hy - 2 * s, 4 * s, 4 * s, white);
        drawRect(buf, hx - 4 * s, hy - 1 * s, 2 * s, 2 * s, black);
        drawRect(buf, hx + 3 * s, hy - 1 * s, 2 * s, 2 * s, black);
        drawCircle(buf, hx, hy + 4 * s, 2 * s, red);
    } else if (faceExpr == EXPR_HURT) {
        drawLine(buf, hx - 5 * s, hy - 2 * s, hx - 2 * s, hy + 1 * s, black);
        drawLine(buf, hx - 2 * s, hy - 2 * s, hx - 5 * s, hy + 1 * s, black);
        drawLine(buf, hx + 2 * s, hy - 2 * s, hx + 5 * s, hy + 1 * s, black);
        drawLine(buf, hx + 5 * s, hy - 2 * s, hx + 2 * s, hy + 1 * s, black);
    } else {
        if (eyeSt % 4 == 1) { // משקפי שמש / סייבר מגניבים!
            drawRect(buf, hx - 6 * s, hy - 2 * s, 12 * s, 3 * s, eyeCol);
            drawRect(buf, hx - 6 * s, hy - 3 * s, 12 * s, 1 * s, black);
        } else if (eyeSt % 4 == 2) { // עין קיקלופ/לייזר מרכזית
            drawCircle(buf, hx, hy - 1 * s, 3 * s, white);
            drawCircle(buf, hx, hy - 1 * s, 1 * s, eyeCol);
        } else {
            drawRect(buf, hx - 5 * s, hy - 2 * s, 3 * s, 3 * s, white);
            drawRect(buf, hx + 2 * s, hy - 2 * s, 3 * s, 3 * s, white);
            drawRect(buf, hx - 4 * s, hy - 1 * s, 2 * s, 2 * s, eyeCol);
            drawRect(buf, hx + 3 * s, hy - 1 * s, 2 * s, 2 * s, eyeCol);
        }
        drawRect(buf, hx - 2 * s, hy + 3 * s, 4 * s, 1 * s, red);
    }

    // 7. זקן ושפם (0 = ללא זקן, 1..199 = זקנים, שפמים וזקני קוסמים!)
    if (beardSt > 0) {
        int bType = beardSt % 4;
        if (bType == 1) { // שפם אצילי מסולסל
            drawRect(buf, hx - 5 * s, hy + 2 * s, 10 * s, 2 * s, beardCol);
        } else if (bType == 2) { // זקן צרפתי מחודד
            drawRect(buf, hx - 3 * s, hy + 3 * s, 6 * s, 4 * s, beardCol);
        } else if (bType == 3) { // זקן קוסם ענק וארוך!
            drawCircle(buf, hx, hy + 5 * s, 5 * s, beardCol);
            drawCircle(buf, hx, hy + 9 * s, 3 * s, beardCol);
        } else { // זקן ויקינג מלא
            drawRect(buf, hx - 6 * s, hy + 2 * s, 12 * s, 5 * s, beardCol);
        }
    }

    // 8. כובעים, כתרים, קסדות והילות (0 = ללא כובע, 1..199 = מאות וריאציות!)
    if (hatSt > 0) {
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

// רינדור מסך מעבדת ההרכבה האישית ומסך המקלדת העברית
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

        // תיבת תצוגת השם המוקלד
        drawRect(buf, 48, 24, 160, 24, C16(10, 15, 30));
        drawRect(buf, 48, 24, 160, 2, green);
        syncGlyphsToUtf8Name();
        drawHebrewString(buf, 128, 30, customChar.nameHeb, white, 2);

        // ציור מקלדת עברית מלאה (27 אותיות: 3 שורות של 9 אותיות)
        for (int i = 1; i <= 27; i++) {
            int idx = i - 1;
            int r = idx / 9;
            int c = idx % 9;
            int kx = 224 - c * 26; // מימין לשמאל!
            int ky = 54 + r * 28;
            drawRect(buf, kx, ky, 23, 24, C16(35, 55, 90));
            drawHebrewString(buf, kx + 11, ky + 8, HEB_LETTER_LABELS[i], white, 1);
        }

        // כפתורי רווח, מחיקה ושמירה לנצח בחנות!
        drawRect(buf, 12, 144, 65, 34, orange);
        drawHebrewString(buf, 44, 157, "מחק אות", white, 1);

        drawRect(buf, 84, 144, 65, 34, blue);
        drawHebrewString(buf, 116, 157, "רווח", white, 1);

        drawRect(buf, 156, 144, 88, 34, green);
        drawHebrewString(buf, 200, 157, "שמור לחנות!", C16(15, 20, 15), 1);
        return;
    }

    // מסך מעבדת ההרכבה האישית הראשי
    drawHebrewString(buf, 128, 6, "הרכבה אישית מטורפת!", yellow, 1);

    // תצוגה חיה של הדמות המורכבת בצד ימין
    drawCircle(buf, 202, 72, 34, C16(30, 48, 78));
    drawCustomCharacterSprite(buf, 202, 76, 120, CHAR_ANIM_WALK, EXPR_HAPPY, frames);
    drawHebrewString(buf, 202, 112, customChar.nameHeb, green, 1);

    // בחירת קטגוריה (מתוך 8 קטגוריות: פנים, זקן, עיניים, גוף, ידיים, רגליים, נעליים, כובעים)
    drawRect(buf, 10, 24, 28, 24, blue);
    drawHebrewString(buf, 24, 32, "-", white, 1);

    drawRect(buf, 42, 24, 84, 24, C16(30, 45, 75));
    drawHebrewString(buf, 84, 32, CAT_NAMES[activeCategory], yellow, 1);

    drawRect(buf, 130, 24, 28, 24, blue);
    drawHebrewString(buf, 144, 32, "!", white, 1);

    // בחירת פריט מתוך 200 אפשרויות בכל קטגוריה!
    drawHebrewString(buf, 125, 56, "סגנון:", white, 1);
    drawRect(buf, 10, 52, 28, 22, purple);
    drawHebrewString(buf, 24, 59, "-", white, 1);

    drawNumber(buf, 58, 56, customChar.partStyle[activeCategory] + 1, yellow, 1);

    drawRect(buf, 130, 52, 28, 22, purple);
    drawHebrewString(buf, 144, 59, "!", white, 1);

    // בחירת צבע מתוך 64 צבעים לכל איבר!
    drawHebrewString(buf, 125, 84, "צבע:", white, 1);
    drawRect(buf, 10, 80, 28, 22, purple);
    drawHebrewString(buf, 24, 87, "-", white, 1);

    u16 curCol = getCustomPaletteColor(customChar.partColor[activeCategory]);
    drawRect(buf, 48, 82, 36, 18, curCol);
    drawNumber(buf, 90, 84, customChar.partColor[activeCategory] + 1, white, 1);

    drawRect(buf, 130, 80, 28, 22, purple);
    drawHebrewString(buf, 144, 87, "!", white, 1);

    // כפתור אקראי מטורף + הצגת המשימה הפעילה
    drawRect(buf, 10, 108, 148, 22, C16(40, 65, 95));
    drawHebrewString(buf, 84, 115, "הגרל עיצוב מטורף!", yellow, 1);

    drawHebrewString(buf, 128, 134, getActiveMissionDesc(), C16(130, 240, 250), 1);

    // כפתורי שמירה/שם במקלדת וחזרה
    drawRect(buf, 10, 150, 72, 34, orange);
    drawHebrewString(buf, 46, 163, "חזור", white, 1);

    drawRect(buf, 88, 150, 76, 34, blue);
    drawHebrewString(buf, 126, 163, "שם במקלדת", white, 1);

    drawRect(buf, 170, 150, 76, 34, green);
    drawHebrewString(buf, 208, 163, "בחר ושחק!", C16(15, 20, 15), 1);
}

void handleCustomizerTouch(int tx, int ty) {
    if (gameState == STATE_CUSTOM_KEYBOARD) {
        // לחיצה על אותיות המקלדת העברית
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
        // כפתורי פקודה במקלדת
        if (ty >= 144 && ty <= 178) {
            if (tx >= 12 && tx <= 77) { // מחק אות
                if (kbLen > 0) kbLen--;
                syncGlyphsToUtf8Name();
            } else if (tx >= 84 && tx <= 149) { // רווח
                if (kbLen < 9) kbGlyphs[kbLen++] = 0;
                syncGlyphsToUtf8Name();
            } else if (tx >= 156 && tx <= 244) { // שמור לחנות לנצח!
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

    // מעבר בין 8 הקטגוריות
    if (ty >= 24 && ty <= 48) {
        if (tx >= 10 && tx <= 38) activeCategory = (activeCategory + 7) % 8;
        else if (tx >= 130 && tx <= 158) activeCategory = (activeCategory + 1) % 8;
    }
    // מעבר בין 200 הסגנונות של הקטגוריה
    else if (ty >= 52 && ty <= 74) {
        if (tx >= 10 && tx <= 38) {
            customChar.partStyle[activeCategory] = (customChar.partStyle[activeCategory] + 199) % 200;
        } else if (tx >= 130 && tx <= 158) {
            customChar.partStyle[activeCategory] = (customChar.partStyle[activeCategory] + 1) % 200;
        }
    }
    // מעבר בין 64 הצבעים של האיבר
    else if (ty >= 80 && ty <= 102) {
        if (tx >= 10 && tx <= 38) {
            customChar.partColor[activeCategory] = (customChar.partColor[activeCategory] + 63) % 64;
        } else if (tx >= 130 && tx <= 158) {
            customChar.partColor[activeCategory] = (customChar.partColor[activeCategory] + 1) % 64;
        }
    }
    // הגרלת עיצוב מטורף אקראי
    else if (ty >= 108 && ty <= 130 && tx >= 10 && tx <= 158) {
        for (int i = 0; i < 8; i++) {
            customChar.partStyle[i] = rand() % 199;
            customChar.partColor[i] = rand() % 64;
        }
    }
    // כפתורים תחתונים
    else if (ty >= 150 && ty <= 184) {
        if (tx >= 10 && tx <= 82) {
            gameState = STATE_START;
        } else if (tx >= 88 && tx <= 164) {
            gameState = STATE_CUSTOM_KEYBOARD;
        } else if (tx >= 170 && tx <= 246) {
            customChar.created = true;
            unlockedChars[CUSTOM_CHAR_IDX] = true;
            selectedCharIdx = CUSTOM_CHAR_IDX;
            savePermanentData();
            gameState = STATE_SHOP;
        }
    }
}
 
