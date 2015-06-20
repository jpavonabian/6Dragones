#include "h/mud.h"

// Revamped pet code is going here.

void do_beckon( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *mob;
    bool                    found;

    set_pager_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) )
        return;

    found = FALSE;

    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            act( AT_CYAN, "$n llama a $N a su lado.", ch, NULL, mob, TO_ROOM );
            act( AT_CYAN, "Llamas a $N a tu lado.", ch, NULL, mob, TO_CHAR );
            ch_printf( ch, "\r\n%s aparece repentinamente a tu lado.\r\n", capitalize( mob->short_descr ) );
            if ( xIS_SET( mob->affected_by, AFF_GRAZE ) )
                xREMOVE_BIT( mob->affected_by, AFF_GRAZE );
            char_from_room( mob );
            char_to_room( mob, ch->in_room );
        }
    }

    if ( !found )
        send_to_char( "No tienes ninguna mascota a la que llamar.\r\n", ch );
}

void do_graze( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *mob;
    bool                    found;

    set_pager_color( AT_PLAIN, ch );
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            if ( xIS_SET( mob->affected_by, AFF_GRAZE ) ) {
                xREMOVE_BIT( mob->affected_by, AFF_GRAZE );
                act( AT_CYAN, "$n le dice a $N que deje de pastar y le atienda.", ch, NULL, mob,
                     TO_ROOM );
                act( AT_CYAN, "Le dices a $N que deje de pastar y te atienda.", ch, NULL, mob,
                     TO_CHAR );
                return;
            }
            found = TRUE;
            if ( ch->position == POS_MOUNTED )
                do_dismount( ch, ( char * ) "" );
            act( AT_CYAN, "$n deja libre a  $N para pastar y comer.", ch, NULL, mob, TO_ROOM );
            act( AT_CYAN, "Dejas libre a $N para pastar y comer.", ch, NULL, mob, TO_CHAR );
            xSET_BIT( mob->affected_by, AFF_GRAZE );
        }
    }
    if ( !found )
        send_to_char( "No tienes ninguna mascota.\r\n", ch );
}

void do_distract( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    bool                    found;
    CHAR_DATA              *mob;

    found = FALSE;

    if ( !ch->fighting ) {
        send_to_char( "¿distraer a qué? ni siquiera estás luchando.\r\n", ch );
        return;
    }

    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            break;
        }
    }

    if ( !found ) {
        send_to_char( "No tienes una mascota.\r\n", ch );
        return;
    }

    if ( xIS_SET( mob->act, ACT_MOUNTABLE ) ) {
        send_to_char
            ( "Las mascotas montables no pueden ser usadas para esto. Solo las invocadas pueden usar el comando.\r\n",
              ch );
        return;
    }

    if ( !mob->fighting || !( victim = who_fighting( mob ) ) ) {
        send_to_char( "No está luchando aún.\r\n", ch );
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        send_to_char( "No puedes hacer eso aquí.\r\n", ch );
        return;
    }

    stop_fighting( victim, TRUE );
    set_fighting( victim, mob );
    start_hating( victim, mob );
    mob->hate_level = ch->hate_level + 2;
    WAIT_STATE( ch, 12 );
    act( AT_LBLUE, "¡Haces una señal a $N para que comienze a distraer a tu enemigo!", ch, NULL, mob, TO_CHAR );
    act( AT_LBLUE, "$n hace una señal a $N para que comience a distraer a su enemigo.", ch, NULL, mob,
         TO_NOTVICT );
    act( AT_CYAN, "\r\n$N deja de luchar momentáneamente\r\n¡$N gruñe amenazadoramente a $n, y ataca a $m!", mob,
         NULL, victim, TO_NOTVICT );
}
