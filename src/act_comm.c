/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Player communication module                                           *
 ***************************************************************************/

#include <ctype.h>
#include <string.h>
#include <time.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/new_auth.h"
#include "h/auction.h"
#include "h/languages.h"
#include "h/arena.h"
#include "h/ftag.h"

/* Externals */
extern bool             challenge;

#define QUOTE_DIR "quotes/"
extern time_t           board_expire_time_t;
bool                    MOBtrigger;
short                   prep_length;

/* Local functions. */
void                    talk_channel( CHAR_DATA *ch, char *argument, int channel,
                                      const char *verb );
char                   *scramble( const char *argument, int modifier );
char                   *drunk_speech( const char *argument, CHAR_DATA *ch );
int                     get_currency_type( char *type );

//For arena challenge -Taon
extern bool             challenge;

/* Text scrambler -- Altrag */
char                   *scramble( const char *argument, int modifier )
{
    static char             arg[MIL];
    short                   position;
    short                   conversion = 0;

    modifier %= number_range( 80, 300 );               /* Bitvectors get way too large #s
                                                        */
    for ( position = 0; position < MIL; position++ ) {
        if ( argument[position] == '\0' ) {
            arg[position] = '\0';
            return arg;
        }
        else if ( argument[position] >= 'A' && argument[position] <= 'Z' ) {
            conversion = -conversion + position - modifier + argument[position] - 'A';
            conversion = number_range( conversion - 5, conversion + 5 );
            while ( conversion > 25 )
                conversion -= 26;
            while ( conversion < 0 )
                conversion += 26;
            arg[position] = conversion + 'A';
        }
        else if ( argument[position] >= 'a' && argument[position] <= 'z' ) {
            conversion = -conversion + position - modifier + argument[position] - 'a';
            conversion = number_range( conversion - 5, conversion + 5 );
            while ( conversion > 25 )
                conversion -= 26;
            while ( conversion < 0 )
                conversion += 26;
            arg[position] = conversion + 'a';
        }
        else if ( argument[position] >= '0' && argument[position] <= '9' ) {
            conversion = -conversion + position - modifier + argument[position] - '0';
            conversion = number_range( conversion - 2, conversion + 2 );
            while ( conversion > 9 )
                conversion -= 10;
            while ( conversion < 0 )
                conversion += 10;
            arg[position] = conversion + '0';
        }
        else
            arg[position] = argument[position];
    }
    arg[position] = '\0';
    return arg;
}

LANG_DATA              *get_lang( const char *name )
{
    LANG_DATA              *lng;

    for ( lng = first_lang; lng; lng = lng->next )
        if ( !str_cmp( lng->name, name ) )
            return lng;
    return NULL;
}

/* percent = percent knowing the language. */
char                   *translate( int percent, const char *in, const char *name )
{
    LCNV_DATA              *cnv;
    static char             buf[MSL * 2];
    char                    buf2[MSL * 2];
    const char             *pbuf;
    char                   *pbuf2 = buf2;
    LANG_DATA              *lng;

    if ( percent > 99 || !str_cmp( name, "common" ) )
        return ( char * ) in;
    /*
     * If we don't know this language... use "default"
     */
    if ( !( lng = get_lang( name ) ) )
        if ( !( lng = get_lang( "default" ) ) )
            return ( char * ) in;
    for ( pbuf = in; *pbuf; ) {
        for ( cnv = lng->first_precnv; cnv; cnv = cnv->next ) {
            if ( !str_prefix( cnv->old, pbuf ) ) {
                if ( percent && ( rand(  ) % 100 ) < percent ) {
                    strncpy( pbuf2, pbuf, cnv->olen );
                    pbuf2[cnv->olen] = '\0';
                    pbuf2 += cnv->olen;
                }
                else {
                    mudstrlcpy( pbuf2, cnv->lnew, MSL * 2 );
                    pbuf2 += cnv->nlen;
                }
                pbuf += cnv->olen;
                break;
            }
        }
        if ( !cnv ) {
            if ( isalpha( *pbuf ) && ( !percent || ( rand(  ) % 100 ) > percent ) ) {
                *pbuf2 = lng->alphabet[LOWER( *pbuf ) - 'a'];
                if ( isupper( *pbuf ) )
                    *pbuf2 = UPPER( *pbuf2 );
            }
            else
                *pbuf2 = *pbuf;
            pbuf++;
            pbuf2++;
        }
    }
    *pbuf2 = '\0';
    for ( pbuf = buf2, pbuf2 = buf; *pbuf; ) {
        for ( cnv = lng->first_cnv; cnv; cnv = cnv->next )
            if ( !str_prefix( cnv->old, pbuf ) ) {
                mudstrlcpy( pbuf2, cnv->lnew, MSL * 2 );
                pbuf += cnv->olen;
                pbuf2 += cnv->nlen;
                break;
            }
        if ( !cnv )
            *( pbuf2++ ) = *( pbuf++ );
    }
    *pbuf2 = '\0';
#if 0
    for ( pbuf = in, pbuf2 = buf; *pbuf && *pbuf2; pbuf++, pbuf2++ )
        if ( isupper( *pbuf ) )
            *pbuf2 = UPPER( *pbuf2 );
    /*
     * Attempt to align spacing..
     */
        else if ( isspace( *pbuf ) )
            while ( *pbuf2 && !isspace( *pbuf2 ) )
                pbuf2++;
#endif
    return buf;
}

char                   *drunk_speech( const char *argument, CHAR_DATA *ch )
{
    const char             *arg = argument;
    static char             buf[MIL * 2];
    char                    buf1[MIL * 2];
    short                   drunk;
    char                   *txt;
    char                   *txt1;

    if ( IS_NPC( ch ) || !ch->pcdata )
        return ( char * ) argument;

    drunk = ch->pcdata->condition[COND_DRUNK];

    if ( drunk <= 0 )
        return ( char * ) argument;
    buf[0] = '\0';
    buf1[0] = '\0';
    if ( !argument ) {
        bug( "%s", "Drunk_speech: NULL argument" );
        return ( char * ) "";
    }
    txt = buf;
    txt1 = buf1;
    while ( *arg != '\0' ) {
        if ( toupper( *arg ) == 'T' ) {
            if ( number_percent(  ) < ( drunk * 2 ) ) { /* add 'h' after an 'T' */
                *txt++ = *arg;
                *txt++ = 'h';
            }
            else
                *txt++ = *arg;
        }
        else if ( toupper( *arg ) == 'X' ) {
            if ( number_percent(  ) < ( drunk * 2 / 2 ) )
                *txt++ = 'c', *txt++ = 's', *txt++ = 'h';
            else
                *txt++ = *arg;
        }
        else if ( number_percent(  ) < ( drunk * 2 / 5 ) ) {    /* slurred letters */
            short                   slurn = number_range( 1, 2 );
            short                   currslur = 0;

            while ( currslur < slurn )
                *txt++ = *arg, currslur++;
        }
        else
            *txt++ = *arg;
        arg++;
    };
    *txt = '\0';
    txt = buf;
    while ( *txt != '\0' ) {                           /* Let's mess with the string's
                                                        * caps */
        if ( number_percent(  ) < ( 2 * drunk / 2.5 ) ) {
            if ( isupper( *txt ) )
                *txt1 = tolower( *txt );
            else if ( islower( *txt ) )
                *txt1 = toupper( *txt );
            else
                *txt1 = *txt;
        }
        else
            *txt1 = *txt;

        txt1++, txt++;
    };
    *txt1 = '\0';
    txt1 = buf1;
    txt = buf;
    while ( *txt1 != '\0' ) {                          /* Let's make them stutter */
        if ( *txt1 == ' ' ) {                          /* If there's a space, then * * *
             * * * * there's gotta be a *//*
             * along there somewhere soon
             */

            while ( *txt1 == ' ' )                     /* Don't stutter on spaces */
                *txt++ = *txt1++;

            if ( ( number_percent(  ) < ( 2 * drunk / 4 ) ) && *txt1 != '\0' ) {
                short                   offset = number_range( 0, 2 );
                short                   pos = 0;

                while ( *txt1 != '\0' && pos < offset )
                    *txt++ = *txt1++, pos++;
                if ( *txt1 == ' ' ) {                  /* Make sure not to stutter a * *
                     * * * * * space after *//*
                     * the initial offset into the word
                     */
                    *txt++ = *txt1++;
                    continue;
                }
                pos = 0;
                offset = number_range( 2, 4 );
                while ( *txt1 != '\0' && pos < offset ) {
                    *txt++ = *txt1;
                    pos++;
                    if ( *txt1 == ' ' || pos == offset ) {  /* Make sure we don't stick *//* A hyphen right before a *
                                                             * * * * * space */
                        txt1--;
                        break;
                    }
                    *txt++ = '-';
                }
                if ( *txt1 != '\0' )
                    txt1++;
            }
        }
        else
            *txt++ = *txt1++;
    }
    *txt = '\0';
    return buf;
}

void do_whisper( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    char                    buf[MAX_INPUT_LENGTH];
    CHAR_DATA              *victim;
    int                     position;
    int                     speaking = -1,
        lang;

#ifndef SCRAMBLE
    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
        if ( ch->speaking & lang_array[lang] ) {
            speaking = lang;
            break;
        }
#endif

    xREMOVE_BIT( ch->deaf, CHANNEL_WHISPER );

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "¿Susurar a quién qué?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Tienes un bonito chat contigo mismo.\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) && ( victim->switched )
         && !IS_AFFECTED( victim->switched, AFF_POSSESS ) ) {
        send_to_char( "Ese jugador no está en su cuerpo.\r\n", ch );
        return;
    }
    else if ( IS_AFFECTED( ch, AFF_BURROW ) ) {
        send_to_char( "Desentiérrate primero.\r\n",
                      ch );
        return;
    }

    else if ( !IS_NPC( victim ) && ( !victim->desc ) ) {
        send_to_char( "No tiene conexión.\r\n", ch );
        return;
    }

    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_AFK ) ) {
        send_to_char( "Ese personaje está afk.\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->deaf, CHANNEL_WHISPER )
         && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) ) {
        act( AT_PLAIN, "No escucha susurros.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_SILENCE ) )
        send_to_char
            ( "Ese personaje ha sido silenciado por un inmortal. Recibirá tu mensaje pero no podrá responderlo.\r\n",
              ch );

    if ( victim->desc                                  /* make sure desc exists first
                                                        * -Thoric */
         && victim->desc->connected == CON_EDITING && get_trust( ch ) < LEVEL_IMMORTAL ) {
        act( AT_PLAIN, "Está en un búfer de escritura. Prueba en unos minutos.",
             ch, 0, victim, TO_CHAR );
        return;
    }

    /*
     * Check to see if target of tell is ignoring the sender
     */
    if ( is_ignoring( victim, ch ) ) {
        /*
         * If the sender is an imm then they cannot be ignored
         */
        if ( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) ) {
            set_char_color( AT_IGNORE, ch );
            ch_printf( ch, "%s te está ignorando.\r\n", victim->name );
            return;
        }
        else {
            set_char_color( AT_IGNORE, victim );
            ch_printf( victim, "Intentas ignorar a %s, pero te es imposible.\r\n",
                       ch->name );
        }
    }

    act( AT_WHISPER, "Susurras a $N '$t'", ch, argument, victim, TO_CHAR );
    position = victim->position;
    victim->position = POS_STANDING;
#ifndef SCRAMBLE
    if ( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) ) {
        int                     speakswell = UMIN( knows_language( victim, ch->speaking, ch ),
                                                   knows_language( ch, ch->speaking, victim ) );

        if ( speakswell < 85 )
            act( AT_WHISPER, "$n te susurra '$t'", ch,
                 translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
        else
            act( AT_WHISPER, "$n te susurra '$t'", ch, argument, victim, TO_VICT );
    }
    else
        act( AT_WHISPER, "$n te susurra '$t'", ch, argument, victim, TO_VICT );
#else
    if ( !knows_language( vch, ch->speaking, ch ) && ( !IS_NPC( ch ) || ch->speaking != 0 ) )
        act( AT_WHISPER, "$n whispers to you '$t'", ch,
             translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
    else
        act( AT_WHISPER, "$n te susurra '$t'", ch, argument, victim, TO_VICT );
#endif

    if ( !IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
        act( AT_WHISPER, "$n susurra algo a $N.", ch, argument, victim, TO_NOTVICT );

    victim->position = position;
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) ) {
        snprintf( buf, MAX_INPUT_LENGTH, "%s: %s (whisper to) %s.",
                  IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                  IS_NPC( victim ) ? victim->short_descr : victim->name );
        append_to_file( LOG_FILE, buf );
    }
    mprog_speech_trigger( argument, ch );
    return;
}

bool                    csreply = FALSE;               /* This will be to keep track of
                                                        * the tell being sent manually or
                                                        * * * * * * * a reply */

void do_tell( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    CHAR_DATA              *switched_victim = NULL;
    char                    logbuf[MSL];
    char                    buf2[MSL];
    char                    arg[MIL];
    int                     position;

#ifndef SCRAMBLE
    int                     speaking = -1,
        lang,
        x;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
        if ( ch->speaking & lang_array[lang] ) {
            speaking = lang;
            break;
        }
#endif

    if ( IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) && !IS_IMMORTAL( ch ) ) {
        send_to_char( "No puedes hacer eso aquí.\r\n", ch );
        return;
    }
    if ( !IS_NPC( ch ) && ( xIS_SET( ch->act, PLR_SILENCE ) || xIS_SET( ch->act, PLR_NO_TELL ) ) ) {
        send_to_char( "No puedes hacer eso.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_BURROW ) ) {
        send_to_char( "Desentiérrate primero.\r\n",
                      ch );
        return;
    }

    argument = one_argument( argument, arg );
        if ( !str_cmp( arg, "recordar" ) && !IS_NPC( ch ) ) {
	        send_to_char( "&CLos últimos 20 contares recibidos:\r\n", ch );
	        for ( x = 0; x < 20; x++ ) {
	            if ( ch->pcdata->tell_history[x] == NULL )
	                break;
	            ch_printf( ch, "%s &D\r\n", ch->pcdata->tell_history[x] );
	        }
	        return;
	    }
	if(!str_cmp( arg, "apagar" ) && !IS_NPC( ch ))
	{
if(	xIS_SET( ch->act, PLR_TELLOFF ))
{
				            send_to_char( "Ya tienes el canal contar apagado. Para encenderlo, escribe contar encender.\r\n", ch );
return;
}
else
{
			            send_to_char( "Apagas el canal contar.\r\n", ch );
		xSET_BIT( ch->act, PLR_TELLOFF );
		return;
	}
}
	if(!str_cmp( arg, "encender" ) && !IS_NPC( ch ))
	{
if(	!xIS_SET( ch->act, PLR_TELLOFF ))
{
				            send_to_char( "Ya tienes el canal contar encendido. Para apagarlo, escribe contar apagar.\r\n", ch );
return;
}
else
{
			            send_to_char( "Enciendes el canal contar.\r\n", ch );
		xREMOVE_BIT( ch->act, PLR_TELLOFF );
		return;
	}
}

		            if ( !VLD_STR( arg ) && !VLD_STR( argument ) && !IS_NPC( ch )) {
		            send_to_char( "Sintaxis: contar <jugador> <mensaje>\r\n", ch );
		            send_to_char( "Sintaxis: contar <recordar>\r\n", ch );
		            send_to_char( "Sintaxis: contar <apagar/encender>\r\n", ch );
return;
}
                if ( IS_NPC( ch ) && !VLD_STR( arg ) || !VLD_STR( argument ) ) {
		            send_to_char( "¿Contar qué a quién?\r\n", ch );
		            return;
		        }
        if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_TELLOFF )) {
        send_to_char( "Tienes el canal contar apagado. Para usarlo, primero deberás encenderlo con contar encender.\r\n",
                      ch );
	        return;
	    }

    if ( ( !( victim = get_char_world( ch, arg ) ) && ( !csreply || !( victim = ch->reply ) ) )
         || ( IS_NPC( victim ) && victim->in_room != ch->in_room ) ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }
    if ( ch == victim ) {
        send_to_char( "tienes un bonito chat contigo mismo.\r\n", ch );
        return;
    }
    if ( !IS_NPC( victim ) && IS_IMMORTAL( victim ) && IS_SET( victim->pcdata->flags, PCFLAG_DND )
         && !IS_IMMORTAL( ch ) && !IS_NPC( ch ) && ( !ch->reply || victim != ch->reply ) ) {
        send_to_char
            ( "          &YLa administración de &r6 Dragones&Y no acepta pribados de jugadores.\r\n", ch );
        send_to_char
            ( "&YNosotros no nos ponemos invisibles para que no nos hagan preguntas, pero pedimos\r\n"
              "que los jugadores no utilicen este método.\r\n"
              "para hacer el mud un lugar mejor.\r\n", ch );
        send_to_char
            ( "&YSi deseas hablar con un miembro de la administración, por favor, usa el canal preguntar.\r\n",
              ch );
        send_to_char( "&YSe educado: preguntar <administrador> ¿Puedes enviarme algo por telepatía, ¿por favor?\r\n",
                      ch );
        send_to_char( "&Y              -o- preguntar <administrador> ¿Puedo hablar contigo en pribado?\r\n",
                      ch );
        send_to_char( "                   &C[&Y La decisión de la administración será definitiva.&C ]\r\n", ch );
        send_to_char
            ( "&YSi no te hacemos caso, envíanos una nota usando los buzones.\r\n",
              ch );
        return;
    }
    if ( !IS_NPC( victim ) && ( victim->switched ) && ( get_trust( ch ) > LEVEL_AVATAR )
         && !IS_AFFECTED( victim->switched, AFF_POSSESS ) ) {
        send_to_char( "Ese jugador no está en su cuerpo.\r\n", ch );
        return;
    }
    else if ( !IS_NPC( victim ) && ( victim->switched )
              && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
        switched_victim = victim->switched;
    else if ( !IS_NPC( victim ) && ( !victim->desc )
              && !IS_SET( victim->pcdata->flags, PCFLAG_PUPPET ) ) {
        send_to_char( "Ese jugador no tiene conexión.\r\n", ch );
        return;
    }
    if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_NO_TELL ) ) {
        act( AT_PLAIN, "Ese jugador no puede hacer ni recibir contares, ya que le fueron desactivados por la Administración.", ch, NULL, victim, TO_CHAR );
        return;
    }
        if ( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_TELLOFF )) {
	        act( AT_PLAIN, "Ese jugador tiene el canal contar apagado.", ch, NULL, victim, TO_CHAR );
	        return;
	    }

    // No longer will players get spammy tells, after walking away. -Taon
    if ( IS_NPC( ch ) && !IS_NPC( victim ) && victim->in_room != ch->in_room ) {
        if ( ch->retell != NULL )
            ch_printf( victim, "\r\n&G%s intenta hablar contigo.&D\r\n",
                       ch->short_descr );
        ch->retell = NULL;
        return;
    }

    if ( xIS_SET( victim->act, PLR_SILENCE ) )
        send_to_char
            ( "Ese personaje ha sido silenciado por la administración. No podrá responderte, aunque hemos enviado tu mensaje.\r\n",
              ch );
    if ( ( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) )
         || ( !IS_NPC( victim ) && IS_SET( victim->in_room->room_flags, ROOM_SILENCE ) ) ) {
        act( AT_PLAIN, "No puede oírte.", ch, 0, victim, TO_CHAR );
        return;
    }
    if ( victim->desc && victim->desc->connected == CON_EDITING && get_trust( ch ) < MAX_LEVEL - 2 ) {
        act( AT_PLAIN, "Está en un búfer. Prueba en unos minutos.",
             ch, 0, victim, TO_CHAR );
        return;
    }

    if ( xIS_SET( victim->act, PLR_COMMUNICATION ) )
        send_to_char( "!!SOUND(sound/tell.wav)\r\n", victim );

    /*
     * Check to see if target of tell is ignoring the sender
     */
    if ( is_ignoring( victim, ch ) ) {
        /*
         * If the sender is an imm then they cannot be ignored
         */
        if ( !IS_IMMORTAL( ch ) || victim->level > ch->level ) {
            /*
             * Drop the command into oblivion, why tell the other guy you're ignoring them?
             */
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        else {
            set_char_color( AT_IGNORE, victim );
            ch_printf( victim, "Tratas de ignorar a %s, pero algo te lo impide.\r\n", ch->name );
        }
    }

    ch->retell = victim;
    if ( switched_victim )
        victim = switched_victim;

    MOBtrigger = FALSE;                                /* BUGFIX - do_tell: Tells were
                                                        * triggering act progs */
    act_printf( AT_TELL, ch, argument, victim, TO_CHAR, "Cuentas a $N ' $t &D%s'",
                color_str( AT_TELL, ch ) );

    if ( !IS_NPC( victim ) && !IS_NPC( ch ) ) {
        snprintf( buf2, MSL, "&W%s tells: %s %s&D", ch->name, victim->name, argument );
        append_to_file( CSAVE_FILE, buf2 );
    }

    position = victim->position;
    set_position( victim, POS_STANDING );
    if ( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) ) {
        int                     speakswell = UMIN( knows_language( victim, ch->speaking, ch ),
                                                   knows_language( ch, ch->speaking, victim ) );

        if ( speakswell < 85 )
            act_printf( AT_TELL, ch, translate( speakswell, argument, lang_names[speaking] ),
                        victim, TO_VICT, "$n te cuenta ' $t &D%s'", color_str( AT_TELL, victim ) );
        else
            act_printf( AT_TELL, ch, argument, victim, TO_VICT, "$n te cuenta ' $t &D%s'",
                        color_str( AT_TELL, victim ) );
    }
    else {
        act_printf( AT_TELL, ch, argument, victim, TO_VICT, "$n te cuenta ' $t &D%s'",
                    color_str( AT_TELL, victim ) );
    }
    if ( !IS_NPC( victim ) ) {
        time_t                  t = time( NULL );
        struct tm              *local = localtime( &t );

        snprintf( logbuf, MSL, "&R[%-2.2d/%-2.2d %-2.2d:%-2.2d] &B%s told you '&W %s &D&B'&D",
                  local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, PERS( ch,
                                                                                          victim ),
                  argument );

        for ( x = 0; x < 20; x++ ) {
            if ( victim->pcdata->tell_history[x] == NULL ) {
                victim->pcdata->tell_history[x] = STRALLOC( logbuf );
                break;
            }
            if ( x == 19 ) {
                int                     i;

                for ( i = 1; i < 20; i++ ) {
                    STRFREE( victim->pcdata->tell_history[i - 1] );
                    victim->pcdata->tell_history[i - 1] =
                        STRALLOC( victim->pcdata->tell_history[i] );
                }
                STRFREE( victim->pcdata->tell_history[x] );
                victim->pcdata->tell_history[x] = STRALLOC( logbuf );
            }
        }

        /*
         * Volk's redirect from puppets to snooping immortal code - redirect to immortal
         * descriptor if( IS_SET( victim->pcdata->flags, PCFLAG_PUPPET ) )
         * ch_printf(redirect->character, "$n tells %s ' $t &D%s'", victim->name,
         * color_str(AT_TELL, victim));
         */
    }

    if ( !IS_NPC( ch ) ) {
        time_t                  t = time( NULL );
        struct tm              *local = localtime( &t );

        snprintf( logbuf, MSL, "&R[%-2.2d/%-2.2d %-2.2d:%-2.2d] &BYou told %s '&W %s &D&B'&D",
                  local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, PERS( victim,
                                                                                          ch ),
                  argument );

        for ( x = 0; x < 20; x++ ) {
            if ( ch->pcdata->tell_history[x] == NULL ) {
                ch->pcdata->tell_history[x] = STRALLOC( logbuf );
                break;
            }
            if ( x == 19 ) {
                int                     i;

                for ( i = 1; i < 20; i++ ) {
                    STRFREE( ch->pcdata->tell_history[i - 1] );
                    ch->pcdata->tell_history[i - 1] = STRALLOC( ch->pcdata->tell_history[i] );
                }
                STRFREE( ch->pcdata->tell_history[x] );
                ch->pcdata->tell_history[x] = STRALLOC( logbuf );
            }
        }
    }

    MOBtrigger = TRUE;                                 /* BUGFIX - do_tell: Tells were
                                                        * triggering act progs */
    set_position( victim, position );
    victim->reply = ch;
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) ) {
        append_to_file_printf( LOG_FILE, "%s: %s(tell to)%s.",
                               IS_NPC( ch ) ? ch->short_descr : ch->name, argument,
                               IS_NPC( victim ) ? victim->short_descr : victim->name );
    }
    if ( IS_NPC( victim ) )
        mprog_speech_trigger( argument, ch );
}

void do_reply( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    CHAR_DATA              *victim;

    if ( !VLD_STR( argument ) ) {
        /*
         * Show tell history
         */
        do_tell( ch, ( char * ) "" );
        return;
    }
    if ( !( victim = ch->reply ) ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }
    csreply = TRUE;
    snprintf( buf, MSL, "%s %s", victim->name, argument );
    do_tell( ch, buf );
    csreply = FALSE;
}

void do_emote( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                   *plast;
    CHAR_DATA              *vch;
    EXT_BV                  actflags;

#ifndef SCRAMBLE
    int                     speaking = -1,
        lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
        if ( ch->speaking & lang_array[lang] ) {
            speaking = lang;
            break;
        }
#endif
    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_NO_EMOTE ) ) {
        send_to_char( "Eres un antisocial.\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char( "¿Emocion que?\r\n", ch );
        return;
    }
    actflags = ch->act;
    if ( IS_NPC( ch ) )
        xREMOVE_BIT( ch->act, ACT_SECRETIVE );
    for ( plast = argument; *plast != '\0'; plast++ );
    mudstrlcpy( buf, argument, MSL );
    if ( isalpha( plast[-1] ) )
        mudstrlcat( buf, ".", MSL );
    for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room ) {
        char                   *sbuf = buf;

        /*
         * Check to see if character is ignoring emoter
         */
        if ( is_ignoring( vch, ch ) ) {
            /*
             * continue unless emoter is an immortal
             */
            if ( !IS_IMMORTAL( ch ) || get_trust( vch ) > get_trust( ch ) )
                continue;
            else {
                set_char_color( AT_IGNORE, vch );
                ch_printf( vch, "Tratas de ignorar a %s, pero algo te lo impide.\r\n",
                           PERS( ch, vch ) );
            }
        }
#ifndef SCRAMBLE
        if ( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) ) {
            int                     speakswell = UMIN( knows_language( vch, ch->speaking, ch ),
                                                       knows_language( ch, ch->speaking, vch ) );

            if ( speakswell < 85 )
                sbuf = translate( speakswell, argument, lang_names[speaking] );
        }
#else
        if ( !knows_language( vch, ch->speaking, ch ) && ( !IS_NPC( ch ) && ch->speaking != 0 ) )
            sbuf = scramble( buf, ch->speaking );
#endif
        MOBtrigger = FALSE;
        act( AT_SOCIAL, "$n $t", ch, sbuf, vch, ( vch == ch ? TO_CHAR : TO_VICT ) );
    }
    ch->act = actflags;
    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) ) {
        snprintf( buf, MSL, "%s %s (emote)", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );
        append_to_file( LOG_FILE, buf );
    }
    return;
}

void do_ide( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_PLAIN, ch );
    send_to_char( "\r\nIf you want to send an idea, type 'idea submit'.\r\n", ch );
    send_to_char( "If you want to identify an object, use the identify spell.\r\n", ch );
    return;
}

void do_rent( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_WHITE, ch );
    send_to_char( "solo sal del juego.\r\n", ch );
    return;
}

/*
 * Function: load_quotes
 * Descr   : Determines how many (if any) quote files are located within
 *           QUOTE_DIR, for later use by "do_quotes".
 * Returns : (void)
 * Syntax  : (none)
 * Written : v1.0 12/97
 * Author  : Gary McNickle <gary@dharvest.com>
 */
void load_quotes( void )
{
    int                     n = 1;
    FILE                   *fp;
    char                    buf[100];

    snprintf( buf, 100, "%squote.%d", QUOTE_DIR, n );
    while ( ( fp = FileOpen( buf, "r" ) ) != NULL ) {
        FileClose( fp );
        snprintf( buf, 100, "%squote.%d", QUOTE_DIR, ++n );
    }
    n--;
    sysdata.numquotes = n;
}                                                      /* end load_quotes */

void quotes( CHAR_DATA *ch )
{
    int                     i;
    char                    buf[MSL];
    FILE                   *fp;

    if ( sysdata.numquotes == 0 )
        load_quotes(  );

    /*
     * Don't display anything if there are none to find - Samson
     */
    if ( sysdata.numquotes <= 0 )
        return;
    i = number_range( 1, sysdata.numquotes );
    if ( i > sysdata.numquotes )
        return;
    snprintf( buf, MSL, "%squote.%d", QUOTE_DIR, i );
    if ( ( fp = FileOpen( buf, "r" ) ) == NULL )
        return;
    send_to_char( "&W\r\n", ch );
    while ( !feof( fp ) ) {
        /*
         * be sure fgets read some more data, we dont want to send the last line twice.
         */
        if ( fgets( buf, MSL, fp ) != NULL )
            send_to_char( buf, ch );
    }
    send_to_char( "\r\n", ch );
    FileClose( fp );
}                                                      /* end do_quotes */

void do_qui( CHAR_DATA *ch, char *argument )
{
    set_char_color( AT_RED, ch );
    send_to_char( "Si quieres salir, escríbe el comando completo.\r\n", ch );
    return;
}

void do_quit( CHAR_DATA *ch, char *argument )
{
    AUTH_LIST              *old_auth;
    int                     x,
                            y;
    int                     level;
    char                    qbuf[MIL];
    char                    logbuf[MSL];
    char                    buf[MIL];
    CLAN_DATA              *clan;

    if ( IS_NPC( ch ) )
        return;

    if ( in_arena( ch ) && ( str_cmp( ch->in_room->area->filename, "allifort.are" ) )
         && ( str_cmp( ch->in_room->area->filename, "throngfort.are" ) ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "No puedes dejar el juego aquí.\r\n", ch );
        return;
    }
    if ( ch->challenged || ch->challenge ) {
        challenge = FALSE;
        ch->challenged = NULL;
        snprintf( buf, MIL, "%s sale de los reinos y cancela el desafío!", ch->name );
        arena_chan( buf );
    }

    if ( xSET_BIT( ch->act, PLR_R54 ) ) {
        xREMOVE_BIT( ch->act, PLR_R54 );
    }

    if ( SET_BIT( ch->pcdata->flags, PCFLAG_PUPPET ) ) {
        REMOVE_BIT( ch->pcdata->flags, PCFLAG_PUPPET );
    }

    if ( ch->position == POS_FIGHTING || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE
         || ch->position == POS_BERSERK ) {
        set_char_color( AT_RED, ch );
        send_to_char( "No puedes, estás luchando.\r\n", ch );
        return;
    }
    if ( ch->position < POS_STUNNED ) {
        set_char_color( AT_BLOOD, ch );
        send_to_char( "No estás muerto todavía.\r\n", ch );
        return;
    }
    if ( IS_SET( ch->pcdata->tag_flags, TAG_PLAYING | TAG_WAITING ) ) {
        send_to_char( "No puedes salir en medio de un evento.\r\n", ch );
        return;
    }
    /*
     * Strip the heavens blessing
     */
    if ( IS_AFFECTED( ch, AFF_HEAVENS_BLESS ) ) {
        AFFECT_DATA            *paf;
        AFFECT_DATA            *paf_next;

        for ( paf = ch->first_affect; paf; paf = paf_next ) {
            paf_next = paf->next;
            if ( xIS_SET( paf->bitvector, AFF_HEAVENS_BLESS ) ) {
                affect_remove( ch, paf );
            }
            if ( xIS_SET( paf->bitvector, AFF_SANCTUARY ) ) {
                affect_remove( ch, paf );
            }
        }
        send_to_char( "La bendición se desvanece.\r\n", ch );
        xREMOVE_BIT( ch->affected_by, AFF_HEAVENS_BLESS );
        xREMOVE_BIT( ch->affected_by, AFF_HEAVENS_BLESS );
        xREMOVE_BIT( ch->affected_by, AFF_HEAVENS_BLESS );
        xREMOVE_BIT( ch->affected_by, AFF_SANCTUARY );
    }
    // update_member(ch);
    if ( IS_SET( ch->in_room->room_flags, ROOM_ARENA )
         && ( str_cmp( ch->in_room->area->filename, "allifort.are" ) )
         && ( str_cmp( ch->in_room->area->filename, "throngfort.are" ) ) ) {
        send_to_char( "No mientras estés en la arena.\r\n", ch );
        return;
    }
    if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "¡La adrenalina no te deja abandonar el juego!\r\n", ch );
        return;
    }
    if ( auction->item != NULL && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) ) {
        send_to_char( "Espera que termine la subasta.\r\n", ch );
        return;
    }
    if ( IS_PKILL( ch ) && ch->wimpy > ( int ) ch->max_hit / 2.25 ) {
        send_to_char( "Tu cobardía ha sido ajustada al máximo permitido.\r\n", ch );
        do_wimpy( ch, ( char * ) "max" );
    }

    if ( IS_CLANNED( ch ) ) {
        clan = ch->pcdata->clan;
        if ( clan->chieftain != NULL ) {
            if ( !str_cmp( ch->name, clan->chieftain ) ) {
                clan->chieflog = current_time;
            }
        }
        if ( clan->warmaster != NULL ) {
            if ( !str_cmp( ch->name, clan->warmaster ) ) {
                clan->warlog = current_time;
            }
        }
        save_clan( clan );

    }
    /*
     * new auth
     */
    if ( NEW_AUTH( ch ) )
        remove_from_auth( ch->name );
    else {
        old_auth = get_auth_name( ch->name );
        if ( old_auth != NULL
             && ( old_auth->state == AUTH_ONLINE || old_auth->state == AUTH_LINK_DEAD ) )
            old_auth->state = AUTH_OFFLINE;            /* Logging off */
    }
    /*
     * Get 'em dismounted until we finish mount saving -- Blodkai, 4/97
     */
    if ( ch->position == POS_MOUNTED ) {
        do_dismount( ch, ( char * ) "" );
    }
    act( AT_YELLOW, "$n ha dejado el juego.", ch, NULL, NULL, TO_CANSEE );
    set_char_color( AT_GREY, ch );
    send_to_char( "\r\n&rLentamente te fundes con las sombras de 6 dragones.&D\r\n", ch );
    quotes( ch );
    quitting_char = ch;
    save_char_obj( ch );
    if ( sysdata.save_pets && ch->pcdata->pet ) {
        act( AT_BYE, "$N se desvanece lentamente.", ch, NULL, ch->pcdata->pet, TO_ROOM );
        extract_char( ch->pcdata->pet, TRUE );
    }
    /*
     * Synch clandata up only when clan member quits now. --Shaddai
     */
    if ( ch->pcdata->clan ) {
        save_clan( ch->pcdata->clan );
        // update_member(ch);
    }
    saving_char = NULL;
    level = get_trust( ch );
    /*
     * After extract_char the ch is no longer valid!
     */
    {
        struct tm              *tme;
        time_t                  now;
        char                    day[50];

        now = time( 0 );
        tme = localtime( &now );
        strftime( day, 50, "%a %b %d %H:%M:%S %Y", tme );
        snprintf( logbuf, MSL, "%20s %24s has quit.", ch->pcdata->filename, day );
        append_to_file( LAST_FILE, logbuf );

        snprintf( logbuf, MSL, "%s has quit the realms of 6 Dragons.", ch->name );
    }
    if ( IS_PUPPET( ch ) )
        REMOVE_BIT( ch->pcdata->flags, PCFLAG_PUPPET );
    extract_char( ch, TRUE );
    for ( x = 0; x < MAX_WEAR; x++ )
        for ( y = 0; y < MAX_LAYERS; y++ )
            save_equipment[x][y] = NULL;
    log_string_plus( logbuf, LOG_COMM, level );
    return;
}

void send_rip_screen( CHAR_DATA *ch )
{
    FILE                   *rpfile;
    int                     num = 0;
    char                    BUFF[MSL * 2];

    if ( ( rpfile = FileOpen( RIPSCREEN_FILE, "r" ) ) != NULL ) {
        while ( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
            num++;
        FileClose( rpfile );
        BUFF[num] = 0;
        write_to_buffer( ch->desc, BUFF, num );
    }
}

void send_rip_title( CHAR_DATA *ch )
{
    FILE                   *rpfile;
    int                     num = 0;
    char                    BUFF[MSL * 2];

    if ( ( rpfile = FileOpen( RIPTITLE_FILE, "r" ) ) != NULL ) {
        while ( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
            num++;
        FileClose( rpfile );
        BUFF[num] = 0;
        write_to_buffer( ch->desc, BUFF, num );
    }
}

void send_ansi_title( CHAR_DATA *ch )
{
    FILE                   *rpfile;
    int                     num = 0;
    char                    BUFF[MSL * 2];

    if ( ( rpfile = FileOpen( ANSITITLE_FILE, "r" ) ) != NULL ) {
        while ( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
            num++;
        FileClose( rpfile );
        BUFF[num] = 0;
        write_to_buffer( ch->desc, BUFF, num );
    }
}

void send_ascii_title( CHAR_DATA *ch )
{
    FILE                   *rpfile;
    int                     num = 0;
    char                    BUFF[MSL];

    if ( ( rpfile = FileOpen( ASCTITLE_FILE, "r" ) ) != NULL ) {
        while ( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
            num++;
        FileClose( rpfile );
        BUFF[num] = 0;
        write_to_buffer( ch->desc, BUFF, num );
    }
}

void do_rip( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "Rip ON o OFF?\r\n", ch );
        return;
    }
    if ( ( strcmp( arg, "on" ) == 0 ) || ( strcmp( arg, "ON" ) == 0 ) ) {
        send_rip_screen( ch );
        xSET_BIT( ch->act, PLR_RIP );
        xSET_BIT( ch->act, PLR_ANSI );
        return;
    }
    if ( ( strcmp( arg, "off" ) == 0 ) || ( strcmp( arg, "OFF" ) == 0 ) ) {
        xREMOVE_BIT( ch->act, PLR_RIP );
        send_to_char( "!|*\r\nRIP off...\r\n", ch );
        return;
    }
}

void do_ansi( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "ANSI ON o OFF?\r\n", ch );
        return;
    }
    if ( ( strcmp( arg, "on" ) == 0 ) || ( strcmp( arg, "ON" ) == 0 ) ) {
        xSET_BIT( ch->act, PLR_ANSI );
        set_char_color( AT_WHITE + AT_BLINK, ch );
        send_to_char( "ANSI ON!!!\r\n", ch );
        return;
    }
    if ( ( strcmp( arg, "off" ) == 0 ) || ( strcmp( arg, "OFF" ) == 0 ) ) {
        xREMOVE_BIT( ch->act, PLR_ANSI );
        send_to_char( "Vale... ANSI off.\r\n", ch );
        return;
    }
}

void do_save( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC( ch ) )
        return;

    WAIT_STATE( ch, 2 );                               /* For big muds with save-happy
                                                        * players, like RoD */
    update_aris( ch );                                 /* update char affects and RIS */
    calc_score( ch );
    save_char_obj( ch );
    saving_char = NULL;
    send_to_char( "Ficha salvada... Recuerda que el juego guarda automáticamente cada 15 minutos.\r\n", ch );
}

/* Something from original DikuMUD that Merc yanked out.
 * Used to prevent following loops, which can cause problems if people
 * follow in a loop through an exit leading back into the same room
 * (Which exists in many maze areas)   -Thoric
 */
bool circle_follow( CHAR_DATA *ch, CHAR_DATA *victim )
{
    CHAR_DATA              *tmp;

    for ( tmp = victim; tmp; tmp = tmp->master )
        if ( tmp == ch )
            return TRUE;
    return FALSE;
}

void do_dismiss( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿perder a quien?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }
    if ( ( IS_AFFECTED( victim, AFF_CHARM ) ) && ( IS_NPC( victim ) ) && ( victim->master == ch ) ) {
        while ( victim->first_affect )
            affect_remove( victim, victim->first_affect );
        stop_follower( victim );
        stop_hating( victim );
        stop_hunting( victim );
        stop_fearing( victim );
        xREMOVE_BIT( ch->act, PLR_BOUGHT_PET );
        act( AT_ACTION, "$n da esquinazo a $N.", ch, NULL, victim,
             TO_NOTVICT );
        act( AT_ACTION, "Das esquinazo a $N.", ch, NULL, victim,
             TO_CHAR );
    }
    else
        send_to_char( "no puedes.\r\n", ch );
}

void do_follow( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "¿Seguir a quién?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_TUTORIAL ) ) {
        send_to_char( "Por favor, espera instrucciones.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) && ch->master ) {
        act( AT_PLAIN, "¡Ya sigues a $N!", ch, NULL, ch->master, TO_CHAR );
        return;
    }
    if ( victim == ch ) {
        if ( !ch->master ) {
            send_to_char( "Ya lo haces.\r\n", ch );
            return;
        }
        stop_follower( ch );
        return;
    }
    if ( circle_follow( ch, victim ) ) {
        send_to_char( "Seguirse en círculos no está permitido. Lo siento.\r\n", ch );
        return;
    }
    if ( ch->master )
        stop_follower( ch );
    add_follower( ch, victim );
    return;
}

void add_follower( CHAR_DATA *ch, CHAR_DATA *master )
{
    if ( ch->master ) {
        bug( "%s", "Add_follower: non-null master." );
        return;
    }
    ch->master = master;
    ch->leader = NULL;
    /*
     * Support for saving pets --Shaddai
     */
    if ( IS_NPC( ch ) && xIS_SET( ch->act, ACT_PET ) && !IS_NPC( master ) )
        master->pcdata->pet = ch;
    if ( !xIS_SET( ch->act, PLR_BOAT ) ) {
        if ( can_see( master, ch ) )
            act( AT_ACTION, "$n te sigue.", ch, NULL, master, TO_VICT );
        act( AT_ACTION, "Sigues a $N.", ch, NULL, master, TO_CHAR );
    }
    return;
}

void stop_follower( CHAR_DATA *ch )
{
    if ( !ch->master ) {
        bug( "%s", "Stop_follower: null master." );
        return;
    }
    if ( IS_NPC( ch ) && !IS_NPC( ch->master ) && ch->master->pcdata->pet == ch ) {
        xREMOVE_BIT( ch->master->act, PLR_BOUGHT_PET );
        ch->master->pcdata->pet = NULL;
    }
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        xREMOVE_BIT( ch->affected_by, AFF_CHARM );
        if ( !IS_NPC( ch->master ) )
            ch->master->pcdata->charmies--;
    }
    if ( !xIS_SET( ch->act, PLR_BOAT ) ) {
        if ( can_see( ch->master, ch ) )
            if ( !( !IS_NPC( ch->master ) && IS_IMMORTAL( ch ) && !IS_IMMORTAL( ch->master ) ) )
                act( AT_ACTION, "$n deja de seguirte.", ch, NULL, ch->master, TO_VICT );
        act( AT_ACTION, "Dejas de seguir a $N.", ch, NULL, ch->master, TO_CHAR );
    }
    ch->master = NULL;
    ch->leader = NULL;
}

void die_follower( CHAR_DATA *ch )
{
    CHAR_DATA              *fch;

    if ( ch->master )
        stop_follower( ch );
    ch->leader = NULL;
    for ( fch = first_char; fch; fch = fch->next ) {
        if ( fch->master == ch )
            stop_follower( fch );
        if ( fch->leader == ch )
            fch->leader = fch;
    }
    return;
}

void do_order( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            arg2[MIL];
    char                    argbuf[MIL];
    CHAR_DATA              *victim;
    CHAR_DATA              *och;
    CHAR_DATA              *och_next;
    bool                    found;
    bool                    fAll;
    char                    logbuf[MSL];

    mudstrlcpy( argbuf, argument, MIL );
    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );
    if ( arg[0] == '\0' || argument[0] == '\0' ) {
        send_to_char( "¿Ordenar a quién qué?\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "No puedes ordenar nada.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "all" ) ) {
        fAll = TRUE;
        victim = NULL;
    }
    else {
        fAll = FALSE;
        if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        if ( victim == ch ) {
            send_to_char( "¡Hazlo tu!\r\n", ch );
            return;
        }
        if ( !IS_AFFECTED( victim, AFF_CHARM ) || victim->master != ch ) {
            send_to_char( "¡Hazlo tu!\r\n", ch );
            return;
        }
        if ( victim->fighting )
        WAIT_STATE( victim, 14 );

       if ( str_cmp( arg2, "cobardia" ))
       {
        if ( strstr( arg2, "mp" ) != NULL ) {
            send_to_char( "No lo creo.\r\n", ch );
            return;
        }
      }
    }
    found = FALSE;
    for ( och = ch->in_room->first_person; och; och = och_next ) {
        och_next = och->next_in_room;
        if ( IS_AFFECTED( och, AFF_CHARM ) && och->master == ch && ( fAll || och == victim ) ) {
            found = TRUE;
            act( AT_ACTION, "$n te ordena '$t'.", ch, argument, och, TO_VICT );
            interpret( och, argument );
        }
    }
    if ( found ) {
/*    snprintf(logbuf, MSL, "%s: order %s.", ch->name, argbuf);
    log_string_plus(logbuf, LOG_NORMAL, ch->level);  */
        WAIT_STATE( ch, 12 );
    }
    else
        send_to_char( "No tienes seguidores aquí.\r\n", ch );
    return;
}

/* Overhauled 06/24/2001 to solve many of the bad issues in the code,
 * such as numerical level checks (gch->level < 50), etc. It is
 * mostly a change to the way the do_group display looks, but some
 * changes have been made to the grouping code to weed out potential
 * bugs. I also commented it all to hell, for those who might need to
 * work on it, for any reason. -Orion Elder
 *
 * Previous version of function by Blodkai.
 */
void do_group( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    arg[MIL];
    CHAR_DATA              *victim;
    int                     MAX_GROUP_LEVEL_DIFF = 11;

    one_argument( argument, arg );
    /*
     * If no argument is given, then show group information -Orion
     */
    if ( arg[0] == '\0' ) {
        CHAR_DATA              *gch;
        CHAR_DATA              *leader = ch->leader ? ch->leader : ch;

        set_char_color( AT_DGREEN, ch );
        /*
         * Display the headers for information. -Orion
         */
        ch_printf( ch, "\r\nSiguiendo %-16.16s [clase] [-raza-] [sal] [mag] [mov] [mst]%s\r\n",
                   PERS( leader, ch ), IS_DEMIGOD( ch ) ? " [--level--]" : "" );
        /*
         * Loop through characters checking for those in your group -Orion
         */
        for ( gch = first_char; gch; gch = gch->next ) {
            /*
             * See if gch (the char being checked) is in ch's (the command
             * * issuer) group. If so, then proceed through.
             */
            if ( is_same_group( gch, ch ) ) {
                set_char_color( AT_DGREEN, ch );
                /*
                 * If affected by possess tell only name and level -Orion
                 */
                if ( IS_AFFECTED( gch, AFF_POSSESS ) ) {
                    ch_printf( ch, "[%-3d] %s\r\n", gch->level, capitalize( PERS( gch, ch ) ) );
                }
                /*
                 * Print off all that nifty information about gch to the
                 * * ch. -Orion
                 */
                else {
                    /*
                     * Percentage variables, for displaying the percentage of
                     * * hp/magic/moves to character. -Orion
                     */
                    int                     hp_perc,
                                            mg_perc,
                                            mv_perc;
                    double                  hp_temp,
                                            mg_temp,
                                            mv_temp;
                    double                  min,
                                            max;

                    /*
                     * Initialize the variables. -Orion
                     */
                    hp_perc = mg_perc = mv_perc = 0;
                    hp_temp = mg_temp = mv_temp = 0.0;
                    min = max = 0.0;
                    /*
                     * Calculate HP into a percentage. -Orion
                     */
                    min = ( double ) gch->hit;
                    max = ( double ) gch->max_hit;
                    hp_temp = ( double ) ( min / max );
                    hp_perc = ( int ) ( hp_temp * 100 );
                    /*
                     * Check for blood/mana then make it a percentage. -Orion
                     */
                    if ( IS_BLOODCLASS( gch ) ) {
                        min = ( double ) gch->blood;
                        max = ( double ) gch->max_blood;
                        mg_temp = ( double ) ( min / max );
                        mg_perc = ( int ) ( mg_temp * 100 );
                    }
                    else {
                        min = ( double ) gch->mana;
                        max = ( double ) gch->max_mana;
                        mg_temp = ( double ) ( min / max );
                        mg_perc = ( int ) ( mg_temp * 100 );
                    }
                    /*
                     * Calculate Moves into a percentage. -Orion
                     */
                    min = ( double ) gch->move;
                    max = ( double ) gch->max_move;
                    mv_temp = ( double ) ( min / max );
                    mv_perc = ( int ) ( mv_temp * 100 );
                    /*
                     * Show a symbol based on alignment. -Orion
                     */
                    if ( IS_GOOD( gch ) )
                        mudstrlcpy( buf, "+", MSL );
                    else if ( IS_NEUTRAL( gch ) )
                        mudstrlcpy( buf, "o", MSL );
                    else if ( IS_EVIL( gch ) )
                        mudstrlcpy( buf, "-", MSL );
                    else
                        mudstrlcpy( buf, "X", MSL );
                    set_char_color( AT_DGREEN, ch );
                    send_to_char( "[", ch );
                    set_char_color( AT_GREEN, ch );
                    ch_printf( ch, "%-3d (", gch->level );
                    /*
                     * Display align color based on alignment. -Orion
                     */
                    if ( IS_GOOD( gch ) )
                        set_char_color( AT_WHITE, ch );
                    else if ( IS_NEUTRAL( gch ) )
                        set_char_color( AT_GREY, ch );
                    else if ( IS_EVIL( gch ) )
                        set_char_color( AT_DGREY, ch );
                    else
                        set_char_color( AT_YELLOW, ch );
                    ch_printf( ch, "%1.1s", buf );
                    set_char_color( AT_GREEN, ch );
                    send_to_char( ")", ch );
                    set_char_color( AT_DGREEN, ch );
                    send_to_char( "] ", ch );
                    set_char_color( AT_GREEN, ch );
                    ch_printf( ch, "%-16.16s ", capitalize( PERS( gch, ch ) ) );
                    ch_printf( ch, "%7.7s ", class_table[gch->Class]->who_name );
                    ch_printf( ch, "%8.8s ", race_table[gch->race]->race_name );
                    /*
                     * Display HP color based on danger level. -Orion
                     */
                    if ( hp_perc <= 33 )
                        set_char_color( AT_DANGER, ch );
                    else if ( hp_perc <= 66 )
                        set_char_color( AT_YELLOW, ch );
                    else
                        set_char_color( AT_GREY, ch );
                    ch_printf( ch, "%4d%% ", hp_perc );
                    /*
                     * Display magic color based on blood/mana. -Orion
                     */
                    if ( IS_BLOODCLASS( gch ) )
                        set_char_color( AT_BLOOD, ch );
                    else
                        set_char_color( AT_LBLUE, ch );
                    ch_printf( ch, "%4d%% ", mg_perc );
                    /*
                     * Display move color based on danger level. -Orion
                     */
                    if ( mv_perc <= 33 )
                        set_char_color( AT_DANGER, ch );
                    else if ( mv_perc <= 66 )
                        set_char_color( AT_YELLOW, ch );
                    else
                        set_char_color( AT_GREY, ch );
                    ch_printf( ch, "%4d%% ", mv_perc );
                    /*
                     * Display mental state color based on danger
                     * * level. -Orion
                     */
                    if ( gch->mental_state < -66 || gch->mental_state > 66 )
                        set_char_color( AT_DANGER, ch );
                    else if ( gch->mental_state < -33 || gch->mental_state > 33 )
                        set_char_color( AT_YELLOW, ch );
                    else
                        set_char_color( AT_GREEN, ch );
                    ch_printf( ch, "%4.4s  ",
                               gch->mental_state > 75 ? "+++" :
                               gch->mental_state > 50 ? "=++" : gch->mental_state >
                               25 ? "==+" : gch->mental_state > -25 ? "===" : gch->mental_state >
                               -50 ? "-==" : gch->mental_state > -75 ? "--=" : "---" );
                    set_char_color( AT_GREEN, ch );
                    if ( IS_DEMIGOD( ch ) && !IS_DEMIGOD( gch ) ) {
                        ch_printf( ch, "%11d ", exp_level( gch, gch->level + 1 ) - gch->exp );
                    }
                    send_to_char( "\r\n", ch );
                }
            }
        }
        return;
    }
    /*
     * Check if they are chose to disband the group, if so continue
     * * through the function. -Orion
     */
    if ( !strcmp( arg, "deshacer" ) ) {
        CHAR_DATA              *gch;
        int                     count = 0;

        /*
         * Is the function caller the leader or master? If not, they
         * * don't HAVE this option. -Orion
         */
        if ( ch->leader || ch->master ) {
            send_to_char( "No eres líder de ningún grupo, sigues a otra persona.\r\n", ch );
            return;
        }
        /*
         * Loop through characters, removing them from the group if they are
         * * in it. -Orion
         */
        for ( gch = first_char; gch; gch = gch->next ) {
            if ( is_same_group( ch, gch ) && ( ch != gch ) ) {
                /*
                 * If their leader isn't the person disbanding the group,
                 * * then doing this is bad. -Orion
                 */
                if ( gch->leader ) {
                    if ( gch->leader == ch )
                        gch->leader = NULL;
                    else
                        bug( "do_group: disbanding non-grouped member" );
                }
                /*
                 * Same as above. Better safe than sorry. -Orion
                 */

                if ( !xIS_SET( gch->act, ACT_MOUNTABLE ) ) {
                    if ( gch->master ) {
                        if ( gch->master == ch )
                            gch->master = NULL;
                        else
                            bug( "do_group: disbanding non-grouped member" );
                    }
                }
                count++;
                send_to_char( "Grupo disuelto.\r\n", gch );
            }
        }
        /*
         * If count is equal to zero, they had no group members. - Orion
         */
        if ( count == 0 )
            send_to_char( "No tienes miembros en el grupo.\r\n", ch );
        else
            send_to_char( "Deshaces el grupo.\r\n", ch );
        return;
    }
    /*
     * Check to see if they chose to group all, if so continue through
     * * the function. -Orion
     */
    if ( !strcmp( arg, "todos" ) ) {
        CHAR_DATA              *rch;
        int                     count = 0;

        /*
         * Loop through characters in the room, adding elligible members to
         * * the caller's group. -Orion
         */
        for ( rch = ch->in_room->first_person; rch; rch = rch->next_in_room ) {
            if ( ch != rch && !IS_NPC( rch ) && can_see( ch, rch ) && rch->master == ch
                 && !ch->master && !ch->leader
                 && ( abs( ch->level - rch->level ) < MAX_GROUP_LEVEL_DIFF )
                 && !is_same_group( rch, ch ) ) {
                rch->leader = ch;
                count++;
            }
        }
        /*
         * If count is zero, then let them know they could not add anyone
         * * to their group, else let them know they did. -Orion
         */
        if ( count == 0 )
            send_to_char( "Nadie que pueda unirse a tu grupo te está siguiendo.\r\n", ch );
        else {
            act( AT_ACTION, "$n agrupa a sus seguidores.", ch, NULL, NULL, TO_ROOM );
            send_to_char( "You group your followers.\r\n", ch );
        }
        return;
    }
    /*
     * If victim is NULL then the victim isn't in the room with the caller,
     * * so let them know that. -Orion
     */
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }
    /*
     * Let them know that they can not group people when they are
     * * following someone. -Orion
     */
    if ( ch->master || ( ch->leader && ch->leader != ch ) ) {
        send_to_char( "¡Imposible!\r\n", ch );
        return;
    }
    /*
     * Lets them know they can not group someone who is not following
     * * them. -Orion
     */
    if ( victim->master != ch && ch != victim ) {
        act( AT_PLAIN, "$N no te está siguiendo.", ch, NULL, victim, TO_CHAR );
        return;
    }
    /*
     * Let them know that grouping themself is a no-no. -Orion
     */
    if ( victim == ch ) {
        act( AT_PLAIN, "¿agrupar contigo mismo?", ch, NULL, victim, TO_CHAR );
        return;
    }
    /*
     * If they're already in the group, remove them. -Orion
     */
    if ( is_same_group( victim, ch ) && ch != victim ) {

        /*
         * If the victim's leader isn't ch, then setting it to null is bad. -Orion
         */
        if ( victim->leader ) {
            if ( victim->leader == ch )
                victim->leader = NULL;
            else
                bug( "do_group: removing non-grouped member." );
        }
        act( AT_ACTION, "$n saca a $N de su grupo.", ch, NULL, victim, TO_NOTVICT );
        act( AT_ACTION, "$n te saca de su grupo.", ch, NULL, victim, TO_VICT );
        act( AT_ACTION, "Sacas a $N de tu grupo.", ch, NULL, victim, TO_CHAR );
        return;
    }
    /*
     * Check their level and pkill status to see if joining the group is a
     * * viable option. If not, tell the involved parties so. -Orion
     */
    if ( !xIS_SET( victim->act, ACT_MOUNTABLE ) ) {
        if ( abs( ch->level - victim->level ) >= MAX_GROUP_LEVEL_DIFF ) {
            act( AT_PLAIN, "$N no puede acceder al grupo de $n.", ch, NULL, victim, TO_NOTVICT );
            act( AT_PLAIN, "No puedes acceder al grupo de $n.", ch, NULL, victim, TO_VICT );
            act( AT_PLAIN, "$N no puede unirse a tu grupo.", ch, NULL, victim, TO_CHAR );
            return;
        }
    }
    /*
     * Go ahead and group them. -Orion
     */
    victim->leader = ch;
    act( AT_ACTION, "$N se une al grupo de $n.", ch, NULL, victim, TO_NOTVICT );
    act( AT_ACTION, "Te unes al grupo de $n.", ch, NULL, victim, TO_VICT );
    act( AT_ACTION, "$N se une a tu grupo.", ch, NULL, victim, TO_CHAR );
    return;
}

/* 'Split' originally by Gnort, God of Chaos. */
void do_split( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    arg[MIL];
    CHAR_DATA              *gch;
    int                     members;
    int                     amount;
    int                     share;
    int                     extra,
                            type;

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' || !argument ) {
        send_to_char( "¿Repartir qué?\r\n", ch );
        return;
    }
    if ( !( type = get_currency_type( argument ) ) )
        type = DEFAULT_CURR;
    amount = atoi( arg );
    if ( amount < 0 ) {
        send_to_char( "¿En un grupo de cero miembros?\r\n", ch );
        return;
    }
    if ( amount == 0 ) {
        send_to_char( "No tienes dinero.\r\n", ch );
        return;
    }
    if ( GET_MONEY( ch, type ) < amount ) {
        ch_printf( ch, "No tienes tanta %s.\r\n", curr_types[type] );
        return;
    }
    members = 0;
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room ) {
        if ( is_same_group( gch, ch ) )
            members++;
    }
    if ( members < 2 ) {
        send_to_char( "¡No hay miembros en tu grupo!\r\n", ch );
        return;
    }

    share = amount / members;
    extra = amount % members;
    if ( share == 0 ) {
        send_to_char( "No.\r\n", ch );
        return;
    }
    GET_MONEY( ch, type ) -= amount;
    GET_MONEY( ch, type ) += share + extra;
    set_char_color( AT_GOLD, ch );
    ch_printf( ch, "Repartes %d monedas de %s.  Tu parte es de %d monedas de %s.\r\n", amount,
               curr_types[type], share + extra, curr_types[type] );
    /*
     * if(IS_CLANNED(ch) && ch->pcdata->clan->tithe) clan_tithe(ch, share, type);
     */
    snprintf( buf, MSL, "$n reparte %d monedas de %s.  Tu parte es de %d monedas de %s.", amount,
              curr_types[type], share, curr_types[type] );
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room ) {
        if ( gch != ch && is_same_group( gch, ch ) ) {
            act( AT_GOLD, buf, ch, NULL, gch, TO_VICT );
            GET_MONEY( gch, type ) += share;
/*      if(IS_CLANNED(gch) && gch->pcdata->clan->tithe)
        clan_tithe(gch, share, type);  */
        }
    }

    return;
}

void do_gtell( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *gch;

#ifndef SCRAMBLE
    int                     speaking = -1,
        lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
        if ( ch->speaking & lang_array[lang] ) {
            speaking = lang;
            break;
        }
#endif
    if ( argument[0] == '\0' ) {
        send_to_char( "¡Contar al grupo qué?\r\n", ch );
        return;
    }
    if ( xIS_SET( ch->act, PLR_NO_TELL ) ) {
        send_to_char( "¡No puedes!\r\n", ch );
        return;
    }
    for ( gch = first_char; gch; gch = gch->next ) {
        if ( is_same_group( gch, ch ) ) {
            set_char_color( AT_GTELL, gch );
            /*
             * Groups unscrambled regardless of clan language.  Other languages
             * * still garble though. -- Altrag
             */
#ifndef SCRAMBLE
            if ( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) ) {
                int                     speakswell = UMIN( knows_language( gch, ch->speaking, ch ),
                                                           knows_language( ch, ch->speaking,
                                                                           gch ) );

                if ( speakswell < 85 )
                    ch_printf( gch, "%s cuenta al grupo '%s'.\r\n", ch->name,
                               translate( speakswell, argument, lang_names[speaking] ) );
                else
                    ch_printf( gch, "%s cuenta val grupo '%s'.\r\n", ch->name, argument );
            }
            else
                ch_printf( gch, "%s tells cuenta al grupo '%s'.\r\n", ch->name, argument );
#else
            if ( knows_language( gch, ch->speaking, gch ) || ( IS_NPC( ch ) && !ch->speaking ) )
                ch_printf( gch, "%s cuenta al grupo '%s'.\r\n", ch->name, argument );
            else
                ch_printf( gch, "%s cuenta al grupo '%s'.\r\n", ch->name,
                           scramble( argument, ch->speaking ) );
#endif
        }
    }
    return;
}

/* It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch )
{
    if ( ach->leader )
        ach = ach->leader;
    if ( bch->leader )
        bch = bch->leader;
    return ach == bch;
}

/*
 * Language support functions. -- Altrag
 * 07/01/96
 * Modified to return how well the language is known 04/04/98 - Thoric
 * Currently returns 100% for known languages... but should really return
 * a number based on player's wisdom (maybe 50+( (25-wisdom)*2) ?)
 */
int knows_language( CHAR_DATA *ch, int language, CHAR_DATA *cch )
{
    int                     lang;
    short                   sn;

    if ( !IS_NPC( ch ) && IS_IMMORTAL( ch ) )
        return 100;
    if ( IS_NPC( ch ) && !ch->speaks )                 /* No langs = knows all for npcs */
        return 100;
    /*
     * everyone KNOWS common tongue
     */
    if ( IS_SET( language, LANG_COMMON ) )
        return 100;
    if ( IS_NPC( ch ) && IS_SET( ch->speaks, language ) )
        return 100;

    /*
     * Racial languages for PCs
     */
    if ( !IS_NPC( ch ) ) {
        if ( IS_SET( race_table[ch->race]->language, language ) )
            return 100;
        for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ ) {
            if ( IS_SET( language, lang_array[lang] )  /* && IS_SET( ch->speaks, * * * *
                                                        * * * lang_array[lang] * ) */  )
                // volk bug fix
            {
                if ( ( sn = skill_lookup( lang_names[lang] ) ) != -1 )
                    return ch->pcdata->learned[sn];
            }
        }
    }
    return 0;
}

int const               lang_array[] = {
    LANG_COMMON, LANG_ELVEN, LANG_DWARVEN, LANG_PIXISH, LANG_OGRIAN,
    LANG_ORCISH, LANG_TROLLESE, LANG_GOBLIC, LANG_HOBBIT, LANG_GNOMISH,
    LANG_INFERNAL, LANG_CELESTIAL, LANG_DRACONIC, LANG_UNCOMMON,
    LANG_DEMONIC, LANG_CENTAURIAN, LANG_SILENT, LANG_UNKNOWN
};

const char             *const lang_names[] = {
    "common", "elven", "dwarven", "pixish", "ogrian",
    "orcish", "trollese", "goblic", "hobbit", "gnomish", "infernal",
    "celestial", "draconic", "uncommon", "demonic", "centaurian",
    "silent", "17", "18", "19", "20", "21", "22", "23", "24", "25",
    "26", "27", "28", "29", "30", "31", "32"
};

/* Note: does not count racial language.  This is intentional (for now). */
int countlangs( int languages )
{
    int                     numlangs = 0;
    int                     looper;

    for ( looper = 0; lang_array[looper] != LANG_UNKNOWN; looper++ ) {
        if ( languages & lang_array[looper] )
            numlangs++;
    }
    return numlangs;
}

void do_speak( CHAR_DATA *ch, char *argument )
{
    int                     langs;
    char                    arg[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );

    if ( !str_cmp( arg, "todos" ) && IS_IMMORTAL( ch ) ) {
        set_char_color( AT_SAY, ch );
        ch->speaking = ~LANG_SILENT;
        send_to_char( "Ahora hablas todos los idiomas.\r\n", ch );
        return;
    }
    for ( langs = 0; lang_array[langs] != LANG_UNKNOWN; langs++ )
        if ( !str_prefix( arg, lang_names[langs] ) )
            if ( knows_language( ch, lang_array[langs], ch ) ) {
                if ( lang_array[langs] == LANG_SILENT && ( IS_NPC( ch ) ) )
                    continue;
                ch->speaking = lang_array[langs];
                set_char_color( AT_SAY, ch );
                ch_printf( ch, "Ahora hablas %s.\r\n", lang_names[langs] );
                return;
            }
            else {
                set_char_color( AT_SAY, ch );
                send_to_char( "No conoces ese idioma.\r\n", ch );
                return;
            }

    set_char_color( AT_SAY, ch );
    send_to_char( "No puedes.\r\n", ch );
}

void do_languages( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    int                     lang;

    argument = one_argument( argument, arg );
    if ( arg && arg[0] != '\0' && !str_prefix( arg, "learn" ) && !IS_NPC( ch ) ) {
        CHAR_DATA              *sch;
        char                    arg2[MIL];
        int                     sn,
                                prct,
                                prac;

        argument = one_argument( argument, arg2 );
        if ( !arg2 || arg2[0] == '\0' ) {
            send_to_char( "¿aprender que idioma?\r\n", ch );
            return;
        }
        for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ ) {
            if ( !str_prefix( arg2, lang_names[lang] ) )
                break;
        }
        if ( lang_array[lang] == LANG_UNKNOWN ) {
            send_to_char( "Eso no es un idioma.\r\n", ch );
            return;
        }
        if ( !( VALID_LANGS & lang_array[lang] ) ) {
            send_to_char( "No puedes aprender ese idioma.\r\n", ch );
            return;
        }
        if ( ( sn = skill_lookup( lang_names[lang] ) ) < 0 ) {
            send_to_char( "No es un idioma.\r\n", ch );
            return;
        }
        if ( race_table[ch->race]->language & lang_array[lang] || lang_array[lang] == LANG_COMMON
             || ch->pcdata->learned[sn] >= 99 ) {
            act( AT_PLAIN, "Ya eres frluente en $t.", ch, lang_names[lang], NULL, TO_CHAR );
            return;
        }
        for ( sch = ch->in_room->first_person; sch; sch = sch->next_in_room ) {
            if ( !IS_NPC( sch ) || !xIS_SET( sch->act, ACT_SCHOLAR ) )  /* Scholar npc? */
                continue;
            if ( knows_language( sch, lang_array[lang], sch ) > 0 )
                break;
        }
        if ( !sch ) {
            send_to_char( "No hay nadie que pueda enseñarte eso aquí.\r\n", ch );
            return;
        }
        /*
         * 0..16 cha = 2 pracs, 17..25 = 1 prac. -- Altrag
         */
        prac = 2 - ( get_curr_cha( ch ) / 17 );
        if ( ch->practice < prac ) {
            act( AT_TELL, "$n te cuenta  'No tienes prácticas suficientes.'", sch, NULL, ch,
                 TO_VICT );
            return;
        }
        ch->practice -= prac;
        /*
         * Max 12% (5 + 4 + 3) at 24+ int and 21+ wis. -- Altrag
         */
        prct = 5 + ( get_curr_int( ch ) / 6 ) + ( get_curr_wis( ch ) / 7 );
        ch->pcdata->learned[sn] += prct;
        ch->pcdata->learned[sn] = UMIN( ch->pcdata->learned[sn], 99 );
        SET_BIT( ch->speaks, lang_array[lang] );
        if ( ch->pcdata->learned[sn] == prct )
            act( AT_PLAIN, "Comienzas tus lecciones de $t.", ch, lang_names[lang], NULL, TO_CHAR );
        else if ( ch->pcdata->learned[sn] < 60 )
            act( AT_PLAIN, "Continúas tus lecciones de $t.", ch, lang_names[lang], NULL, TO_CHAR );
        else if ( ch->pcdata->learned[sn] < 60 + prct )
            act( AT_PLAIN, "Sientes que podrías comunicarte en $t.", ch, lang_names[lang],
                 NULL, TO_CHAR );
        else if ( ch->pcdata->learned[sn] < 99 )
            act( AT_PLAIN, "Ahora eres fluente en $t.", ch, lang_names[lang], NULL, TO_CHAR );
        else
            act( AT_PLAIN, "Ahora hablas perfectamente en $t.", ch, lang_names[lang], NULL, TO_CHAR );
        return;
    }

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ ) {
        if ( knows_language( ch, lang_array[lang], ch ) ) {
            if ( ch->speaking & lang_array[lang] || ( IS_NPC( ch ) && !ch->speaking ) )
                set_char_color( AT_SAY, ch );
            else
                set_char_color( AT_PLAIN, ch );
            send_to_char( lang_names[lang], ch );
            send_to_char( "\r\n", ch );
        }
    }
    send_to_char( "\r\n", ch );
}

/* Beep command courtesy of Altrag */
/* Installed Samson unknown date and time, allows user to beep other users. */

//Mods: Players can't beep themselves, invokes a true beep tone,
//and logging for better tracking of usage. -Taon

void do_beep( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( !argument || argument[0] == '\0' || !( victim = get_char_world( ch, argument ) ) ) {
        send_to_char( "¿Beep a quien?\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "Estás tonto.\r\n", ch );
        return;
    }
    /*
     * NPC check added by Samson 2-15-98
     */
    if ( IS_NPC( victim ) || is_ignoring( victim, ch ) || IS_NPC( ch ) ) {
        send_to_char( "¿A quien?\r\n", ch );
        return;
    }
    if ( ch->pcdata && ch->pcdata->release_date != 0 ) {
        send_to_char( "No desde el infierno.\r\n", ch );
        return;
    }
    /*
     * PCFLAG_NOBEEP check added by Samson 2-15-98
     */
    if ( IS_SET( victim->pcdata->flags, PCFLAG_NOBEEP ) ) {
        ch_printf( ch, "%s no acepta beeps.\r\n", victim->name );
        return;
    }
    log_printf( "%s beeps %s.", ch->name, victim->name );
    ch_printf( ch, "Enviando un beep a %s.\r\n", PERS( victim, ch ) );
    ch_printf( victim, "\r\n%s te manda un beep!\a\r\n", PERS( ch, victim ) );

    return;
}

/*

 * Just pop this into act_comm.c somewhere. (Or anywhere else)

 * It's pretty much say except modified to take args.

 *

 * Written by Kratas (moon@deathmoon.com)

 */

void do_talk_to_char( CHAR_DATA *ch, char *argument )
{

    char                    arg[MAX_INPUT_LENGTH],
                            last_char;

    char                    buf[MAX_STRING_LENGTH];

    char                   *sbuf;

    CHAR_DATA              *victim;

    EXT_BV                  actflags;

    int                     arglen;

#ifndef SCRAMBLE

    int                     speaking = -1,
        lang;

    for ( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )

        if ( ch->speaking & lang_array[lang] )
        {

            speaking = lang;

            break;

        }

#endif

    argument = one_argument( argument, arg );

    if ( !arg || arg[0] == '\0' || !argument || argument[0] == '\0' )
    {

        send_to_char( "¿Decir qué a quien?\n\r", ch );

        return;

    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL
         || ( IS_NPC( victim ) && victim->in_room != ch->in_room )

         || ( !NOT_AUTHED( ch ) && NOT_AUTHED( victim ) && !IS_IMMORTAL( ch ) ) )
    {

        send_to_char( "No está aquí.\n\r", ch );

        return;

    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
    {

        send_to_char( "No puedes hacer eso aquí.\n\r", ch );

        return;

    }

    if ( victim == ch )
    {

        send_to_char( "¿Necesitas ayuda?\n\r", ch );

        return;

    }

    arglen = strlen( argument ) - 1;

    /*
     * Remove whitespace and tabs.
     */

    while ( argument[arglen] == ' ' || argument[arglen] == '\t' )

        --arglen;

    last_char = argument[arglen];

    actflags = ch->act;

    if ( IS_NPC( ch ) )

        xREMOVE_BIT( ch->act, ACT_SECRETIVE );

    sbuf = argument;

    /*
     * Check to see if character is ignoring speaker
     */

    if ( is_ignoring( victim, ch ) )
    {

        /*
         * continue unless speaker is an immortal
         */

        if ( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) )

            return;

        else
        {

            ch_printf( victim, "Intentas ignorar a %s, pero no te lo permite.\n\r", ch->name );

        }

    }

#ifndef SCRAMBLE

    if ( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
    {

        int                     speakswell =
            UMIN( knows_language( victim, ch->speaking, ch ),
                  knows_language( ch, ch->speaking, victim ) );

        if ( speakswell < 75 )

            sbuf = translate( speakswell, argument, lang_names[speaking] );

    }

#else

    if ( !knows_language( victim, ch->speaking, ch ) && ( !IS_NPC( ch ) || ch->speaking != 0 ) )

        sbuf = scramble( argument, ch->speaking );

#endif

    sbuf = drunk_speech( sbuf, ch );

    ch->act = actflags;

    MOBtrigger = FALSE;

    switch ( last_char )
    {

        case '?':

            act( AT_SAY, "Preguntas a $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );

            act( AT_SAY, "$n pregunta a $N, '$t&c'", ch, drunk_speech( argument, ch ), victim,
                 TO_NOTVICT );

            act( AT_SAY, "$n te pregunta '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_VICT );

            break;

        case '!':

            act( AT_SAY, "Exclamas a $N, '$t&c'", ch, drunk_speech( argument, ch ), victim,
                 TO_CHAR );

            act( AT_SAY, "$n exclama a $N, '$t&c'", ch, drunk_speech( argument, ch ), victim,
                 TO_NOTVICT );

            act( AT_SAY, "$n te exclama , '$t&c'", ch, drunk_speech( argument, ch ), victim,
                 TO_VICT );

            break;

        default:

            act( AT_SAY, "Dices a $N '$t&c'", ch, drunk_speech( argument, ch ), victim,
                 TO_CHAR );

            act( AT_SAY, "$n dice a $N '$t&c'", ch, drunk_speech( argument, ch ), victim,
                 TO_NOTVICT );

            act( AT_SAY, "$n te dice '$t&c'", ch, drunk_speech( argument, ch ), victim,
                 TO_VICT );

            break;

    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
    {

        snprintf( buf, MAX_STRING_LENGTH, "%s: %s", IS_NPC( ch ) ? ch->short_descr : ch->name,
                  argument );

        append_to_file( LOG_FILE, buf );

    }

    mprog_speech_trigger( argument, ch );

    if ( char_died( ch ) )

        return;

    oprog_speech_trigger( argument, ch );

    if ( char_died( ch ) )

        return;

    rprog_speech_trigger( argument, ch );

    return;

}

void do_cpareja( CHAR_DATA *ch, char *argument )
{
	char buf[MSL];
    CHAR_DATA              *victim;
    char arg1[MAX_INPUT_LENGTH];
    argument = one_argument( argument, arg1 );

       if ( arg1[0] == '\0') {
send_to_char("¿Decirle a tu gran amor qué?\r\n", ch);
return;
}

    if ( ch->position == POS_FIGHTING || ch->position == POS_BERSERK || ch->position == POS_EVASIVE
         || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE ) {
        send_to_char( "¿Qué tal si terminas la pelea primero?\r\n", ch );
        return;
    }
	if( ch->pcdata->pareja== NULL)
	{
		send_to_char( "&cPrimero deberías casarte&P... ¡&cA que esperas&P!&D\r\n", ch );
		return;
	}
	if( ( victim= get_char_world ( ch, ch->pcdata->pareja) ) == NULL )
	{
		sprintf( buf, "&cNo encuentras a %s &cpor ningún lado&P.&D\r\n", ch->pcdata->pareja);
		send_to_char( buf, ch );
		return;
	}
		sprintf( buf, "&CTransmites telepáticamente con dulzura a %s '%s'.&w\r\n", ch->pcdata->pareja, arg1);
send_to_char(buf, ch);
victim= get_char_world ( ch, ch->pcdata->pareja);
		sprintf( buf, "&%s te transmite telepáticamente con dulzura '%s'.&w\r\n", ch->name, arg1);
send_to_char(buf, victim);
return;
}
