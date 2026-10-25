#include "game.h"

#undef C16
#define C16(r,g,b) (0x8000 | (((b)>>3)<<10) | (((g)>>3)<<5) | ((r)>>3))

// מאגר של 14 קטגוריות * 200 פריטים = 2,800 אלמנטי התאמה אישית שבהם אף אחד מ-200 הסגנונות לא זהה!
CustomPartSpec customPartsDB[CUSTOM_PARTS_COUNT][200];
static bool dbInitialized = false;

void initCustomPartsDatabase(void) {
    if (dbInitialized) return;

    for (int cat = 0; cat < CUSTOM_PARTS_COUNT; cat++) {
        for (int i = 0; i < 200; i++) {
            CustomPartSpec* sp = &customPartsDB[cat][i];
            // נוסחה חד-חד-ערכית שמבטיחה שלכל אינדקס (0..199) יש צירוף ייחודי לחלוטין של תתי-אלמנטים:
            // מכיוון ש-200 = 8 * 5 * 5, הצמד (baseShape, subPattern, extraOrnament) לבדו הוא ייחודי ב-100% לכל i!
            sp->baseShape     = (u8)(i % 8);                  // 8 צורות בסיס שונות לכל איבר
            sp->subPattern    = (u8)((i / 8) % 5);            // 5 דוגמאות פנימיות שונות
            sp->extraOrnament = (u8)((i / 40) % 5);           // 5 תוספות קישוט ועיטורים שונים
            sp->modWidth      = (s8)(((i * 3 + cat) % 5) - 2);// שינוי רוחב ייחודי (-2..+2)
            sp->modHeight     = (s8)(((i * 7 + cat) % 5) - 2);// שינוי גובה ייחודי (-2..+2)
            sp->offsetY       = (s8)(((i * 11 + cat) % 3) - 1);// היסט גובה עדין (-1..+1)
            sp->accentTone    = (u8)((i * 13 + cat * 5) & 63);// צבע משני משלים ייחודי
        }
    }
    dbInitialized = true;
}

const CustomPartSpec* getCustomPartSpec(int category, int styleIdx) {
    if (!dbInitialized) initCustomPartsDatabase();
    if (category < 0 || category >= CUSTOM_PARTS_COUNT) category = 0;
    int idx = styleIdx % 200;
    if (idx < 0) idx += 200;
    return &customPartsDB[category][idx];
}

// ציור מפורט של איבר ספציפי מתוך מאגר 200 האלמנטים הייחודיים של כל קטגוריה!
void renderStoredCustomPart(u16* buf, int cat, int styleIdx, u16 mainCol, int hx, int hy, int x, int y, int s, int legL, int legR, enum FaceExpr faceExpr) {
    const CustomPartSpec* sp = getCustomPartSpec(cat, styleIdx);
    u16 black = C16(18, 18, 24);
    u16 white = C16(252, 252, 255);
    u16 gold  = C16(255, 215, 0);
    u16 red   = C16(235, 40, 40);
    u16 accCol = getCustomPaletteColor(sp->accentTone);

    switch (cat) {
        case 13: { // 13: גלימה, כנפיים ואביזרי גב (200 וריאציות ייחודיות)
            if (styleIdx == 0) break;
            int w = (10 + sp->modWidth) * s;
            int h = (14 + sp->modHeight) * s;
            int oy = sp->offsetY * s;
            if (sp->baseShape == 0 || sp->baseShape == 1) { // גלימות באורכים וצורות שונות
                drawRect(buf, x - w, y - 5 * s + oy, w * 2, h, mainCol);
                if (sp->subPattern > 0) drawRect(buf, x - w + s, y - 5 * s + oy, w * 2 - 2 * s, 2 * s, accCol);
            } else if (sp->baseShape == 2 || sp->baseShape == 3) { // כנפיים פרוסות בגדלים שונים
                int wingR = (5 + sp->subPattern) * s;
                drawCircle(buf, x - 11 * s, y - 2 * s + oy, wingR, mainCol);
                drawCircle(buf, x + 11 * s, y - 2 * s + oy, wingR, mainCol);
                if (sp->extraOrnament > 1) {
                    drawCircle(buf, x - 11 * s, y - 2 * s + oy, wingR / 2, accCol);
                    drawCircle(buf, x + 11 * s, y - 2 * s + oy, wingR / 2, accCol);
                }
            } else if (sp->baseShape == 4 || sp->baseShape == 5) { // תרמילי סילון ומנועי גב
                drawRect(buf, x - 9 * s, y - 4 * s + oy, 18 * s, 9 * s, mainCol);
                drawCircle(buf, x - 8 * s, y + 6 * s + oy, (2 + (sp->subPattern % 2)) * s, gold);
                drawCircle(buf, x + 8 * s, y + 6 * s + oy, (2 + (sp->subPattern % 2)) * s, red);
            } else { // חרבות גב מוצלבות / הילת כוח אחורית
                drawLine(buf, x - 12 * s, y - 12 * s + oy, x + 12 * s, y + 8 * s + oy, mainCol);
                drawLine(buf, x + 12 * s, y - 12 * s + oy, x - 12 * s, y + 8 * s + oy, accCol);
            }
            if (sp->extraOrnament == 4) { // עיטור יהלום מרחף מאחור
                drawCircle(buf, x - 13 * s, y - 9 * s, 2 * s, gold);
                drawCircle(buf, x + 13 * s, y - 9 * s, 2 * s, gold);
            }
            break;
        }

        case 5: { // 5: רגליים ומכנסיים (200 וריאציות ייחודיות)
            int lw = (4 + (sp->baseShape % 3) + (sp->modWidth > 0 ? 1 : 0)) * s;
            int lh = (6 + (sp->modHeight > 0 ? 1 : 0)) * s;
            int lxL = x - 6 * s + (legL * s) / 2;
            int lxR = x + 1 * s + (legR * s) / 2;
            drawRect(buf, lxL - 1, y + 5 * s, lw + 2, lh + 2, black);
            drawRect(buf, lxL,     y + 5 * s, lw,     lh,     mainCol);
            drawRect(buf, lxR - 1, y + 5 * s, lw + 2, lh + 2, black);
            drawRect(buf, lxR,     y + 5 * s, lw,     lh,     mainCol);
            // דוגמאות מכנסיים ייחודיות (פסים, מגני ברכיים, כיסים, ניטים)
            if (sp->subPattern == 1) {
                drawRect(buf, lxL, y + (7 + sp->offsetY) * s, lw, 1 * s, accCol);
                drawRect(buf, lxR, y + (7 + sp->offsetY) * s, lw, 1 * s, accCol);
            } else if (sp->subPattern == 2) {
                drawCircle(buf, lxL + lw / 2, y + 8 * s, 1 * s, gold);
                drawCircle(buf, lxR + lw / 2, y + 8 * s, 1 * s, gold);
            } else if (sp->subPattern == 3) {
                drawRect(buf, lxL, y + 5 * s, lw, 2 * s, accCol);
                drawRect(buf, lxR, y + 5 * s, lw, 2 * s, accCol);
            } else if (sp->subPattern == 4) {
                drawRect(buf, lxL + s, y + 6 * s, 1 * s, lh - s, white);
                drawRect(buf, lxR + s, y + 6 * s, 1 * s, lh - s, white);
            }
            break;
        }

        case 6: { // 6: נעליים ומגפיים (200 וריאציות ייחודיות)
            int sw = (5 + (sp->baseShape % 4)) * s;
            int sh = (3 + (sp->subPattern % 3)) * s;
            int sxL = x - 7 * s + (legL * s) / 2;
            int sxR = x + 1 * s + (legR * s) / 2;
            drawRect(buf, sxL, y + 11 * s, sw, sh, black);
            drawRect(buf, sxL + s, y + 11 * s + 1, sw - 2 * s, sh - 2, mainCol);
            drawRect(buf, sxR, y + 11 * s, sw, sh, black);
            drawRect(buf, sxR + s, y + 11 * s + 1, sw - 2 * s, sh - 2, mainCol);
            // סוליות, שרוכים, כנפיים או להבות סילון ייחודיות
            if (sp->extraOrnament == 1) {
                drawRect(buf, sxL, y + 11 * s + sh - s, sw, s, white);
                drawRect(buf, sxR, y + 11 * s + sh - s, sw, s, white);
            } else if (sp->extraOrnament == 2) {
                drawRect(buf, sxL + s, y + 15 * s, 3 * s, 2 * s, gold);
                drawRect(buf, sxR + s, y + 15 * s, 3 * s, 2 * s, red);
            } else if (sp->extraOrnament == 3) {
                drawCircle(buf, sxL, y + 12 * s, 2 * s, accCol);
                drawCircle(buf, sxR + sw, y + 12 * s, 2 * s, accCol);
            } else if (sp->extraOrnament == 4) {
                drawRect(buf, sxL + 2 * s, y + 11 * s, s, sh, accCol);
                drawRect(buf, sxR + 2 * s, y + 11 * s, s, sh, accCol);
            }
            break;
        }

        case 3: { // 3: גוף ושריון (200 וריאציות ייחודיות)
            int bw = (13 + (sp->baseShape % 4)) * s;
            int bh = (9 + (sp->modHeight > 0 ? 2 : 1)) * s;
            drawRect(buf, x - bw / 2 - 1, y - 4 * s, bw + 2, bh + 2, black);
            drawRect(buf, x - bw / 2,     y - 3 * s, bw,     bh,     mainCol);
            // 5 דוגמאות חזה * 8 צורות * 5 עיטורים = 200 שריונות וחולצות ייחודיים!
            if (sp->subPattern == 1) {
                drawRect(buf, x - bw / 2, y + 2 * s, bw, 2 * s, gold); // חגורת זהב
            } else if (sp->subPattern == 2) {
                drawCircle(buf, x, y, (2 + (sp->baseShape % 2)) * s, accCol); // ליבת כוח בחזה
            } else if (sp->subPattern == 3) {
                drawLine(buf, x - bw / 2 + s, y - 3 * s, x + bw / 2 - s, y + 4 * s, accCol); // רצועת קרב אלכסונית
            } else if (sp->subPattern == 4) {
                drawRect(buf, x - 2 * s, y - 3 * s, 4 * s, bh, accCol); // פס וסט/עניבה מרכזי
            }
            if (sp->extraOrnament >= 2) { // כתפיות שריון בגדלים וצבעים שונים
                int pr = (2 + (sp->extraOrnament % 2)) * s;
                drawCircle(buf, x - bw / 2, y - 3 * s, pr, (sp->extraOrnament == 4) ? gold : accCol);
                drawCircle(buf, x + bw / 2, y - 3 * s, pr, (sp->extraOrnament == 4) ? gold : accCol);
            }
            break;
        }

        case 4: { // 4: ידיים, כפפות וכלי נשק (200 וריאציות ייחודיות)
            int aw = (3 + (sp->baseShape % 2)) * s;
            int ah = (6 + (sp->subPattern % 3)) * s;
            drawRect(buf, x - 11 * s, y - 3 * s, aw + 2, ah + 2, black);
            drawRect(buf, x - 10 * s, y - 2 * s, aw,     ah,     mainCol);
            drawRect(buf, x + 7 * s,  y - 3 * s, aw + 2, ah + 2, black);
            drawRect(buf, x + 8 * s,  y - 2 * s, aw,     ah,     mainCol);
            drawCircle(buf, x - 9 * s, y + 4 * s, (2 + (sp->extraOrnament % 2)) * s, mainCol);
            drawCircle(buf, x + 9 * s, y + 4 * s, (2 + (sp->extraOrnament % 2)) * s, mainCol);
            // כלי נשק ואביזרי יד ייחודיים לפי baseShape ו-subPattern
            if (sp->baseShape == 1 || sp->baseShape == 5) { // חרב / קטאנה / חרב אור באורכים שונים
                int swLen = (8 + sp->subPattern * 2) * s;
                drawLine(buf, x + 9 * s, y + 3 * s, x + 13 * s, y + 3 * s - swLen, accCol);
            } else if (sp->baseShape == 2 || sp->baseShape == 6) { // מגן קרב ביד שמאל
                drawCircle(buf, x - 11 * s, y + 2 * s, (3 + (sp->subPattern % 3)) * s, accCol);
                drawCircle(buf, x - 11 * s, y + 2 * s, 1 * s, gold);
            } else if (sp->baseShape == 3 || sp->baseShape == 7) { // מטה קסמים / שרביט מלכותי
                drawLine(buf, x + 11 * s, y + 10 * s, x + 11 * s, y - (10 + sp->subPattern) * s, gold);
                drawCircle(buf, x + 11 * s, y - (11 + sp->subPattern) * s, (2 + (sp->extraOrnament % 2)) * s, accCol);
            } else if (sp->extraOrnament >= 3) { // צמידי כוח זוהרים על פרקי הידיים
                drawRect(buf, x - 11 * s, y + 2 * s, 4 * s, 2 * s, gold);
                drawRect(buf, x + 7 * s,  y + 2 * s, 4 * s, 2 * s, gold);
            }
            break;
        }

        case 0: { // 0: פנים וראש (200 וריאציות ייחודיות + ראש איש עוגייה ב-199!)
            if (styleIdx == 199) {
                drawCircle(buf, hx, hy, 9 * s, C16(90, 45, 15));
                drawCircle(buf, hx, hy, 8 * s, C16(195, 125, 60));
                drawCircle(buf, hx - 5 * s, hy - 5 * s, 2 * s, white);
                drawCircle(buf, hx,         hy - 6 * s, 2 * s, C16(255, 105, 180));
                drawCircle(buf, hx + 5 * s, hy - 5 * s, 2 * s, white);
                break;
            }
            int hr = (7 + (sp->baseShape % 2)) * s;
            drawCircle(buf, hx, hy, hr + s, black);
            drawCircle(buf, hx, hy, hr, mainCol);
            // אוזניים ייחודיות (אלף, חיה, רובוט, חייזר)
            if (sp->subPattern == 1) {
                drawCircle(buf, hx - hr - s, hy + sp->offsetY * s, 2 * s, mainCol);
                drawCircle(buf, hx + hr + s, hy + sp->offsetY * s, 2 * s, mainCol);
            } else if (sp->subPattern == 2) {
                drawRect(buf, hx - hr - 2 * s, hy - 2 * s, 2 * s, 4 * s, accCol);
                drawRect(buf, hx + hr,         hy - 2 * s, 2 * s, 4 * s, accCol);
            } else if (sp->subPattern == 3) {
                drawCircle(buf, hx - 5 * s, hy - hr, 3 * s, mainCol);
                drawCircle(buf, hx + 5 * s, hy - hr, 3 * s, mainCol);
            }
            // סימני פנים ייחודיים (צלקת, איפור שבטי, לחיים, נמשים)
            if (sp->extraOrnament == 1) {
                drawLine(buf, hx - 5 * s, hy - 3 * s, hx - 3 * s, hy + 2 * s, red);
            } else if (sp->extraOrnament == 2) {
                drawRect(buf, hx - 6 * s, hy + s, 2 * s, 2 * s, C16(255, 130, 150));
                drawRect(buf, hx + 4 * s, hy + s, 2 * s, 2 * s, C16(255, 130, 150));
            } else if (sp->extraOrnament == 3) {
                drawRect(buf, hx - 1 * s, hy - 5 * s, 2 * s, 2 * s, gold);
            } else if (sp->extraOrnament == 4) {
                drawLine(buf, hx - 6 * s, hy, hx - 4 * s, hy, accCol);
                drawLine(buf, hx + 4 * s, hy, hx + 6 * s, hy, accCol);
            }
            break;
        }

        case 2: { // 2: עיניים ומשקפיים (200 וריאציות ייחודיות)
            if (faceExpr == EXPR_SCARED) {
                drawRect(buf, hx - 5 * s, hy - 2 * s, 4 * s, 4 * s, white);
                drawRect(buf, hx + 2 * s, hy - 2 * s, 4 * s, 4 * s, white);
                drawRect(buf, hx - 4 * s, hy - 1 * s, 2 * s, 2 * s, black);
                drawRect(buf, hx + 3 * s, hy - 1 * s, 2 * s, 2 * s, black);
                break;
            }
            if (faceExpr == EXPR_HURT) {
                drawLine(buf, hx - 5 * s, hy - 2 * s, hx - 2 * s, hy + 1 * s, black);
                drawLine(buf, hx - 2 * s, hy - 2 * s, hx - 5 * s, hy + 1 * s, black);
                drawLine(buf, hx + 2 * s, hy - 2 * s, hx + 5 * s, hy + 1 * s, black);
                drawLine(buf, hx + 5 * s, hy - 2 * s, hx + 2 * s, hy + 1 * s, black);
                break;
            }
            int ey = hy - 2 * s + sp->offsetY * s;
            int ew = (3 + (sp->modWidth > 0 ? 1 : 0)) * s;
            int eh = (2 + (sp->baseShape % 3)) * s;
            if (sp->baseShape == 1) { // משקפי סייבר / שמש במגוון מסגרות
                drawRect(buf, hx - 6 * s, ey, 12 * s, (2 + (sp->subPattern % 3)) * s, mainCol);
                drawRect(buf, hx - 6 * s, ey - s, 12 * s, s, (sp->extraOrnament > 2) ? gold : black);
            } else if (sp->baseShape == 2) { // עין קיקלופ/רובוט מרכזית
                drawCircle(buf, hx, ey + s, (2 + (sp->subPattern % 2)) * s, white);
                drawCircle(buf, hx, ey + s, 1 * s, mainCol);
            } else if (sp->baseShape == 3) { // עיניים קורצות / עצומות בשמחה
                drawLine(buf, hx - 5 * s, ey + s, hx - 3 * s, ey, black);
                drawLine(buf, hx - 3 * s, ey,     hx - 1 * s, ey + s, black);
                drawLine(buf, hx + 1 * s, ey + s, hx + 3 * s, ey, black);
                drawLine(buf, hx + 3 * s, ey,     hx + 5 * s, ey + s, black);
            } else { // עיניים מפורטות בגדלים, אישונים וניצוצות שונים
                drawRect(buf, hx - 5 * s, ey, ew, eh, white);
                drawRect(buf, hx + 2 * s, ey, ew, eh, white);
                drawRect(buf, hx - 4 * s, ey + s / 2, 2 * s, 2 * s, mainCol);
                drawRect(buf, hx + 3 * s, ey + s / 2, 2 * s, 2 * s, mainCol);
                if (sp->extraOrnament > 0) {
                    drawRect(buf, hx - 5 * s, ey, s, s, (sp->extraOrnament == 4) ? gold : white);
                    drawRect(buf, hx + 2 * s, ey, s, s, (sp->extraOrnament == 4) ? gold : white);
                }
            }
            break;
        }

        case 8: { // 8: גבות + קטגוריה 9 (קמירות גבות) - 200 גבות שונות לחלוטין!
            int curve = customChar.partStyle[9] % 7;
            int byL_out = hy - (4 - sp->offsetY) * s;
            int byL_in  = hy - (4 - sp->offsetY) * s;
            int byR_in  = hy - (4 - sp->offsetY) * s;
            int byR_out = hy - (4 - sp->offsetY) * s;

            if (curve == 1)      { byL_out += s; byR_out += s; }
            else if (curve == 2) { byL_in  += s; byR_in  += s; byL_out -= s; byR_out -= s; }
            else if (curve == 3) { byL_out += s; byR_out += s; byL_in  -= s; byR_in  -= s; }
            else if (curve == 4) { byL_out -= s; byL_in  -= s; byR_in  += s; }
            else if (curve == 5) { byR_out -= s; byR_in  -= s; byL_in  += s; }
            else if (curve == 6) { byL_in  -= s; byR_in  -= s; }

            int bLen = (3 + (sp->baseShape % 3)) * s;
            if (sp->subPattern == 4) { // חד-גבה מחוברת עם קמירות
                drawLine(buf, hx - 2 * s - bLen, byL_out, hx, byL_in, mainCol);
                drawLine(buf, hx, byR_in, hx + 2 * s + bLen, byR_out, mainCol);
            } else {
                drawLine(buf, hx - 2 * s - bLen, byL_out, hx - 2 * s, byL_in, mainCol);
                drawLine(buf, hx + 2 * s, byR_in, hx + 2 * s + bLen, byR_out, mainCol);
                if (sp->baseShape >= 4) { // גבות עבות כפולות
                    drawLine(buf, hx - 2 * s - bLen, byL_out - 1, hx - 2 * s, byL_in - 1, mainCol);
                    drawLine(buf, hx + 2 * s, byR_in - 1, hx + 2 * s + bLen, byR_out - 1, mainCol);
                }
                if (sp->extraOrnament == 3) { // פירסינג / חריץ אופנתי בגבה!
                    drawRect(buf, hx - 5 * s, byL_out - 1, s, 2 * s, gold);
                }
            }
            break;
        }

        case 10: { // 10: אפים (200 וריאציות ייחודיות)
            int ny = hy + sp->offsetY * s;
            int nw = (1 + (sp->baseShape % 3)) * s;
            int nh = (1 + (sp->subPattern % 3)) * s;
            if (sp->baseShape == 0) {
                drawRect(buf, hx - nw / 2, ny, nw, nh, mainCol);
            } else if (sp->baseShape == 1) {
                drawCircle(buf, hx, ny + s, (1 + (sp->subPattern % 2)) * s, mainCol);
            } else if (sp->baseShape == 2) {
                drawLine(buf, hx, ny - s, hx + nw, ny + nh, mainCol);
            } else if (sp->baseShape == 3) {
                drawRect(buf, hx - 2 * s, ny, 4 * s, 2 * s, white);
                drawRect(buf, hx - s, ny, 2 * s, s, mainCol);
            } else {
                drawRect(buf, hx - nw / 2, ny, nw, nh, mainCol);
                if (sp->extraOrnament >= 2) drawRect(buf, hx - s, ny + nh, 2 * s, s, gold); // נזם זהב!
            }
            break;
        }

        case 11: { // 11: פיות, חיוכים ושיניים (200 וריאציות ייחודיות + קמירות מקטגוריה 9!)
            int my = hy + (4 + sp->offsetY) * s;
            int mw = (4 + (sp->baseShape % 4)) * s;
            int mCurve = (customChar.partStyle[9] / 7) % 4;

            if (faceExpr == EXPR_SCARED) {
                drawCircle(buf, hx, my, 2 * s, black);
                drawCircle(buf, hx, my, 1 * s, red);
                break;
            }
            if (sp->baseShape == 0 || sp->baseShape == 1) { // חיוכים קמורים / ישרים / הפוכים
                drawRect(buf, hx - mw / 2, my, mw, s, mainCol);
                if (mCurve == 0 || sp->subPattern == 1) {
                    drawRect(buf, hx - mw / 2 - s, my - s, s, s, mainCol);
                    drawRect(buf, hx + mw / 2,     my - s, s, s, mainCol);
                } else if (mCurve == 2 || sp->subPattern == 2) {
                    drawRect(buf, hx - mw / 2 - s, my + s, s, s, mainCol);
                    drawRect(buf, hx + mw / 2,     my + s, s, s, mainCol);
                }
            } else if (sp->baseShape == 2 || sp->baseShape == 3) { // פה פתוח עם שיניים ולשון
                drawRect(buf, hx - mw / 2, my - s, mw, 3 * s, black);
                drawRect(buf, hx - mw / 2 + s, my - s, mw - 2 * s, s, white);
                if (sp->extraOrnament > 1) drawRect(buf, hx - s, my + s, 2 * s, s, red);
            } else if (sp->baseShape == 4 || sp->baseShape == 5) { // פה עם ניבים בגדלים שונים
                drawRect(buf, hx - mw / 2, my, mw, s, mainCol);
                drawRect(buf, hx - mw / 2 + s, my + s, s, 2 * s, white);
                drawRect(buf, hx + mw / 2 - 2 * s, my + s, s, 2 * s, white);
            } else { // פה שורק / חתולי / לשון בחוץ
                drawCircle(buf, hx, my, (1 + (sp->subPattern % 2)) * s, mainCol);
                if (sp->extraOrnament >= 2) drawRect(buf, hx, my + s, 2 * s, 2 * s, C16(255, 100, 140));
            }
            break;
        }

        case 1: { // 1: זקן ושפם (200 וריאציות ייחודיות)
            if (styleIdx == 0) break;
            int bw = (6 + (sp->baseShape % 5)) * s;
            int bh = (2 + (sp->subPattern % 5)) * s;
            if (sp->baseShape == 0 || sp->baseShape == 4) { // שפמים מעוצבים
                drawRect(buf, hx - bw / 2, hy + 2 * s, bw, 2 * s, mainCol);
                if (sp->extraOrnament > 2) {
                    drawRect(buf, hx - bw / 2 - s, hy + s, s, s, mainCol);
                    drawRect(buf, hx + bw / 2,     hy + s, s, s, mainCol);
                }
            } else if (sp->baseShape == 1 || sp->baseShape == 5) { // זקן צרפתי / תיש
                drawRect(buf, hx - 3 * s, hy + 5 * s, 6 * s, bh, mainCol);
            } else if (sp->baseShape == 2 || sp->baseShape == 6) { // זקן קוסם מדורג ארוך
                drawCircle(buf, hx, hy + 6 * s, (4 + (sp->subPattern % 3)) * s, mainCol);
                drawCircle(buf, hx, hy + (9 + (sp->extraOrnament % 3)) * s, 3 * s, mainCol);
            } else { // זקן ויקינג עבות עם חרוזי זהב
                drawRect(buf, hx - bw / 2, hy + 3 * s, bw, bh + 2 * s, mainCol);
                if (sp->extraOrnament >= 2) {
                    drawCircle(buf, hx - 2 * s, hy + 6 * s, s, gold);
                    drawCircle(buf, hx + 2 * s, hy + 6 * s, s, gold);
                }
            }
            break;
        }

        case 12: { // 12: שיער ותסרוקות כשאין כובע (200 וריאציות ייחודיות!)
            if (styleIdx == 0) break;
            int hw = (14 + sp->modWidth * 2) * s;
            if (sp->baseShape == 0 || sp->baseShape == 1) { // קוצים בסגנון אנימה בגבהים שונים
                drawRect(buf, hx - hw / 2, hy - 9 * s, hw, 3 * s, mainCol);
                int spikeH = (2 + sp->subPattern) * s;
                drawCircle(buf, hx - 5 * s, hy - 9 * s - spikeH / 2, 2 * s, mainCol);
                drawCircle(buf, hx,         hy - 10 * s - spikeH / 2, 3 * s, mainCol);
                drawCircle(buf, hx + 5 * s, hy - 9 * s - spikeH / 2, 2 * s, mainCol);
            } else if (sp->baseShape == 2 || sp->baseShape == 3) { // מוהוק פאנק גבוה עם פס צבעוני
                int mh = (5 + sp->subPattern) * s;
                drawRect(buf, hx - 2 * s, hy - 8 * s - mh, 4 * s, mh + 2 * s, mainCol);
                if (sp->extraOrnament > 1) drawRect(buf, hx - s, hy - 9 * s - mh, 2 * s, 2 * s, accCol);
            } else if (sp->baseShape == 4 || sp->baseShape == 5) { // שיער גולש ארוך בצדדים + פוני
                int sideLen = (7 + sp->subPattern * 2) * s;
                drawRect(buf, hx - 8 * s, hy - 9 * s, 16 * s, 3 * s, mainCol);
                drawRect(buf, hx - 9 * s, hy - 7 * s, 3 * s, sideLen, mainCol);
                drawRect(buf, hx + 6 * s, hy - 7 * s, 3 * s, sideLen, mainCol);
            } else { // אפרו / תלתלים / קוקו סמוראי
                int ar = (5 + (sp->subPattern % 3)) * s;
                drawCircle(buf, hx, hy - 9 * s, ar, mainCol);
                drawCircle(buf, hx - 5 * s, hy - 7 * s, ar - s, mainCol);
                drawCircle(buf, hx + 5 * s, hy - 7 * s, ar - s, mainCol);
                if (sp->extraOrnament == 4) drawRect(buf, hx - 7 * s, hy - 7 * s, 14 * s, 2 * s, accCol); // סרט שיער!
            }
            break;
        }

        case 7: { // 7: כובעים, כתרים, קסדות והילות (200 וריאציות ייחודיות)
            if (styleIdx == 0) break;
            int cw = (14 + sp->modWidth * 2) * s;
            int ch = (4 + (sp->subPattern % 3)) * s;
            if (sp->baseShape == 0 || sp->baseShape == 1) { // כתרים מלכותיים משובצים
                drawRect(buf, hx - cw / 2, hy - 10 * s, cw, ch, mainCol);
                drawCircle(buf, hx - 5 * s, hy - 11 * s, 2 * s, gold);
                drawCircle(buf, hx,         hy - 12 * s, 2 * s, accCol);
                drawCircle(buf, hx + 5 * s, hy - 11 * s, 2 * s, gold);
            } else if (sp->baseShape == 2 || sp->baseShape == 3) { // מצנפות קוסמים וכובעי מכשפים
                drawRect(buf, hx - 10 * s, hy - 8 * s, 20 * s, 3 * s, mainCol);
                drawCircle(buf, hx, hy - 11 * s, (4 + (sp->subPattern % 2)) * s, mainCol);
                drawCircle(buf, hx, hy - (14 + (sp->extraOrnament % 3)) * s, 3 * s, accCol);
            } else if (sp->baseShape == 4 || sp->baseShape == 5) { // קסדות ויקינג / אביר / חלל עם קרניים או נוצה
                drawRect(buf, hx - cw / 2, hy - 10 * s, cw, ch + s, mainCol);
                drawCircle(buf, hx - 8 * s, hy - 11 * s, (2 + (sp->subPattern % 2)) * s, white);
                drawCircle(buf, hx + 8 * s, hy - 11 * s, (2 + (sp->subPattern % 2)) * s, white);
            } else { // כובעי מצחייה, הילות מלאכים וצילינדרים
                drawRect(buf, hx - cw / 2, hy - (10 + sp->extraOrnament) * s, cw, ch, mainCol);
                drawRect(buf, hx - 2 * s, hy - 7 * s, 11 * s, 2 * s, accCol);
            }
            break;
        }
    }
}
 
