#include "game.h"
#include <stdlib.h>

#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

// ציור מותאם אישית של כל 99 הדמויות לפי השם והצורה שלהן!
void drawCharacterSprite(u16* buf, int charIdx, int cx, int cy, int scalePct, bool isMoving, bool grounded, int angle, int frameCnt) {
    if (charIdx < 0 || charIdx >= NUM_CHARS) charIdx = 0;
    const CharData* ch = &CHARACTERS[charIdx];

    // חישוב היסט רגליים וידיים לפי סגנון הדמות
    int legOff = 0;
    int bodyBob = 0;
    if (isMoving && grounded) {
        int phase = (frameCnt / 4) % 4;
        int amp = (ch->style == STYLE_WOBBLE) ? 7 : ((ch->style == STYLE_BLOCKY) ? 5 : 4);
        if (phase == 0) legOff = -amp;
        else if (phase == 2) legOff = amp;
        bodyBob = (phase % 2 == 0) ? -1 : 1;
    } else if (!grounded) {
        // בתנועת קפיצה: קיפול רגליים או סיבוב סלטה
        legOff = (ch->style == STYLE_SPIN) ? ((frameCnt % 4) * 3 - 4) : 4;
        if (ch->style == STYLE_SPIN) {
            bodyBob = ((frameCnt / 2) % 2 == 0) ? -3 : 3;
        }
    }

    int y = cy + bodyBob;
    int s = (scalePct >= 120) ? 2 : 1; // הגדלה בתצוגת חנות אם נדרש

    u16 black = C16(20, 20, 20);
    u16 white = C16(250, 250, 250);
    u16 skin  = C16(245, 198, 135);
    u16 gold  = C16(255, 215, 0);
    u16 red   = C16(230, 40, 40);

    // 1. כנפיים או גלימה (אם זו דמות מעופפת: דרקון, מלאך, פניקס, באטמן, תרנגול, ערפד)
    if (ch->shape == SHAPE_WINGED) {
        int wingFlap = (!grounded || isMoving) ? (((frameCnt / 3) % 2) * 6 - 3) : 0;
        drawRect(buf, cx - 14 * s, y - (4 + wingFlap) * s, 7 * s, 9 * s, ch->subColor);
        drawRect(buf, cx + 8 * s,  y - (4 + wingFlap) * s, 7 * s, 9 * s, ch->subColor);
    }

    // 2. רגליים מותאמות
    drawLine(buf, cx - 4 * s, y + 6 * s, cx - (4 + legOff) * s, y + 14 * s, black);
    drawLine(buf, cx + 4 * s, y + 6 * s, cx + (4 + legOff) * s, y + 14 * s, black);

    // 3. ציור הגוף והראש לפי סוג הדמות המדויק!
    switch (ch->shape) {
        case SHAPE_BURGER: {
            // המבורגר אמיתי: לחמנייה תחתונה, קציצה, גבינה צהובה, חסה, עגבנייה, לחמנייה עליונה עם שומשום ועיניים!
            drawRect(buf, cx - 10 * s, y + 4 * s, 20 * s, 4 * s, C16(220, 145, 55)); // לחמנייה תחתונה
            drawRect(buf, cx - 11 * s, y - 1 * s, 22 * s, 5 * s, C16(95, 55, 30));   // קציצת בשר
            drawRect(buf, cx - 10 * s, y - 2 * s, 20 * s, 2 * s, gold);              // גבינה
            drawRect(buf, cx - 12 * s, y - 4 * s, 24 * s, 2 * s, C16(46, 204, 113)); // חסה
            drawRect(buf, cx - 9 * s,  y - 6 * s, 18 * s, 2 * s, red);               // עגבנייה
            drawCircle(buf, cx, y - 6 * s, 9 * s, C16(230, 155, 60));                // כיפת לחמנייה
            // שומשום
            drawRect(buf, cx - 4 * s, y - 12 * s, 2 * s, 1 * s, white);
            drawRect(buf, cx + 3 * s, y - 11 * s, 2 * s, 1 * s, white);
            // עיניים
            drawRect(buf, cx - 4 * s, y - 8 * s, 2 * s, 2 * s, black);
            drawRect(buf, cx + 3 * s, y - 8 * s, 2 * s, 2 * s, black);
            break;
        }
        case SHAPE_PIZZA: {
            // משולש פיצה עם גבינה מותכת, קרום אפוי ופפרוני!
            drawRect(buf, cx - 10 * s, y - 12 * s, 20 * s, 4 * s, C16(210, 130, 50)); // קרום
            for (int r = 0; r < 18 * s; r++) {
                int halfW = (18 * s - r) / 2;
                drawRect(buf, cx - halfW, y - 8 * s + r, halfW * 2, 1, C16(245, 200, 20));
            }
            // פפרוני
            drawCircle(buf, cx - 3 * s, y - 4 * s, 2 * s, red);
            drawCircle(buf, cx + 3 * s, y - 2 * s, 2 * s, red);
            drawCircle(buf, cx,         y + 3 * s, 2 * s, red);
            // עיניים
            drawRect(buf, cx - 4 * s, y - 6 * s, 2 * s, 2 * s, black);
            drawRect(buf, cx + 2 * s, y - 6 * s, 2 * s, 2 * s, black);
            break;
        }
        case SHAPE_SUSHI: {
            // רול סושי / ניגירי: אורז לבן, פס אצת נורי שחורה באמצע, וחתיכת סלמון כתומה למעלה!
            drawRect(buf, cx - 9 * s, y - 6 * s, 18 * s, 12 * s, white); // אורז
            drawRect(buf, cx - 10 * s, y - 10 * s, 20 * s, 5 * s, C16(255, 105, 65)); // סלמון
            drawRect(buf, cx - 4 * s, y - 10 * s, 8 * s, 16 * s, C16(20, 45, 25)); // אצת נורי
            drawRect(buf, cx - 6 * s, y - 3 * s, 2 * s, 2 * s, black);
            drawRect(buf, cx + 5 * s, y - 3 * s, 2 * s, 2 * s, black);
            break;
        }
        case SHAPE_DONUT: {
            // דונאט עגול עם ציפוי ורוד, חור באמצע וסוכריות צבעוניות!
            drawCircle(buf, cx, y - 2 * s, 10 * s, C16(225, 160, 80));
            drawCircle(buf, cx, y - 3 * s, 8 * s, ch->mainColor);
            drawCircle(buf, cx, y - 2 * s, 3 * s, C16(135, 206, 235)); // החור בדונאט
            drawRect(buf, cx - 5 * s, y - 7 * s, 2 * s, 1 * s, gold);
            drawRect(buf, cx + 4 * s, y - 6 * s, 2 * s, 1 * s, white);
            drawRect(buf, cx - 4 * s, y + 2 * s, 2 * s, 1 * s, C16(0, 255, 255));
            break;
        }
        case SHAPE_BANANA: {
            // בננה צהובה קמורה עם קצה חום ועיניים שמחות!
            drawRect(buf, cx - 4 * s, y - 13 * s, 6 * s, 3 * s, C16(110, 75, 45));
            drawRect(buf, cx - 5 * s, y - 10 * s, 10 * s, 16 * s, C16(255, 235, 40));
            drawRect(buf, cx - 3 * s, y - 5 * s, 2 * s, 2 * s, black);
            drawRect(buf, cx + 2 * s, y - 5 * s, 2 * s, 2 * s, black);
            break;
        }
        case SHAPE_MINECRAFT: {
            // סטיב / קריפר / זומבי / אנדרמן - דמות קובייתית מדויקת!
            drawRect(buf, cx - 6 * s, y - 3 * s, 12 * s, 10 * s, ch->mainColor); // גוף מרובע
            drawRect(buf, cx - 7 * s, y - 15 * s, 14 * s, 12 * s, (charIdx == 17) ? skin : ch->mainColor); // ראש מרובע
            if (charIdx == 17) { // סטיב: שיער חום ומכנסיים כחולים
                drawRect(buf, cx - 7 * s, y - 15 * s, 14 * s, 4 * s, C16(95, 60, 35));
                drawRect(buf, cx - 6 * s, y + 3 * s, 12 * s, 4 * s, C16(40, 60, 160));
            }
            if (charIdx == 16) { // קריפר: פרצוף הקריפר המפורסם!
                drawRect(buf, cx - 5 * s, y - 12 * s, 3 * s, 3 * s, black);
                drawRect(buf, cx + 2 * s, y - 12 * s, 3 * s, 3 * s, black);
                drawRect(buf, cx - 2 * s, y - 9 * s,  4 * s, 4 * s, black);
                drawRect(buf, cx - 4 * s, y - 7 * s,  2 * s, 3 * s, black);
                drawRect(buf, cx + 2 * s, y - 7 * s,  2 * s, 3 * s, black);
            } else {
                u16 eyeCol = (charIdx == 41) ? C16(220, 60, 255) : black; // עיניים סגולות לאנדרמן
                drawRect(buf, cx - 4 * s, y - 10 * s, 2 * s, 2 * s, eyeCol);
                drawRect(buf, cx + 2 * s, y - 10 * s, 2 * s, 2 * s, eyeCol);
            }
            break;
        }
        case SHAPE_ANIMAL_EARS: {
            // חיות (חתול, כלב, שועל, פנדה, אריה, נמר, ארנב, חד-קרן וכו')
            drawRect(buf, cx - 7 * s, y - 3 * s, 14 * s, 10 * s, ch->mainColor);
            drawCircle(buf, cx, y - 8 * s, 7 * s, ch->mainColor);
            // אוזניים (ארנב מקבל אוזניים ארוכות במיוחד!)
            int earH = (charIdx == 53) ? 9 * s : 5 * s;
            drawRect(buf, cx - 6 * s, y - 14 * s - earH/2, 3 * s, earH, ch->subColor);
            drawRect(buf, cx + 3 * s, y - 14 * s - earH/2, 3 * s, earH, ch->subColor);
            // קרן זהב לחד-קרן!
            if (charIdx == 60) {
                drawLine(buf, cx, y - 14 * s, cx + 2 * s, y - 22 * s, gold);
                drawLine(buf, cx - 1, y - 14 * s, cx + 1 * s, y - 22 * s, gold);
            }
            // חוטם ועיניים
            drawCircle(buf, cx, y - 6 * s, 3 * s, white);
            drawRect(buf, cx - 4 * s, y - 10 * s, 2 * s, 2 * s, black);
            drawRect(buf, cx + 2 * s, y - 10 * s, 2 * s, 2 * s, black);
            drawRect(buf, cx - 1 * s, y - 7 * s, 2 * s, 2 * s, black);
            break;
        }
        case SHAPE_ONE_EYE: {
            // קיקלופ / קראקן - מפלצת עם עין אחת גדולה במרכז!
            drawCircle(buf, cx, y - 3 * s, 10 * s, ch->mainColor);
            drawCircle(buf, cx, y - 5 * s, 4 * s, white);
            drawCircle(buf, cx, y - 5 * s, 2 * s, black);
            break;
        }
        case SHAPE_SLIME: {
            // סליים / אלמנטל / דלעת / איש שלג
            int squash = (!grounded) ? 2 * s : 0;
            drawCircle(buf, cx, y - 2 * s, (9 - squash/2) * s, ch->mainColor);
            if (charIdx == 25) { // איש שלג: כדור שני לראש ואף גזר!
                drawCircle(buf, cx, y - 10 * s, 6 * s, white);
                drawRect(buf, cx + 2 * s, y - 10 * s, 5 * s, 2 * s, C16(255, 140, 0));
            }
            drawRect(buf, cx - 4 * s, y - 6 * s, 2 * s, 2 * s, black);
            drawRect(buf, cx + 2 * s, y - 6 * s, 2 * s, 2 * s, black);
            break;
        }
        case SHAPE_ROBOT: {
            // רובוט / אביר / אסטרונאוט
            drawRect(buf, cx - 7 * s, y - 3 * s, 14 * s, 10 * s, ch->mainColor);
            drawRect(buf, cx - 7 * s, y - 14 * s, 14 * s, 10 * s, ch->mainColor);
            // משקף קסדה זוהר
            drawRect(buf, cx - 5 * s, y - 11 * s, 10 * s, 4 * s, ch->subColor);
            // אנטנה לרובוט
            drawLine(buf, cx, y - 14 * s, cx, y - 18 * s, ch->subColor);
            break;
        }
        default: {
            // דמויות אנושיות ולוחמים עם כובעים, כתרים, קסדות ומסכות
            drawRect(buf, cx - 6 * s, y - 3 * s, 12 * s, 10 * s, ch->mainColor);
            drawCircle(buf, cx, y - 9 * s, 6 * s, (ch->shape == SHAPE_MONSTER) ? ch->mainColor : skin);

            // אביזרי ראש ייחודיים
            if (ch->specialAcc == 1) { // כתר מלכותי מוזהב
                drawRect(buf, cx - 6 * s, y - 17 * s, 12 * s, 3 * s, gold);
                drawRect(buf, cx - 6 * s, y - 19 * s, 2 * s, 2 * s, gold);
                drawRect(buf, cx - 1 * s, y - 19 * s, 2 * s, 2 * s, gold);
                drawRect(buf, cx + 4 * s, y - 19 * s, 2 * s, 2 * s, gold);
            } else if (ch->specialAcc == 2) { // מצנפת קוסם / שף / ליצן גבוהה
                drawRect(buf, cx - 8 * s, y - 15 * s, 16 * s, 2 * s, ch->subColor);
                drawRect(buf, cx - 5 * s, y - 20 * s, 10 * s, 5 * s, ch->subColor);
                drawRect(buf, cx - 2 * s, y - 24 * s, 4 * s, 4 * s, ch->subColor);
            } else if (ch->specialAcc == 3) { // קרניים (ויקינג, שטן, מינוטאור)
                drawLine(buf, cx - 5 * s, y - 14 * s, cx - 9 * s, y - 19 * s, white);
                drawLine(buf, cx + 5 * s, y - 14 * s, cx + 9 * s, y - 19 * s, white);
            } else if (ch->specialAcc == 4) { // הילת מלאך מרחפת
                drawRect(buf, cx - 5 * s, y - 19 * s, 10 * s, 2 * s, gold);
            } else if (ch->specialAcc == 6) { // סרט נינג'ה / משקפי האקר-מטריקס
                drawRect(buf, cx - 6 * s, y - 12 * s, 12 * s, 3 * s, ch->subColor);
            } else { // כובע רגיל
                drawRect(buf, cx - 6 * s, y - 16 * s, 12 * s, 4 * s, ch->subColor);
            }

            // עיניים
            drawRect(buf, cx - 3 * s, y - 10 * s, 2 * s, 2 * s, black);
            drawRect(buf, cx + 2 * s, y - 10 * s, 2 * s, 2 * s, black);
            break;
        }
    }

    // 4. ידיים מתנדנדות
    drawLine(buf, cx - 7 * s, y - 1 * s, cx - (9 + legOff/2) * s, y + 4 * s, black);
    drawLine(buf, cx + 7 * s, y - 1 * s, cx + (9 + legOff/2) * s, y + 4 * s, black);
}
