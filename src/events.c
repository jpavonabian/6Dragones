/***************************************************************************
*   Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*   Michael Seifert, Hans Henrik St√¶rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                          *
*   Merc Diku Mud improvements copyright (C) 1992, 1993 by Michael         *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*                                                                          *
*       ROM 2.4 is copyright 1993-1995 Russ Taylor                         *
*       ROM has been brought to you by the ROM consortium                  *
*           Russ Taylor (rtaylor@pacinfo.com)                              *
*           Gabrielle Taylor (gtaylor@pacinfo.com)                         *
*           Brian Moore (rom@rom.efn.org)                                  *
*       ROT 2.0 is copyright 1996-1999 by Russ Walsh                       *
*       DRM 1.0a is copyright 2000-2002 by Joshua Chance Blackwell         *
*        SD 2.0 is copyright 2004-2006 by Patrick Mylund Nielsen           *
***************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/events.h"

bool                    happyhouron;
HOLIDAY_DATA           *get_holiday( short month, short day );
void                    auto_tag( void );
bool                    eventexp = FALSE;

// extern bool doubleexp;
/*
 * CHANCE function. I use this everywhere in my code, very handy :>
 */
bool chance( int num )
{
    if ( number_range( 1, 100 ) <= num )
        return TRUE;
    else
        return FALSE;
}

bool bigchance( int num )
{
    if ( number_range( 1, 1500 ) * 2 <= num )
        return TRUE;
    else
        return FALSE;
}

WOLLA( dailyevents )
{
    HOLIDAY_DATA           *holiday;
    short                   day;

    day = time_info.day + 1;

    /*
     * Basic events that have their own chance check 
     */
    holiday = get_holiday( time_info.month, day - 1 );
/*
   if( holiday != NULL && !doubleexp )
  {

      do_dexp( NULL, "" );
    starteventdoubleexp = current_time; 
   }
   else if ( !doubleexp && !eventexp )
   {
      event_doubleexp( );
      event_happyhour( );
   }
  */

    if ( holiday != NULL ) {
        event_happyhour(  );
    }

    /*
     * Random pill and angel/demon token spreading 
     */
    random_spread(  );

    /*
     * Auto Freezetag Game 
     */
    if ( bigchance( 5 ) )
        auto_tag(  );

    if ( chance( 1 ) )
        happymoment(  );

    /*
     * City-wide plauge 
     */
/*
   if( bigchance( 1 ) )
      event_plauge(  );
*/
}

WOLLA( event_doubleexp )
{
    CHAR_DATA              *ch;

    if ( ( !eventexp ) && ( chance( 5 ) ) ) {
        eventexp = TRUE;
        echo_to_all( AT_RED, "[&WEvento&R] &C°Es tiempo de doble experiencia! °A matar!\r\n",
                     ECHOTAR_ALL );
    }
    else if ( ( eventexp ) && ( chance( 8 ) ) ) {
        eventexp = FALSE;
        echo_to_all( AT_RED, "[&WEvento&R] &CSe acabÛ la doble experiencia.\r\n", ECHOTAR_ALL );
    }
}

WOLLA( event_happyhour )
{
    CHAR_DATA              *ch;

    if ( ( !happyhouron ) && ( chance( 3 ) ) ) {
        happyhouron;
        echo_to_all( AT_RED, "[&WEvento&R] &C°ha llegado la hora feliz! °Agrupad todos!\r\n",
                     ECHOTAR_ALL );
        return;
    }
    else if ( ( happyhouron ) && ( chance( 3 ) ) ) {
        !happyhouron;
        echo_to_all( AT_RED, "[&Wevento&R] &CSe acabÛ la hora feliz.\r\n", ECHOTAR_ALL );
        return;
    }
}

WOLLA( happymoment )
{
    CHAR_DATA              *ch;
    DESCRIPTOR_DATA        *d;
    long                    happymomentbonus;

    for ( d = first_descriptor; d; d = d->next ) {
        if ( d->connected == CON_PLAYING
             && ( ch = ( d->original ? d->original : d->character ) ) != NULL ) {
            if ( !IS_NPC( ch ) && !xIS_SET( ch->act, PLR_AFK ) ) {
                send_to_char
                    ( "&Y°La administraciÛn de 6 Dragones te da las gracias por jugar!\r\n\r\n",
                      ch );

                if ( ( ch->level < LEVEL_AVATAR ) ) {
                    if ( chance( 70 ) ) {
                        happymomentbonus = number_range( 10, 10000 );
                        gain_exp( ch, happymomentbonus );

                        printf_to_char( ch, "&W°Experiencia gratis! &r(&R%ld&r)\r\n", happymomentbonus );
                    }
                    else
                        send_to_char( "&WEsta vez no hay premio de experiencia.\r\n", ch );
                }
                if ( xIS_SET( ch->act, PLR_RP ) && chance( 60 ) ) {
                    ch_printf( ch,
                               "&W°Ganas 5 puntos de gloria por tu participaciÛn en el rol!&D\r\n" );
                    ch->quest_curr += 5;
                }
                if ( chance( 60 ) && !xIS_SET( ch->act, PLR_RP ) ) {
                    happymomentbonus = number_range( 1, 300 );
                    GET_MONEY( ch, CURR_SILVER ) += happymomentbonus;
                    printf_to_char( ch, "&W°Ganas &r(&R%ld&r)&w monedas de plata!\r\n",
                                    happymomentbonus );
                }
                else
                    send_to_char( "&WEsta vez no ganas monedas de plata.\r\n", ch );
                if ( ch->hit < ch->max_hit )
                    ch->hit = ch->max_hit;
                if ( ch->move < ch->max_move )
                    ch->move = ch->max_move;
                send_to_char( "&Y°Te han restaurado!&D\r\n",
                              d->character );
            }
        }
    }
}

WOLLA( random_spread )
{
    OBJ_DATA               *pill = NULL;
    ROOM_INDEX_DATA        *room = NULL;
    AREA_DATA              *pArea = NULL;
    int                     tried = 0;

    if ( bigchance( 40 ) ) {
        for ( ;; ) {
            if ( ++tried == 10000 ) {
                room = NULL;
                pArea = NULL;
                break;
            }
            if ( !( room = get_room_index( number_range( 100, 41000 ) ) ) )
                continue;
            pArea = room->area;
            if ( !IS_SET( pArea->flags, AFLAG_UNOTSEE ) )
                break;
        }
        if ( room ) {
            if ( ( pill = create_object( get_obj_index( OBJ_VNUM_QUEST_PILL ), 0 ) ) ) {
                pill->timer = 340;
                obj_to_room( pill, room );
            }
        }

        room = NULL;
        pArea = NULL;
        tried = 0;

        if ( bigchance( 1 ) && ( time_info.hour == 2 ) ) {
            for ( ;; ) {
                if ( ++tried == 10000 ) {
                    room = NULL;
                    pArea = NULL;
                    break;
                }
                if ( !( room = get_room_index( number_range( 100, 41000 ) ) ) )
                    continue;
                pArea = room->area;
                if ( !IS_SET( pArea->flags, AFLAG_UNOTSEE ) )
                    break;
            }
        }
        if ( room ) {
            if ( ( pill = create_object( get_obj_index( OBJ_VNUM_DEMON_TOKEN_FOUND ), 0 ) ) )
                obj_to_room( pill, room );
        }
    }

    room = NULL;
    pArea = NULL;
    tried = 0;

    if ( bigchance( 1 ) && ( time_info.hour == 6 ) ) {
        for ( ;; ) {
            if ( ++tried == 10000 ) {
                room = NULL;
                pArea = NULL;
                break;
            }
            if ( !( room = get_room_index( number_range( 100, 41000 ) ) ) )
                continue;
            pArea = room->area;
            if ( !IS_SET( pArea->flags, AFLAG_UNOTSEE ) )
                break;
        }
        if ( room ) {
            if ( ( pill = create_object( get_obj_index( OBJ_VNUM_ANGEL_TOKEN_FOUND ), 0 ) ) )
                obj_to_room( pill, room );
        }
    }
}
