#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// מחולל משימות פרוצדורלי שמייצר אינסוף משימות מגוונות בכל סיבוב ומעניק חלקי הרכבה מיוחדים!
enum ProcMissionType {
    PMISS_REACH_BIOME = 0, // הגעה לביומה ספציפית בסיבוב אחד
    PMISS_COLLECT_COINS,   // איסוף כמות מטבעות בסיבוב אחד
    PMISS_JUMP_COUNT,      // ביצוע מספר קפיצות בסיבוב אחד
    PMISS_REACH_SCORE,     // הגעה לניקוד יעד בסיבוב אחד
    PMISS_FIND_DIAMONDS,   // איסוף יהלומים כחולים בסיבוב אחד
    PMISS_BEAT_HIGHSCORE   // שבירת שיא או התקרבות אליו
};

static enum ProcMissionType activeMType = PMISS_REACH_BIOME;
static int missionTargetVal = 3;
static int missionProgressVal = 0;
static int rewardCat = 0;
static int rewardStyle = 199;
static char missionDescBuf[84] = "משימה: הגע לממתקים לראש עוגיה!";

static const char* CAT_SHORT_HEB[CUSTOM_PARTS_COUNT] = {
    "פנים", "זקן", "עיניים", "שריון", "נשק", "מכנסיים", "נעליים",
    "כובע", "גבות", "הבעה", "אף", "פה", "שיער", "גלימה"
};

void generateProceduralMission(void) {
    currentMissionDone = false;
    missionProgressVal = 0;

    // בסיבובים הראשונים ניתן עדיפות גבוהה למשימת ביומת הממתקים (ראש איש עוגייה!) ולמשימות הביומות הקלאסיות
    int roll = rand() % 100;
    if (roll < 25 && !(unlockedCustomRewards & 1)) {
        activeMType = PMISS_REACH_BIOME;
        missionTargetVal = 3; // ביומת הממתקים!
        rewardCat = 0;
        rewardStyle = 199;    // ראש איש עוגייה!
        strcpy(missionDescBuf, "משימה: הגע לממתקים לראש עוגיה!");
        return;
    }

    activeMType = (enum ProcMissionType)(rand() % 6);
    rewardCat   = rand() % CUSTOM_PARTS_COUNT;
    rewardStyle = 10 + (rand() % 189); // אחד מ-200 הפריטים הייחודיים במאגר!

    switch (activeMType) {
        case PMISS_REACH_BIOME: {
            int maxB = (bestBiomeIdx + 2 < NUM_BIOMES) ? (bestBiomeIdx + 2) : (NUM_BIOMES - 1);
            if (maxB < 1) maxB = 1;
            missionTargetVal = 1 + (rand() % maxB);
            if (missionTargetVal == 3) {
                rewardCat = 0; rewardStyle = 199;
                strcpy(missionDescBuf, "משימה: הגע לממתקים לראש עוגיה!");
            } else {
                snprintf(missionDescBuf, sizeof(missionDescBuf), "משימה: הגע ל%s לפרס %s!",
                         BIOMES[missionTargetVal].nameHeb, CAT_SHORT_HEB[rewardCat]);
            }
            break;
        }
        case PMISS_COLLECT_COINS: {
            missionTargetVal = 40 + (rand() % 8) * 15; // 40..145 מטבעות בריצה אחת
            snprintf(missionDescBuf, sizeof(missionDescBuf), "משימה: אסוף %d מטבעות ל%s!",
                     missionTargetVal, CAT_SHORT_HEB[rewardCat]);
            break;
        }
        case PMISS_JUMP_COUNT: {
            missionTargetVal = 8 + (rand() % 18); // 8..25 קפיצות בריצה אחת
            snprintf(missionDescBuf, sizeof(missionDescBuf), "משימה: קפוץ %d פעמים לפרס %s!",
                     missionTargetVal, CAT_SHORT_HEB[rewardCat]);
            break;
        }
        case PMISS_REACH_SCORE: {
            missionTargetVal = 250 + (rand() % 12) * 100; // 250..1350 נקודות
            snprintf(missionDescBuf, sizeof(missionDescBuf), "משימה: השג %d נק׳ לפרס %s!",
                     missionTargetVal, CAT_SHORT_HEB[rewardCat]);
            break;
        }
        case PMISS_FIND_DIAMONDS: {
            missionTargetVal = 1 + (rand() % 3); // 1..3 יהלומים בסיבוב אחד
            snprintf(missionDescBuf, sizeof(missionDescBuf), "משימה: תפוס %d יהלומים ל%s!",
                     missionTargetVal, CAT_SHORT_HEB[rewardCat]);
            break;
        }
        case PMISS_BEAT_HIGHSCORE:
        default: {
            missionTargetVal = (highScore > 100) ? highScore : 200;
            snprintf(missionDescBuf, sizeof(missionDescBuf), "משימה: שבור שיא לפרס %s נדיר!",
                     CAT_SHORT_HEB[rewardCat]);
            break;
        }
    }
}

const char* getProceduralMissionText(void) {
    return missionDescBuf;
}

static void awardProceduralMissionPrize(void) {
    currentMissionDone = true;
    unlockedCustomRewards |= (1 << (currentMissionId % 30));

    // מצייד אוטומטית את הפריט החדש שנפתח בדמות האישית ושומר לנצח!
    customChar.partStyle[rewardCat] = (u8)rewardStyle;
    customChar.partColor[rewardCat] = (u8)((rewardStyle * 7) & 63);

    if (rewardCat == 0 && rewardStyle == 199) {
        bubbleText = "נפתח ראש איש עוגיה!";
    } else {
        bubbleText = "משימה הושלמה! פרס חדש נפתח!";
    }
    bubbleTimer = 165;
    playMissionCompleteSfx();
    savePermanentData();
}

void notifyProceduralMissionEvent(int dispScore, int biomeIdx, int coinsAdded, int jumpsAdded, bool brokeRecord) {
    if (currentMissionDone) return;

    if (activeMType == PMISS_REACH_BIOME) {
        if (biomeIdx >= missionTargetVal) awardProceduralMissionPrize();
    } else if (activeMType == PMISS_COLLECT_COINS) {
        missionProgressVal += coinsAdded;
        if (missionProgressVal >= missionTargetVal) awardProceduralMissionPrize();
    } else if (activeMType == PMISS_JUMP_COUNT) {
        missionProgressVal += jumpsAdded;
        if (missionProgressVal >= missionTargetVal) awardProceduralMissionPrize();
    } else if (activeMType == PMISS_REACH_SCORE) {
        if (dispScore >= missionTargetVal) awardProceduralMissionPrize();
    } else if (activeMType == PMISS_FIND_DIAMONDS) {
        if (coinsAdded == 20) {
            missionProgressVal++;
            if (missionProgressVal >= missionTargetVal) awardProceduralMissionPrize();
        }
    } else if (activeMType == PMISS_BEAT_HIGHSCORE) {
        if (brokeRecord && dispScore >= 120) awardProceduralMissionPrize();
    }
}
 
