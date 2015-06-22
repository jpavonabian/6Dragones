#include "h/mud.h"

/* Let noobs go to places for free under level 20, after that take people to general locations at cost */
void do_activate( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *stone,
                           *rune = NULL;
    bool                    found = FALSE;
    char                    arg[MIL];
    ROOM_INDEX_DATA        *room;
    int                     rvnum = 0,
        ivnum,
        unum;

    argument = one_argument( argument, arg );

    if ( ch->level < 2 ) {
        send_to_char( "Debes completar el tutorial para usar este comando.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        return;
    }

    for ( stone = ch->in_room->first_content; stone; stone = stone->next_content ) {
        if ( stone->item_type == ITEM_STONE ) {
            found = TRUE;
            break;
        }
    }

    ivnum = ch->in_room->vnum;
    if ( ivnum == 18636 || ivnum == 1501 ) {
        if ( ivnum == 18636 )
            rvnum = 1501;                              // Shattered Refuge
        else if ( ivnum == 1501 )
            rvnum = 18636;                             // Back to Time Machine

        if ( !( room = get_room_index( rvnum ) ) ) {
            send_to_char( "No te podemos enviar all�. Por favor informa a un administrador.\r\n", ch );
            return;
        }

        send_to_char( "La m�quina del tiempo comienza a funcionar.\r\n", ch );
        char_from_room( ch );
        char_to_room( ch, room );
        if ( ch->mount ) {
            char_from_room( ch->mount );
            char_to_room( ch->mount, room );
        }

        act( AT_YELLOW, "$n aparece de un remolino de colores.", ch, NULL, ch, TO_ROOM );
        do_look( ch, ( char * ) "auto" );
        return;
    }

    if ( !found && ch->in_room->sector_type != SECT_PORTALSTONE ) {
        send_to_char( "Debes estar ante un portal de piedra para usar el comando activar.\r\n",
                      ch );
        return;
    }

    /*
     * Make level 20 and higher require a rune to use 
     */
    if ( ch->level >= 20 ) {
        if ( ms_find_obj( ch ) )
            return;

        if ( !( rune = get_eq_char( ch, WEAR_HOLD ) ) || rune->item_type != ITEM_RUNE ) {
            send_to_char( "Necesitas sostener una runa para activar el portal de piedra.\r\n", ch );
            return;
        }
    }

    if ( ch->level < 5 && ch->pcdata->portalstone > 1 ) {
        send_to_char( "El portal de piedra est� recargando sus energ�as m�gicas.\r\n", ch );
        ch->pcdata->portalstone -= 1;
        WAIT_STATE( ch, 5 );
        return;
    }
    short                   chance = number_range( 1, 4 );

    if ( ch->level < 5 && ch->pcdata->portalstone == 1 ) {
        if ( chance == 1 ) {
            send_to_char
                ( "el portal de piedra est� recargando sus energ�as m�gicas y estar� listo muy pronto.\r\n",
                  ch );
            ch->pcdata->portalstone -= 1;
        }
        if ( chance == 2 ) {
            send_to_char( "El portal de piedra emite un zumbido.\r\n", ch );
            ch->pcdata->portalstone -= 1;
        }
        if ( chance == 3 ) {
            send_to_char( "El portal de piedra produce calor.\r\n", ch );
            ch->pcdata->portalstone -= 1;
        }
        if ( chance == 4 ) {
            ch->pcdata->portalstone -= 1;
        }
        return;
    }

    if ( !xIS_SET( ch->act, PLR_ACTIVATE ) ) {
        if ( !arg || arg[0] == '\0' ) {
            act( AT_ORANGE,
                 "El portal de piedra emite un zumbido y una serie de lugares aparecen en tu mente.",
                 ch, NULL, ch, TO_CHAR );
            act( AT_CYAN, "$n toca el portal de piedra que comienza a emitir un zumbido.", ch, NULL, ch,
                 TO_ROOM );
            send_to_char
                ( "&cLos nombres de los lugares y su peligrosidad se revelan en tu mente.\r\n",
                  ch );
            xSET_BIT( ch->act, PLR_ACTIVATE );
        }
        else {
            send_to_char
                ( "No puedes activar el portal de piedra hacia la zona que tienes en mente.\r\n",
                  ch );
            send_to_char( "Sintaxis: Activar - Activa el portal de piedra.\r\n", ch );
            send_to_char
                ( "Sintaxis: Activar < nombre del lugar> - Te transporta al lugar elegido despu�s de que el portal se active.\r\n",
                  ch );
            return;
        }
    }

    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "\r\n\r\n&GLugar                      Nivel de dificultad\r\n", ch );
        send_to_char( "&G1. Merchant's Discovery        00 - 10\r\n", ch );
        send_to_char( "&G2. Norrinton's South Harbor    10 - 20\r\n", ch );
        send_to_char( "&G3. The Abyss                   05 - 10\r\n", ch );
        send_to_char( "&G4. Gnome Tower                 05 - 20\r\n", ch );
        send_to_char( "&G5. Kirwood Swamp               00 - 05\r\n", ch );
        send_to_char( "&G6. Abandoned Cabin             05 - 20\r\n", ch );
        send_to_char( "&G7. Garden                      15 - 20\r\n", ch );
        send_to_char( "&G8. Tufkul'ar                   15 - 30\r\n", ch );
        send_to_char( "&G9. Durdun                      10 - 20\r\n", ch );
        send_to_char( "&G10.Spiral Village              10 - 20\r\n", ch );
        send_to_char( "&G11.Ice Cavern                   5 - 20\r\n", ch );
        if ( ch->race == RACE_DRAGON ) {
            send_to_char( "&G12.Under the Volcano           10 - 20 [&RDragon Only Area&G]\r\n",
                          ch );
            send_to_char( "&cSintaxis: Activar < n�mero de lugar ( 1 - 12 )>\r\n", ch );
        }
        else {
            send_to_char( "&G12.Under Sewer                 03 - 10 [&RGroup Area&G]\r\n", ch );
            send_to_char( "&G13.Bowels of the Citadel       10 - 25 [&RGroup Area&G]\r\n", ch );
        // Only for the holidays
            send_to_char( "&G14. Santas Workshop            30 - 60\r\n", ch );
            send_to_char( "&cSintaxis: Activar < n�mero de lugar ( 1 - 13 )>\r\n", ch );
        }
        return;
    }

    if ( !is_number( arg ) ) {
        send_to_char( "Sintaxis: Activar <#>\r\n", ch );
        return;
    }
    unum = atoi( arg );

    if ( unum == 1 )
        rvnum = 2758;
    else if ( unum == 2 )
        rvnum = 10001;
    else if ( unum == 3 )
        rvnum = 7501;
    else if ( unum == 4 )
        rvnum = 18631;
    else if ( unum == 5 )
        rvnum = 27001;
    else if ( unum == 6 )
        rvnum = 29000;
    else if ( unum == 7 )
        rvnum = 4008;
    else if ( unum == 8 )
        rvnum = 28251;
    else if ( unum == 9 )
        rvnum = 18300;
    else if ( unum == 10 )
        rvnum = 13027;
    else if ( unum == 11 )
        rvnum = 6279;
    else if ( unum == 12 ) {
        if ( ch->race == RACE_DRAGON )
            rvnum = 8310;
        else
            rvnum = 4901;
    }
    else if ( unum == 13 )
        rvnum = 5300;
    // holidays only
    else if ( unum == 14 )
        rvnum = 34001;

    if ( !( room = get_room_index( rvnum ) ) ) {
        send_to_char( "No se te ha podido transportar ah�. Por favor avisa a alguien de la administraci�n.\r\n", ch );
        return;
    }

    if ( ch->level >= 20 && rune )
        extract_obj( rune );

    act( AT_ORANGE, "�Una extra�a luz de colores te envuelve!", ch, NULL, ch, TO_CHAR );
    act( AT_ORANGE, "�A $n le envuelve una extra�a luz de colores!", ch, NULL, ch, TO_ROOM );
    char_from_room( ch );

    char_to_room( ch, room );
    if ( ch->mount ) {
        char_from_room( ch->mount );
        char_to_room( ch->mount, room );
    }

    act( AT_ORANGE, "�La luz desaparece!", ch, NULL, ch, TO_CHAR );
    act( AT_ORANGE, "�$n aparece en una luz de colores!", ch, NULL, ch, TO_ROOM );
    do_look( ch, ( char * ) "auto" );
    xREMOVE_BIT( ch->act, PLR_ACTIVATE );

    if ( ch->level < 5 && ch->pcdata->portalstone == 0 ) {
        ch->pcdata->portalstone = 7;
    }

}
