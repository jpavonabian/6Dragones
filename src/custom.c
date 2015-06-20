/***************************************************************************** * SKILLS MODULE * 
****************************************************************************/

#include "h/mud.h"
#include "h/damage.h"
#include "h/polymorph.h"

void                    remove_all_equipment( CHAR_DATA *ch );

void do_maul( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Maul attack who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->Class != CLASS_BEAR ) {
        send_to_char( "You have to be a bear to maul someone.\r\n", ch );
        return;
    }

    if ( ch->move < nomore ) {
        send_to_char( "You do not have enough move to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    send_to_char( "\r\n\r\n&OYou take off in blurring speed!!\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_maul ) ) {
        WAIT_STATE( ch, skill_table[gsn_maul]->beats );
        act( AT_GREEN, "You deliver a mauling attack with your claws at $N!", ch,
             NULL, victim, TO_CHAR );
        act( AT_GREEN,
             "Your are knocked down from the speed and power of $n's mauling attack!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_maul );
        ch->move = ( ch->move - nomore );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/maul.wav)\r\n", ch );

        global_retcode = damage( ch, victim, extrahigh, gsn_maul );

    }
    else {
        learn_from_failure( ch, gsn_maul );
        global_retcode = damage( ch, victim, 0, gsn_maul );
        send_to_char( "&cYour maul attack misses the target.\r\n", ch );
    }
}


void do_ferocious_strike( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a ferocious strike at who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( ch->move < nomore ) {
        send_to_char( "You do not have enough move to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You cannot deliver a ferocious strike while mounted.\r\n", ch );
        return;
    }
    send_to_char( "\r\n\r\n&OYou attack with an incredible rate of speed!\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_ferocious_strike ) ) {
        WAIT_STATE( ch, skill_table[gsn_ferocious_strike]->beats );
        act( AT_GREEN, "You deliver a ferocious strike to $N!", ch, NULL, victim, TO_CHAR );
        act( AT_GREEN,
             "Your vision blurs from $n's ferocious strike!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_ferocious_strike );
        ch->move = ( ch->move - nomore );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/strike.wav)\r\n", ch );

            global_retcode = damage( ch, victim, ludicrous, gsn_ferocious_strike );

    }
    else {
	learn_from_failure( ch, gsn_ferocious_strike );
        global_retcode = damage( ch, victim, 0, gsn_ferocious_strike );
        send_to_char( "&cYour ferocious strike just misses it's target.\r\n", ch );
    }
}


void do_elbow( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a downward elbow strike at who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->move < nomore ) {
        send_to_char( "You do not have enough move to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You cannot deliver a downward elbow strike while mounted.\r\n", ch );
        return;
    }
    send_to_char( "\r\n\r\n&OYou leap forward with at a incredible rate of speed!\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_elbow ) ) {
        WAIT_STATE( ch, skill_table[gsn_elbow]->beats );
        act( AT_GREEN, "You deliver a downward elbow strike to $N!", ch, NULL, victim, TO_CHAR );
        act( AT_GREEN,
             "Your vision blurs from the ferocity of $n's downward elbow strike!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_elbow );
        ch->move = ( ch->move - nomore );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/elbow.wav)\r\n", ch );

            global_retcode = damage( ch, victim, high, gsn_elbow );

    }
    else {
	learn_from_failure( ch, gsn_elbow );
        global_retcode = damage( ch, victim, 0, gsn_elbow );
        send_to_char( "&cYour downward elbow strike just misses it's target.\r\n", ch );
    }
}



// Start of Shadow Knight skills
void do_knee( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a high knee strike at who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->move < nomore ) {
        send_to_char( "You do not have enough move to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You cannot deliver a high knee while mounted.\r\n", ch );
        return;
    }

    send_to_char( "\r\n\r\n&OYou leap up high in the air!\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_knee ) ) {
        WAIT_STATE( ch, skill_table[gsn_knee]->beats );
        act( AT_GREEN, "You deliver a high knee strike to $N!", ch, NULL, victim, TO_CHAR );
        act( AT_GREEN,
             "Your vision blurs from the ferocity of $n's high knee strike!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_knee );
        ch->move = ( ch->move - nomore );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/knee.wav)\r\n", ch );

        if ( ch->Class != CLASS_MONK )
            global_retcode = damage( ch, victim, medium, gsn_knee );
        else
            global_retcode = damage( ch, victim, mediumhigh, gsn_knee );

    }
    else {
        learn_from_failure( ch, gsn_knee );
        global_retcode = damage( ch, victim, 0, gsn_knee );
        send_to_char( "&cYour high knee strike just misses it's target.\r\n", ch );
    }
}

void do_chilled_touch( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Use a chilled touch at who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->move < nomore ) {
        send_to_char( "You do not have enough move to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    send_to_char( "\r\n\r\n&WYour hands turn white from cold!\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_chilled_touch ) ) {
        WAIT_STATE( ch, skill_table[gsn_chilled_touch]->beats );
        act( AT_CYAN, "Your chilled touch BURNS $N's flesh!", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "Your skin literally burns from $n's chilled touch!", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_chilled_touch );
        ch->move = ( ch->move - nomore );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/chilled.wav)\r\n", ch );
        global_retcode = damage( ch, victim, mediumhigh, gsn_chilled_touch );

    }
    else {
        learn_from_failure( ch, gsn_chilled_touch );
        global_retcode = damage( ch, victim, 0, gsn_chilled_touch );
        send_to_char( "&cYour chilled touch just misses it's target.\r\n", ch );
    }
}

void do_frost_touch( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Use a frost touch at who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->move < nomore ) {
        send_to_char( "You do not have enough move to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    send_to_char( "\r\n\r\n&WYour hands turn white from cold!\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_frost_touch ) ) {
        WAIT_STATE( ch, skill_table[gsn_frost_touch]->beats );
        act( AT_CYAN, "Your frost touch BURNS $N's flesh!", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN,
             "Your skin literally burns from $n's frost touch!", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_frost_touch );
        ch->move = ( ch->move - nomore );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/frost.wav)\r\n", ch );
        global_retcode = damage( ch, victim, high, gsn_frost_touch );

    }
    else {
        learn_from_failure( ch, gsn_frost_touch );
        global_retcode = damage( ch, victim, 0, gsn_frost_touch );
        send_to_char( "&cYour frost touch just misses it's target.\r\n", ch );
    }
}

void do_icy_touch( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Use a icy touch at who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->move < nomore ) {
        send_to_char( "You do not have enough move to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }
    send_to_char( "\r\n\r\n&WYour hands turn white from cold!\r\n", ch );
    if ( can_use_skill( ch, number_percent(  ), gsn_icy_touch ) ) {
        WAIT_STATE( ch, skill_table[gsn_icy_touch]->beats );
        act( AT_CYAN, "Your icy touch BURNS $N's flesh!", ch, NULL, victim, TO_CHAR );
        act( AT_CYAN, "Your skin literally burns from $n's icy touch!", ch, NULL, victim, TO_VICT );

        learn_from_success( ch, gsn_icy_touch );
        ch->move = ( ch->move - nomore );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/ice.wav)\r\n", ch );
        global_retcode = damage( ch, victim, extrahigh, gsn_icy_touch );

    }
    else {
        learn_from_failure( ch, gsn_icy_touch );
        global_retcode = damage( ch, victim, 0, gsn_icy_touch );
        send_to_char( "&cYour icy touch just misses it's target.\r\n", ch );
    }
}

void do_tame( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;

    if ( xIS_SET( ch->act, PLR_BOUGHT_PET ))
    {
       send_to_char("You canot gain affinity with more then one pet at a time.\r\n", ch );
       return;
    }

    if ( ch->fighting )
        victim = who_fighting( ch );
    else {
        send_to_char( "You can't tame a animal your not fighting.\r\n", ch );
        return;
    }

    if ( victim->hit > victim->max_hit / 2 ) {
        send_to_char( "They are not weakened enough to try to tame yet.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_tame ) ) {
        WAIT_STATE( ch, skill_table[gsn_tame]->beats );
        add_follower( victim, ch );
        xSET_BIT( ch->act, PLR_BOUGHT_PET );
        xSET_BIT( victim->act, ACT_BEASTPET );
        xSET_BIT( victim->act, ACT_PET );
        xSET_BIT( victim->affected_by, AFF_CHARM );
        xSET_BIT( victim->affected_by, AFF_INFRARED );
        xSET_BIT( victim->attacks, ATCK_BITE );
        xSET_BIT( victim->defenses, DFND_DODGE );
        xSET_BIT( victim->xflags, PART_GUTS );
        xREMOVE_BIT( victim->act, ACT_AGGRESSIVE );
        xREMOVE_BIT( victim->act, ACT_WILD_AGGR );
        ch->pcdata->charmies++;
        ch->pcdata->pet = victim;
        ch->pcdata->petlevel = victim->level;
        ch->pcdata->petaffection = 10;
        ch->pcdata->pethungry = 950;
        ch->pcdata->petthirsty = 950;
        victim->hit = ch->hit;
        victim->max_hit = victim->hit;
        victim->move = ch->move;
        victim->max_move = victim->move;
        xSET_BIT( victim->affected_by, AFF_TRUESIGHT );
        stop_fighting( victim, TRUE );
        stop_hunting( victim );
        stop_hating( victim );
        ch->hate_level = 0;
        ch == victim->master;
        act( AT_BLUE, "\r\nYou have weakened $N to the point, $E can be tamed.\r\n", ch,
             NULL, victim, TO_CHAR );
        act( AT_BLUE, "$n has weakened $N to the point $E can be tamed.\r\n", ch,
             NULL, victim, TO_ROOM );
        learn_from_success( ch, gsn_tame );
        return;
    }
    else {
        act( AT_BLUE, "\r\nYou try to tame a ferral $N, but fail.\r\n", ch, NULL, victim, TO_CHAR );
        act( AT_BLUE, "$n tries to tame a ferral $N, but fails.\r\n", ch, NULL, victim, TO_ROOM );
        learn_from_failure( ch, gsn_tame );
    }
    return;
}

void do_bull_spirit( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *mob;
    bool                    found;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( mob ) {
            if ( IS_NPC( mob ) && mob->in_room && ch->in_room ) {
                if ( mob->master ) {
                    if ( !str_cmp( ch->name, mob->master->name )
                         && ( mob->in_room == ch->in_room ) ) {
                        found = TRUE;
                        break;
                    }
                }
            }
	}
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_bull_spirit ) ) {
        affect_strip( ch, gsn_bull_spirit );
        af.type = gsn_bull_spirit;
        af.duration = ch->level;
        af.location = APPLY_CON;
        af.modifier = 1+ch->level/17;
        af.level = ch->level;
        xCLEAR_BITS( af.bitvector );
        affect_to_char( ch, &af );

        if ( found == TRUE ) {
            affect_strip( mob, gsn_bull_spirit );
            af.type = gsn_bull_spirit;
            af.duration = ch->level;
            af.location = APPLY_CON;
            af.modifier = 1+ch->level/17;
            af.level = ch->level;
             xCLEAR_BITS( af.bitvector );
            affect_to_char( mob, &af );
        }

	learn_from_success( ch, gsn_bull_spirit );
        act( AT_PLAIN, "Your bull spirit fills your being.", ch, NULL, NULL,
             TO_CHAR );
        return;
    }
    else {
	act( AT_PLAIN, "Your attempt using a bull spirit fails to fully work.", ch, NULL, NULL,
             TO_CHAR );
        learn_from_failure( ch, gsn_bull_spirit );
    }
return;
}

void do_bear_spirit( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *mob;
    bool                    found;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( mob ) {
            if ( IS_NPC( mob ) && mob->in_room && ch->in_room ) {
                if ( mob->master ) {
                    if ( !str_cmp( ch->name, mob->master->name )
                         && ( mob->in_room == ch->in_room ) ) {
                        found = TRUE;
                        break;
                    }
                }
            }
          }
        }

    if ( can_use_skill( ch, number_percent(  ), gsn_bear_spirit ) ) {
        affect_strip( ch, gsn_bear_spirit );
        af.type = gsn_bear_spirit;
        af.duration = ch->level;
        af.location = APPLY_STR;
        af.modifier = 1+ch->level/17;
        af.level = ch->level;
        xCLEAR_BITS( af.bitvector );
        affect_to_char( ch, &af );

        if ( found == TRUE ) {
            affect_strip( mob, gsn_bear_spirit );
            af.type = gsn_bear_spirit;
            af.duration = ch->level;
            af.location = APPLY_STR;
            af.modifier = 1+ch->level/17;
            af.level = ch->level;
             xCLEAR_BITS( af.bitvector );
            affect_to_char( mob, &af );
        }

	learn_from_success( ch, gsn_bear_spirit );
        act( AT_PLAIN, "Your bear spirit fills your being.", ch, NULL, NULL,
             TO_CHAR );
        return;
    }
    else {
	act( AT_PLAIN, "Your attempt using a bear spirit fails to fully work.", ch, NULL, NULL,
             TO_CHAR );
        learn_from_failure( ch, gsn_bear_spirit );
    }
return;


}
void do_wolf_spirit( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *mob;
    bool                    found;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( mob ) {
            if ( IS_NPC( mob ) && mob->in_room && ch->in_room ) {
                if ( mob->master ) {
                    if ( !str_cmp( ch->name, mob->master->name )
                         && ( mob->in_room == ch->in_room ) ) {
                        found = TRUE;
                        break;
                    }
                }
            }
          }
        }
    if ( can_use_skill( ch, number_percent(  ), gsn_wolf_spirit ) ) {
        affect_strip( ch, gsn_wolf_spirit );
        af.type = gsn_wolf_spirit;
        af.duration = ch->level;
        af.location = APPLY_MOVE;
        af.modifier = 50+ch->level*20;
        af.level = ch->level;
        xCLEAR_BITS( af.bitvector );
        affect_to_char( ch, &af );

        if ( found == TRUE ) {
            affect_strip( mob, gsn_wolf_spirit );
            af.type = gsn_wolf_spirit;
            af.duration = ch->level;
            af.location = APPLY_MOVE;
            af.modifier = 50+ch->level*20;
            af.level = ch->level;
             xCLEAR_BITS( af.bitvector );
            affect_to_char( mob, &af );
              }
	learn_from_success( ch, gsn_wolf_spirit );
        act( AT_PLAIN, "Your wolf spirit fills your being.", ch, NULL, NULL,
             TO_CHAR );
        return;
    }
    else {
	act( AT_PLAIN, "Your attempt using a wolf spirit fails to fully work.", ch, NULL, NULL,
             TO_CHAR );
        learn_from_failure( ch, gsn_wolf_spirit );
    }

}



void do_shroud_spirit( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *mob;
    bool                    found;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( mob ) {
            if ( IS_NPC( mob ) && mob->in_room && ch->in_room ) {
                if ( mob->master ) {
                    if ( !str_cmp( ch->name, mob->master->name )
                         && ( mob->in_room == ch->in_room ) ) {
                        found = TRUE;
                        break;
                    }
                }
            }
        }
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_shroud_spirit ) ) {
        affect_strip( ch, gsn_shroud_spirit );
        xREMOVE_BIT( ch->affected_by, AFF_HIDE );
        af.type = gsn_shroud_spirit;
        af.duration = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_HIDE );
        affect_to_char( ch, &af );

        if ( found == TRUE ) {
            affect_strip( mob, gsn_shroud_spirit );
            xREMOVE_BIT( mob->affected_by, AFF_HIDE );
            af.type = gsn_shroud_spirit;
            af.duration = ch->level;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.level = ch->level;
            af.bitvector = meb( AFF_HIDE );
            affect_to_char( mob, &af );
        }

        learn_from_success( ch, gsn_shroud_spirit );
        act( AT_PLAIN, "Your shroud spirit sends everyone into a hidden position.", ch, NULL, NULL,
             TO_CHAR );
        return;
    }
    else {
        act( AT_PLAIN, "Your attempt at a shroud spirit fails to fully work.", ch, NULL, NULL,
             TO_CHAR );
        learn_from_failure( ch, gsn_shroud_spirit );
    }

}

void do_hog_affinity( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA            *paf;
    int                     sn = 1;
    MOB_INDEX_DATA         *temp;
    CHAR_DATA              *mob;
    AFFECT_DATA             af;
    char                    buf[MSL];
    char                   *name;
    bool                    found;

    if ( IS_NPC( ch ) ) {
        return;
    }

    if ( ( temp = get_mob_index( MOB_VNUM_LSKELLIE ) ) == NULL ) {
        bug( "Skill_Hog_affinity: Hog vnum %d doesn't exist.", MOB_VNUM_LSKELLIE );
        return;
    }

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            break;
        }
    }

    if ( xIS_SET( ch->act, PLR_BOUGHT_PET ) && found == TRUE ) {
        send_to_char( "You already have a pet, dismiss it first.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_hog_affinity ) ) {
        WAIT_STATE( ch, skill_table[gsn_hog_affinity]->beats );
        mob = create_mobile( temp );
        char_to_room( mob, ch->in_room );
        mob->race = 82;
        mob->level = ch->level;
        mob->hit = ch->hit;
        mob->max_hit = mob->hit;
        if ( ch->level < 20 ) {
            mob->move = ch->level * 100;
        }
        else if ( ch->level > 19 && ch->level < 50 ) {
            mob->move = ch->level * 150;
        }
        else {
            mob->move = ch->level * 200;
        }
        mob->max_move = mob->move;
        mob->perm_str = 20;
        mob->sex = 2;
        mob->armor = set_armor_class( mob->level );
        mob->hitroll = set_hitroll( mob->level );
        mob->damroll = set_damroll( mob->level );
        mob->numattacks = set_num_attacks( mob->level );
        mob->hitplus = set_hp( mob->level );
        mob->alignment = ch->alignment;
        mudstrlcpy( buf, "feral hog", MSL );
        STRFREE( mob->name );
        mob->name = STRALLOC( buf );
        if ( VLD_STR( mob->short_descr ) )
            STRFREE( mob->short_descr );
        mob->short_descr = STRALLOC( "a feral hog" );
        snprintf( buf, MSL, "A feral hog is rooting around here.\r\n" );
        if ( VLD_STR( mob->description ) )
            STRFREE( mob->long_descr );
        mob->long_descr = STRALLOC( buf );
        mob->color = 1;
        act( AT_BLUE, "\r\nYou rely on your hog affinity to call yourself a feral hog.\r\n", ch,
             NULL, NULL, TO_CHAR );
        act( AT_BLUE, "$n makes some grunting sounds, and a feral hog comes to $s side.\r\n", ch,
             NULL, NULL, TO_ROOM );
        add_follower( mob, ch );
        xSET_BIT( ch->act, PLR_BOUGHT_PET );
        xSET_BIT( mob->act, ACT_PET );
        xSET_BIT( mob->affected_by, AFF_CHARM );
        xSET_BIT( mob->attacks, ATCK_BITE );
        xSET_BIT( mob->defenses, DFND_DODGE );
        xSET_BIT( mob->xflags, PART_GUTS );
        learn_from_success( ch, gsn_hog_affinity );
        ch->pcdata->charmies++;
        ch->pcdata->pet = mob;
        xSET_BIT( mob->affected_by, AFF_TRUESIGHT );
        return;
    }
    else {
        act( AT_CYAN, "You make some grunting noises.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n makes some grunting noises.\r\n", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You loose your concentration and fail to call out to a feral hog.\r\n", ch );
        learn_from_failure( ch, gsn_hog_affinity );
        return;
    }
    return;
}

void do_steed( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA            *paf;
    int                     sn = 1;
    MOB_INDEX_DATA         *temp;
    CHAR_DATA              *mob;
    AFFECT_DATA             af;
    char                    buf[MSL];
    OBJ_DATA               *obj;
    char                   *name;
    bool                    found;

    if ( IS_NPC( ch ) ) {
        return;
    }

    if ( ( temp = get_mob_index( MOB_VNUM_LSKELLIE ) ) == NULL ) {
        bug( "Skill_Steed: Steed vnum %d doesn't exist.", MOB_VNUM_LSKELLIE );
        return;
    }

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master ) {
            found = TRUE;
            break;
        }
    }

    if ( xIS_SET( ch->act, PLR_BOUGHT_PET ) && found == TRUE ) {
        send_to_char( "You already have a pet, dismiss it first.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_steed ) ) {
        WAIT_STATE( ch, skill_table[gsn_steed]->beats );
        mob = create_mobile( temp );
        char_to_room( mob, ch->in_room );
        mob->race = 82;
        mob->level = ch->level;
        mob->hit = set_hp( mob->level );
        mob->max_hit = set_hp( mob->level );
        mob->hit = mob->hit * 2;
        mob->max_hit = mob->max_hit * 2;
        if ( ch->level < 20 ) {
            mob->move = ch->level * 100;
        }
        else if ( ch->level > 19 && ch->level < 50 ) {
            mob->move = ch->level * 150;
        }
        else {
            mob->move = ch->level * 200;
        }
        mob->max_move = mob->move;
        mob->perm_str = 20;
        mob->sex = 2;
        mob->armor = set_armor_class( mob->level );
        mob->hitroll = set_hitroll( mob->level );
        mob->damroll = set_damroll( mob->level );
        mob->numattacks = set_num_attacks( mob->level );
        mob->hitplus = set_hp( mob->level );
        mob->alignment = ch->alignment;
        mudstrlcpy( buf, "warhorse war horse", MSL );
        STRFREE( mob->name );
        mob->name = STRALLOC( buf );
        if ( VLD_STR( mob->short_descr ) )
            STRFREE( mob->short_descr );
        if ( ch->level < 20 ) {
            mob->short_descr = STRALLOC( "a light warhorse" );
            snprintf( buf, MSL, "A light warhorse stands here.\r\n" );
        }
        else if ( ch->level > 19 && ch->level < 50 ) {
            mob->short_descr = STRALLOC( "a medium warhorse" );
            snprintf( buf, MSL, "A medium warhorse stands here.\r\n" );
        }
        else {
            mob->short_descr = STRALLOC( "a heavy warhorse" );
            snprintf( buf, MSL, "A heavy warhorse stands here.\r\n" );
        }
        if ( VLD_STR( mob->description ) )
            STRFREE( mob->long_descr );
        mob->long_descr = STRALLOC( buf );
        mob->color = 1;
        act( AT_BLUE, "\r\nYou call out to your warhorse steed.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_BLUE,
             "$n utters some arcane words, and a warhorse steed comes to $s side.\r\n",
             ch, NULL, NULL, TO_ROOM );
        add_follower( mob, ch );
        xSET_BIT( ch->act, PLR_BOUGHT_PET );
        xSET_BIT( mob->act, ACT_PET );
        xSET_BIT( mob->affected_by, AFF_CHARM );
        xSET_BIT( mob->act, ACT_MOUNTABLE );
        xSET_BIT( mob->attacks, ATCK_BITE );
        xSET_BIT( mob->defenses, DFND_DODGE );
        xSET_BIT( mob->xflags, PART_GUTS );
        learn_from_success( ch, gsn_steed );
        ch->pcdata->charmies++;
        ch->pcdata->pet = mob;
        xSET_BIT( mob->affected_by, AFF_TRUESIGHT );
        obj = create_object( get_obj_index( 1281 ), 0 );
        obj->level = mob->level;
        if ( mob->level > 19 ) {
            // affect 1
            CREATE( paf, AFFECT_DATA, 1 );

            paf->type = sn;
            paf->duration = -1;
            paf->location = APPLY_NONE;
            paf->modifier = 0;
            paf->bitvector = meb( AFF_AQUA_BREATH );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }
        if ( mob->level > 49 ) {
            // affect 2
            CREATE( paf, AFFECT_DATA, 1 );

            paf->type = sn;
            paf->duration = -1;
            paf->location = APPLY_NONE;
            paf->modifier = 0;
            paf->bitvector = meb( AFF_PROTECT );
            LINK( paf, obj->first_affect, obj->last_affect, next, prev );
        }
        obj_to_char( obj, mob );
        equip_char( mob, obj, WEAR_WAIST );

        return;
    }
    else {
        act( AT_CYAN, "You utter some arcane words.\r\n", ch, NULL, NULL, TO_CHAR );
        act( AT_CYAN, "$n utters some arcane words.\r\n", ch, NULL, NULL, TO_ROOM );
        send_to_char( "You speak an incorrect phrase and fail to call to your steed.\r\n", ch );
        learn_from_failure( ch, gsn_steed );
        return;
    }
    return;
}

void do_poultice( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    short                   nomore;

    nomore = 5;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        victim = ch;
    }

    // Prevent ch from healing mob they're fighting. -Taon
    if ( IS_NPC( victim ) && who_fighting( ch ) == victim ) {
        send_to_char( "You cant bring yourself to heal your foe.\r\n", ch );
        return;
    }

    if ( ch->mana < 10 ) {
        send_to_char( "You don't have enough mana to do that.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_poultice ) ) {
        WAIT_STATE( ch, skill_table[gsn_poultice]->beats );

        if ( victim->hit >= victim->max_hit ) {
            send_to_char( "There is no wound to poultice.\r\n", ch );
            return;
        }

        if ( victim->hit < victim->max_hit ) {
            if ( xIS_SET( ch->act, PLR_BATTLE ) )
                send_to_char( "!!SOUND(sound/cure.wav)\r\n", ch );
            ch->mana -= nomore;
            act( AT_CYAN, "You gather the herbs necessary and create a poultice.", ch, NULL, victim,
                 TO_CHAR );
            if ( victim = ch ) {
                act( AT_CYAN, "You apply the poultice to your wounds.", ch, NULL, victim, TO_CHAR );
            }
            else {
                act( AT_CYAN, "$n applies a poultice to your wounds.", ch, NULL, victim, TO_VICT );
            }
            act( AT_CYAN, "$n applies a poultice to $N's wounds.", ch, NULL, victim, TO_NOTVICT );
            if ( victim->level < 20 ) {
                victim->hit += number_range( 1, 6 );
            }
            if ( victim->level > 19 && victim->level < 60 ) {
                victim->hit += number_range( 3, 12 );
            }
            if ( victim->level > 59 ) {
                victim->hit += number_range( 5, 20 );
            }

            if ( victim->hit >= victim->max_hit ) {
                victim->hit = victim->max_hit;
            }
            learn_from_success( ch, gsn_poultice );
            return;
        }
    }
    else {
        act( AT_CYAN,
             "You gather the herbs necessary and create a poultice, but don't mix it properly.", ch,
             NULL, victim, TO_CHAR );
        learn_from_failure( ch, gsn_poultice );
        WAIT_STATE( ch, skill_table[gsn_poultice]->beats );
        return;
    }

}

void do_butterfly_kick( CHAR_DATA *ch, char *argument )
{
    short                   nomore;

    nomore = 10;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Send a butterfly kick at who?\r\n", ch );
            return;
        }
    }

    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( ch->move < nomore ) {
        send_to_char( "You do not have enough move to do that.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "Suicide is a mortal sin.\r\n", ch );
        return;
    }

    if ( ch->mount ) {
        send_to_char( "You cannot deliver a butterfly kick while mounted.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_butterfly_kick ) ) {
        WAIT_STATE( ch, skill_table[gsn_butterfly_kick]->beats );
        act( AT_GREEN, "You deliver a butterfly kick to $N!", ch, NULL, victim, TO_CHAR );

        act( AT_GREEN,
             "Your vision blurs from the ferocity of $n's butterfly kick!",
             ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_butterfly_kick );
        ch->move = ( ch->move - nomore );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/butterflykick.wav)\r\n", ch );

        global_retcode = damage( ch, victim, extrahigh, gsn_butterfly_kick );

    }

    else {
        learn_from_failure( ch, gsn_butterfly_kick );
        global_retcode = damage( ch, victim, 0, gsn_butterfly_kick );
        send_to_char( "&cYour butterfly kick just misses it's target.\r\n", ch );
    }
}

void animal_change( CHAR_DATA *ch, bool tohuman )
{                                                      /* tohuman=TRUE if going to human,
                                                        * * =FALSE if going to animal */
    short                   backup[MAX_SKILL];
    bool                    dshowbackup[MAX_SKILL];
    int                     sn = 0;
    short                   ability = 0;
    AFFECT_DATA             af;
    bool                    fForm = FALSE;
    MORPH_DATA             *morph;
    char                    buf[MSL];

    if ( IS_NPC( ch ) ) {
        send_to_char( "You can't do animal_change if you are NPC!", ch );
        return;
    }

    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "You can't do animal_change if you are sleeping!", ch );
        return;
    }

    for ( sn = 0; sn < MAX_SKILL; sn++ ) {
        dshowbackup[sn] = FALSE;
        backup[sn] = 0;
    }

    if ( tohuman ) {
        send_to_char( "\r\nYou remove all your gear as your shape is about to vastly change.\r\n",
                      ch );
        remove_all_equipment( ch );

        ch->pcdata->tmprace = ch->race;
        ch->pcdata->tmpclass = ch->Class;
        ch->race = RACE_ANIMAL;
        short                   num = 0;

        if ( ch->pcdata->choice == 1 )
            num = 1001;
        else if ( ch->pcdata->choice == 2 )
            num = 1004;
        else if ( ch->pcdata->choice == 3 )
            num = 1002;
        else if ( ch->pcdata->choice == 4 )
            num = 1003;
        else if ( ch->pcdata->choice == 5 )
            num = 1006;
        else if ( ch->pcdata->choice == 6 )
            num = 1007;

        morph = get_morph_vnum( num );

        if ( !morph ) {
            send_to_char( "No morph data stopped here.\r\n", ch );
            return;
        }

        if ( ch->pcdata->choice == 1 ) {
            ch->Class = CLASS_WOLF;
            morph->vnum = 1001;
        }
        else if ( ch->pcdata->choice == 2 ) {
            ch->Class = CLASS_OWL;
            morph->vnum = 1004;
        }
        else if ( ch->pcdata->choice == 3 ) {
            ch->Class = CLASS_FISH;
            morph->vnum = 1002;
        }
        else if ( ch->pcdata->choice == 4 ) {
            ch->Class = CLASS_BEAR;
            morph->vnum = 1003;
        }
        else if ( ch->pcdata->choice == 5 ) {
            ch->Class = CLASS_GRYPHON;
            morph->vnum = 1006;
        }
        else if ( ch->pcdata->choice == 6 ) {
            ch->Class = CLASS_TREANT;
            morph->vnum = 1007;
        }
        if ( ch->race == RACE_ANIMAL && ch->secondclass != -1 ) {
            ch->pcdata->tmpsecond = ch->secondclass;
            ch->secondclass = -1;
        }
        if ( ch->race == RACE_ANIMAL && ch->thirdclass != -1 ) {
            ch->pcdata->tmpthird = ch->thirdclass;
            ch->thirdclass = -1;
        }

        fForm = TRUE;

        if ( fForm ) {
            af.type = gsn_animal_form;
            af.duration = -1;
            af.level = ch->level;
            af.location = APPLY_AFFECT;
            af.modifier = 0;
            af.bitvector = meb( AFF_ANIMALFORM );
            affect_to_char( ch, &af );

            snprintf( buf, MSL, "%d", morph->vnum );
            do_imm_morph( ch, buf );
        }

        if ( ch->morph == NULL || ch->morph->morph == NULL ) {
            fForm = FALSE;
            return;
        }

        /*
         * Ok this is a simple way of handling it, we toss through the sns and set
         * everything up as we go 
         */
        for ( sn = 0; sn < MAX_SKILL; sn++ ) {
            /*
             * Toss into backup 
             */
            if ( ch->pcdata->learned[sn] > 0 ) {
                backup[sn] = ch->pcdata->learned[sn];
                dshowbackup[sn] = ch->pcdata->dshowlearned[sn];
            }

            /*
             * Toss on animal skills 
             */
            if ( ch->pcdata->dlearned[sn] > 0 ) {
                ch->pcdata->learned[sn] = ch->pcdata->dlearned[sn];
                ch->pcdata->dlearned[sn] = 0;
                ch->pcdata->dshowlearned[sn] = ch->pcdata->dshowdlearned[sn];
            }

            /*
             * Toss backup into dlearned 
             */
            if ( backup[sn] > 0 ) {
                ch->pcdata->dlearned[sn] = backup[sn];
                backup[sn] = 0;
                ch->pcdata->dshowdlearned[sn] = dshowbackup[sn];
            }
        }

        // Lastly reinstate human form % ..
        ability = skill_lookup( "animal form" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "common" );

        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];

        while ( ch->first_affect )
            affect_remove( ch, ch->first_affect );
        ch->pcdata->tmparmor = ch->armor;
        ch->pcdata->tmpmax_hit = ch->max_hit;
        ch->pcdata->tmpmax_mana = ch->max_mana;
        ch->pcdata->tmpmax_move = ch->max_move;

        ch->pcdata->tmpheight = ch->height;
        ch->pcdata->tmpweight = ch->weight;

        // Change this based on type of Morph
        if ( ch->pcdata->choice == 1 ) {
            ch->max_hit += ( ch->level * 2 + get_curr_con( ch ) );
            ch->hit = ch->max_hit;
            ch->armor -= ch->level;
            ch->height = 60;
            ch->weight = 200;
        }
        else if ( ch->pcdata->choice == 2 ) {
            ch->max_mana += ( ch->level * 2 + get_curr_int( ch ) );
            ch->mana = ch->max_mana;
            ch->armor -= ch->level;
            ch->height = 36;
            ch->weight = 40;
        }
        else if ( ch->pcdata->choice == 3 ) {
            ch->max_move += ( ch->level * 2 + get_curr_str( ch ) );
            ch->move = ch->max_move;
            ch->armor -= ch->level;
            ch->height = 12;
            ch->weight = 20;
        }
        else if ( ch->pcdata->choice == 4 ) {
            ch->max_hit += ( ch->level * 5 + get_curr_con( ch ) * 2 );
            ch->hit = ch->max_hit;
            ch->height = 90;
            ch->weight = 400;
            ch->armor -= ch->level;
        }
        else if ( ch->pcdata->choice == 5 ) {
            ch->max_hit += ( ch->level * 8 + get_curr_con( ch ) * 2 );
            ch->hit = ch->max_hit;
            ch->height = 80;
            ch->weight = 300;
            ch->armor -= ch->level;
        }
        else if ( ch->pcdata->choice == 6 ) {
            ch->max_hit += ( ch->level * 10 + get_curr_con( ch ) * 2 );
            ch->hit = ch->max_hit;
            ch->height = 144;
            ch->weight = 600;
            ch->armor -= ch->level;
        }

        if ( !IS_NPC( ch ) )
            update_aris( ch );

        save_char_obj( ch );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/humanform.wav)\r\n", ch );

        WAIT_STATE( ch, skill_table[gsn_animal_form]->beats );
        act( AT_MAGIC, "You call upon ancient magic and take on a animal form.", ch, NULL, NULL,
             TO_CHAR );
        act( AT_MAGIC, "$n suddenly shimmers and assumes a animal form.", ch, NULL, NULL, TO_ROOM );
        act( AT_CYAN,
             "You feel elation as your animal senses come in fully!", ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_animal_form );
        send_to_char( "\r\nNow in your new form, you once again don your gear.\r\n", ch );
        interpret( ch, ( char * ) "wear all" );
        save_char_obj( ch );
        return;
    }
    else {                                             /* going to normal */

        send_to_char( "\r\nYou remove all your gear as your shape is about to vastly change.\r\n",
                      ch );
        remove_all_equipment( ch );

        send_to_char( "You release the ancient magic, and change into your normal form.\r\n", ch );

        act( AT_MAGIC, "$n suddenly shimmers and assumes their normal form!", ch, NULL,
             NULL, TO_ROOM );
        act( AT_CYAN, "You feel disoriented as you get used to your normal form!", ch,
             NULL, NULL, TO_CHAR );
        ch->pcdata->choice = 0;
        while ( ch->first_affect )
            affect_remove( ch, ch->first_affect );
        do_unmorph_char( ch );
        ch->armor = ch->pcdata->tmparmor;
        ch->max_hit = ch->pcdata->tmpmax_hit;
        ch->hit = ch->max_hit;
        ch->max_mana = ch->pcdata->tmpmax_mana;
        ch->mana = ch->max_mana;
        ch->max_move = ch->pcdata->tmpmax_move;
        ch->move = ch->max_move;
        ch->height = ch->pcdata->tmpheight;
        ch->weight = ch->pcdata->tmpweight;
        ch->race = ch->pcdata->tmprace;
        ch->Class = ch->pcdata->tmpclass;
        if ( ch->pcdata->tmpsecond != -1 ) {
            ch->secondclass = ch->pcdata->tmpsecond;
            ch->pcdata->tmpsecond = -1;
        }
        if ( ch->pcdata->tmpthird != -1 ) {
            ch->thirdclass = ch->pcdata->tmpthird;
            ch->pcdata->tmpthird = -1;
        }

        if ( ch->race != RACE_ANIMAL && ch->Class != CLASS_DRUID ) {
            ch->pcdata->tmpmax_hit = 0;
            ch->pcdata->tmpmax_mana = 0;
            ch->pcdata->tmpmax_move = 0;
            ch->pcdata->tmpheight = 0;
            ch->pcdata->tmpweight = 0;
            ch->pcdata->tmprace = 0;
            ch->pcdata->tmpclass = 0;
            ch->pcdata->tmparmor = 0;
        }
        if ( ch->pcdata->tmplevel > 100 ) {
            ch->pcdata->tmpmax_hit = 30000;
            ch->pcdata->tmpmax_mana = 30000;
            ch->pcdata->tmpmax_move = 30000;
        }

        /*
         * Ok this is a simple way of handling it, we toss through the sns and set
         * everything up as we go 
         */
        for ( sn = 0; sn < MAX_SKILL; sn++ ) {
            /*
             * Toss into backup 
             */
            if ( ch->pcdata->learned[sn] > 0 ) {
                backup[sn] = ch->pcdata->learned[sn];
                dshowbackup[sn] = ch->pcdata->dshowlearned[sn];
            }

            /*
             * Toss on human skills 
             */
            if ( ch->pcdata->dlearned[sn] > 0 ) {
                ch->pcdata->learned[sn] = ch->pcdata->dlearned[sn];
                ch->pcdata->dlearned[sn] = 0;
                ch->pcdata->dshowlearned[sn] = ch->pcdata->dshowdlearned[sn];
                ch->pcdata->dshowdlearned[sn] = FALSE;
            }

            /*
             * Toss backup into dlearned 
             */
            if ( backup[sn] > 0 ) {
                ch->pcdata->dlearned[sn] = backup[sn];
                backup[sn] = 0;
                ch->pcdata->dshowdlearned[sn] = dshowbackup[sn];
            }
        }

        /*
         * Lastly reinstate human form % .. 
         */
        ability = skill_lookup( "animal form" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "common" );

        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];

        if ( !IS_NPC( ch ) )
            update_aris( ch );
        save_char_obj( ch );
    }
}

void do_animal_form( CHAR_DATA *ch, char *argument )
{
    short                   nomore = 100;
    AFFECT_DATA             af;

    // Temp fix for players switching from animal to human form to remove curse and
    // poison. -Taon
    if ( IS_AFFECTED( ch, AFF_CURSE ) || IS_AFFECTED( ch, AFF_POISON ) ) {
        send_to_char( "You can't change form when cursed or affected by poison.", ch );
        return;
    }

    if ( IS_NPC( ch ) )
        return;

    if ( ch->race == RACE_ANIMAL ) {
        affect_strip( ch, gsn_animal_form );
        xREMOVE_BIT( ch->affected_by, AFF_ANIMALFORM );
        animal_change( ch, FALSE );
        return;
    }

    if ( !VLD_STR( argument ) ) {
        send_to_char( "Syntax: animal type\r\n", ch );
        if ( ch->level < 20 )
            send_to_char( "Available types: wolf\r\n", ch );
        if ( ch->level < 30 && ch->level > 19 )
            send_to_char( "Available types: wolf owl\r\n", ch );
        if ( ch->level < 40 && ch->level > 29 )
            send_to_char( "Available types: wolf owl fish\r\n", ch );
        if ( ch->level < 60 && ch->level > 39 )
            send_to_char( "Available types: wolf owl fish bear\r\n", ch );
        if ( ch->level < 80 && ch->level > 59 )
            send_to_char( "Available types: wolf owl fish bear gryphon\r\n", ch );
        if ( ch->level > 79 )
            send_to_char( "Available types: wolf owl fish bear gryphon treant\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "wolf" ) && ch->level > 9 ) {
        ch->pcdata->choice = 1;
    }
    if ( !str_cmp( argument, "wolf" ) && ch->level < 10 ) {
        send_to_char( "You are not high enough level to do that.\r\n", ch );
        return;
    }
    if ( !str_cmp( argument, "owl" ) && ch->level > 19 ) {
        ch->pcdata->choice = 2;
        // set it to wolf
    }
    if ( !str_cmp( argument, "owl" ) && ch->level < 20 ) {
        send_to_char( "You are not high enough level to do that.\r\n", ch );
        return;
    }
    if ( !str_cmp( argument, "fish" ) && ch->level > 29 ) {
        ch->pcdata->choice = 3;
        // set it to wolf
    }
    if ( !str_cmp( argument, "fish" ) && ch->level < 30 ) {
        send_to_char( "You are not high enough level to do that.\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "bear" ) && ch->level > 39 ) {
        ch->pcdata->choice = 4;
        // set it to wolf
    }
    if ( !str_cmp( argument, "bear" ) && ch->level < 40 ) {
        send_to_char( "You are not high enough level to do that.\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "gryphon" ) && ch->level > 59 ) {
        ch->pcdata->choice = 5;
        // set it to wolf
    }
    if ( !str_cmp( argument, "gryphon" ) && ch->level < 60 ) {
        send_to_char( "You are not high enough level to do that.\r\n", ch );
        return;
    }

    if ( !str_cmp( argument, "treant" ) && ch->level > 79 ) {
        ch->pcdata->choice = 6;
        // set it to wolf
    }
    if ( !str_cmp( argument, "treant" ) && ch->level < 80 ) {
        send_to_char( "You are not high enough level to do that.\r\n", ch );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "You do not have enough mana to do that.\r\n", ch );
        return;
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_animal_form ) ) {
        ch->mana = ( ch->mana - nomore );
        animal_change( ch, TRUE );
        return;
    }
    else {
        learn_from_failure( ch, gsn_animal_form );
        send_to_char( "&cYou fail to summon the ancient magic.\r\n", ch );
        return;
    }
}

void do_stalk_prey( CHAR_DATA *ch, char *argument )
{
    MOB_INDEX_DATA         *temp;
    CHAR_DATA              *mob;
    CHAR_DATA              *prey;
    bool                    found;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( mob ) {
            if ( IS_NPC( mob ) && mob->in_room && ch->in_room ) {
                if ( mob->master ) {
                    if ( !str_cmp( ch->name, mob->master->name )
                         && ( mob->in_room == ch->in_room ) ) {
                        found = TRUE;
                        break;
                    }
                }
            }
        }
    }

    if ( found == FALSE ) {
        send_to_char( "Your pet is not here, to stalk any prey.\r\n", ch );
        return;
    }

    if ( !IS_OUTSIDE( ch ) || ch->in_room->sector_type == SECT_INSIDE
         || ch->in_room->sector_type == SECT_ROAD ) {
        act( AT_CYAN, "$n tries to find suitable prey to stalk, but nothing is near.", mob, NULL,
             NULL, TO_ROOM );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_stalk_prey ) ) {
        WAIT_STATE( ch, skill_table[gsn_stalk_prey]->beats );
        learn_from_success( ch, gsn_stalk_prey );
        int                     vnum;

        vnum = number_range( 1, 4 );
        if ( vnum == 1 )
            vnum = 4410;
        else if ( vnum == 2 )
            vnum = 1510;
        else if ( vnum == 3 )
            vnum = 16710;
        else if ( vnum == 4 )
            vnum = 13009;

        temp = get_mob_index( vnum );

        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/create.wav)\r\n", ch );
        prey = create_mobile( temp );
        prey->level = 1;
        char_to_room( prey, ch->in_room );
        act( AT_CYAN, "$n successfully stalks down $N and slay's it!", mob, NULL, prey, TO_ROOM );
        raw_kill( ch, prey );
        return;
    }
    else {
        act( AT_CYAN, "$n tries to stalk a small animal, but fails to get near.", mob, NULL, NULL,
             TO_ROOM );
        learn_from_failure( ch, gsn_stalk_prey );
        return;
    }

}

void do_infectious_claws( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *mob;
    bool                    found;

    found = FALSE;

    for ( mob = first_char; mob; mob = mob->next ) {
        if ( mob ) {
            if ( IS_NPC( mob ) && mob->in_room && ch->in_room ) {
                if ( mob->master ) {
                    if ( !str_cmp( ch->name, mob->master->name )
                         && ( mob->in_room == ch->in_room ) ) {
                        found = TRUE;
                        break;
                    }
                }
            }
        }
    }
    if ( found == FALSE ) {
        send_to_char( "Your pet is not here.\r\n", ch );
        return;
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_infectious_claws ) ) {
        WAIT_STATE( ch, skill_table[gsn_infectious_claws]->beats );
        learn_from_success( ch, gsn_infectious_claws );
        affect_strip( mob, gsn_infectious_claws );

        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/create.wav)\r\n", ch );
        act( AT_CYAN, "$n opens a poison gland to $s claws.", mob, NULL, NULL, TO_ROOM );
        xSET_BIT( mob->attacks, ATCK_POISON );
        af.type = gsn_infectious_claws;
        af.duration = ch->level + 50;
        af.location = APPLY_NONE;
        af.level = ch->level;
        af.bitvector = meb( AFF_INFECTIOUS_CLAWS );
        af.modifier = 0;
        affect_to_char( mob, &af );
        return;
    }
    else {
        act( AT_CYAN, "$n attempts to open a poison gland to $s claws, but fails.", mob, NULL, NULL,
             TO_ROOM );
        learn_from_failure( ch, gsn_infectious_claws );
        return;
    }

}

void do_sharpen_claws( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *mob;
    bool                    found;

    found = FALSE;

    for ( mob = first_char; mob; mob = mob->next ) {
        if ( mob ) {
            if ( IS_NPC( mob ) && mob->in_room && ch->in_room ) {
                if ( mob->master ) {
                    if ( !str_cmp( ch->name, mob->master->name )
                         && ( mob->in_room == ch->in_room ) ) {
                        found = TRUE;
                        break;
                    }
                }
            }
        }
    }
    if ( found == FALSE ) {
        send_to_char( "Your pet is not here, to sharpen it's claws.\r\n", ch );
        return;
    }

    if ( !IS_OUTSIDE( ch ) || ch->in_room->sector_type == SECT_INSIDE
         || ch->in_room->sector_type == SECT_ROAD ) {
        act( AT_CYAN,
             "$n tries to find something suitable to sharpen it's claws on, but nothing is near.",
             mob, NULL, NULL, TO_ROOM );
        return;
    }

    if ( mob->fighting )
    {
    return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_sharpen_claws ) ) {
        WAIT_STATE( ch, skill_table[gsn_sharpen_claws]->beats );
        learn_from_success( ch, gsn_sharpen_claws );
        affect_strip( mob, gsn_sharpen_claws );

        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/create.wav)\r\n", ch );
        act( AT_CYAN, "$n finds a small tree and begins to sharpen it's claws.", mob, NULL, NULL,
             TO_ROOM );
        af.type = gsn_sharpen_claws;
        af.duration = ch->level + 50;
        af.location = APPLY_DAMROLL;
        af.level = ch->level;
        xCLEAR_BITS( af.bitvector );
        af.modifier = 5;
        affect_to_char( mob, &af );
        return;
    }
    else {
        act( AT_CYAN, "$n tries to find suitable place to sharpen it's claws, but nothing is near.",
             mob, NULL, NULL, TO_ROOM );
        learn_from_failure( ch, gsn_sharpen_claws );
        return;
    }

}

void do_net( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim;
    char                    arg[MIL];
    OBJ_DATA               *obj;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Try to throw a net on who?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_SLOW ))
    {
        send_to_char("They are already slowed down by a net.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_net ) ) {
        WAIT_STATE( ch, skill_table[gsn_net]->beats );
        learn_from_success( ch, gsn_net );

        af.bitvector = meb( AFF_SLOW );
        af.type = gsn_net;
        af.duration = ( int ) ( ( ch->level - get_curr_int( victim ) ) * 2 );
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        affect_to_char( victim, &af );
       act( AT_CYAN, "You manage to throw a net around $N, slowing them down.", ch, NULL,
             victim, TO_CHAR );
       act( AT_CYAN, "$n throws a net around $N, slowing down $S movements.", ch, NULL,
             victim, TO_ROOM );
        global_retcode = damage( ch, victim, 1, gsn_net );
        obj = create_object( get_obj_index( 41028 ), 0 );
        obj_to_room( obj, victim->in_room );
        make_scraps( obj );
    }
    else {
       act( AT_CYAN, "You try to throw a net around $N, but the dodge to the side.", ch, NULL,
             victim, TO_CHAR );
        learn_from_failure( ch, gsn_net );
        return;
    }

}

void do_find_water( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *mob;
    bool                    found;
    OBJ_DATA               *water;

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( mob ) {
            if ( IS_NPC( mob ) && mob->in_room && ch->in_room ) {
                if ( mob->master ) {
                    if ( !str_cmp( ch->name, mob->master->name )
                         && ( mob->in_room == ch->in_room ) ) {
                        found = TRUE;
                        break;
                    }
                }
            }
        }
    }
    if ( found == FALSE ) {
        send_to_char( "Your pet is not here, to find any water.\r\n", ch );
        return;
    }

    if ( !IS_OUTSIDE( ch ) || ch->in_room->sector_type == SECT_INSIDE
         || ch->in_room->sector_type == SECT_ROAD ) {
        act( AT_CYAN, "$n tries to find suitable drinking water, but nothing is near.", mob, NULL,
             NULL, TO_ROOM );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_find_water ) ) {
        WAIT_STATE( ch, skill_table[gsn_find_water]->beats );
        learn_from_success( ch, gsn_find_water );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/create.wav)\r\n", ch );
        water = create_object( get_obj_index( 41027 ), 0 );

        act( AT_CYAN, "$n is able to locate $p.", mob, water, NULL, TO_ROOM );
        obj_to_room( water, ch->in_room );
        return;
    }
    else {
        act( AT_CYAN, "$n tries to find suitable drinking water, but nothing is near.", mob, NULL,
             NULL, TO_ROOM );
        learn_from_failure( ch, gsn_find_water );
        return;
    }
}

void do_beast_meld( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *mob;
    bool                    found;
    AFFECT_DATA             af;

    set_char_color( AT_IMMORT, ch );

    if ( !ch->desc )
        return;

    if ( ch->desc->original ) {
        ch_printf( ch, "You release your beast meld.\r\n" );
        interpret( ch->master, ( char * ) "stand" );
        interpret( ch->master, ( char * ) "visible" );
        ch->desc->character = ch->desc->original;
        ch->desc->original = NULL;
        ch->desc->character->desc = ch->desc;
        ch->desc->character->switched = NULL;
        ch->desc = NULL;
       	xREMOVE_BIT( ch->act, ACT_BEASTMELD );
        REMOVE_BIT( ch->master->pcdata->flags, PCFLAG_PUPPET );
        return;
    }

    found = FALSE;
    for ( mob = first_char; mob; mob = mob->next ) {
        if ( IS_NPC( mob ) && mob->in_room && ch == mob->master && ch->in_room ) {
            found = TRUE;
            break;
        }
    }
    if ( found == FALSE ) {
        send_to_char( "Your pet is not in the same room to meld with.\r\n", ch );
        return;
    }

    if ( mob->desc ) {
        send_to_char( "Character pet meld in use.\r\n", ch );
        return;
    }

    if ( ch->switched ) {
        send_to_char( "You can't switch into a player that is beast melded!\r\n", ch );
        return;
    }
    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_FREEZE ) ) {
        send_to_char( "You shouldn't switch into a player that is frozen!\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_beast_meld ) ) {
        WAIT_STATE( ch, skill_table[gsn_beast_meld]->beats );
        learn_from_success( ch, gsn_beast_meld );
        af.type = gsn_hide;
        af.duration = ch->level + 10;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_HIDE );
        af.level = ch->level;
        affect_to_char( ch, &af );
        ch->desc->character = mob;
        ch->desc->original = ch;
        mob->desc = ch->desc;
        ch->desc = NULL;
        ch->switched = mob;
        send_to_char( "You begin to beast meld.\r\n", mob );
        xSET_BIT( mob->act, ACT_BEASTMELD );
        act( AT_CYAN,
             "$n lays down and $s eyes go blank while $s melds into the surroundings.\r\n",
             ch, NULL, NULL, TO_ROOM );
        interpret( ch, ( char * ) "rest" );
        SET_BIT( ch->pcdata->flags, PCFLAG_PUPPET );
        return;
    }
    else {
        act( AT_CYAN, "\r\nYou begin to beast meld, but loose your concentration.\r\n", ch, NULL,
             NULL, TO_CHAR );
        learn_from_failure( ch, gsn_beast_meld );
        return;
    }
}
