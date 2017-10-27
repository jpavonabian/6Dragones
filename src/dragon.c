/* Copyright 2014, 2015 Jesús Pavón Abián
Programa bajo licencia GPL. Encontrará una copia de la misma en el archivo COPYING.txt que se encuentra en la raíz de los fuentes. */
/* Figured it would be nice to put all dragon skills in one file, easier for me to balance the power anyways.
   - Vladaar
:::::::-.  :::::::..    :::.      .,-:::::/      ...   :::.    :::. .::::::. 
 ;;,   `';,;;;;``;;;;   ;;`;;   ,;;-'````'    .;;;;;;;.`;;;;,  `;;;;;;`    ` 
 `[[     [[ [[[,/[[['  ,[[ '[[, [[[   [[[[[[/,[[     \[[,[[[[[. '[['[==/[[[[,
  $$,    $$ $$$$$$c   c$$$cc$$$c"$$c.    "$$ $$$,     $$$$$$ "Y$c$$  '''    $
  888_,o8P' 888b "88bo,888   888,`Y8bo,,,o88o"888,_ _,88P888    Y88 88b    dP
  MMMMP"`   MMMM   "W" YMM   ""`   `'YMUP"YMM  "YMMMMMP" MMM     YM  "YMmMY" 
*/

#include <ctype.h>
#include <string.h>
#include "h/mud.h"
#include "h/languages.h"
#include "h/new_auth.h"
#include "h/files.h"
#include "h/damage.h"

//For some of the newer dragon spells. -Taon
extern void             failed_casting( SKILLTYPE * skill, CHAR_DATA *ch, CHAR_DATA *victim,
                                        OBJ_DATA *obj );

bool can_fly( CHAR_DATA *ch )
{
    set_char_color( AT_GREY, ch );

    switch ( ch->position ) {
        default:
            break;

        case POS_SLEEPING:
        case POS_RESTING:
        case POS_SITTING:
        case POS_MEDITATING:
            send_to_char( "¡Levántate primero para poder volar!\r\n", ch );
            return FALSE;
    }

    if ( ch->position < POS_FIGHTING ) {
        send_to_char( "No puedes concentrarte en eso.\r\n", ch );
        return FALSE;
    }

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "No puedes concentrarte en eso.\r\n", ch );
        return FALSE;
    }

    if ( IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "Ya estás volando. Aterriza primero y luego vuelve a intentarlo.\r\n", ch );
        return 0;
    }

    if ( ch->mount ) {
        send_to_char( "No puedes hacer eso estando sobre una montura.\r\n", ch );
        return 0;
    }

    return TRUE;
}

void do_wings( CHAR_DATA *ch, char *argument )
{
    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual no te permite usar esta habilidad.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/wings.wav)\r\n", ch );
    set_char_color( AT_GREY, ch );

    if ( ch->race != RACE_DRAGON && ch->race != RACE_CELESTIAL && ch->race != RACE_DEMON
         && ch->race != RACE_VAMPIRE && ch->Class != CLASS_OWL && ch->Class != CLASS_GRYPHON ) {
        if ( ch->race == RACE_PIXIE ) {
            do_fly( ch, ( char * ) "" );
            return;
        }
        error( ch );
        return;
    }

    if ( !can_fly( ch ) )
        return;

    if ( ch->move < 25 ) {
        send_to_char( "El cansancio te lo impide.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_wings]->beats );
    if ( ch->race == RACE_VAMPIRE ) {
        ch->blood -= 1;
    }
    else
        ch->move -= 25;
    if ( !IS_NPC( ch ) ) {
        if ( ch->race == RACE_VAMPIRE && ch->pcdata->hp_balance != 5 ) {
            act( AT_CYAN,
                 "¡Te brotan alas en la espalda!", ch,
                 NULL, NULL, TO_CHAR );
            act( AT_CYAN, "¡A $n le brotan alas en la espalda!",
                 ch, NULL, NULL, TO_ROOM );
            ch->pcdata->hp_balance = 5;
            global_retcode = damage( ch, ch, 1, gsn_wings );
        }
    }
    if ( can_use_skill( ch, number_percent(  ), gsn_wings ) ) {
        AFFECT_DATA             af;

        af.type = gsn_wings;
        af.duration = ch->level * 2 + 40;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_FLYING );
        affect_to_char( ch, &af );
        act( AT_GREY, "Despliegas cuidadosamente las alas y alzas el vuelo.", ch, NULL,
             NULL, TO_CHAR );
        act( AT_GREY, "$n despliega sus alas y se eleva en el aire.", ch, NULL, NULL,
             TO_ROOM );
        learn_from_success( ch, gsn_wings );
    }
    else {
        act( AT_GREY,
             "Despliegas las alas pero no consigues elevarte del suelo.", ch,
             NULL, NULL, TO_CHAR );
        act( AT_GREY, "$n despliega las alas pero no logra elevarse en el aire.", ch, NULL,
             NULL, TO_ROOM );
        learn_from_failure( ch, gsn_wings );
    }
    return;
}

//Skill is complete.  8-5-08  -Taon

void do_submerged( CHAR_DATA *ch, char *argument )
{

    int                     move_loss;
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_AQUA_BREATH ) ) {
        send_to_char( "¿Para qué? Tus pulmones ya pueden respirar bajo el agua.\r\n", ch );
        return;
    }

    move_loss = 50 - get_curr_dex( ch );

    if ( ch->move < move_loss ) {
        send_to_char( "No tienes fuerzas suficientes.\r\n", ch );
        return;
    }
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/submerged.wav)\r\n", ch );

    if ( can_use_skill( ch, number_percent(  ), gsn_submerged ) ) {
        send_to_char( "Te salen branquias que te permiten respirar bajo agua.\r\n", ch );
        af.type = gsn_submerged;
        af.duration = ch->level * 10;
        af.level = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_AQUA_BREATH );
        affect_to_char( ch, &af );
        ch->move -= move_loss;
        learn_from_success( ch, gsn_submerged );
    }
    else
        send_to_char( "No consigues respirar bajo el agua.\r\n", ch );
    learn_from_failure( ch, gsn_submerged );
    return;
}

//Skill for black dragons, wrote as an autoskill by Taon, rewrote as
//a skill by Taon.

void do_dominate( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    short                   lvl_diff,
                            chance;
    char                    arg[MIL];

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual te impide usar esta habilidad.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "¿Dominar a quién?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "Debes estar luchando para dominar a alguien.\r\n", ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }

    if ( ch->move < 25 ) {
        send_to_char( "El cansancio te impide realizar esta tarea.\r\n", ch );
        return;
    }

    chance = number_range( 1, 100 );
    lvl_diff = ch->level - victim->level;

    if ( lvl_diff < 0 )
        chance -= 20;
    else
        chance += lvl_diff;

    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        chance -= 5;

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) || IS_IMMORTAL( victim ) )
        chance = 0;

    if ( can_use_skill( ch, number_percent(  ), gsn_ballistic ) && chance > 75 ) {
        WAIT_STATE( ch, skill_table[gsn_ballistic]->beats );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/dominate.wav)\r\n", ch );

        if ( !IS_NPC( victim ) ) {
            ch_printf( ch, "Miras fijamente a los ojos de %s, dominando su mente.\r\n",
                       victim->name );
            ch_printf( victim, "%s te mira fijamente a los ojos, dominando tu mente.\r\n",
                       ch->name );
        }
        else
            ch_printf( ch, "Miras a los ojos a %s, dominando su mente.",
                       capitalize( victim->short_descr ) );

        ch->move -= 25;
        do_flee( victim, ( char * ) "" );
        learn_from_success( ch, gsn_dominate );
    }
    else
        send_to_char( "Eres incapaz.\r\n", ch );

    if ( ch->pcdata->learned[gsn_dominate] > 0 )
        learn_from_failure( ch, gsn_dominate );

    return;
}

//Tangle will be a skill meant for silver dragons to tangle their foes
//with their tails. Preventing them from escape, during the duration of
//the entanglement.   -Taon

void do_entangle( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    AFFECT_DATA             af;
    short                   chance,
                            lvl_diff;
    char                    arg[MIL];

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "tu forma actual te lo impide.\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "Debes estar luchando para eso.\r\n", ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }
    if ( ch == victim ) {
        send_to_char( "¿Por qué?\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( victim, AFF_TANGLED ) ) {
        send_to_char( "Ya está.\r\n", ch );
        return;
    }
    if ( ch->move < 25 ) {
        send_to_char( "El cansancio te impide realizar esta tarea.\r\n", ch );
        return;
    }

    chance = number_range( 1, 100 );
    lvl_diff = ch->level - victim->level;

    if ( lvl_diff < 0 )
        chance -= 20;
    else
        chance += lvl_diff;

    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        chance -= 5;

    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) || IS_IMMORTAL( victim ) )
        chance = 0;

    if ( can_use_skill( ch, number_percent(  ), gsn_entangle ) && chance > 75 ) {
        WAIT_STATE( ch, skill_table[gsn_entangle]->beats );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/entangle.wav)\r\n", ch );

        if ( !IS_NPC( victim ) ) {
            ch_printf( ch, "Enredas las piernas de %s con tu cola.\r\n", victim->name );
            ch_printf( victim, "%s te enreda la cola en las piernas.\r\n", ch->name );
        }
        else
            ch_printf( ch, "Enredas las piernas de %s con tu cola.\r\n",
                       capitalize( victim->short_descr ) );

        af.type = gsn_entangle;
        af.duration = ch->level / 2;
        af.level = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = meb( AFF_TANGLED );
        affect_to_char( victim, &af );
        ch->move -= 25;
        learn_from_success( ch, gsn_entangle );
    }
    else
        send_to_char( "No eres capaz.\r\n", ch );

    if ( ch->pcdata->learned[gsn_entangle] > 0 )
        learn_from_failure( ch, gsn_entangle );

    return;
}

//This skill isnt complete, just getting a good foothold on it. -Taon
void do_ballistic( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    char                    arg[MIL];
    short                   miss,
                            xFactor;
    bool                    exist = FALSE;

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual te impide usar esta habilidad.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "¿A quién quieres golpear con tu ataque balístico?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        if ( !ch->fighting ) {
            send_to_char( "Debes estar luchando para hacer eso.\r\n",
                          ch );
            return;
        }
        else
            victim = who_fighting( ch );
    }
    if ( ch->mana < 40 ) {
        send_to_char( "No tienes suficiente mana.\r\n", ch );
        return;
    }
    if ( ch == victim ) {
        send_to_char( "¿por qué?\r\n", ch );
        return;
    }
/* Why a silver dragon that breaths steel, titanium would need an object?  - Vladaar
 for(obj = ch->first_carrying; obj; obj = obj->next_content)
 {
   if(!obj)
    break;

   if(obj->item_type == ITEM_STONE)
   {
    exist = TRUE;
    break;
   }  
 }

 if(!exist)
 {
  send_to_char("No llevas ninguna piedra.\r\n", ch);
  return;
 }
*/

    if ( can_use_skill( ch, number_percent(  ), gsn_ballistic ) ) {
        WAIT_STATE( ch, skill_table[gsn_ballistic]->beats );

        miss = ( get_curr_dex( victim ) * 2 ) + number_chance( 25, 70 );
        xFactor = ch->pcdata->learned[gsn_ballistic];
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/ballistic.wav)\r\n", ch );

        if ( miss > 100 ) {
            act( AT_MAGIC,
                 "Lanzas con tu magia una piedra a $N, pero solo consigues golpear el suelo.", ch,
                 NULL, victim, TO_CHAR );
            act( AT_MAGIC, "te apartas del camino de la piedra que te ha lanzado $n mágicamente.", ch,
                 NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n lanza una piedra mágicamente a $N, pero falla.", ch, NULL, victim,
                 TO_NOTVICT );
            global_retcode = damage( ch, victim, 0, gsn_ballistic );
        }
        else if ( xFactor <= 40 ) {
            act( AT_MAGIC, "Lanzas una piedra mágicamente a $N.", ch, NULL, victim, TO_CHAR );
            act( AT_MAGIC, "$n te lanza una piedra mágicamente.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n lanza mágicamente una piedra a $N.", ch, NULL, victim, TO_NOTVICT );
            global_retcode = damage( ch, victim, mediumhigh, gsn_ballistic );
        }
        else if ( xFactor <= 65 ) {
            act( AT_MAGIC, "Mágicamente empujas una piedra hacia $N.", ch, NULL, victim, TO_CHAR );
            act( AT_MAGIC, "$n empuja una piedra mágicamente hacia ti.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n empuja mágicamente una piedra hacia $N.", ch, NULL, victim, TO_NOTVICT );
            global_retcode = damage( ch, victim, high, gsn_ballistic );
        }
        else if ( xFactor <= 85 ) {
            act( AT_MAGIC, "Invocas las energías antiguas para proyectar mágicamente una piedra hacia $N.", ch, NULL,
                 victim, TO_CHAR );
            act( AT_MAGIC, "$n te lanza una piedra mágicamente.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n lanza una piedra mágicamente a $N.", ch, NULL, victim, TO_NOTVICT );
            global_retcode = damage( ch, victim, extrahigh, gsn_ballistic );
        }
        else {
            act( AT_MAGIC, "Lanzas una piedra a $N.", ch, NULL, victim, TO_CHAR );
            act( AT_MAGIC, "$n te lanza una piedra.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n lanza una piedra a $N.", ch, NULL, victim, TO_NOTVICT );
            global_retcode = damage( ch, victim, ludicrous, gsn_ballistic );
        }

/*
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);
*/
        learn_from_success( ch, gsn_ballistic );
        ch->mana -= 40;
    }
    else {
        send_to_char( "No consigues invocar las antiguas energías.\r\n", ch );
        global_retcode = damage( ch, victim, 0, gsn_ballistic );
        learn_from_failure( ch, gsn_ballistic );
        ch->mana -= 20;
        return;
    }
    return;
}

void do_gut( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    int                     chance;
    AFFECT_DATA             af;

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual te impide usar esta habilidad.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "¿A quién?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }
// Fixed so can't target self -- Aurin 12/3/2010
    if ( victim == ch ) {
        send_to_char( "¿por qué?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_gut ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/gut.wav)\r\n", ch );

        WAIT_STATE( ch, skill_table[gsn_gut]->beats );
        act( AT_RED, "Destripas a $N con tus garras.", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "$n te destripa con sus garras!", ch, NULL, victim, TO_VICT );
        learn_from_success( ch, gsn_gut );
        global_retcode = damage( ch, victim, low, gsn_gut );    /* nerfed gut some -
                                                                 * Vladaar */
        chance = number_range( 1, 100 );
        if ( chance < 95 )
            return;

        if ( !char_died( victim ) ) {
            if ( IS_AFFECTED( victim, AFF_POISON ) )
                return;
            act( AT_GREEN, "¡tus garras han envenenado a $N!", ch, NULL, victim, TO_CHAR );
            act( AT_GREEN, "¡$n te ha envenenado con sus garras!", ch, NULL, victim, TO_VICT );
            af.type = gsn_gut;
            af.duration = ch->level;
            af.level = ch->level;
            af.location = APPLY_STR;
            af.modifier = -2;
            af.bitvector = meb( AFF_POISON );
            affect_join( victim, &af );
            set_char_color( AT_GREEN, victim );
            send_to_char( "Te sientes fatal.\r\n", victim );
        }
        else {
            learn_from_failure( ch, gsn_gut );
            WAIT_STATE( ch, 8 );
            global_retcode = damage( ch, victim, 0, gsn_gut );
            send_to_char( "No consigues destriparle.\r\n", ch );
            return;
        }
        return;

    }
    else {
        act( AT_RED, "$N ha conseguido esquivarte.", ch, NULL, victim, TO_CHAR );
    }
}

void do_tears( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   nomore;
    AFFECT_DATA             af;

    nomore = 20;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        victim = ch;
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "no mientras luches.\r\n", ch );
        return;
    }

    if ( victim == who_fighting( ch ) ) {
        send_to_char( "pero si le estás pegando....\r\n", ch );
        return;
    }
    if ( ch->mana <= nomore ) {
        send_to_char( "No tienes poder mágico suficiente para invocar tus lágrimas.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_tears ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/tears.wav)\r\n", ch );

        affect_strip( victim, gsn_tears );
        WAIT_STATE( ch, skill_table[gsn_tears]->beats );
        ch->mana = ( ch->mana - nomore );
        learn_from_success( ch, gsn_tears );
        act( AT_YELLOW, "Concentras tu poder en una bendición.\r\n", ch, NULL, victim,
             TO_CHAR );
        if ( victim == ch ) {
            act( AT_CYAN, "¡Tus lágrimas te bendicen!", ch, NULL, victim,
                 TO_CHAR );
            act( AT_CYAN, "¡Las lágrimas de $n le hacen asumir una apariencia justa!", ch, NULL,
                 victim, TO_NOTVICT );
        }
        else {
            act( AT_CYAN, "¡tus lágrimas caen sobre $N bendiciéndole!", ch,
                 NULL, victim, TO_CHAR );
            act( AT_CYAN, "¡$n deja caer lágrimas sobre tí, bendiciéndote!", ch,
                 NULL, victim, TO_VICT );
            act( AT_CYAN, "$n deja caer algunas lágrimas sobre $N, bendiciéndole!", ch,
                 NULL, victim, TO_NOTVICT );
        }
        af.type = gsn_tears;
        af.duration = ch->level * 2;
        af.location = APPLY_RESISTANT;
        af.modifier = 16;
        xCLEAR_BITS( af.bitvector );
        af.level = ch->level;
        affect_to_char( victim, &af );

        af.type = gsn_tears;
        af.duration = ch->level * 2;
        af.location = APPLY_RESISTANT;
        af.modifier = 32;
        xCLEAR_BITS( af.bitvector );
        af.level = ch->level;
        affect_to_char( victim, &af );

        af.type = gsn_tears;
        af.duration = ch->level * 2;
        af.location = APPLY_RESISTANT;
        af.modifier = 64;
        xCLEAR_BITS( af.bitvector );
        af.level = ch->level;
        affect_to_char( victim, &af );

        return;
    }
    else
        act( AT_CYAN, "Intentas soltar algunas lágrimas, pero te distraes.", ch, NULL, victim,
             TO_CHAR );
    learn_from_failure( ch, gsn_tears );
    return;
}

void do_defend( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;

    nomore = 20;

    if ( ch->mana < nomore ) {
        send_to_char( "No tienes mana suficiente.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_SANCTUARY ) ) {
        send_to_char( "ya tienes un escudo defensivo.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_defend ) ) {
        if ( !IS_AFFECTED( ch, AFF_SANCTUARY ) )
            if ( xIS_SET( ch->act, PLR_BATTLE ) )
                send_to_char( "!!SOUND(sound/defend.wav)\r\n", ch );

        act( AT_MAGIC, "$n pronuncia algunos encantamientos.", ch, NULL, NULL, TO_ROOM );
        spell_sanctuary( skill_lookup( "sanctuary" ), ch->level, ch, ch );
        WAIT_STATE( ch, skill_table[gsn_defend]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else
        send_to_char( "Te concentras para invocar un escudo defensivo, pero algo te distrae.\r\n", ch );
    return;
}

void do_cone( CHAR_DATA *ch, char *argument )
{
    short                   nomore;
    AFFECT_DATA             af;
    short                   chance;

    nomore = 20;
    CHAR_DATA              *victim;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "¿A quién?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "¿por qué?\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        act( AT_DGREY, "¡Alzas tus manos y lanzas un cono de energía oscura!", ch, NULL, victim,
             TO_CHAR );
        act( AT_DGREY, "¡$n lanza un cono de energía oscura que te envuelve!", ch, NULL, victim,
             TO_VICT );
        act( AT_DGREY, "¡$n lanza un cono de energía oscura, envolviendo a $N!", ch, NULL,
             victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "No tienes suficiente poder mágico para eso.\r\n", ch );
        return;
    }

    chance = ( 1, 10 );

    if ( chance < 3 ) {
        send_to_char( "Intentas lanzar un cono de energía pero te distraes.\r\n", ch );
        return;
    }

    short                   twoheaded = 0;

    if ( ch->Class == CLASS_TWOHEADED ) {
        twoheaded = number_range( 1, 5 );
    }
    if ( ch->pcdata->tmpclass == 8 || twoheaded == 1 ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/cone.wav)\r\n", ch );

        act( AT_DGREY, "¡alzas las manos y lanzas un cono de energía oscura!", ch, NULL, victim,
             TO_CHAR );
        act( AT_DGREY, "¡$n lanza un cono de energía oscura que te envuelve!", ch, NULL, victim,
             TO_VICT );
        act( AT_DGREY, "¡$n lanza un cono de energía oscura que envuelve a $N!", ch, NULL,
             victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else if ( ch->pcdata->tmpclass == 9 || twoheaded == 2 ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/cone.wav)\r\n", ch );

        act( AT_YELLOW, "¡alzas las manos y lanzas un cono de energía divina!", ch, NULL, victim,
             TO_CHAR );
        act( AT_YELLOW, "¡$n lanza un cono de energía divina que te envuelve!", ch, NULL, victim,
             TO_VICT );
        act( AT_YELLOW, "¡$n lanza un cono de energía divina, envolviendo a $N!", ch, NULL,
             victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else if ( ch->pcdata->tmpclass == 10 || twoheaded == 3 ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/cone.wav)\r\n", ch );

        act( AT_GREY, "¡alzas las manos y lanzas un cono de fragmentos de hierro!", ch, NULL, victim,
             TO_CHAR );
        act( AT_GREY, "¡$n lanza un cono de fragmentos de hierro que te golpea!", ch, NULL, victim,
             TO_VICT );
        act( AT_GREY, "¡$n lanza un cono de fragmentos de hierro que golpea a $N!", ch, NULL, victim,
             TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else if ( ch->pcdata->tmpclass == 11 || twoheaded == 4 ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/cone.wav)\r\n", ch );

        act( AT_RED, "¡alzas tus manos y lanzas un cono de humo!", ch, NULL, victim,
             TO_CHAR );
        act( AT_RED, "¡$n lanza un cono de humo que te afixia!", ch, NULL, victim, TO_VICT );
        act( AT_RED, "¡$n lanza un cono de humo, afixiando a $N!", ch, NULL, victim, TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else if ( ch->pcdata->tmpclass == 12 || twoheaded == 5 ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/cone.wav)\r\n", ch );

        act( AT_LBLUE, "¡alzas tus manos y lanzas un cono de agua!", ch, NULL, victim, TO_CHAR );
        act( AT_LBLUE, "¡$n lanza un cono de agua que te envuelve!", ch, NULL,
             victim, TO_VICT );
        act( AT_LBLUE, "¡$n lanza un cono de agua envolviendo a $N!", ch, NULL, victim,
             TO_NOTVICT );
        global_retcode = damage( ch, victim, insane, gsn_cone );
        WAIT_STATE( ch, skill_table[gsn_cone]->beats );
        ch->mana = ( ch->mana - nomore );
        return;
    }
    else
        log_string( "Bug: Dragon Lord player with incorrect tmpclass #" );
    return;
}

/* Strip all their gear for changing form */
void remove_all_equipment( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;
    int                     x,
                            y;

    if ( !ch )
        return;

    for ( obj = ch->first_carrying; obj; obj = obj->next_content )
        if ( obj->wear_loc > -1 )
            unequip_char( ch, obj );
}

void humanform_change( CHAR_DATA *ch, bool tohuman )
{                                                      /* tohuman=TRUE if going to human,
                                                        * * =FALSE if going to dragon */
    short                   backup[MAX_SKILL];
    bool                    dshowbackup[MAX_SKILL];
    int                     sn = 0;
    short                   ability = 0;
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) ) {
        send_to_char( "¡Mobs no!", ch );
        return;
    }

    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "¡Despierta primero!", ch );
        return;
    }

    for ( sn = 0; sn < MAX_SKILL; sn++ ) {
        dshowbackup[sn] = FALSE;
        backup[sn] = 0;
    }

    if ( tohuman ) {
        send_to_char( "\r\nDesvistes tu equipo sabiendo que tu tamaño cambiará.\r\n",
                      ch );
        remove_all_equipment( ch );

        ch->pcdata->tmprace = ch->race;
        ch->pcdata->tmpclass = ch->Class;
        ch->race = 0;
        ch->Class = CLASS_DRAGONLORD;

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
        ability = skill_lookup( "human form" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "common" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "draconic" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];

        while ( ch->first_affect )
            affect_remove( ch, ch->first_affect );

        ch->pcdata->tmpmax_hit = ch->max_hit;
        ch->pcdata->tmpheight = ch->height;
        ch->pcdata->tmpweight = ch->weight;

        /*
         * Lets give them the same percent of their higher hp 
         */
        {
            double                  hitprcnt = 0.0;

            if ( ch->hit > 0 && ch->max_hit > 0 )
                hitprcnt = ( ch->hit / ch->max_hit );
            ch->max_hit -= ch->max_hit / 6;
            if ( hitprcnt > 0.0 )
                ch->hit = ( int ) ( ch->max_hit * hitprcnt );
        }

        ch->height = 72;
        ch->weight = 200;

        if ( !IS_NPC( ch ) )
            update_aris( ch );

        save_char_obj( ch );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/humanform.wav)\r\n", ch );

        WAIT_STATE( ch, skill_table[gsn_human_form]->beats );
        act( AT_MAGIC, "Invocas las antiguas energías y adoptas forma humana.", ch, NULL, NULL,
             TO_CHAR );
        act( AT_MAGIC, "$n adopta forma humana.", ch, NULL, NULL, TO_ROOM );
        act( AT_CYAN,
             "¡Te desorientas al cambiar tan bruscamente de cuerpo!",
             ch, NULL, NULL, TO_CHAR );
        learn_from_success( ch, gsn_human_form );
        af.type = gsn_human_form;
        af.duration = -1;
        af.level = ch->level;
        af.location = APPLY_AFFECT;
        af.modifier = 0;
        af.bitvector = meb( AFF_DRAGONLORD );
        affect_to_char( ch, &af );
        send_to_char( "\r\nEn tu nueva forma puedes reequiparte.\r\n", ch );
        interpret( ch, ( char * ) "vestir todo" );
        save_char_obj( ch );
    }
    else {                                             /* going to dragon */

        send_to_char( "\r\nDesvistes tu equipo sabiendo que tu cuerpo cambiará.\r\n",
                      ch );
        remove_all_equipment( ch );

        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/humanform.wav)\r\n", ch );

        send_to_char( "Invocas las antiguas energías y retornas a tu forma original de dragón.\r\n", ch );

        act( AT_MAGIC, "¡$n regresa a su forma original de dragón!", ch, NULL,
             NULL, TO_ROOM );
        act( AT_CYAN, "¡Sientes un intenso frío al regresar a tu forma original!", ch,
             NULL, NULL, TO_CHAR );

        while ( ch->first_affect )
            affect_remove( ch, ch->first_affect );

        /*
         * Lets give them the same percent of their higher hp 
         */
        {
            double                  hitprcnt = 0.0;

            if ( ch->hit > 0 && ch->max_hit > 0 )
                hitprcnt = ( ch->hit / ch->max_hit );
            if ( ch->pcdata->tmpmax_hit )
                ch->max_hit = ch->pcdata->tmpmax_hit;
            else {
                ch->max_hit *= 2;                      /* Since all we did was divide it
                                                        * might as well multiply it now
                                                        * but give bug message too. */
                bug( "%s: %s had 0 tmpmax_hit and had to double their current hp.", __FUNCTION__,
                     ch->name );
            }
            if ( hitprcnt > 0.0 )
                ch->hit = ( int ) ( ch->max_hit * hitprcnt );
        }
        ch->height = ch->pcdata->tmpheight;
        ch->weight = ch->pcdata->tmpweight;
        ch->race = ch->pcdata->tmprace;
        ch->Class = ch->pcdata->tmpclass;
        ch->pcdata->tmpmax_hit = 0;
        ch->pcdata->tmpheight = 0;
        ch->pcdata->tmpweight = 0;
        ch->pcdata->tmprace = 0;
        ch->pcdata->tmpclass = 0;

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
        ability = skill_lookup( "human form" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "common" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];
        ability = skill_lookup( "draconic" );
        if ( ability > 0 )
            ch->pcdata->learned[ability] = ch->pcdata->dlearned[ability];

        if ( !IS_NPC( ch ) )
            update_aris( ch );
        save_char_obj( ch );
    }
}

void do_human_form( CHAR_DATA *ch, char *argument )
{
    short                   nomore = 100;
    AFFECT_DATA             af;

    // Temp fix for players switching from dragons to human form to remove curse and
    // poison. -Taon
    if ( IS_AFFECTED( ch, AFF_CURSE ) || IS_AFFECTED( ch, AFF_POISON ) ) {
        send_to_char( "El veneno que corre por tu sangre te lo impide.", ch );
        return;
    }

    if ( IS_NPC( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) && ch->in_room->height <= 120
         && ch->in_room->height >= 12 ) {
        send_to_char( "No puedes cambiar de forma aquí.\r\n", ch );
        return;
    }
    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        affect_strip( ch, gsn_human_form );
        xREMOVE_BIT( ch->affected_by, AFF_DRAGONLORD );
        return;
    }

    if ( ch->mana < nomore ) {
        send_to_char( "No tienes suficiente mana para hacer eso.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_human_form ) ) {
        ch->mana = ( ch->mana - nomore );
        humanform_change( ch, TRUE );
        return;
    }
    else {
        learn_from_failure( ch, gsn_human_form );
        send_to_char( "&cFallas al invocar las antiguas energías.\r\n", ch );
        return;
    }
}

void do_lick( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   nomore;

    nomore = ch->level + 10;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        victim = ch;
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "no está aquí.\r\n", ch );
        return;
    }

    if ( ch->fighting ) {
        send_to_char( "No mientras luchas.\r\n", ch );
        return;
    }

    if ( victim == who_fighting( ch ) ) {
        send_to_char( "Pero si estás luchando....\r\n", ch );
        return;
    }
    if ( ch->mana <= nomore ) {
        send_to_char( "No tienes suficiente mana.\r\n", ch );
        return;
    }

    if ( victim->hit >= victim->max_hit ) {
        send_to_char( "No lo necesita.\r\n", ch );
        return;
    }

    if ( ch->race != RACE_DRAGON && ch->race != RACE_ANIMAL && ch->Class != CLASS_DRAGONLORD ) {
        send_to_char( "No puedes usar esta habilidad.", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_lick ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/lick.wav)\r\n", ch );

        WAIT_STATE( ch, skill_table[gsn_lick]->beats );
        ch->mana = ( ch->mana - nomore );
        learn_from_success( ch, gsn_lick );
        act( AT_YELLOW, "Concentras tu magia en invocar las energías sanadoras.\r\n", ch, NULL, victim,
             TO_CHAR );
        if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
            if ( victim == ch ) {
                act( AT_CYAN, "Curas tus heridas.", ch, NULL, victim, TO_CHAR );
                act( AT_CYAN, "¡$n cura sus heridas!", ch, NULL, victim, TO_NOTVICT );
                act( AT_MAGIC, "¡Tus heridas se cierran y dejas de sangrar!", ch, NULL, victim,
                     TO_CHAR );
            }
        }
        if ( !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
            if ( victim == ch ) {
                act( AT_CYAN, "Abres tus fauces y curas tus heridas.", ch, NULL, victim,
                     TO_CHAR );
                act( AT_CYAN, "¡$n abre sus fauces y cura sus heridas!", ch, NULL, victim,
                     TO_NOTVICT );
                act( AT_MAGIC, "¡Tus heridas se cierran y dejas de sangrar!", ch, NULL, victim,
                     TO_CHAR );
            }
            else {
                act( AT_CYAN, "Abres tus fauces y curas las heridas de $N.", ch, NULL, victim,
                     TO_CHAR );
                act( AT_CYAN, "¡$n abre sus fauces y cura tus heridas!", ch, NULL, victim,
                     TO_VICT );
                act( AT_CYAN, "¡$n abre sus fauces y cura las heridas de $N!", ch, NULL, victim,
                     TO_NOTVICT );
                act( AT_MAGIC, "¡Tus heridas se cierran y dejas de sangrar!", ch, NULL, victim,
                     TO_VICT );
            }
        }
        victim->hit = victim->hit + ( ch->level / 2 ) + 50;
        if ( victim->hit > victim->max_hit ) {         // Bug fix below, -Taon
            victim->hit = victim->max_hit;
        }
        return;
    }
    else
        act( AT_CYAN, "Concentras tu magia en la sanación de heridas, pero te distraes.", ch,
             NULL, victim, TO_CHAR );
    learn_from_failure( ch, gsn_lick );
    return;
}

const int               MidDBLevel = 30;
const int               HighDBLevel = 60;

int GetBreathDam( int level, int power )
{
// Rewrote the entire damage scale here. -Taon
// Rewrote it again based on breath levels - Torin - 9/12/2007 2:28PM  dam1 is level 1, dam2 is level 30, dam3 is level 60
// power = URANGE(1, power, 3);
// Updated default and Case 2 so damage wasn't the same for <50 level - 
// Aurin 11/21/2010
    switch ( power ) {
        default:
            if ( level < 8 )
                return number_range( 30, 50 );
            if ( level < 15 )
                return number_range( 40, 60 ) + level * 2;
            if ( level < 25 )
                return number_range( 55, 75 ) + level * 3;
            if ( level < 35 )
                return ( int ) ( number_range( 95, 105 ) + level * 3.5 );
            if ( level < 50 )
                return ( int ) ( number_range( 150, 160 ) + level * 3.8 );
            if ( level < 65 )
                return ( int ) ( number_range( 250, 260 ) + level * 4.0 );
            if ( level < 80 )
                return ( int ) ( number_range( 255, 265 ) + level * 5.0 );
            return ( int ) ( number_range( 255, 265 ) + level * 6 );
        case 2:
            if ( level < 8 )
                return number_range( 45, 65 );
            if ( level < 15 )
                return number_range( 65, 85 ) + level * 2;
            if ( level < 25 )
                return number_range( 85, 95 ) + level * 3;
            if ( level < 35 )
                return ( int ) ( number_range( 155, 185 ) + level * 3.5 );
            if ( level < 50 )
                return ( int ) ( number_range( 185, 265 ) + level * 4.0 );
            if ( level < 65 )
                return ( int ) ( number_range( 265, 295 ) + level * 5.0 );
            if ( level < 80 )
                return ( int ) ( number_range( 295, 310 ) + level * 6.0 );
            return ( int ) ( number_range( 295, 310 ) + level * 7 );
        case 3:
            if ( level < 8 )
                return number_range( 80, 90 );
            if ( level < 15 )
                return ( int ) ( number_range( 100, 110 ) + level * 2.5 );
            if ( level < 25 )
                return ( int ) ( number_range( 190, 200 ) + level * 3.5 );
            if ( level < 35 )
                return ( int ) ( number_range( 250, 260 ) + level * 4 );
            if ( level < 50 )
                return ( int ) ( number_range( level * 3, level * 6 ) );
            if ( level < 65 )
                return ( int ) ( number_range( level * 5, level * 8 ) );
            if ( level < 80 )
                return ( int ) ( number_range( level * 8, level * 10 ) );
            return ( int ) ( number_range(level * 8, level * 10 ));
    }
}

void SendDBHelp( CHAR_DATA *ch )
{
    char                    buf[MIL];

    send_to_char( "¡Qué aliento quieres respirar?\r\n", ch );
    send_to_char( "Sintáxis: respirar [víctima] [tipo]\r\n", ch );
    send_to_char( "Los tipos pueden ser:\r\n", ch );
    switch ( ch->Class ) {
        default:
            strcpy( buf, "Ninguno\r\n" );
            break;
        case CLASS_BLACK:
            sprintf( buf, "sombra, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "oscuridad, ",
                     ( ch->level < HighDBLevel ) ? "" : "muerte, " );
            break;
        case CLASS_RED:
            sprintf( buf, "humo, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "fuego, ",
                     ( ch->level < HighDBLevel ) ? "" : "infierno, " );
            break;
        case CLASS_SILVER:
            sprintf( buf, "hierro, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "acero, ",
                     ( ch->level < HighDBLevel ) ? "" : "titanio, " );
            break;
        case CLASS_BLUE:
            sprintf( buf, "Agua, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "hielo, ",
                     ( ch->level < HighDBLevel ) ? "" : "granizo, " );
            break;
        case CLASS_GOLD:
            sprintf( buf, "bendición, %s%saoe\r\n", ( ch->level < MidDBLevel ) ? "" : "divino, ",
                     ( ch->level < HighDBLevel ) ? "" : "celestial, " );
            break;
        case CLASS_TWOHEADED:
            sprintf( buf, "sombra, humo, hierro, agua, bendicion, %s%saoe\r\n",
                     ( ch->level < MidDBLevel ) ? "" : "oscuridad, fuego, acero, hielo, divino,\r\n",
                     ( ch->level <
                       HighDBLevel ) ? "" : "celestial, muerte, titanio, infierno, granizo, " );
            break;
    }
    send_to_char( buf, ch );
    return;
}

void do_breathe( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch,
                           *vch_next;
    AFFECT_DATA             af;
    char                    arg[MIL];
    char                    arg2[MIL];
    bool                    ch_died = FALSE;

    int                     nomore = UMIN( ch->level + 10, 60 );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual impide que uses tus alientos.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && ( ch->move <= 25 || ch->move < nomore ) ) {
        set_char_color( AT_WHITE, ch );
        send_to_char( "El cansancio no te lo permite.\r\n", ch );
        return;                                        /* missing return fixed March
                                                        * 11/96 */
    }
    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/breathe.wav)\r\n", ch );

    if ( ( !str_cmp( arg, "aoe" ) && !*arg2 ) || !str_cmp( arg2, "aoe" ) ) {
        if ( ch->move < nomore * 2 ) {
            set_char_color( AT_WHITE, ch );
            send_to_char( "El cansancio te lo impide.\r\n", ch );
            return;
        }

        short                   twoheaded;

        twoheaded = 0;
        if ( ch->Class == CLASS_TWOHEADED ) {
            twoheaded = number_range( 1, 5 );
        }

        if ( ch->Class == CLASS_GOLD || twoheaded == 1 ) {
            act( AT_YELLOW, "¡Respiras una nube de energía divina!", ch, NULL, NULL, TO_CHAR );
            act( AT_YELLOW,
                 "¡$n respira una nube de energía divina, envolviéndolo todo en su furia!", ch,
                 NULL, NULL, TO_ROOM );
        }
        else if ( ch->Class == CLASS_RED || twoheaded == 2 ) {
            act( AT_RED, "¡Respiras una inmensa nube de humo!", ch, NULL, NULL, TO_CHAR );
            act( AT_RED,
                 "¡$n respira una inmensa y afixiante nube de humo que lo envuelve todo con su furia!", ch,
                 NULL, NULL, TO_ROOM );
        }
        else if ( ch->Class == CLASS_BLACK || twoheaded == 3 ) {
            act( AT_DGREY, "¡Respiras una inmensa nube de energía oscura!", ch, NULL, NULL, TO_CHAR );
            act( AT_DGREY,
                 "$n respira una inmensa nube de sombras que lo envuelve todo!", ch,
                 NULL, NULL, TO_ROOM );
        }
        else if ( ch->Class == CLASS_SILVER || twoheaded == 4 ) {
            act( AT_GREY, "¡Respiras una enorme masa de fragmentos de metal!", ch, NULL, NULL, TO_CHAR );
            act( AT_GREY,
                 "¡$n respira una enorme masa de metal que lo envuelve todo!",
                 ch, NULL, NULL, TO_ROOM );
        }
        else if ( ch->Class == CLASS_BLUE || twoheaded == 5 ) {
            act( AT_LBLUE, "¡Respiras una inmensa corriente de agua!", ch, NULL, NULL, TO_CHAR );
            act( AT_LBLUE,
                 "¡$n respira una inmensa corriente de agua que lo envuelve todo en su furia!", ch,
                 NULL, NULL, TO_ROOM );
        }

        WAIT_STATE( ch, skill_table[gsn_breath]->beats );
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;

            if ( !IS_NPC( vch ) && check_phase( ch, vch ) )
                return;
            if ( !IS_NPC( vch ) && check_tumble( ch, vch ) )
                return;

            if ( IS_NPC( vch ) && ( vch->pIndexData->vnum == MOB_VNUM_SOLDIERS || vch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) // Aurin
                continue;

            // Bug fix here, was striking grouped members. -Taon
            if ( is_same_group( vch, ch ) )
                continue;
            if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                 && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;
            if ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) ) {
                if ( char_died( ch ) ) {
                    ch_died = TRUE;
                    break;
                }
                global_retcode = damage( ch, vch, GetBreathDam( ch->level, 1 ), gsn_breath );
            }
        }
        ch->move = ( ch->move - nomore * 2 );
        learn_from_success( ch, gsn_breath );
        return;
    }
    if ( arg[0] == '\0' && !ch->fighting ) {
        SendDBHelp( ch );
        return;
    }
    CHAR_DATA              *victim = get_char_room( ch, arg );

    if ( !victim ) {
        if ( *arg2 || ( victim = who_fighting( ch ) ) == NULL ) {
            send_to_char( "No está aquí.\r\n", ch );
            return;
        }
        strcpy( arg2, arg );
    }

    short                   coin;

    coin = number_chance( 1, 4 );

    if ( !IS_NPC( victim ) ) {
        OBJ_DATA               *obj;

        if ( victim != ch ) {
            if ( ( obj = get_eq_char( victim, WEAR_SHIELD ) ) != NULL ) {
                act( AT_DGREEN, "Bloqueas el aliento de dragón de $n con tu escudo.", ch, NULL, victim,
                     TO_VICT );
                ch_printf( ch, "&G¡%s bloquea con su escudo tu aliento!", victim->name );
                return;
            }
            else if ( victim->pcdata->learned[skill_lookup( "tumble" )] > 0 && coin > 2 ) {
                act( AT_CYAN, "Te apartas del aliento de dragón de $n.", ch, NULL, victim,
                     TO_VICT );
                ch_printf( ch, "&c%s consigue apartarse justo a tiempo de tu aliento.", victim->name );
                return;
            }
            else if ( victim->pcdata->learned[skill_lookup( "phase" )] > 0 && coin > 2 ) {
                act( AT_DGREEN, "Tu cuerpo absorbe el aliento de draón de $n.", ch, NULL, victim,
                     TO_VICT );
                ch_printf( ch, "&cEl cuerpo de %s absorbe tu aliento de dragón.", victim->name );
                return;
            }
        }
    }

    int                     chance = number_range( 1, 100 );

// Updated to allow damage from second head only if target specified; 
// reduced second head damage so there is a noticeable difference at 
// higher levels...this was originally intended as a small damage 
// increase - Aurin 11/21/2010

    if ( ch->Class == CLASS_TWOHEADED && chance ) {
        act( AT_SKILL, "¡Respiras al mismo tiempo con tu segunda cabeza!", ch, NULL, NULL,
             TO_CHAR );
        act( AT_SKILL, "¡$n respira al mismo tiempo con su segunda cabeza!", ch, NULL, NULL,
             TO_ROOM );
        if ( ch->fighting )
            victim = who_fighting( ch );
        global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
    }

// BLACK Dragons - Torin
// Following ifcheck is a bugfix - Aurin 11/22/2010
    if ( char_died( victim ) ) {
        ch_died = TRUE;
    }
    else {
        if ( ch->Class == CLASS_BLACK || ch->Class == CLASS_TWOHEADED ) {
            if ( !str_prefix( arg2, "muerte" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    learn_from_success( ch, gsn_breath );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_DGREY, "¡Exalas una nube que contiene el mal en su estado puro!", ch, NULL,
                         victim, TO_CHAR );
                    act( AT_DGREY,
                         "¡$n exala una nube que contiene el mal puro a tu alrededor!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_DGREY,
                         "¡$n exala una nube que contiene el mal en su estado puro envolviendo a $N!",
                         ch, NULL, victim, TO_NOTVICT );
                    if ( chance < 11 ) {
// stop players being changed to human priests - Vladaar
                        if ( IS_AFFECTED( victim, AFF_DRAGONLORD ) ) {
                            interpret( victim, ( char * ) "human" );
                        }

                        while ( victim->first_affect )
                            affect_remove( victim, victim->first_affect );
                        if ( IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
                            xREMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
                        }
                        else if ( IS_AFFECTED( victim, AFF_SHIELD ) ) {
                            xREMOVE_BIT( victim->affected_by, AFF_SHIELD );
                        }
                        if ( IS_AFFECTED( victim, AFF_POISON ) )
                            return;
                        act( AT_DGREY,
                             "¡Tu nube maligna disipa y envenena a $N!",
                             ch, NULL, victim, TO_CHAR );
                        act( AT_DGREY,
                             "¡La nube maligna de $n te disipa y envenena!",
                             ch, NULL, victim, TO_VICT );
                        af.type = gsn_poison;
                        af.duration = ch->level;
                        af.level = ch->level;
                        af.location = APPLY_STR;
                        af.modifier = -2;
                        victim->degree = 1;
                        af.bitvector = meb( AFF_POISON );
                        affect_join( victim, &af );

                    }

                    // Made death breath hit a little harder because of the
                    // chance being taking. -Taon
                    global_retcode =
                        damage( ch, victim, GetBreathDam( ch->level, 3 ) + ( ch->level * 2 ),
                                gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "oscuridad" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_DGREY, "¡Exalas una nube de oscuridad pura!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_DGREY, "¡$n exala una nube de oscuridad pura que te rodea!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_DGREY, "¡$n exala una nube de oscuridad pura que rodea a $N!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_DGREY, "¡Tu nube de oscuridad disipa a $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_DGREY, "¡La nube de oscuridad de $n te disipa!", ch,
                             NULL, victim, TO_VICT );
                        while ( victim->first_affect )
                            affect_remove( victim, victim->first_affect );
                        if ( IS_AFFECTED( victim, AFF_SANCTUARY ) ) {
                            xREMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
                        }
                        else if ( IS_AFFECTED( victim, AFF_SHIELD ) ) {
                            xREMOVE_BIT( victim->affected_by, AFF_SHIELD );
                        }

                    }
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "sombra" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_DGREY, "¡Exalas una nube de sombras!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_DGREY, "¡$n exala una nube de sombras que te rodea!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_DGREY,
                         "¡$n exala una nube de sombras que envuelve a $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        if ( IS_AFFECTED( victim, AFF_POISON ) )
                            return;
                        act( AT_DGREY, "¡Tu nube de sombras envenena a $N!", ch, NULL, victim,
                             TO_CHAR );
                        act( AT_DGREY, "¡La nube de sombra de $n te envenena!", ch, NULL, victim,
                             TO_VICT );
                        af.type = gsn_poison;
                        af.duration = ch->level;
                        af.level = ch->level;
                        af.location = APPLY_STR;
                        af.modifier = -2;
                        victim->degree = 1;
                        af.bitvector = meb( AFF_POISON );
                        affect_join( victim, &af );
                    }
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
        }

        if ( ch->Class == CLASS_RED || ch->Class == CLASS_TWOHEADED ) {
            // RED dragons - Torin
            if ( !str_prefix( arg2, "infierno" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );

                    act( AT_RED, "¡Exalas llamas blancas provenientes del mismo infierno!!!", ch, NULL,
                         victim, TO_CHAR );
                    act( AT_RED,
                         "¡$n exala blancas llamas provenientes del infierno que te rodean!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_RED,
                         "¡$n exala blancas llamas provenientes del infierno que rodean a $N!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_RED, "¡Tus llamaradas infernales capturan a $N!", ch, NULL, victim,
                             TO_CHAR );
                        act( AT_RED, "¡las llamaradas infernales de $n te atrapan!", ch, NULL, victim,
                             TO_VICT );
                        af.type = gsn_thaitin;
                        af.duration = 50;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.level = ch->level;
                        af.bitvector = meb( AFF_THAITIN );
                        affect_to_char( victim, &af );
                        if ( !IS_AFFECTED( victim, AFF_BLINDNESS ) ) {
                            af.type = gsn_blindness;
                            af.location = APPLY_HITROLL;
                            af.modifier = -6;
                            if ( !IS_NPC( victim ) && !IS_NPC( ch ) )
                                af.duration = ( ch->level + 10 ) / get_curr_con( victim );
                            else
                                af.duration = 3 + ( ch->level / 15 );
                            af.bitvector = meb( AFF_BLINDNESS );
                            af.level = ch->level;
                            affect_join( victim, &af );
                            act( AT_SKILL, "¡No puedes ver nada!", victim, NULL, NULL, TO_CHAR );
                        }

                    }
                    else
                        global_retcode =
                            damage( ch, victim, GetBreathDam( ch->level, 3 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "fuego" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_RED, "¡Exalas una enorme nube de fuego!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_RED, "¡$n exala una enorme nube de fuego que te rodea!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_RED,
                         "¡$n exala una enorme nube de fuego que envuelve a $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_RED, "¡Tus llamaradas atrapan a $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_RED, "¡$n te atrapa con sus llamas!", ch, NULL,
                             victim, TO_VICT );
                        af.type = gsn_thaitin;
                        af.duration = 50;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.level = ch->level;
                        af.bitvector = meb( AFF_THAITIN );
                        affect_to_char( victim, &af );
                    }
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "humo" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_RED, "¡Exalas una nube de humo!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_RED, "¡$n exala una nube de humo sobre ti!", ch, NULL,
                         victim, TO_VICT );
                    act( AT_RED, "¡$n exala una nube de humo sobre $N!", ch, NULL, victim,
                         TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( !IS_AFFECTED( victim, AFF_BLINDNESS ) && chance < 11 ) {
                        act( AT_RED, "¡Tu nube de humo ciega a $N!", ch, NULL, victim, TO_CHAR );
                        act( AT_RED, "¡La nube de humo de $n te ciega!", ch, NULL, victim, TO_VICT );

                        af.type = gsn_blindness;
                        af.location = APPLY_HITROLL;
                        af.modifier = -6;
                        if ( !IS_NPC( victim ) && !IS_NPC( ch ) )
                            af.duration = ( ch->level + 10 ) / get_curr_con( victim );
                        else
                            af.duration = 3 + ( ch->level / 15 );
                        af.bitvector = meb( AFF_BLINDNESS );
                        af.level = ch->level;
                        affect_join( victim, &af );
                    }
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
        }

        if ( ch->Class == CLASS_SILVER || ch->Class == CLASS_TWOHEADED ) {
            // SILVER dragons - Torin
            if ( !str_prefix( arg2, "titanio" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_GREY, "¡Exalas una sólida masa de titanio!!!", ch, NULL,
                         victim, TO_CHAR );
                    act( AT_GREY,
                         "¡$n exala una sólida masa de titanio que te golpea!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_GREY,
                         "¡$n exala una sólida masa de titanio que golpea a $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_GREY, "¡Tu aliento de titanio derriba y debilita a $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_GREY, "¡El aliento de titanio de $n te derriba y debilita!!!", ch,
                             NULL, victim, TO_VICT );
                        af.type = gsn_breath;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.duration = 1;
                        af.level = ch->level;
                        af.bitvector = meb( AFF_PARALYSIS );
                        affect_to_char( victim, &af );
                        if ( IS_NPC( victim ) )
                            WAIT_STATE( victim, 2 * ( PULSE_VIOLENCE / 2 ) );
                        else
                            WAIT_STATE( victim, 1 * ( PULSE_VIOLENCE / 2 ) );
                        update_pos( victim );
                        af.type = gsn_breath;
                        af.duration = ch->level + 10;
                        af.location = APPLY_STR;
                        af.modifier = -4;
                        af.level = ch->level;
                        xCLEAR_BITS( af.bitvector );
                        affect_to_char( victim, &af );

                    }
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 3 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "acero" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_GREY, "¡Exalas una corriente de acero fundido!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_GREY, "¡$n exala una corriente de acero fundido que te envuelve!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_GREY,
                         "¡$n exala una corriente de acero fundido, envolviendo a $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_GREY, "¡Tu corriente de acero fundido derriba a $N!!!", ch, NULL, victim, TO_CHAR );   // Aurin
                        act( AT_GREY, "¡La corriente de acero fundido de $n te derriba!!!", ch, NULL, victim, TO_VICT );  // Aurin
                        af.type = gsn_breath;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.duration = 1;
                        af.level = ch->level;
                        af.bitvector = meb( AFF_PARALYSIS );
                        affect_to_char( victim, &af );
                        if ( IS_NPC( victim ) )
                            WAIT_STATE( victim, 2 * ( PULSE_VIOLENCE / 2 ) );
                        else
                            WAIT_STATE( victim, 1 * ( PULSE_VIOLENCE / 2 ) );
                        update_pos( victim );
                    }

                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "hierro" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_GREY, "¡Exalas una enorme masa de fragmentos de hierro!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_GREY, "¡$n exala una enorme masa de fragmentos de hierro que te envuelve!",
                         ch, NULL, victim, TO_VICT );
                    act( AT_GREY, "¡$n exala una enorme masa de hierro que envuelve a $N!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_GREY, "¡Tus fragmentos de hierro debilitan a $N!", ch, NULL, victim, TO_CHAR );
                        act( AT_GREY, "¡Los fragmentos de hierro de $n te debilitan!", ch, NULL, victim, TO_VICT );
                        af.type = gsn_breath;
                        af.duration = ch->level + 10;
                        af.location = APPLY_STR;
                        af.modifier = -4;
                        af.level = ch->level;
                        xCLEAR_BITS( af.bitvector );
                        affect_to_char( victim, &af );
                    }
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
        }

        if ( ch->Class == CLASS_BLUE || ch->Class == CLASS_TWOHEADED ) {
            // BLUE Dragons
            if ( !str_prefix( arg2, "granizo" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_LBLUE, "¡Exalas un sólido torbellino de granizos!!!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_LBLUE, "¡$n exala un sólido torbellino de granizos que te rodea de golpe!",
                         ch, NULL, victim, TO_VICT );
                    act( AT_LBLUE, "¡$n exala un torbellino de granizos que encierra a $N!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 3 ), gsn_breath );
                    if ( chance < 11 && !char_died( victim ) ) {
                        act( AT_LBLUE,
                             "¡Tus granizos congelan a $N que comienza a ahogarse!", ch,
                             NULL, victim, TO_CHAR );
                        act( AT_LBLUE,
                             "¡Los granizos de $n te congelan y comienzas a ahogarte!", ch,
                             NULL, victim, TO_VICT );
                        if ( IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 300, 400 ), gsn_breath );
                        }
                        else if ( !IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 300, 400 ), gsn_breath );
                            victim->pcdata->frostbite = -5;
                            victim->pcdata->holdbreath = -5;
                        }

                    }
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "hielo" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_LBLUE, "¡Exalas una lluvia de fragmentos de hielo!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_LBLUE, "¡$n exala una lluvia de fragmentos de hielo que te envuelve!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_LBLUE,
                         "¡$n exala una lluvia de fragmentos de hielo que envuelven a $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                    if ( chance < 11 && !char_died( victim ) ) {
                        act( AT_LBLUE, "¡Tus fragmentos de hielo congelan a $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_LBLUE, "¡Los fragmentos de hielo de $n te congelan!", ch, NULL,
                             victim, TO_VICT );
                        if ( IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 100, 300 ), gsn_breath );
                        }
                        else if ( !IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 100, 300 ), gsn_breath );
                            victim->pcdata->frostbite = -5;
                        }
                    }

                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "agua" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_LBLUE, "¡Exalas un torrente de agua!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_LBLUE,
                         "¡$n exala un torrente de agua que te envuelve!",
                         ch, NULL, victim, TO_VICT );
                    act( AT_LBLUE,
                         "¡$n exala un torrente de agua que envuelve a $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                    if ( chance < 11 && !char_died( victim ) ) {
                        act( AT_LBLUE, "¡Tu torrente de agua comienza a ahogar a $N!", ch, NULL,
                             victim, TO_CHAR );
                        act( AT_LBLUE, "El torrente de agua de $n te ahoga!", ch, NULL,
                             victim, TO_VICT );
                        if ( IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 50, 200 ), gsn_breath );
                        }
                        else if ( !IS_NPC( victim ) ) {
                            global_retcode =
                                damage( ch, victim, number_range( 50, 200 ), gsn_breath );
                            victim->pcdata->holdbreath = -5;
                        }
                    }
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
        }

        if ( ch->Class == CLASS_GOLD || ch->Class == CLASS_TWOHEADED ) {
            // GOLD dragons - Torin
            if ( !str_prefix( arg2, "celestial" ) && ch->level >= HighDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_YELLOW, "¡Exalas una vorágine de poder celestial!!!", ch, NULL,
                         victim, TO_CHAR );
                    act( AT_YELLOW,
                         "¡$n exala una vorágine de poder celestial que te envuelve!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_YELLOW,
                         "¡$n exala una vorágine de poder celestial que envuelve a $N!",
                         ch, NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 3 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_YELLOW,
                             "Tu vorágine de poder celestial cura tus heridas, ¡absorbiendo la salud de $N!",
                             ch, NULL, victim, TO_CHAR );
                        if ( ch->hit < ch->max_hit - 500 ) {
                            ch->hit += 500;
                        }
                        affect_strip( ch, gsn_maim );
                        affect_strip( ch, gsn_brittle_bone );
                        affect_strip( ch, gsn_festering_wound );
                        affect_strip( ch, gsn_poison );
                        affect_strip( ch, gsn_thaitin );
                        xREMOVE_BIT( ch->affected_by, AFF_THAITIN );
                        xREMOVE_BIT( ch->affected_by, AFF_BRITTLE_BONES );
                        xREMOVE_BIT( ch->affected_by, AFF_FUNGAL_TOXIN );
                        xREMOVE_BIT( ch->affected_by, AFF_MAIM );
                    }
                    return;
                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "divino" ) && ch->level >= MidDBLevel ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_YELLOW, "¡Exalas una corriente de energía divina!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_YELLOW, "¡$n exala una corriente de energía divina que te envuelve!",
                         ch, NULL, victim, TO_VICT );
                    act( AT_YELLOW,
                         "¡$n exala una corriente de energía divina que envuelve a $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 2 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_YELLOW,
                             "¡Tu corriente de energía divina absorbe parte de la salud de $N y te la regresa a ti!",
                             ch, NULL, victim, TO_CHAR );
                        act( AT_YELLOW,
                             "¡La corriente de energía divina de $n absorbe parte de tu salud para entregársela a su creador!", ch,
                             NULL, victim, TO_VICT );
                        if ( ch->hit < ch->max_hit - 200 ) {
                            ch->hit += 200;
                        }
                    }

                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
            if ( !str_prefix( arg2, "bendicion" ) ) {
                if ( can_use_skill( ch, number_percent(  ), gsn_breath ) ) {
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    ch->move = ( ch->move - nomore );
                    if ( ch->fighting )
                        victim = who_fighting( ch );
                    act( AT_YELLOW, "¡Exalas una nube de energía bendita!", ch, NULL, victim,
                         TO_CHAR );
                    act( AT_YELLOW, "¡$n exala una nube de energía bendita que te envuelve!", ch,
                         NULL, victim, TO_VICT );
                    act( AT_YELLOW,
                         "¡$n exala una nube de energía bendita que envuelve a $N!", ch,
                         NULL, victim, TO_NOTVICT );
                    learn_from_success( ch, gsn_breath );
                    global_retcode = damage( ch, victim, GetBreathDam( ch->level, 1 ), gsn_breath );
                    if ( chance < 11 ) {
                        act( AT_YELLOW, "¡Tu nube de energía bendita te cura!", ch,
                             NULL, victim, TO_CHAR );
                        affect_strip( ch, gsn_maim );
                        affect_strip( ch, gsn_brittle_bone );
                        affect_strip( ch, gsn_festering_wound );
                        affect_strip( ch, gsn_poison );
                        affect_strip( ch, gsn_thaitin );
                        xREMOVE_BIT( ch->affected_by, AFF_THAITIN );
                        xREMOVE_BIT( ch->affected_by, AFF_BRITTLE_BONES );
                        xREMOVE_BIT( ch->affected_by, AFF_FUNGAL_TOXIN );
                        xREMOVE_BIT( ch->affected_by, AFF_MAIM );

                    }

                }
                else {
                    learn_from_failure( ch, gsn_breath );
                    WAIT_STATE( ch, skill_table[gsn_breath]->beats );
                    global_retcode = damage( ch, victim, 0, gsn_breath );
                    send_to_char( "No puedes usar tu aliento ahora.\r\n",
                                  ch );
                }
                return;
            }
        }
        SendDBHelp( ch );
        return;
    }
}                                                      // Closes out char_died ifcheck -

                                                       // Aurin

void do_stomp( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual te impide usar esta habilidad.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "Debes estar luchando para eso.\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "¿Por qué?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_stomp ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/stomp.wav)\r\n", ch );

        WAIT_STATE( ch, skill_table[gsn_stomp]->beats );
        act( AT_RED, "¡Levantas el pié para pisar a $N!", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "¡Ves la sombra de $n cuando trata de pisarte!", ch, NULL,
             victim, TO_VICT );
        act( AT_RED, "$n trata de pisar a $N y aplastarle contra el suelo!", ch,
             NULL, victim, TO_NOTVICT );
        act( AT_MAGIC, "¡La tierra tiembla bajo tus pies!", ch, NULL, NULL, TO_CHAR );
        act( AT_MAGIC, "$n hace que la tierra tiemble bajo sus pies.", ch, NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_stomp );
        global_retcode = damage( ch, victim, medium, gsn_stomp );
    }
    else {
        learn_from_failure( ch, gsn_stomp );
        global_retcode = damage( ch, victim, 0, gsn_stomp );
        send_to_char( "Lo intentas, pero te esquiva.\r\n", ch );
    }
    return;
}

//Not quite completed but installed for testing and general use. 8-5-08 -Taon
void do_deathroll( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   dam;

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual te impide usar esta habilidad.\r\n", ch );
        return;
    }
    if ( arg[0] == '\0' ) {
        send_to_char( "Do the deathroll on who?\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "They aren't here.\r\n", ch );
        return;
    }
    if ( ch == victim ) {
        send_to_char( "You can't do a deathroll on yourself...\r\n", ch );
        return;
    }
    if ( !IS_AFFECTED( ch, AFF_FLYING ) || ch->in_room->sector_type == SECT_UNDERWATER ) {
        send_to_char( "You must either be flying, or underwater to properly preform this task.\r\n",
                      ch );
        return;
    }
    if ( victim->position == POS_SITTING || victim->position == POS_RESTING
         || victim->position == POS_SLEEPING ) {
        send_to_char( "They're already to close to the ground.", ch );
        return;
    }
    if ( ch->move < 30 ) {
        send_to_char( "You're too weak to do such a task.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_deathroll ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/deathroll.wav)\r\n", ch );

        WAIT_STATE( ch, skill_table[gsn_deathroll]->beats );
        act( AT_RED,
             "You dive toward $N snatching them up with your powerful jaws, putting them into a deathroll!",
             ch, NULL, victim, TO_CHAR );
        act( AT_RED,
             "A wavery shadow appears overhead in the water as $n comes crashing down at you, snatching you with their jaws, sending you into a deathroll!",
             ch, NULL, victim, TO_VICT );
        act( AT_RED,
             "$n dives down upon $N snatching them with their jaws, sending them into a death roll!",
             ch, NULL, victim, TO_NOTVICT );

        if ( ch->in_room->sector_type == SECT_UNDERWATER
             || ch->in_room->sector_type == SECT_OCEAN
             || ch->in_room->sector_type == SECT_WATERFALL
             || ch->in_room->sector_type == SECT_RIVER
             || ch->in_room->sector_type == SECT_LAKE
             || ch->in_room->sector_type == SECT_DOCK || ch->in_room->sector_type == SECT_WATER_SWIM
             || ch->in_room->sector_type == SECT_WATER_NOSWIM
             || ch->in_room->sector_type == SECT_SWAMP ) {
            act( AT_RED, "You drag $N deeper into the water!", ch, NULL, victim, TO_CHAR );
            act( AT_RED, "$n drags you deeper into the water, before releasing.", ch, NULL, victim,
                 TO_VICT );
            send_to_char( "&cYou panic and swallow a lot of water causing you to choke.\r\n&D",
                          victim );
            dam = number_chance( 1, 5 );
            victim->hit -= dam;
            victim->move -= dam;
        }
        else {
            act( AT_RED, "You slam $N into the ground!", ch, NULL, victim, TO_CHAR );
            act( AT_RED, "$n throws you from their jaws to the ground!", ch, NULL, victim,
                 TO_VICT );
            send_to_char( "&OYou slam into the ground, leaving you gasping for breath.\r\n&D",
                          victim );
            victim->position = POS_SITTING;

            if ( IS_AFFECTED( victim, AFF_FLYING ) ) {
                xREMOVE_BIT( victim->affected_by, AFF_FLYING );
                affect_strip( victim, gsn_wings );
            }
            dam = number_chance( 5, 10 );
            victim->hit -= dam;
            victim->move -= dam;
            update_pos( victim );
        }

        ch->move -= 30;
        learn_from_success( ch, gsn_deathroll );
        global_retcode = damage( ch, victim, extrahigh, gsn_deathroll );
    }
    else {
        ch->move -= 15;
        learn_from_failure( ch, gsn_deathroll );
        global_retcode = damage( ch, victim, 0, gsn_deathroll );
        send_to_char( "Your dive was off allowing your foe to dodge your attempt.\r\n", ch );
    }
    return;
}

//Skill hellfire for red dragons, nearly complete. -Taon
void do_hellfire( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim = NULL;
    char                    arg[MIL];
    short                   dam,
                            chance;

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "tu forma actual te impide usar esta habilidad.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "¿Invocar el fuego del infierno sobre quién?\r\n", ch );
        return;
    }

    if ( VLD_STR( arg ) )
        victim = get_char_room( ch, arg );
    else if ( ch->fighting )
        victim = who_fighting( ch );

    if ( !victim ) {
        send_to_char( "Debes especificar un objetivo o estar luchando para utilizar esta habilidad.", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "¿por qué?\r\n", ch );
        return;
    }

    if ( ch->move < 50 || ch->mana < 50 ) {
        send_to_char( "Estás demasiado débil para acometer esta tarea.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_hellfire ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/forgefire.wav)\r\n", ch );

        WAIT_STATE( ch, skill_table[gsn_hellfire]->beats );
        act( AT_RED, "¡Abres tus poderosas fauces e invocas el poderoso fuego del infierno sobre $N!", ch,
             NULL, victim, TO_CHAR );
        act( AT_RED, "¡$n abre sus poderosas fauces e invoca el poderoso fuego del infierno sobre ti!", ch,
             NULL, victim, TO_VICT );
        act( AT_RED, "¡$n abre sus poderosas fauces e invoca el poderoso fuego del infierno sobre $N!", ch,
             NULL, victim, TO_NOTVICT );

        // Give Balrog remort a little advantage here. -Taon
        if ( ch->Class != CLASS_BALROG )
            chance = number_chance( 1, 10 );
        else
            chance = number_chance( 1, 15 );

        if ( IS_AFFECTED( victim, AFF_SANCTUARY ) && chance > 8 ) {
            act( AT_RED, "¡Quemas el santuario de $N consiguiendo que decaiga!", ch,
                 NULL, victim, TO_CHAR );
            act( AT_RED, "¡$n quema tu protección del santuario, haciendo que decaiga!", ch, NULL, victim,
                 TO_VICT );
            xREMOVE_BIT( victim->affected_by, AFF_SANCTUARY );
            affect_strip( victim, gsn_sanctuary );
        }

        ch->move -= 50;
        if ( ch->Class != CLASS_BALROG )
            ch->mana -= 50;
        else
            ch->blood -= 50;
        learn_from_success( ch, gsn_hellfire );

        // Balrogs get this spell at a higher level, giving them
        // a stage up damage bonus. - Taon
        if ( ch->Class != CLASS_BALROG )
            global_retcode = damage( ch, victim, high, gsn_hellfire );
        else
            global_retcode = damage( ch, victim, extrahigh, gsn_hellfire );
    }
    else {
        ch->move -= 25;

        if ( ch->Class != CLASS_BALROG )
            ch->mana -= 25;
        else
            ch->blood -= 25;
        learn_from_failure( ch, gsn_hellfire );
        global_retcode = damage( ch, victim, 0, gsn_hellfire );
        send_to_char( "No eres capaz de invocar el infierno.\r\n", ch );
    }
}

void do_devour( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( ch->position == POS_MEDITATING ) {
        send_to_char( "¡Perderías la concentración!\r\n", ch );
        return;
    }
    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "Solo puedes soñar en devorar cuerpos mientras duermes.\r\n", ch );
        return;
    }

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu nueva forma te previene de usar esta habilidad.\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) || ch->switched ) {
        send_to_char( "Debes ser un jugador dragón para hacer eso.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Devorar qué cuerpo?\r\n", ch );
        return;
    }
    if ( ms_find_obj( ch ) )
        return;

    obj = get_obj_list_rev( ch, arg, ch->in_room->last_content );
    if ( !obj ) {
        send_to_char( "No lo encuentras.\r\n", ch );
        return;
    }

    if ( obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_COOK ) {
        send_to_char( "No puedes devorar eso.\r\n", ch );
        return;
    }
    if ( ch->pcdata->condition[COND_THIRST] >= 100 && ch->pcdata->condition[COND_FULL] >= 100 ) {
        send_to_char( "No puedes devorar nada con el estómago tan lleno.\r\n", ch );
        return;
    }

    separate_obj( obj );
    extract_obj( obj );

    if ( can_use_skill( ch, number_percent(  ), gsn_devour ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/eat1.wav)\r\n", ch );

        act( AT_RED, "Devoras con avidez $p de un bocado.", ch, obj, NULL, TO_CHAR );
        act( AT_RED, "¡$n devora $p de un bocado!", ch, obj, NULL,
             TO_ROOM );

        if ( ch->pcdata->condition[COND_THIRST] < 100 ) {
            ch->pcdata->condition[COND_THIRST] += ch->level / 4;
        }
        if ( ch->pcdata->condition[COND_FULL] < 100 ) {
            ch->pcdata->condition[COND_FULL] += ch->level / 4;
        }

        learn_from_success( ch, gsn_devour );
    }
    else {
        send_to_char( "Ese cuerpo está demasiado rancio.\r\n", ch );
        learn_from_failure( ch, gsn_devour );
    }
    return;
}

void do_tail_swipe( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;
    CHAR_DATA              *victim;
    char                    arg[MIL];
    short                   chance;

    chance = number_range( 1, 10 );

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual te impide usar esta habilidad.\r\n", ch );
        return;
    }

    /*
     * Lets slow down how often an NPC does this attack lol its murder on players, I think
     * half the time is good 
     */
    if ( IS_NPC( ch ) && chance <= 8 )
        return;

    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "¿Dar un coletazo a quién?\r\n", ch );
        return;
    }

    if ( ( victim = who_fighting( ch ) ) == NULL ) {
        send_to_char( "No estás luchando con nadie.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "El suicidio es para tontos.\r\n", ch );
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_tail_swipe]->beats );
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/tailswipe.wav)\r\n", ch );
    if ( can_use_skill( ch, ( number_percent(  ) ), gsn_tail_swipe ) ) {
        if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) ) {
            WAIT_STATE( victim, PULSE_VIOLENCE );
            act( AT_WHITE, "$n te da un coletazo.", ch, NULL, victim, TO_VICT );
            act( AT_WHITE, "Das un coletazo a $N.", ch, NULL, victim, TO_CHAR );
            act( AT_WHITE, "$n da un coletazo a $N.", ch, NULL, victim, TO_NOTVICT );
            af.type = gsn_tail_swipe;
            af.location = APPLY_AC;
            af.modifier = 20;
            af.duration = 1;
            af.level = ch->level;
            af.bitvector = meb( AFF_PARALYSIS );
            affect_to_char( victim, &af );
            update_pos( victim );
            learn_from_success( ch, gsn_tail_swipe );
            global_retcode = damage( ch, victim, number_chance( 5, 20 ), gsn_tail_swipe );
        }
    }
    else {
        WAIT_STATE( ch, PULSE_VIOLENCE );
        if ( !IS_NPC( ch ) )
            learn_from_failure( ch, gsn_tail_swipe );
        act( AT_WHITE, "$n te intenta dar un coletazo, pero logras esquivarle.", ch, NULL, victim,
             TO_VICT );
        act( AT_WHITE, "Intentas dar un coletazo a $N, pero consigue apartarse justo a tiempo.", ch, NULL, victim,
             TO_CHAR );
        act( AT_WHITE, "$n intenta dar un coletazo a $N, pero no lo consigue.", ch, NULL, victim,
             TO_NOTVICT );
    }
}

void do_rage( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;


    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "No puedes.\r\n", ch );
        return;
    }

    if ( is_affected( ch, gsn_rage ) ) {
        send_to_char( "Ya tienes suficiente rabia.\r\n", ch );
        return;
    }

    if ( ch->Class == CLASS_DRUID || ch->secondclass == CLASS_DRUID
         || ch->thirdclass == CLASS_DRUID ) {
        send_to_char( "No puedes usar esta habilidad.\r\n", ch );
        return;
    }


    if ( can_use_skill( ch, number_percent(  ), gsn_rage ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/dominate.wav)\r\n", ch );

        WAIT_STATE( ch, skill_table[gsn_rage]->beats );
        af.type = gsn_rage;
        af.duration = ch->level + 10;
        af.location = APPLY_HITROLL;
        af.modifier = 2 + ( ch->level / 17 );
        xCLEAR_BITS( af.bitvector );
        af.level = ch->level;
        affect_to_char( ch, &af );

        af.type = gsn_rage;
        af.duration = ch->level + 10;
        af.location = APPLY_DAMROLL;
        af.level = ch->level;
        af.modifier = 2 + ( ch->level / 17 );
        xCLEAR_BITS( af.bitvector );
        affect_to_char( ch, &af );

        af.type = gsn_rage;
        af.duration = ch->level + 10;
        af.location = APPLY_STR;
        af.level = ch->level;
        af.modifier = 2 + ( ch->level / 25 );
        xCLEAR_BITS( af.bitvector );
        affect_to_char( ch, &af );

         if ( ch->race == RACE_ANIMAL )
         {
        act( AT_RED, "¡Aprovechas la rabia animal sintiendo crecer la fuerza en tu interior!",
             ch, NULL, NULL, TO_CHAR );
         }
         else
         {
        act( AT_RED, "¡Aprovechas la ira de los dragones y notas un enorme poder destructivo dentro de ti!",
             ch, NULL, NULL, TO_CHAR );
         }
        act( AT_RED, "¡$n enrojece de furia!", ch, NULL, NULL, TO_ROOM );
        learn_from_success( ch, gsn_rage );
        return;
    }
    else
        learn_from_failure( ch, gsn_rage );
    send_to_char( "Gritas y enrojeces cual tomate, pero no pasa nada.\r\n", ch );
    return;
}

void do_truesight( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "No puedes.\r\n", ch );
        return;
    }

    if ( ch->Class == CLASS_DRUID || ch->secondclass == CLASS_DRUID
         || ch->thirdclass == CLASS_DRUID ) {
        send_to_char( "No puedes usar esta habilidad.\r\n", ch );
        return;
    }


    affect_strip( ch, gsn_truesight );

    WAIT_STATE( ch, skill_table[gsn_truesight]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_truesight ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/truesight.wav)\r\n", ch );

        af.type = gsn_truesight;
        af.duration = ch->level * 3 + 25;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.level = ch->level;
        af.bitvector = meb( AFF_TRUESIGHT );
        affect_to_char( ch, &af );
        send_to_char( "Tus ojos brillan y tu visión aumenta.\r\n", ch );
        learn_from_success( ch, gsn_truesight );
        return;
    }
    else
        learn_from_failure( ch, gsn_truesight );
    send_to_char( "Tus ojos brillan, y cuando piensas que lo has conseguido, se apagan y tu visión sigue como siempre.\r\n", ch );
    return;
}

void do_fly_home( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *sroom;
    int                     rvnum = 42081;
    CHAR_DATA              *opponent;

    if ( ch->move < 50 ) {
        send_to_char( "necesitas descansar para volar a casa.\r\n", ch );
        return;
    }

    if ( !( sroom = get_room_index( rvnum ) ) ) {
        send_to_char( "No existe la sala a la que se te debería enviar si vuelas a casa. Contacta con la administración, aunque deberían haberse enterado ya.\r\n", ch );
        bug( "%s: No se ha encontrado la room con vnum %d.", __FUNCTION__, rvnum );
        return;
    }

    if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
        do_wings( ch, ( char * ) "" );
        send_to_char( "\r\n", ch );
    }

    if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "Tienes que desplegar tus alas primero.\r\n", ch );
        return;
    }

    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/wings.wav)\r\n", ch );

    if ( ( opponent = who_fighting( ch ) ) != NULL ) {
        int                     lose;

        if ( number_bits( 1 ) == 0 || ( !IS_NPC( opponent ) && number_bits( 3 ) > 1 ) ) {
            WAIT_STATE( ch, 4 );
            lose =
                ( int ) ( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.1 );

            if ( ch->desc )
                lose /= 2;
            gain_exp( ch, 0 - lose );
            send_to_char( "¡No has conseguido volar suficientemente rápido! Pierdes experiencia.\r\n", ch );
            return;
        }
        lose = ( int ) ( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.1 );

        if ( ch->desc )
            lose /= 2;
        gain_exp( ch, 0 - lose );
        send_to_char( "¡Huyes del combate! Pierdes experiencia.\r\n", ch );
        stop_fighting( ch, TRUE );
    }

    if ( ch->in_room->sector_type == SECT_INSIDE || ch->in_room->sector_type == SECT_UNDERGROUND
         || IS_SET( ch->in_room->room_flags, ROOM_INDOORS ) ) {
        if ( ch->mana < 50 ) {
            send_to_char( "necesitas más poder mágico para atravesar el vacío.\r\n", ch );
            return;
        }
        send_to_char( "Incumples las reglas del espacio tiempo y reapareces en el cielo.\r\n",
                      ch );
    }

    WAIT_STATE( ch, skill_table[gsn_fly_home]->beats );
    if ( can_use_skill( ch, number_percent(  ), gsn_fly_home ) ) {
        act( AT_WHITE, "Vuelas en el cielo, buscando tu hogar.", ch, NULL, NULL, TO_CHAR );
        if ( ch->in_room->sector_type == SECT_INSIDE || ch->in_room->sector_type == SECT_UNDERGROUND
             || IS_SET( ch->in_room->room_flags, ROOM_INDOORS ) ) {
            act( AT_WHITE, "$n rompe las reglas convencionales del espacio tiempo y desaparece.\r\n", ch,
                 NULL, NULL, TO_ROOM );
            ch->mana = ch->mana - 50;
        }
        else {
            act( AT_WHITE, "$n se eleva hacia el cielo, y vuela hacia el horizonte.\r\n", ch,
                 NULL, NULL, TO_ROOM );
        }
        char_from_room( ch );
        char_to_room( ch, sroom );
        act( AT_RMNAME, "En el cielo", ch, NULL, NULL, TO_CHAR );
        act( AT_RMDESC,
             "&CVes la inmensidad de la tierra debajo de ti,\r\ny el gran cielo a tu alrededor.\r\n",
             ch, NULL, NULL, TO_CHAR );
        act( AT_EXITS, "Salidas: norte este sur oeste", ch, NULL, NULL, TO_CHAR );
        send_to_char( "\r\n&cAterrizas lentamente en el suelo.\r\r\n\n", ch );
        /*
         * Moved the below to just after the land command, so that it shows the correct
         * room landing in, instead of a random wilderness map - Aurin 9/19/2010
         * do_look(ch, (char *)""); 
         */
        if ( ch->pcdata->lair ) {
            send_to_char
                ( "\r\nRompes las reglas convencionales del espacio y el tiempo y reapareces en el cielo.\r\n\r\n",
                  ch );
            sroom = get_room_index( ch->pcdata->lair );
            char_from_room( ch );
            char_to_room( ch, sroom );
            save_char_obj( ch );
        }
        do_land( ch, ( char * ) "" );
        do_look( ch, ( char * ) "" );                  /* Aurin 9/19/2010 */
        ch->move = ch->move - 50;
        learn_from_success( ch, gsn_fly_home );
        return;
    }
    else {
        send_to_char( "Fallas al romper las reglas del contínuo espacio tiempo.\r\n", ch );
        learn_from_failure( ch, gsn_fly_home );
        return;
    }
}

void do_pluck( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg[MIL];

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "tu forma actual te lo impide.\r\n", ch );
        return;
    }

    if ( !IS_AFFECTED( ch, AFF_FLYING ) ) {
        send_to_char( "Debes estar volando primero.\r\n",
                      ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        if ( ch->fighting )
            victim = who_fighting( ch );
        else {
            send_to_char( "¡A quién?\r\n", ch );
            return;
        }
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "¿Por qué?\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_pluck ) ) {
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/pluck.wav)\r\n", ch );
        WAIT_STATE( ch, skill_table[gsn_pluck]->beats );
        act( AT_RED, "¡Caes en picado y atrapas a $N con tus garras!", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "¡$n cae del cielo y te atrapa entre sus garras!", ch, NULL, victim,
             TO_VICT );
        act( AT_RED, "¡$n cae del cielo y atrapa a $N entre sus garras!", ch, NULL, victim,
             TO_NOTVICT );
        global_retcode = damage( ch, victim, mediumhigh, gsn_pluck );
        learn_from_success( ch, gsn_pluck );
        if ( victim->position != POS_DEAD ) {
            act( AT_WHITE, "¡Te elevas en el aire con $N!", ch, NULL, victim, TO_CHAR );
            act( AT_WHITE, "¡$n se eleva en el aire llevándote consigo!", ch, NULL,
                 victim, TO_VICT );
            act( AT_WHITE, "¡$n se eleva en el aire con $N!", ch, NULL, victim, TO_NOTVICT );
            act( AT_RED, "¡Liberas a $N de tus garras y dejas que caiga al suelo!", ch, NULL,
                 victim, TO_CHAR );
            act( AT_RED, "¡$n te libera de su agarre y te deja caer al suelo!", ch, NULL, victim,
                 TO_VICT );
            act( AT_RED, "¡$n suelta a $N que cae desde el cielo y golpea el suelo!", ch, NULL, victim,
                 TO_NOTVICT );
            global_retcode = damage( ch, victim, mediumhigh, gsn_drop );
            return;
        }
    }
    else {
        learn_from_failure( ch, gsn_pluck );
        global_retcode = damage( ch, victim, 0, gsn_pluck );
        act( AT_RED, "¡Intentas atrapar a $N con tus garras, pero fallas!", ch, NULL,
             victim, TO_CHAR );
    }
    return;
}

void do_snatch( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim = NULL;
    char                    arg[MIL];

    argument = one_argument( argument, arg );

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "tu forma actual te impide usar esta habilidad.\r\n", ch );
        return;
    }

    if ( VLD_STR( arg ) )
        victim = get_char_room( ch, arg );
    else if ( ch->fighting )
        victim = who_fighting( ch );

    if ( !victim ) {
        send_to_char( "Debes especificar un objetivo o estar luchando con alguien.\r\n", ch );
        return;
    }

    if ( ch == victim ) {
        send_to_char( "¿por qué?\r\n", ch );
        return;
    }

    if ( xIS_SET( victim->act, ACT_PACIFIST ) ) {      /* Gorog */
        send_to_char( "¡Es pacifista, aprende!\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_snatch ) ) {
        WAIT_STATE( ch, skill_table[gsn_snatch]->beats );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/snatch.wav)\r\n", ch );
        act( AT_RED, "You reach out and snatch $N with your jaws!", ch, NULL, victim, TO_CHAR );
        act( AT_RED, "$n has snatched you up, mauling you in powerful jaws!", ch, NULL, victim,
             TO_VICT );
        act( AT_RED, "$n reaches out and snatches up $N in powerful jaws!", ch, NULL, victim,
             TO_NOTVICT );
        learn_from_success( ch, gsn_snatch );
        global_retcode = damage( ch, victim, low, gsn_snatch );
    }
    else {
        learn_from_failure( ch, gsn_snatch );
        global_retcode = damage( ch, victim, 0, gsn_snatch );
        send_to_char( "You try to snatch, but they dodge your attempt.\r\n", ch );
    }
    return;
}

void do_impale( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    // OBJ_DATA *obj;
    int                     percent;

    if ( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) ) {
        send_to_char( "No puedes hacer eso ahora.\r\n", ch );
        return;
    }

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Empalar a quién?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( victim->fighting ) {
        send_to_char( "No puedes empalar a alguien que está en combate.\r\n", ch );
        return;
    }

    percent = number_percent(  ) - ( get_curr_lck( ch ) - 14 ) + ( get_curr_lck( victim ) - 13 );

    check_attacker( ch, victim );
    WAIT_STATE( ch, skill_table[gsn_impale]->beats );
    if ( !IS_AWAKE( victim ) || can_use_skill( ch, percent, gsn_impale ) ) {

        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/backstab1.wav)\r\n", ch );

        learn_from_success( ch, gsn_impale );
/* Volk: Insane damage is good. Want to give added damage though
 * for things like if the mob can't see you, if you're sneaking etc */
        if ( can_see( victim, ch ) && victim->hit >= victim->max_hit / 2 )
            global_retcode = damage( ch, victim, insane, gsn_impale );

        else if ( can_see( victim, ch ) && victim->hit < victim->max_hit / 2 ) {
            global_retcode = damage( ch, victim, high, gsn_impale );
        }
        else if ( !can_see( victim, ch ) && victim->hit < victim->max_hit / 2 ) {
            send_to_char( "&R¡Tu víctima no te ha visto venir!&w\r\n", ch );
            global_retcode = damage( ch, victim, high, gsn_impale );
        }
        else if ( !can_see( victim, ch ) && victim->hit >= victim->max_hit / 2 ) {
            send_to_char( "&R¡Tu víctima no te ha visto venir!&w\r\n", ch );
            global_retcode = damage( ch, victim, maximum, gsn_impale );
        }

        check_illegal_pk( ch, victim );
    }
    else {
        learn_from_failure( ch, gsn_impale );
        send_to_char( "&ctu empalamiento falla.\r\n", ch );
        check_illegal_pk( ch, victim );
    }
    return;
}

 // Nearly complete, just a few adjustments here and there and some
 // more in depth support throughout the code and its finished. Completed
 // enough I've already got it in play. -Taon
ch_ret spell_flaming_shield( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual te impide hacer eso.\r\n", ch );
        return rNONE;
    }
    if ( IS_AFFECTED( ch, AFF_FLAMING_SHIELD ) ) {
        send_to_char( "Ya te rodea un escudo ígneo.\r\n", ch );
        return rNONE;
    }
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/forgefire.wav)\r\n", ch );
    send_to_char( "&rTe rodea un escudo de llamas.&d\r\n", ch );
    act( AT_MAGIC, "$n is suddenly surrounded by a shield of flames.", ch, NULL, NULL, TO_ROOM );
    af.type = gsn_flaming_shield;
    af.duration = ch->level * 10;
    af.level = ch->level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = meb( AFF_FLAMING_SHIELD );
    affect_to_char( ch, &af );

    return rNONE;
}

//Simple spell made with low level in mind, for black dragons. -Taon
ch_ret spell_acidic_touch( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;
    int                     dam;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual no te permite usar esta habilidad.\r\n", ch );
        return rNONE;
    }

    if ( !IS_NPC( victim ) )
        ch_printf( ch, "Acumulas materiales ácidos en la puta de tus dedos y tocas a %s.\r\n",
                   victim->name );
    else
        ch_printf( ch, "Acumulas ácido en tus dedos y tocas a %s.\r\n",
                   capitalize( victim->short_descr ) );

    if ( ch->level < 50 )
        dam = low;
    else
        dam = medium;

    if ( !IS_AFFECTED( victim, AFF_ACIDIC_TOUCH ) ) {
        send_to_char( "Causas que tu rival se retuerza de dolor.\r\n", ch );
        send_to_char( "Te retuerces de dolor.\r\n", victim );
        af.type = gsn_acidic_touch;
        af.duration = ch->level * 10;
        af.level = ch->level;
        af.location = APPLY_STR;

        if ( ch->level < 25 )
            af.modifier = -1;
        else if ( ch->level < 50 )
            af.modifier = -2;
        else if ( ch->level < 75 )
            af.modifier = -3;
        else
            af.modifier = -4;
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/sizzle.wav)\r\n", ch );
        af.bitvector = meb( AFF_ACIDIC_TOUCH );
        affect_to_char( victim, &af );
    }

    return damage( ch, victim, dam, sn );
}

//Spell for Golden dragons. -Taon
ch_ret spell_searing_light( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    AFFECT_DATA             af;

    if ( IS_AFFECTED( victim, AFF_BLINDNESS ) ) {
        send_to_char( "Ya está..\r\n", ch );
        return rNONE;
    }
    if ( ch == victim ) {
        send_to_char( "¿Por qué?\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;

    if ( ch->level < 50 )
        af.duration = 2;
    else
        af.duration = 4;

    af.bitvector = meb( AFF_BLINDNESS );
    affect_to_char( victim, &af );
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/bless.wav)\r\n", ch );
    if ( !IS_NPC( victim ) )
        ch_printf( ch, "Invocas una poderosa luz que ciega a %s.\r\n", victim->name );
    else
        ch_printf( ch, "Invocas una poderosa luz que ciega a %s.\r\n", victim->short_descr );

    ch_printf( victim, "%s invoca una luz que ciega tus ojos.\r\n", ch->name );

    return rNONE;
}

//Spell for lower level golden dragons. -Taon
ch_ret spell_lend_health( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;

    if ( ch->fighting ) {
        send_to_char( "No mientras luchas.\r\n", ch );
        return rNONE;
    }
    if ( victim->hit >= victim->max_hit ) {
        send_to_char( "No necesita curación.\r\n", ch );
        return rNONE;
    }
    if ( ch->hit <= ch->level ) {
        send_to_char( "No tienes vida suficiente.\r\n", ch );
        return rNONE;
    }

    if ( !IS_NPC( victim ) )
        ch_printf( ch, "Curas a %s.\r\n", victim->name );
    else
        ch_printf( ch, "curas a %s.\r\n", victim->short_descr );

    ch_printf( victim, "%s te cura.\r\n", ch->name );

    victim->hit += ch->level;
    ch->hit -= ch->level;
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/solve.wav)\r\n", ch );
    if ( victim->hit > victim->max_hit )
        victim->hit = victim->max_hit;

    return rNONE;
}

//Completed on 8-11-08 by: Taon.
ch_ret spell_shadow_bolt( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *victim = ( CHAR_DATA * ) vo;
    short                   align_mod;
    int                     dam;

    if ( IS_NPC( ch ) )
        return rNONE;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
        send_to_char( "Tu forma actual te impide usar este hechizo.\r\n", ch );
        return rNONE;
    }

    if ( ch->move < 25 ) {
        send_to_char( "No tienes movimientos suficiente para completar esta tarea.\r\n", ch );
        return rNONE;
    }

    if ( ch->alignment <= -300 )
        dam = maximum;
    else if ( ch->alignment > -300 )
        dam = extrahigh;

    align_mod = number_chance( 1, 20 );
    victim->alignment -= align_mod;
    ch->move -= 25;
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/thunder5.wav)\r\n", ch );
    if ( !IS_NPC( victim ) )
        ch_printf( ch, "&zInvocas un rayo de energía oscura para atacar a %s.\r\n",
                   victim->name );
    else
        ch_printf( ch, "&zinvocas un rayo de energía oscvura para atacar a %s.\r\n",
                   capitalize( victim->short_descr ) );

    return damage( ch, victim, dam, sn );
}

//Spell completed on 8-9-08 by: Taon.
ch_ret spell_aura_of_life( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_AURA_LIFE ) ) {
        send_to_char( "Ya te rodea un aura de vida.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;

    if ( ch->level < 50 )
        af.duration = ch->level * 2;
    else
        af.duration = ch->level * 4;
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/aura.wav)\r\n", ch );
    af.bitvector = meb( AFF_AURA_LIFE );
    affect_to_char( ch, &af );

    act( AT_MAGIC, "$n brilla cuando le rodea un aura de vida.", ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "Un aura de energía curativa te rodea.", ch, NULL, NULL, TO_CHAR );

    return rNONE;

}

//Divine intervention for high level Gold dragons. -Taon
ch_ret spell_greater_restore( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( ch->fighting ) {
        send_to_char( "No mientras luches.\r\n", ch );
        return rNONE;
    }
    if ( IS_AFFECTED( ch, AFF_GREATER_RESTORE ) ) {
        send_to_char( "No puedes conjurar eso todavía..\r\n", ch );
        return rNONE;
    }
    if ( ch->alignment < 300 ) {
        send_to_char( "Necesitas ser una persona más bondadosa para invocar las energías de este conjuro.\r\n", ch );
        return rNONE;
    }
    if ( ch->hit >= ch->max_hit ) {
        send_to_char( "No necesitas curación.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;
    af.duration = 1000;
    af.bitvector = meb( AFF_GREATER_RESTORE );
    affect_to_char( ch, &af );

    act( AT_MAGIC, "$n brilla intensamente y sus heridas se cierran.", ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "Brillas intensamente y tus heridas se curan rápidamente..", ch, NULL, NULL, TO_CHAR );
    ch->hit = ch->max_hit;
    return rNONE;

}

ch_ret spell_healing_essence( int sn, int level, CHAR_DATA *ch, void *vo )
{
    AFFECT_DATA             af;

    if ( IS_AFFECTED( ch, AFF_REACTIVE ) ) {
        send_to_char( "No lo necesitas.\r\n", ch );
        return rNONE;
    }

    af.type = sn;
    af.location = APPLY_AFFECT;
    af.modifier = 0;
    af.level = ch->level;

    if ( ch->level < 50 )
        af.duration = ch->level + 30;
    else
        af.duration = ch->level + 50;

    af.bitvector = meb( AFF_REACTIVE );
    affect_to_char( ch, &af );
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/essence.wav)\r\n", ch );
    act( AT_MAGIC, "$n ruje, 'GarGacha!", ch, NULL, NULL, TO_ROOM );
    act( AT_MAGIC, "Rujes, 'GarGacha!", ch, NULL, NULL, TO_CHAR );

    return rNONE;
}

//Spell firestorm was derived from earthquake, by Taon.
ch_ret spell_firestorm( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch,
                           *vch_next;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
    act( AT_MAGIC, "¡lanzas una inmensa llamarada al aire, dañando a todo aquel que te rodea!", ch,
         NULL, NULL, TO_CHAR );
    act( AT_MAGIC, "$n lanza una increíble llamarada.", ch, NULL, NULL, TO_ROOM );
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/forgefire.wav)\r\n", ch );
    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;

        if ( !vch )
            break;
        if ( vch == ch )
            continue;
        if ( !vch->in_room )
            continue;
        if ( is_same_group( vch, ch ) )
            continue;

        if ( IS_NPC( vch ) && ( vch->pIndexData->vnum == MOB_VNUM_SOLDIERS || vch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) // Aurin
            continue;

        if ( vch->in_room == ch->in_room ) {
            ch_printf( vch, "&R%s engulfs you in flames.&d\r\n", ch->name );
            global_retcode = damage( ch, vch, extrahigh, sn );
        }
    }
    return rNONE;
}

//Spell death field for black dragons. -Taon
//Not quite finished, but installed for testing.
ch_ret spell_death_field( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch,
                           *vch_next;
    SKILLTYPE              *skill = get_skilltype( sn );
    ROOM_INDEX_DATA        *location;
    short                   chance;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }

    act( AT_MAGIC, "¡Comienzas a pronunciar las palabras de lo nunca dicho!", ch, NULL, NULL, TO_CHAR );
    act( AT_MAGIC, "$n comienza a pronunciar las palabras de lo nunca dicho.", ch, NULL, NULL, TO_ROOM );
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/deathfield.wav)\r\n", ch );

    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;
        chance = number_chance( 1, 100 );

        if ( !vch || !vch->next )
            break;
        if ( vch == ch )
            continue;
        if ( !vch->in_room )
            continue;
        if ( is_same_group( vch, ch ) )
            continue;

        if ( IS_NPC( vch ) && ( vch->pIndexData->vnum == MOB_VNUM_SOLDIERS || vch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) // Aurin
            continue;
        if ( vch->in_room == ch->in_room ) {
            if ( ch->level - vch->level >= 10 && ch->alignment <= -500 ) {
                if ( chance < 5 ) {
                    location = get_room_index( ROOM_VNUM_TEMPLE );
                    ch_printf( vch, "You immediatly succumb to &R%s attack.&d\r\n", ch->name );
                    ch_printf( ch, "%s immediatly succumbs to your attacks.&d\r\n", vch->name );
                    vch->hit = 1;
                    stop_fighting( vch, TRUE );

                    if ( !IS_NPC( vch ) ) {
                        char_from_room( vch );
                        char_to_room( vch, location );
                    }
                }
                else if ( chance < 10 ) {
                    if ( !IS_NPC( ch ) && ch->pcdata->lair )
                        location = get_room_index( ch->pcdata->lair );
                    if ( !location )
                        location = get_room_index( ROOM_VNUM_TEMPLE );

                    ch_printf( ch,
                               "Fallas al invocar los poderes de las sombras, y quedas exausto.&R%s.&d\r\n",
                               ch->name );
                    ch->hit = 1;
                    ch->move = 1;
                    ch->mana = 1;
                    ch->alignment += 50;
                    stop_fighting( ch, TRUE );

                    if ( !IS_NPC( ch ) ) {
                        char_from_room( ch );
                        char_to_room( ch, location );
                    }
                }
                else {
                    ch_printf( vch,
                               "&z%s invade tu mente, causándote un enorme dolor.&d\r\n",
                               ch->name );
                    ch_printf( ch, "%s brilla intensamente.\r\n", vch->name );
                    global_retcode = damage( ch, vch, extrahigh, sn );
                }
            }
            else {
                ch_printf( vch,
                           "&z%s intenta invadir tu mente, causándote algo de dolor.&d\r\n",
                           ch->name );
                ch_printf( ch, "%s brilla intensamente.\r\n", vch->name );
                global_retcode = damage( ch, vch, high, sn );
            }
        }
    }
    return rNONE;
}

//Spell Clavus was derived from earthquake by Taon.
ch_ret spell_clavus( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA              *vch,
                           *vch_next;
    SKILLTYPE              *skill = get_skilltype( sn );

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) ) {
        failed_casting( skill, ch, NULL, NULL );
        return rSPELL_FAILED;
    }
    if ( ch->move < 50 ) {
        send_to_char( "no tienes el movimiento necesario para realizar esta tarea.\r\n", ch );
        return rSPELL_FAILED;
    }

    act( AT_MAGIC, "Gritas con furia y sueltas docenas de clavos metálicos al aire.", ch,
         NULL, NULL, TO_CHAR );
    act( AT_MAGIC, "$n grita furiosamente y lanza una docena de clavos metálicos al aire.", ch,
         NULL, NULL, TO_ROOM );
    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/clavus.wav)\r\n", ch );
    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;

        if ( !vch )
            break;
        if ( vch == ch )
            continue;
        if ( !vch->in_room )
            continue;
        if ( IS_AFFECTED( vch, AFF_FLYING ) )
            continue;
        if ( is_same_group( vch, ch ) )
            continue;

        if ( IS_NPC( vch ) && ( vch->pIndexData->vnum == MOB_VNUM_SOLDIERS || vch->pIndexData->vnum == MOB_VNUM_ARCHERS ) ) // Aurin
            continue;

        if ( IS_IMMORTAL( vch ) && vch->in_room ) {
            continue;
        }

        if ( vch->in_room == ch->in_room ) {
            ch_printf( vch, "&zUn clavo metálico de %s te golpea.&d\r\n", ch->name );
            global_retcode = damage( ch, vch, extrahigh, sn );
        }

    }

    ch->move -= 50;
    return rNONE;
}

// skill version of astral_walk spell for dragons
void do_breech( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;

    argument = one_argument( argument, arg );

    if ( ( victim = get_char_world( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "¿No te das cuenta que no tiene sentido?\r\n", ch );
        return;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_CLANSTOREROOM ) ) {
        send_to_char( "No puedes ir allí.\r\n", ch );
        return;
    }

    if ( can_use_skill( ch, number_percent(  ), gsn_breech ) ) {

        if ( IS_SET( ch->in_room->room_flags, ROOM_NO_ASTRAL )
             || IS_SET( ch->in_room->area->flags, AFLAG_NOASTRAL ) ) {
            send_to_char
                ( "&G¡utilizas todas tus energías para cambiar de lugar!\r\n",
                  ch );
            if ( ch->move < ( ch->max_move - 50 ) ) {
                ch->move = 0;
                send_to_char
                    ( "&Gno tienes energías suficientes para ir a una sala con escudo.\r\n", ch );
                return;
            }
            ch->move = 0;
        }
        if ( IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
             || IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) ) {
            send_to_char
                ( "&G¡Utilizas todas tus energías para cambiar de lugar!\r\n",
                  ch );
            if ( ch->move < ( ch->max_move - 50 ) ) {
                ch->move = 0;
                send_to_char
                    ( "&GNo tienes energías suficientes para ir a una sala con escudo.\r\n", ch );
                return;
            }
            ch->move = 0;
        }

        if ( !IS_SET( victim->in_room->room_flags, ROOM_NO_ASTRAL )
             && !IS_SET( victim->in_room->area->flags, AFLAG_NOASTRAL ) ) {
            if ( ch->move > 30 ) {
                ch->move -= 30;
            }
            else {
                send_to_char( "No lo consigues.\r\n", ch );
                return;
            }
        }

        if ( IS_IMMORTAL( victim ) ) {
            send_to_char( "&GNo puedes.\r\n", ch );
            return;
        }
        if ( IS_AFFECTED( ch, AFF_SNARE ) || IS_AFFECTED( ch, AFF_PRAYER )
             || IS_AFFECTED( ch, AFF_ROOT ) ) {
            send_to_char( "No puedes moverte.\r\n", ch );
            return;
        }
        if ( xIS_SET( ch->act, PLR_BATTLE ) )
            send_to_char( "!!SOUND(sound/breech.wav)\r\n", ch );
        act( AT_WHITE, "$n rompe el tiempo y el espacio y desaparece.\r\n", ch,
             NULL, NULL, TO_ROOM );
        act( AT_WHITE, "rompes el contínuo espacio tiempo y desapareces.\r\n", ch,
             NULL, NULL, TO_CHAR );

        char_from_room( ch );
        char_to_room( ch, victim->in_room );
        act( AT_RMNAME, "En el cielo", ch, NULL, NULL, TO_CHAR );
        act( AT_RMDESC,
             "&CVes la inmensidad de la tierra debajo de ti,\r\ny el gran cielo a tu alrededor.\r\n",
             ch, NULL, NULL, TO_CHAR );
        act( AT_EXITS, "Salidas: norte este sur oeste", ch, NULL, NULL, TO_CHAR );
        if ( ch->on ) {
            ch->on = NULL;
            set_position( ch, POS_STANDING );
        }
        if ( ch->position != POS_STANDING ) {
            set_position( ch, POS_STANDING );
        }
        act( AT_WHITE, "Te posas en la tierra causando un remolino de polvo.\r\n", ch,
             NULL, NULL, TO_CHAR );
        act( AT_YELLOW, "¡$n levanta un remolino de polvo al aterrizar!", ch, NULL, NULL, TO_ROOM );
        do_look( ch, ( char * ) "auto" );
        learn_from_success( ch, gsn_breech );
        return;
    }
    else {
        send_to_char( "Fallas al invocar las energías para romper el contínuo espacio tiempo.\r\n", ch );
        learn_from_failure( ch, gsn_breech );
        return;
    }
}
