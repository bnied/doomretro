/*
========================================================================

                               DOOM RETRO
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright (C) 2013-2015 Brad Harding.

  DOOM RETRO is a fork of CHOCOLATE DOOM by Simon Howard.
  For a complete list of credits, see the accompanying AUTHORS file.

  This file is part of DOOM RETRO.

  DOOM RETRO is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM RETRO is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM RETRO. If not, see <http://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM RETRO is in no way affiliated with nor endorsed by
  id Software LLC.

========================================================================
*/

#include "doomstat.h"
#include "g_game.h"
#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"

//
// Animating textures and planes
// There is another anim_t used in wi_stuff, unrelated.
//
typedef struct
{
    boolean     istexture;
    int         picnum;
    int         basepic;
    int         numpics;
    int         speed;
} anim_t;

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

//
// source animation definition
//
typedef struct
{
    int         istexture;              // if false, it is a flat
    char        endname[9];
    char        startname[9];
    int         speed;
    boolean     isliquid;
} animdef_t;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#define MAXANIMS        32

static anim_t   *lastanim;
static anim_t   *anims;         // new structure w/o limits -- killough
static size_t   maxanims;

//
// P_InitPicAnims
//

// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.
//
animdef_t animdefs[] =
{
    { false, "NUKAGE3",  "NUKAGE1",  7, true  },
    { false, "FWATER4",  "FWATER1",  7, true  },
    { false, "SWATER4",  "SWATER1",  7, true  },
    { false, "LAVA4",    "LAVA1",    7, true  },
    { false, "BLOOD3",   "BLOOD1",   7, true  },

    // DOOM II flat animations.
    { false, "RROCK08",  "RROCK05",  7, false },
    { false, "SLIME04",  "SLIME01",  7, true  },
    { false, "SLIME08",  "SLIME05",  7, true  },
    { false, "SLIME12",  "SLIME09",  7, false },

    { true,  "BLODGR4",  "BLODGR1",  7, false },
    { true,  "SLADRIP3", "SLADRIP1", 7, false },

    { true,  "BLODRIP4", "BLODRIP1", 7, false },
    { true,  "FIREWALL", "FIREWALA", 7, false },
    { true,  "GSTFONT3", "GSTFONT1", 7, false },
    { true,  "FIRELAVA", "FIRELAV3", 7, false },
    { true,  "FIREMAG3", "FIREMAG1", 7, false },
    { true,  "FIREBLU2", "FIREBLU1", 7, false },
    { true,  "ROCKRED3", "ROCKRED1", 7, false },

    { true,  "BFALL4",   "BFALL1",   7, false },
    { true,  "SFALL4",   "SFALL1",   7, false },
    { true,  "WFALL4",   "WFALL1",   7, false },
    { true,  "DBRAIN4",  "DBRAIN1",  7, false },

    { -1,    "",         "",         0, false }
};

//
// Animating line specials
//
extern int      numflats;
extern boolean  canmodify;

boolean         *isliquid;

void P_InitPicAnims(void)
{
    int i;
    int size = (numflats + 1) * sizeof(boolean);

    isliquid = Z_Malloc(size, PU_STATIC, 0);
    memset(isliquid, false, size);

    //  Init animation
    lastanim = anims;
    for (i = 0; animdefs[i].istexture != -1; i++)
    {
        char    *startname = animdefs[i].startname;
        char    *endname = animdefs[i].endname;

        // 1/11/98 killough -- removed limit by array-doubling
        if (lastanim >= anims + maxanims)
        {
            size_t      newmax = (maxanims ? maxanims * 2 : MAXANIMS);

            anims = realloc(anims, newmax * sizeof(*anims));
            lastanim = anims + maxanims;
            maxanims = newmax;
        }

        if (animdefs[i].istexture)
        {
            // different episode?
            if (R_CheckTextureNumForName(startname) == -1)
                continue;

            lastanim->picnum = R_TextureNumForName(endname);
            lastanim->basepic = R_TextureNumForName(startname);

            lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;
        }
        else
        {
            int j;

            if (W_CheckNumForName(startname) == -1)
                continue;

            lastanim->picnum = R_FlatNumForName(endname);
            lastanim->basepic = R_FlatNumForName(startname);

            lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

            if (animdefs[i].isliquid)
                for (j = 0; j < lastanim->numpics; j++)
                    isliquid[lastanim->basepic + j] = true;
        }

        lastanim->istexture = animdefs[i].istexture;
        

        lastanim->speed = animdefs[i].speed;
        lastanim++;
    }
}

//
// UTILITIES
//

//
// getSide()
// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.
//
side_t *getSide(int currentSector, int line, int side)
{
    return &sides[(sectors[currentSector].lines[line])->sidenum[side]];
}

//
// getSector()
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
sector_t *getSector(int currentSector, int line, int side)
{
    return sides[(sectors[currentSector].lines[line])->sidenum[side]].sector;
}

//
// twoSided()
// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.
//
int twoSided(int sector, int line)
{
    // jff 1/26/98 return what is actually needed, whether the line
    // has two sidedefs, rather than whether the 2S flag is set
    return (sectors[sector].lines[line]->sidenum[1] != NO_INDEX);
}

//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
sector_t *getNextSector(line_t *line, sector_t *sec)
{
    // jff 1/26/98 check unneeded since line->backsector already
    // returns NULL if the line is not two sided, and does so from
    // the actual two-sidedness of the line, rather than its 2S flag
    //if (!(line->flags & ML_TWOSIDED))
    //    return NULL;
    return (line->frontsector == sec ? (line->backsector != sec ? line->backsector : NULL) :
        line->frontsector);
}

//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t P_FindLowestFloorSurrounding(sector_t *sec)
{
    int         i;
    sector_t    *other;
    fixed_t     floor = sec->floorheight;

    for (i = 0; i < sec->linecount; i++)
        if ((other = getNextSector(sec->lines[i], sec)) && other->floorheight < floor)
            floor = other->floorheight;
    return floor;
}

//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
    int         i;
    sector_t    *other;
    fixed_t     floor = -32000 << FRACBITS;

    for (i = 0; i < sec->linecount; i++)
        if ((other = getNextSector(sec->lines[i], sec)) && other->floorheight > floor)
            floor = other->floorheight;
    return floor;
}

//
// P_FindNextHighestFloor
// FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS
//
fixed_t P_FindNextHighestFloor(sector_t *sec, int currentheight)
{
    int         i;
    sector_t    *other;

    for (i = 0; i < sec->linecount; i++)
        if ((other = getNextSector(sec->lines[i], sec)) && other->floorheight > currentheight)
        {
            int height = other->floorheight;

            while (++i < sec->linecount)
                if ((other = getNextSector(sec->lines[i], sec)) && other->floorheight < height
                    && other->floorheight > currentheight)
                    height = other->floorheight;
            return height;
        }
    return currentheight;
}

//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindLowestCeilingSurrounding(sector_t *sec)
{
    int         i;
    sector_t    *other;
    fixed_t     height = 32000 << FRACBITS;

    for (i = 0; i < sec->linecount; i++)
        if ((other = getNextSector(sec->lines[i], sec)) && other->ceilingheight < height)
            height = other->ceilingheight;
    return height;
}

//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindHighestCeilingSurrounding(sector_t *sec)
{
    int         i;
    sector_t    *other;
    fixed_t     height = -32000 << FRACBITS;

    for (i = 0; i < sec->linecount; i++)
        if ((other = getNextSector(sec->lines[i], sec)) && other->ceilingheight > height)
            height = other->ceilingheight;
    return height;
}

//
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//

// Find the next sector with the same tag as a linedef.
// Rewritten by Lee Killough to use chained hashing to improve speed
int P_FindSectorFromLineTag(const line_t *line, int start)
{
    start = (start >= 0 ? sectors[start].nexttag :
        sectors[(unsigned int)line->tag % (unsigned int)numsectors].firsttag);
    while (start >= 0 && sectors[start].tag != line->tag)
        start = sectors[start].nexttag;
    return start;
}

// killough 4/16/98: Same thing, only for linedefs
int P_FindLineFromLineTag(const line_t *line, int start)
{
    start = (start >= 0 ? lines[start].nexttag :
        lines[(unsigned int)line->tag % (unsigned int)numlines].firsttag);
    while (start >= 0 && lines[start].tag != line->tag)
        start = lines[start].nexttag;
    return start;
}

// Hash the sector tags across the sectors and linedefs.
static void P_InitTagLists(void)
{
    int i;

    for (i = numsectors; --i >= 0;)     // Initially make all slots empty.
        sectors[i].firsttag = -1;
    for (i = numsectors; --i >= 0;)     // Proceed from last to first sector
    {                                   // so that lower sectors appear first
        int     j = (unsigned int)sectors[i].tag % (unsigned int)numsectors;    // Hash func

        sectors[i].nexttag = sectors[j].firsttag;     // Prepend sector to chain
        sectors[j].firsttag = i;
    }

    // killough 4/17/98: same thing, only for linedefs
    for (i = numlines; --i >= 0;)       // Initially make all slots empty.
        lines[i].firsttag = -1;
    for (i = numlines; --i >= 0;)       // Proceed from last to first linedef
    {                                   // so that lower linedefs appear first
        int     j = (unsigned int)lines[i].tag % (unsigned int)numlines;        // Hash func

        lines[i].nexttag = lines[j].firsttag;   // Prepend linedef to chain
        lines[j].firsttag = i;
    }
}

//
// Find minimum light from an adjacent sector
//
int P_FindMinSurroundingLight(sector_t *sector, int min)
{
    int         i;
    sector_t    *check;

    for (i = 0; i < sector->linecount; i++)
        if ((check = getNextSector(sector->lines[i], sector)) && check->lightlevel < min)
            min = check->lightlevel;
    return min;
}

//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//

//
// P_CheckTag()
//
// Passed a line, returns true if the tag is non-zero or the line special
// allows no tag without harm.
//
// Note: Only line specials activated by walkover, pushing, or shooting are
//       checked by this routine.
//
// jff 2/27/98 Added to check for zero tag allowed for regular special types
//
boolean P_CheckTag(line_t *line)
{
    // tag not zero, allowed
    if (line->tag)
        return true;

    switch (line->special)
    {
        case DR_OpenDoorWait4SecondsClose:
        case S1_ExitLevel:
        case W1_LightsToMaximumNeighbouringLevel:
        case W1_LightsTo255:
        case W1_StartLightsBlinkingEverySecond:
        case DR_OpenDoorWait4SecondsCloseBlueKeyRequired:
        case DR_OpenDoorWait4SecondsCloseYellowKeyRequired:
        case DR_OpenDoorWait4SecondsCloseRedKeyRequired:
        case D1_OpenDoorStayOpen:
        case D1_OpenDoorStayOpenBlueKeyRequired:
        case D1_OpenDoorStayOpenRedKeyRequired:
        case D1_OpenDoorStayOpenYellowKeyRequired:
        case W1_LightsTo0:
        case W1_TeleportToTaggedSectorContainingTeleportLanding:
        case MovingWallTextureToLeft:
        case S1_ExitLevelAndGoToSecretLevel:
        case W1_ExitLevel:
        case WR_LightsTo0:
        case WR_LightsToMaximumNeighbouringLevel:
        case WR_LightsTo255:
        case WR_RaiseCeilingToHighestNeighbouringCeiling:
        case WR_TeleportToTaggedSectorContainingTeleportLanding:
        case W1_LightsToMinimumNeighbouringLevel:
        case DR_OpenFastDoorWait4SecondsClose:
        case D1_OpenFastDoorStayOpen:
        case W1_ExitLevelAndGoToSecretLevel:
        case M1_TeleportToTaggedSectorContainingTeleportLanding:
        case MR_TeleportToTaggedSectorContainingTeleportLanding:
        case SR_LightsTo255:
        case SR_LightsTo0:
            return 1;   // zero tag allowed

        default:
            break;
    }
    return false;       // zero tag not allowed
}

//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
void P_CrossSpecialLine(line_t *line, int side, mobj_t *thing)
{
    int ok;

    // Triggers that other things can activate
    if (!thing->player)
    {
        // Things that should NOT trigger specials...
        switch (thing->type)
        {
            case MT_ROCKET:
            case MT_PLASMA:
            case MT_BFG:
            case MT_TROOPSHOT:
            case MT_HEADSHOT:
            case MT_BRUISERSHOT:
            case MT_TRACER:
            case MT_FATSHOT:
            case MT_SPAWNSHOT:
            case MT_ARACHPLAZ:
                return;
                break;

            default:
                break;
        }

        ok = 0;
        switch (line->special)
        {
            case W1_TeleportToTaggedSectorContainingTeleportLanding:
            case WR_TeleportToTaggedSectorContainingTeleportLanding:
            case M1_TeleportToTaggedSectorContainingTeleportLanding:
            case MR_TeleportToTaggedSectorContainingTeleportLanding:
            case W1_OpenDoorWait4SecondsClose:
            case W1_LowerLiftWait3SecondsRise:
            case WR_LowerLiftWait3SecondsRise:
                ok = 1;
                break;
        }
        if (!ok)
            return;
    }

    if (!P_CheckTag(line))      // jff 2/27/98 disallow zero tag on some types
        return;

    switch (line->special)
    {
        // TRIGGERS.
        // All from here to RETRIGGERS.
        case W1_OpenDoorStayOpen:
            if (EV_DoDoor(line, open))
            {
                line->special = 0;

                if (nomonsters && (line->flags & ML_TRIGGER666))
                {
                    line_t      junk;

                    switch (gameepisode)
                    {
                        case 1:
                            junk.tag = 666;
                            EV_DoFloor(&junk, lowerFloorToLowest);
                            break;

                        case 4:
                            junk.tag = 666;
                            EV_DoDoor(&junk, blazeOpen);
                            break;
                    }
                    line->flags &= ~ML_TRIGGER666;
                }
            }
            break;

        case W1_CloseDoor:
            if (EV_DoDoor(line, close))
                line->special = 0;
            break;

        case W1_OpenDoorWait4SecondsClose:
            if (EV_DoDoor(line, normal))
                line->special = 0;
            break;

        case W1_SetFloorToLowestNeighbouringCeiling:
            if (gamemission == doom && gameepisode == 4 && gamemap == 3 && canmodify)
            {
                if (EV_DoFloor(line, raiseFloorCrush))
                    line->special = 0;
            }
            else if (EV_DoFloor(line, raiseFloor))
                line->special = 0;
            break;

        case W1_StartFastCrusher:
            if (EV_DoCeiling(line, fastCrushAndRaise))
                line->special = 0;
            break;

        case W1_RaiseStairsHeight8Units:
            if (EV_BuildStairs(line, build8))
                line->special = 0;
            break;

        case W1_LowerLiftWait3SecondsRise:
            if (EV_DoPlat(line, downWaitUpStay, 0))
                line->special = 0;
            break;

        case W1_LightsToMaximumNeighbouringLevel:
            if (EV_LightTurnOn(line, 0))
                line->special = 0;
            break;

        case W1_LightsTo255:
            if (EV_LightTurnOn(line, 255))
                line->special = 0;
            break;

        case W1_CloseDoorWait30SecondsOpen:
            if (EV_DoDoor(line, close30ThenOpen))
                line->special = 0;
            break;

        case W1_StartLightsBlinkingEverySecond:
            if (EV_StartLightStrobing(line))
                line->special = 0;
            break;

        case W1_SetFloorToHighestNeighbouringFloor:
            if (EV_DoFloor(line, lowerFloor))
                line->special = 0;
            break;

        case W1_RaiseFloorToNextFloorChangeFloorTextureAndType:
            if (EV_DoPlat(line, raiseToNearestAndChange, 0))
                line->special = 0;
            break;

        case W1_StartSlowCrusher:
            if (EV_DoCeiling(line, crushAndRaise))
                line->special = 0;
            break;

        case W1_RaiseFloorBy64Units:
            if (EV_DoFloor(line, raiseToTexture))
                line->special = 0;
            break;

        case W1_LightsTo0:
            if (EV_LightTurnOn(line, 35))
                line->special = 0;
            break;

        case W1_SetFloorTo8UnitsAboveHighestNeighbouringFloor:
            if (EV_DoFloor(line, turboLower))
                line->special = 0;
            break;

        case W1_SetFloorToLowestNeighbouringFloorChangeFloorTextureAndType:
            if (EV_DoFloor(line, lowerAndChange))
                line->special = 0;
            break;

        case W1_SetFloorToLowestNeighbouringFloor:
            if (EV_DoFloor(line, lowerFloorToLowest))
                line->special = 0;
            break;

        case W1_TeleportToTaggedSectorContainingTeleportLanding:
            if (EV_Teleport(line, side, thing))
                line->special = 0;
            break;

        case W1_RaiseCeilingToHighestNeighbouringCeiling:
            if (EV_DoCeiling(line, raiseToHighest))
                line->special = 0;
            break;

        case W1_LowerCeilingTo8UnitsAboveFloor:
            if (EV_DoCeiling(line, lowerAndCrush))
                line->special = 0;
            break;

        case W1_ExitLevel:
            if (!(thing->player && thing->player->health <= 0))
                G_ExitLevel();
            break;

        case W1_StartUpDownMovingFloor:
            if (EV_DoPlat(line, perpetualRaise, 0))
                line->special = 0;
            break;

        case W1_StopUpDownMovingFloor:
            if (EV_StopPlat(line))
                line->special = 0;
            break;

        case W1_CrushFloorTo8UnitsUnderCeiling:
            if (EV_DoFloor(line, raiseFloorCrush))
                line->special = 0;
            break;

        case W1_StopCrusher:
            if (EV_CeilingCrushStop(line))
                line->special = 0;
            break;

        case W1_RaiseFloorBy24Units:
            if (EV_DoFloor(line, raiseFloor24))
                line->special = 0;
            break;

        case W1_RaiseFloorBy24UnitsChangeFloorTextureAndType:
            if (EV_DoFloor(line, raiseFloor24AndChange))
                line->special = 0;
            break;

        case W1_RaiseFastStairsHeight16Units:
            if (EV_BuildStairs(line, turbo16))
                line->special = 0;
            break;

        case W1_LightsToMinimumNeighbouringLevel:
            if (EV_TurnTagLightsOff(line))
                line->special = 0;
            break;

        case W1_OpenFastDoorWait4SecondsClose:
            if (EV_DoDoor (line, blazeRaise))
                line->special = 0;
            break;

        case W1_OpenFastDoorStayOpen:
            if (EV_DoDoor (line, blazeOpen))
                line->special = 0;
            break;

        case W1_CloseFastDoor:
            if (EV_DoDoor (line, blazeClose))
                line->special = 0;
            break;

        case W1_RaiseFloorToNextFloor:
            if (EV_DoFloor(line, raiseFloorToNearest))
                line->special = 0;
            break;

        case W1_LowerFastLiftWait3SecondsRise:
            if (EV_DoPlat(line, blazeDWUS, 0))
                line->special = 0;
            break;

        case W1_ExitLevelAndGoToSecretLevel:
            if (!(thing->player && thing->player->health <= 0))
                G_SecretExitLevel();
            break;

        case M1_TeleportToTaggedSectorContainingTeleportLanding:
            if (!thing->player && EV_Teleport(line, side, thing))
                line->special = 0;
            break;

        case W1_RaiseFastFloorToNextFloor:
            if (EV_DoFloor(line, raiseFloorTurbo))
                line->special = 0;
            break;

        case W1_StartSlowQuietCrusher:
            if (EV_DoCeiling(line, silentCrushAndRaise))
                line->special = 0;
            break;

        // RETRIGGERS.  All from here till end.
        case WR_LowerCeilingTo8UnitsAboveFloor:
            EV_DoCeiling(line, lowerAndCrush);
            break;

        case WR_StartSlowCrusher:
            EV_DoCeiling(line, crushAndRaise);
            break;

        case WR_StopCrusher:
            EV_CeilingCrushStop(line);
            break;

        case WR_CloseDoor:
            EV_DoDoor(line, close);
            break;

        case WR_CloseDoorWait30SecondsOpen:
            EV_DoDoor(line, close30ThenOpen);
            break;

        case WR_StartFastCrusher:
            EV_DoCeiling(line, fastCrushAndRaise);
            break;

        case WR_LightsTo0:
            EV_LightTurnOn(line, 35);
            break;

        case WR_LightsToMaximumNeighbouringLevel:
            EV_LightTurnOn(line, 0);
            break;

        case WR_LightsTo255:
            EV_LightTurnOn(line, 255);
            break;

        case WR_SetFloorToLowestNeighbouringFloor:
            EV_DoFloor(line, lowerFloorToLowest);
            break;

        case WR_SetFloorToHighestNeighbouringFloor:
            EV_DoFloor(line, lowerFloor);
            break;

        case WR_SetFloorToLowestNeighbouringFloorChangeFloorTextureAndType:
            EV_DoFloor(line, lowerAndChange);
            break;

        case WR_OpenDoorStayOpen:
            EV_DoDoor(line, open);
            break;

        case WR_StartUpDownMovingFloor:
            EV_DoPlat(line, perpetualRaise, 0);
            break;

        case WR_LowerLiftWait3SecondsRise:
            EV_DoPlat(line, downWaitUpStay, 0);
            break;

        case WR_StopUpDownMovingFloor:
            EV_StopPlat(line);
            break;

        case WR_OpenDoorWait4SecondsClose:
            EV_DoDoor(line, normal);
            break;

        case WR_SetFloorToLowestNeighbouringCeiling:
            EV_DoFloor(line, raiseFloor);
            break;

        case WR_RaiseFloorBy24Units:
            EV_DoFloor(line, raiseFloor24);
            break;

        case WR_RaiseFloorBy24UnitsChangeFloorTextureAndType:
            EV_DoFloor(line, raiseFloor24AndChange);
            break;

        case WR_CrushFloorTo8UnderCeiling:
            EV_DoFloor(line, raiseFloorCrush);
            break;

        case WR_RaiseFloorToNextFloorChangeFloorTextureAndType:
            EV_DoPlat(line, raiseToNearestAndChange, 0);
            break;

        case WR_RaiseFloorBy64Units:
            EV_DoFloor(line, raiseToTexture);
            break;

        case WR_TeleportToTaggedSectorContainingTeleportLanding:
            EV_Teleport(line, side, thing);
            break;

        case WR_SetFloorTo8UnitsAboveHighestNeighbouringFloor:
            EV_DoFloor(line, turboLower);
            break;

        case WR_OpenFastDoorWait4SecondsClose:
            EV_DoDoor(line, blazeRaise);
            break;

        case WR_OpenFastDoorStayOpen:
            EV_DoDoor(line, blazeOpen);
            break;

        case WR_CloseFastDoor:
            EV_DoDoor(line, blazeClose);
            break;

        case WR_LowerFastLiftWait3SecondsRise:
            EV_DoPlat(line, blazeDWUS, 0);
            break;

        case MR_TeleportToTaggedSectorContainingTeleportLanding:
            if (!thing->player)
                EV_Teleport(line, side, thing);
            break;

        case WR_RaiseFloorToNextFloor:
            EV_DoFloor(line, raiseFloorToNearest);
            break;

        case WR_RaiseFastFloorToNextFloor:
            EV_DoFloor(line, raiseFloorTurbo);
            break;
    }
}

//
// P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
void P_ShootSpecialLine(mobj_t *thing, line_t *line)
{
    // Impacts that other things can activate.
    if (!thing->player)
    {
        int     ok = 0;

        switch (line->special)
        {
            case G1_OpenDoorStayOpen:
                ok = 1;
                break;
        }
        if (!ok)
            return;
    }

    if (!P_CheckTag(line))      // jff 2/27/98 disallow zero tag on some types
        return;

    switch (line->special)
    {
        case G1_SetFloorToLowestNeighbouringCeiling:
            if (EV_DoFloor(line, raiseFloor))
                P_ChangeSwitchTexture(line, 0);
            break;

        case G1_OpenDoorStayOpen:
            EV_DoDoor(line, open);
            P_ChangeSwitchTexture(line, 1);
            if (canmodify && gamemission == doom2 && gamemap == 18)
                line->special = -G1_OpenDoorStayOpen;
            break;

        case G1_RaiseFloorToNextFloorChangeFloorTextureAndType:
            if (EV_DoPlat(line, raiseToNearestAndChange, 0))
                P_ChangeSwitchTexture(line, 0);
            break;
    }
}

//
// P_PlayerInSpecialSector
// Called every tic frame
//  that the player origin is in a special sector
//
void P_PlayerInSpecialSector(player_t *player)
{
    int      i;
    sector_t *sector = player->mo->subsector->sector;

    // Falling, not all the way down yet?
    if (player->mo->z != sector->floorheight)
        return;

    // Has hit ground.
    switch (sector->special)
    {
        case HellslimeDamage:
            if (!player->powers[pw_ironfeet])
                if (!(leveltime & 0x1f))
                    P_DamageMobj(player->mo, NULL, NULL, 10);
            break;

        case NukageDamage:
            if (!player->powers[pw_ironfeet])
                if (!(leveltime & 0x1f))
                      P_DamageMobj(player->mo, NULL, NULL, 5);
            break;

        case StrobeHurt:
        case SuperHellslimeDamage:
            if (!player->powers[pw_ironfeet] || P_Random() < 5)
                if (!(leveltime & 0x1f))
                    P_DamageMobj(player->mo, NULL, NULL, 20);
            break;

        case SecretSector:
            player->secretcount++;
            sector->special = 0;

            for (i = 0; i < sector->linecount; i++)
                sector->lines[i]->flags &= ~ML_SECRET;
            break;

        case ExitSuperDamage:
            // for E1M8 finale
            player->cheats &= ~CF_GODMODE;
            player->powers[pw_invulnerability] = 0;

            if (!(leveltime & 0x1f))
                P_DamageMobj(player->mo, NULL, NULL, 20);

            if (player->health <= 10)
            {
                player->health = 0;
                G_ExitLevel();
            }
            break;

        default:
            break;
    }
}

//
// P_UpdateSpecials
// Animate planes, scroll walls, etc.
//
#define MAXLINEANIMS    16384

short   numlinespecials;
line_t  *linespeciallist[MAXLINEANIMS];
boolean levelTimer;
int     levelTimeCount;

void P_UpdateSpecials(void)
{
    anim_t      *anim;
    int         pic;
    int         i;
    line_t      *line;

    // LEVEL TIMER
    if (levelTimer && --levelTimeCount)
        G_ExitLevel();

    // ANIMATE FLATS AND TEXTURES GLOBALLY
    for (anim = anims; anim < lastanim; anim++)
    {
        for (i = anim->basepic; i < anim->basepic + anim->numpics; i++)
        {
            pic = anim->basepic + ((leveltime / anim->speed + i) % anim->numpics);
            if (anim->istexture)
                texturetranslation[i] = pic;
            else
                flattranslation[i] = pic;
        }
    }

    // ANIMATE LINE SPECIALS
    for (i = 0; i < numlinespecials; i++)
    {
        line = linespeciallist[i];
        switch (line->special)
        {
            case MovingWallTextureToLeft:
                sides[line->sidenum[0]].textureoffset += FRACUNIT;
                break;
        }
    }

    // DO BUTTONS
    for (i = 0; i < MAXBUTTONS; i++)
        if (buttonlist[i].btimer)
        {
            buttonlist[i].btimer--;
            if (!buttonlist[i].btimer)
            {
                switch (buttonlist[i].where)
                {
                    case top:
                        sides[buttonlist[i].line->sidenum[0]].toptexture =
                            buttonlist[i].btexture;
                        break;

                    case middle:
                        sides[buttonlist[i].line->sidenum[0]].midtexture =
                            buttonlist[i].btexture;
                        break;

                    case bottom:
                        sides[buttonlist[i].line->sidenum[0]].bottomtexture =
                            buttonlist[i].btexture;
                        break;
                }
                if (buttonlist[i].line->special != -G1_OpenDoorStayOpen)
                    S_StartSound(buttonlist[i].soundorg, sfx_swtchn);
                memset(&buttonlist[i], 0, sizeof(button_t));
            }
        }
}

//
// Special Stuff that can not be categorized
//
int EV_DoDonut(line_t *line)
{
    sector_t    *s1;
    sector_t    *s2;
    sector_t    *s3;
    int         secnum = -1;
    int         rtn = 0;
    int         i;
    floormove_t *floor;
    fixed_t     s3_floorheight;
    short       s3_floorpic;

    while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
    {
        s1 = &sectors[secnum];

        // ALREADY MOVING?  IF SO, KEEP GOING...
        if (s1->specialdata)
            continue;

        rtn = 1;
        s2 = getNextSector(s1->lines[0], s1);

        if (s2 == NULL)
            continue;

        for (i = 0; i < s2->linecount; i++)
        {
            s3 = s2->lines[i]->backsector;

            if (s3 == s1)
                continue;

            if (s3 == NULL)
                continue;
            else
            {
                s3_floorheight = s3->floorheight;
                s3_floorpic = s3->floorpic;
            }

            // Spawn rising slime
            floor = (floormove_t *)Z_Malloc(sizeof(*floor), PU_LEVSPEC, 0);
            P_AddThinker(&floor->thinker);
            s2->specialdata = floor;
            floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
            floor->type = donutRaise;
            floor->crush = false;
            floor->direction = 1;
            floor->sector = s2;
            floor->speed = FLOORSPEED / 2;
            floor->texture = s3_floorpic;
            floor->newspecial = 0;
            floor->floordestheight = s3_floorheight;
            floor->stopsound = (floor->sector->floorheight != floor->floordestheight);

            // Spawn lowering donut-hole
            floor = (floormove_t *)Z_Malloc(sizeof(*floor), PU_LEVSPEC, 0);
            P_AddThinker(&floor->thinker);
            s1->specialdata = floor;
            floor->thinker.function.acp1 = (actionf_p1)T_MoveFloor;
            floor->type = lowerFloor;
            floor->crush = false;
            floor->direction = -1;
            floor->sector = s1;
            floor->speed = FLOORSPEED / 2;
            floor->floordestheight = s3_floorheight;
            floor->stopsound = (floor->sector->floorheight != floor->floordestheight);
            break;
        }
    }
    return rtn;
}

//
// SPECIAL SPAWNING
//

//
// P_SpawnSpecials
// After the map has been loaded, scan for specials
//  that spawn thinkers
//

// Parses command line parameters.
void P_SpawnSpecials(void)
{
    sector_t    *sector;
    int         i;

    // See if -TIMER was specified.

    if (timelimit > 0 && deathmatch)
    {
        levelTimer = true;
        levelTimeCount = timelimit * 60 * TICRATE;
    }
    else
        levelTimer = false;

    // Init special SECTORs.
    sector = sectors;
    for (i = 0; i < numsectors; i++, sector++)
    {
        if (!sector->special)
            continue;

        switch (sector->special)
        {
            case FlickeringLights:
                P_SpawnLightFlash(sector);
                break;

            case StrobeFast:
                P_SpawnStrobeFlash(sector, FASTDARK, 0);
                break;

            case StrobeSlow:
                P_SpawnStrobeFlash(sector, SLOWDARK, 0);
                break;

            case StrobeHurt:
                P_SpawnStrobeFlash(sector, FASTDARK, 0);
                sector->special = StrobeHurt;
                break;

            case GlowingLight:
                P_SpawnGlowingLight(sector);
                break;

            case SecretSector:
                totalsecret++;
                break;

            case CloseDoorIn30Seconds:
                P_SpawnDoorCloseIn30(sector);
                break;

            case SyncStrobeSlow:
                P_SpawnStrobeFlash(sector, SLOWDARK, 1);
                break;

            case SyncStrobeFast:
                P_SpawnStrobeFlash(sector, FASTDARK, 1);
                break;

            case RaiseDoorIn5Minutes:
                P_SpawnDoorRaiseIn5Mins(sector);
                break;

            case FlickeringFire:
                P_SpawnFireFlicker(sector);
                break;
        }
    }

    // Init line EFFECTs
    numlinespecials = 0;
    for (i = 0; i < numlines; i++)
    {
        switch (lines[i].special)
        {
            case MovingWallTextureToLeft:
                linespeciallist[numlinespecials] = &lines[i];
                numlinespecials++;
                break;
        }
    }

    // Init other misc stuff
    activeceilingshead = NULL;
    activeplatshead = NULL;

    for (i = 0; i < MAXBUTTONS; i++)
        memset(&buttonlist[i], 0, sizeof(button_t));

    P_InitTagLists();
}
