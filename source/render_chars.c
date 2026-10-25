#include "game.h"
#include <stdlib.h>

#undef C16
#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

// פונקציית עזר לציור מלבן עם מסגרת כהה (לפריטים מכניים או מיינקראפט)
static void drawBoxOut(u16* buf, int x, int y, int w, int h, u16 fill, u16 out) {
    drawRect(buf, x, y, w, h, out);
    if (w > 2 && h > 2) {
        drawRect(buf, x + 1, y + 1, w - 2, h - 2, fill);
    }
}

// פונקציית עזר לקפסולה מעוגלת ומוצללת - מבטלת לחלוטין את המראה המרובע של הדמויות!
static void drawRoundedPart(u16* buf, int x, int y, int w, int h, u16 fill, u16 light, u16 out) {
    if (w < 4 || h < 4) {
        drawBoxOut(buf, x, y, w, h, fill, out);
        return;
    }
    // מילוי פנימי מעוגל בפינות
    drawRect(buf, x + 1, y + 2, w - 2, h - 4, fill);
    drawRect(buf, x + 2, y + 1, w - 4, h - 2, fill);
    // קו מתאר מעוגל ללא שפיצים מרובעים
    drawRect(buf, x + 2, y, w - 4, 1, out);
    drawRect(buf, x + 2, y + h - 1, w - 4, 1, out);
    drawRect(buf, x, y + 2, 1, h - 4, out);
    drawRect(buf, x + w - 1, y + 2, 1, h - 4, out);
    // פיקסלי פינה פנימיים לסגירת המתאר המעוגל
    drawRect(buf, x + 1, y + 1, 1, 1, out);
    drawRect(buf, x + w - 2, y + 1, 1, 1, out);
    drawRect(buf, x + 1, y + h - 2, 1, 1, out);
    drawRect(buf, x + w - 2, y + h - 2, 1, 1, out);
    // פס הארה ליצירת נפח תלת-ממדי עגול
    if (w > 6 && h > 6) {
        drawRect(buf, x + 2, y + 2, 2, h - 5, light);
    }
}

// ציור 4 הבעות פנים דינמיות ומשתנות לכל 99 הדמויות!
static void drawDynamicFace(u16* buf, int cx, int y, int s, u16 pupilCol, enum FaceExpr expr, int frameCnt) {
    u16 white = C16(255, 255, 255);
    u16 black = C16(18, 18, 24);
    u16 red   = C16(235, 45, 45);
    u16 cyan  = C16(80, 220, 255);

    if (expr == EXPR_HURT) {
        // הבעת כאב/הלם בפגיעה: עיני X ופה פעור רועד!
        drawLine(buf, cx - 6 * s, y - 1 * s, cx - 2 * s, y + 3 * s, black);
        drawLine(buf, cx - 2 * s, y - 1 * s, cx - 6 * s, y + 3 * s, black);
        drawLine(buf, cx + 2 * s, y - 1 * s, cx + 6 * s, y + 3 * s, black);
        drawLine(buf, cx + 6 * s, y - 1 * s, cx + 2 * s, y + 3 * s, black);
        drawRoundedPart(buf, cx - 3 * s, y + 4 * s, 6 * s, 4 * s, red, white, black);
        return;
    }

    if (expr == EXPR_SCARED) {
        // הבעה מפוחדת כשמתקרבת מפלצת: עיניים קרועות לרווחה, אישונים מכווצים רועדים, פה פעור וטיפת זיעה!
        int jitter = (frameCnt / 2) % 2;
        drawRect(buf, cx - 6 * s, y - 1 * s, 4 * s, 5 * s, white);
        drawRect(buf, cx + 2 * s, y - 1 * s, 4 * s, 5 * s, white);
        drawRect(buf, cx - (4 + jitter) * s, y + 1 * s, 2 * s, 2 * s, black);
        drawRect(buf, cx + (3 + jitter) * s, y + 1 * s, 2 * s, 2 * s, black);
        // גבות מורמות בפחד
        drawLine(buf, cx - 6 * s, y - 2 * s, cx - 3 * s, y - 3 * s, black);
        drawLine(buf, cx + 3 * s, y - 3 * s, cx + 6 * s, y - 2 * s, black);
        // פה פעור באימה
        drawCircle(buf, cx, y + 5 * s, 2 * s, black);
        drawCircle(buf, cx, y + 5 * s, 1 * s, red);
        // טיפת זיעה תכלת בצד המצח
        int dropY = (frameCnt / 4) % 3;
        drawRect(buf, cx + 7 * s, y - (2 - dropY) * s, 2 * s, 3 * s, cyan);
        return;
    }

    if (expr == EXPR_HAPPY) {
        // הבעה שמחה ומנצחת באיסוף מטבע/יהלום: עיניים שמחות קמורות וחיוך רחב!
        drawLine(buf, cx - 6 * s, y + 1 * s, cx - 4 * s, y - 1 * s, black);
        drawLine(buf, cx - 4 * s, y - 1 * s, cx - 2 * s, y + 1 * s, black);
        drawLine(buf, cx + 2 * s, y + 1 * s, cx + 4 * s, y - 1 * s, black);
        drawLine(buf, cx + 4 * s, y - 1 * s, cx + 6 * s, y + 1 * s, black);
        // לחיים סמוקות וחיוך פתוח
        drawRect(buf, cx - 7 * s, y + 2 * s, 2 * s, 2 * s, C16(255, 130, 150));
        drawRect(buf, cx + 5 * s, y + 2 * s, 2 * s, 2 * s, C16(255, 130, 150));
        drawRect(buf, cx - 3 * s, y + 4 * s, 6 * s, 2 * s, red);
        drawRect(buf, cx - 4 * s, y + 3 * s, 1 * s, 2 * s, black);
        drawRect(buf, cx + 3 * s, y + 3 * s, 1 * s, 2 * s, black);
        return;
    }

    // EXPR_NORMAL: עיניים חיות עם לובן, אישון וניצוץ לבן + חיוך קטן
    drawRect(buf, cx - 5 * s, y, 3 * s, 4 * s, white);
    drawRect(buf, cx - 4 * s, y + 1 * s, 2 * s, 3 * s, pupilCol);
    drawRect(buf, cx - 5 * s, y, 1 * s, 1 * s, white);

    drawRect(buf, cx + 2 * s, y, 3 * s, 4 * s, white);
    drawRect(buf, cx + 3 * s, y + 1 * s, 2 * s, 3 * s, pupilCol);
    drawRect(buf, cx + 2 * s, y, 1 * s, 1 * s, white);

    drawRect(buf, cx - 2 * s, y + 5 * s, 4 * s, 1 * s, C16(165, 70, 60));
}

// הפונקציה המלאה שמציירת כל דמות בצורה עגולה ומפורטת + 4 אנימציות ייחודיות + הבעות פנים!
void drawCharacterSpriteEx(u16* buf, int charIdx, int cx, int cy, int scalePct, enum CharAnimState animState, enum FaceExpr faceExpr, int angle, int frameCnt) {
    if (charIdx < 0 || charIdx >= NUM_CHARS) charIdx = 0;
    const CharData* ch = &CHARACTERS[charIdx];
    int s = (scalePct >= 120) ? 2 : 1;

    u16 black   = C16(18, 18, 24);
    u16 white   = C16(252, 252, 255);
    u16 skin    = C16(250, 202, 150);
    u16 skinLit = C16(255, 228, 190);
    u16 gold    = C16(255, 215, 0);
    u16 red     = C16(235, 35, 35);
    u16 silver  = C16(205, 215, 230);

    // =========================================================================
    // חישוב 4 האנימציות הייחודיות לכל דמות לפי הסגנון והמאפיינים שלה!
    // =========================================================================
    int legOffL = 0, legOffR = 0;
    int armOffY = 0, armSpread = 0;
    int bodyBobY = 0, bodyShiftX = 0;
    int headTiltX = 0, headBobY = 0;

    if (animState == CHAR_ANIM_WALK) {
        // 1. אנימציית הליכה/ריצה ייחודית לכל דמות!
        int phase = (frameCnt / 3) % 4;
        int stride = 4;
        if (ch->style == STYLE_SPIN) {
            // ריצת נינג'ה/סמוראי מהירה עם רכינה קדימה!
            stride = 6;
            headTiltX = 2;
            armOffY = -2;
        } else if (ch->style == STYLE_FLOAT) {
            // ריחוף גלי עדין מעל הקרקע!
            stride = 2;
            bodyBobY = ((frameCnt / 5) % 2 == 0) ? -2 : 1;
            armSpread = 2;
        } else if (ch->style == STYLE_BOUNCE) {
            // ניתורים קופצניים ושמחים!
            stride = 5;
            bodyBobY = (phase % 2 == 0) ? -3 : 1;
        } else if (ch->style == STYLE_WOBBLE) {
            // הליכה מתנדנדת מצחיקה מצד לצד (ליצן/בננה/פיראט)!
            stride = 6;
            headTiltX = (phase < 2) ? -2 : 2;
            armOffY = (phase % 2 == 0) ? -3 : 2;
        } else if (ch->style == STYLE_HEAVY) {
            // צעידה כבדה ועוצמתית (אביר/קינג קונג/גודזילה)!
            stride = 4;
            bodyBobY = (phase == 1 || phase == 3) ? 2 : 0;
        }

        if (phase == 0)      { legOffL = -stride; legOffR =  stride; }
        else if (phase == 2) { legOffL =  stride; legOffR = -stride; }
        if (bodyBobY == 0) bodyBobY = (phase % 2 == 0) ? -1 : 1;

    } else if (animState == CHAR_ANIM_JUMP) {
        // 2. אנימציית קפיצה ייחודית לכל דמות!
        if (ch->style == STYLE_SPIN) {
            // סלטת אוויר מלאה (4 שלבי סיבוב) עם שובל תנועה!
            int spinDir = (frameCnt / 2) % 4;
            bodyBobY = (spinDir % 2 == 0) ? -3 : 2;
            bodyShiftX = (spinDir == 1) ? 2 : ((spinDir == 3) ? -2 : 0);
            legOffL = -5; legOffR = 5;
            armOffY = -6;
            drawCircle(buf, cx - 4 * s, cy - 2 * s, 11 * s, ch->subColor);
        } else if (ch->style == STYLE_FLOAT) {
            // ריחוף קסום באוויר עם טבעת אנרגיה מתחת לרגליים!
            bodyBobY = -3;
            armOffY = -4;
            armSpread = 3;
            legOffL = -2; legOffR = 2;
            drawRect(buf, cx - 8 * s, cy + 14 * s, 16 * s, 2 * s, C16(120, 240, 255));
        } else if (ch->style == STYLE_BOUNCE) {
            // מתיחה אנכית קפיצית באוויר!
            bodyBobY = -4;
            headBobY = -2;
            legOffL = 3; legOffR = -3;
            armOffY = -5;
        } else if (ch->style == STYLE_FLAP) {
            // תעופת כנפיים עוצמתית באוויר!
            bodyBobY = -3;
            armOffY = ((frameCnt / 2) % 2 == 0) ? -6 : 2;
            legOffL = -3; legOffR = -3;
        } else if (ch->style == STYLE_HEAVY) {
            // קפיצת ריסוק כבדה עם ידיים מונפות!
            armOffY = -7;
            armSpread = 2;
            legOffL = -4; legOffR = 4;
        } else {
            // קפיצת פלטפורמה קלאסית דינמית!
            bodyBobY = -2;
            armOffY = -5;
            legOffL = -4; legOffR = 3;
        }

    } else if (animState == CHAR_ANIM_HIT_GROUND) {
        // 3. אנימציית פגיעה באויב קרקע / היתקלות באבן: הדפה אחורה, רטט וכוכבי סחרחורת!
        bodyShiftX = -3 + ((frameCnt % 2) * 3);
        bodyBobY = -1;
        headTiltX = -3;
        legOffL = 6;  // רגליים נזרקות קדימה מעוצמת המכה באבן
        legOffR = -5;
        armOffY = -6;
        armSpread = 4;
        faceExpr = EXPR_HURT;

    } else if (animState == CHAR_ANIM_HIT_AIR) {
        // 4. אנימציית פגיעה מאויב מעופף: התכווצות והדפה כלפי מטה + ניצוצות פגיעה מלמעלה!
        bodyShiftX = ((frameCnt % 2) == 0) ? -2 : 2;
        bodyBobY = 4;     // הדמות נמעכת כלפי מטה מהמכה האווירית
        headBobY = 3;     // הראש מתכווץ לכתפיים
        armOffY = -7;     // ידיים מגינות על הראש
        legOffL = -5; legOffR = 5;
        faceExpr = EXPR_HURT;
    }

    int x = cx + bodyShiftX * s;
    int y = cy + bodyBobY * s;

    // 1. גלימות או כנפיים מעוגלות מאחורי הגוף
    if (charIdx == 3 || charIdx == 44 || charIdx == 21 || charIdx == 90 || charIdx == 45) {
        u16 capeCol = (charIdx == 45) ? C16(30, 35, 50) : red; // גלימה כהה לאיש העטלף, אדומה למלך/סופרמן/דרקולה
        int capeWave = ((frameCnt / 4) % 2) * 2;
        drawRoundedPart(buf, x - (11 + capeWave) * s, y - 5 * s, 22 * s, 16 * s, capeCol, white, black);
        if (charIdx == 3) {
            drawRoundedPart(buf, x - 10 * s, y - 6 * s, 20 * s, 4 * s, white, white, black);
        }
    } else if (ch->shape == SHAPE_WINGED) {
        int flap = (animState == CHAR_ANIM_JUMP || animState == CHAR_ANIM_WALK) ? (((frameCnt / 3) % 2) * 6 - 3) : 0;
        drawRoundedPart(buf, x - 16 * s, y - (6 + flap) * s, 9 * s, 11 * s, ch->subColor, white, black);
        drawRoundedPart(buf, x + 7 * s,  y - (6 + flap) * s, 9 * s, 11 * s, ch->subColor, white, black);
    }

    // 2. רגליים מעוגלות ומפורטות עם נעליים עגולות
    drawRoundedPart(buf, x - (6 - legOffL / 2) * s, y + 5 * s, 5 * s, 7 * s, C16(45, 60, 95), C16(80, 105, 150), black);
    drawRoundedPart(buf, x + (1 + legOffR / 2) * s, y + 5 * s, 5 * s, 7 * s, C16(45, 60, 95), C16(80, 105, 150), black);
    drawRoundedPart(buf, x - (7 - legOffL / 2) * s, y + 10 * s, 6 * s, 5 * s, C16(85, 50, 28), C16(130, 80, 45), black);
    drawRoundedPart(buf, x + (1 + legOffR / 2) * s, y + 10 * s, 6 * s, 5 * s, C16(85, 50, 28), C16(130, 80, 45), black);

    // 3. דמויות האוכל המעוגלות והמפורטות:
    if (ch->shape == SHAPE_BURGER) {
        // המבורגר עגול ועסיסי (לא מלבני!)
        drawRoundedPart(buf, x - 11 * s, y + 3 * s, 22 * s, 5 * s, C16(225, 150, 60), C16(250, 190, 100), black);
        drawRoundedPart(buf, x - 12 * s, y - 2 * s, 24 * s, 6 * s, C16(105, 55, 25), C16(150, 85, 45), black);
        drawRect(buf, x - 10 * s, y - 3 * s, 20 * s, 2 * s, gold);
        drawRoundedPart(buf, x - 13 * s, y - 5 * s, 26 * s, 4 * s, C16(46, 210, 90), C16(110, 245, 140), black);
        // כיפת לחמנייה עגולה אמיתית!
        drawCircle(buf, x, y - 7 * s, 11 * s, black);
        drawCircle(buf, x, y - 7 * s, 10 * s, C16(235, 165, 70));
        drawCircle(buf, x - 3 * s, y - 10 * s, 6 * s, C16(250, 195, 110));
        // שומשום
        drawRect(buf, x - 5 * s, y - 14 * s, 2 * s, 1 * s, white);
        drawRect(buf, x + 1 * s, y - 15 * s, 2 * s, 1 * s, white);
        drawRect(buf, x + 5 * s, y - 13 * s, 2 * s, 1 * s, white);
        drawDynamicFace(buf, x, y - 11 * s, s, black, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    if (ch->shape == SHAPE_PIZZA) {
        // משולש פיצה מעוגל בקרום עם גבינה מותכת ופפרוני
        drawRoundedPart(buf, x - 12 * s, y - 16 * s, 24 * s, 6 * s, C16(215, 135, 50), C16(245, 175, 90), black);
        for (int r = 0; r < 20 * s; r++) {
            int hw = (20 * s - r) / 2;
            drawRect(buf, x - hw - 1, y - 10 * s + r, hw * 2 + 2, 1, black);
            drawRect(buf, x - hw,     y - 10 * s + r, hw * 2,     1, C16(255, 210, 35));
        }
        drawCircle(buf, x - 4 * s, y - 4 * s, 3 * s, red);
        drawCircle(buf, x + 4 * s, y - 2 * s, 3 * s, red);
        drawDynamicFace(buf, x, y - 9 * s, s, black, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    if (ch->shape == SHAPE_SUSHI) {
        // רול סושי מעוגל עם אורז, אצה וסלמון
        drawRoundedPart(buf, x - 10 * s, y - 7 * s, 20 * s, 14 * s, white, white, black);
        drawRoundedPart(buf, x - 11 * s, y - 13 * s, 22 * s, 7 * s, C16(255, 110, 60), C16(255, 175, 140), black);
        drawRoundedPart(buf, x - 4 * s, y - 13 * s, 8 * s, 20 * s, C16(20, 45, 25), C16(45, 80, 50), black);
        drawDynamicFace(buf, x, y - 5 * s, s, black, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    if (ch->shape == SHAPE_DONUT) {
        drawCircle(buf, x, y - 3 * s, 12 * s, black);
        drawCircle(buf, x, y - 3 * s, 11 * s, C16(230, 165, 85));
        drawCircle(buf, x, y - 4 * s, 9 * s,  ch->mainColor);
        drawCircle(buf, x, y - 3 * s, 3 * s,  black);
        drawRect(buf, x - 6 * s, y - 9 * s, 3 * s, 1 * s, gold);
        drawRect(buf, x + 4 * s, y - 8 * s, 3 * s, 1 * s, C16(0, 240, 255));
        drawDynamicFace(buf, x, y - 7 * s, s, black, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    if (ch->shape == SHAPE_BANANA) {
        // בננה מעוגלת, קמורה ולא מרובעת! עם קליפות פתוחות בצדדים!
        drawRoundedPart(buf, x - 11 * s, y - 4 * s, 6 * s, 9 * s, C16(240, 210, 30), C16(255, 245, 120), black);
        drawRoundedPart(buf, x + 5 * s,  y - 4 * s, 6 * s, 9 * s, C16(240, 210, 30), C16(255, 245, 120), black);
        drawCircle(buf, x, y - 10 * s, 7 * s, black);
        drawCircle(buf, x, y + 1 * s,  7 * s, black);
        drawRoundedPart(buf, x - 7 * s, y - 15 * s, 14 * s, 22 * s, C16(255, 235, 45), C16(255, 250, 160), black);
        drawRoundedPart(buf, x - 3 * s, y - 19 * s, 6 * s, 5 * s, C16(110, 75, 35), C16(150, 105, 55), black);
        drawDynamicFace(buf, x, y - 10 * s, s, black, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    // 4. דמויות מיינקראפט (סטיב, קריפר, זומבי, אנדרמן) - נשארות קובייתיות בכוונה לפי המקור!
    if (ch->shape == SHAPE_MINECRAFT) {
        drawBoxOut(buf, x - 7 * s, y - 4 * s, 14 * s, 11 * s, ch->mainColor, black);
        u16 headCol = (charIdx == 17) ? skin : ch->mainColor;
        drawBoxOut(buf, x - 8 * s, y - 17 * s + headBobY * s, 16 * s, 14 * s, headCol, black);

        if (charIdx == 17) { // סטיב + מכוש יהלום
            drawBoxOut(buf, x - 8 * s, y - 17 * s + headBobY * s, 16 * s, 5 * s, C16(95, 60, 35), black);
            drawDynamicFace(buf, x, y - 11 * s + headBobY * s, s, C16(50, 80, 200), faceExpr, frameCnt);
            drawLine(buf, x + 8 * s, y + 2 * s, x + 14 * s, y - 8 * s, C16(130, 85, 40));
            drawLine(buf, x + 11 * s, y - 10 * s, x + 16 * s, y - 6 * s, C16(0, 240, 255));
        } else if (charIdx == 16 && faceExpr == EXPR_NORMAL) { // קריפר
            drawRect(buf, x - 5 * s, y - 13 * s, 3 * s, 3 * s, black);
            drawRect(buf, x + 2 * s, y - 13 * s, 3 * s, 3 * s, black);
            drawRect(buf, x - 2 * s, y - 10 * s, 4 * s, 4 * s, black);
            drawRect(buf, x - 4 * s, y - 8 * s,  2 * s, 4 * s, black);
            drawRect(buf, x + 2 * s, y - 8 * s,  2 * s, 4 * s, black);
        } else {
            u16 pCol = (charIdx == 41) ? C16(220, 40, 255) : red;
            drawDynamicFace(buf, x, y - 11 * s + headBobY * s, s, pCol, faceExpr, frameCnt);
        }
        goto draw_hit_fx;
    }

    // 5. חיות פרוותיות ומעוגלות (חתול, כלב, פנדה, שועל, אריה, נמר, ארנב, חד קרן):
    if (ch->shape == SHAPE_ANIMAL_EARS) {
        // זנב עגול וגוף אליפטי מעוגל
        drawCircle(buf, x - 10 * s, y + 2 * s, 4 * s, black);
        drawCircle(buf, x - 10 * s, y + 2 * s, 3 * s, ch->subColor);
        drawRoundedPart(buf, x - 7 * s, y - 4 * s, 14 * s, 11 * s, ch->mainColor, white, black);
        drawCircle(buf, x, y + 1 * s, 4 * s, white);

        int hx = x + headTiltX * s;
        int hy = y - 10 * s + headBobY * s;

        if (charIdx == 49) { // רעמת אריה עגולה
            drawCircle(buf, hx, hy, 11 * s, black);
            drawCircle(buf, hx, hy, 10 * s, C16(175, 85, 20));
        }

        // אוזניים מעוגלות
        int earH = (charIdx == 53) ? 9 * s : 5 * s;
        drawRoundedPart(buf, hx - 7 * s, hy - 6 * s - earH, 5 * s, earH + 3 * s, ch->mainColor, white, black);
        drawRoundedPart(buf, hx + 2 * s, hy - 6 * s - earH, 5 * s, earH + 3 * s, ch->mainColor, white, black);
        drawRect(buf, hx - 5 * s, hy - 4 * s - earH, 2 * s, earH - 1 * s, C16(255, 165, 195));
        drawRect(buf, hx + 3 * s, hy - 4 * s - earH, 2 * s, earH - 1 * s, C16(255, 165, 195));

        // ראש עגול שלם!
        drawCircle(buf, hx, hy, 8 * s, black);
        drawCircle(buf, hx, hy, 7 * s, ch->mainColor);

        if (charIdx == 60) { // קרן חד-קרן מוזהבת
            drawRoundedPart(buf, hx - 2 * s, hy - 14 * s, 4 * s, 8 * s, gold, white, black);
        }

        drawCircle(buf, hx, hy + 2 * s, 3 * s, white);
        drawDynamicFace(buf, hx, hy - 2 * s, s, black, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    // 6. כל הדמויות האנושיות, הגיבורים והלוחמים - בעיצוב אנטומי עגול, מפורט ולא מרובע!
    // פלג גוף עליון מעוגל עם חגורה
    drawRoundedPart(buf, x - 7 * s, y - 4 * s, 14 * s, 11 * s, ch->mainColor, white, black);
    drawRect(buf, x - 6 * s, y + 3 * s, 12 * s, 2 * s, C16(50, 35, 25));
    drawRect(buf, x - 2 * s, y + 3 * s, 4 * s, 2 * s, gold);

    // סמלי חזה מיוחדים לגיבורי על (סופר-איש = 44, איש העטלף = 45, איש העכביש = 46, האקר = 42):
    if (charIdx == 44) { // סמל יהלום אדום-צהוב לסופר-איש
        drawCircle(buf, x, y - 1 * s, 3 * s, gold);
        drawRect(buf, x - 2 * s, y - 1 * s, 4 * s, 2 * s, red);
    } else if (charIdx == 45) { // סמל העטלף הצהוב על החזה של איש העטלף
        drawRect(buf, x - 4 * s, y - 2 * s, 8 * s, 3 * s, gold);
        drawRect(buf, x - 3 * s, y - 1 * s, 6 * s, 1 * s, black);
    } else if (charIdx == 46) { // רשת וסמל עכביש לאיש העכביש
        drawRect(buf, x - 1 * s, y - 2 * s, 2 * s, 4 * s, black);
        drawLine(buf, x - 4 * s, y - 2 * s, x + 4 * s, y + 1 * s, black);
        drawLine(buf, x - 4 * s, y + 1 * s, x + 4 * s, y - 2 * s, black);
    } else if (charIdx == 42) { // סמל בינארי זוהר להאקר
        drawRect(buf, x - 3 * s, y - 2 * s, 6 * s, 3 * s, C16(118, 255, 3));
    }

    // כתפיים וזרועות מעוגלות עם כפפות עגולות (משתנות לפי האנימציה!)
    drawRoundedPart(buf, x - (11 + armSpread) * s, y - (3 - armOffY / 2) * s, 5 * s, 8 * s, ch->mainColor, white, black);
    drawRoundedPart(buf, x + (6 + armSpread) * s,  y - (3 - armOffY / 2) * s, 5 * s, 8 * s, ch->mainColor, white, black);
    drawCircle(buf, x - (9 + armSpread) * s, y + (4 + armOffY / 2) * s, 2 * s, skin);
    drawCircle(buf, x + (8 + armSpread) * s, y + (4 + armOffY / 2) * s, 2 * s, skin);

    // צוואר וראש אנושי עגול וטבעי (במקום ריבוע!)
    int hx = x + headTiltX * s;
    int hy = y - 10 * s + headBobY * s;
    drawRect(buf, hx - 2 * s, y - 5 * s, 4 * s, 3 * s, skin);

    u16 faceCol = (ch->shape == SHAPE_MONSTER) ? ch->mainColor : skin;
    if (charIdx == 46) faceCol = red;              // מסכת ספיידרמן אדומה עגולה
    if (charIdx == 1)  faceCol = C16(30, 32, 45);  // מסכת נינג'ה עגולה
    if (charIdx == 4)  faceCol = C16(135, 235, 255); // ראש קרח גבישי עגול

    // ציור גולגולת וראש עגולים לחלוטין עם אור וצל!
    drawCircle(buf, hx, hy, 8 * s, black);
    drawCircle(buf, hx, hy, 7 * s, faceCol);
    if (faceCol == skin) {
        drawCircle(buf, hx - 2 * s, hy - 2 * s, 3 * s, skinLit);
    }

    // אפקטים מיוחדים לראש לפי הדמות:
    if (charIdx == 1) {
        // נינג'ה: חרב קטאנה באלכסון, פתח עיניים מעוגל וסרט מצח מתנופף
        drawLine(buf, x - 8 * s, y - 4 * s, x + 9 * s, y - 19 * s, silver);
        drawRoundedPart(buf, hx - 6 * s, hy - 3 * s, 12 * s, 5 * s, skin, skinLit, black);
        drawRoundedPart(buf, hx - 8 * s, hy - 6 * s, 16 * s, 4 * s, red, C16(255, 110, 110), black);
        drawDynamicFace(buf, hx, hy - 2 * s, s, black, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    if (charIdx == 2) {
        // חייזר: ראש חייזרי עגול ורחב עם אנטנות זוהרות ועיניים חיות!
        drawCircle(buf, hx, hy - 1 * s, 9 * s, black);
        drawCircle(buf, hx, hy - 1 * s, 8 * s, C16(46, 215, 110));
        drawLine(buf, hx - 4 * s, hy - 8 * s, hx - 7 * s, hy - 13 * s, C16(46, 215, 110));
        drawLine(buf, hx + 4 * s, hy - 8 * s, hx + 7 * s, hy - 13 * s, C16(46, 215, 110));
        drawCircle(buf, hx - 7 * s, hy - 13 * s, 2 * s, C16(0, 255, 255));
        drawCircle(buf, hx + 7 * s, hy - 13 * s, 2 * s, C16(0, 255, 255));
        drawDynamicFace(buf, hx, hy - 2 * s, s, black, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    if (charIdx == 45) {
        // איש העטלף: מסכת עטלף מעוגלת עם אוזניים מחודדות ועיניים לבנות!
        drawCircle(buf, hx, hy - 1 * s, 8 * s, black);
        drawCircle(buf, hx, hy - 1 * s, 7 * s, C16(35, 40, 55));
        drawRoundedPart(buf, hx - 7 * s, hy - 12 * s, 4 * s, 7 * s, C16(35, 40, 55), silver, black);
        drawRoundedPart(buf, hx + 3 * s, hy - 12 * s, 4 * s, 7 * s, C16(35, 40, 55), silver, black);
        drawRoundedPart(buf, hx - 4 * s, hy + 1 * s, 8 * s, 5 * s, skin, skinLit, black);
        drawDynamicFace(buf, hx, hy - 2 * s, s, white, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    if (charIdx == 46) {
        // איש העכביש: עיני מסכה לבנות אלכסוניות + הבעה משתנה!
        drawDynamicFace(buf, hx, hy - 2 * s, s, white, faceExpr, frameCnt);
        goto draw_hit_fx;
    }

    if (charIdx == 42 || charIdx == 35) {
        // האקר / מטריקס: קפוצ'ון (Hoodie) כהה מעוגל ומשקפי סייבר ירוקים זוהרים!
        drawCircle(buf, hx, hy - 1 * s, 9 * s, black);
        drawCircle(buf, hx, hy - 1 * s, 8 * s, C16(25, 35, 30));
        drawCircle(buf, hx, hy, 6 * s, skin);
        drawRoundedPart(buf, hx - 6 * s, hy - 3 * s, 12 * s, 4 * s, C16(118, 255, 3), white, black);
        if (faceExpr != EXPR_NORMAL) {
            drawDynamicFace(buf, hx, hy - 2 * s, s, black, faceExpr, frameCnt);
        }
        goto draw_hit_fx;
    }

    // ציור הפנים הדינמיות לכל שאר הדמויות
    drawDynamicFace(buf, hx, hy - 2 * s, s, black, faceExpr, frameCnt);

    // זקן עגול ומטה קסמים לקוסם (5)
    if (charIdx == 5 || charIdx == 82 || charIdx == 83) {
        drawCircle(buf, hx, hy + 5 * s, 5 * s, white);
        drawLine(buf, x + 11 * s, y + 12 * s, x + 11 * s, y - 12 * s, C16(130, 80, 35));
        drawCircle(buf, x + 11 * s, y - 14 * s, 4 * s, C16(0, 245, 255));
    }

    // שרביט מלכות מוזהב למלך (3)
    if (charIdx == 3) {
        drawLine(buf, x + 11 * s, y + 10 * s, x + 11 * s, y - 10 * s, gold);
        drawCircle(buf, x + 11 * s, y - 12 * s, 3 * s, red);
    }

    // כובעים, כתרים, קסדות אבירים ומצנפות מעוגלות:
    if (ch->specialAcc == 1) {
        // כתר מלכותי מעוגל ומשובץ אבני חן
        drawRoundedPart(buf, hx - 8 * s, hy - 10 * s, 16 * s, 5 * s, gold, white, black);
        drawCircle(buf, hx - 5 * s, hy - 11 * s, 2 * s, gold);
        drawCircle(buf, hx,         hy - 12 * s, 2 * s, gold);
        drawCircle(buf, hx + 5 * s, hy - 11 * s, 2 * s, gold);
        drawRect(buf, hx, hy - 9 * s, 2 * s, 2 * s, red);
    } else if (ch->specialAcc == 2) {
        // מצנפת קוסם / כובע ליצן עגול עם פונפונים!
        drawRoundedPart(buf, hx - 11 * s, hy - 8 * s, 22 * s, 4 * s, ch->subColor, white, black);
        drawCircle(buf, hx, hy - 11 * s, 6 * s, ch->mainColor);
        drawCircle(buf, hx, hy - 15 * s, 4 * s, ch->mainColor);
        drawCircle(buf, hx, hy - 18 * s, 2 * s, gold);
    } else if (ch->specialAcc == 3) {
        // קסדה עגולה עם קרניים מעוקלות
        drawRoundedPart(buf, hx - 8 * s, hy - 10 * s, 16 * s, 5 * s, ch->subColor, white, black);
        drawCircle(buf, hx - 9 * s, hy - 11 * s, 3 * s, white);
        drawCircle(buf, hx + 9 * s, hy - 11 * s, 3 * s, white);
    } else if (ch->specialAcc == 4) {
        // הילת מלאך מעוגלת וזוהרת
        drawRoundedPart(buf, hx - 7 * s, hy - 13 * s, 14 * s, 4 * s, gold, white, black);
    } else if (ch->specialAcc == 5) {
        // קסדת אביר עגולה עם נוצה מתנופפת
        drawCircle(buf, hx, hy - 4 * s, 8 * s, silver);
        drawRoundedPart(buf, hx - 2 * s, hy - 14 * s, 5 * s, 6 * s, red, white, black);
    } else {
        // כובע/תספורת עגולה ומעוצבת (ללא פינות מרובעות!)
        if (charIdx == 44) { // תספורת סופרמן כהה עם תלתל במצח
            drawRoundedPart(buf, hx - 8 * s, hy - 9 * s, 16 * s, 5 * s, C16(25, 30, 45), C16(60, 70, 100), black);
            drawCircle(buf, hx + 1 * s, hy - 5 * s, 2 * s, C16(25, 30, 45));
        } else {
            drawRoundedPart(buf, hx - 8 * s, hy - 10 * s, 16 * s, 5 * s, ch->subColor, white, black);
            drawRoundedPart(buf, hx - 3 * s, hy - 7 * s, 13 * s, 3 * s, ch->mainColor, white, black);
        }
    }

draw_hit_fx:
    // =========================================================================
    // אפקטים ויזואליים ייחודיים ל-2 אנימציות הפגיעה (קרקע לעומת מעופף)!
    // =========================================================================
    if (animState == CHAR_ANIM_HIT_GROUND) {
        // כוכבי סחרחורת זהובים שמסתובבים מעל הראש בהיתקלות באבן/אויב קרקע!
        int starSpin = (frameCnt / 3) % 3;
        drawCircle(buf, x - 8 * s + starSpin * 3 * s, y - 21 * s, 2 * s, gold);
        drawCircle(buf, x + 8 * s - starSpin * 3 * s, y - 21 * s, 2 * s, C16(255, 100, 0));
        drawCircle(buf, x,                            y - 23 * s, 2 * s, white);
    } else if (animState == CHAR_ANIM_HIT_AIR) {
        // גלי הדף וניצוצות ברק אדומים-צהובים בפגיעה מאויב מעופף!
        drawLine(buf, x - 12 * s, y - 18 * s, x - 5 * s, y - 11 * s, gold);
        drawLine(buf, x + 12 * s, y - 18 * s, x + 5 * s, y - 11 * s, red);
        drawCircle(buf, x, y - 16 * s, 3 * s, gold);
    }
}

// שמירה על תאימות מלאה לכל הקריאות הקיימות בקוד
void drawCharacterSprite(u16* buf, int charIdx, int cx, int cy, int scalePct, bool isMoving, bool grounded, int angle, int frameCnt) {
    enum CharAnimState st = grounded ? CHAR_ANIM_WALK : CHAR_ANIM_JUMP;
    drawCharacterSpriteEx(buf, charIdx, cx, cy, scalePct, st, currentFaceExpr, angle, frameCnt);
}
 
