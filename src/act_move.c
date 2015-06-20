 /***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Player movement module                                                *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#include "h/mud.h"
#include "h/hint.h"
#include "h/ftag.h"
#include "h/hometowns.h"
#include "h/polymorph.h"

/* Volk - from do_look in act_info.c */
void                    check_random_mobs( CHAR_DATA *ch );
void                    check_water_mobs( CHAR_DATA *ch );
void                    check_sky_mobs( CHAR_DATA *ch );
void                    do_build_walk( CHAR_DATA *ch, char *argument );
void                    check_sea_monsters( CHAR_DATA *ch );

/* Vladaar - changed this 08/04/2008, please dont change anymore.  This is due to findings
that although movement is cool to be realistic, but as a player it really sucks.  They spend
all their time having to rest, because their movement points were getting used way too fast.  
Realism vs. fun....  Fun wins every time or no players will we have.  
*/

/* Give them more moves, make moves regen faster, let refresh heal more moves. We need more 
 * leeway otherwise move mod spells like fly and float lose functionality..  - Volk
 */

const short             movement_loss[SECT_MAX] = {
/* 47 all up! SECT_INSIDE, SECT_ROAD, SECT_FIELD, SECT_FOREST, SECT_HILLS, SECT_MOUNTAIN, */
    1, 1, 1, 1, 2, 3,
/*  SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_UNDERWATER, SECT_AIR, SECT_DESERT, */
    20, 20, 22, 1, 1,
/*  SECT_AREA_ENT, SECT_OCEANFLOOR, SECT_UNDERGROUND, SECT_LAVA, SECT_SWAMP,  */
    1, 22, 1, 4, 6,
/*  SECT_CITY, SECT_VROAD, SECT_HROAD, SECT_OCEAN, SECT_JUNGLE, SECT_GRASSLAND, */
    1, 1, 1, 100, 2, 1,
/*  SECT_CROSSROAD, SECT_THICKFOREST, SECT_HIGHMOUNTAIN, SECT_ARCTIC,  */
    1, 2, 3, 2,
/*  SECT_WATERFALL, SECT_RIVER, SECT_DOCK, SECT_LAKE, SECT_CAMPSITE, SECT_PORTALSTONE, */
    12, 20, 1, 20, 1, 1,
/*  SECT_DEEPMUD, SECT_QUICKSAND, SECT_PASTURELAND, SECT_VALLEY, SECT_MOUNTAINPASS,  */
    4, 5, 1, 1, 2,
/*  SECT_BEACH, SECT_FOG, SECT_NOCHANGE, SECT_SKY, SECT_CLOUD,  */
    1, 2, 1, 1, 1,
/*  SECT_DCLOUD, SECT_ORE, SECT_MAX */
    1, 1
};

const char             *const dir_name[] = {
    "norte", "este", "sur", "oeste",
    "arriba", "abajo", "noreste", "noroeste",
    "sureste", "suroeste", "lugar", "explorar"
};
const char             *const dir_name2[] = {
    "el norte", "el este", "el sur", "el oeste",
    "arriba", "abajo", "el noreste", "el noroeste",
    "el sureste", "el suroeste", "algún lugar", "explorar"
};

const int               trap_door[] = {
    TRAP_N, TRAP_E, TRAP_S, TRAP_W, TRAP_U, TRAP_D,
    TRAP_NE, TRAP_NW, TRAP_SE, TRAP_SW
};

const short             rev_dir[] = {
    2, 3, 0, 1,
    5, 4, 9, 8,
    7, 6, 10, 11
};

/* Build walk to new rooms - Vladaar - http://6dragons.org */
void do_build_walk( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA        *location,
                           *ch_location;
    AREA_DATA              *pArea;
    int                     vnum,
                            edir = 0;
    char                    tmpcmd[MAX_INPUT_LENGTH];
    EXIT_DATA              *xit;

    if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
        set_char_color( AT_PLAIN, ch );

        ch_location = ch->in_room;

        argument = one_argument( argument, arg );

        edir = get_dir( arg );

        xit = get_exit( ch_location, edir );

        if ( !xit ) {
            pArea = ch->in_room->area;
            vnum = pArea->low_r_vnum;

            if ( !pArea ) {
                bug( "buildwalking: !pArea" );
                return;
            }

            while ( vnum <= pArea->hi_r_vnum && get_room_index( vnum ) != NULL )
                vnum++;
            if ( vnum > pArea->hi_r_vnum ) {
                send_to_char
                    ( "&GYou cannot buildwalk anymore as there are no empty higher number rooms to be found.\r\n",
                      ch );
                return;
            }
            ch_printf( ch, "&GBuildwalking from room %d to %d to the %s.\r\r\n\n",
                       ch->in_room->vnum, vnum, arg );

            location = make_room( vnum, pArea );
            if ( !location ) {
                bug( "buildwalking: make_room failed" );
                return;
            }
            location->area = pArea;
            sprintf( tmpcmd, "bexit %s %d", arg, vnum );
            do_redit( ch, tmpcmd );
        }
        else {
            vnum = xit->vnum;
            location = get_room_index( vnum );
            ch_printf( ch,
                       "&GCannot buildwalk back into a room that you already created an exit.\r\r\n\n" );
        }

        location->sector_type = ch_location->sector_type;
        location->room_flags = ch_location->room_flags;
        sprintf( buf, "%d", vnum );
        do_goto( ch, buf );

        return;
    }
    move_char( ch, get_exit( ch->in_room, edir ), 0, FALSE );
}

/*
 * Local functions.
 */
OBJ_DATA               *has_key args( ( CHAR_DATA *ch, int key ) );
void                    to_channel( const char *argument, const char *xchannel, int level );
void                    trap_sprung(  );

const char             *const sect_names[SECT_MAX][2] = {
    {"In a room", "inside"}, {"In a city", "cities"},
    {"In a field", "fields"}, {"In a forest", "forests"},
    {"hill", "hills"}, {"On a mountain", "mountains"},
    {"In the water", "waters"}, {"In rough water", "waters"},
    {"Underwater", "underwaters"}, {"In the air", "air"},
    {"In a desert", "deserts"}, {"Somewhere", "unknown"},
    {"ocean floor", "ocean floor"}, {"underground", "underground"}
};

const int               sent_total[SECT_MAX] = {
    3, 5, 4, 4, 1, 1, 1, 1, 1, 2, 2, 25, 1, 1
};

const char             *const room_sents[SECT_MAX][25] = {
    {
     "rough hewn walls of granite with the occasional spider crawling around",
     "signs of a recent battle from the bloodstains on the floor",
     "a damp musty odour not unlike rotting vegetation"},
    {
     "the occasional stray digging through some garbage",
     "merchants trying to lure customers to their tents",
     "some street people putting on an interesting display of talent",
     "an argument between a customer and a merchant about the price of an item",
     "several shady figures talking down a dark alleyway"},
    {
     "sparce patches of brush and shrubs",
     "a small cluster of trees far off in the distance",
     "grassy fields as far as the eye can see",
     "a wide variety of weeds and wildflowers"},
    {
     "tall, dark evergreens prevent you from seeing very far",
     "many huge oak trees that look several hundred years old",
     "a solitary lonely weeping willow",
     "a patch of bright white birch trees slender and tall"},
    {
     "rolling hills lightly speckled with violet wildflowers"},
    {
     "the rocky mountain pass offers many hiding places"},
    {
     "the water is smooth as glass"},
    {
     "rough waves splash about angrily"},
    {
     "a small school of fish"},
    {
     "the land far below",
     "a misty haze of clouds"},
    {
     "sand as far as the eye can see",
     "an oasis far in the distance"},
    {
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual", "nothing unusual", "nothing unusual",
     "nothing unusual",
     },
    {"rocks and coral which litter the ocean floor."},
    {"a lengthy tunnel of rock."}
};

char                   *grab_word( char *argument, char *arg_first )
{
    char                    cEnd;
    short                   count;

    count = 0;

    while ( isspace( *argument ) )
        argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
        cEnd = *argument++;

    while ( *argument != '\0' || ++count >= 255 ) {
        if ( *argument == cEnd ) {
            argument++;
            break;
        }
        *arg_first++ = *argument++;
    }
    *arg_first = '\0';

    while ( isspace( *argument ) )
        argument++;

    return argument;
}

char                   *wordwrap( char *txt, short wrap )
{
    static char             buf[MSL];
    char                   *bufp;

    buf[0] = '\0';
    bufp = buf;
    if ( txt != NULL ) {
        char                    line[MSL];
        char                    temp[MSL];
        char                   *ptr,
                               *p;
        int                     ln,
                                x;

        ++bufp;
        line[0] = '\0';
        ptr = txt;
        while ( *ptr ) {
            ptr = grab_word( ptr, temp );
            ln = strlen( line );
            x = strlen( temp );
            if ( ( ln + x + 1 ) < wrap ) {
                if ( ln > 0 && line[ln - 1] == '.' )
                    mudstrlcat( line, "  ", MSL );
                else
                    mudstrlcat( line, " ", MSL );
                mudstrlcat( line, temp, MSL );
                p = strchr( line, '\n' );
                if ( !p )
                    p = strchr( line, '\r' );
                if ( p ) {
                    mudstrlcat( buf, line, MSL );
                    line[0] = '\0';
                }
            }
            else {
                mudstrlcat( line, "\r\n", MSL );
                mudstrlcat( buf, line, MSL );
                mudstrlcpy( line, temp, MSL );
            }
        }
        if ( line[0] != '\0' )
            mudstrlcat( buf, line, MSL );
    }
    return bufp;
}

const char             *rev_exit( short vdir )
{
    switch ( vdir ) {
        default:
            return "algún lugar";
        case 0:
            return "el sur";
        case 1:
            return "el oeste";
        case 2:
            return "el norte";
        case 3:
            return "el este";
        case 4:
            return "abajo";
        case 5:
            return "arriba";
        case 6:
            return "el suroeste";
        case 7:
            return "el sureste";
        case 8:
            return "el noroeste";
        case 9:
            return "el noreste";
        case 11:
            return "la entrada";
    }

    return "<??\?>";
}

/*
 * Function to get the equivelant exit of DIR 0-MAXDIR out of linked list.
 * Made to allow old-style diku-merc exit functions to work.  -Thoric
 */
EXIT_DATA              *get_exit( ROOM_INDEX_DATA *room, short dir )
{
    EXIT_DATA              *xit;

    if ( !room ) {
        bug( "%s", "Get_exit: NULL room" );
        return NULL;
    }

    for ( xit = room->first_exit; xit; xit = xit->next )
        if ( xit->vdir == dir )
            return xit;
    return NULL;
}

/*
 * Function to get an exit, leading the the specified room
 */
EXIT_DATA              *get_exit_to( ROOM_INDEX_DATA *room, short dir, int vnum )
{
    EXIT_DATA              *xit;

    if ( !room ) {
        bug( "%s", "Get_exit: NULL room" );
        return NULL;
    }

    for ( xit = room->first_exit; xit; xit = xit->next )
        if ( xit->vdir == dir && xit->vnum == vnum )
            return xit;
    return NULL;
}

/*
 * Function to get the nth exit of a room   -Thoric
 */
EXIT_DATA              *get_exit_num( ROOM_INDEX_DATA *room, short count )
{
    EXIT_DATA              *xit;
    int                     cnt;

    if ( !room ) {
        bug( "%s", "Get_exit: NULL room" );
        return NULL;
    }

    for ( cnt = 0, xit = room->first_exit; xit; xit = xit->next )
        if ( ++cnt == count )
            return xit;
    return NULL;
}

/*
 * Modify movement due to encumbrance     -Thoric
 */
short encumbrance( CHAR_DATA *ch, short move )
{
    int                     cur,
                            max;

    max = can_carry_w( ch );
    cur = ch->carry_weight;
    if ( cur >= max )
        return move * 4;
    else if ( cur >= max * 0.95 )
        return ( short ) ( move * 3.5 );
    else if ( cur >= max * 0.90 )
        return move * 3;
    else if ( cur >= max * 0.85 )
        return ( short ) ( move * 2.5 );
    else if ( cur >= max * 0.80 )
        return move * 2;
    else if ( cur >= max * 0.75 )
        return ( short ) ( move * 1.5 );
    else
        return move;
}

/*
 * Check to see if a character can fall down, checks for looping   -Thoric
 */
bool will_fall( CHAR_DATA *ch, int fall )
{
    if ( !ch ) {
        bug( "%s", "will_fall: NULL *ch!!" );
        return FALSE;
    }

    if ( !ch->in_room ) {
        bug( "will_fall: Character in NULL room: %s", ch->name ? ch->name : "Unknown?!?" );
        return FALSE;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_NOFLOOR ) && CAN_GO( ch, DIR_DOWN )
         && ( !IS_AFFECTED( ch, AFF_FLYING )
              || ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) ) ) ) {
        if ( fall > 80 ) {
            bug( "Falling (in a loop?) more than 80 rooms: vnum %d", ch->in_room->vnum );
            char_from_room( ch );
            char_to_room( ch, get_room_index( ch->pcdata->htown->recall ) );
            fall = 0;
            return TRUE;
        }
        set_char_color( AT_FALLING, ch );
        send_to_char( "Caes...\r\n", ch );
        move_char( ch, get_exit( ch->in_room, DIR_DOWN ), ++fall, FALSE );
        return TRUE;
    }
    return FALSE;
}

void remove_bexit_flag( EXIT_DATA *pexit, int flag )
{
    EXIT_DATA              *pexit_rev;

    REMOVE_BIT( pexit->exit_info, flag );
    if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
        REMOVE_BIT( pexit_rev->exit_info, flag );
}

void set_bexit_flag( EXIT_DATA *pexit, int flag )
{
    EXIT_DATA              *pexit_rev;

    SET_BIT( pexit->exit_info, flag );
    if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
        SET_BIT( pexit_rev->exit_info, flag );
}

//Slightly modified to prevent those that are praying, ensared, or 
//rooted from moving. -Taon
ch_ret move_char( CHAR_DATA *ch, EXIT_DATA *pexit, int fall, bool running )
{
    ROOM_INDEX_DATA        *in_room;
    ROOM_INDEX_DATA        *to_room;
    ROOM_INDEX_DATA        *from_room;
    OBJ_DATA               *boat;
    char                    buf[MSL];
    const char             *txt,
                           *dtxt;
    ch_ret                  retcode;
    int                     move = 0;
    short                   door,
                            chance;
    bool                    drunk = FALSE;
    bool                    nuisance = FALSE;
    bool                    brief = FALSE;
    bool                    autofly = FALSE;
    bool                    autoopen = FALSE;

    if ( IS_AFFECTED( ch, AFF_OTTOS_DANCE ) ) {
        short                   chance;

        chance = number_range( 1, 4 );
        if ( chance < 4 ) {
            act( AT_ACTION, "Te intentas mover, pero estás demasiado inmerso en la danza.", ch, NULL,
                 NULL, TO_CHAR );
            act( AT_ACTION, "$n casi se cae mientras trata de alejarse bailando.", ch, NULL,
                 NULL, TO_ROOM );
            return FALSE;
        }
        if ( chance == 4 )
            act( AT_ACTION, "Te las arreglas para danzar hacia la dirección a la que deseas ir.", ch,
                 NULL, NULL, TO_CHAR );
        act( AT_ACTION, "$n baila al ritmo de la música.", ch, NULL, NULL, TO_ROOM );
    }

// Make players config obj id when in tradeskills rooms
    if ( !IS_SET( ch->in_room->room_flags, ROOM_TRADESKILLS ) ) {
        if ( xIS_SET( ch->act, PLR_OBJID ) ) {
            xREMOVE_BIT( ch->act, PLR_OBJID );
        }
        if ( !xIS_SET( ch->act, PLR_COMBINE )) {
            xSET_BIT( ch->act, PLR_COMBINE );
        }
    }



    if ( !IS_NPC( ch ) ) {
        if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
            send_to_char( "Estás bajo tierra, no puedes.\r\n", ch );
            return FALSE;
        }
        if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
            xREMOVE_BIT( ch->affected_by, AFF_BURROW );
        }

        if ( !IS_NPC( ch ) ) {
            if ( ch->desc && !ch->switched ) {
                if ( ch->pcdata->lastip != NULL && str_cmp( ch->desc->host, "(Link-Dead)" ) ) {
                    STRFREE( ch->pcdata->lastip );
                }
                ch->pcdata->lastip = STRALLOC( ch->desc->host );
                save_char_obj( ch );
            }
        }

        if ( IS_AFFECTED( ch, AFF_SNARE ) ) {
            send_to_char( "¡Estás en una trampa, no puedes moverte!\r\n", ch );
            return FALSE;
        }
        if ( IS_AFFECTED( ch, AFF_ROOT ) && !IS_IMMORTAL( ch ) ) {
            send_to_char( "No te puedes mover, estás enraizado.\r\n", ch );
            return FALSE;
        }
        if ( xIS_SET( ch->act, PLR_MUSIC ) ) {
            if ( ch->in_room && ( ch->in_room->vnum == 42300 ) )
                send_to_char( "!!SOUND(sound/paleon.wav)\r\n", ch );
            if ( ch->in_room && ( ch->in_room->vnum == 27024 ) )
                send_to_char( "!!SOUND(sound/dakar.wav)\r\n", ch );
            if ( ch->in_room && ( ch->in_room->vnum == 42081 ) )
                send_to_char( "!!SOUND(sound/forbidden.wav)\r\n", ch );
        }

        if ( IS_AFFECTED( ch, AFF_TANGLE ) && !IS_IMMORTAL( ch ) ) {
            send_to_char( "Estás enredado. No puedes moverte.\r\n", ch );
            return FALSE;
        }
        if ( IS_AFFECTED( ch, AFF_PRAYER ) && !IS_IMMORTAL( ch ) ) {
            send_to_char( "Deja de meditar primero.\r\n", ch );
            return FALSE;
        }

        if ( !IS_NPC( ch ) && IS_SET( ch->pcdata->tag_flags, TAG_FROZEN )
             && IS_SET( ch->pcdata->tag_flags, TAG_PLAYING ) ) {
            send_to_char( "¡Ahora no!\r\n", ch );
            return FALSE;
        }

        chance = number_range( 1, 100 );
        if ( chance < 10 && ch->level < 6 ) {
            hint_update(  );
        }

        if ( ch->position == POS_MEDITATING ) {
            send_to_char( "Perderías la concentración.\r\n", ch );
            return FALSE;
        }

        if ( ch->in_room->sector_type == SECT_OCEAN && ch->move == 0
             && IS_AFFECTED( ch, AFF_FLYING ) ) {
            xREMOVE_BIT( ch->affected_by, AFF_FLYING );
        }

        if ( IS_IMMORTAL( ch ) ) {
            ch->pcdata->tmproom = ch->in_room->vnum;
        }

        if ( IS_DRUNK( ch, 2 ) && ch->position != POS_SHOVE && ch->position != POS_DRAG )
            drunk = TRUE;

        if ( ch->pcdata && xIS_SET( ch->act, PLR_TUTORIAL ) ) {
            ch_printf( ch, "&cPor favor, espera instrucciones.\r\n" );
            if ( !IS_IMMORTAL( ch ) &&
                 ( ch->in_room && !str_cmp( ch->in_room->area->filename, "tutorial.are" ) )
                 || ( ch->in_room && !str_cmp( ch->in_room->area->filename, "etutorial.are" ) )
                 || ( ch->in_room && !str_cmp( ch->in_room->area->filename, "dtutorial.are" ) ) ) {
                ch_printf( ch,
                           "&cQuédate en tu lugar\r\nesperando instrucciones\r\necesarias para completar el tutorial. Puedes\r\ndisminuir la velocidad del tutorial con el comando &WTEXTSPEED&c.\r\n" );
            }
            return FALSE;
        }

        /*
         * Nuisance flag, makes them walk in random directions 50% of the time. -Shaddai
         */
        if ( ch->pcdata->nuisance && ch->pcdata->nuisance->flags > 8 &&
             ch->position != POS_SHOVE && ch->position != POS_DRAG
             && number_percent(  ) > ( ch->pcdata->nuisance->flags * ch->pcdata->nuisance->power ) )
            nuisance = TRUE;
    }
    if ( IS_AFFECTED( ch, AFF_PARALYSIS ) ) {
        send_to_char( "¡No puedes mover tus piernas!\r\n", ch );
        return FALSE;
    }

    in_room = ch->in_room;

    if ( !fall && ( nuisance || drunk ) ) {
        door = number_door(  );
        pexit = get_exit( ch->in_room, door );
    }

    retcode = rNONE;
    txt = NULL;

    if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_MOUNTED ) )
        return retcode;

    from_room = in_room;

    if ( !pexit || ( to_room = pexit->to_room ) == NULL ) {
        if ( drunk && ch->position != POS_MOUNTED && ch->in_room->sector_type != SECT_WATER_SWIM
             && ch->in_room->sector_type != SECT_WATER_NOSWIM
             && ch->in_room->sector_type != SECT_UNDERWATER
             && ch->in_room->sector_type != SECT_OCEANFLOOR ) {
            switch ( number_bits( 4 ) ) {
                default:
                    act( AT_ACTION, "Te tambaleas golpeando cosas.", ch, NULL, NULL,
                         TO_CHAR );
                    act( AT_ACTION, "$n se tambalea y golpea cosas.", ch, NULL, NULL,
                         TO_ROOM );
                    break;
                case 3:
                    act( AT_ACTION,
                         "Tropiezas con tus propios pies y caes al suelo.",
                         ch, NULL, NULL, TO_CHAR );
                    act( AT_ACTION, "$n tropieza con sus propios pies y cae al suelo.", ch,
                         NULL, NULL, TO_ROOM );
                    set_position( ch, POS_RESTING );
                    break;
                case 4:
                    act( AT_SOCIAL, "Sueltas una cadena de palabrotas.", ch, NULL, NULL,
                         TO_CHAR );
                    act( AT_ACTION,
                         "Algo borroso intercepta tu camino.",
                         ch, NULL, NULL, TO_CHAR );
                    act( AT_HURT,
                         "Oh, ¡Qué daño! Todo se vuelve oscuro...",
                         ch, NULL, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se golpea con algo.", ch, NULL, NULL,
                         TO_ROOM );
                    act( AT_SOCIAL, "$n suelta una retaíla de palabrotas: @*&^%@*&!", ch,
                         NULL, NULL, TO_ROOM );
                    act( AT_ACTION, "$n se cae al suelo.", ch, NULL, NULL,
                         TO_ROOM );
                    set_position( ch, POS_INCAP );
                    break;
            }
        }
        else if ( nuisance )
            act( AT_ACTION, "Intentas recordar hacia donde querías ir.", ch, NULL,
                 NULL, TO_CHAR );
        else if ( drunk )
            act( AT_ACTION,
                 "Miras a tu alrededor, tratando de darle sentido a las cosas en tu borrachera.", ch,
                 NULL, NULL, TO_CHAR );
        else
            send_to_char( "No puedes ir por ahí.\r\n", ch );
        return rSTOP;
    }
    door = pexit->vdir;

    if ( ch->morph != NULL && !str_cmp( ch->morph->morph->name, "fish" )
         && ( to_room->sector_type != SECT_UNDERWATER && to_room->sector_type != SECT_OCEAN
              && to_room->sector_type != SECT_LAKE && to_room->sector_type != SECT_WATER_SWIM
              && to_room->sector_type != SECT_OCEANFLOOR && to_room->sector_type != SECT_RIVER
              && to_room->sector_type != SECT_WATER_NOSWIM ) ) {
        act( AT_ACTION, "¡Ya no hay agua!", ch, NULL, NULL,
             TO_CHAR );
        act( AT_ACTION, "¡$n chapotea en el aire!", ch, NULL, NULL,
             TO_ROOM );
        return rSTOP;
    }

    if ( xIS_SET( ch->act, PLR_BOAT )
         && ( to_room->sector_type != SECT_UNDERWATER && to_room->sector_type != SECT_OCEAN
              && to_room->sector_type != SECT_LAKE && to_room->sector_type != SECT_WATER_SWIM
              && to_room->sector_type != SECT_OCEANFLOOR && to_room->sector_type != SECT_RIVER
              && to_room->sector_type != SECT_WATER_NOSWIM
              && to_room->sector_type != SECT_DOCK ) ) {
        act( AT_ACTION, "¡No puedes mover el barco en la tierra!", ch, NULL, NULL, TO_CHAR );
        return rSTOP;
    }
    if ( xIS_SET( ch->act, PLR_BOAT ) && IN_WILDERNESS( ch ) ) {
        if ( ch->in_room->sector_type == SECT_OCEAN )
            check_sea_monsters( ch );
    }

    /*
     * Exit is only a "window", there is no way to travel in that direction
     * unless it's a door with a window in it -Thoric
     */
    if ( IS_SET( pexit->exit_info, EX_WINDOW ) && !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
        send_to_char( "No puedes ir por ahí.\r\n", ch );
        return rSTOP;
    }

    if ( IS_SET( pexit->exit_info, EX_DIG ) ) {
        send_to_char( "No puedes ir por ahí.\r\n", ch );
        return rSTOP;
    }

    if ( IS_SET( pexit->exit_info, EX_PORTAL ) && IS_NPC( ch ) ) {
        act( AT_PLAIN, "Los mobs no pueden usar portales.", ch, NULL, NULL, TO_CHAR );
        return rSTOP;
    }

    // Put out flamnig shields if they enter the water. -Taon
    if ( ( to_room->sector_type == SECT_UNDERWATER || to_room->sector_type == SECT_OCEAN
           || to_room->sector_type == SECT_LAKE || to_room->sector_type == SECT_WATER_SWIM
           || to_room->sector_type == SECT_RIVER || to_room->sector_type == SECT_WATER_NOSWIM )
         && IS_AFFECTED( ch, AFF_FLAMING_SHIELD ) ) {
        send_to_char( "&BTu escudo de fuego desaparece con el agua.&d\r\n", ch );
        affect_strip( ch, gsn_flaming_shield );
        xREMOVE_BIT( ch->affected_by, AFF_FLAMING_SHIELD );
    }

    // stop water wilderness mobs from following out of water?
    if ( IS_NPC( ch ) ) {
        if ( xIS_SET( ch->act, ACT_WATER )
             && ( to_room->sector_type != SECT_OCEANFLOOR || to_room->sector_type != SECT_OCEAN
                  || to_room->sector_type != SECT_UNDERWATER
                  || to_room->sector_type != SECT_WATER_SWIM
                  || to_room->sector_type != SECT_WATER_NOSWIM
                  || to_room->sector_type != SECT_RIVER ) ) {
            return FALSE;
        }
    }

    if ( rprog_pre_enter_trigger( ch, to_room ) == TRUE )
        return rSTOP;

//  fix for beastmaster
    if ( !xIS_SET( ch->act, ACT_BEASTPET ))
     {
        if ( IS_NPC( ch )
             && ( IS_SET( pexit->exit_info, EX_NOMOB )
                  || IS_SET( to_room->room_flags, ROOM_NO_MOB ) ) ) {
            act( AT_PLAIN, "Mobs no, gracias.", ch, NULL, NULL, TO_CHAR );
            return rSTOP;
        }
    }
    // Status: Just getting a good foot-hold.. -Taon
    if ( IN_WILDERNESS( ch ) ) {
        switch ( to_room->sector_type ) {              // Allow fall throughs in this
                // switch. -Taon
            case SECT_OCEAN:
                if ( IS_AFFECTED( ch, AFF_FLYING ) )   // && ch->race == RACE_DRAGON )
                {
                    send_to_char( "Te elevas sobre el poderoso océano.\r\n", ch );
                }
                else if ( xIS_SET( ch->act, PLR_BOAT ) )
                    send_to_char( "Navegas por el poderoso océano.\r\n", ch );

                /*
                 * else if( IS_AFFECTED( ch, AFF_FLYING ) && ch->race != RACE_DRAGON ) {
                 * send_to_char( "Expending enormous energy, you soar over the mighty ocean
                 * below.\r\n", ch ); ch->move = ch->move/4; } 
                 */
                else if ( IS_IMMORTAL( ch ) )
                    send_to_char( "Caminas sobre el agua.\r\n", ch );
                else {
                    if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
                        send_to_char
                            ( "No se puede flotar en el agua, tendrás que aterrizar y nadar.\r\n",
                              ch );
                        return rSTOP;
                    }
                }
                break;
            case SECT_ARCTIC:
                break;
            case SECT_HILLS:
                break;
            case SECT_GRASSLAND:
            case SECT_FIELD:
                break;
            case SECT_LAKE:
                break;
            case SECT_CAMPSITE:
                break;
            case SECT_CROSSROAD:
            case SECT_VROAD:
            case SECT_HROAD:
                send_to_char( "Viajas por la carretera.\r\n", ch );
                break;
            case SECT_SWAMP:
                break;
            case SECT_FOREST:
                break;
            case SECT_MOUNTAIN:
                break;
            case SECT_HIGHMOUNTAIN:
                if ( !IS_IMMORTAL( ch ) ) {
                    if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
                        send_to_char( "Necesitas volar para ir en esa dirección.\r\n.\r\n", ch );
                        return FALSE;
                    }
                }
                break;
            case SECT_RIVER:
                break;
            case SECT_LAVA:
                if ( ch->race != RACE_DEMON && !IS_IMMORTAL( ch ) ) {
                    send_to_char( "Te quemas al alcanzarte la lava ardiente.\r\n", ch );
                    if ( ch->hit > 50 )
                        ch->hit -= 50;
                    else
                        ch->hit = 1;
                }
                break;
            case SECT_DOCK:
                {
                    if ( xIS_SET( ch->act, PLR_COMMUNICATION ) )
                        send_to_char( "!!SOUND(sound/dock.wav)\r\n", ch );
                }
                break;
            case SECT_DESERT:
                break;
            case SECT_JUNGLE:
            case SECT_THICKFOREST:
                break;
            case SECT_WATERFALL:
                break;
            default:
                break;
        }
    }

    if ( IS_SET( pexit->exit_info, EX_CLOSED )
         && ( ( !IS_AFFECTED( ch, AFF_PASS_DOOR ) && !xIS_SET( ch->act, PLR_SHADOWFORM ) )
              || IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) ) {
        if ( !IS_SET( pexit->exit_info, EX_SECRET ) && !IS_SET( pexit->exit_info, EX_DIG ) ) {
            if ( drunk ) {
                act( AT_PLAIN, "$n corre hacia el $d en su estado de borrachera.", ch, NULL, pexit->keyword,
                     TO_ROOM );
                act( AT_PLAIN, "Corres hacia el $d en tu estado de borrachera.", ch, NULL,
                     pexit->keyword, TO_CHAR );
            }
            else {
                if ( IS_AFFECTED( ch, AFF_FLYING )
                     && ch->in_room->sector_type != SECT_INSIDE
                     && pexit->to_room->sector_type != SECT_INSIDE
                     && !IS_SET( ch->in_room->room_flags, ROOM_INDOORS )
                     && !IS_SET( pexit->to_room->room_flags, ROOM_INDOORS ) )
                    autofly = TRUE;
                else if ( xIS_SET( ch->act, PLR_AUTODOOR )
                          && !IS_SET( pexit->exit_info, EX_LOCKED ) )
                    autoopen = TRUE;
                else
                    act( AT_PLAIN, "$d te impide el paso.", ch, NULL, pexit->keyword, TO_CHAR );
            }
        }
        else {
            if ( drunk )
                send_to_char( "Tu borrachera te hace dudar.\r\n", ch );
            else
                send_to_char( "No puedes ir por ahí.\r\n", ch );
        }

        if ( !autofly && !autoopen )                   /* Let autofly and autoopen go on */
            return rSTOP;
    }

    if ( room_is_private( to_room ) ) {
        send_to_char( "Esa sala es pribada ahora mismo.\r\n", ch );
        return rSTOP;
    }

    if ( room_is_dnd( ch, to_room ) ) {
        send_to_char( "Esa sala es \"antidisturvios\" ahora.\r\n", ch );
        return rSTOP;
    }

if ( !xIS_SET( ch->act, ACT_BEASTPET ))
{
    if ( IS_NPC( ch )
         && ( IS_SET( pexit->exit_info, EX_NOMOB )
              || IS_SET( to_room->room_flags, ROOM_NO_MOB ) ) ) {
        return rSTOP;
    }
}
    if ( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ch->in_room->area != to_room->area ) {
        if ( ch->level < to_room->area->low_hard_range ) {
            if ( ch->race == RACE_DRAGON && to_room->area->low_hard_range < 100 ) {
                set_char_color( AT_TELL, ch );
                send_to_char
                    ( "Una voz en tu cabeza dice, 'Puede que no estés preparado para ir por ahí. ¡O quizás sea tu forma de dragón!\r\n",
                      ch );
            }
            else if ( ch->race != RACE_DRAGON
                      || ( to_room->area->low_hard_range == 100 && ch->level < 100 ) ) {
                set_char_color( AT_TELL, ch );
                switch ( to_room->area->low_hard_range - ch->level ) {
                    case 1:
                        send_to_char
                            ( "Una voz en tu cabeza dice, 'Te falta algo de experiencia para poder entrar ahí...'\r\n",
                              ch );
                        break;
                    case 2:
                        send_to_char
                            ( "Una voz en tu cabeza dice, 'Muy poronto podrás pasar... soon.'\r\n",
                              ch );
                        break;
                    case 3:
                        send_to_char
                            ( "Una voz en tu cabeza dice, 'Te falta experiencia para ir por ese lugar... yet.'.\r\n",
                              ch );
                        break;
                    default:
                        send_to_char
                            ( "Una voz en tu cabeza dice, 'No tienes suficiente experiencia para caminar por ahí.'.\r\n",
                              ch );
                }
                return rSTOP;
            }
        }
        else if ( ch->level > to_room->area->hi_hard_range ) {
            set_char_color( AT_TELL, ch );
            send_to_char
                ( "Una voz en tu cabeza dice, 'Eres demasiado grande para entrar ahí.'\r\n",
                  ch );
            return rSTOP;
        }
    }

    if ( !fall && !IS_NPC( ch ) ) {
        /*
         * Prevent deadlies from entering an antipkill-flagged area from a non-flagged
         * area, but allow them to move around if already inside one. - Blodkai
         */
        if ( IS_SET( to_room->area->flags, AFLAG_ANTIPKILL )
             && !IS_SET( ch->in_room->area->flags, AFLAG_ANTIPKILL ) && ( IS_PKILL( ch )
                                                                          && !IS_IMMORTAL( ch ) ) ) {
            set_char_color( AT_MAGIC, ch );
            send_to_char
                ( "\r\nUna fuerza misteriosa te impide pasar...\r\n",
                  ch );
            return rSTOP;
        }

        if ( in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR
             || IS_SET( pexit->exit_info, EX_FLY ) ) {
            if ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLYING ) ) {
                send_to_char( "tu montura no puede volar.\r\n", ch );
                return rSTOP;
            }
            if ( !ch->mount && !IS_AFFECTED( ch, AFF_FLYING ) ) {
                send_to_char( "tu mascota no puede volar.\r\n", ch );
                return rSTOP;
            }
        }

        if ( in_room->sector_type == SECT_WATER_NOSWIM
             || to_room->sector_type == SECT_WATER_NOSWIM ) {
            if ( ( ch->mount && !IS_FLOATING( ch->mount ) ) || !IS_FLOATING( ch ) ) {
                /*
                 * Look for a boat.
                 * We can use the boat obj for a more detailed description.
                 */
                if ( ( boat = get_objtype( ch, ITEM_BOAT ) ) != NULL ) {
                    if ( drunk )
                        txt = "rema de forma desigual";
                    else
                        txt = "rema";
                }
                else {
                    if ( ch->mount )
                        send_to_char( "¡Tu montura se niega a ir por ahí!\r\n", ch );
                    else
                        send_to_char( "Necesitas una embarcación para ir por ahí.\r\n", ch );
                    return rSTOP;
                }
            }
        }
        /*
         * Added this for swimming underwater 
         */
        if ( in_room->sector_type == SECT_UNDERWATER || in_room->sector_type == SECT_OCEANFLOOR ) {
            if ( ch->pcdata->learned[gsn_swim] > 50 ) {
                txt = "nada con confianza";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats / 2 ) );
            }
            else if ( ch->pcdata->learned[gsn_swim] > 0 ) {
                txt = "nada";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats ) );
            }
            else {
                txt = "nada";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats * 2 ) );
            }
        }

        if ( ( in_room->sector_type == SECT_WATER_SWIM
               || in_room->sector_type == SECT_WATER_NOSWIM || in_room->sector_type == SECT_OCEAN )
             && ( to_room->sector_type == SECT_UNDERWATER
                  || to_room->sector_type == SECT_OCEANFLOOR ) ) {
            if ( ch->pcdata->learned[gsn_swim] > 50 ) {
                txt = "vucea confiadamente";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats / 2 ) );
            }
            else if ( ch->pcdata->learned[gsn_swim] > 0 ) {
                txt = "vucea";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats ) );
            }
            else {
                txt = "vucea";
                WAIT_STATE( ch, ( skill_table[gsn_swim]->beats * 2 ) );
            }
        }

        if ( IS_SET( pexit->exit_info, EX_CLIMB ) ) {
            bool                    found;

            found = FALSE;
            if ( ch->mount && IS_AFFECTED( ch->mount, AFF_FLYING ) )
                found = TRUE;
            else if ( IS_AFFECTED( ch, AFF_FLYING ) )
                found = TRUE;

            if ( !found && !ch->mount ) {
                if ( ( !IS_NPC( ch ) && number_percent(  ) > LEARNED( ch, gsn_climb ) ) || drunk
                     || ch->mental_state < -90 ) {
                    send_to_char( "¡Comienzas a trepar pero pierdes agarrre y caes!\r\n", ch );
                    learn_from_failure( ch, gsn_climb );
                    if ( pexit->vdir == DIR_DOWN ) {
                        retcode = move_char( ch, pexit, 1, running );
                        return retcode;
                    }
                    set_char_color( AT_RED, ch );
                    send_to_char( "¡OUCH! ¡Golpeas el suelo!\r\n", ch );
                    WAIT_STATE( ch, 10 );
                    retcode = damage( ch, ch, ( pexit->vdir == DIR_UP ? 10 : 5 ), TYPE_UNDEFINED );
                    return retcode;
                }
                found = TRUE;
                learn_from_success( ch, gsn_climb );
                WAIT_STATE( ch, skill_table[gsn_climb]->beats );
                txt = "trepa";
            }

            if ( !found ) {
                send_to_char( "No puedes trepar.\r\n", ch );
                return rNONE;
            }
        }

        if ( ch->mount ) {
            bool                    retVal = FALSE;

            switch ( ch->mount->position ) {
                case POS_DEAD:
                    send_to_char( "¡Tu montura ha muerto!\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_MORTAL:
                case POS_INCAP:
                    send_to_char( "Tu montura no puede moverse.\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_STUNNED:
                    send_to_char( "Tu montura no puede moverse.\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_SLEEPING:
                    send_to_char( "Tu montura está durmiendo.\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_RESTING:
                    send_to_char( "Tu montura está descansando.\r\n", ch );
                    retVal = TRUE;
                    break;
                case POS_SITTING:
                    send_to_char( "Tu montura está sentada.\r\n", ch );
                    retVal = TRUE;
                    break;
                default:
                    break;
            }

            if ( retVal == TRUE )
                return rSTOP;

            if ( !IS_FLOATING( ch->mount ) )
                move += movement_loss[UMIN( SECT_MAX - 1, in_room->sector_type )];
            else
                move += 1;

            if ( ch->race == RACE_DRAGON && ch->Class == CLASS_BLUE
                 && ( ch->in_room->sector_type == SECT_OCEAN
                      || ch->in_room->sector_type == SECT_UNDERWATER
                      || ch->in_room->sector_type == SECT_RIVER
                      || ch->in_room->sector_type == SECT_OCEANFLOOR
                      || ch->in_room->sector_type == SECT_WATER_SWIM
                      || ch->in_room->sector_type == SECT_LAKE ) ) {
                move += 1;
            }

            if ( ch->mount->move < move ) {
                send_to_char( "Tu montura está exausta.\r\n", ch );
                return rSTOP;
            }
        }
        else {

            if ( ch->pcdata->ship == 0 ) {
                if ( !IS_FLOATING( ch ) )
                    move +=
                        encumbrance( ch,
                                     movement_loss[UMIN( SECT_MAX - 1, in_room->sector_type )] );
                else
                    move += 1;
            }
            if ( ch->move < move ) {
                send_to_char( "El cansancio te impide dar un paso más.\r\n", ch );
                return rSTOP;
            }
        }

        WAIT_STATE( ch, 1 );
        if ( ch->mount )
            ch->mount->move -= move;
        else
            ch->move -= move;
    }

    /*
     * Check if player can fit in the room
     */
    if ( to_room->tunnel > 0 ) {
        CHAR_DATA              *ctmp;
        int                     count = ch->mount ? 1 : 0;

        for ( ctmp = to_room->first_person; ctmp; ctmp = ctmp->next_in_room ) {
            if ( ++count >= to_room->tunnel ) {
                if ( ch->mount && count == to_room->tunnel )
                    send_to_char( "No puedes ir hacia allí, no hay sala para ti y tu montura.\r\n", ch );
                else
                    send_to_char( "No hay sala por allí.\r\n", ch );
                return rSTOP;
            }
        }
    }

    if ( to_room->height > 0 ) {
        short                   gap;

        gap = ch->height - to_room->height;
        if ( ch->height > to_room->height ) {
            ch_printf( ch,
                       "Tu altura te impide pasar por %d pulgadas.\r\nprueba a gatear.\r\n",
                       gap );
            return rSTOP;
        }
    }

    if ( ch->position == POS_CRAWL ) {
        txt = "gatea";
    }

    if ( door == DIR_EXPLORE ) {
        txt = "se va hacia";
    }

    /*
     * Check for traps on exit - later
     */
    if ( IS_NPC( ch ) || !xIS_SET( ch->act, PLR_WIZINVIS ) ) {
        if ( fall )
            txt = "cae hacia";
        else if ( !txt ) {
            if ( ch->mount ) {
                if ( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
                    txt = "flota hacia";
                else if ( IS_AFFECTED( ch->mount, AFF_FLYING ) )
                    txt = "vuela hacia";
                else
                    txt = "cavalgahacia ";
            }
            else {
                if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
                    if ( drunk )
                        txt = "flota descontroladamente hacia";
                    else
                        txt = "flota hacia";
                }
                else if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
                    if ( drunk )
                        txt = "vuela descontroladamente hacia";
                    else
                        txt = "vuela hacia";
                }
                else if ( ch->position == POS_SHOVE )
                    txt = "recibe un empujón hacia";
                else if ( ch->position == POS_DRAG )
                    txt = "se arrastra hacia";
                else if ( ch->position == POS_CROUCH )
                    txt = "en cuclillas, camina hacia";
                else if ( ch->position == POS_CRAWL )
                    txt = "gatea hacia";
                else {
                    if ( drunk )
                        txt = "se tambalea hacia";
                    else
                        txt = "se va hacia";
                }
            }
        }

        if ( VLD_STR( pexit->keyword ) ) {
            if ( autoopen ) {
                /*
                 * act( AT_PLAIN, "\r\nAbres $d.\r\n", ch, NULL, pexit->keyword,
                 * TO_CHAR ); act( AT_PLAIN, "\r\n$n abre la salida $d.\r\n", ch, NULL,
                 * pexit->keyword, TO_ROOM ); 
                 */
                do_open( ch, ( char * ) pexit->keyword );
            }
            if ( autofly ) {
                if ( ch->race == RACE_DRAGON ) {
                    act( AT_PLAIN,
                         "\r\nUsando tus poderosas alas alzas el vuelo y pasas $d.\r\n",
                         ch, NULL, pexit->keyword, TO_CHAR );
                    act( AT_PLAIN,
                         "\r\n$n usa sus poderosas alas para elevarse y pasar $d.\r\n",
                         ch, NULL, pexit->keyword, TO_ROOM );
                }
                else {
                    act( AT_PLAIN, "\r\nAlzas el vuelo y esquivas la salida cerrada hacia $d.\r\n", ch, NULL,
                         pexit->keyword, TO_CHAR );
                    act( AT_PLAIN, "\r\n$n alza el vuelo y esquiva la salida cerrada hacia $d.\r\n", ch, NULL,
                         pexit->keyword, TO_ROOM );
                }
            }
        }

        /*
         * Print some messages about leaving the room. -Orion
         */
        if ( !IS_AFFECTED( ch, AFF_SNEAK ) ) {
            if ( ch->mount ) {
                act_printf( AT_ACTION, ch, NULL, ch->mount, TO_NOTVICT, "$n %s %s sobre $N.", txt,
                            dir_name2[door] );
            }
            else {
                act_printf( AT_ACTION, ch, NULL, ( void * ) dir_name2[door], TO_ROOM, "$n %s $T.",
                            txt );
            }
        }
        else {
            CHAR_DATA              *temp_char,
                                   *temp_next;
            char                    sneak_buf1[MSL],
                                    sneak_buf2[MSL];
            bool                    hasMount = ch->mount ? TRUE : FALSE;

            if ( hasMount ) {
                snprintf( sneak_buf1, MSL, "$n intenta caminar sigilosamente hacia %s.%c",
                          IS_NPC( ch->mount ) ? ch->mount->short_descr : ch->mount->name, '\0' );
                snprintf( sneak_buf2, MSL, "$n %s %s sobre %s.%c", txt, dir_name2[door],
                          IS_NPC( ch->mount ) ? ch->short_descr : ch->name, '\0' );
            }
            else {
                snprintf( sneak_buf1, MSL, "$n intenta caminar sigilosamente.%c", '\0' );
                snprintf( sneak_buf2, MSL, "$n %s %s.%c", txt, dir_name2[door], '\0' );
            }

            for ( temp_char = ch->in_room->first_person; temp_char; temp_char = temp_next ) {
                temp_next = temp_char->next_in_room;

                if ( IS_AFFECTED( temp_char, AFF_DETECT_SNEAK ) ) {
                    act( AT_ACTION, sneak_buf1, ch, NULL, temp_char, TO_VICT );
                    act( AT_ACTION, sneak_buf2, ch, NULL, temp_char, TO_VICT );
                }
            }
        }
    }

    rprog_leave_trigger( ch );

    if ( char_died( ch ) )
        return global_retcode;

    char_from_room( ch );
    char_to_room( ch, to_room );
    if ( ch->mount ) {
        rprog_leave_trigger( ch->mount );

        /*
         * Mount bug fix test. -Orion
         */
        if ( char_died( ch->mount ) )
            return global_retcode;

        if ( ch->mount ) {
            char_from_room( ch->mount );
            char_to_room( ch->mount, to_room );
        }

    }

    if ( IS_NPC( ch ) || !xIS_SET( ch->act, PLR_WIZINVIS ) ) {
        if ( fall )
            txt = "cae desde";
        else if ( ch->mount ) {
            if ( IS_AFFECTED( ch->mount, AFF_FLOATING ) )
                txt = "flota desde";
            else if ( IS_AFFECTED( ch->mount, AFF_FLYING ) )
                txt = "vuela desde";
            else
                txt = "cavalga desde";
        }
        else {

            if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
                if ( drunk )
                    txt = "flota descontroladamente desde";
                else
                    txt = "flota desde";
            }
            else if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
                if ( drunk )
                    txt = "vuela inestablemente desde";
                else
                    txt = "vuela desde";
            }
            else if ( ch->position == POS_SHOVE )
                txt = "recibe un empujón desde";
            else if ( ch->position == POS_DRAG )
                txt = "se arrastra desde";
            else if ( ch->position == POS_CROUCH )
                txt = "en cuclillas, camina desde";
            else if ( ch->position == POS_CRAWL )
                txt = "gatea desde";
            else {
                if ( drunk )
                    txt = "se tambalea desde";
                else
                    txt = "llega desde";
            }
        }

        dtxt = rev_exit( door );

        /*
         * Print some entering messages. -Orion
         */
        if ( !IS_AFFECTED( ch, AFF_SNEAK ) ) {
            if ( ch->mount ) {
                act_printf( AT_ACTION, ch, NULL, ch->mount, TO_ROOM, "$n %s %s sobre $N.", txt,
                            dtxt );
            }
            else {
                act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "$n %s %s.", txt, dtxt );
            }
        }
        else {
            CHAR_DATA              *temp_char,
                                   *temp_next;
            char                    sneak_buf1[MSL],
                                    sneak_buf2[MSL];
            bool                    hasMount = ch->mount ? TRUE : FALSE;

            if ( hasMount ) {
                snprintf( sneak_buf1, MSL, "$n intenta caminar sigilosamente sobre %s.%c",
                          IS_NPC( ch->mount ) ? ch->mount->short_descr : ch->mount->name, '\0' );
                snprintf( sneak_buf2, MSL, "$n %s %s sobre %s.%c", txt, dtxt,
                          IS_NPC( ch->mount ) ? ch->short_descr : ch->name, '\0' );
            }
            else {
                mudstrlcpy( sneak_buf1, "$n intenta caminar sigilosamente.", MSL );
                snprintf( sneak_buf2, MSL, "$n %s %s.", txt, dtxt );
            }

            for ( temp_char = ch->in_room->first_person; temp_char; temp_char = temp_next ) {
                temp_next = temp_char->next_in_room;

                if ( IS_AFFECTED( temp_char, AFF_DETECT_SNEAK )
                     || ( IS_AFFECTED( temp_char, AFF_DEMONIC_SIGHT )
                          && ( IS_AFFECTED( temp_char, AFF_NOSIGHT )
                               || !IS_AFFECTED( temp_char, AFF_BLINDNESS ) ) ) ) {
                    act( AT_ACTION, sneak_buf1, ch, NULL, temp_char, TO_VICT );
                    act( AT_ACTION, sneak_buf2, ch, NULL, temp_char, TO_VICT );
                }
            }
        }
    }

    if ( !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ch->in_room->area != to_room->area ) {
        set_char_color( AT_MAGIC, ch );
        if ( ch->level < to_room->area->low_soft_range ) {
            send_to_char( "No estás bien en este lugar...\r\n", ch );
        }
        else if ( ch->level > to_room->area->hi_soft_range ) {
            send_to_char( "Sientes que no ganarás mucho visitando este sitio...\r\n", ch );
        }
    }

    /*
     * Make sure everyone sees the room description of death traps. -Orion
     */
    if ( IS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !IS_IMMORTAL( ch ) ) {
        if ( xIS_SET( ch->act, PLR_BRIEF ) )
            brief = TRUE;
        xREMOVE_BIT( ch->act, PLR_BRIEF );
    }

// Volk - ONCE per group!!
/* Righto - we'll check if ch is in a group first. If not, hit them with encounter. If so
   and they are the leader, hit them. If so and their leader IS NOT in the room, hit them */

    CHAR_DATA              *gch;
    bool                    random = FALSE;

    if ( !IS_GROUPED( ch ) )
        random = TRUE;
    else if ( ch->leader == ch )
        random = TRUE;
    else                                               /* ch is in a group - where is * * 
                                                        * leader */
        for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
            if ( ch->leader == gch )
                break;

    if ( !random && ch->leader != gch )
        random = TRUE;

    do_look( ch, ( char * ) "auto" );

    if ( random ) {
        if ( IN_WILDERNESS( ch ) || IN_RIFT( ch ) ) {
            if ( ch->in_room->sector_type == SECT_OCEAN )
                check_water_mobs( ch );
            else
                check_random_mobs( ch );
        }

    }

    if ( brief )
        xSET_BIT( ch->act, PLR_BRIEF );

    /*
     * Put good-old EQ-munching death traps back in!  -Thoric
     */
    if ( IS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !IS_IMMORTAL( ch ) ) {
        act( AT_DEAD, "$n cae y muere horriblemente!", ch, NULL, NULL, TO_ROOM );
        set_char_color( AT_DEAD, ch );
        send_to_char( "UUPS... ¡has muerto!\r\n", ch );
        snprintf( buf, MSL, "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
        log_string( buf );
        to_channel( buf, "log", LEVEL_IMMORTAL );
        extract_char( ch, FALSE );

        return rCHAR_DIED;
    }

    /*
     * BIG ugly looping problem here when the character is mptransed back to the starting 
     * room. To avoid this, check how many chars are in the room at the start and stop
     * processing followers after doing the right number of them. -Narn
     */
    if ( !fall ) {
        CHAR_DATA              *fch;
        CHAR_DATA              *nextinroom;
        int                     chars = 0,
            count = 0;

        for ( fch = from_room->first_person; fch; fch = fch->next_in_room )
            chars++;

        for ( fch = from_room->first_person; fch && ( count < chars ); fch = nextinroom ) {
            nextinroom = fch->next_in_room;
            count++;
            if ( fch != ch && fch->master == ch && !xIS_SET( fch->affected_by, AFF_GRAZE ) ) {
                if ( fch->position == POS_STANDING ) {
                    if ( !xIS_SET( fch->act, PLR_BOAT ) )
                        act( AT_ACTION, "Sigues a $N.", fch, NULL, ch, TO_CHAR );
                    move_char( fch, pexit, 0, running );
                }
                else if ( fch->position == POS_MOUNTED ) {
                    act( AT_ACTION, "Haces que tu montura siga a $N.", fch, NULL, ch, TO_CHAR );
                    move_char( fch, pexit, 0, running );
                }
                else
                    send_to_char( "¡Levántate!", fch );
            }
        }
    }

    if ( ch->in_room->first_content )
        retcode = check_room_for_traps( ch, TRAP_ENTER_ROOM );

    if ( retcode != rNONE )
        return retcode;

    if ( char_died( ch ) )
        return retcode;

    /*
     * Really should check mounts in this chunk of code. -Orion
     */
    mprog_entry_trigger( ch );
    if ( char_died( ch ) )
        return retcode;

    rprog_enter_trigger( ch );
    if ( char_died( ch ) )
        return retcode;

    mprog_greet_trigger( ch );
    if ( char_died( ch ) )
        return retcode;

    oprog_greet_trigger( ch );
    if ( char_died( ch ) )
        return retcode;

    if ( !will_fall( ch, fall ) && fall > 0 ) {
        if ( !IS_AFFECTED( ch, AFF_FLOATING )
             || ( ch->mount && !IS_AFFECTED( ch->mount, AFF_FLOATING ) ) ) {
            set_char_color( AT_RED, ch );
            send_to_char( "¡OUCH! ¡Te golpeas contra el suelo!\r\n", ch );
            WAIT_STATE( ch, 10 );
            retcode = damage( ch, ch, 20 * fall, TYPE_UNDEFINED );
        }
        else {
            set_char_color( AT_MAGIC, ch );
            send_to_char( "Flotas sobre el suelo.\r\n", ch );
        }
    }
    return retcode;
}

void do_crouch( CHAR_DATA *ch, char *argument )
{

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "No puedes moverte de tu posición estando bajo tierra.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "No mientras vueles.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "No mientras vueles.\r\n", ch );
        return;
    }

    if ( ch->race == RACE_DRAGON ) {
        send_to_char( "No puedes agacharte.\r\n", ch );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "¿Qué tal si terminas la pelea primero?\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "Baja primero de tu montura.\r\n", ch );
        return;
    }

    if ( ch->position == POS_STANDING ) {
        send_to_char( "Te agachas.\r\n", ch );
        act( AT_ACTION, "$n se agacha.", ch, NULL, ch, TO_ROOM );
        set_position( ch, POS_CROUCH );
//    ch->height = ch->height / 2;
//    ch->hitroll = ch->hitroll / 2;
//    (int) ch->damroll =( double ) ch->damroll / 2;
        return;
    }
    else {
        send_to_char( "Debes estar en pie para agacharte.\r\n", ch );
        return;
    }

    return;
}

void do_crawl( CHAR_DATA *ch, char *argument )
{

    if ( ch->position == POS_CRAWL ) {
        send_to_char( "¡Ya estás gateando!\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "Desentiérrate primero.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "No puedes gatear mientras vuelas.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "No puedes gatear mientras glotas.\r\n", ch );
        return;
    }

    if ( ch->race == RACE_DRAGON ) {
        send_to_char( "Los dragones no pueden gatear.\r\n", ch );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "¿qué tal si terminas el combate antes?\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "Baja primero de tu montura.\r\n", ch );
        return;
    }

    if ( ch->position == POS_STANDING ) {
        send_to_char( "Te inclinas y colocas las manos sobre el suelo, comenzando a gatear.\r\n", ch );
        act( AT_ACTION, "$n se inclina y coloca las manos en el suelo, comenzando a gatear.", ch, NULL, NULL, TO_ROOM );
        set_position( ch, POS_CRAWL );
        return;
    }

    send_to_char( "¡Levántate primero!\r\n", ch );
    return;
}

void do_north( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BOAT ) ) {
        send_to_char
            ( "Estás en un barco, necesitas fijar un curso y luego navegar en esa dirección.\r\n",
              ch );
        return;
    }

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );

        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te ha dado esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N falla al darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }

    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "north" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_NORTH ), 0, FALSE );
            return;
        }
    }
    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_NORTH ), 0, FALSE );
        return;
    }
}

void do_explore( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );

        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }

    }
           // beast master fix

  if ( xIS_SET( ch->act, ACT_BEASTPET ) || xIS_SET( ch->act, ACT_BEASTMELD ))
{
           move_char( ch, get_exit( ch->in_room, DIR_EXPLORE ), 0, FALSE );
           return;
}

    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "explore" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_EXPLORE ), 0, FALSE );
            return;
        }
    }
}

void do_east( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BOAT ) ) {
        send_to_char
            ( "Estás en un barco, necesitas fijar un curso de navegación y luego navegar en esa direción.\r\n",
              ch );
        return;
    }

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }

    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "east" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_EAST ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_EAST ), 0, FALSE );
        return;
    }

}

void do_south( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BOAT ) ) {
        send_to_char
            ( "Estás en un barco, necesitas fijar un curso de navegación y luego navegar en esa direción.\r\n",
              ch );
        return;
    }

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }

        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }
    }

    if ( !IS_NPC( ch ) ) {
        if ( ch->pcdata->invite > 0
             && ( ch->in_room->vnum == 54000 || ch->in_room->vnum == 57000
                  || ch->in_room->vnum == 40166 ) ) {
            char_from_room( ch );
            char_to_room( ch, get_room_index( ch->pcdata->invite ) );
            do_look( ch, ( char * ) "auto" );
            ch->pcdata->invite = 0;
            return;
        }

        if ( ch->pcdata->lair > 0 ) {
            if ( ch->in_room->vnum == 54000 && !str_cmp( ch->pcdata->htown_name, "Ciudad Paleon" ) ) {
                char_from_room( ch );
                char_to_room( ch, get_room_index( ch->pcdata->lair ) );
                do_look( ch, ( char * ) "auto" );
                return;

            }
            if ( ch->in_room->vnum == 57000 && !str_cmp( ch->pcdata->htown_name, "Ciudad Dakar" ) ) {
                char_from_room( ch );
                char_to_room( ch, get_room_index( ch->pcdata->lair ) );
                do_look( ch, ( char * ) "auto" );
                return;

            }
            if ( ch->in_room->vnum == 40166
                 && !str_cmp( ch->pcdata->htown_name, "Ciudad Prohibida" ) ) {
                char_from_room( ch );
                char_to_room( ch, get_room_index( ch->pcdata->lair ) );
                do_look( ch, ( char * ) "auto" );
                return;
            }
        }
    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "south" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_SOUTH ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_SOUTH ), 0, FALSE );
        return;
    }

}

void do_west( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BOAT ) ) {
        send_to_char
            ( "Estás en un barco, necesitas fijar un curso de navegación y luego navegar en esa direción.\r\n",
              ch );
        return;
    }

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }

        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "west" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_WEST ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_WEST ), 0, FALSE );
        return;
    }

}

void do_up( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }
    }

    if ( IN_WILDERNESS( ch ) && IS_AFFECTED( ch, AFF_FLYING ) ) {
        set_char_color( AT_RMNAME, ch );
        send_to_char( "\r\nWithin the Vast Sky\r\n", ch );
        set_char_color( AT_LBLUE, ch );
        send_to_char( "The clear open sky shows the vastness of the ground below,\r\n", ch );
        send_to_char( "and sky around.  Landmarks that cannot be seen on the ground\r\n", ch );
        send_to_char( "are easily viewed from the sky.\r\n", ch );
        set_char_color( AT_EXITS, ch );
        send_to_char( "Exits: up down.\r\n\r\n", ch );
        WAIT_STATE( ch, 5 );
        send_to_char( "\r\n&cYou soar up into the sky.\r\n\r\n", ch );
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "up" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_UP ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_UP ), 0, FALSE );
        return;
    }
}

void do_down( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "down" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_DOWN ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_DOWN ), 0, FALSE );
        return;
    }
}

void do_northeast( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BOAT ) ) {
        send_to_char
            ( "Estás en un barco, necesitas fijar un curso de navegación y luego navegar en esa direción.\r\n",
              ch );
        return;
    }

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "northeast" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_NORTHEAST ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_NORTHEAST ), 0, FALSE );
        return;
    }
}

void do_northwest( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BOAT ) ) {
        send_to_char
            ( "Estás en un barco, necesitas fijar un curso de navegación y luego navegar en esa direción.\r\n",
              ch );
        return;
    }

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "northwest" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_NORTHWEST ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_NORTHWEST ), 0, FALSE );
        return;
    }
}

void do_southeast( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BOAT ) ) {
        send_to_char
            ( "Estás en un barco, necesitas fijar un curso de navegación y luego navegar en esa direción.\r\n",
              ch );
        return;
    }

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }
    }

    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "southeast" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_SOUTHEAST ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_SOUTHEAST ), 0, FALSE );
        return;
    }
}

void do_southwest( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    short                   chance;

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BOAT ) ) {
        send_to_char
            ( "Estás en un barco, necesitas fijar un curso de navegación y luego navegar en esa direción.\r\n",
              ch );
        return;
    }

    if ( ( arg[0] != '\0' ) && ( !str_cmp( arg, "eludir" ) ) && ( arg2[0] != '\0' ) ) {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( !victim->master ) {
            send_to_char( "Nadie te está siguiendo.\r\n", ch );
            return;
        }

        chance = number_range( 1, 10 );
        if ( chance > 7 ) {
            act( AT_PLAIN, "¡Le das esquinazo a tu seguidor!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "$n te da esquinazo!", ch, NULL,
                 victim, TO_VICT );
            stop_follower( victim );
        }
        else if ( chance <= 7 ) {
            act( AT_PLAIN, "$N trata de darte esquinazo!", ch, NULL,
                 victim, TO_VICT );
        }
    }
    if ( !IS_NPC( ch ) ) {
        if ( IS_IMMORTAL( ch ) && IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            do_build_walk( ch, ( char * ) "southwest" );
            return;
        }

        if ( !IS_SET( ch->pcdata->flags, PCFLAG_BUILDWALK ) ) {
            move_char( ch, get_exit( ch->in_room, DIR_SOUTHWEST ), 0, FALSE );
            return;
        }
    }

    if ( IS_NPC( ch ) ) {
        move_char( ch, get_exit( ch->in_room, DIR_SOUTHWEST ), 0, FALSE );
        return;
    }
}

EXIT_DATA              *find_door( CHAR_DATA *ch, char *arg, bool quiet )
{
    EXIT_DATA              *pexit;
    int                     door;

    if ( arg == NULL || !str_cmp( arg, "" ) )
        return NULL;

    pexit = NULL;
    if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) || !str_cmp( arg, "norte" ) )
        door = 0;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) || !str_cmp( arg, "este" ))
        door = 1;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) || !str_cmp( arg, "sur" ) )
        door = 2;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) || !str_cmp( arg, "o" ) || !str_cmp( arg, "oeste" ))
        door = 3;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) || !str_cmp( arg, "ar" ) || !str_cmp( arg, "arriba" ))
        door = 4;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) || !str_cmp( arg, "ab" ) || !str_cmp( arg, "abajo" ) )
        door = 5;
    else if ( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) || !str_cmp( arg, "noreste" ) )
        door = 6;
    else if ( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) || !str_cmp( arg, "no" ) || !str_cmp( arg, "noroeste" ) )
        door = 7;
    else if ( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) || !str_cmp( arg, "sureste" ) )
        door = 8;
    else if ( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) || !str_cmp( arg, "so" ) || !str_cmp( arg, "suroeste" ) )
        door = 9;
    else {
        for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next ) {
            if ( ( quiet || IS_SET( pexit->exit_info, EX_ISDOOR ) ) && pexit->keyword
                 && nifty_is_name_prefix( arg, pexit->keyword ) )
                return pexit;
        }
        if ( !quiet )
            act( AT_PLAIN, "No ves $T aquí.", ch, NULL, arg, TO_CHAR );
        return NULL;
    }

    if ( ( pexit = get_exit( ch->in_room, door ) ) == NULL ) {
        if ( !quiet )
            act( AT_PLAIN, "No ves $T aquí.", ch, NULL, arg, TO_CHAR );
        return NULL;
    }

    if ( quiet )
        return pexit;

    if ( IS_SET( pexit->exit_info, EX_SECRET ) ) {
        act( AT_PLAIN, "No ves $T aquí.", ch, NULL, arg, TO_CHAR );
        return NULL;
    }

    if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
        send_to_char( "No puedes hacer eso.\r\n", ch );
        return NULL;
    }

    return pexit;
}

void toggle_bexit_flag( EXIT_DATA *pexit, int flag )
{
    EXIT_DATA              *pexit_rev;

    TOGGLE_BIT( pexit->exit_info, flag );
    if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev != pexit )
        TOGGLE_BIT( pexit_rev->exit_info, flag );
}

void do_open( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    EXIT_DATA              *pexit;
    int                     door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Abrir qué?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        /*
         * 'open door' 
         */
        EXIT_DATA              *pexit_rev;

        if ( ch->level < ( MAX_LEVEL - 4 ) ) {
            if ( IS_SET( pexit->exit_info, EX_SECRET ) && pexit->keyword
                 && !nifty_is_name( arg, pexit->keyword ) ) {
                ch_printf( ch, "No ves %s aquí.\r\n", arg );
                return;
            }
            if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
                send_to_char( "No puedes hacer eso.\r\n", ch );
                return;
            }
            if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
                send_to_char( "Pero si ya está...\r\n", ch );
                return;
            }
            if ( IS_SET( pexit->exit_info, EX_LOCKED ) && IS_SET( pexit->exit_info, EX_BOLTED ) ) {
                send_to_char( "Debes abrir la cerradura primero, y está atornillada.\r\n", ch );
                return;
            }
            if ( IS_SET( pexit->exit_info, EX_BOLTED ) ) {
                send_to_char( "Está atornillada.\r\n", ch );
                return;
            }
            if ( IS_SET( pexit->exit_info, EX_LOCKED ) ) {
                send_to_char( "Debes abrir primero la cerradura.\r\n", ch );
                return;
            }
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name_prefix( arg, pexit->keyword ) ) ) {
            act( AT_ACTION, "$n abre la $d.", ch, NULL, pexit->keyword, TO_ROOM );
            act( AT_ACTION, "Abres la $d.", ch, NULL, pexit->keyword, TO_CHAR );
            if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room ) {
                CHAR_DATA              *rch;

                for ( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
                    act( AT_ACTION, "La $d se abre.", rch, NULL, pexit_rev->keyword, TO_CHAR );
            }
            remove_bexit_flag( pexit, EX_CLOSED );
            if ( ( door = pexit->vdir ) >= 0 && door < 10 )
                check_room_for_traps( ch, trap_door[door] );
            return;
        }
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'open object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            ch_printf( ch, "%s no es un contenedor.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) ) {
            ch_printf( ch, "%s no necesita que hagas eso.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSEABLE ) ) {
            ch_printf( ch, "%s ni se puede abrir ni se puede cerrar.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_LOCKED ) ) {
            ch_printf( ch, "%s tiene una cerradura que deberías abrir primero.\r\n", capitalize( obj->short_descr ) );
            return;
        }

        REMOVE_BIT( obj->value[1], CONT_CLOSED );
        act( AT_ACTION, "Abres $p.", ch, obj, NULL, TO_CHAR );
        act( AT_ACTION, "$n abre $p.", ch, obj, NULL, TO_ROOM );
        check_for_trap( ch, obj, TRAP_OPEN );
        return;
    }

    ch_printf( ch, "No ves %s aquí.\r\n", arg );
    return;
}

void do_close( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    EXIT_DATA              *pexit;
    int                     door;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Cerrar qué?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        /*
         * 'close door' 
         */
        EXIT_DATA              *pexit_rev;

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "No puedes hacer eso.\r\n", ch );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "Pero si ya está...\r\n", ch );
            return;
        }

        act( AT_ACTION, "$n cierra $d.", ch, NULL, pexit->keyword, TO_ROOM );
        act( AT_ACTION, "Cierras $d.", ch, NULL, pexit->keyword, TO_CHAR );

        /*
         * close the other side 
         */
        if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room ) {
            CHAR_DATA              *rch;

            SET_BIT( pexit_rev->exit_info, EX_CLOSED );
            for ( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
                act( AT_ACTION, "$d se cierra.", rch, NULL, pexit_rev->keyword, TO_CHAR );
        }
        set_bexit_flag( pexit, EX_CLOSED );
        if ( ( door = pexit->vdir ) >= 0 && door < 10 )
            check_room_for_traps( ch, trap_door[door] );
        return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'close object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            ch_printf( ch, "%s no es un contenedor.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_CLOSED ) ) {
            ch_printf( ch, "%s no necesita que hagas eso.\r\n", capitalize( obj->short_descr ) );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSEABLE ) ) {
            ch_printf( ch, "%s ni se puede abrir ni se puede cerrar.\r\n", capitalize( obj->short_descr ) );
            return;
        }

        SET_BIT( obj->value[1], CONT_CLOSED );
        act( AT_ACTION, "Cierras $p.", ch, obj, NULL, TO_CHAR );
        act( AT_ACTION, "$n cierra $p.", ch, obj, NULL, TO_ROOM );
        check_for_trap( ch, obj, TRAP_CLOSE );
        return;
    }

    ch_printf( ch, "No ves %s aquí.\r\n", arg );
    return;
}

/*
 * Keyring support added by Thoric
 * Idea suggested by Onyx <MtRicmer@worldnet.att.net> of Eldarion
 *
 * New: returns pointer to key/NULL instead of TRUE/FALSE
 *
 * If you want a feature like having immortals always have a key... you'll
 * need to code in a generic key, and make sure extract_obj doesn't extract it
 */
OBJ_DATA               *has_key( CHAR_DATA *ch, int key )
{
    OBJ_DATA               *obj,
                           *obj2;

    for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
        if ( obj->pIndexData->vnum == key
             || ( obj->item_type == ITEM_KEY && obj->value[0] == key ) )
            return obj;
        else if ( obj->item_type == ITEM_KEYRING )
            for ( obj2 = obj->first_content; obj2; obj2 = obj2->next_content )
                if ( obj2->pIndexData->vnum == key || obj2->value[0] == key )
                    return obj2;
    }

    return NULL;
}

void do_lock( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj,
                           *key;
    EXIT_DATA              *pexit;
    int                     count;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Echar la llave a qué?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        /*
         * 'lock door' 
         */

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "No puedes hacer eso.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "Si lo haces, luego no se podría cerrar.\r\n", ch );
            return;
        }
        if ( pexit->key < 0 ) {
            send_to_char( "No tiene cerradura.\r\n", ch );
            return;
        }
        if ( ( key = has_key( ch, pexit->key ) ) == NULL ) {
            send_to_char( "¿y la llave?\r\n", ch );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_LOCKED ) ) {
            send_to_char( "Resulta imposible echar una cerradura que ya está echada.\r\n", ch );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) ) {
            send_to_char( "*Click*\r\n", ch );
            count = key->count;
            key->count = 1;
            act( AT_ACTION, "$n echa la llave a $d con $p.", ch, key, pexit->keyword, TO_ROOM );
            key->count = count;
            set_bexit_flag( pexit, EX_LOCKED );
            return;
        }
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'lock object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            send_to_char( "Pero si no es un contenedor...\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) ) {
            send_to_char( "Si haces eso, luego no se podrá cerrar.\r\n", ch );
            return;
        }
        if ( obj->value[2] < 0 ) {
            send_to_char( "Resulta difícil echar la llave a algo que no tiene ninguna cerradura.\r\n", ch );
            return;
        }
        if ( ( key = has_key( ch, obj->value[2] ) ) == NULL ) {
            send_to_char( "¿Y la llave?\r\n", ch );
            return;
        }
        if ( IS_SET( obj->value[1], CONT_LOCKED ) ) {
            send_to_char( "Es imposible echar la llave a algo que ya tiene la llave echada.\r\n", ch );
            return;
        }

        SET_BIT( obj->value[1], CONT_LOCKED );
        send_to_char( "*Click*\r\n", ch );
        count = key->count;
        key->count = 1;
        act( AT_ACTION, "$n echa la llave a $p con $P.", ch, obj, key, TO_ROOM );
        key->count = count;
        return;
    }

    ch_printf( ch, "No ves %s aquí.\r\n", arg );
    return;
}

void do_unlock( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj,
                           *key;
    EXIT_DATA              *pexit;
    int                     count;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Abrir la cerradura de qué?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {
        /*
         * 'unlock door' 
         */

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "No puedes hacer eso.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "Resulta un tanto complicado abrir la cerradura de algo que ni siquiera está cerrado.\r\n", ch );
            return;
        }
        if ( pexit->key < 0 ) {
            send_to_char( "Es complejo abrir la cerradura de algo que no tiene cerradura.\r\n", ch );
            return;
        }
        if ( ( key = has_key( ch, pexit->key ) ) == NULL ) {
            send_to_char( "¿Y la llave?\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_LOCKED ) ) {
            send_to_char( "Pero si ya está...\r\n", ch );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) ) {
            send_to_char( "*Click*\r\n", ch );
            count = key->count;
            key->count = 1;
            act( AT_ACTION, "$n abre la cerradura de $d con $p.", ch, key, pexit->keyword, TO_ROOM );
            key->count = count;
            if ( IS_SET( pexit->exit_info, EX_EATKEY ) ) {
                separate_obj( key );
                extract_obj( key );
            }
            remove_bexit_flag( pexit, EX_LOCKED );
            return;
        }
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL ) {
        /*
         * 'unlock object' 
         */
        if ( obj->item_type != ITEM_CONTAINER ) {
            send_to_char( "Eso no es un contenedor.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_CLOSED ) ) {
            send_to_char( "Resulta complejo abrir la cerradura de una cosa que está abierta.\r\n", ch );
            return;
        }
        if ( obj->value[2] < 0 ) {
            send_to_char( "Es difícil abrir la cerradura de algo que no tiene cerradura, ¿verdad?.\r\n", ch );
            return;
        }
        if ( ( key = has_key( ch, obj->value[2] ) ) == NULL ) {
            send_to_char( "¿Y la llave?\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[1], CONT_LOCKED ) ) {
            send_to_char( "Pero si ya está...\r\n", ch );
            return;
        }

        REMOVE_BIT( obj->value[1], CONT_LOCKED );
        send_to_char( "*Click*\r\n", ch );
        count = key->count;
        key->count = 1;
        act( AT_ACTION, "$n abre la cerradura de $p con $P.", ch, obj, key, TO_ROOM );
        key->count = count;
        if ( IS_SET( obj->value[1], CONT_EATKEY ) ) {
            separate_obj( key );
            extract_obj( key );
        }
        return;
    }

    ch_printf( ch, "No ves %s aquí.\r\n", arg );
    return;
}

void do_bashdoor( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    char                    arg[MIL];

    /*
     * Adjusted check to make it more multi-class friendly.  -Taon 
     */
    if ( !IS_NPC( ch ) && ch->pcdata->learned[gsn_bashdoor] <= 0 ) {
        send_to_char( "¡No eres guerrero suficiente para echar puertas abajo!\r\n", ch );
        return;
    }
    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Derribar qué?\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "Termina de pelear primero.\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, FALSE ) ) != NULL ) {
        ROOM_INDEX_DATA        *to_room;
        EXIT_DATA              *pexit_rev;
        int                     chance;
        const char             *keyword;

        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "&GCálmate. la salida está abierta.\r\n", ch );
            return;
        }

        WAIT_STATE( ch, skill_table[gsn_bashdoor]->beats );

        if ( IS_SET( pexit->exit_info, EX_SECRET ) )
            keyword = "pared";
        else
            keyword = pexit->keyword;
        if ( !IS_NPC( ch ) )
            chance = LEARNED( ch, gsn_bashdoor ) / 2;
        else
            chance = 90;
        if ( IS_SET( pexit->exit_info, EX_LOCKED ) )
            chance /= 3;

        if ( IS_SET( pexit->exit_info, EX_BASHPROOF ) ) {
            act( AT_GREEN,
                 "WHAAM!  Te lanzas hacia $d para nada!",
                 ch, NULL, keyword, TO_CHAR );
            act( AT_GREEN, "$n se lanza hacia $d!", ch, NULL, keyword, TO_ROOM );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_BASHPROOF ) && ch->move >= 15
             && number_percent(  ) < ( chance + 4 * ( get_curr_str( ch ) - 16 ) ) ) {
            REMOVE_BIT( pexit->exit_info, EX_CLOSED );
            if ( IS_SET( pexit->exit_info, EX_LOCKED ) )
                REMOVE_BIT( pexit->exit_info, EX_LOCKED );
            SET_BIT( pexit->exit_info, EX_BASHED );

            act( AT_GREEN, "Crash!  ¡Derribas $d!", ch, NULL, keyword, TO_CHAR );
            act( AT_GREEN, "¡$n derriba $d!", ch, NULL, keyword, TO_ROOM );
            learn_from_success( ch, gsn_bashdoor );

            if ( ( to_room = pexit->to_room ) != NULL && ( pexit_rev = pexit->rexit ) != NULL
                 && pexit_rev->to_room == ch->in_room ) {
                CHAR_DATA              *rch;

                REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
                if ( IS_SET( pexit_rev->exit_info, EX_LOCKED ) )
                    REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
                SET_BIT( pexit_rev->exit_info, EX_BASHED );

                for ( rch = to_room->first_person; rch; rch = rch->next_in_room ) {
                    act( AT_SKILL, "¡$d se cae!", rch, NULL, pexit_rev->keyword, TO_CHAR );
                }
            }
            damage( ch, ch, ( ch->max_hit / 20 ), gsn_bashdoor );

        }
        else {
            act( AT_GREEN, "WHAAAAM!!!  Te lanzas sobre $d, pero solo consigues hacerte daño.", ch, NULL,
                 keyword, TO_CHAR );
            act( AT_GREEN, "WHAAAAM!!!  $n se lanza contra $d, pero solo consigue hacerse daño.", ch, NULL,
                 keyword, TO_ROOM );
            damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
        }
    }
    else {
        act( AT_GREEN, "WHAAAAM!!!  te lanzas contra la pared, pero esta permanece en su sitio.", ch, NULL,
             NULL, TO_CHAR );
        act( AT_GREEN, "WHAAAAM!!!  $n se lanza contra una pared, como si pudiera derribarla...", ch, NULL,
             NULL, TO_ROOM );
        damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
    }
    return;
}

void do_stand( CHAR_DATA *ch, char *argument )
{

    OBJ_DATA               *obj = NULL;
    int                     aon = 0;
    CHAR_DATA              *fch = NULL;
    int                     val0;
    int                     val1;
    ROOM_INDEX_DATA        *in_room;
    ch_ret                  retcode;

    in_room = ch->in_room;

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "Desentiérrate primero.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FEIGN ) ) {
        send_to_char( "Dejas de fingir tu muerte y te pones en pie.\r\n", ch );
        act( AT_ACTION, "$n se pone de pie, dejando de fingir su muerte.", ch, NULL, ch, TO_ROOM );
        set_position( ch, POS_STANDING );
        affect_strip( ch, gsn_feign_death );
        xREMOVE_BIT( ch->affected_by, AFF_FEIGN );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "¿y si terminas de pelear primero?\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "Deberías bajar de tu montura.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_UNHOLY_SPHERE ) ) {
        act( AT_BLOOD, "Dejas de descansar.", ch, NULL, NULL,
             TO_CHAR );
        act( AT_BLOOD, "$n deja de descansar.", ch, NULL, NULL, TO_ROOM );
        affect_strip( ch, gsn_unholy_sphere );
        xREMOVE_BIT( ch->affected_by, AFF_UNHOLY_SPHERE );
        return;
    }

    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' ) {
        obj = get_obj_list( ch, argument, ch->in_room->first_content );
        if ( obj == NULL ) {
            send_to_char( "No ves eso aquí.\r\n", ch );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE ) {
            send_to_char( "No puedes.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[2], STAND_ON ) && !IS_SET( obj->value[2], STAND_IN )
             && !IS_SET( obj->value[2], STAND_AT ) ) {
            send_to_char( "No puedes subirte a eso.\r\n", ch );
            return;
        }
        if ( obj->value[0] == 0 )
            val0 = 1;
        else
            val0 = obj->value[0];
        if ( ch->on != obj && count_users( obj ) >= val0 ) {
            act( AT_ACTION, "No hay sala para subirse a $p.", ch, obj, NULL, TO_CHAR );
            return;
        }
        if ( ch->on == obj )
            aon = 1;
        else
            ch->on = obj;
    }

    switch ( ch->position ) {
        case POS_MEDITATING:
            send_to_char( "Dejas de meditar y te levantas.\r\n", ch );
            act( AT_ACTION, "$n deja de meditar y se levanta.", ch, NULL, NULL,
                 TO_ROOM );
            set_position( ch, POS_STANDING );
            break;

        case POS_CRAWL:
            if ( in_room->height > 0 ) {
                if ( ch->height > in_room->height ) {
                    send_to_char( "¡Te das con la cabeza en el techo al intentar levantarte!\r\n",
                                  ch );
                    send_to_char( "&R¡Ouch, eso ha dolido!\r\n", ch );
                    retcode = damage( ch, ch, 2, TYPE_UNDEFINED );
                    return;
                }
            }
            send_to_char( "Te levantas.\r\n", ch );
            act( AT_ACTION, "$n se levanta.", ch, NULL, NULL,
                 TO_ROOM );
            set_position( ch, POS_STANDING );
            break;

        case POS_SLEEPING:
            if ( IS_AFFECTED( ch, AFF_SLEEP ) ) {
                send_to_char( "¡No puedes despertar!\r\n", ch );
                return;
            }

            if ( obj == NULL ) {
                send_to_char( "Te despiertas y te pones en pie.\r\n", ch );
                act( AT_ACTION, "$n se despierta y se pone de pie.", ch, NULL, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else if ( IS_SET( obj->value[2], STAND_AT ) ) {
                act( AT_ACTION, "Te despiertas y te pones en pie sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se despierta y se pone en pie sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], STAND_ON ) ) {
                act( AT_ACTION, "Te despiertas y te subes a $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se despierta y se sube a $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Te despiertas y te subes a $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se despierta y se sube a $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_STANDING );
            do_look( ch, ( char * ) "auto" );
            break;
        case POS_RESTING:
        case POS_SITTING:
            if ( obj == NULL ) {
                send_to_char( "Te levantas.\r\n", ch );
                act( AT_ACTION, "$n se levanta.", ch, NULL, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else if ( IS_SET( obj->value[2], STAND_AT ) ) {
                act( AT_ACTION, "Te pones en pie sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se pone de pie sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], STAND_ON ) ) {
                act( AT_ACTION, "Te subes encima de $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sube encima de $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Te levantas y te subes encima de $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sube encima de $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_STANDING );
            break;
        case POS_STANDING:
            if ( obj != NULL && aon != 1 ) {

                if ( IS_SET( obj->value[2], STAND_AT ) ) {
                    act( AT_ACTION, "Te levantas sobre $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se levanta sobre $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], STAND_ON ) ) {
                    act( AT_ACTION, "Te subes a $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se sube a $p.", ch, obj, NULL, TO_ROOM );
                }
                else {
                    act( AT_ACTION, "Te subes en $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se sube a $p.", ch, obj, NULL, TO_ROOM );
                }
            }
            else if ( aon == 1 ) {
                act( AT_ACTION, "ya usas $p.", ch, obj, NULL, TO_CHAR );
            }
            else if ( ch->on != NULL && obj == NULL ) {
                act( AT_ACTION, "Te bajas de $p.", ch, ch->on, NULL,
                     TO_CHAR );
                act( AT_ACTION, "$n se baja de $p.", ch, ch->on, NULL,
                     TO_ROOM );
                ch->on = NULL;
            }
            else
                send_to_char( "ya estás de pie.\r\n", ch );
            break;

    }
    if ( obj != NULL ) {
        if ( obj->value[1] == 0 )
            val1 = 750;
        else
            val1 = obj->value[1];
        if ( max_weight( obj ) > val1 ) {
            act( AT_ACTION, "El peso de $n fue demasiado para $p.", ch, ch->on, NULL,
                 TO_ROOM );
            act( AT_ACTION, "Tu intento de sentarse sobre $p causa su ruptura.", ch, ch->on, NULL,
                 TO_CHAR );
            for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room ) {
                if ( fch->on == obj ) {
                    if ( fch->position == POS_RESTING ) {
                        fch->hit = ( fch->hit - 30 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION,
                             "Tu intento de descanso se ve interrumpido cuando $p se rompe.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SLEEPING ) {
                        fch->hit = ( fch->hit - 40 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        set_position( fch, POS_RESTING );
                        act( AT_ACTION,
                             "Tu intento de dormir es interrumpido cuando $p se rompe.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SITTING ) {
                        fch->hit = ( fch->hit - 5 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Tu descanso es interrumpido cuando $p se rompe.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_STANDING ) {
                        fch->hit = ( fch->hit - 55 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Te caes al suelo cuando $p se rompe.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    fch->on = NULL;
                }
            }
            make_scraps( obj );
        }
    }

    return;
}

void do_trance( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "Desentiérrate primero.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "No puedes entrar en trance mientras vuelas.\r\n", ch );
        return;
    }

    if ( ch->mana >= ch->max_mana ) {
        send_to_char( "No lo necesitas.\r\n", ch );
        return;
    }

    if ( ch->position != POS_MEDITATING ) {
        send_to_char( "No puedes entrar en trance si no estás en una posición de meditación.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_trance ) ) {
        if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 ) {
            set_char_color( AT_RED, ch );
            send_to_char
                ( "¡Tu adrenalina te impide entrar en trance!\r\n&cYou are able to calm yourself enough for the lesser trance.\r\n",
                  ch );
        }

        if ( ch->mana <= ch->max_mana / 2 && get_timer( ch, TIMER_RECENTFIGHT ) <= 0 ) {
            WAIT_STATE( ch, ( 16 ) );
            act( AT_CYAN, "Entras en un profundo trance aprovechando la meditación.", ch, NULL, NULL,
                 TO_CHAR );
            act( AT_CYAN, "$n parece relajarse y entrar en un profundo trance.", ch, NULL, NULL, TO_ROOM );
            ch->mana = ch->mana + ( ch->max_mana / 6 );
            if ( ch->mana > ch->max_mana ) {
                ch->mana = ch->max_mana / 2 + number_range( 1, 20 );
            }
        }
        else {
            if ( get_timer( ch, TIMER_RECENTFIGHT ) <= 0 ) {
                send_to_char
                    ( "&cTienes demasiado mana para entrar en trance profundo, así pues entras en un trance menor.\r\n",
                      ch );
            }
            if ( ch->mana <= ch->max_mana - 100 ) {
                WAIT_STATE( ch, ( 32 ) );
                ch->mana += 50;
            }
            return;
        }
        learn_from_success( ch, gsn_trance );
        return;
    }
    else {
        send_to_char( "&cNo puedes concentrarte lo suficiente.\r\n", ch );
        learn_from_failure( ch, gsn_trance );
    }
    return;

}

void do_meditate( CHAR_DATA *ch, char *argument )
{
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "Desentiérrate primero.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "No puedes meditar mientras vuelas.\r\n", ch );
        return;
    }

    if ( ch->mana >= ch->max_mana ) {
        send_to_char( "No necesitas meditar.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_meditate ) ) {
        switch ( ch->position ) {
            case POS_SLEEPING:
                if ( IS_AFFECTED( ch, AFF_SLEEP ) ) {
                    send_to_char( "¡No puedes despertar!\r\n", ch );
                    return;
                }

                send_to_char( "Despiertas y comienzas a meditar.\r\n", ch );
                act( AT_ACTION,
                     "$n despierta y rueda los ojoss antes de entrar en una meditación.", ch, NULL,
                     NULL, TO_ROOM );
                set_position( ch, POS_MEDITATING );
                break;

            case POS_RESTING:
                send_to_char( "Dejas de descansar y comienzas a meditar.\r\n", ch );
                act( AT_ACTION,
                     "$n deja de descansar, rueda los ojjos y se pone a meditar.", ch,
                     NULL, NULL, TO_ROOM );
                set_position( ch, POS_MEDITATING );
                break;

            case POS_STANDING:
                send_to_char( "Comienzas a meditar.\r\n", ch );
                act( AT_ACTION, "$n rueda los ojos y comienza a meditar.", ch,
                     NULL, NULL, TO_ROOM );
                set_position( ch, POS_MEDITATING );
                break;

            case POS_SITTING:
                send_to_char( "Te levantas y comienzas a meditar.\r\n", ch );
                act( AT_ACTION,
                     "$n rueda los ojos, se levanta y comienza a meditar.", ch,
                     NULL, NULL, TO_ROOM );
                set_position( ch, POS_MEDITATING );
                learn_from_success( ch, gsn_meditate );
                return;

            case POS_MEDITATING:
                send_to_char( "Ya estás meditando.\r\n", ch );
                return;

            case POS_FIGHTING:
            case POS_EVASIVE:
            case POS_DEFENSIVE:
            case POS_AGGRESSIVE:
            case POS_BERSERK:
                send_to_char( "¡Imposible! ¡Estás peleando!\r\n", ch );
                return;

            case POS_MOUNTED:
                send_to_char( "Desmonta primero.\r\n", ch );
                return;
        }
    }
    else {
        if ( ch->position == POS_MEDITATING )
            send_to_char( "¡Ya estás meditando!\r\n", ch );
        else {
            send_to_char( "No puedes concentrarte.\r\n", ch );
            learn_from_failure( ch, gsn_meditate );
        }
    }
    return;

}

void do_sit( CHAR_DATA *ch, char *argument )
{

    OBJ_DATA               *obj = NULL;
    int                     aon = 0;
    CHAR_DATA              *fch = NULL;
    int                     val0;
    int                     val1;

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "Desentiérrate primero.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FEIGN ) ) {
        send_to_char( "Dejas de fingir tu muerte y te sientas.\r\n", ch );
        act( AT_ACTION, "$n deja de fingir su muerte y se sienta.", ch, NULL, ch, TO_ROOM );
        set_position( ch, POS_SITTING );
        affect_strip( ch, gsn_feign_death );
        xREMOVE_BIT( ch->affected_by, AFF_FEIGN );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "¿Qué tal si terminas tu pelea antes?\r\n", ch );
        return;
    }
    if ( ch->position == POS_MOUNTED ) {
        send_to_char( "Baja de tu montura primero.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "Deberías aterrizar primero.\r\n", ch );
        return;
    }

    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' ) {
        obj = get_obj_list( ch, argument, ch->in_room->first_content );
        if ( obj == NULL ) {
            send_to_char( "No ves eso aquí.\r\n", ch );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE ) {
            send_to_char( "No puedes.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[2], SIT_ON ) && !IS_SET( obj->value[2], SIT_IN )
             && !IS_SET( obj->value[2], SIT_AT ) ) {
            send_to_char( "No puedes sentarte sobre eso.\r\n", ch );
            return;
        }
        if ( obj->value[0] == 0 )
            val0 = 1;
        else
            val0 = obj->value[0];
        if ( ch->on != obj && count_users( obj ) >= val0 ) {
            act( AT_ACTION, "No hay sala para sentarse en $p.", ch, obj, NULL, TO_CHAR );
            return;
        }
        if ( ch->on == obj )
            aon = 1;
        else
            ch->on = obj;
    }

    switch ( ch->position ) {
        case POS_MEDITATING:
            send_to_char( "Dejas de meditar y te sientas.\r\n", ch );
            act( AT_ACTION, "$n deja de meditar y se sienta.", ch, NULL, NULL, TO_ROOM );
            set_position( ch, POS_SITTING );
            break;
        case POS_SLEEPING:
            if ( obj == NULL ) {
                send_to_char( "Despiertas y te sientas.\r\n", ch );
                act( AT_ACTION, "$n se despierta y se sienta.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_AT ) ) {
                act( AT_ACTION, "Despiertas y te sientas sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n despierta y se sienta sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_ON ) ) {
                act( AT_ACTION, "Despiertas y te sientas en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se despierta y se sienta en $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Despiertas y te sientas en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se despierta y se sienta en $p.", ch, obj, NULL, TO_ROOM );
            }

            set_position( ch, POS_SITTING );
            break;
        case POS_RESTING:
            if ( obj == NULL )
                send_to_char( "Dejas de descansar.\r\n", ch );
            else if ( IS_SET( obj->value[2], SIT_AT ) ) {
                act( AT_ACTION, "Te sientas sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sienta sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_ON ) ) {
                act( AT_ACTION, "Te sientas encima de $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sienta encima de $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SITTING );
            break;

        case POS_CRAWL:
            send_to_char( "Dejas de gatear y te sientas.\r\n", ch );
            act( AT_ACTION, "$n deja de gatear y se sienta.", ch, NULL, NULL, TO_ROOM );
            set_position( ch, POS_SITTING );
            break;

        case POS_SITTING:
            if ( obj != NULL && aon != 1 ) {

                if ( IS_SET( obj->value[2], SIT_AT ) ) {
                    act( AT_ACTION, "Te sientas sobre $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se sienta sobre $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], STAND_ON ) ) {
                    act( AT_ACTION, "Te sientas en $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se sienta en $p.", ch, obj, NULL, TO_ROOM );
                }
                else {
                    act( AT_ACTION, "Te sientas en $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se sienta en $p.", ch, obj, NULL, TO_ROOM );
                }
            }
            else if ( aon == 1 ) {
                act( AT_ACTION, "Ya estás usando $p.", ch, obj, NULL, TO_CHAR );
            }
            else if ( ch->on != NULL && obj == NULL ) {
                act( AT_ACTION, "Te bajas de $p y te sientas en el suelo.", ch, ch->on, NULL,
                     TO_CHAR );
                act( AT_ACTION, "$n se baja de $p y se sienta en el suelo.", ch, ch->on, NULL,
                     TO_ROOM );
                ch->on = NULL;
            }
            else
                send_to_char( "Ya estás.\r\n", ch );
            break;
        case POS_STANDING:
            if ( obj == NULL ) {
                send_to_char( "Te sientas.\r\n", ch );
                act( AT_ACTION, "$n se sienta en el suelo.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_AT ) ) {
                act( AT_ACTION, "Te sientas sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sienta sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SIT_ON ) ) {
                act( AT_ACTION, "Te sientas en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sienta en $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Te sientas en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sienta en $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SITTING );
            break;
    }
    if ( obj != NULL ) {
        if ( obj->value[1] == 0 )
            val1 = 750;
        else
            val1 = obj->value[1];
        if ( max_weight( obj ) > val1 ) {
            act( AT_ACTION, "El peso de $n es demasiado para $p.", ch, ch->on, NULL,
                 TO_ROOM );
            act( AT_ACTION, "$p se rompe debido a tu peso.", ch, ch->on, NULL,
                 TO_CHAR );
            for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room ) {
                if ( fch->on == obj ) {
                    if ( fch->position == POS_RESTING ) {
                        fch->hit = ( fch->hit - 30 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION,
                             "Tu descanso se interrumpe cuando caes al suelo tras romperse $p.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SLEEPING ) {
                        fch->hit = ( fch->hit - 40 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        set_position( fch, POS_RESTING );
                        act( AT_ACTION,
                             "Tu sueño se interrumpe cuando $p se rompe.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SITTING ) {
                        fch->hit = ( fch->hit - 5 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Caes al suelo cuando $p se rompe.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_STANDING ) {
                        fch->hit = ( fch->hit - 55 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Sufres un duro golpe cuando $p se rompe.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    fch->on = NULL;
                }
            }
            make_scraps( obj );
        }
    }

    return;
}

void do_rest( CHAR_DATA *ch, char *argument )
{

    OBJ_DATA               *obj = NULL;
    int                     aon = 0;
    CHAR_DATA              *fch = NULL;
    int                     val0;
    int                     val1;

    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position != POS_FIGHTING ) {
        send_to_char( "Desentiérrate primero.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) && ch->position == POS_FIGHTING ) {
        xREMOVE_BIT( ch->affected_by, AFF_BURROW );
    }

    if ( IS_AFFECTED( ch, AFF_FEIGN ) ) {
        send_to_char( "Dejas de fingir tu muerte y te pones a descansar.\r\n", ch );
        act( AT_ACTION, "$n deja de fingir su muerte y se pone a descansar.", ch, NULL, ch, TO_ROOM );
        set_position( ch, POS_RESTING );
        affect_strip( ch, gsn_feign_death );
        xREMOVE_BIT( ch->affected_by, AFF_FEIGN );
        return;
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "¿Y si terminas tu pelea primero?\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "Aterriza primero.\r\n", ch );
        return;
    }

    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' ) {
        obj = get_obj_list( ch, argument, ch->in_room->first_content );
        if ( obj == NULL ) {
            send_to_char( "No ves eso aquí.\r\n", ch );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE ) {
            send_to_char( "No puedes.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[2], REST_ON ) && !IS_SET( obj->value[2], REST_IN )
             && !IS_SET( obj->value[2], REST_AT ) ) {
            send_to_char( "No puedes descansar ahí.\r\n", ch );
            return;
        }
        if ( obj->value[0] == 0 )
            val0 = 1;
        else
            val0 = obj->value[0];
        if ( ch->on != obj && count_users( obj ) >= val0 ) {
            act( AT_ACTION, "No hay sala para descansar en $p.", ch, obj, NULL, TO_CHAR );
            return;
        }
        if ( ch->on == obj )
            aon = 1;
        else
            ch->on = obj;
    }

    switch ( ch->position ) {
        case POS_MEDITATING:
            send_to_char( "Dejas de meditar y te pones a descansar.\r\n", ch );
            act( AT_ACTION, "$n deja de meditar y se pone a descansar.", ch, NULL, NULL,
                 TO_ROOM );
            set_position( ch, POS_RESTING );
            break;
        case POS_SLEEPING:
            if ( obj == NULL ) {
                send_to_char( "Despiertas y te pones a descansar.\r\n", ch );
                act( AT_ACTION, "$n despierta y se pone a descansar.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) ) {
                act( AT_ACTION, "Despiertas y te pones a descansar sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n despierta y se pone a descansar sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) ) {
                act( AT_ACTION, "Despiertas y te pones a descansar en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n despierta y se pone a descansar en $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Despiertas y te pones a descansar en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se despierta y se pone a descansar en $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_RESTING );
            break;
        case POS_RESTING:
            if ( obj != NULL && aon != 1 ) {

                if ( IS_SET( obj->value[2], REST_AT ) ) {
                    act( AT_ACTION, "Te pones a descansar sobre $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se pone a descansar sobre $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], REST_ON ) ) {
                    act( AT_ACTION, "Te pones a descansar sobre $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se pone a descansar sobre $p.", ch, obj, NULL, TO_ROOM );
                }
                else {
                    act( AT_ACTION, "Te pones a descansar sobre $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n se pone a descansar sobre $p.", ch, obj, NULL, TO_ROOM );
                }
            }
            else if ( aon == 1 ) {
                act( AT_ACTION, "ya usas $p.", ch, obj, NULL, TO_CHAR );
            }
            else if ( ch->on != NULL && obj == NULL ) {
                act( AT_ACTION, "Te bajas de $p y comienzas a descansar en el suelo.", ch, ch->on,
                     NULL, TO_CHAR );
                act( AT_ACTION, "$n se baja de $p y comienza a descansar en el suelo.", ch, ch->on,
                     NULL, TO_ROOM );
                ch->on = NULL;
            }
            else
                send_to_char( "Ya estás descansando.\r\n", ch );
            break;
        case POS_STANDING:
            if ( obj == NULL ) {
                send_to_char( "Te estiras y descansas pacíficamente.\r\n", ch );
                act( AT_ACTION, "$n se estira y comienza a descansar.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) ) {
                act( AT_ACTION, "Te sientas sobre $p y comienzas a descansar.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sienta sobre $p y comienza a descansar.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) ) {
                act( AT_ACTION, "Te sientas sobre $p y te pones a descansar.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se sienta en $p y se pone a descansar.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Comienzas a descansar en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se pone a descansar en $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_RESTING );
            break;

        case POS_CRAWL:
            send_to_char( "Ruedas sobre tu espalda y te pones a descansar.\r\n", ch );
            act( AT_ACTION, "$n rueda sobre su espalda y se pone a descansar.", ch, NULL, NULL, TO_ROOM );
            set_position( ch, POS_RESTING );
            break;

        case POS_SITTING:
            if ( obj == NULL ) {
                send_to_char( "Te estiras y comienzas a descansar pacíficamente.\r\n", ch );
                act( AT_ACTION, "$n se estira y comienza a descansar.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_AT ) ) {
                act( AT_ACTION, "Te pones a descansar sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se pone a descansar sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], REST_ON ) ) {
                act( AT_ACTION, "Te pones a descansar en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se pone a descansar en $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Te pones a descansar en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se pone a descansar en $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_RESTING );
            break;
    }
    if ( obj != NULL ) {
        if ( obj->value[1] == 0 )
            val1 = 750;
        else
            val1 = obj->value[1];
        if ( max_weight( obj ) > val1 ) {
            act( AT_ACTION, "El peso de $n es demasiado para $p.", ch, ch->on, NULL,
                 TO_ROOM );
            act( AT_ACTION, "Intentas sentarte sobre $p causando que se rompa.", ch, ch->on, NULL,
                 TO_CHAR );
            for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room ) {
                if ( fch->on == obj ) {
                    if ( fch->position == POS_RESTING ) {
                        fch->hit = ( fch->hit - 30 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION,
                             "Tu descanso se interrumpe cuando $p se rompe y caes al suelo.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SLEEPING ) {
                        fch->hit = ( fch->hit - 40 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        set_position( fch, POS_RESTING );
                        act( AT_ACTION,
                             "Tu sueño se interrumpe cuando $p se rompe y caes al suelo.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SITTING ) {
                        fch->hit = ( fch->hit - 5 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Tu descanso se interrumpe cuando $p se rompe y caes al suelo.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_STANDING ) {
                        fch->hit = ( fch->hit - 55 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Caes al suelo cuando $p se rompe.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    fch->on = NULL;
                }
            }
            make_scraps( obj );
        }
    }
    rprog_rest_trigger( ch );
    return;
}

void do_sleep( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj = NULL;
    int                     aon = 0;
    CHAR_DATA              *fch = NULL;
    int                     val0;
    int                     val1;

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "¿Qué tal si terminas primero tu pelea?\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
        send_to_char( "Tienes que aterrizar primero.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_PRAYER ) ) {
        send_to_char( "¡No mientras meditas!\r\n", ch );
        return;
    }
    /*
     * okay, now that we know we can sit, find an object to sit on 
     */
    if ( argument[0] != '\0' ) {
        obj = get_obj_list( ch, argument, ch->in_room->first_content );
        if ( obj == NULL ) {
            send_to_char( "No ves eso aquí.\r\n", ch );
            return;
        }
        if ( obj->item_type != ITEM_FURNITURE ) {
            send_to_char( "No puedes.\r\n", ch );
            return;
        }
        if ( !IS_SET( obj->value[2], SLEEP_ON ) && !IS_SET( obj->value[2], SLEEP_IN )
             && !IS_SET( obj->value[2], SLEEP_AT ) ) {
            send_to_char( "No puedes dormir sobre eso.\r\n", ch );
            return;
        }
        if ( obj->value[0] == 0 )
            val0 = 1;
        else
            val0 = obj->value[0];
        if ( ch->on != obj && count_users( obj ) >= val0 ) {
            act( AT_ACTION, "No hay lugar para dormir en $p.", ch, obj, NULL, TO_CHAR );
            return;
        }
        if ( ch->on == obj )
            aon = 1;
        else
            ch->on = obj;
    }

    switch ( ch->position ) {
        case POS_MEDITATING:
            send_to_char( "Dejas de meditar y cierras los ojos para caer en un profundo sueño.\r\n", ch );
            act( AT_ACTION, "$n deja de meditar y tras cerrar los ojos cae en un profundo sueño.", ch, NULL, NULL,
                 TO_ROOM );
            set_position( ch, POS_SLEEPING );
            break;

        case POS_SLEEPING:
            if ( obj != NULL && aon != 1 ) {

                if ( IS_SET( obj->value[2], SLEEP_AT ) ) {
                    act( AT_ACTION, "Duermes sobre $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n duerme sobre $p.", ch, obj, NULL, TO_ROOM );
                }
                else if ( IS_SET( obj->value[2], SLEEP_ON ) ) {
                    act( AT_ACTION, "Duermes en $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n duerme en $p.", ch, obj, NULL, TO_ROOM );
                }
                else {
                    act( AT_ACTION, "Duermes en $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n duerme en $p.", ch, obj, NULL, TO_ROOM );
                }
            }
            else if ( aon == 1 ) {
                act( AT_ACTION, "ya estás usando $p.", ch, obj, NULL, TO_CHAR );
            }
            else if ( ch->on != NULL && obj == NULL ) {
                act( AT_ACTION, "Te bajas de $p e intentas dormir en el suelo.", ch, ch->on,
                     NULL, TO_CHAR );
                act( AT_ACTION, "$n se baja de $p y se pone a dormir en el suelo.", ch,
                     ch->on, NULL, TO_ROOM );
                ch->on = NULL;
            }
            else
                send_to_char( "ya estás durmiendo.\r\n", ch );
            break;
        case POS_RESTING:
            if ( obj == NULL ) {
                send_to_char( "Cierras los ojos y caes en un profundo sueño.\r\n", ch );
                act( AT_ACTION, "$n cierra los ojos y cae en un profundo sueño.", ch, NULL, NULL,
                     TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_AT ) ) {
                act( AT_ACTION, "Duermes sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n duerme sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_ON ) ) {
                act( AT_ACTION, "Duermes en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n duerme en $p.", ch, obj, NULL, TO_ROOM );
            }

            else {
                act( AT_ACTION, "Duermes en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n duerme en $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SLEEPING );
            break;
        case POS_SITTING:
            if ( obj == NULL ) {
                send_to_char( "Cierras los ojos y caes en un profundo sueño.\r\n", ch );
                act( AT_ACTION, "$n se tumba y cae en un profundo sueño tras cerrar los ojos.", ch, NULL, NULL,
                     TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_AT ) ) {
                act( AT_ACTION, "Duermes sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n duerme sobre $p.", ch, obj, NULL, TO_ROOM );
            }

            else if ( IS_SET( obj->value[2], SLEEP_ON ) ) {
                act( AT_ACTION, "Duermes en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n duerme en $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Duermes en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n duerme en $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SLEEPING );

            break;
        case POS_STANDING:
            if ( IS_AFFECTED( ch, AFF_FEIGN ) && ( obj == NULL ) ) {
                act( AT_ACTION, "$n cae al suelo.", ch, NULL, NULL, TO_ROOM );
            }
            else if ( obj == NULL && !IS_AFFECTED( ch, AFF_FEIGN ) ) {
                send_to_char( "Te pones a dormir en el suelo.\r\n", ch );
                act( AT_ACTION, "$n se pone a dormir en el suelo.", ch, NULL, NULL,
                     TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_AT ) ) {
                act( AT_ACTION, "Duermes sobre $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n duermes sobre $p.", ch, obj, NULL, TO_ROOM );
            }
            else if ( IS_SET( obj->value[2], SLEEP_ON ) ) {
                act( AT_ACTION, "Duermes en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n duerme en $p.", ch, obj, NULL, TO_ROOM );
            }
            else {
                act( AT_ACTION, "Te pones a dormir en $p.", ch, obj, NULL, TO_CHAR );
                act( AT_ACTION, "$n se pone a dormir en $p.", ch, obj, NULL, TO_ROOM );
            }
            set_position( ch, POS_SLEEPING );
            break;
    }
    if ( obj != NULL ) {
        if ( obj->value[1] == 0 )
            val1 = 750;
        else
            val1 = obj->value[1];
        if ( max_weight( obj ) > val1 ) {
            act( AT_ACTION, "El peso de $n es demasiado para $p.", ch, ch->on, NULL,
                 TO_ROOM );
            act( AT_ACTION, "Tu intento de dormir en $p causa que se rompa.", ch, ch->on, NULL,
                 TO_CHAR );
            for ( fch = obj->in_room->first_person; fch != NULL; fch = fch->next_in_room ) {
                if ( fch->on == obj ) {
                    if ( fch->position == POS_RESTING ) {
                        fch->hit = ( fch->hit - 30 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION,
                             "Tu descanso se interrumpe cuando $p se rompe.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SLEEPING ) {
                        fch->hit = ( fch->hit - 40 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        set_position( fch, POS_RESTING );
                        act( AT_ACTION,
                             "Caes al suelo de golpe cuando $p se rompe.",
                             fch, fch->on, NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_SITTING ) {
                        fch->hit = ( fch->hit - 5 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Caes al suelo de golpe cuando $p se rompe.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    if ( fch->position == POS_STANDING ) {
                        fch->hit = ( fch->hit - 55 );
                        if ( fch->hit <= 0 )
                            fch->hit = 1;
                        act( AT_ACTION, "Te das un gran golpe contra el suelo cuando $p se rompe.", fch, fch->on,
                             NULL, TO_CHAR );
                    }
                    fch->on = NULL;
                }
            }
            make_scraps( obj );
        }
    }
    rprog_sleep_trigger( ch );
    return;
}

void do_wake( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        do_stand( ch, argument );
        return;
    }

    if ( !IS_AWAKE( ch ) ) {
        send_to_char( "¡Pero si no estás durmiendo!\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( IS_AWAKE( victim ) ) {
        act( AT_PLAIN, "$N no está durmiendo.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_SLEEP ) || victim->position < POS_SLEEPING ) {
        act( AT_PLAIN, "¡No puedes despertar a $N!", ch, NULL, victim, TO_CHAR );
        return;
    }

    act( AT_ACTION, "Despiertas a $M.", ch, NULL, victim, TO_CHAR );
    set_position( victim, POS_STANDING );
    act( AT_ACTION, "$n te despierta.", ch, NULL, victim, TO_VICT );
    return;
}

/*
 * teleport a character to another room
 */
void teleportch( CHAR_DATA *ch, ROOM_INDEX_DATA *room, bool show )
{
    char                    buf[MSL];

    if ( room_is_private( room ) )
        return;
    act( AT_ACTION, "¡$n desaparece!", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, room );
    act( AT_ACTION, "¡$n aparece!", ch, NULL, NULL, TO_ROOM );
    if ( show )
        do_look( ch, ( char * ) "auto" );
    if ( IS_SET( ch->in_room->room_flags, ROOM_DEATH ) && !IS_IMMORTAL( ch ) ) {
        act( AT_DEAD, "¡$n muere horriblemente!", ch, NULL, NULL, TO_ROOM );
        set_char_color( AT_DEAD, ch );
        send_to_char( "¡Mueres horriblemente!\r\n", ch );
        snprintf( buf, MSL, "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
        log_string( buf );
        to_channel( buf, "log", LEVEL_IMMORTAL );
        extract_char( ch, FALSE );
    }
}

void teleport( CHAR_DATA *ch, int room, EXT_BV * flags )
{
    CHAR_DATA              *nch,
                           *nch_next;
    ROOM_INDEX_DATA        *start = ch->in_room,
        *dest;
    bool                    show;

    dest = get_room_index( room );
    if ( !dest ) {
        bug( "teleport: bad room vnum %d", room );
        return;
    }

    if ( xIS_SET( *flags, TELE_SHOWDESC ) )
        show = TRUE;
    else
        show = FALSE;
    if ( !xIS_SET( *flags, TELE_TRANSALL ) ) {
        teleportch( ch, dest, show );
        return;
    }

    /*
     * teleport everybody in the room 
     */
    for ( nch = start->first_person; nch; nch = nch_next ) {
        nch_next = nch->next_in_room;
        teleportch( nch, dest, show );
    }

    /*
     * teleport the objects on the ground too 
     */
    if ( xIS_SET( *flags, TELE_TRANSALLPLUS ) ) {
        OBJ_DATA               *obj,
                               *obj_next;

        for ( obj = start->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;
            obj_from_room( obj );
            obj_to_room( obj, dest );
        }
    }
}

/*
 * "Climb" in a certain direction.     -Thoric
 */
void do_climb( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    bool                    found;

    found = FALSE;
    if ( argument[0] == '\0' ) {
        for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            if ( IS_SET( pexit->exit_info, EX_xCLIMB ) ) {
                move_char( ch, pexit, 0, FALSE );
                return;
            }
        send_to_char( "No puedes trepar aquí.\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
         && IS_SET( pexit->exit_info, EX_xCLIMB ) ) {
        move_char( ch, pexit, 0, FALSE );
        return;
    }
    send_to_char( "No puedes trepar aquí.\r\n", ch );
    return;
}

/*
 * "enter" something (moves through an exit)   -Thoric
 */
void do_enter( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    bool                    found;
    OBJ_DATA               *obj;

    found = FALSE;
    CHAR_DATA              *victim = NULL;

    if ( argument[0] == '\0' ) {
        for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            if ( IS_SET( pexit->exit_info, EX_xENTER ) ) {
                move_char( ch, pexit, 0, FALSE );
                return;
            }
        if ( ch->in_room->sector_type != SECT_INSIDE && IS_OUTSIDE( ch ) )
            for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
                if ( pexit->to_room
                     && ( pexit->to_room->sector_type == SECT_INSIDE
                          || IS_SET( pexit->to_room->room_flags, ROOM_INDOORS ) ) ) {
                    move_char( ch, pexit, 0, FALSE );
                    return;
                }
        send_to_char( "No ves ninguna entrada aquí.\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
         && IS_SET( pexit->exit_info, EX_xENTER ) ) {
        move_char( ch, pexit, 0, FALSE );
        return;
    }
    send_to_char( "No puedes entrar ahí.\r\n", ch );
    return;
}

/*
 * Leave through an exit.     -Thoric
 */
void do_leave( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    bool                    found;

    found = FALSE;

    if ( argument[0] == '\0' ) {
        for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
            if ( IS_SET( pexit->exit_info, EX_xLEAVE ) ) {
                move_char( ch, pexit, 0, FALSE );
                return;
            }
        if ( ch->in_room->sector_type == SECT_INSIDE || !IS_OUTSIDE( ch ) )
            for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
                if ( pexit->to_room && pexit->to_room->sector_type != SECT_INSIDE
                     && !IS_SET( pexit->to_room->room_flags, ROOM_INDOORS ) ) {
                    move_char( ch, pexit, 0, FALSE );
                    return;
                }
        send_to_char( "No encuentras ninguna salida aquí.\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, argument, TRUE ) ) != NULL
         && IS_SET( pexit->exit_info, EX_xLEAVE ) ) {
        move_char( ch, pexit, 0, FALSE );
        return;
    }
    send_to_char( "No puedes salir por ahí.\r\n", ch );
    return;
}

/*
 * Check to see if an exit in the room is pulling (or pushing) players around.
 * Some types may cause damage.     -Thoric
 *
 * People kept requesting currents (like SillyMUD has), so I went all out
 * and added the ability for an exit to have a "pull" or a "push" force
 * and to handle different types much beyond a simple water current.
 *
 * This check is called by violence_update().  I'm not sure if this is the
 * best way to do it, or if it should be handled by a special queue.
 *
 * Future additions to this code may include equipment being blown away in
 * the wind (mostly headwear), and people being hit by flying objects
 *
 * TODO:
 *  handle more pulltypes
 *  give "entrance" messages for players and objects
 *  proper handling of player resistance to push/pulling
 */
ch_ret pullcheck( CHAR_DATA *ch, int pulse )
{
    ROOM_INDEX_DATA        *room;
    EXIT_DATA              *xtmp,
                           *xit = NULL;
    OBJ_DATA               *obj,
                           *obj_next;
    bool                    move = FALSE,
        moveobj = TRUE,
        showroom = TRUE;
    int                     pullfact,
                            pull;
    int                     resistance;
    const char             *tochar = NULL,
        *toroom = NULL,
        *objmsg = NULL;
    const char             *destrm = NULL,
        *destob = NULL,
        *dtxt = "somewhere";

    if ( ( room = ch->in_room ) == NULL ) {
        bug( "pullcheck: %s not in a room?!?", ch->name );
        return rNONE;
    }

    /*
     * Find the exit with the strongest force (if any) 
     */
    for ( xtmp = room->first_exit; xtmp; xtmp = xtmp->next )
        if ( xtmp->pull && xtmp->to_room && ( !xit || abs( xtmp->pull ) > abs( xit->pull ) ) )
            xit = xtmp;

    if ( !xit )
        return rNONE;

    pull = xit->pull;

    /*
     * strength also determines frequency 
     */
    pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );

    /*
     * strongest pull not ready yet... check for one that is 
     */
    if ( ( pulse % pullfact ) != 0 ) {
        for ( xit = room->first_exit; xit; xit = xit->next )
            if ( xit->pull && xit->to_room ) {
                pull = xit->pull;
                pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );
                if ( ( pulse % pullfact ) != 0 )
                    break;
            }

        if ( !xit )
            return rNONE;
    }

    /*
     * negative pull = push... get the reverse exit if any 
     */
    if ( pull < 0 )
        if ( ( xit = get_exit( room, rev_dir[xit->vdir] ) ) == NULL )
            return rNONE;

    dtxt = rev_exit( xit->vdir );

    /*
     * First determine if the player should be moved or not
     * Check various flags, spells, the players position and strength vs.
     * the pull, etc... any kind of checks you like.
     */
    switch ( xit->pulltype ) {
        case PULL_CURRENT:
        case PULL_WHIRLPOOL:
            switch ( room->sector_type ) {
                    /*
                     * allow whirlpool to be in any sector type 
                     */
                default:
                    if ( xit->pulltype == PULL_CURRENT )
                        break;
                case SECT_OCEAN:
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                    if ( ( ch->mount && !IS_FLOATING( ch->mount ) )
                         || ( !ch->mount && !IS_FLOATING( ch ) ) )
                        move = TRUE;
                    break;

                case SECT_UNDERWATER:
                case SECT_OCEANFLOOR:
                    move = TRUE;
                    break;
            }
            break;
        case PULL_GEYSER:
        case PULL_WAVE:
            move = TRUE;
            break;

        case PULL_WIND:
        case PULL_STORM:
            /*
             * if not flying... check weight, position & strength 
             */
            move = TRUE;
            break;

        case PULL_COLDWIND:
            /*
             * if not flying... check weight, position & strength 
             */
            /*
             * also check for damage due to bitter cold 
             */
            move = TRUE;
            break;

        case PULL_HOTAIR:
            /*
             * if not flying... check weight, position & strength 
             */
            /*
             * also check for damage due to heat 
             */
            move = TRUE;
            break;

            /*
             * light breeze -- very limited moving power 
             */
        case PULL_BREEZE:
            move = FALSE;
            break;

            /*
             * exits with these pulltypes should also be blocked from movement
             * ie: a secret locked pickproof door with the name "_sinkhole_", etc
             */
        case PULL_EARTHQUAKE:
        case PULL_SINKHOLE:
        case PULL_QUICKSAND:
        case PULL_LANDSLIDE:
        case PULL_SLIP:
        case PULL_LAVA:
            if ( ( ch->mount && !IS_FLOATING( ch->mount ) )
                 || ( !ch->mount && !IS_FLOATING( ch ) ) )
                move = TRUE;
            break;

            /*
             * as if player moved in that direction him/herself 
             */
        case PULL_UNDEFINED:
            return move_char( ch, xit, 0, FALSE );

            /*
             * all other cases ALWAYS move 
             */
        default:
            move = TRUE;
            break;
    }

    /*
     * assign some nice text messages 
     */
    switch ( xit->pulltype ) {
        case PULL_MYSTERIOUS:
            /*
             * no messages to anyone 
             */
            showroom = FALSE;
            break;
        case PULL_WHIRLPOOL:
        case PULL_VACUUM:
            tochar = "Algo te succiona hacia $T!";
            toroom =  "Algo succiona a $n hacia $T!";
            destrm = "$n llega desde $T!";
            objmsg = "Algo succiona $p hacia $T.";
            destob = "$p es succionado desde $T!";
            break;
        case PULL_CURRENT:
        case PULL_LAVA:
            tochar = "Te mueves hacia $T.";
            toroom = "$n se mueve hacia $T.";
            destrm = "$n llega desde $T.";
            objmsg = "$p se mueve hacia hacia $T.";
            destob = "$p llega desde $T.";
            break;
        case PULL_BREEZE:
            tochar = "Te mueves hacia $T.";
            toroom = "$n se mueve hacia $T.";
            destrm = "$n llega desde $T.";
            objmsg = "$p se mueve hacia $T.";
            destob = "$p llega desde $T.";
            break;
        case PULL_GEYSER:
        case PULL_WAVE:
            tochar = "Te empujan hacia $T!";
            toroom = "A $n le empujan hacia $T!";
            destrm = "$n es empujado desde $T!";
            destob = "$p flota hacia $T.";
            break;
        case PULL_EARTHQUAKE:
            tochar = "La tierra se abre y caes hacia $T!";
            toroom = "La tierra se abre y $n cae hacia $T!";
            destrm = "$n cae desde $T!";
            objmsg = "$p cae hacia $T.";
            destob = "$p cae desde $T.";
            break;
        case PULL_SINKHOLE:
            tochar = "Caes hacia $T!";
            toroom = "$n cae!";
            destrm = "$n cae desde $T!";
            objmsg = "$p cae hacia $T.";
            destob = "$p cae desde $T.";
            break;
        case PULL_QUICKSAND:
            tochar = "Comienzas a hundirte en las arenas movedizas hacia $T!";
            toroom = "$n comienza a hundirse en las arenas movedizas hacia $T!";
            destrm = "$n llega desde $T.";
            objmsg = "$p se hunde en las arenas movedizas hacia $T.";
            destob = "$p se hunde desde $T.";
            break;
        case PULL_LANDSLIDE:
            tochar = "El suelo se inclina hacia $T, llevándote con el!";
            toroom = "El suelo se inclina hacia $T, llevándose a $n con el!";
            destrm = "$n se desliza desde $T.";
            objmsg = "$p se desliza hacia $T.";
            destob = "$p se desliza desde $T.";
            break;
        case PULL_SLIP:
            tochar = "Pierdes pie!";
            toroom = "$n pierde pie!";
            destrm = "$n se desliza desde $T.";
            objmsg = "$p se desliza hacia $T.";
            destob = "$p se desliza desde $T.";
            break;
        case PULL_VORTEX:
            tochar = "¡Un remolino de colores te absorbe!";
            toroom = "$n desaparece cuando le absorbe un remolino de colores!";
            toroom = "$n aparece de un remolino de colores!";
            objmsg = "$p desaparece en un remolino de colores!";
            objmsg = "$p aparece entre un remolino de colores!";
            break;
        case PULL_HOTAIR:
            tochar = "Una corriente de aire te empuja hacia $T!";
            toroom = "$n desaparece cuando una corriente de aire le empuja hacia $T!";
            destrm = "$n aparece desde $T cuando una corriente de aire le empuja!";
            objmsg = "$p vuela hacia $T.";
            destob = "$p vuela desde $T.";
            break;
        case PULL_COLDWIND:
            tochar = "¡El gélido viento te obliga a ir hacia $T!";
            toroom = "¡$n se va hacia $T cuando el gélido viento le empuja!";
            destrm = "¡$n llega desde $T debido a un fuerte viento gélido!";
            objmsg = "$p vuela hacia $T.";
            destob = "$p llega volando desde $T.";
            break;
        case PULL_WIND:
            tochar = "Un fuerte viento te empuja hacia $T!";
            toroom = "¡Un fuerte viento empuja a $n hacia $T!";
            destrm = "¡$n llega desde $T debido a un fuerte viento!";
            objmsg = "$p vuela hacia $T.";
            destob = "$p vuela desde from $T.";
            break;
        case PULL_STORM:
            tochar = "La poderosa tormenta te obliga a ir hacia $T!";
            toroom = "¡La poderosa tormenta obliga a $n a ir hacia $T!";
            destrm = "¡$n llega desde $T debido a una gran tormenta!";
            objmsg = "$p vuela hacia $T.";
            destob = "$p vuela desde $T.";
            break;
        default:
            if ( pull > 0 ) {
                tochar = "Te empujan hacia $T!";
                toroom = "$n es empujado hacia $T.";
                destrm = "$n es empujado desde $T.";
                objmsg = "$p es empujado desde $T.";
                objmsg = "$p es empujado desde $T.";
            }
            else {
                tochar = "Te empujan hacia $T!";
                toroom = "Empujan a $n hacia $T.";
                destrm = "$n llega desde $T.";
                objmsg = "Empujan a $p hacia $T.";
                objmsg = "$p llega desde $T.";
            }
            break;
    }

    /*
     * Do the moving 
     */
    if ( move ) {
        /*
         * display an appropriate exit message 
         */
        if ( tochar ) {
            act( AT_PLAIN, tochar, ch, NULL, dir_name[xit->vdir], TO_CHAR );
            send_to_char( "\r\n", ch );
        }
        if ( toroom )
            act( AT_PLAIN, toroom, ch, NULL, dir_name[xit->vdir], TO_ROOM );

        /*
         * move the char 
         */
        if ( xit->pulltype == PULL_SLIP )
            return move_char( ch, xit, 1, FALSE );
        char_from_room( ch );
        char_to_room( ch, xit->to_room );
        /*
         * move the mount too 
         */
        if ( ch->mount ) {
            char_from_room( ch->mount );
            char_to_room( ch->mount, xit->to_room );
            if ( showroom )
                do_look( ch->mount, ( char * ) "auto" );
        }

        /*
         * If they are mounted mount should show up when it auto looks 
         */
        if ( showroom )
            do_look( ch, ( char * ) "auto" );
        /*
         * display an appropriate entrance message 
         */
        /*
         * Only need to send to room really, since char already knows where they were pushed etc... 
         */
        if ( destrm )
            act( AT_PLAIN, destrm, ch, NULL, dtxt, TO_ROOM );
    }

    /*
     * move objects in the room 
     */
    if ( moveobj ) {
        for ( obj = room->first_content; obj; obj = obj_next ) {
            obj_next = obj->next_content;

            if ( IS_OBJ_STAT( obj, ITEM_BURIED ) || !CAN_WEAR( obj, ITEM_TAKE ) )
                continue;

            resistance = get_obj_weight( obj, FALSE );
            if ( IS_OBJ_STAT( obj, ITEM_METAL ) )
                resistance = ( resistance * 6 ) / 5;
            switch ( obj->item_type ) {
                case ITEM_SCROLL:
                case ITEM_NOTE:
                case ITEM_TRASH:
                    resistance >>= 2;
                    break;
                case ITEM_SCRAPS:
                case ITEM_CONTAINER:
                    resistance >>= 1;
                    break;
                case ITEM_PEN:
                case ITEM_WAND:
                    resistance = ( resistance * 5 ) / 6;
                    break;

                case ITEM_CORPSE_PC:
                case ITEM_CORPSE_NPC:
                case ITEM_FOUNTAIN:
                    resistance <<= 2;
                    break;
            }

            /*
             * is the pull greater than the resistance of the object? 
             */
            if ( ( abs( pull ) * 10 ) > resistance ) {
                if ( objmsg && room->first_person ) {
                    act( AT_PLAIN, objmsg, room->first_person, obj, dir_name[xit->vdir], TO_CHAR );
                    act( AT_PLAIN, objmsg, room->first_person, obj, dir_name[xit->vdir], TO_ROOM );
                }
                if ( destob && xit->to_room->first_person ) {
                    act( AT_PLAIN, destob, xit->to_room->first_person, obj, dtxt, TO_CHAR );
                    act( AT_PLAIN, destob, xit->to_room->first_person, obj, dtxt, TO_ROOM );
                }
                obj_from_room( obj );
                obj_to_room( obj, xit->to_room );
            }
        }
    }

    return rNONE;
}

/*
 * This function bolts a door. Written by Blackmane
 */

void do_bolt( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    EXIT_DATA              *pexit;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "atornillar qué?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "No puedes hacer eso.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "No está cerrada.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_ISBOLT ) ) {
            send_to_char( "No ves que se pueda hacer.\r\n", ch );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_BOLTED ) ) {
            send_to_char( "ya está.\r\n", ch );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) ) {
            send_to_char( "*Clunk*\r\n", ch );
            act( AT_ACTION, "$n atornilla $d.", ch, NULL, pexit->keyword, TO_ROOM );
            set_bexit_flag( pexit, EX_BOLTED );
            return;
        }
    }

    ch_printf( ch, "No ves %s aquí.\r\n", arg );
    return;
}

/*
 * This function unbolts a door.  Written by Blackmane
 */

void do_unbolt( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    EXIT_DATA              *pexit;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Desatornillar qué?\r\n", ch );
        return;
    }

    if ( ( pexit = find_door( ch, arg, TRUE ) ) != NULL ) {

        if ( !IS_SET( pexit->exit_info, EX_ISDOOR ) ) {
            send_to_char( "No puedes hacer eso.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "No está cerrada.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_ISBOLT ) ) {
            send_to_char( "No puedes.\r\n", ch );
            return;
        }
        if ( !IS_SET( pexit->exit_info, EX_BOLTED ) ) {
            send_to_char( "Ya está.\r\n", ch );
            return;
        }

        if ( !IS_SET( pexit->exit_info, EX_SECRET )
             || ( pexit->keyword && nifty_is_name( arg, pexit->keyword ) ) ) {
            send_to_char( "*Clunk*\r\n", ch );
            act( AT_ACTION, "$n desatornilla $d.", ch, NULL, pexit->keyword, TO_ROOM );
            remove_bexit_flag( pexit, EX_BOLTED );
            return;
        }
    }

    ch_printf( ch, "No ves %s aquí.\r\n", arg );
    return;
}

short max_holdbreath( CHAR_DATA *ch )
{
    int                     breath;

    switch ( ch->race ) {
        default:
            breath = 60;
            break;

        case RACE_DRAGON:
            breath = 100;
            break;

        case RACE_GNOME:
        case RACE_ELF:
        case RACE_DROW:
        case RACE_HALFLING:
            breath = 40;
            break;

        case RACE_DWARF:
        case RACE_TROLL:
        case RACE_ORC:
        case RACE_OGRE:
            breath = 80;
            break;

        case RACE_PIXIE:
            breath = 30;
            break;
    }

    breath += get_curr_con( ch );
    breath += ch->level;

    if ( breath )
        return breath;
    else
        return 0;
}

bool can_swim( CHAR_DATA *ch )
{
    if ( IS_NPC( ch ) )
        return TRUE;

    if ( ch->move > 0 && ch->hit > 0 ) {
        if ( ch->pcdata->learned[gsn_swim] > number_percent(  ) )
            return TRUE;
    }
    else
        return FALSE;

    if ( !ch->pcdata->learned[gsn_swim] )
        return FALSE;

    if ( ch->weight < ( ch->level * ch->pcdata->learned[gsn_swim] ) )
        return TRUE;

    return FALSE;
}

void fbite_msg( CHAR_DATA *ch, int percentage )
{
    if ( percentage == 0 )
        percentage = 100 * ch->pcdata->frostbite / max_holdbreath( ch );

    if ( percentage > 90 )
        send_to_char( "&WFrío:(          &W)&D\r\n", ch );
    else if ( percentage > 80 )
        send_to_char( "&WFrío: (&R+         &W)&D\r\n", ch );
    else if ( percentage > 70 )
        send_to_char( "&WFrío: (&R++        &W)&D\r\n", ch );
    else if ( percentage > 60 )
        send_to_char( "&WFrío: (&R+++       &W)&D\r\n", ch );
    else if ( percentage > 50 )
        send_to_char( "&WFrío: (&Y++++      &W)&D\r\n", ch );
    else if ( percentage > 40 )
        send_to_char( "&WFrío: (&Y+++++     &W)&D\r\n", ch );
    else if ( percentage > 30 )
        send_to_char( "&WFrío: (&Y++++++    &W)&D\r\n", ch );
    else if ( percentage > 20 )
        send_to_char( "&WFrío: (&G+++++++   &W)&D\r\n", ch );
    else if ( percentage > 10 )
        send_to_char( "&WFrío: (&G++++++++  &W)&D\r\n", ch );
    else if ( percentage > 0 )
        send_to_char( "&WFrío: (&G+++++++++ &W)&D\r\n", ch );
    else if ( percentage == 0 )
        send_to_char( "&WFrío: (&G++++++++++&W)&D\r\n", ch );

    return;
}

void breath_msg( CHAR_DATA *ch, int percentage )
{
    if ( percentage == 0 )
        percentage = 100 * ch->pcdata->holdbreath / max_holdbreath( ch );

    if ( percentage > 90 )
        send_to_char( "&WNivel de respiración:(          &W)&D\r\n", ch );
    else if ( percentage > 80 )
        send_to_char( "&WNivel de respiración: (&R+         &W)&D\r\n", ch );
    else if ( percentage > 70 )
        send_to_char( "&WNivel de respiración: (&R++        &W)&D\r\n", ch );
    else if ( percentage > 60 )
        send_to_char( "&WNivel de respiración: (&R+++       &W)&D\r\n", ch );
    else if ( percentage > 50 )
        send_to_char( "&WNivel de respiración: (&Y++++      &W)&D\r\n", ch );
    else if ( percentage > 40 )
        send_to_char( "&WNivel de respiración: (&Y+++++     &W)&D\r\n", ch );
    else if ( percentage > 30 )
        send_to_char( "&WNivel de respiración: (&Y++++++    &W)&D\r\n", ch );
    else if ( percentage > 20 )
        send_to_char( "&WNivel de respiración: (&G+++++++   &W)&D\r\n", ch );
    else if ( percentage > 10 )
        send_to_char( "&WNivel de respiración: (&G++++++++  &W)&D\r\n", ch );
    else if ( percentage > 0 )
        send_to_char( "&WNivel de respiración: (&G+++++++++ &W)&D\r\n", ch );
    else if ( percentage == 0 )
        send_to_char( "&WNivel de respiración: (&G++++++++++&W)&D\r\n", ch );

    return;
}

void water_sink( CHAR_DATA *ch, int time )
{
    EXIT_DATA              *pexit;
    OBJ_DATA               *obj;
    ROOM_INDEX_DATA        *room = ch->in_room;

/*  Bug fix - if not already underwater, fly/float will stop sinking.  */
    if ( room->sector_type != SECT_UNDERWATER
         && ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) )
        return;

    for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next ) {
        if ( pexit->to_room
             && ( pexit->to_room->sector_type == SECT_UNDERWATER
                  || pexit->to_room->sector_type == SECT_OCEANFLOOR ) && pexit->vdir == DIR_DOWN ) {
            if ( !can_swim( ch ) && ( number_bits( time ) == 0 ) ) {    /* Chance to sink 
                                                                         */
                act( AT_CYAN, "¡Te hundes en las profundidades!", ch, NULL, NULL, TO_CHAR );
                act( AT_CYAN, "$n no consigue mantenerse a flote y se hunde lentamente.", ch,
                     NULL, NULL, TO_ROOM );

                if ( pexit->to_room->first_person )
                    act( AT_CYAN, "$n no consigue mantenerse a flote y cae ante ti.",
                         pexit->to_room->first_person, NULL, NULL, TO_ROOM );
                char_from_room( ch );
                char_to_room( ch, pexit->to_room );
                interpret( ch, ( char * ) "look" );

                if ( ch->mount ) {
                    char_from_room( ch->mount );
                    char_to_room( ch->mount, pexit->to_room );
                    interpret( ch->mount, ( char * ) "look" );
                }
            }

/*  Let's deal with sinking objects too  */

            for ( obj = room->first_content; obj; obj = obj->next_content ) {
                if ( number_bits( 3 ) == 0 ) {
                    if ( room->first_person ) {
                        act( AT_CYAN, "$p se hunde en las profundidades.", room->first_person,
                             obj, NULL, TO_CHAR );
                        act( AT_CYAN, "$p se hunde lentamente en las profundidades.", room->first_person,
                             obj, NULL, TO_ROOM );
                    }
                    if ( pexit->to_room->first_person ) {
                        act( AT_CYAN, "$p cae desde arriba.", pexit->to_room->first_person,
                             obj, NULL, TO_CHAR );
                        act( AT_CYAN, "$p cae desde arriba.", pexit->to_room->first_person,
                             obj, NULL, TO_ROOM );
                    }
                    obj_from_room( obj );
                    obj_to_room( obj, pexit->to_room );
                }
            }
        }
    }
    return;
}

void swim_check( CHAR_DATA *ch, int time )
{
    int                     maxbreath = max_holdbreath( ch );
    int                     percentage = 100 * ch->pcdata->holdbreath / maxbreath;
    OBJ_DATA               *obj;
    int                     dam;
    short                   chance;

/*  Mobs can't drown - yet! Nor immortals. */
    if ( IS_NPC( ch ) )
        return;

    chance = number_range( 1, 100 );

/*  Players are swimming or above the surface of the water. They may also have a boat.  */

    if ( ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_OCEAN
         || ch->in_room->sector_type == SECT_LAKE || ch->in_room->sector_type == SECT_RIVER ) {
        if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) ) {
            if ( chance == 3 ) {
                send_to_char( "&CFlotas por la superficie del agua.&D\r\n", ch );
            }
            return;
        }

        for ( obj = ch->first_carrying; obj; obj = obj->next_content )
            if ( obj->item_type == ITEM_BOAT )
                break;

        if ( obj ) {
            if ( number_bits( 4 ) == 0 )
                send_to_char( "&CFlotas con tu barco por el agua.&D\r\n", ch );
            return;
        }

        if ( xIS_SET( ch->act, PLR_BOAT ) ) {
            short                   mess = 0;

            mess = number_range( 1, 100 );
            if ( mess < 2 )
                send_to_char
                    ( "Una gran ola hace que el barco se tambalee.\r\n",
                      ch );
            return;
        }

        /*
         * Check swim or sink. No damage until underwater. Aqua breath will reduce damage 
         * to 0.  
         */

        int                     mov;
        int                     swim = ch->pcdata->learned[gsn_swim] - number_range( 1,
                                                                                     ( ch->pcdata->
                                                                                       learned
                                                                                       [gsn_swim] /
                                                                                       2 ) );

        if ( ch->move > 0 ) {                          /* With moves, player can swim or
                                                        * * struggle to swim */
            mov = number_range( ch->max_move - 5, ch->max_move - 15 );
            mov = UMAX( 1, mov );

            if ( ch->position == POS_SLEEPING && !IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) {
                send_to_char( "&R¡el cansancio te supera y no puedes evitar hundirte y tragar agua!&D\r\n",
                              ch );
                dam = number_range( ch->max_hit / 20, ch->max_hit / 15 );
                dam = UMAX( 1, dam );
                damage( ch, ch, dam, TYPE_UNDEFINED );
                ch->move += 5;
                return;
            }
            if ( swim > 90 ) {
                mov /= 10;
                if ( number_bits( 2 ) == 0 )
                    switch ( number_range( 1, 3 ) ) {
                        default:
                            break;

                        case 1:
                            send_to_char
                                ( "&CTe colocas boca arriba y comienzas a nadar de espalda.&D\r\n",
                                  ch );
                            learn_from_success( ch, gsn_swim );
                            break;

                        case 2:
                            send_to_char
                                ( "&CCon ambos brazos frente a ti, comienzas a nadar con confianza usando el estilo mariposa.&D\r\n",
                                  ch );
                            learn_from_success( ch, gsn_swim );
                            break;

                        case 3:
                            send_to_char
                                ( "&CComienzas a nadar a braza.&D\r\n",
                                  ch );
                            learn_from_success( ch, gsn_swim );
                            break;
                    }
            }
            else if ( swim > 70 ) {
                mov /= 8;
                if ( number_bits( 3 ) == 0 ) {
                    send_to_char
                        ( "&CNadas en el agua con confianza.&D\r\n",
                          ch );
                    learn_from_success( ch, gsn_swim );
                }
            }
            else if ( swim > 50 ) {
                mov /= 6;
                if ( number_bits( 3 ) == 0 ) {
                    send_to_char( "&CNadas bastante bien por el agua.&D\r\n", ch );
                    learn_from_success( ch, gsn_swim );
                }
            }
            else if ( swim > 30 ) {
                mov /= 4;
                if ( number_bits( 3 ) == 0 ) {
                    send_to_char( "&CTe las arreglas para nadar medio decentemente.&D\r\n", ch );
                    learn_from_success( ch, gsn_swim );
                }
            }
            else if ( swim > 10 ) {
                mov /= 3;
                if ( number_bits( 2 ) == 0 ) {
                    send_to_char
                        ( "&CEres un poco torpe, pero consigues nadar.&D\r\n",
                          ch );
                    learn_from_success( ch, gsn_swim );
                }
            }
            else if ( swim > 0 ) {
                mov /= 2;
                if ( number_bits( 2 ) == 0 ) {
                    send_to_char
                        ( "&CLuchas para mantener la cabeza fuera de la superficie del agua.&D\r\n",
                          ch );
                    learn_from_failure( ch, gsn_swim );
                }
            }
            else {                                     /* Eep - drowning! */

                if ( number_bits( 2 ) == 0 ) {
                    send_to_char( "&C¡Chapoteas con furia pero solo produces espuma!&D\r\n",
                                  ch );
                    learn_from_failure( ch, gsn_swim );
                }
            }
            mov = number_range( 5, 15 );

            if ( ch->race == RACE_DRAGON && ch->Class == CLASS_BLUE ) {
                mov /= 2;
            }

            if ( ch->morph != NULL && !str_cmp( ch->morph->morph->name, "fish" ) ) {
                mov /= 2;
                mov /= 2;
            }

            if ( ch->move - mov < 0 )
                ch->move = 0;
            else
                ch->move -= mov;
        }
        else {                                         /* No moves, start damaging hp * * 
                                                        * (only a little - more chance to * * *
                                                        * sink) */
            int                     inroom = ch->in_room->vnum;

            water_sink( ch, 0 );

            if ( IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) {
                if ( number_bits( 3 ) == 0 )
                    send_to_char
                        ( "&CSientes pánico al hundirte bajo el agua, pero te las arreglas para respirar bajo esta con facilidad.&D\r\n",
                          ch );
                return;
            }

            if ( ch->in_room->vnum == inroom ) {       /* Character didn't sink */
                if ( number_bits( 1 ) == 0 )
                    send_to_char
                        ( "&R¡el cansancio te supera y no puedes evitar tragar agua!&D\r\n", ch );
                dam = number_range( ch->max_hit / 20, ch->max_hit / 15 );
                dam = UMAX( 1, dam );
                damage( ch, ch, dam, TYPE_UNDEFINED );
                ch->move += 5;
            }
            else
                send_to_char
                    ( "&C¡El cansancio te supera y contienes la respiración hundiéndote bajo el agua!&D\r\n",
                      ch );

            return;
        }
    }
    else {                                             /* Players are underwater (and * * 
                                                        * drowning). Swim has no affect. */

        if ( IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) {
            if ( number_bits( 5 ) == 0 )
                send_to_char( "&CRespiras con facilidad bajo el agua.&D\r\n", ch );
            return;
        }

        if ( ch->pcdata->holdbreath >= maxbreath ) {
            ch->pcdata->holdbreath = maxbreath;
            if ( number_bits( 1 ) == 0 )
                water_sink( ch, 1 );
            send_to_char( "&C¡Te ahogas bajo el agua! &R¡No puedes respirar!&D\r\n", ch );
            dam = number_range( ch->max_hit / 20, ch->max_hit / 10 );
            damage( ch, ch, dam, TYPE_UNDEFINED );
        }

        if ( ( percentage > 50 && number_bits( 1 ) == 0 ) || number_bits( 2 ) == 0 ) {
            breath_msg( ch, percentage );

            if ( number_bits( 2 ) == 0 ) {
                if ( percentage > 90 )
                    send_to_char( "&R¡Necesitas subir a la superficie para tomar aire!!&D\r\n", ch );
                else if ( percentage > 80 )
                    send_to_char( "&C¡Necesitas subir a la superficie para tomar aire!&D\r\n", ch );
                else if ( percentage > 70 )
                    send_to_char( "&CNecesitas tomar aire.&D\r\n", ch );
                else if ( percentage > 20 )
                    send_to_char
                        ( "&CEmpiezas a cansarte. Deberías subir a la soperficie a tomar aire.&D\r\n", ch );
                else if ( percentage == 0 )
                    send_to_char( "&CTienes un montón de aire en los pulmones.\r\n", ch );
            }
        }
    }
    return;
}

void hint_update(  )
{
    DESCRIPTOR_DATA        *d;

    if ( time_info.hour % 1 == 0 ) {
        for ( d = first_descriptor; d; d = d->next ) {
            if ( d->connected == CON_PLAYING && IS_AWAKE( d->character ) && d->character->pcdata ) {
                if ( IS_SET( d->character->pcdata->flags, PCFLAG_HINTS ) && number_bits( 1 ) == 0 ) {
                    if ( d->character->level > 5 )
                        REMOVE_BIT( d->character->pcdata->flags, PCFLAG_HINTS );
                    else
                        ch_printf_color( d->character, "\r\n&CConsejo: &c%s\r\n",
                                         get_hint( d->character->level ) );
                }
            }
        }
    }
    return;
}
