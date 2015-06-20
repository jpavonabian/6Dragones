/*
 *Code Information: I find this command quite useful, it reveals
 *important information which pertains to the player invoking it.
 *do_status was written by: Taon "Dustan Gunn", for use on 6Dragons
 *mud. This function is still currently under major construction. -Taon
 *Note: This isnt the final information header, just using it to
 *keep those with current shell access a little informed.
 */

#include "h/mud.h"

//I intend on cleaning up the format once finished. -Taon

void do_status( CHAR_DATA *ch, char *argument )
{

    if ( IS_NPC( ch ) )
        return;

    if ( ch->position == POS_SLEEPING ) {
        send_to_char( "¡Despierta primero!\r\n", ch );
        return;
    }

    send_to_char( "\r\n", ch );
    send_to_char
        ( "\r\n&W-------------------&B[&D &i&W&uEstado&D&D&D &B]&D&W---------------------&D\r\n\r\n",
          ch );

    send_to_char( "&B[&D&WHambre&D&B]&D: ", ch );

    if ( !IS_IMMORTAL( ch ) ) {
        if ( ch->pcdata->condition[COND_FULL] <= 0 )
            send_to_char( "&O¡Te estás muriendo de hambre!!!&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_FULL] <= 10 && ch->pcdata->condition[COND_FULL] >= 1 )
            send_to_char( "&O¡Tienes mucha hambre!&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_FULL] <= 20 && ch->pcdata->condition[COND_FULL] >= 11 )
            send_to_char( "&OTienes algo de hambre.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_FULL] <= 30 && ch->pcdata->condition[COND_FULL] >= 21 )
            send_to_char( "&OEmpiezas a tener hambre.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_FULL] >= 31 )
            send_to_char( "&ONo tienes hambre.&D\r\n", ch );
    }
    else
        send_to_char( "&O¡Nunca tienes hambre!&D\r\n", ch );

    send_to_char( "&B[&D&WSed&D&B]&D: ", ch );

    if ( !IS_IMMORTAL( ch ) ) {
        if ( ch->pcdata->condition[COND_THIRST] <= 0 )
            send_to_char( "&O¡Estás muriendo de sed!&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_THIRST] <= 10
                  && ch->pcdata->condition[COND_THIRST] >= 1 )
            send_to_char( "&OAgradecerías hasta el más mínimo sorbo.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_THIRST] <= 20
                  && ch->pcdata->condition[COND_THIRST] >= 11 )
            send_to_char( "&OTu garganta está seca.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_THIRST] <= 30
                  && ch->pcdata->condition[COND_THIRST] >= 21 )
            send_to_char( "&OComienzas a tener sed.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_THIRST] >= 31 )
            send_to_char( "&ONo tienes sed.&D\r\n", ch );
    }
    else
        send_to_char( "&O¡Nunca tienes sed!&D\r\n", ch );

    send_to_char( "&B[&D&WCarga&D&B]&D: ", ch );
    ch_printf( ch, "&O%d/%d&D\r\n", ch->carry_weight, can_carry_w( ch ) );

    send_to_char( "&B[&D&WPosición&D&B]&D: ", ch );

    switch ( ch->position ) {
        case POS_DEAD:
            send_to_char( "&O¡Muerto!!!&D\r\n", ch );
            break;
        case POS_INCAP:
            send_to_char( "&O¡Te han incapacitado!&D\r\n", ch );
            break;
        case POS_STUNNED:
            send_to_char( "&O¡Estás inconsciente!!!&D\r\n", ch );
            break;
        case POS_STANDING:
            send_to_char( "&OEstás de pié.&D\r\n", ch );
            break;
        case POS_MEDITATING:
            send_to_char( "&OEstás meditando profundamente.&D\r\n", ch );
            break;
        case POS_SITTING:
        case POS_RESTING:
            send_to_char( "&OEstás descansando.&D\r\n", ch );
            break;
        case POS_SLEEPING:
            send_to_char( "&OEstás en un profundo sueño.&D\r\n", ch );
            break;
        case POS_MOUNTED:
            send_to_char( "&OEstás cavalgando.&D\r\n", ch );
            break;
        case POS_MORTAL:
            send_to_char( "&O¡Eres un muerto andante!&D\r\n", ch );
            break;
        case POS_EVASIVE:
            send_to_char( "&OEstás en una posición evasiva.&D\r\n", ch );
            break;
        case POS_DEFENSIVE:
            send_to_char( "&OEstás a la defensiva.&D\r\n", ch );
            break;
        case POS_AGGRESSIVE:
            send_to_char( "&OEstás en una posición agresiva.&D\r\n", ch );
            break;
        case POS_BERSERK:
            send_to_char( "&OEstás en modo &iBERSERKER*D!!&D\r\n", ch );
            break;
        case POS_FIGHTING:
            send_to_char( "&O¡Estás luchando!&D\r\n", ch );
            break;
        default:
            send_to_char( "&OError: Position out of bounds, for status.&D\r\n", ch );
            bug( "Error: do_status doesn't reconize %s's current position.", ch->name );
            break;
    }

    send_to_char( "&B[&D&WEstado mental&D&B]&D: ", ch );

    // Allow the fall throughs in this switch. -Taon
/* Whoops, think these are backwards! Were from -1 to -10, fixed 16/7/08 Volk */
    switch ( ch->mental_state / 10 ) {
        case -10:
        case -9:
            send_to_pager( "&OTe cuesta mantener los ojos abiertos.&D\r\n", ch );
            break;
        case -8:
        case -7:
        case -6:
            send_to_pager( "&OTienes muchísimo sueño.&D\r\n", ch );
            break;
        case -5:
        case -4:
        case -3:
            send_to_pager( "&ONecesitas un descanso.&D\r\n", ch );
            break;
        case -2:
        case -1:
            send_to_pager( "&OTe sientes bien.&D\r\n", ch );
            break;
        case 0:
        case 1:
            send_to_pager( "&OTe sientes estupendamente.&D\r\n", ch );
            break;
        case 2:
        case 3:
            send_to_pager( "&Otu mente vuela impidiéndote pensar con claridad.&D\r\n",
                           ch );
            break;
        case 4:
        case 5:
        case 6:
            send_to_pager( "&Otu cerebro va a 100 kilómetros por hora.&D\r\n", ch );
            break;
        case 7:
        case 8:
            send_to_pager( "&ONo sabes lo que es real y lo que no.&D\r\n", ch );
            break;
        case 9:
        case 10:
            send_to_pager( "&O&uEres un Dios&D&D.\r\n", ch );
            break;
        default:
            send_to_pager( "&O¡Estás en muy mal estado!&D\r\n", ch );
            break;
    }

    if ( ch->Class == CLASS_MONK || ch->secondclass == CLASS_MONK || ch->thirdclass == CLASS_MONK ) {
        send_to_char( "&B[&D&WEnfoque&D&B]&D: ", ch );

        if ( ch->focus_level == 0 )
            send_to_char( "&ONo tienes enfoque.&D\r\n", ch );
        else if ( ch->focus_level < 10 )
            send_to_char( "&OTienes un pequeño enfoque.\r\n", ch );
        else if ( ch->focus_level < 20 )
            send_to_char( "&OTienes algo de enfoque.&D\r\n", ch );
        else if ( ch->focus_level < 30 )
            send_to_char( "&OTienes bastante enfoque.&D\r\n", ch );
        else if ( ch->focus_level < 40 )
            send_to_char( "&OTienes un alto enfoque.&D\r\n", ch );
        else if ( ch->focus_level <= 50 )
            send_to_char( "&OTienes un enfoque muy alto.&D\r\n", ch );
        else if ( ch->focus_level > 50 || ch->focus_level < 0 ) {
            send_to_char( "Error: Fuera de rango, contacta con la administración.\r\n", ch );
            bug( "Error: [adjust = %d] focus_level out of bounds on %s.\r\n", ch->focus_level,
                 ch->name );
        }
    }

    if ( ch->quest_curr > 1 )
        ch_printf( ch, "&B[&D&WPuntos de gloria&D&B]&D: &O%d&D\r\n", ch->quest_curr );

    send_to_char( "&W\r\nInformación general:&D\r\n", ch );

    if ( ch->move <= ch->max_move / 5 )
        send_to_char( "&ONecesitas descansar.&D\r\n", ch );
    if ( ch->mana <= ch->max_mana / 5 )
        send_to_char( "&Osientes que tu poder mágico ha sido drenado.&D\r\n", ch );
    if ( ch->hit <= ch->max_hit / 5 )
        send_to_char( "&Otienes heridas mortales.&D\r\n", ch );

    if ( IS_BLOODCLASS( ch ) && ( ch->blood <= ch->max_blood / 6 ) )
        send_to_char( "&OTu cuerpo comienza a &unecesitar&D&O sangre. Mucha sangre.&D\r\n", ch );

    if ( ch->pcdata->condition[COND_DRUNK] > 0 ) {
        if ( ch->pcdata->condition[COND_DRUNK] < 3 )
            send_to_char( "&OEstás un poco borracho.&D.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_DRUNK] < 5 )
            send_to_char( "&OEstás borracho.&D.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_DRUNK] < 10 )
            send_to_char( "&OEstás borracho.&D\r\n", ch );
        else if ( ch->pcdata->condition[COND_DRUNK] >= 10 )
            send_to_char( "&Oestás borracho.&D\r\n", ch );
    }

    if ( IS_VAMPIRE( ch ) ) {
        if ( IS_OUTSIDE( ch ) ) {
            switch ( time_info.sunlight ) {
                case SUN_LIGHT:
                    send_to_char( "&YTe quemas bajo el sol.&D\r\n", ch );
                    break;
                case SUN_RISE:
                    send_to_char( "&YSientes como tu piel se quema bajo el sol de la mañana.&D\r\n",
                                  ch );
                    break;
                case SUN_SET:
                    send_to_char( "&Ytu piel se quema debido al sol.&D\r\n", ch );
                    break;
            }
        }
    }

    if ( IS_AFFECTED( ch, AFF_PRAYER ) )
        send_to_char( "&OEstás en un &i rezo profundo.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_SHAPESHIFT ) )
        send_to_char( "&OEstás en otra forma.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_DISGUISE ) )
        send_to_char( "&OOcultas tu identidad bajo un disfraz.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_ANOINT ) )
        send_to_char( "&OTe han ungido recientemente.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_POISON ) )
        send_to_char( "&OSientes el veneno correr por tus venas.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_FURY ) || IS_AFFECTED( ch, AFF_BERSERK ) )
        send_to_char( "&OTe envuelven la &usangre y la rabia&D&O.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_FLYING ) || IS_AFFECTED( ch, AFF_FLOATING ) )
        send_to_char( "&Otus pies no tocan el suelo.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_CHARM ) )
        send_to_char( "&ONo tienes voluntad.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_BLINDNESS ) )
        send_to_char( "&O&i&uUn velo mágico cubre tus ojos impidiéndote la visión.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_HIDE ) )
        send_to_char( "&OTe ocultas de la gente.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_MAIM ) )
        send_to_char( "&OSangras a lo bestia.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_SILENCE ) )
        send_to_char( "&OUna fuerza mágica te impide hablar.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_SURREAL_SPEED ) || IS_AFFECTED( ch, AFF_BOOST ) )
        send_to_char( "&Otu cuerpo se mueve más rápido que un rayo.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_SLOW ) )
        send_to_char( "&O&itu cuerpo se mueve a baja velocidad..&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_PARALYSIS ) || ch->position == POS_STUNNED )
        send_to_char( "&OSufres debido a una &i&uparalysis.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_FEIGN ) )
        send_to_char( "&OEstás fingiendo tu propia muerte.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_CURSE ) )
        send_to_char( "&OSientes una maldición.&D\r\n", ch );
    if ( IS_AFFECTED( ch, AFF_IRON_SKIN ) )
        send_to_char( "&OTu piel es tan sólida como el metal.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_UNSEARING_SKIN ) )
        send_to_char( "&OEres resistente a las llamas.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_ROOT ) )
        send_to_char( "&OTus pies están &uenraizados&D&O en el suelo.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_SHIELD ) || IS_AFFECTED( ch, AFF_WARD ) )
        send_to_char( "&OTe rodea un escudo mágico.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_INFRARED ) )
        send_to_char( "&Otus ojos brillan con un extraño color rojizo.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_DETECT_INVIS ) )
        send_to_char( "&OPuedes ver personas y objetos invisibles.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_DETECT_MAGIC ) )
        send_to_char( "&OPuedes ver la magia.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_DETECT_HIDDEN ) )
        send_to_char( "&OPuedes ver cosas y personas ocultas.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_HEAVENS_BLESS ) )
        send_to_char( "&OEl cielo te ha bendecido.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_SANCTUARY ) )
        send_to_char( "&OEl cielo te proteje.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_REACTIVE ) )
        send_to_char( "&OTu cuerpo se regenera durante el combate.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_TRUESIGHT ) )
        send_to_char( "&OTu visión ha sido mejorada.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_NOSIGHT ) )
        send_to_char( "&OPuedes ver sin tus ojos.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_ICESHIELD ) )
        send_to_char( "&OTe rodea un escudo de hielo.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
        send_to_char( "&OTe rodea un escudo de energía.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_FIRESHIELD ) )
        send_to_char( "&OTe rodea un escudo de fuego.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_RECOIL ) )
        send_to_char( "&Otienes una posición enroyada.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_KEEN_EYE ) )
        send_to_char( "&OTus ojos están en sintonía con tu entorno.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_KINETIC ) )
        send_to_char( "&OTe rodea una barrera de energía cinética.\r\n&D", ch );
    if ( IS_AFFECTED( ch, AFF_SUSTAIN_SELF ) )
        send_to_char( "&OTu cuerpo lucha contra el hambre.\r\n&D", ch );

    if ( ch->hate_level > 0 ) {
        if ( ch->hate_level <= 5 )
            send_to_char( "&OMiras hacia fuera buscando a alguien.&D\r\n", ch );
        else if ( ch->hate_level < 20 )
            send_to_char( "&OTienes la sensación de que alguien te obserba.\r\n&D", ch );
        else if ( ch->hate_level >= 20 )
            send_to_char( "&OTe odian tus propios compañeros.&D\r\n", ch );
    }

    send_to_char( "\r\n&W--------------------------------------------------&D\r\n\r\n", ch );
    return;
}
