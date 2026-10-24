#include "game.h"
#include <stdlib.h>

#undef C16
#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

// פונקציית עזר לציור מלבן עם מסגרת כהה והצללה פנימית
static void drawBoxOut(u16* buf, int x, int y, int w, int h, u16 fill, u16 out) {
    drawRect(buf, x, y, w, h, out);
    if (w > 2 && h > 2) {
        drawRect(buf, x + 1, y + 1, w - 2, h - 2, fill);
    }
}

// פונקציית עזר לעיניים מפורטות עם לובן, אישון וניצוץ
static void drawDetailedEyes(u16* buf, int cx, int y, int s, u16 pupilCol) {
    u16 white = C16(255, 255, 255);
    // עין שמאל
    drawRect(buf, cx - 5 * s, y, 3 * s, 3 * s, white);
    drawRect(buf, cx - 4 * s, y + 1 * s, 2 * s, 2 * s, pupilCol);
    drawRect(buf, cx - 5 * s, y, 1 * s, 1 * s, white);
    // עין ימין
    drawRect(buf, cx + 2 * s, y, 3 * s, 3 * s, white);
    drawRect(buf, cx + 3 * s, y + 1 * s, 2 * s, 2 * s, pupilCol);
    drawRect(buf, cx + 2 * s, y, 1 * s, 1 * s, white);
}

// ציור מפורט וייחודי לכל אחת מ-99 הדמויות לפי השם והאינדקס שלה!
void drawCharacterSprite(u16* buf, int charIdx, int cx, int cy, int scalePct, bool isMoving, bool grounded, int angle, int frameCnt) {
    if (charIdx < 0 || charIdx >= NUM_CHARS) charIdx = 0;
    const CharData* ch = &CHARACTERS[charIdx];

    int legOff = 0;
    int bodyBob = 0;
    if (isMoving && grounded) {
        int phase = (frameCnt / 4) % 4;
        int amp = (ch->style == STYLE_WOBBLE) ? 6 : 4;
        if (phase == 0) legOff = -amp;
        else if (phase == 2) legOff = amp;
        bodyBob = (phase % 2 == 0) ? -1 : 1;
    } else if (!grounded) {
        legOff = 3;
        if (ch->style == STYLE_SPIN) bodyBob = ((frameCnt / 2) % 2 == 0) ? -2 : 2;
    }

    int y = cy + bodyBob;
    int s = (scalePct >= 120) ? 2 : 1;

    u16 black   = C16(18, 18, 24);
    u16 white   = C16(252, 252, 255);
    u16 skin    = C16(250, 200, 145);
    u16 skinShd = C16(215, 155, 100);
    u16 gold    = C16(255, 215, 0);
    u16 red     = C16(235, 35, 35);
    u16 silver  = C16(200, 210, 225);

    // 1. גלימות או כנפיים מאחורי הגוף
    if (charIdx == 3 || charIdx == 44 || charIdx == 21 || charIdx == 90) {
        // גלימת מלכות אדומה למלך (3), סופר-איש (44), ערפד (21), דרקולה (90)
        drawBoxOut(buf, cx - 10 * s, y - 5 * s, 20 * s, 16 * s, red, black);
        if (charIdx == 3) { // עיטור פרווה לבנה לגלימת המלך
            drawRect(buf, cx - 9 * s, y - 5 * s, 18 * s, 3 * s, white);
        }
    } else if (ch->shape == SHAPE_WINGED) {
        int flap = (!grounded || isMoving) ? (((frameCnt / 3) % 2) * 5 - 2) : 0;
        drawBoxOut(buf, cx - 16 * s, y - (6 + flap) * s, 8 * s, 11 * s, ch->subColor, black);
        drawBoxOut(buf, cx + 8 * s,  y - (6 + flap) * s, 8 * s, 11 * s, ch->subColor, black);
    }

    // 2. רגליים מפורטות עם מכנסיים ונעליים/מגפיים
    drawBoxOut(buf, cx - (6 + legOff / 2) * s, y + 5 * s, 5 * s, 7 * s, C16(45, 55, 80), black);
    drawBoxOut(buf, cx + (1 + legOff / 2) * s, y + 5 * s, 5 * s, 7 * s, C16(45, 55, 80), black);
    // נעליים עם סוליה
    drawBoxOut(buf, cx - (7 + legOff / 2) * s, y + 11 * s, 6 * s, 4 * s, C16(75, 45, 25), black);
    drawBoxOut(buf, cx + (1 + legOff / 2) * s, y + 11 * s, 6 * s, 4 * s, C16(75, 45, 25), black);

    // 3. עיצוב ייחודי ומפורט לדמויות האוכל:
    if (ch->shape == SHAPE_BURGER) {
        // המבורגר גורמה רב-שכבתי עם קו מתאר!
        drawBoxOut(buf, cx - 11 * s, y + 3 * s, 22 * s, 5 * s, C16(225, 150, 60), black); // לחמנייה תחתונה
        drawBoxOut(buf, cx - 12 * s, y - 2 * s, 24 * s, 6 * s, C16(105, 55, 25), black);  // קציצת בשר עסיסית
        drawBoxOut(buf, cx - 11 * s, y - 4 * s, 22 * s, 3 * s, gold, black);              // גבינה צהובה נמסה
        drawBoxOut(buf, cx - 13 * s, y - 6 * s, 26 * s, 3 * s, C16(46, 210, 90), black);  // חסה פריכה
        drawBoxOut(buf, cx - 10 * s, y - 8 * s, 20 * s, 3 * s, red, black);               // פרוסת עגבנייה
        drawBoxOut(buf, cx - 11 * s, y - 16 * s, 22 * s, 9 * s, C16(235, 165, 70), black);// לחמנייה עליונה
        // זרעי שומשום לבנים
        drawRect(buf, cx - 6 * s, y - 14 * s, 2 * s, 1 * s, white);
        drawRect(buf, cx,         y - 15 * s, 2 * s, 1 * s, white);
        drawRect(buf, cx + 5 * s, y - 14 * s, 2 * s, 1 * s, white);
        drawDetailedEyes(buf, cx, y - 12 * s, s, black);
        return;
    }

    if (ch->shape == SHAPE_PIZZA) {
        // משולש פיצה איטלקית חמה עם קרום אפוי, גבינה מבעבעת ופפרוני!
        drawBoxOut(buf, cx - 12 * s, y - 16 * s, 24 * s, 5 * s, C16(215, 135, 50), black);
        for (int r = 0; r < 20 * s; r++) {
            int hw = (20 * s - r) / 2;
            drawRect(buf, cx - hw - 1, y - 11 * s + r, hw * 2 + 2, 1, black);
            drawRect(buf, cx - hw,     y - 11 * s + r, hw * 2,     1, C16(255, 210, 35));
        }
        drawCircle(buf, cx - 4 * s, y - 5 * s, 3 * s, red);
        drawCircle(buf, cx + 4 * s, y - 3 * s, 3 * s, red);
        drawCircle(buf, cx,         y + 3 * s, 2 * s, red);
        drawDetailedEyes(buf, cx, y - 9 * s, s, black);
        return;
    }

    if (ch->shape == SHAPE_SUSHI) {
        // סושי ניגירי מפורט עם גרגרי אורז, חבק אצת נורי ונתח סלמון מפוספס!
        drawBoxOut(buf, cx - 10 * s, y - 7 * s, 20 * s, 14 * s, white, black);
        drawBoxOut(buf, cx - 12 * s, y - 13 * s, 24 * s, 7 * s, C16(255, 110, 60), black);
        drawLine(buf, cx - 8 * s, y - 12 * s, cx - 4 * s, y - 7 * s, C16(255, 200, 175));
        drawLine(buf, cx + 2 * s, y - 12 * s, cx + 6 * s, y - 7 * s, C16(255, 200, 175));
        drawBoxOut(buf, cx - 4 * s, y - 13 * s, 8 * s, 20 * s, C16(20, 45, 25), black);
        drawDetailedEyes(buf, cx, y - 4 * s, s, black);
        return;
    }

    if (ch->shape == SHAPE_DONUT) {
        // דונאט מזוגג עם קו מתאר, חור וסוכריות צבעוניות!
        drawCircle(buf, cx, y - 3 * s, 12 * s, black);
        drawCircle(buf, cx, y - 3 * s, 11 * s, C16(230, 165, 85));
        drawCircle(buf, cx, y - 4 * s, 9 * s,  ch->mainColor);
        drawCircle(buf, cx, y - 3 * s, 3 * s,  black);
        drawRect(buf, cx - 6 * s, y - 9 * s, 3 * s, 1 * s, gold);
        drawRect(buf, cx + 4 * s, y - 8 * s, 3 * s, 1 * s, C16(0, 240, 255));
        drawRect(buf, cx - 5 * s, y + 2 * s, 3 * s, 1 * s, white);
        drawDetailedEyes(buf, cx, y - 6 * s, s, black);
        return;
    }

    if (ch->shape == SHAPE_BANANA) {
        // בננה מקולפת למחצה עם כתר חום וחיוך!
        drawBoxOut(buf, cx - 4 * s, y - 18 * s, 8 * s, 4 * s, C16(110, 75, 35), black);
        drawBoxOut(buf, cx - 7 * s, y - 14 * s, 14 * s, 21 * s, C16(255, 235, 45), black);
        drawRect(buf, cx - 5 * s, y - 12 * s, 3 * s, 17 * s, C16(255, 250, 150));
        drawDetailedEyes(buf, cx, y - 8 * s, s, black);
        drawRect(buf, cx - 2 * s, y - 3 * s, 4 * s, 2 * s, red);
        return;
    }

    // 4. עיצוב מפורט לדמויות מיינקראפט (סטיב, קריפר, זומבי, אנדרמן):
    if (ch->shape == SHAPE_MINECRAFT) {
        // גוף מרובע עם חולצה וחגורה
        drawBoxOut(buf, cx - 7 * s, y - 4 * s, 14 * s, 11 * s, ch->mainColor, black);
        // ראש מרובע גדול
        u16 headCol = (charIdx == 17) ? skin : ch->mainColor;
        drawBoxOut(buf, cx - 8 * s, y - 17 * s, 16 * s, 14 * s, headCol, black);

        if (charIdx == 17) { // סטיב: תספורת מרובעת חומה, צווארון ופה/זקן, ומכוש יהלום ביד!
            drawBoxOut(buf, cx - 8 * s, y - 17 * s, 16 * s, 5 * s, C16(95, 60, 35), black);
            drawDetailedEyes(buf, cx, y - 11 * s, s, C16(50, 80, 200));
            drawRect(buf, cx - 3 * s, y - 7 * s, 6 * s, 2 * s, C16(135, 85, 55));
            // מכוש יהלום ביד ימין של סטיב!
            drawLine(buf, cx + 8 * s, y + 2 * s, cx + 14 * s, y - 8 * s, C16(130, 85, 40));
            drawLine(buf, cx + 11 * s, y - 10 * s, cx + 16 * s, y - 6 * s, C16(0, 240, 255));
        } else if (charIdx == 16) { // קריפר מפורט עם טקסטורת פיקסלים ירוקה והפנים האייקוניות!
            drawRect(buf, cx - 6 * s, y - 15 * s, 3 * s, 3 * s, C16(110, 215, 95));
            drawRect(buf, cx - 5 * s, y - 13 * s, 3 * s, 3 * s, black);
            drawRect(buf, cx + 2 * s, y - 13 * s, 3 * s, 3 * s, black);
            drawRect(buf, cx - 2 * s, y - 10 * s, 4 * s, 4 * s, black);
            drawRect(buf, cx - 4 * s, y - 8 * s,  2 * s, 4 * s, black);
            drawRect(buf, cx + 2 * s, y - 8 * s,  2 * s, 4 * s, black);
        } else {
            u16 pCol = (charIdx == 41) ? C16(220, 40, 255) : red;
            drawDetailedEyes(buf, cx, y - 11 * s, s, pCol);
        }
        return;
    }

    // 5. חיות מפורטות (חתול, כלב, פנדה, שועל, אריה, נמר, ארנב, חד קרן וכו'):
    if (ch->shape == SHAPE_ANIMAL_EARS) {
        // זנב מאחורי הגוף
        drawBoxOut(buf, cx - 12 * s, y - 1 * s, 6 * s, 4 * s, ch->subColor, black);
        // גוף עם כתם בטן בהיר
        drawBoxOut(buf, cx - 7 * s, y - 4 * s, 14 * s, 11 * s, ch->mainColor, black);
        drawRect(buf, cx - 4 * s, y - 2 * s, 8 * s, 7 * s, white);

        // רעמת אריה מפוארת (אריה = 49)
        if (charIdx == 49) {
            drawCircle(buf, cx, y - 10 * s, 11 * s, C16(175, 85, 20));
        }

        // ראש החיה ואוזניים עם פנים ורוד
        int earH = (charIdx == 53) ? 10 * s : 6 * s; // אוזניים ארוכות לארנב
        drawBoxOut(buf, cx - 7 * s, y - 16 * s - earH / 2, 5 * s, earH, ch->mainColor, black);
        drawBoxOut(buf, cx + 2 * s, y - 16 * s - earH / 2, 5 * s, earH, ch->mainColor, black);
        drawRect(buf, cx - 5 * s, y - 15 * s - earH / 2, 2 * s, earH - 2 * s, C16(255, 160, 190));
        drawRect(buf, cx + 3 * s, y - 15 * s - earH / 2, 2 * s, earH - 2 * s, C16(255, 160, 190));

        drawBoxOut(buf, cx - 8 * s, y - 16 * s, 16 * s, 12 * s, ch->mainColor, black);

        // כתמים שחורים בעיניים לפנדה (11)
        if (charIdx == 11) {
            drawRect(buf, cx - 6 * s, y - 13 * s, 4 * s, 5 * s, black);
            drawRect(buf, cx + 2 * s, y - 13 * s, 4 * s, 5 * s, black);
        }

        // קרן מוזהבת ורעמת קשת לחד-קרן (60)
        if (charIdx == 60) {
            drawBoxOut(buf, cx - 2 * s, y - 24 * s, 4 * s, 8 * s, gold, black);
        }

        // חוטם בולט, אף משולש ושפם
        drawBoxOut(buf, cx - 4 * s, y - 9 * s, 8 * s, 5 * s, white, black);
        drawRect(buf, cx - 2 * s, y - 9 * s, 4 * s, 2 * s, C16(255, 110, 140));
        drawDetailedEyes(buf, cx, y - 13 * s, s, black);
        return;
    }

    // 6. גוף הדמויות האנושיות, הגיבורים והלוחמים (כולל 6 הדמויות הראשונות בחנות!):
    // טורסו עם חגורה ואבזם זהב
    drawBoxOut(buf, cx - 7 * s, y - 4 * s, 14 * s, 11 * s, ch->mainColor, black);
    drawRect(buf, cx - 6 * s, y + 3 * s, 12 * s, 2 * s, C16(50, 35, 25)); // חגורה
    drawRect(buf, cx - 2 * s, y + 3 * s, 4 * s, 2 * s, gold);              // אבזם חגורה

    // ידיים ושרוולים עם כפפות
    drawBoxOut(buf, cx - 11 * s, y - 3 * s, 5 * s, 7 * s, ch->mainColor, black);
    drawBoxOut(buf, cx + 6 * s,  y - 3 * s, 5 * s, 7 * s, ch->mainColor, black);
    drawBoxOut(buf, cx - 11 * s, y + 3 * s, 4 * s, 3 * s, skin, black);
    drawBoxOut(buf, cx + 7 * s,  y + 3 * s, 4 * s, 3 * s, skin, black);

    // ציור ייחודי ומדויק ל-6 הדמויות הראשונות בחנות ולדמויות המיוחדות:
    if (charIdx == 1) {
        // נינג'ה (אינדקס 1): חליפת שינובי שחורה, רצועת חזה אדומה, חרב קטאנה על הגב ומסכת פנים מלאה!
        drawLine(buf, cx - 8 * s, y - 4 * s, cx + 8 * s, y - 19 * s, silver); // להב חרב קטאנה מאחורי הכתף!
        drawBoxOut(buf, cx + 6 * s, y - 21 * s, 4 * s, 4 * s, gold, black);   // ניצב החרב
        drawLine(buf, cx - 5 * s, y - 3 * s, cx + 5 * s, y + 4 * s, red);     // רצועת חזה אדומה
        // ראש עם מסכת נינג'ה שחורה ופתח צר לעיניים
        drawBoxOut(buf, cx - 8 * s, y - 16 * s, 16 * s, 13 * s, C16(30, 32, 42), black);
        drawBoxOut(buf, cx - 7 * s, y - 12 * s, 14 * s, 5 * s, skin, black);
        // סרט מצח אדום עם קשר מתנופף בצד!
        drawBoxOut(buf, cx - 8 * s, y - 15 * s, 16 * s, 4 * s, red, black);
        drawBoxOut(buf, cx - 13 * s, y - 15 * s, 5 * s, 3 * s, red, black);
        drawBoxOut(buf, cx - 15 * s, y - 12 * s, 5 * s, 3 * s, red, black);
        drawDetailedEyes(buf, cx, y - 11 * s, s, black);
        return;
    }

    if (charIdx == 2) {
        // חייזר (אינדקס 2): חליפת חלל כסופה, גולגולת חייזרית ירוקה רחבה, עיני שקד שחורות ענקיות ו-2 אנטנות!
        drawBoxOut(buf, cx - 9 * s, y - 17 * s, 18 * s, 13 * s, C16(46, 215, 110), black);
        // אנטנות חייזר עם קצה זוהר
        drawLine(buf, cx - 5 * s, y - 17 * s, cx - 8 * s, y - 22 * s, C16(46, 215, 110));
        drawLine(buf, cx + 5 * s, y - 17 * s, cx + 8 * s, y - 22 * s, C16(46, 215, 110));
        drawCircle(buf, cx - 8 * s, y - 22 * s, 2 * s, C16(0, 255, 255));
        drawCircle(buf, cx + 8 * s, y - 22 * s, 2 * s, C16(0, 255, 255));
        // עיני חייזר שחורות ענקיות עם ניצוץ לבן
        drawBoxOut(buf, cx - 7 * s, y - 13 * s, 6 * s, 6 * s, black, black);
        drawBoxOut(buf, cx + 1 * s, y - 13 * s, 6 * s, 6 * s, black, black);
        drawRect(buf, cx - 6 * s, y - 12 * s, 2 * s, 2 * s, white);
        drawRect(buf, cx + 2 * s, y - 12 * s, 2 * s, 2 * s, white);
        return;
    }

    if (charIdx == 4) {
        // איש קרח (אינדקס 4): גוף קרח גבישי זוהר, כתפיות קרח חדות וכתר קוצי כפור תכלת-לבן!
        drawBoxOut(buf, cx - 8 * s, y - 16 * s, 16 * s, 13 * s, C16(130, 235, 255), C16(20, 90, 160));
        // קוצי קרח בראש
        drawBoxOut(buf, cx - 7 * s, y - 21 * s, 4 * s, 6 * s, white, C16(20, 90, 160));
        drawBoxOut(buf, cx - 2 * s, y - 23 * s, 4 * s, 8 * s, white, C16(20, 90, 160));
        drawBoxOut(buf, cx + 3 * s, y - 21 * s, 4 * s, 6 * s, white, C16(20, 90, 160));
        drawDetailedEyes(buf, cx, y - 11 * s, s, C16(0, 110, 220));
        return;
    }

    // ראש אנושי מפורט עם הצללת פנים וגבות לשאר הדמויות
    u16 faceCol = (ch->shape == SHAPE_MONSTER) ? ch->mainColor : skin;
    drawBoxOut(buf, cx - 8 * s, y - 16 * s, 16 * s, 13 * s, faceCol, black);
    if (faceCol == skin) {
        drawRect(buf, cx - 7 * s, y - 6 * s, 14 * s, 2 * s, skinShd); // הצללת סנטר
    }
    drawDetailedEyes(buf, cx, y - 11 * s, s, black);
    drawRect(buf, cx - 2 * s, y - 6 * s, 4 * s, 1 * s, C16(160, 70, 60)); // פה

    // זקן לבן ארוך ומטה קסמים זוהר לקוסם (אינדקס 5) ולמכשפים!
    if (charIdx == 5 || charIdx == 82 || charIdx == 83) {
        drawBoxOut(buf, cx - 6 * s, y - 6 * s, 12 * s, 7 * s, white, black);
        drawBoxOut(buf, cx - 4 * s, y + 1 * s, 8 * s, 4 * s, white, black);
        // מטה קסמים עם כדור בדולח זוהר ביד!
        drawLine(buf, cx + 11 * s, y + 12 * s, cx + 11 * s, y - 12 * s, C16(130, 80, 35));
        drawCircle(buf, cx + 11 * s, y - 14 * s, 4 * s, C16(0, 245, 255));
        drawCircle(buf, cx + 11 * s, y - 14 * s, 2 * s, white);
    }

    // שרביט מלכות מוזהב וזקן מלכותי למלך (אינדקס 3)!
    if (charIdx == 3) {
        drawLine(buf, cx + 11 * s, y + 10 * s, cx + 11 * s, y - 10 * s, gold);
        drawCircle(buf, cx + 11 * s, y - 12 * s, 3 * s, red);
    }

    // כובעים, כתרים, קסדות אבירים ומצנפות מפורטות:
    if (ch->specialAcc == 1) {
        // כתר זהב משובץ באבני אודם וברקת!
        drawBoxOut(buf, cx - 9 * s, y - 19 * s, 18 * s, 5 * s, gold, black);
        drawBoxOut(buf, cx - 9 * s, y - 23 * s, 4 * s, 5 * s, gold, black);
        drawBoxOut(buf, cx - 2 * s, y - 24 * s, 4 * s, 6 * s, gold, black);
        drawBoxOut(buf, cx + 5 * s, y - 23 * s, 4 * s, 5 * s, gold, black);
        drawRect(buf, cx - 6 * s, y - 18 * s, 2 * s, 2 * s, red);
        drawRect(buf, cx,         y - 18 * s, 2 * s, 2 * s, C16(0, 220, 90));
        drawRect(buf, cx + 5 * s, y - 18 * s, 2 * s, 2 * s, red);
    } else if (ch->specialAcc == 2) {
        // מצנפת קוסם מחודדת עם שוליים רחבים וכוכב זהב!
        drawBoxOut(buf, cx - 12 * s, y - 17 * s, 24 * s, 4 * s, ch->subColor, black);
        drawBoxOut(buf, cx - 8 * s,  y - 22 * s, 16 * s, 6 * s, ch->mainColor, black);
        drawBoxOut(buf, cx - 5 * s,  y - 27 * s, 10 * s, 6 * s, ch->mainColor, black);
        drawBoxOut(buf, cx - 2 * s,  y - 30 * s, 5 * s,  4 * s, ch->mainColor, black);
        drawRect(buf, cx - 1 * s, y - 21 * s, 3 * s, 3 * s, gold);
    } else if (ch->specialAcc == 3) {
        // קסדת ויקינג / קרניים מעוקלות עם קו מתאר!
        drawBoxOut(buf, cx - 9 * s, y - 19 * s, 18 * s, 5 * s, ch->subColor, black);
        drawBoxOut(buf, cx - 13 * s, y - 23 * s, 5 * s, 6 * s, white, black);
        drawBoxOut(buf, cx + 8 * s,  y - 23 * s, 5 * s, 6 * s, white, black);
    } else if (ch->specialAcc == 4) {
        // הילת מלאך זוהרת מרחפת מעל הראש!
        drawBoxOut(buf, cx - 7 * s, y - 22 * s, 14 * s, 4 * s, gold, black);
        drawRect(buf, cx - 5 * s, y - 21 * s, 10 * s, 2 * s, white);
    } else if (ch->specialAcc == 5) {
        // קסדת אביר / רובוט / ספרטני עם נוצה ומשקף!
        drawBoxOut(buf, cx - 9 * s, y - 19 * s, 18 * s, 6 * s, silver, black);
        drawBoxOut(buf, cx - 3 * s, y - 24 * s, 6 * s, 6 * s, red, black);
        drawRect(buf, cx - 6 * s, y - 12 * s, 12 * s, 3 * s, ch->subColor);
    } else {
        // כובע מצחייה מעוצב לדמות הרגילה (אינדקס 0) ולשאר הדמויות!
        drawBoxOut(buf, cx - 9 * s, y - 19 * s, 18 * s, 5 * s, ch->subColor, black);
        drawBoxOut(buf, cx - 4 * s, y - 16 * s, 15 * s, 3 * s, ch->mainColor, black); // מצחייה בולטת קדימה
        drawRect(buf, cx - 2 * s, y - 18 * s, 3 * s, 2 * s, white);                   // סמל לבן על הכובע
    }
}
 
