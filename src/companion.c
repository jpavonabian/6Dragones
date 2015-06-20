#include "h/mud.h"
/* companion code - attempt to allow players to
group with one another no matter what the level range,
also allow pkillers to pkill in any level range */

/* known bugs that would need fixing.  Like life command
occassionally hp, mana, move will go to all zeros instead
of their former, could be with crashes or copyovers, not
sure yet. */
void                    remove_all_equipment( CHAR_DATA *ch );

void do_companion( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *buddy;
    char                    arg[MIL];
    char                    buf[MIL];
    int                     sn;

    if ( IS_NPC( ch ) )
        return;

    set_char_color( AT_IMMORT, ch );
    argument = one_argument( argument, arg );

    if ( IS_IMMORTAL( ch ) )
        return;

    if ( ch->race == RACE_ANIMAL || xIS_SET( ch->act, ACT_BEASTMELD ) ||
       ch->Class == CLASS_DRAGONLORD )
      {
      send_to_char("No puedes hacer eso en tu forma actual.\r\n", ch );
      return;
      }

    if ( ( arg[0] == '\0' ) ) {
        send_to_char( "&cSintaxis: companyero <&Cpersona&c>\r\n", ch );
        send_to_char( "        Companyero revertir\r\n", ch );
        return;
    }

    if ( !xIS_SET( ch->act, PLR_LIFE ) && ch->hit != ch->max_hit ) {
        send_to_char( "Necesitas estar al 100% de vida, movimiento y mana para usar este comando.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_LIFE ) && str_cmp( arg, "revert" ) ) {
        send_to_char( "Ya estás en modo compañero, usa compañero revertir.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "revertir" ) ) {
        if ( !xIS_SET( ch->act, PLR_LIFE ) || ch->position == POS_FIGHTING ) {
            send_to_char( "ahora mismo no.\r\n", ch );
            return;
        }
        remove_all_equipment( ch );
        for ( sn = 1; sn < top_sn; sn++ )
            ch->pcdata->learned[sn] = ch->pcdata->dlearned[sn];
        ch->level = ch->pcdata->tmplevel;
        ch->trust = ch->pcdata->tmptrust;
        ch->max_hit = ch->pcdata->tmpmax_hit;
        ch->max_move = ch->pcdata->tmpmax_move;
        if ( IS_BLOODCLASS( ch ) ) {
            ch->max_blood = ch->pcdata->tmpmax_blood;
        }
        else {
            ch->max_mana = ch->pcdata->tmpmax_mana;
        }
        ch->pcdata->tmpmax_hit = 0;
        ch->pcdata->tmpmax_move = 0;
        if ( IS_BLOODCLASS( ch ) ) {
            ch->pcdata->tmpmax_blood = 0;
        }
        else {
            ch->pcdata->tmpmax_mana = 0;
        }
        ch->pcdata->tmplevel = 0;
        ch->pcdata->tmptrust = 0;
        if ( xIS_SET( ch->act, PLR_LIFE ) )
            xREMOVE_BIT( ch->act, PLR_LIFE );
        save_char_obj( ch );
        act( AT_MAGIC, "$n vuelve a ser quien era.", ch, NULL, NULL, TO_ROOM );
        act( AT_MAGIC, "Recuperas todo tu poder.", ch, NULL, NULL, TO_CHAR );
        snprintf( buf, MIL,
                  "%s ha dejado de ser compañero y vuelve a su nivel.",
                  ch->name );
        echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );
        if ( ch->max_hit > 100000 || ch->max_hit < 0 ) {
            ch->max_hit = 30000;
        }
        restore_char( ch );
// Adding fix for reverted characters remaining in group out of their 
// level range - Aurin 12/9/2010
        if ( ch->leader != NULL ) {
            do_follow( ch, ( char * ) "self" );
        }
        else {
            do_group( ch, ( char * ) "deshacer" );
        }
        return;
    }

    if ( ( buddy = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "No está en el juego.\r\n", ch );
        return;
    }

    if ( buddy ) {
        short                   level;
        short                   num;

        num = 1;
        level = buddy->level;
        if ( level > ch->level || level < 2 ) {
            send_to_char( "Nivel inválido.\r\n", ch );
            return;
        }

        if ( ch->pcdata->tmplevel == 0 )
            ch->pcdata->tmplevel = ch->level;
        if ( ch->pcdata->tmptrust == 0 )
            ch->pcdata->tmptrust = ch->trust;
        remove_all_equipment( ch );
        for ( sn = 1; sn < top_sn; sn++ )
            ch->pcdata->dlearned[sn] = ch->pcdata->learned[sn];
        ch->level = level;
        ch->trust = 0;
        ch->pcdata->tmpmax_hit = ch->max_hit;
        ch->pcdata->tmpmax_move = ch->max_move;
        if ( IS_BLOODCLASS( ch ) ) {
            ch->pcdata->tmpmax_blood = ch->max_blood;
        }
        else {
            ch->pcdata->tmpmax_mana = ch->max_mana;
        }
        save_char_obj( ch );
        ch->max_hit = 10;
        ch->hit = 10;
        ch->max_move = 10;
        ch->move = 10;

        if ( !xIS_SET( ch->act, PLR_LIFE ) )
            xSET_BIT( ch->act, PLR_LIFE );

        while ( num <= level ) {
            advance_level( ch );
            num++;
        }
        restore_char( ch );
        snprintf( buf, MIL, "%s ha usado el comando compañero para bajar de nivel.",
                  ch->name );
        echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );
        act( AT_MAGIC, "$n ahora es más débil.", ch, NULL, NULL, TO_ROOM );
        act( AT_MAGIC, "Notas como el poder escapa de ti. Te sientes más débil.", ch, NULL, NULL, TO_CHAR );
        return;
    }
    return;
}
