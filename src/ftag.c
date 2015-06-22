/***************************************************************************
* Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer, *
* Michael Seifert, Hans Henrik Stærfeldt, Tom Madsen, and Katja Nyboe. *
*  *
* Merc Diku Mud improvements copyright (C) 1992, 1993 by Michael *
* Chastain, Michael Quan, and Mitchell Tse. *
* *
* ROM 2.4 is copyright 1993-1995 Russ Taylor *
* ROM has been brought to you by the ROM consortium *
* Russ Taylor (rtaylor@pacinfo.com) *
* Gabrielle Taylor (gtaylor@pacinfo.com) *
* Brian Moore (rom@rom.efn.org) *
* ROT 2.0 is copyright 1996-1999 by Russ Walsh *
* DRM 1.0a is copyright 2000-2002 by Joshua Chance Blackwell *
* SD 2.0 is copyright 2004-2006 by Patrick Mylund Nielsen *
***************************************************************************/

/***************************************************************************
* Automated Freeze Tag Code *
* Markanth : dlmud@dlmud.com *
* Devil's Lament : dlmud.com port 3778 *
* Web Page : http://www.dlmud.com *
* *
* Provides automated freeze tag games in an area. *
* Code orginally done by Nebseni of Clandestine MUD *
* <clandestine.mudnet.net:9476>. *
* *
* All I ask in return is that you give me credit on your mud somewhere *
* or email me if you use it. *
****************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/ftag.h"

void                    arena_chan( char *argument );
TAG_DATA                tag_game;
extern bool             arena_underway;
extern bool             arena_prep;
extern bool             challenge;

void do_red( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA        *d,
                           *d_next;
    char                    buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    if ( tag_game.status == TAG_OFF ) {
        send_to_char( "No hay ningún evento de congelación.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Sintaxis: rojo <mensaje>\r\n", ch );
        return;
    }

    if ( !IS_IMMORTAL( ch ) && !xIS_SET( ch->act, PLR_PLAYING )
         && !xIS_SET( ch->act, PLR_WAITING ) ) {
        send_to_char( "Necesitas estar en un evento de colgelación para usar este canal.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_BLUE ) ) {
        send_to_char( "Debes estar en el equipo rojo para usar este canal.\r\n", ch );
        return;
    }

    sprintf( buf, "&RROJO &W %s: %s\r\n", ch->name, argument );
    for ( d = first_descriptor; d; d = d_next ) {
        d_next = d->next;

        if ( d->connected != CON_PLAYING || !d->character || IS_NPC( d->character ) )
            continue;
        if ( !IS_IMMORTAL( d->character ) && !xIS_SET( d->character->act, PLR_RED ) )
            continue;
        send_to_char( buf, d->character );
    }
}

void do_blue( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA        *d,
                           *d_next;
    char                    buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    if ( tag_game.status == TAG_OFF ) {
        send_to_char( "No hay ningún evento de congelación.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Sintaxis: azul <mensaje>\r\n", ch );
        return;
    }

    if ( !IS_IMMORTAL( ch ) && !xIS_SET( ch->act, PLR_PLAYING )
         && !xIS_SET( ch->act, PLR_WAITING ) ) {
        send_to_char( "Debes estar en un evento de congelación para usar este canal.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_RED ) ) {
        send_to_char( "Debes estar en el equipo azul para usar este canal.\r\n", ch );
        return;
    }

    sprintf( buf, "&CAZUL&W %s: %s\r\n", ch->name, argument );
    for ( d = first_descriptor; d; d = d_next ) {
        d_next = d->next;

        if ( d->connected != CON_PLAYING || !d->character || IS_NPC( d->character ) )
            continue;
        if ( !IS_IMMORTAL( d->character ) && !xIS_SET( d->character->act, PLR_BLUE ) )
            continue;
        send_to_char( buf, d->character );
    }
}

void end_tag( void )
{
    DESCRIPTOR_DATA        *d,
                           *d_next;

    tag_game.status = TAG_OFF;
    tag_game.timer = -1;
    tag_game.next = number_range( 30, 50 );
    tag_game.playing = 0;

    for ( d = first_descriptor; d; d = d_next ) {
        d_next = d->next;

        if ( d->connected != CON_PLAYING || !d->character || !d->character->in_room )
            continue;
        if ( d->character->in_room->vnum < FTAG_MIN_VNUM - 1
             || d->character->in_room->vnum > FTAG_MAX_VNUM )
            continue;
        xREMOVE_BIT( d->character->act, PLR_RED );
        xREMOVE_BIT( d->character->act, PLR_BLUE );
        xREMOVE_BIT( d->character->act, PLR_FROZEN );
        xREMOVE_BIT( d->character->act, PLR_PLAYING );
        xREMOVE_BIT( d->character->act, PLR_WAITING );
        xREMOVE_BIT( d->character->act, PLR_PKSAFE );
        char_from_room( d->character );
        char_to_room( d->character, get_room_index( ROOM_VNUM_TEMPLE ) );
        do_look( d->character, ( char * ) "auto" );
        act( AT_TELL, "¡$n cae del cielo!\r\n", d->character, NULL, NULL, TO_ROOM );
        d->character->quest_curr += 10;
        d->character->quest_accum += 10;
        ch_printf( d->character,
                   "¡Ganas 10 puntos de gloria por participar en el evento de congelación!\r\n" );
    }
}

void start_tag( void )
{
    DESCRIPTOR_DATA        *d,
                           *d_next;
    ROOM_INDEX_DATA        *loc;
    char                    buf[MAX_INPUT_LENGTH];
    int                     count = 0;

    tag_game.status = TAG_ISPLAY;
    tag_game.timer = 2 * tag_game.playing;

    for ( d = first_descriptor; d; d = d_next ) {
        d_next = d->next;

        if ( d->connected != CON_PLAYING || !d->character || IS_NPC( d->character ) )
            continue;

        if ( xIS_SET( d->character->act, PLR_WAITING ) ) {
            count++;
            loc = get_room_index( number_range( FTAG_MIN_VNUM, FTAG_MAX_VNUM ) );
            xREMOVE_BIT( d->character->act, PLR_FROZEN );
            xREMOVE_BIT( d->character->act, PLR_WAITING );
            xSET_BIT( d->character->act, PLR_PLAYING );
            char_from_room( d->character );
            char_to_room( d->character, loc );
            do_look( d->character, ( char * ) "auto" );
        }
    }
    sprintf( buf, "&R[&WEvento de congelación&R] &C¡El evento va a comenzar! hay &Y%d&C personas jugando.", count );
    tag_channel( NULL, buf );
}

bool                    fRed = FALSE;

void check_team_frozen( CHAR_DATA *ch )
{
    DESCRIPTOR_DATA        *d,
                           *d_next;
    char                    buf[MAX_STRING_LENGTH];
    bool                    cblue = FALSE,
        cred = FALSE;

    if ( IS_NPC( ch ) || !ch->pcdata )
        return;

    if ( xIS_SET( ch->act, PLR_BLUE ) )
        cblue = TRUE;
    else if ( xIS_SET( ch->act, PLR_RED ) )
        cred = TRUE;
    else
        return;

    for ( d = first_descriptor; d; d = d_next ) {
        d_next = d->next;

        if ( d->connected != CON_PLAYING || !d->character || IS_NPC( d->character ) )
            continue;
        if ( !xIS_SET( d->character->act, PLR_PLAYING ) )
            continue;
        if ( cblue && !xIS_SET( d->character->act, PLR_BLUE ) )
            continue;
        if ( cred && !xIS_SET( d->character->act, PLR_RED ) )
            continue;
        if ( !xIS_SET( d->character->act, PLR_FROZEN ) )    /* At least one person on the 
                                                             * team isn't * frozen */
            return;
    }

    /*
     * If we made it this far then everyone on the team has been frozen 
     */
    snprintf( buf, sizeof( buf ), "&R[&WEvento de congelación&R] &WEl equipo %s &Wha ganado!!!",
              cblue ? "&RROJO" : "&CAZUL" );
    tag_channel( NULL, buf );
    end_tag(  );
}

void do_ftag( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    ROOM_INDEX_DATA        *loc;
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) )
        return;

    argument = one_argument( argument, arg1 );
    if ( !arg1 || arg1[0] == '\0' ) {
        send_to_char( "sintaxis: congelacion entrar\r\n", ch );
        send_to_char( "para comunicarte usa los canales rojo o azul.\r\n", ch );
        send_to_char( "para congelar o descongelar a alguien una vez haya empezado el evento usa el comando tocar.\r\n", ch );
        if ( IS_IMMORTAL( ch ) ) {
            send_to_char( "\r\nSintaxis: congelacion reset\r\n", ch );
            send_to_char( " congelacion next\r\n", ch );
            send_to_char( " congelacion start\r\n", ch );
            send_to_char( " congelacion rojo <player>\r\n", ch );
            send_to_char( " congelacion azul <player>\r\n", ch );
        }
        return;
    }

    if ( arena_prep || challenge || arena_underway ) {
        send_to_char( "Espera a que termine el evento actual de arena.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "entrar" ) ) {
        if ( tag_game.status != TAG_ISWAIT ) {
            send_to_char( "no hay ningún evento de congelación activo.\r\n", ch );
            return;
        }
        if ( xIS_SET( ch->act, PLR_PLAYING ) || xIS_SET( ch->act, PLR_WAITING ) ) {
            send_to_char( "ya estás jugando.\r\n", ch );
            return;
        }
        if ( !( loc = get_room_index( FTAG_WAIT_ROOM ) ) ) {
            send_to_char( "Espera a que termine el que ya hay empezado.\r\n", ch );
            return;
        }
        send_to_char( "Entras al evento de congelación.\r\n", ch );
        char_from_room( ch );
        char_to_room( ch, loc );
        tag_game.playing += 1;
        do_look( ch, ( char * ) "auto" );
        xSET_BIT( ch->act, PLR_WAITING );
        xREMOVE_BIT( ch->act, PLR_FROZEN );
        xSET_BIT( ch->act, PLR_PKSAFE );
        ch->quest_curr += 5;
        ch->quest_accum += 5;
        ch_printf( ch, "¡Ganas 5 puntos de gloria por participar en el evento!\r\n" );
        if ( ( fRed = !fRed ) ) {
            xSET_BIT( ch->act, PLR_RED );
            xREMOVE_BIT( ch->act, PLR_BLUE );
            send_to_char( "&W¡formas parte del equipo &RROJO&W!\r\n", ch );
            tag_channel( ch, "&R[&WEvento de congelación&R] &W¡$n forma parte del equipo &RROJO&W!" );
        }
        else {
            xSET_BIT( ch->act, PLR_BLUE );
            xREMOVE_BIT( ch->act, PLR_RED );
            send_to_char( "&W¡formas parte del equipo &CAZUL&W!\r\n", ch );
            tag_channel( ch, "&R[&WEvento de congelación&R] &W¡$n forma parte del equipo &CAZUL&W!" );
        }
        return;
    }

    if ( !str_cmp( arg1, "start" ) ) {
        if ( GET_MONEY( ch, CURR_SILVER ) < 10 ) {
            send_to_char
                ( "necesitas 10 monedas de plata para comenzar el evento.\r\n", ch );
            return;
        }
        if ( tag_game.status != TAG_OFF ) {
            send_to_char( "ya hay un juego en progreso.\r\n", ch );
            return;
        }
        GET_MONEY( ch, CURR_SILVER ) -= 10;

        tag_channel( NULL,
                     "&R[&WEvento&R] &C¡Ha comenzado un evento de congelación! ¡Teclea '&Wcongelacion entrar&C' para unirte!" );
        tag_game.status = TAG_ISWAIT;
        tag_game.timer = 3;
        tag_game.playing = 0;
        tag_game.next = -1;

        if ( !IS_IMMORTAL( ch ) ) {
            GET_MONEY( ch, CURR_SILVER ) -= 10;
            return;
        }
        return;
    }

    if ( !IS_IMMORTAL( ch ) ) {
        do_ftag( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg1, "next" ) ) {
        tag_game.next = atoi( argument );
        ch_printf( ch, "El próximo juego comenzará en %d ticks.\r\n", tag_game.next );
        return;
    }

    if ( !str_cmp( arg1, "reset" ) ) {
        end_tag(  );
        send_to_char( "Jugadores reseteados.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg2 );
    if ( arg2[0] == '\0' || ( str_cmp( arg1, "rojo" ) && str_cmp( arg1, "azul" ) ) ) {
        send_to_char( "sintaxis: congelacion rojo  <player>\r\n", ch );
        send_to_char( "        Congelacion azul <player>\r\n", ch );
        return;
    }

    if ( tag_game.status == TAG_ISPLAY ) {
        send_to_char( "El juego ya ha comenzado.\r\n", ch );
        return;
    }

    if ( !( victim = get_char_world( ch, arg2 ) ) ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "No puede jugar.\r\n", ch );
        return;
    }

    if ( !( loc = get_room_index( FTAG_WAIT_ROOM ) ) ) {
        send_to_char( "El evento de congelación todavía no ha finalizado.\r\n", ch );
        return;
    }

    send_to_char( "Alguien te une al evento de congelación.\r\n", victim );
    char_from_room( victim );
    char_to_room( victim, loc );
    tag_game.playing += 1;
    do_look( victim, ( char * ) "auto" );

    xSET_BIT( victim->act, PLR_WAITING );
    xREMOVE_BIT( victim->act, PLR_FROZEN );
    xSET_BIT( victim->act, PLR_PKSAFE );

    if ( !str_cmp( arg1, "rojo" ) ) {
        xSET_BIT( victim->act, PLR_RED );
        xREMOVE_BIT( victim->act, PLR_BLUE );
        act( AT_WHITE, "¡Formas parte del equipo &RROJO&W!", ch, NULL, victim, TO_VICT );
        act( AT_WHITE, "¡$N forma parte del equipo &RROJO&W!", ch, NULL, victim, TO_NOTVICT );
        act( AT_WHITE, "¡$N forma parte del equipo &RROJO&W!", ch, NULL, victim, TO_CHAR );
    }
    else if ( !str_cmp( arg1, "blue" ) ) {
        xSET_BIT( victim->act, PLR_BLUE );
        xREMOVE_BIT( victim->act, PLR_RED );
        act( AT_WHITE, "¡Formas parte del equipo &Cazul&W!", ch, NULL, victim, TO_VICT );
        act( AT_WHITE, "¡$N forma parte del equipo &CAZUL&W!", ch, NULL, victim, TO_NOTVICT );
        act( AT_WHITE, "¡$N forma parte del equipo &CAZUL&W!", ch, NULL, victim, TO_CHAR );
    }
}

void do_tag( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    DESCRIPTOR_DATA        *d;
    char                    buf[MAX_STRING_LENGTH];
    char                    arg[MAX_INPUT_LENGTH];

    if ( IS_NPC( ch ) || ch->fighting != NULL ) {
        send_to_char( "Deja de pelear primero.\r\n", ch );
        return;
    }

    if ( tag_game.status == TAG_OFF ) {
        send_to_char( "No hay evento de congelación activo.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( !xIS_SET( ch->act, PLR_PLAYING ) ) {
        send_to_char( "No estás jugando ningún evento de congelación.\r\n", ch );
        return;
    }

    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "¡congelar a quién?\r\n", ch );
        return;
    }

    if ( !( victim = get_char_room( ch, arg ) ) ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "Claro, claaaro...\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "Eso es un bicho.\r\n", ch );
        return;
    }

    if ( !xIS_SET( victim->act, PLR_PLAYING ) ) {
        send_to_char( "No está jugando el evento de congelación.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_FROZEN ) ) {
        send_to_char( "¡No puedes tocar, necesitas descongelarte!\r\n", ch );
        return;
    }

    act( AT_GREEN, "$n te toca.", ch, NULL, victim, TO_VICT );
    act( AT_GREEN, "$n toca a $N.", ch, NULL, victim, TO_NOTVICT );
    act( AT_GREEN, "Tocas a $N.", ch, NULL, victim, TO_CHAR );

    if ( ( xIS_SET( ch->act, PLR_RED ) && xIS_SET( victim->act, PLR_RED ) )
         || ( xIS_SET( ch->act, PLR_BLUE ) && xIS_SET( victim->act, PLR_BLUE ) ) ) {
        if ( xIS_SET( victim->act, PLR_FROZEN ) ) {
            xREMOVE_BIT( victim->act, PLR_FROZEN );
            act( AT_YELLOW, "¡Te descongelas!", ch, NULL, victim, TO_VICT );
            act( AT_YELLOW, "¡$N se descongela!", ch, NULL, victim, TO_ROOM );
            act( AT_YELLOW, "¡$N se descongela!", ch, NULL, victim, TO_CHAR );
            snprintf( buf, MIL, "[&WEvento&R] %s &Cacaba de  &Ydescongelar &Ra %s&C!!!\r\n", ch->name,
                      victim->name );
            arena_chan( buf );
        }
        else
            act( AT_YELLOW, "¡$N se congela!", ch, NULL, victim, TO_CHAR );
    }
    else {
        if ( xIS_SET( victim->act, PLR_FROZEN ) )
            act( AT_YELLOW, "¡$N ya está!", ch, NULL, victim, TO_CHAR );
        else {
            xSET_BIT( victim->act, PLR_FROZEN );
            act( AT_YELLOW, "¡Te han congelado!", ch, NULL, victim, TO_VICT );
            act( AT_YELLOW, "¡$N se congela!", ch, NULL, victim, TO_NOTVICT );
            act( AT_YELLOW, "¡$N ha sido congelado!", ch, NULL, victim, TO_CHAR );
            snprintf( buf, MIL, "[&WEvento&R] %s &Cacaba de &Ccongelar &Ra %s&C!!!\r\n", ch->name,
                      victim->name );
            arena_chan( buf );
            check_team_frozen( victim );
        }
    }
}

void auto_tag( void )
{
    DESCRIPTOR_DATA        *d,
                           *d_next;
    CHAR_DATA              *ch;
    int                     count = 0;

    // Had an instance of auto_tag invoking during arena. Trying to 
    // prevent here. -Taon
    if ( arena_prep || challenge || arena_underway )
        return;

    if ( tag_game.status != TAG_OFF )
        return;

    for ( d = first_descriptor; d; d = d_next ) {
        d_next = d->next;

        if ( d->connected == CON_PLAYING
             && ( ch = ( d->original ? d->original : d->character ) ) != NULL ) {
            ++count;
        }
    }
    if ( count < 8 )
        return;
    tag_channel( NULL,
                 "&R[&WEvent&R] &C¡Ha comenzado un evento de congelación! ¡Teclea congelacion entrar para jugar!" );
    tag_game.status = TAG_ISWAIT;
    tag_game.timer = 3;
    tag_game.playing = 0;
    tag_game.next = -1;
}

void tag_update( void )
{
    char                    buf[MAX_STRING_LENGTH];

    if ( tag_game.next > 0 && tag_game.status == TAG_OFF ) {
    }
    else if ( tag_game.status == TAG_ISWAIT ) {
        tag_game.timer--;

        if ( tag_game.timer > 0 ) {
            sprintf( buf, "&R[&WEvento de congelación&R] &C%d minuto%s para comenzar.",
                     tag_game.timer, tag_game.timer == 1 ? "" : "s" );
            tag_channel( NULL, buf );
        }
        else {
            if ( tag_game.playing < 2 ) {
                end_tag(  );
                sprintf( buf,
                         "&R[&WEvento de congelación&R] &Cno hay suficientes personas para jugar. Juego cancelado." );
                tag_channel( NULL, buf );
                return;
            }
            else
                start_tag(  );
        }
    }
    else if ( tag_game.status == TAG_ISPLAY ) {
        if ( tag_game.playing == 0 ) {
            end_tag(  );
            sprintf( buf,
                     "&R[&WEvento de congelación&R] &CNo Nadie juega esta partida, siguiente juego en %d minutos.",
                     tag_game.next );
            tag_channel( NULL, buf );
            return;
        }
        switch ( tag_game.timer ) {
            case 0:
                end_tag(  );
                sprintf( buf,
                         "&R[&WEvento de congelación&R] &Ctiempo para el próximo juego: %d minutos.",
                         tag_game.next );
                tag_channel( NULL, buf );
                return;

            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 10:
            case 15:
                sprintf( buf, "&R[&WEvento de congelación&R]&Y %d &C minuto%s restantes.",
                         tag_game.timer, tag_game.timer > 1 ? "s" : "" );
                tag_channel( NULL, buf );
            default:
                tag_game.timer--;
                break;
        }
    }
}

bool is_tagging( CHAR_DATA *ch )
{
    if ( !ch || IS_NPC( ch ) )
        return FALSE;

    if ( xIS_SET( ch->act, PLR_PLAYING ) && xIS_SET( ch->act, PLR_WAITING )
         && tag_game.status != TAG_OFF )
        return TRUE;

    return FALSE;
}

void tag_channel( CHAR_DATA *ch, const char *message )
{
    DESCRIPTOR_DATA        *d,
                           *d_next;
    CHAR_DATA              *dch;
    char                    buf[MAX_INPUT_LENGTH];

    sprintf( buf, "%s", message );

    for ( d = first_descriptor; d; d = d_next ) {
        d_next = d->next;

        if ( d->connected != CON_PLAYING )
            continue;

        if ( !( dch = d->character ) )
            continue;

        if ( ch )                                      /* don't use $N only $n in message 
                                                        */
            act( AT_CYAN, buf, ch, NULL, dch, TO_VICT );
        else {
            send_to_char( buf, dch );
            send_to_char( "\r\n", dch );
        }
    }
}
