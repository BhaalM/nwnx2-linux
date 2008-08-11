
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

volatile uintptr_t ExaltHook_PPDC_Return;
static volatile int ExaltHook_PPDC_ExtraDC = 0;
static volatile CNWSCreature *ExaltHook_PPDC_Thief, *ExaltHook_PPDC_Victim;


static int Hook_GetPickPocketDCAdjustment (CNWSCreature *thief, CNWSCreature *victim) {
    if (victim == NULL             ||
        victim->cre_stats == NULL  ||
        victim->obj.obj_type != OBJECT_TYPE_CREATURE)
        return;

    int spot = CNWSCreatureStats__GetSkillRank(victim->cre_stats, SKILL_SPOT, NULL, 0);

    if (spot > 0)
        return spot;

    return 0;
}


void ExaltHook_PickPocketDC (void) {
    asm("leave");

    /* duplicate the work originally done */
    asm("cmp $0x2, %al");
    asm("sete %dl");
    asm("dec %edx");
    asm("and $0xfffffff6, %edx");
    asm("add $0x1e, %edx");

    /* get the thief and victim */
    asm("pushl 0xffffff74(%ebp)");
    asm("popl ExaltHook_PPDC_Victim");

    asm("pushl 0x8(%ebp)");
    asm("popl ExaltHook_PPDC_Thief");

    ExaltHook_PPDC_ExtraDC = ExaltReplace_CalculatePPDC(
        (CNWSCreature *)ExaltHook_PPDC_Thief,
        (CNWSCreature *)ExaltHook_PPDC_Victim);

    /* the result of Hook_GetPickPocketDCAdjustment() is in %eax */
    asm("add %eax, %edx");

    /* return to the normal pick pocket check */
    asm("push ExaltHook_PPDC_Return");
    asm("ret");
}


/* vim: set sw=4: */
