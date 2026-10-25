#include "game.h"
#include <stdlib.h>

// ============================================================================
// מנוע אודיו ומוזיקה פרוצדורלית רב-ערוצית לחומרת ה-Nintendo DS!
// משתמש בערוצי ה-PSG וה-Noise המובנים של ה-DS ללא צורך בקובצי סאונד חיצוניים!
// ============================================================================

static bool soundReady = false;
static int chLead = -1;
static int chArp  = -1;
static int chBass = -1;
static int chSfx  = -1;
static int chPerc = -1;

// משתני מיזוג הדרגתי של המוזיקה בין 27 הביומות!
static float smoothRootFreq = 261.6f; // תדר הבסיס שעובר בהדרגה בין עולמות
static float smoothTempoDiv = 9.0f;   // מהירות המוזיקה שמשתנה בהדרגה
static int musicStep = 0;
static int stepTick  = 0;

// טבלאות מרווחים מוזיקליים (ביחס של אלפיות תדר) לסולמות שונים לפי אופי הביומה:
static const int SCALE_HAPPY[8]   = {1000, 1122, 1260, 1335, 1498, 1682, 1888, 2000}; // מז'ור (יער, ממתקים, קשת, צעצועים)
static const int SCALE_DESERT[8]  = {1000, 1059, 1260, 1335, 1498, 1587, 1888, 2000}; // חיג'אז מזרחי (מדבר, סוואנה, עתיק)
static const int SCALE_DARK[8]    = {1000, 1122, 1189, 1335, 1498, 1587, 1782, 2000}; // מינור דרמטי (הר געש, גיהנום, קברות, מעמקים)
static const int SCALE_CYBER[8]   = {1000, 1189, 1335, 1498, 1782, 2000, 2378, 2670}; // פנטטוני חללי (חלל, סייבר, קריסטל, רטרו, מאדים)

// תבניות מלודיה פרוצדורליות שמתחלפות ומשתלבות
static const u8 MELODY_PATTERNS[4][16] = {
    {0, 2, 4, 7, 4, 2, 5, 4, 3, 1, 2, 4, 2, 0, 1, 0},
    {0, 4, 7, 5, 4, 2, 3, 1, 0, 2, 4, 6, 7, 5, 4, 2},
    {7, 6, 4, 2, 0, 2, 4, 5, 6, 4, 2, 1, 0, 4, 2, 0},
    {0, 0, 4, 4, 5, 5, 4, 2, 3, 3, 2, 2, 1, 2, 4, 0}
};

// אפקט קולי מונפש בזמן אמת (עבור קפיצת 100 הדמויות, איסוף מטבע ופגיעה באויב)
static int sfxTimer = 0;
static int sfxMode  = 0; // 1=קפיצת דמות, 2=מטבע/יהלום, 3=פגיעת קרקע, 4=פגיעה אווירית, 5=משימה הושלמה
static int sfxBaseFreq = 440;
static int sfxSlide    = 0;

void initProceduralAudio(void) {
    if (soundReady) return;
    soundEnable();
    soundReady = true;
}

// בחירת הסולם המוזיקלי לפי הביומה הנוכחית
static const int* getScaleForBiome(int bIdx) {
    switch (bIdx) {
        case 1: case 9: case 20: case 23:
            return SCALE_DESERT; // מדבר, סוואנה, חוף, עתיק
        case 4: case 10: case 13: case 17: case 22: case 26:
            return SCALE_DARK;   // הר געש, ביצה, גיהנום, קברות, רעל, מעמקים
        case 5: case 8: case 15: case 16: case 19: case 24:
            return SCALE_CYBER;  // חלל, סייבר, קריסטל, קיטור, רטרו, מאדים
        default:
            return SCALE_HAPPY;  // יער, ג'ונגל, ממתקים, קרחון, אוקיינוס, עננים, פריחה, קשת, צעצועים
    }
}

// עדכון המוזיקה הפרוצדורלית וצלילי האפקטים בכל פריים (60FPS)!
void updateProceduralAudio(void) {
    if (!soundReady) initProceduralAudio();

    // 1. עדכון אפקט קולי פעיל (צלילי הדמויות, פגיעה או מטבעות)
    if (sfxTimer > 0) {
        sfxTimer--;
        if (sfxMode == 1) { // צליל קפיצה/קול ייחודי לדמות!
            sfxBaseFreq += sfxSlide;
            if (sfxBaseFreq < 120) sfxBaseFreq = 120;
            if (sfxBaseFreq > 2400) sfxBaseFreq = 2400;
            if (chSfx >= 0) soundSetFreq(chSfx, sfxBaseFreq);
        } else if (sfxMode == 2) { // צליל פעמון למטבע / יהלום!
            if (sfxTimer == 5 && chSfx >= 0) soundSetFreq(chSfx, sfxBaseFreq * 3 / 2);
        } else if (sfxMode == 3 || sfxMode == 4) { // צליל פגיעה וריסוק איכותי!
            sfxBaseFreq -= (sfxMode == 3) ? 28 : 45;
            if (sfxBaseFreq < 65) sfxBaseFreq = 65;
            if (chSfx >= 0) soundSetFreq(chSfx, sfxBaseFreq);
        } else if (sfxMode == 5) { // תרועת ניצחון להשלמת משימה!
            if (sfxTimer == 16 && chSfx >= 0) soundSetFreq(chSfx, 659);
            else if (sfxTimer == 10 && chSfx >= 0) soundSetFreq(chSfx, 784);
            else if (sfxTimer == 5 && chSfx >= 0) soundSetFreq(chSfx, 1046);
        }
        if (sfxTimer == 0 && chSfx >= 0) {
            soundKill(chSfx);
            chSfx = -1;
        }
    }

    // בעצירה מוחלטת משתיקים את ערוצי המוזיקה
    if (gameState == STATE_PAUSED) {
        if (chLead >= 0) { soundKill(chLead); chLead = -1; }
        if (chArp >= 0)  { soundKill(chArp);  chArp  = -1; }
        if (chBass >= 0) { soundKill(chBass); chBass = -1; }
        if (chPerc >= 0) { soundKill(chPerc); chPerc = -1; }
        return;
    }

    // 2. חישוב מעבר הדרגתי וחלק של תדר הבסיס והקצב לפי הביומה הנוכחית!
    float targetRoot = 220.0f + (currentBiomeIdx % 12) * 18.5f;
    if (BIOMES[currentBiomeIdx].isNight) targetRoot *= 0.84f; // אווירה עמוקה יותר בביומות לילה!
    float targetTempo = (gameState == STATE_PLAYING) ? (10.5f - (currentSpeed - 3.2f) * 1.1f) : 11.0f;
    if (targetTempo < 5.0f) targetTempo = 5.0f;

    // אינטרפולציה חלקה (המוזיקה משתנה בהדרגה בין הביומות ולא בבום!)
    smoothRootFreq += (targetRoot - smoothRootFreq) * 0.015f;
    smoothTempoDiv += (targetTempo - smoothTempoDiv) * 0.02f;

    stepTick++;
    if (stepTick < (int)smoothTempoDiv) return;
    stepTick = 0;
    musicStep = (musicStep + 1) & 63;

    const int* scale = getScaleForBiome(currentBiomeIdx);
    int patIdx = (currentBiomeIdx + (musicStep / 16)) & 3;
    int deg = MELODY_PATTERNS[patIdx][musicStep & 15];
    int leadFreq = ((int)smoothRootFreq * scale[deg & 7]) / 1000;

    // החלפת גוון הצליל (Duty Cycle) לפי עולם המשחק
    DutyCycle leadDuty = (currentBiomeIdx % 3 == 0) ? DutyCycle_50 : ((currentBiomeIdx % 3 == 1) ? DutyCycle_25 : DutyCycle_12);

    // ערוץ 1: מנגינה ראשית (Lead Synth)
    if (chLead < 0) {
        chLead = soundPlayPSG(leadDuty, leadFreq, 42, 64);
    } else {
        soundSetFreq(chLead, leadFreq);
        soundSetVolume(chLead, (gameState == STATE_PLAYING) ? 44 : 28);
    }

    // ערוץ 2: ארפג'יו והרמוניה עשירה (Arpeggio Chord)
    int arpDeg = (deg + ((musicStep & 1) ? 2 : 4)) & 7;
    int arpFreq = ((int)smoothRootFreq * scale[arpDeg]) / 1000;
    if (chArp < 0) {
        chArp = soundPlayPSG(DutyCycle_25, arpFreq, 26, 32);
    } else {
        soundSetFreq(chArp, arpFreq);
        soundSetVolume(chArp, 26);
    }

    // ערוץ 3: בס קצבי עמוק (Bassline)
    int bassDeg = (musicStep & 4) ? 4 : 0;
    int bassFreq = (((int)smoothRootFreq / 2) * scale[bassDeg]) / 1000;
    if (chBass < 0) {
        chBass = soundPlayPSG(DutyCycle_50, bassFreq, 48, 96);
    } else {
        soundSetFreq(chBass, bassFreq);
        soundSetVolume(chBass, (musicStep & 1) ? 48 : 22);
    }

    // ערוץ 4: תופים ומקצב פרוצדורלי (Hi-Hat & Snare Noise) בזמן ריצה
    if (gameState == STATE_PLAYING) {
        if ((musicStep & 3) == 0 || (musicStep & 3) == 2) {
            int noiseFreq = ((musicStep & 3) == 0) ? 1800 : 6500;
            int noiseVol  = ((musicStep & 3) == 0) ? 34 : 20;
            if (chPerc < 0) chPerc = soundPlayNoise(noiseFreq, noiseVol, 64);
            else {
                soundSetFreq(chPerc, noiseFreq);
                soundSetVolume(chPerc, noiseVol);
            }
        } else if (chPerc >= 0) {
            soundSetVolume(chPerc, 0);
        }
    } else if (chPerc >= 0) {
        soundKill(chPerc);
        chPerc = -1;
    }
}

// ============================================================================
// צלילים ייחודיים לכל אחת מ-100 הדמויות במשחק!
// כל דמות מפיקה גוון קול, תדר התחלתי ושיפוע תדר (Pitch Envelope) משלה!
// ============================================================================
void playCharacterVoiceSfx(int charIdx) {
    if (!soundReady) initProceduralAudio();
    if (chSfx >= 0) soundKill(chSfx);

    int idx = (charIdx < 0 || charIdx >= TOTAL_SHOP_CHARS) ? 0 : charIdx;
    enum AnimStyle st = (idx == CUSTOM_CHAR_IDX) ? STYLE_NORMAL : CHARACTERS[idx].style;
    enum CharShape sh = (idx == CUSTOM_CHAR_IDX) ? SHAPE_HUMAN  : CHARACTERS[idx].shape;

    // חישוב תדר ייחודי לכל אחת מ-100 הדמויות כך שאף שתי דמויות לא נשמעות זהה!
    sfxBaseFreq = 320 + ((idx * 19) % 420);
    sfxSlide    = 28 + (idx % 15);
    DutyCycle duty = DutyCycle_50;

    if (sh == SHAPE_MONSTER || st == STYLE_HEAVY) {
        // דמויות כבדות ומפלצות (קינג קונג, גודזילה, אביר, דוב): קול בס עמוק ועוצמתי!
        sfxBaseFreq = 150 + ((idx * 11) % 140);
        sfxSlide = 14;
        duty = DutyCycle_12;
    } else if (sh == SHAPE_ROBOT || sh == SHAPE_MINECRAFT) {
        // רובוטים ודמויות מיינקראפט: צליל סינתטי מתכתי חד!
        sfxBaseFreq = 420 + ((idx * 29) % 350);
        sfxSlide = 42;
        duty = DutyCycle_25;
    } else if (sh == SHAPE_ANIMAL_EARS || sh == SHAPE_WINGED) {
        // חיות ומעופפים: קפיצה קלילה וגבוהה!
        sfxBaseFreq = 480 + ((idx * 23) % 380);
        sfxSlide = 36;
        duty = DutyCycle_50;
    } else if (idx == CUSTOM_CHAR_IDX) {
        // הדמות האישית: הצליל שלה נקבע לפי האיברים שבחרת לה!
        sfxBaseFreq = 360 + (customChar.partStyle[0] * 3) + (customChar.partStyle[11] * 2);
        sfxSlide = 34;
    }

    sfxMode  = 1;
    sfxTimer = 9;
    chSfx = soundPlayPSG(duty, sfxBaseFreq, 92, 64);
}

// צליל איסוף מטבע או יהלום מנצנץ
void playCoinSfx(bool isDiamond) {
    if (!soundReady) initProceduralAudio();
    if (chSfx >= 0) soundKill(chSfx);

    sfxBaseFreq = isDiamond ? 1174 : 880;
    sfxMode  = 2;
    sfxTimer = 9;
    chSfx = soundPlayPSG(DutyCycle_50, sfxBaseFreq, 95, 64);
}

// צלילי פגיעה איכותיים רב-שכבתיים (שונים לפגיעה באבן/קרקע לעומת פגיעה באויב מעופף!)
void playHitSfx(bool isAirHit) {
    if (!soundReady) initProceduralAudio();
    if (chSfx >= 0) soundKill(chSfx);

    if (isAirHit) {
        // התנגשות אווירית: שריקת הדף גבוהה שצונחת במהירות
        sfxBaseFreq = 720;
        sfxMode  = 4;
        sfxTimer = 18;
        chSfx = soundPlayPSG(DutyCycle_12, sfxBaseFreq, 115, 64);
    } else {
        // התנגשות באויב קרקע / היתקלות באבן: רעש ריסוק כבד + תדר הדף נמוך!
        sfxBaseFreq = 380;
        sfxMode  = 3;
        sfxTimer = 20;
        chSfx = soundPlayNoise(950, 120, 64);
    }
}

// צליל חגיגי להשלמת משימה ופתיחת פריט הרכבה חדש!
void playMissionCompleteSfx(void) {
    if (!soundReady) initProceduralAudio();
    if (chSfx >= 0) soundKill(chSfx);

    sfxBaseFreq = 523;
    sfxMode  = 5;
    sfxTimer = 22;
    chSfx = soundPlayPSG(DutyCycle_50, sfxBaseFreq, 105, 64);
}
 
