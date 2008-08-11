
/***************************************************************************
    NWNXFuncs.cpp - Implementation of the CNWNXFuncs class.
    Copyright (C) 2007 Doug Swarin (zac@intertex.net)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ***************************************************************************/

#include "NWNXDefenses.h"

volatile uintptr_t Hook_CCONC_ReturnHit;
volatile uintptr_t Hook_CCONC_ReturnMiss;
static volatile CNWSCreature *Hook_CCONC_Attacker;
static volatile CNWSCreature *Hook_CCONC_Target;
static volatile int Hook_CCONC_Concealment;
static volatile int Hook_CCONC_MissChance;


static int Hook_GetConcealmentCheckResult (CNWSCreature *attacker, CNWSCreature *target, int concealment, int misschance) {
    double conc, lis = 0.0;

    if (attacker == NULL            ||
        target == NULL              ||
        attacker->cre_stats == NULL ||
        target->cre_stats == NULL)
        return 0;

    if (concealment < 1 && misschance < 1)
        return 0;


    if (target->obj.obj_type == OBJECT_TYPE_CREATURE) {
        int sc = 0;

        if (target->cre_is_pc && nwn_GetLevelByClass(target->cre_stats, CLASS_TYPE_FIGHTER) > 20) {
            int parry = CNWSCreatureStats__GetSkillRank(target->cre_stats, SKILL_PARRY, NULL, 0) / 4;

            if (random() % 100 < parry)
                return 100;
        }

        int rogue = nwn_GetLevelByClass(target->cre_stats, CLASS_TYPE_ROGUE);

        if (rogue >= 30) {
            if (nwn_GetKnowsFeat(target->cre_stats, FEAT_EPIC_SELF_CONCEALMENT_50))
                sc = 5;
            else if (nwn_GetKnowsFeat(target->cre_stats, FEAT_EPIC_SELF_CONCEALMENT_40))
                sc = 4;
            else if (nwn_GetKnowsFeat(target->cre_stats, FEAT_EPIC_SELF_CONCEALMENT_30))
                sc = 3;
            else if (nwn_GetKnowsFeat(target->cre_stats, FEAT_EPIC_SELF_CONCEALMENT_20))
                sc = 2;
            else if (nwn_GetKnowsFeat(target->cre_stats, FEAT_EPIC_SELF_CONCEALMENT_10))
                sc = 1;

            if (sc > 0) {
                int percent = 0;

                switch (sc) {
                    case 1:  percent =  5; break;
                    case 2:  percent =  9; break;
                    case 3:  percent = 12; break;
                    case 4:  percent = 14; break;
                    default: percent = 15; break;
                }

                if (percent > rogue - 20)
                    percent = rogue - 20;

                sc = (sc * 10) + ((CNWSCreatureStats__GetSkillRank(target->cre_stats, SKILL_HIDE, NULL, 0) * percent) / 100);
            }
        }

        switch (concealment) {
            case 16:
            case 21:
            case 26:
            case 31:
            case 36:
                /* camouflage / mass camouflage */
                concealment = (concealment - 1) + (CNWSCreatureStats__GetSkillRank(target->cre_stats, SKILL_HIDE, NULL, 0) / 4);
                break;

            case 56: {
                /* assassin improved invisibility */
                int hide = nwn_GetLevelByClass(target->cre_stats, CLASS_TYPE_ASSASSIN) +
                           CNWSCreatureStats__GetSkillRank(target->cre_stats, SKILL_HIDE, NULL, 0);

                if (hide > 127)
                    hide = 127;

                concealment = (concealment - 31) + (hide / 3);
            }
            break;
        }

        if (sc > concealment)
            concealment = sc;
    }


    /* select the greater of miss chance and concealment (they do not stack) */
    if (misschance > concealment)
        conc = misschance * 0.01;
    else
        conc = concealment * 0.01;


    if (attacker->cre_is_pc) {
        lis = CNWSCreatureStats__GetSkillRank(attacker->cre_stats, SKILL_LISTEN, NULL, 0);

        if (lis < 60.0 && CNWSCreatureStats__HasFeat(attacker->cre_stats, FEAT_BLIND_FIGHT))
            lis = 60.0;
    } else {
        if (CNWSCreatureStats__HasFeat(attacker->cre_stats, FEAT_BLIND_FIGHT))
            lis += 60.0;

        if (CNWSCreatureStats__HasFeat(attacker->cre_stats, FEAT_SKILL_FOCUS_LISTEN))
            lis += 15.0;

        if (CNWSCreatureStats__HasFeat(attacker->cre_stats, FEAT_EPIC_SKILL_FOCUS_LISTEN))
            lis += 30.0;
    }


    if (!target->cre_is_pc) {
        if (CNWSCreatureStats__HasFeat(attacker->cre_stats, FEAT_SKILL_FOCUS_MOVE_SILENTLY))
            lis -= 15.0;

        if (CNWSCreatureStats__HasFeat(attacker->cre_stats, FEAT_EPIC_SKILL_FOCUS_MOVESILENTLY))
            lis -= 30.0;
    }


    if (lis > 60.0)
        lis = 60.0 + ((lis - 60.0) / 2.0);
    else if (lis < 0.0)
        lis = 0.0;

    /* ensure there are no rounding errors by adding 0.00005 - this hits
     * specifically at 85% concealment */
    concealment = (pow(conc, 1.0 + (lis / 60.0)) + 0.00005) * 1000.0;


    if (random() % 1000 < concealment)
        return concealment / 10;

    return 0;
}


void Hook_CheckConcealment (void) {
    asm("leave");

    /* copy attacker, concealment, and miss chance out */
    asm("pushl 0x8(%ebp)");
    asm("popl Hook_CCONC_Attacker");

    asm("pushl 0xffffffac(%ebp)");
    asm("popl Hook_CCONC_Target");

    asm("pushl 0xffffffbc(%ebp)");
    asm("popl Hook_CCONC_Concealment");

    asm("pushl 0xffffffc0(%ebp)");
    asm("popl Hook_CCONC_MissChance");

    Hook_CCONC_Concealment = Hook_GetConcealmentCheckResult(
        (CNWSCreature *)Hook_CCONC_Attacker,
        (CNWSCreature *)Hook_CCONC_Target,
        Hook_CCONC_Concealment, Hook_CCONC_MissChance);

    if (Hook_CCONC_Concealment) {
        asm("pushl Hook_CCONC_Concealment");
        asm("popl 0xffffffbc(%ebp)");

        asm("pushl Hook_CCONC_ReturnMiss");
    } else
        asm("pushl Hook_CCONC_ReturnHit");

    asm("ret");
}


/* vim: set sw=4: */
