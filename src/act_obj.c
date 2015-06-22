/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Object manipulation module                                            *
 ***************************************************************************/

#include <string.h>
#include "h/mud.h"
#include "h/city.h"
#include "h/bet.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/auction.h"
#include "h/currency.h"
/*
 * External functions
 */
CITY_DATA              *get_city( const char *name );
void write_corpses      args( ( CHAR_DATA *ch, char *name, OBJ_DATA *objrem ) );
const char             *get_chance_verb( OBJ_DATA *obj );
const char             *wear_bit_name( int wear_flags );
bool                    can_afford( CHAR_DATA *ch, double cost, short fctype );
bool                    spend_coins( CHAR_DATA *ch, CHAR_DATA *keeper, int cost, short ctype );

int                     check_size( CHAR_DATA *ch );
void                    bard_stop_playing( CHAR_DATA *ch, OBJ_DATA *obj );

/*
 * Local functions.
 */
void get_obj            args( ( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container ) );
bool remove_obj         args( ( CHAR_DATA *ch, int iWear, bool fReplace ) );
void wear_obj           args( ( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, short wear_bit ) );

char                   *get_ed_number args( ( OBJ_DATA *obj, int number ) );
void weapon_type        args( ( CHAR_DATA *ch, int value ) );

OBJ_DATA               *recursive_note_find args( ( OBJ_DATA *obj, char *argument ) );

/*
 * how resistant an object is to damage     -Thoric
 */
short get_obj_resistance( OBJ_DATA *obj )
{
    short                   resist;

    resist = number_fuzzy( MAX_ITEM_IMPACT );

    /*
     * magical items are more resistant
     */
    if ( IS_OBJ_STAT( obj, ITEM_MAGIC ) )
        resist += number_fuzzy( 22 );
    /*
     * metal objects are definately stronger
     */
    if ( IS_OBJ_STAT( obj, ITEM_METAL ) )
        resist += number_fuzzy( 22 );

    if ( IS_OBJ_STAT( obj, ITEM_GLOW ) )
        resist += number_fuzzy( 14 );

    if ( IS_OBJ_STAT( obj, ITEM_HUM ) )
        resist += number_fuzzy( 13 );

    if ( IS_OBJ_STAT( obj, ITEM_LOYAL ) )
        resist += number_fuzzy( 12 );
    /*
     * organic objects are most likely weaker
     */
    if ( IS_OBJ_STAT( obj, ITEM_ORGANIC ) )
        resist -= number_fuzzy( 5 );
    /*
     * blessed objects should have a little bonus
     */
    if ( IS_OBJ_STAT( obj, ITEM_BLESS ) )
        resist += number_fuzzy( 19 );
    /*
     * lets make store inventory pretty tough
     */
    if ( IS_OBJ_STAT( obj, ITEM_INVENTORY ) )
        resist += 30;

    /*
     * okay... let's add some bonus/penalty for item level...
     */
    resist += ( obj->level / 10 ) - 2;

    /*
     * and lasty... take armor or weapon's condition into consideration
     */
    if ( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON )
        resist += ( obj->value[0] / 2 ) - 2;

    return URANGE( 10, resist, 99 );
}

void get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
    int                     weight;

    if ( !CAN_WEAR( obj, ITEM_TAKE ) && ( ch->level < sysdata.level_getobjnotake ) ) {
        send_to_char( "No puedes coger eso.\r\n", ch );
        return;
    }

    if ( xIS_SET( obj->extra_flags, ITEM_PKDISARMED ) && !IS_NPC( ch ) ) {
        if ( CAN_PKILL( ch ) && !get_timer( ch, TIMER_PKILLED ) ) {
            if ( ch->level - obj->value[5] > 5 || obj->value[5] - ch->level > 5 ) {
                send_to_char_color( "\r\n&bLa administración de 6D congela tus acciones.\r\n",
                                    ch );
                return;
            }
            else {
                xREMOVE_BIT( obj->extra_flags, ITEM_PKDISARMED );
                obj->value[5] = 0;
            }
        }
        else {
            send_to_char_color( "\r\n&BLa administración de 6D congela tus acciones.\r\n", ch );
            return;
        }
    }

    if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) && !can_take_proto( ch ) ) {
        send_to_char( "La administración de 6D congela tus acciones.\r\n", ch );
        return;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ) {
        act( AT_PLAIN, "$d: No puedes cargar más objetos.", ch, NULL, obj->short_descr,
             TO_CHAR );
        return;
    }

    if ( IS_OBJ_STAT( obj, ITEM_COVERING ) )
        weight = obj->weight;
    else
        weight = get_obj_weight( obj, FALSE );         /* Allow them to pick up portals *
                                                        * with alot of weight */

    /*
     * Money weight shouldn't count
     */
    if ( obj->item_type != ITEM_MONEY ) {
        if ( obj->in_obj ) {
            OBJ_DATA               *tobj = obj->in_obj;
            int                     inobj = 1;
            bool                    checkweight = FALSE;

            while ( tobj->in_obj ) {
                tobj = tobj->in_obj;
                inobj++;
            }

            if ( !tobj->carried_by || tobj->carried_by != ch ) {
                if ( ( ch->carry_weight + weight ) > can_carry_w( ch ) ) {
                    act( AT_PLAIN, "$d: No puedes cargar con tanto peso.", ch, NULL,
                         obj->short_descr, TO_CHAR );
                    return;
                }
            }
        }
        else if ( ( ch->carry_weight + weight ) > can_carry_w( ch ) ) {
            act( AT_PLAIN, "$d: No puedes cargar con tanto peso.", ch, NULL, obj->short_descr,
                 TO_CHAR );
            return;
        }
    }

    if ( container ) {
        if ( container->item_type == ITEM_KEYRING && !IS_OBJ_STAT( container, ITEM_COVERING ) ) {
            act( AT_ACTION, "Coges $p de $P", ch, obj, container, TO_CHAR );
            act( AT_ACTION, "$n coge $p de $P", ch, obj, container, TO_ROOM );
        }
        else {
            act( AT_ACTION,
                 IS_OBJ_STAT( container,
                              ITEM_COVERING ) ? "Coges $p de $P." :
                 "Coges $p de $P.", ch, obj, container, TO_CHAR );
            act( AT_ACTION,
                 IS_OBJ_STAT( container,
                              ITEM_COVERING ) ? "$n coge $p de $P." :
                 "$n coge $p de $P.", ch, obj, container, TO_ROOM );
        }
        if ( IS_OBJ_STAT( container, ITEM_CLANCORPSE ) && !IS_NPC( ch )
             && str_cmp( container->name + 7, ch->name ) )
            container->value[5]++;
        obj_from_obj( obj );
    }
    else {
        act( AT_ACTION, "Coges $p.", ch, obj, container, TO_CHAR );
        act( AT_ACTION, "$n coge $p.", ch, obj, container, TO_ROOM );
        obj_from_room( obj );
        if ( obj->owner && str_cmp( ch->name, obj->owner ) && obj->item_type == ITEM_TRASH ) {
            act( AT_RED, "$p reniega de ti y de tus manos.", ch,
                 obj, NULL, TO_CHAR );
            separate_obj( obj );
            obj_from_char( obj );
            obj_to_room( obj, ch->in_room );
            ch->hit = ch->hit - 1;
            act( AT_ACTION, "$p se cae de las manos de $n.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "$p se cae de tus manos.", ch, obj, NULL, TO_CHAR );
            return;
        }

    }

    /*
     * Clan storeroom checks
     */
    if ( IS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM )
         && ( !container || container->carried_by == NULL ) ) {
        save_clan_storeroom( ch );
    }

    if ( obj->item_type != ITEM_CONTAINER )
        check_for_trap( ch, obj, TRAP_GET );
    if ( char_died( ch ) )
        return;

    if ( obj->item_type == ITEM_MONEY ) {
        GET_MONEY( ch, obj->value[2] ) += obj->value[0];
        extract_obj( obj );
    }

    else {
        if ( xIS_SET( obj->extra_flags, ITEM_SKELEROT ) ) {
            xTOGGLE_BIT( obj->extra_flags, ITEM_SKELEROT );
            obj->timer = 0;
        }
        obj = obj_to_char( obj, ch );
    }

    if ( char_died( ch ) || obj_extracted( obj ) )
        return;
    oprog_get_trigger( ch, obj );
    return;
}

void                    remove_all_equipment( CHAR_DATA *ch );

void do_get( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;
    OBJ_DATA               *container;
    short                   number;
    bool                    found;

    argument = one_argument( argument, arg1 );

    if ( ch->carry_number < 0 || ch->carry_weight < 0 ) {
        send_to_char
            ( "Dejas caer todo al suelo para reorganizar tu equipo...\r\n",
              ch );
        remove_all_equipment( ch );
        interpret( ch, ( char * ) "dejar todo" );
        ch->carry_weight = 0;
        return;
    }

    if ( is_number( arg1 ) ) {
        number = atoi( arg1 );
        if ( number < 1 ) {
            send_to_char( "Eso fue fácil...\r\n", ch );
            return;
        }
        if ( ( ch->carry_number + number ) > can_carry_n( ch ) ) {
            send_to_char( "No puedes cargar tantos objetos.\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg1 );
    }
    else
        number = 0;
    argument = one_argument( argument, arg2 );
    /*
     * munch optional words
     */
    if ( !str_cmp( arg2, "from" ) && argument[0] != '\0' )
        argument = one_argument( argument, arg2 );
    /*
     * Get type.
     */
    if ( arg1[0] == '\0' ) {
        if ( !IS_NPC( ch ) )
            if ( xIS_SET( ch->act, PLR_OBJID ) )
                send_to_char( "¿Coger id.object?\r\n", ch );
            else {
                send_to_char( "¿Coger qué?\r\n", ch );
            }
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( arg2[0] == '\0' ) {
        if ( number <= 1 && str_cmp( arg1, "todo" ) && str_prefix( "todo.", arg1 ) ) {
            /*
             * 'get obj'
             */
            obj = get_obj_list( ch, arg1, ch->in_room->first_content );
            if ( !obj ) {
                act( AT_PLAIN, "No veo $T por aquí.", ch, NULL, arg1, TO_CHAR );
                return;
            }
            separate_obj( obj );
            get_obj( ch, obj, NULL );
            if ( char_died( ch ) )
                return;
            /*
             * save_house_by_vnum(ch->in_room->vnum);
             */

            if ( xIS_SET( sysdata.save_flags, SV_GET ) )
                save_char_obj( ch );
        }
        else {
            short                   cnt = 0;
            bool                    fTodo;
            char                   *chk;

            if ( !str_cmp( arg1, "todo" ) )
                fTodo = TRUE;
            else
                fTodo = FALSE;
            if ( number > 1 )
                chk = arg1;
            else
                chk = &arg1[4];
            /*
             * 'get all' or 'get all.obj'
             */
            found = FALSE;
            for ( obj = ch->in_room->last_content; obj; obj = obj_next ) {
                obj_next = obj->prev_content;
                if ( ( fTodo || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj ) ) {
                    found = TRUE;
                    if ( number && ( cnt + obj->count ) > number )
                        split_obj( obj, number - cnt );
                    cnt += obj->count;
                    get_obj( ch, obj, NULL );
                    if ( char_died( ch ) || ch->carry_number >= can_carry_n( ch )
                         || ch->carry_weight >= can_carry_w( ch ) || ( number && cnt >= number ) ) {
                        if ( xIS_SET( sysdata.save_flags, SV_GET ) && !char_died( ch ) )
                            save_char_obj( ch );
                        return;
                    }
                }
            }
            if ( !found ) {
                if ( fTodo )
                    send_to_char( "No veo nada por aquí.\r\n", ch );
                else
                    act( AT_PLAIN, "No veo $T por aquí.", ch, NULL, chk, TO_CHAR );
            }
            else if ( xIS_SET( sysdata.save_flags, SV_GET ) )
                save_char_obj( ch );
        }
    }
    else {
        /*
         * 'get ... container'
         */
        if ( !str_cmp( arg2, "todo" ) || !str_prefix( "todo.", arg2 ) ) {
            send_to_char( "No puedes hacer eso.\r\n", ch );
            return;
        }
        if ( ( container = get_obj_here( ch, arg2 ) ) == NULL ) {
            act( AT_PLAIN, "No veo $T por aquí.", ch, NULL, arg2, TO_CHAR );
            return;
        }
        switch ( container->item_type ) {
            default:
                if ( !IS_OBJ_STAT( container, ITEM_COVERING ) ) {
                    send_to_char( "Eso no es un contenedor.\r\n", ch );
                    return;
                }
                if ( ch->carry_weight + container->weight > can_carry_w( ch ) ) {
                    send_to_char( "Es demasiado pesado para ti.\r\n", ch );
                    return;
                }
                break;
            case ITEM_CONTAINER:
            case ITEM_CORPSE_NPC:
            case ITEM_KEYRING:
            case ITEM_QUIVER:
                break;
            case ITEM_CORPSE_PC:
                {
                    char                    name[MIL];
                    CHAR_DATA              *gch;
                    char                   *pd;

                    if ( IS_NPC( ch ) ) {
                        send_to_char( "No puedes hacer eso.\r\n", ch );
                        return;
                    }
                    pd = container->short_descr;
                    pd = one_argument( pd, name );
                    pd = one_argument( pd, name );
                    pd = one_argument( pd, name );
                    pd = one_argument( pd, name );
                    if ( IS_OBJ_STAT( container, ITEM_CLANCORPSE ) && !IS_NPC( ch )
                         && ( get_timer( ch, TIMER_PKILLED ) > 0 ) && str_cmp( name, ch->name ) ) {
                        send_to_char( "Todavía no puedes mirar en ese cuerpo.\r\n", ch );
                        return;
                    }
                    /*
                     * Killer/owner loot only if die to pkill blow --Blod
                     */
                    /*
                     * Added check for immortal so IMMS can get things out of
                     * * corpses --Shaddai
                     */
                    if ( IS_OBJ_STAT( container, ITEM_CLANCORPSE ) && !IS_NPC( ch )
                         && !IS_IMMORTAL( ch ) && VLD_STR( container->action_desc )
                         && str_cmp( name, ch->name )
                         && str_cmp( container->action_desc, ch->name ) ) {
                        send_to_char( "Tu no le has dado el golpe de muerte a este cuerpo.\r\n",
                                      ch );
                        return;
                    }
                    if ( IS_OBJ_STAT( container, ITEM_CLANCORPSE ) && !IS_NPC( ch )
                         && str_cmp( name, ch->name ) && container->value[5] >= 3 ) {
                        send_to_char
                            ( "Este cuerpo ya no puede ser saqueado.\r\n",
                              ch );
                        return;
                    }
                    if ( IS_OBJ_STAT( container, ITEM_CLANCORPSE ) && !IS_NPC( ch )
                         && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
                         && container->value[4] - ch->level < 6
                         && container->value[4] - ch->level > -6 )
                        break;
                    if ( str_cmp( name, ch->name ) && !IS_IMMORTAL( ch ) ) {
                        bool                    fGroup;

                        fGroup = FALSE;
                        for ( gch = first_char; gch; gch = gch->next ) {
                            if ( !IS_NPC( gch ) && is_same_group( ch, gch )
                                 && !str_cmp( name, gch->name ) ) {
                                fGroup = TRUE;
                                break;
                            }
                        }
                        if ( !fGroup && !IS_PKILL( ch ) ) {
                            send_to_char( "Ese es el cuerpo de otro jugador.\r\n", ch );
                            return;
                        }
                    }
                }
        }
        if ( !IS_OBJ_STAT( container, ITEM_COVERING )
             && IS_SET( container->value[1], CONT_CLOSED ) ) {
            act( AT_PLAIN, "Primero necesitas abrir $d.", ch, NULL, container->name, TO_CHAR );
            return;
        }
        if ( number <= 1 && str_cmp( arg1, "todo" ) && str_prefix( "todo.", arg1 ) ) {
            /*
             * 'get obj container'
             */
            obj = get_obj_list( ch, arg1, container->first_content );
            if ( !obj ) {
                act( AT_PLAIN,
                     IS_OBJ_STAT( container,
                                  ITEM_COVERING ) ? "No veo nada como eso debajo de $T." :
                     "No veo nada como eso en $T.", ch, NULL, arg2, TO_CHAR );
                return;
            }
            separate_obj( obj );
            get_obj( ch, obj, container );
            /*
             * Oops no wonder corpses were duping oopsie did I do that
             * * --Shaddai
             */
            if ( container->item_type == ITEM_CORPSE_PC )
                write_corpses( NULL, container->short_descr + 14, NULL );

            check_for_trap( ch, container, TRAP_GET );
            if ( char_died( ch ) )
                return;
            if ( xIS_SET( sysdata.save_flags, SV_GET ) )
                save_char_obj( ch );
        }
        else {
            int                     cnt = 0;
            bool                    fTodo;
            char                   *chk;

            /*
             * 'get all container' or 'get all.obj container'
             */
            if ( IS_OBJ_STAT( container, ITEM_DONATION ) ) {
                send_to_char( "¡La administración de 6D frunce el ceño ante tal muestra de codicia!\r\n", ch );
                return;
            }
            if ( IS_OBJ_STAT( container, ITEM_CLANCORPSE ) && !IS_IMMORTAL( ch ) && !IS_NPC( ch )
                 && str_cmp( ch->name, container->name + 7 ) ) {
                send_to_char( "La administración de 6D frunce el ceño ante tal muestra de codicia desenfrenada!\r\n", ch );
                return;
            }
            if ( !str_cmp( arg1, "todo" ) )
                fTodo = TRUE;
            else
                fTodo = FALSE;
            if ( number > 1 )
                chk = arg1;
            else
                chk = &arg1[4];
            found = FALSE;
            for ( obj = container->first_content; obj; obj = obj_next ) {
                obj_next = obj->next_content;
                if ( ( fTodo || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj ) ) {
                    found = TRUE;
                    if ( number && ( cnt + obj->count ) > number )
                        split_obj( obj, number - cnt );
                    cnt += obj->count;
                    get_obj( ch, obj, container );
                    if ( char_died( ch ) || ch->carry_number >= can_carry_n( ch )
                         || ch->carry_weight >= can_carry_w( ch ) || ( number && cnt >= number ) ) {

                        if ( container->item_type == ITEM_CORPSE_PC )
                            write_corpses( NULL, container->short_descr + 14, NULL );
                        if ( container->item_type == ITEM_CORPSE_PC )
                            write_corpses( NULL, container->short_descr + 14, NULL );
                        return;
                    }
                }
            }
            if ( !found ) {
                if ( fTodo ) {
                    if ( container->item_type == ITEM_KEYRING
                         && !IS_OBJ_STAT( container, ITEM_COVERING ) )
                        act( AT_PLAIN, "$T no tiene cerradura.", ch, NULL, arg2, TO_CHAR );
                    else
                        act( AT_PLAIN,
                             IS_OBJ_STAT( container,
                                          ITEM_COVERING ) ? "No veo nada como eso debajo de $T." :
                             "No veo nada como eso en $T.", ch, NULL, arg2, TO_CHAR );
                }
                else {
                    if ( container->item_type == ITEM_KEYRING
                         && !IS_OBJ_STAT( container, ITEM_COVERING ) )
                        act( AT_PLAIN, "$T no se abre con esa llave.", ch, NULL, arg2, TO_CHAR );
                    else
                        act( AT_PLAIN,
                             IS_OBJ_STAT( container,
                                          ITEM_COVERING ) ?
                             "No veo nada como eso debajo de $T." :
                             "No veo nada como eso en $T.", ch, NULL, arg2, TO_CHAR );
                }
            }
            else
                check_for_trap( ch, container, TRAP_GET );
            /*
             * Oops no wonder corpses were duping oopsie did I do that
             * * --Shaddai
             */
            if ( container->item_type == ITEM_CORPSE_PC )
                write_corpses( NULL, container->short_descr + 14, NULL );
            if ( char_died( ch ) )
                return;
            if ( found && xIS_SET( sysdata.save_flags, SV_GET ) )
                save_char_obj( ch );
        }
    }
    return;
}

void do_put( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    OBJ_DATA               *container;
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;
    short                   count;
    int                     number;
    bool                    save_char = FALSE;

    argument = one_argument( argument, arg1 );
    if ( is_number( arg1 ) ) {
        number = atoi( arg1 );
        if ( number < 1 ) {
            send_to_char( "Eso fue fácil...\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg1 );
    }
    else
        number = 0;
    argument = one_argument( argument, arg2 );
    /*
     * munch optional words
     */

    if ( ( !str_cmp( arg2, "into" ) || !str_cmp( arg2, "inside" ) || !str_cmp( arg2, "in" )
           || !str_cmp( arg2, "under" ) || !str_cmp( arg2, "onto" ) || !str_cmp( arg2, "on" ) )
         && argument[0] != '\0' )
        argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "¿Poner qué en qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( !str_cmp( arg2, "todo" ) || !str_prefix( "todo.", arg2 ) ) {
        send_to_char( "No puedes hacer eso.\r\n", ch );
        return;
    }

    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL ) {
        act( AT_PLAIN, "No veo $T por aquí.", ch, NULL, arg2, TO_CHAR );
        return;
    }

    if ( !container->carried_by && xIS_SET( sysdata.save_flags, SV_PUT ) )
        save_char = TRUE;

    if ( IS_OBJ_STAT( container, ITEM_COVERING ) ) {
        if ( ch->carry_weight + container->weight > can_carry_w( ch ) ) {
            send_to_char( "Es demasiado pesado para ti.\r\n", ch );
            return;
        }
    }
    else {
        if ( container->item_type != ITEM_CONTAINER && container->item_type != ITEM_KEYRING
             && container->item_type != ITEM_QUIVER && container->item_type != ITEM_CORPSE_NPC ) {
            send_to_char( "Eso no es un contenedor.\r\n", ch );
            return;
        }

        if ( IS_SET( container->value[1], CONT_CLOSED ) ) {
            act( AT_PLAIN, "Primero necesitas abrir $d.", ch, NULL, container->name, TO_CHAR );
            return;
        }
    }

    if ( number <= 1 && str_cmp( arg1, "todo" ) && str_prefix( "todo.", arg1 ) ) {
        /*
         * 'put obj container'
         */
        if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
            send_to_char( "No tienes ese objeto.\r\n", ch );
            return;
        }

        if ( obj == container ) {
            send_to_char( "No puedes poner un objeto dentro de si mismo.\r\n", ch );
            return;
        }

        if ( !can_drop_obj( ch, obj ) ) {
            send_to_char( "No puedes dejar ir ese objeto.\r\n", ch );
            return;
        }

        if ( container->item_type == ITEM_KEYRING && obj->item_type != ITEM_KEY ) {
            send_to_char( "Eso no es una llave.\r\n", ch );
            return;
        }

        if ( container->item_type == ITEM_QUIVER && obj->item_type != ITEM_PROJECTILE ) {
            send_to_char( "Eso no es un proyectil.\r\n", ch );
            return;
        }

        if ( ( IS_OBJ_STAT( container, ITEM_COVERING )
               && ( get_obj_weight( obj, FALSE ) / obj->count ) >
               ( ( get_obj_weight( container, FALSE ) / container->count ) -
                 container->weight ) ) ) {
            send_to_char( "No cabrá.\r\n", ch );
            return;
        }

        if ( obj->pIndexData->vnum == OBJ_VNUM_OPORTAL ) {
            send_to_char( "&BLa magia del portal no te lo permite.\r\n", ch );
            return;
        }

        if ( obj->item_type == ITEM_TRASH && xIS_SET( container->extra_flags, ITEM_CLANCONTAINER ) ) {
            act( AT_CYAN, "No puedes poner basura en un contenedor de clan.", ch, NULL, NULL,
                 TO_CHAR );
            return;
        }

        /*
         * note use of get_obj_weight
         */
        if ( ( ( get_real_obj_weight( obj ) / obj->count ) +
               ( get_real_obj_weight( container ) / container->count ) > container->value[0] )
             && container->item_type != ITEM_CORPSE_NPC ) {
            send_to_char( "No cabrá.\r\n", ch );
            return;
        }

        if ( container->item_type == ITEM_CORPSE_NPC && container->value[5] == 1 ) {    /* Skeleton
                                                                                         */
            send_to_char( "Eso no es un contenedor.\r\n", ch );
            return;
        }

        if ( ( container->item_type == ITEM_CORPSE_NPC ) && ( obj->weight > container->value[2] )
             && !IS_IMMORTAL( ch ) ) {
            send_to_char( "No cabrá.\r\n", ch );
            return;
        }

        separate_obj( obj );
        separate_obj( container );
        obj_from_char( obj );
        obj = obj_to_obj( obj, container );
        check_for_trap( ch, container, TRAP_PUT );
        if ( char_died( ch ) )
            return;
        count = obj->count;
        obj->count = 1;
        if ( container->item_type == ITEM_KEYRING && !IS_OBJ_STAT( container, ITEM_COVERING ) ) {
            act( AT_ACTION, "$n pone $p en $P.", ch, obj, container, TO_ROOM );
            act( AT_ACTION, "Pones  $p en $P.", ch, obj, container, TO_CHAR );
        }
        else {
            act( AT_ACTION,
                 IS_OBJ_STAT( container,
                              ITEM_COVERING ) ? "$n oculta $p en $P." : "$n pone $p en $P.", ch,
                 obj, container, TO_ROOM );
            act( AT_ACTION,
                 IS_OBJ_STAT( container,
                              ITEM_COVERING ) ? "Ocultas $p en $P." : "Pones $p en $P.", ch,
                 obj, container, TO_CHAR );
        }
        obj->count = count;

        /*
         * Oops no wonder corpses were duping oopsie did I do that
         * * --Shaddai
         */
        if ( container->item_type == ITEM_CORPSE_PC )
            write_corpses( NULL, container->short_descr + 14, NULL );

        if ( save_char )
            save_char_obj( ch );
        /*
         * Clan storeroom check
         */
        if ( IS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM )
             && container->carried_by == NULL ) {
            save_clan_storeroom( ch );
        }
    }
    else {
        bool                    found = FALSE;
        bool                    full = FALSE;
        int                     cnt = 0;
        bool                    fTodo;
        char                   *chk;

        if ( !str_cmp( arg1, "Todo" ) )
            fTodo = TRUE;
        else
            fTodo = FALSE;
        if ( number > 1 )
            chk = arg1;
        else
            chk = &arg1[4];

        separate_obj( container );
        /*
         * 'put all container' or 'put all.obj container'
         */
        for ( obj = ch->first_carrying; obj; obj = obj_next ) {
            obj_next = obj->next_content;

            if ( ( fTodo || nifty_is_name( chk, obj->name ) )
                 && can_see_obj( ch, obj )
                 && obj->wear_loc == WEAR_NONE
                 && obj != container
                 && can_drop_obj( ch, obj ) && ( container->item_type != ITEM_KEYRING
                                                 || obj->item_type == ITEM_KEY )
                 && ( container->item_type != ITEM_QUIVER || obj->item_type == ITEM_PROJECTILE ) ) {
                if ( ( get_obj_weight( obj, TRUE ) + get_obj_weight( container, TRUE ) ) >
                     container->value[0] ) {
                    full = TRUE;
                    continue;
                }
                if ( container->item_type == ITEM_CORPSE_NPC && container->value[5] == 1 ) {    /* Skeleton
                                                                                                 */
                    send_to_char( "Eso no es un contenedor.\r\n", ch );
                    return;
                }

                if ( obj->pIndexData->vnum == OBJ_VNUM_OPORTAL ) {
                    send_to_char( "&BLa magia del portal no te lo permite.\r\n", ch );
                    return;
                }

                if ( number && ( cnt + obj->count ) > number )
                    split_obj( obj, number - cnt );
                cnt += obj->count;
                obj_from_char( obj );

                if ( container->item_type == ITEM_KEYRING ) {
                    act( AT_ACTION, "$n pone $p en $P.", ch, obj, container, TO_ROOM );
                    act( AT_ACTION, "Pones $p en $P.", ch, obj, container, TO_CHAR );
                }
                else {
                    act( AT_ACTION, "$n pone $p en $P.", ch, obj, container, TO_ROOM );
                    act( AT_ACTION, "Pones $p en $P.", ch, obj, container, TO_CHAR );
                }
                obj = obj_to_obj( obj, container );
                found = TRUE;

                check_for_trap( ch, container, TRAP_PUT );
                if ( char_died( ch ) )
                    return;
                if ( number && cnt >= number )
                    break;
            }
        }

        /*
         * Don't bother to save anything if nothing was dropped   -Thoric
         */
        if ( !found ) {
            if ( !full ) {
                if ( fTodo )
                    act( AT_PLAIN, "No estás cargando nada.", ch, NULL, NULL, TO_CHAR );
                else
                    act( AT_PLAIN, "No estás cargando ningún $T.", ch, NULL, chk, TO_CHAR );
            }
            else {
                if ( fTodo )
                    act( AT_PLAIN, "$p no tiene tanta capacidad.", ch, container,
                         NULL, TO_CHAR );
                else
                    act( AT_PLAIN, "$p no tiene espacio para guardar $T.", ch, container, chk, TO_CHAR );
            }
            return;
        }

        if ( save_char )
            save_char_obj( ch );
        /*
         * Oops no wonder corpses were duping oopsie did I do that
         * * --Shaddai
         */
        if ( container->item_type == ITEM_CORPSE_PC )
            write_corpses( NULL, container->short_descr + 14, NULL );

        /*
         * Clan storeroom check
         */

        if ( IS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM )
             && container->carried_by == NULL ) {
            save_clan_storeroom( ch );
        }
    }

    return;
}

void do_drop( CHAR_DATA *ch, char *argument )
{
    ROOM_INDEX_DATA        *room;
    char                    arg[MIL];
    OBJ_DATA               *obj;
    OBJ_DATA               *obj_next;
    bool                    found;
    int                     number;
    char                    buf[MSL];

    room = ch->in_room;
    argument = one_argument( argument, arg );
    if ( is_number( arg ) ) {
        number = atoi( arg );
        if ( number < 1 ) {
            send_to_char( "That was easy...\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg );
    }
    else
        number = 0;
    if ( arg[0] == '\0' ) {
        if ( !IS_NPC( ch ) )
            if ( xIS_SET( ch->act, PLR_OBJID ) )
                send_to_char( "Drop id.object?\r\n", ch );
            else {
                send_to_char( "Drop what?\r\n", ch );
            }
        return;
    }
    if ( ms_find_obj( ch ) )
        return;
    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_LITTERBUG ) ) {
        set_char_color( AT_YELLOW, ch );
        send_to_char( "La policía del medio ambiente te impide dejar nada aquí...\r\n",
                      ch );
        return;
    }
    if ( IS_SET( ch->in_room->room_flags, ROOM_NODROP ) && ch != supermob && !IS_IMMORTAL( ch ) ) {
        set_char_color( AT_MAGIC, ch );
        send_to_char( "¡Una fuerza mágica te detiene!\r\n", ch );
        set_char_color( AT_TELL, ch );
        send_to_char( "Alguien te cuenta, '¡No tires nada aquí!'\r\n", ch );
        return;
    }

    if ( number > 0 ) {
        int                     type;

        /*
         * 'drop NNNN coins'
         */

        if ( ( type = get_currency_type( arg ) ) ) {
            if ( GET_MONEY( ch, type ) < number ) {
                ch_printf( ch, "No tienes tantas monedas de %s.\r\n", curr_types[type] );
                return;
            }

            GET_MONEY( ch, type ) -= number;

            for ( obj = room->first_content; obj; obj = obj_next ) {
                obj_next = obj->next_content;

                switch ( obj->pIndexData->vnum ) {
                    default:
                        if ( obj->value[2] == type ) {
                            number += obj->value[0];
                            extract_obj( obj );
                        }
                        break;

                    case OBJ_VNUM_MONEY_ONE:
                        if ( obj->value[2] == type ) {
                            number += 1;
                            extract_obj( obj );
                        }
                        break;
                }
            }

            if ( number < 100 ) {
                act( AT_ACTION, "$n deja algunas monedas de $T en el suelo.", ch, NULL, curr_types[type], TO_ROOM );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR,
                            "Dejas en el suelo algunas monedas de %s.", curr_types[type] );
            }
            else if ( number < 500 ) {
                act( AT_ACTION, "$n deja muchas monedas de $T.", ch, NULL, curr_types[type],
                     TO_ROOM );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR, "Dejas muchas monedas de %s.",
                            curr_types[type] );
            }
            else if ( number < 2000 ) {
                act( AT_ACTION, "$n deja una pila de monedas de $T.", ch, NULL, curr_types[type],
                     TO_ROOM );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR, "Dejas una pila de monedas de %s.",
                            curr_types[type] );
            }
            else if ( number < 10000 ) {
                act( AT_ACTION, "$n deja muchas monedas de $T.", ch, NULL, curr_types[type],
                     TO_ROOM );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR, "dejas muchas monedas de %s.",
                            curr_types[type] );
            }
            else if ( number < 100000 ) {
                act( AT_ACTION, "$n deja un montón de monedas de $T!", ch, NULL, curr_types[type],
                     TO_ROOM );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR,
                            "Dejas una enorme cantidad de monedas de %s!", curr_types[type] );
            }
            else if ( number < 1000000 ) {
                act( AT_ACTION, "$n deja un enorme montón de monedas de $T!", ch, NULL,
                     curr_types[type], TO_ROOM );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR,
                            "Dejas una enorme cantidad de monedas de %s!", curr_types[type] );
            }
            else {
                act( AT_ACTION,
                     "$n vacía un bolsillo extradimensional y deja más de 1000000 de monedas de $T!!",
                     ch, NULL, curr_types[type], TO_ROOM );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR,
                            "Vacías un bolsillo extradimensional y dejas más de 1000000 de monedas de %s!!",
                            curr_types[type] );
            }

            obj = create_money( number, type );
/* save_house_by_vnum(ch->in_room->vnum); */
            obj_to_room( obj, room );
            if ( xIS_SET( sysdata.save_flags, SV_DROP ) )
                save_char_obj( ch );
            return;
        }
    }

    if ( number <= 1 && str_cmp( arg, "todo" ) && str_prefix( "todo.", arg ) ) {
        /*
         * 'drop obj'
         */
        if ( ( obj = get_obj_carry( ch, arg ) ) == NULL ) {
            send_to_char( "No tienes ese objeto.\r\n", ch );
            return;
        }

        if ( !can_drop_obj( ch, obj ) ) {
            send_to_char( "No puedes dejar ir ese objeto.\r\n", ch );
            return;
        }

        if ( !IS_NPC( ch ) ) {
            if ( obj->pIndexData->vnum == 41002 && obj->item_type == ITEM_SABOTAGE ) {
                CITY_DATA              *city = NULL;
                SIEGE_DATA             *siege = NULL;
                SIEGE_DATA             *siege_next = NULL;
                bool                    foundsiege = FALSE;

                for ( siege = first_siege; siege; siege = siege_next ) {
                    siege_next = siege->next;
                    if ( siege )
                        foundsiege = TRUE;
                }

                if ( foundsiege == TRUE ) {
                    send_to_char( "No se pueden endurecer las defensas de la ciudad mientras que dure el asedio.\r\n",
                                  ch );
                    return;
                }

                // check to see if it is dropped in a homeland
                if ( ch->in_room && !str_cmp( ch->in_room->area->filename, "paleon.are" ) ) {
                    city = get_city( "Ciudad Paleon" );
                }
                else if ( ch->in_room && !str_cmp( ch->in_room->area->filename, "dakar.are" ) ) {
                    city = get_city( "Ciudad Dakar" );
                }
                else if ( ch->in_room && !str_cmp( ch->in_room->area->filename, "forbidden.are" ) ) {
                    city = get_city( "Ciudad Prohibida" );
                }
                else {
                    send_to_char( "necesitas estar en una ciudad para endurecer sus defensas.\r\n", ch );
                    return;
                }
                // if so check to see if max hardened already there for the city.
                if ( city ) {
                    if ( city->hardened < 10 ) {
                        city->hardened += 1;
                        save_city( city );
                        send_to_char( "¡Tu defensa artesanal se suma a las defensas de la ciudad!\r\n", ch );
                        separate_obj( obj );
                        obj_from_char( obj );
                        extract_obj( obj );
                        return;
                    }
                    if ( city->hardened > 9 ) {
                        send_to_char
                            ( "Las defensas ya son todo lo duras que pueden llegar a ser.\r\n",
                              ch );
                        return;
                    }

                }
                // if not harden city defenses and make object purge
            }
        }

        if ( !IS_NPC( ch ) && IS_OBJ_STAT( obj, ITEM_CLANOBJECT ) ) {
            snprintf( buf, MSL,
                      "%s dropping %s clan object! make sure he doesn't allow another to pick up.",
                      ch->name, obj->short_descr );
            log_string( buf );
        }

        separate_obj( obj );
        act( AT_ACTION, "$n deja $p en el suelo.", ch, obj, NULL, TO_ROOM );
        act( AT_ACTION, "Dejas $p en el suelo.", ch, obj, NULL, TO_CHAR );

        obj_from_char( obj );
        obj = obj_to_room( obj, ch->in_room );
        oprog_drop_trigger( ch, obj );                 /* mudprogs */

        if ( char_died( ch ) || obj_extracted( obj ) )
            return;

        /*
         * Clan storeroom saving
         */
        if ( IS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) ) {
            save_clan_storeroom( ch );
        }

    }
    else {
        int                     cnt = 0;
        char                   *chk;
        bool                    fTodo;

        if ( !str_cmp( arg, "todo" ) )
            fTodo = TRUE;
        else
            fTodo = FALSE;
        if ( number > 1 )
            chk = arg;
        else
            chk = &arg[4];
        /*
         * 'drop all' or 'drop all.obj'
         */
        if ( IS_SET( ch->in_room->room_flags, ROOM_NODROPALL )
             || IS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) ) {
            send_to_char( "No parece que puedas hacer eso aquí...\r\n", ch );
            return;
        }
        found = FALSE;
        for ( obj = ch->first_carrying; obj; obj = obj_next ) {
            obj_next = obj->next_content;

            if ( ( fTodo || nifty_is_name( chk, obj->name ) ) && can_see_obj( ch, obj )
                 && obj->wear_loc == WEAR_NONE && can_drop_obj( ch, obj ) ) {
                found = TRUE;
                if ( HAS_PROG( obj->pIndexData, DROP_PROG ) && obj->count > 1 ) {
                    ++cnt;
                    separate_obj( obj );
                    obj_from_char( obj );
                    if ( !obj_next )
                        obj_next = ch->first_carrying;
                }
                else {
                    if ( number && ( cnt + obj->count ) > number )
                        split_obj( obj, number - cnt );
                    cnt += obj->count;
                    obj_from_char( obj );
                }
                act( AT_ACTION, "$n deja $p en el suelo.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Dejas $p en el suelo.", ch, obj, NULL, TO_CHAR );
                obj = obj_to_room( obj, ch->in_room );
                oprog_drop_trigger( ch, obj );         /* mudprogs */
                if ( char_died( ch ) )
                    return;
                if ( number && cnt >= number )
                    break;
            }
        }

        if ( !found ) {
            if ( fTodo )
                act( AT_PLAIN, "No estás cargando nada.", ch, NULL, NULL, TO_CHAR );
            else
                act( AT_PLAIN, "No estás cargando $T.", ch, NULL, chk, TO_CHAR );
        }
    }
/* save_house_by_vnum(ch->in_room->vnum); */
    if ( xIS_SET( sysdata.save_flags, SV_DROP ) )
        save_char_obj( ch );                           /* duping protector */
    return;
}

void do_give( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL],
                            arg2[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    int                     number,
                            type;
    char                    buf[MSL];

    if ( !ch )
        return;
    argument = one_argument( argument, arg1 );
    if ( is_number( arg1 ) ) {
        number = atoi( arg1 );
        if ( number < 1 ) {
            send_to_char( "Eso fue fácil...\r\n", ch );
            return;
        }
        argument = one_argument( argument, arg1 );
    }
    else
        number = 0;
    argument = one_argument( argument, arg2 );
    /*
     * munch optional words
     */
    if ( ( !str_cmp( arg2, "to" ) ) )
        argument = one_argument( argument, arg2 );
    if ( !VLD_STR( arg1 ) || !VLD_STR( arg2 ) ) {
        send_to_char( "¿Dar qué a quién?\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "todo" ) || !str_prefix( "todo.", arg2 ) ) {
        send_to_char( "No puedes hacer eso.\r\n", ch );
        return;
    }
    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "¿De verdad quieres darte un objeto a ti mismo?\r\n",
                      ch );
        return;
    }
    if ( ( type = get_currency_type( arg1 ) )
         && ( type == CURR_GOLD || type == CURR_SILVER || type == CURR_BRONZE
              || type == CURR_COPPER ) ) {
        if ( number <= 0 ) {
            send_to_char( "No puedes hacer eso.\r\n", ch );
            return;
        }
        if ( GET_MONEY( ch, type ) < number ) {
            ch_printf( ch, "Eres muy generoso, pero no tienes suficiente %s.\r\n",
                       curr_types[type] );
            return;
        }
        if ( ( victim->carry_weight ) >= can_carry_w( victim ) ) {
            act( AT_ACTION, "$N no puede cargar más objetos.", ch, NULL, victim, TO_CHAR );
            return;
        }

        GET_MONEY( ch, type ) -= number;
        GET_MONEY( victim, type ) += number;
        act_printf( AT_ACTION, ch, NULL, victim, TO_VICT, "$n te da %d moneda%s de %s.", number, number == 1 ? "" : "s",
                       curr_types[type]);
        act_printf( AT_ACTION, ch, NULL, victim, TO_NOTVICT, "$n da a $N algunas monedas de %s.",
                    curr_types[type] );
        act_printf( AT_ACTION, ch, NULL, victim, TO_CHAR, "Das a $N %d moneda%s de %s.", number, number == 1 ? "" : "s",                    curr_types[type]);

        /*
         * We only want this to work with the right currencty
         */
        if ( type == DEFAULT_CURR ) {
            mprog_bribe_trigger( victim, ch, number );
        }
        if ( xIS_SET( sysdata.save_flags, SV_GIVE ) && !char_died( ch ) )
            save_char_obj( ch );
        if ( xIS_SET( sysdata.save_flags, SV_RECEIVE ) && !char_died( victim ) )
            save_char_obj( victim );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "No tienes ese objeto.\r\n", ch );
        return;
    }
    separate_obj( obj );
    if ( obj->wear_loc != WEAR_NONE ) {
        send_to_char( "Primero debes desvestirlo.\r\n", ch );
        return;
    }
    if ( !can_drop_obj( ch, obj ) ) {
        send_to_char( "No puedes dejar ir ese objeto.\r\n", ch );
        return;
    }
    if ( !in_arena( victim ) ) {
        if ( victim->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( victim ) ) {
            act( AT_PLAIN, "$N tiene sus manos llenas.", ch, NULL, victim, TO_CHAR );
            if ( IS_NPC( ch ) ) {
                act( AT_SAY,
                     "$n says, 'Veo que tienes las manos llenas, tendré que dejar esto en el suelo.'",
                     ch, NULL, victim, TO_ROOM );
                obj_from_char( obj );
                obj_to_room( obj, victim->in_room );
            }
            return;
        }
        if ( victim->carry_weight + ( get_obj_weight( obj, FALSE ) / obj->count ) >
             can_carry_w( victim ) ) {
            act( AT_PLAIN, "$N no puede cargar con tanto peso.", ch, NULL, victim, TO_CHAR );
            if ( IS_NPC( ch ) ) {
                act( AT_SAY,
                     "$n says, 'Veo que tienes las manos llenas, tendré que dejar esto en el suelo.'",
                     ch, NULL, victim, TO_ROOM );
                obj_from_char( obj );
                obj_to_room( obj, victim->in_room );
            }
            return;
        }
    }
    if ( !can_see_obj( victim, obj ) ) {
        act( AT_PLAIN, "$N no puede verlo.", ch, NULL, victim, TO_CHAR );
        return;
    }
    if ( !IS_NPC( ch ) && IS_OBJ_STAT( obj, ITEM_CLANOBJECT ) ) {
        snprintf( buf, MSL, "%s ha intentado dar a %s %s, un objeto de clan!", ch->name, victim->name,
                  obj->short_descr );
        log_string( buf );
        act( AT_TELL, "No puedes dar objetos de clan.", ch, NULL, NULL, TO_CHAR );
        return;
    }

    if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) && !can_take_proto( victim ) ) {
        act( AT_PLAIN, "¡No puedes dar eso a $N!", ch, NULL, victim, TO_CHAR );
        return;
    }
    obj_from_char( obj );
    act( AT_ACTION, "$n da $p a $N.", ch, obj, victim, TO_NOTVICT );
    act( AT_ACTION, "$n te da $p.", ch, obj, victim, TO_VICT );
    act( AT_ACTION, "Das $p a $N.", ch, obj, victim, TO_CHAR );
    obj = obj_to_char( obj, victim );
    if ( obj->owner && str_cmp( victim->name, obj->owner ) && obj->item_type == ITEM_TRASH ) {
        act( AT_RED, "$p no te pertenece. Sientes como quema tus manos.", victim,
             obj, NULL, TO_CHAR );
        obj_from_char( obj );
        obj_to_room( obj, victim->in_room );
        victim->hit = victim->hit - 1;
        act( AT_ACTION, "$p cae de las manos de $n.", victim, obj, NULL, TO_ROOM );
        act( AT_ACTION, "$p cae de tus manos.", victim, obj, NULL, TO_CHAR );
        return;
    }

    mprog_give_trigger( victim, ch, obj );

    snprintf( buf, MSL, "%-20s has given %s %s.", ch->name, victim->name, obj->short_descr );
    append_to_file( VLOG_FILE, buf );

    if ( !IS_NPC( ch ) && !char_died( ch ) )
        save_char_obj( ch );
    if ( !IS_NPC( victim ) && !char_died( victim ) )
        save_char_obj( victim );
    return;
}

/*
 * Damage an object.      -Thoric
 * Affect player's AC if necessary.
 * Make object into scraps if necessary.
 * Send message about damaged object.
 */
// Volk - this is a bit dirty, let's tidy it up.
obj_ret damage_obj( OBJ_DATA *obj )
{
    CHAR_DATA              *ch,
                           *vch;
    obj_ret                 objcode;

    ch = obj->carried_by;
    objcode = rNONE;

    separate_obj( obj );

    if ( ( ch && in_arena( ch ) ) || obj->item_type == ITEM_LIGHT )
        return objcode;

    if ( ch->switched )                                // checks added to stop mud from
        // crashing when staff are
        // switched
        return rNONE;

    if ( ch && !IS_NPC( ch ) && ( !IS_PKILL( ch ) || !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) ) )
        act( AT_OBJECT, "($p se ha dañado)", ch, obj, NULL, TO_CHAR );
    else if ( obj->in_room && ( vch = obj->in_room->first_person ) != NULL ) {
        act( AT_OBJECT, "($p se ha dañado)", vch, obj, NULL, TO_ROOM );
        act( AT_OBJECT, "($p se ha dañado)", vch, obj, NULL, TO_CHAR );
    }

    oprog_damage_trigger( ch, obj );
    if ( obj_extracted( obj ) )
        return global_objcode;

    switch ( obj->item_type ) {
        default:
            if ( ch && IS_PKILL( ch ) && obj->wear_loc != -1 )
                break;
            make_scraps( obj );
            objcode = rOBJ_SCRAPPED;
            break;

        case ITEM_CONTAINER:
        case ITEM_KEYRING:
        case ITEM_QUIVER:
            if ( ch && IS_PKILL( ch ) && obj->wear_loc != -1 ) {
                if ( --obj->value[3] <= 0 )
                    obj->value[3] = 1;
                break;
            }
            if ( --obj->value[3] <= 0 ) {
                make_scraps( obj );
                objcode = rOBJ_SCRAPPED;
            }
            break;

        case ITEM_LIGHT:
            if ( ch && IS_PKILL( ch ) && obj->wear_loc != -1 ) {
                if ( --obj->value[0] <= 0 )
                    obj->value[0] = 1;
                break;
            }
            if ( --obj->value[0] <= 0 ) {
                make_scraps( obj );
                objcode = rOBJ_SCRAPPED;
            }
            break;

        case ITEM_ARMOR:
            if ( ch && IS_PKILL( ch ) && obj->wear_loc != -1 ) {
                if ( --obj->value[0] <= 0 )
                    obj->value[0] = 1;
                break;
            }
            if ( ch && obj->value[0] >= 1 )
                ch->armor += apply_ac( obj, obj->wear_loc );
            if ( --obj->value[0] <= 0 ) {
                make_scraps( obj );
                objcode = rOBJ_SCRAPPED;
                ch->armor -= apply_ac( obj, obj->wear_loc );
            }
            else if ( ch && obj->value[0] >= 1 )
                ch->armor -= apply_ac( obj, obj->wear_loc );
            break;

        case ITEM_WEAPON:
            if ( ch && IS_PKILL( ch ) && obj->wear_loc != -1 ) {
                if ( --obj->value[0] <= 0 )
                    obj->value[0] = 1;
                break;
            }
            if ( --obj->value[0] <= 0 ) {
                make_scraps( obj );
                objcode = rOBJ_SCRAPPED;
            }
            break;
    }

    if ( ch != NULL )
        save_char_obj( ch );                           // Stop scrap duping - Samson
    // 1-2-00

    return objcode;
}

/*
 * Remove an object.
 */
bool remove_obj( CHAR_DATA *ch, int iWear, bool fReplace )
{
    OBJ_DATA               *obj,
                           *tmpobj;

    if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
        return TRUE;

    if ( !fReplace && ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ) {
        act( AT_PLAIN, "$d: no puedes cargar con tantos objetos.", ch, NULL, obj->name, TO_CHAR );
        return FALSE;
    }

    if ( IS_AFFECTED( ch, AFF_GRAFT ) && obj == get_eq_char( ch, WEAR_WIELD ) ) {
        send_to_char( "¡No puedes quitarte eso, está aderido a tu mano!\r\n", ch );
        return FALSE;
    }

    if ( obj == get_eq_char( ch, WEAR_SHIELD ) && ch->fighting ) {
        send_to_char( "No puedes coger el escudo mientras estés luchando.\r\n", ch );
        return FALSE;
    }

    if ( !fReplace )
        return FALSE;

    if ( IS_OBJ_STAT( obj, ITEM_NOREMOVE ) ) {
        act( AT_PLAIN, "No puedes guardar $p.", ch, obj, NULL, TO_CHAR );
        return FALSE;
    }

    if ( obj == get_eq_char( ch, WEAR_WIELD )
         && ( tmpobj = get_eq_char( ch, WEAR_DUAL_WIELD ) ) != NULL )
        tmpobj->wear_loc = WEAR_WIELD;

    unequip_char( ch, obj );

    act( AT_ACTION, "$n deja de usar $p.", ch, obj, NULL, TO_ROOM );
    act( AT_ACTION, "Dejas de usar $p.", ch, obj, NULL, TO_CHAR );
    oprog_remove_trigger( ch, obj );

/* Now, check if it was an instrument and if so, stop any music playing */
    if ( obj->item_type == ITEM_INSTRUMENT )
        bard_stop_playing( ch, obj );

    return TRUE;
}

/*
 * See if char could be capable of dual-wielding  -Thoric
 */
bool could_dual( CHAR_DATA *ch )
{
    if ( IS_NPC( ch ) || ch->pcdata->learned[gsn_dual_wield] )
        return TRUE;

    return FALSE;
}

/*
 * Check for the ability to dual-wield under all conditions.  -Orion
 *
 * Original version by Thoric.
 */
bool can_dual( CHAR_DATA *ch )
{
    /*
     * We must assume that when they come in, they are NOT wielding something. We
     * take care of the actual value later. -Orion
     */
    bool                    wielding[2],
                            alreadyWielding = FALSE;

    wielding[0] = FALSE;
    wielding[1] = FALSE;

    /*
     * If they don't have the ability to dual-wield, why should we allow them to
     * do so? -Orion
     */
    if ( !could_dual( ch ) )
        return FALSE;

    /*
     * Get that true wielding value I mentioned earlier. If they're wielding and
     * missile wielding, we can simply return FALSE. If not, set the values. -Orion
     */
    if ( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( ch, WEAR_MISSILE_WIELD ) ) {
        send_to_char( "¡Ya estás empuñando dos armas!\r\n", ch );
        return FALSE;
    }
    else if ( get_eq_char( ch, WEAR_WIELD ) && IS_TWOHANDS( get_eq_char( ch, WEAR_WIELD ) ) ) {
        send_to_char( "¡No mientras estés empuñando un arma a dos manos!\r\n", ch );
        return FALSE;
    }
    else {
        /*
         * Wield position. -Orion
         */
        wielding[0] = get_eq_char( ch, WEAR_WIELD ) ? TRUE : FALSE;
        /*
         * Missile wield position. -Orion
         */
        wielding[1] = get_eq_char( ch, WEAR_MISSILE_WIELD ) ? TRUE : FALSE;
    }

    /*
     * Save some extra typing farther down. -Orion
     */
    if ( wielding[0] || wielding[1] )
        alreadyWielding = TRUE;

    /*
     * If wielding and dual wielding, then they can't wear another weapon. Return
     * FALSE. We can assume that dual wield will not be full if there is no wield
     * slot already filled. -Orion
     */
    else if ( get_eq_char( ch, WEAR_WIELD ) && IS_TWOHANDS( get_eq_char( ch, WEAR_WIELD ) ) ) {
        send_to_char( "¡No mientras estés emuñando un arma a dos manos!\r\n", ch );
        return FALSE;
    }

    if ( wielding[0] && get_eq_char( ch, WEAR_DUAL_WIELD ) ) {
        send_to_char( "¡Ya estás empuñando dos armas!\r\n", ch );
        return FALSE;
    }

    /*
     * If wielding or missile wielding and holding a shield, then we can return
     * FALSE. -Orion
     */
    if ( alreadyWielding && get_eq_char( ch, WEAR_SHIELD ) ) {
        send_to_char( "¡No puedes blandir dos armas mientras uses un escudo!\r\n", ch );
        return FALSE;
    }
    if ( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( ch, WEAR_MISSILE_WIELD ) ) {
        send_to_char( "¡Ya estás empuñando dos armas!\r\n", ch );
        return FALSE;
    }

    /*
     * If wielding or missile wielding and holding something, then we can return
     * FALSE. -Orion
     */
    if ( alreadyWielding && get_eq_char( ch, WEAR_HOLD ) ) {
        send_to_char
            ( "¡No puedes empuñar otra arma mientras sostengas algo en tus manos!\r\n",
              ch );
        return FALSE;
    }

    return TRUE;
}

/*
 * Check to see if there is room to wear another object on this location
 * (Layered clothing support)
 */
bool can_layer( CHAR_DATA *ch, OBJ_DATA *obj, short wear_loc )
{
    OBJ_DATA               *otmp;
    short                   bitlayers = 0;
    short                   objlayers = obj->pIndexData->layers;

    for ( otmp = ch->first_carrying; otmp; otmp = otmp->next_content ) {
        if ( otmp->wear_loc == wear_loc ) {
            if ( !otmp->pIndexData->layers ) {
                return FALSE;
            }
            else {
                bitlayers |= otmp->pIndexData->layers;
            }
        }
    }

    if ( ( bitlayers && !objlayers ) || bitlayers > objlayers )
        return FALSE;
    if ( !bitlayers || ( ( bitlayers & ~objlayers ) == bitlayers ) )
        return TRUE;
    return FALSE;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 *
 * Restructured a bit to allow for specifying body location  -Thoric
 * & Added support for layering on certain body locations
 */
void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace, short wear_bit )
{
    OBJ_DATA               *tmpobj = NULL;
    short                   bit,
                            tmp;

    separate_obj( obj );
    if ( get_trust( ch ) < obj->level ) {
        ch_printf( ch, "Debes ser nivel %d para usar este objeto.\r\n", obj->level );
        act( AT_ACTION, "$n intenta usar $p, pero no posee suficiente experiencia.", ch, obj, NULL, TO_ROOM );
        return;
    }

    if ( obj->owner && str_cmp( ch->name, obj->owner ) ) {
        act( AT_RED, "$p no te pertenece. Sientes como quema tus manos.", ch,
             obj, NULL, TO_CHAR );
        obj_from_char( obj );
        obj_to_room( obj, ch->in_room );
        ch->hit = ch->hit - 1;
        act( AT_ACTION, "$p cae de las manos de $n.", ch, obj, NULL, TO_ROOM );
        act( AT_ACTION, "$p cae de tus manos.", ch, obj, NULL, TO_CHAR );
        return;
    }

    if ( obj->glory > 1 ) {                            /* Wearing an item with glory, *
                                                        * let's check how many items they
                                                        * * * * * * * * * * can have.. */
        OBJ_DATA               *findobj,
                               *findobj_next;
        short                   count = 0;

        for ( findobj = ch->first_carrying; findobj != NULL; findobj = findobj_next ) {
            findobj_next = findobj->next_content;
            if ( findobj->wear_loc != WEAR_NONE && findobj->glory > 0 )
                count++;
        }
        if ( ( ch->level < LEVEL_AVATAR && count > 0 ) || count > 1 ) { /* avs can wear *
                                                                         * TWO glory * *
                                                                         * pieces */
            send_to_char
                ( "&R¡Ya estás vistiendo equipo modificado con gloria! Debes desvestir algo.\r\n",
                  ch );
            WAIT_STATE( ch, 2 );
            return;
        }
    }

    if ( wear_bit > -1 ) {
        bit = wear_bit;
        if ( !CAN_WEAR( obj, 1 << bit ) ) {
            if ( fReplace ) {
                switch ( 1 << bit ) {
                    case ITEM_HOLD:
                        send_to_char( "No puedes sostener eso.\r\n", ch );
                        break;
                    case ITEM_WIELD:
                    case ITEM_MISSILE_WIELD:
                        send_to_char( "No puedes empuñar eso.\r\n", ch );
                        break;
                    default:
                        ch_printf( ch, "No puedes vestir eso en %s.\r\n", w_flags[bit] );
                }
            }
            return;
        }
    }
    else {
        for ( bit = -1, tmp = 1; tmp < 31; tmp++ ) {
            if ( CAN_WEAR( obj, 1 << tmp ) ) {
                bit = tmp;
                break;
            }
        }
    }

/* Volk - moved size here */
    short                   size = check_size( ch );
    bool                    toobig = FALSE,
        toosmall = FALSE,
        big = FALSE,
        small = FALSE;

    if ( obj->size && size > 0 && obj->size > 0 ) {
        if ( size < obj->size ) {
            if ( obj->size - size == 1 )
                big = TRUE;
            else
                toobig = TRUE;
        }

        if ( size > obj->size ) {
            if ( size - obj->size == 1 )
                small = TRUE;
            else
                toosmall = TRUE;
        }

/* Volk - we'll go with switches for the messages, as we'll need to put in actprogs later
(ie $n tries to wear whatever but it is too big/too small/just squeezes into it/drowning in */
        if ( toobig ) {
            switch ( obj->item_type ) {
                default:
                case ITEM_ARMOR:
                    send_to_char( "¡Esto es demasiado grande para que lo uses!\r\n", ch );
                    break;

                case ITEM_WEAPON:
                    send_to_char( "¡Eso es demasiado grande para que puedas empuñarlo!\r\n", ch );
                    break;

            }
            return;
        }

        if ( toosmall ) {
            switch ( obj->item_type ) {
                default:
                case ITEM_ARMOR:
                    send_to_char( "¡Esto es demasiado pequeño para que lo uses!\r\n", ch );
                    break;

                case ITEM_WEAPON:
                    send_to_char( "¡Eso es demasiado pequeño para que puedas empuñarlo!\r\n", ch );
                    break;

            }
            return;
        }

        if ( big ) {
            switch ( obj->item_type ) {
                default:
                case ITEM_ARMOR:
                    send_to_char( "Esto es un poco grande, pero puedes acomodarte...\r\n", ch );
                    break;

                case ITEM_WEAPON:
                    send_to_char
                        ( "Esto es un poco grande, pero sabrás defenderte...\r\n",
                          ch );
                    break;

            }
        }

        if ( small ) {
            switch ( obj->item_type ) {
                default:
                case ITEM_ARMOR:
                    send_to_char( "Esto es un poco pequeño, pero puedes acomodarte.\r\n", ch );
                    break;

                case ITEM_WEAPON:
                    send_to_char
                        ( "Esto es un poco pequeño, pero sabrás defenderte...\r\n",
                          ch );
                    break;

            }
        }

    }                                                  /* end of function */

/*
   * currently cannot have a light in non-light position
   */
    if ( obj->item_type == ITEM_LIGHT ) {
        if ( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
            return;
        if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
            act( AT_ACTION, "$n usa $p como luz.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "Usas $p como luz.", ch, obj, NULL, TO_CHAR );
        }
        equip_char( ch, obj, WEAR_LIGHT );
        oprog_wear_trigger( ch, obj );
        return;
    }

    if ( bit == -1 ) {
        if ( fReplace )
            send_to_char( "No puedes vestir, empuñar o sostener eso.\r\n", ch );
        return;
    }

    switch ( 1 << bit ) {
        default:
            bug( "wear_obj: uknown/unused item_wear bit %d", bit );
            if ( fReplace )
                send_to_char( "No puedes vestir, empuñar o sostener eso.\r\n", ch );
            return;

        case ITEM_LODGE_RIB:
            act( AT_ACTION, "¡$p te golpea y se incrusta profundamente en tus costillas!", ch, obj, NULL,
                 TO_CHAR );
            act( AT_ACTION, "¡$p golpea a $n y se incrusta profundamente en sus costillas!", ch, obj, NULL,
                 TO_ROOM );
            equip_char( ch, obj, WEAR_LODGE_RIB );
            break;

        case ITEM_LODGE_ARM:
            act( AT_ACTION, "¡$p te golpea y se incrusta profundamente en tu brazo!", ch, obj, NULL,
                 TO_CHAR );
            act( AT_ACTION, "¡$p golpea a $n and se incrusta profundamente en su brazo!", ch, obj, NULL,
                 TO_ROOM );
            equip_char( ch, obj, WEAR_LODGE_ARM );
            break;

        case ITEM_LODGE_LEG:
            act( AT_ACTION, "¡$p te golpea y se incrusta profundamente en tu pierna!", ch, obj, NULL,
                 TO_CHAR );
            act( AT_ACTION, "¡$p golpea a $n y se incrusta profundamente en su pierna!", ch, obj, NULL,
                 TO_ROOM );
            equip_char( ch, obj, WEAR_LODGE_LEG );
            break;

        case ITEM_WEAR_FINGER:
            if ( get_eq_char( ch, WEAR_FINGER_L ) && get_eq_char( ch, WEAR_FINGER_R )
                 && !remove_obj( ch, WEAR_FINGER_L, fReplace )
                 && !remove_obj( ch, WEAR_FINGER_R, fReplace ) )
                return;

            if ( !get_eq_char( ch, WEAR_FINGER_L ) ) {
                if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                    if ( ( ch )->race == RACE_DRAGON && !IS_NPC( ch )
                         && !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
                        send_to_char( "¿Dónde usan los dragones los anillos, en los talones?\r\n", ch );
                        return;
                    }
                    if ( ( ch )->race == RACE_ANIMAL && !IS_NPC( ch ) ) {
                        send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                        return;
                    }

                    act( AT_ACTION, "$n se pone $p en su dedo izquierdo.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "Te pones $p en tu dedo izquierdo.", ch, obj, NULL, TO_CHAR );
                }
                equip_char( ch, obj, WEAR_FINGER_L );
                oprog_wear_trigger( ch, obj );
                return;
            }

            if ( !get_eq_char( ch, WEAR_FINGER_R ) ) {
                if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                    if ( ( ch )->race == RACE_DRAGON && !IS_NPC( ch )
                         && !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
                        send_to_char( "¿Dónde usan los dragones los anillos, en los talones?\r\n", ch );
                        return;
                    }
                    if ( ( ch )->race == RACE_ANIMAL && !IS_NPC( ch ) ) {
                        send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                        return;
                    }

                    act( AT_ACTION, "$n se pone $p en su dedo derecho.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "Te pones $p en tu dedo derecho.", ch, obj, NULL, TO_CHAR );
                }
                equip_char( ch, obj, WEAR_FINGER_R );
                oprog_wear_trigger( ch, obj );
                return;
            }

            bug( "%s", "Wear_obj: no tiene dedos libres." );
            send_to_char( "Ya llevas algo puesto en ambos dedos.\r\n", ch );
            return;

        case ITEM_WEAR_NECK:
            if ( get_eq_char( ch, WEAR_NECK_1 ) != NULL && get_eq_char( ch, WEAR_NECK_2 ) != NULL
                 && !remove_obj( ch, WEAR_NECK_1, fReplace )
                 && !remove_obj( ch, WEAR_NECK_2, fReplace ) )
                return;

            if ( !get_eq_char( ch, WEAR_NECK_1 ) ) {
                if ( ( ch )->race == RACE_DRAGON && !IS_NPC( ch )
                     && !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
                    send_to_char( "Tu cuello es demasiado grande como para usar collares o amuletos.\r\n",
                                  ch );
                    return;
                }
                if ( ( ch )->race == RACE_ANIMAL && !IS_NPC( ch ) ) {
                    send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                    return;
                }

                if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                    act( AT_ACTION, "$n se pone $p alrededor de su cuello.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "Te pones $p alrededor de tu cuello.", ch, obj, NULL, TO_CHAR );
                }
                equip_char( ch, obj, WEAR_NECK_1 );
                oprog_wear_trigger( ch, obj );
                return;
            }

            if ( !get_eq_char( ch, WEAR_NECK_2 ) ) {
                if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                    if ( ( ch )->race == RACE_DRAGON && !IS_NPC( ch )
                         && !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
                        send_to_char
                            ( "Tu cuello es demasiado grande como para usar collares o amuletos.\r\n",
                              ch );
                        return;
                    }

                    if ( ( ch )->race == RACE_ANIMAL && !IS_NPC( ch ) ) {
                        send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                        return;
                    }

                    act( AT_ACTION, "$n se pone $p alrededor de su cuello.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "Te pones $p alrededor de tu cuello.", ch, obj, NULL, TO_CHAR );
                }
                equip_char( ch, obj, WEAR_NECK_2 );
                oprog_wear_trigger( ch, obj );
                return;
            }

            bug( "%s", "Wear_obj: no hay más lugar en el cuello." );
            send_to_char( "Ya llevas dos objetos vestidos en el cuello.\r\n", ch );
            return;

        case ITEM_WEAR_BODY:
            /*
             * if(!remove_obj(ch, WEAR_BODY, fReplace))
             * return;
             */
            if ( !can_layer( ch, obj, WEAR_BODY ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                /*
                 * if( (ch)->race == RACE_DRAGON && !IS_AFFECTED(ch, AFF_DRAGONLORD)) {
                 * send_to_char("Your scaly body is far too massive to wear any sort of
                 * armor.\r\n", ch); return; }
                 */
                act( AT_ACTION, "$n se pone $p sobre su cuerpo.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p sobre tu cuerpo.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_BODY );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_SHEATH:
            if ( !remove_obj( ch, WEAR_SHEATH, fReplace ) )
                return;
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n usa $p como vaina para sus armas.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Usas $p como vaina para tus armas.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_SHEATH );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_HEAD:
            if ( !remove_obj( ch, WEAR_HEAD, fReplace ) )
                return;
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p en su cabeza.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p en tu cabeza.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_HEAD );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_EYES:
            if ( !remove_obj( ch, WEAR_EYES, fReplace ) )
                return;
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p sobre sus ojos.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p sobre tus ojos.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_EYES );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_FACE:
            if ( ch->race == RACE_DRAGON && !can_layer( ch, obj, WEAR_FACE ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( obj->pIndexData->layers > 0 ) {
                if ( ch->race != RACE_DRAGON ) {
                    send_to_char( "Solo los dragones pueden vestir eso.\r\n", ch );
                    return;
                }
            }
            if ( ch->race != RACE_DRAGON ) {
                if ( !remove_obj( ch, WEAR_FACE, fReplace ) )
                    return;
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p sobre su cara.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p sobre tu cara.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_FACE );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_EARS:
            if ( !can_layer( ch, obj, WEAR_EARS ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            /*
             * if( !remove_obj( ch, WEAR_EARS, fReplace ) ) return;
             */
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n viste $p en sus orejas.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p en tus orejas.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_EARS );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_LEGS:
            if ( !can_layer( ch, obj, WEAR_LEGS ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                if ( ( ch )->race == RACE_DRAGON && !IS_NPC( ch )
                     && !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
                    send_to_char( "Tus piernas son demasiado grandes como para vestir algún objeto.\r\n",
                                  ch );
                    return;
                }
                if ( ( ch )->race == RACE_ANIMAL && !IS_NPC( ch ) ) {
                    send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                    return;
                }

                act( AT_ACTION, "$n mete sus piernas en $p.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Metes tus piernas en $p.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_LEGS );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_FEET:
/*
      if(!remove_obj(ch, WEAR_FEET, fReplace))
        return;
*/
            if ( !can_layer( ch, obj, WEAR_FEET ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                if ( ( ch )->race == RACE_DRAGON && !IS_NPC( ch )
                     && !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
                    send_to_char( "Tus pies son demasiado grandes como para que puedas usar algo.\r\n", ch );
                    return;
                }
                if ( ( ch )->race == RACE_ANIMAL && !IS_NPC( ch ) ) {
                    send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                    return;
                }

                act( AT_ACTION, "$n se pone $p en sus pies.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p en tus pies.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_FEET );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_HANDS:
/*
      if(!remove_obj(ch, WEAR_HANDS, fReplace))
        return;
*/
            if ( !can_layer( ch, obj, WEAR_HANDS ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p en sus manos.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p en tus manos.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_HANDS );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_ARMS:
/*
      if(!remove_obj(ch, WEAR_ARMS, fReplace))
        return;
*/
            if ( !can_layer( ch, obj, WEAR_ARMS ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p en sus brazos.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p en tus brazos.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_ARMS );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_ABOUT:
            /*
             * if(!remove_obj(ch, WEAR_ABOUT, fReplace))
             * return;
             */
            if ( !can_layer( ch, obj, WEAR_ABOUT ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p alrededor de su cuerpo.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p alrededor de tu cuerpo.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_ABOUT );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_L_WING:

            if ( ch->race != RACE_DRAGON || IS_NPC( ch ) ) {
                send_to_char( "Solo los Dragones pueden vestir equipo en las alas.\r\n", ch );
                return;
            }

            if ( !can_layer( ch, obj, WEAR_L_WING ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }

            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se coloca $p en su ala izquierda.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te colocas $p en tu ala izquierda.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_L_WING );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_R_WING:

            if ( ch->race != RACE_DRAGON || IS_NPC( ch ) ) {
                send_to_char( "Solo los Dragones pueden vestir equipo en las alas.\r\n", ch );
                return;
            }

            if ( !can_layer( ch, obj, WEAR_R_WING ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }

            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se coloca $p en su ala derecha.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te colocas $p en tu ala derecha.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_R_WING );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_BACK:
            if ( ch->race == RACE_DRAGON && !can_layer( ch, obj, WEAR_BACK ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( obj->pIndexData->layers > 0 ) {
                if ( ch->race != RACE_DRAGON ) {
                    send_to_char( "Solo los Dragones pueden vestir eso.\r\n", ch );
                    return;
                }
            }
            if ( ch->race != RACE_DRAGON ) {
                if ( !remove_obj( ch, WEAR_BACK, fReplace ) )
                    return;
            }

            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p en su espalda.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p en tu espalda.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_BACK );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_SHOULDERS:
            if ( !remove_obj( ch, WEAR_SHOULDERS, fReplace ) )
                return;

            if ( IS_NPC( ch ) )
                return;

            if ( !ch->pcdata->city ) {
                send_to_char( "Solo los ciudadanos pueden vestir hombreras en sus hombros.\r\n", ch );
                return;
            }

            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p en sus hombros.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p en tus hombros.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_SHOULDERS );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_WAIST:
/*
      if(!remove_obj(ch, WEAR_WAIST, fReplace))
        return;
*/
            if ( !can_layer( ch, obj, WEAR_WAIST ) ) {
                send_to_char( "No puedes vestir eso sobre lo que ya estás usando.\r\n", ch );
                return;
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n se pone $p alrededor de su cintura.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te pones $p alrededor de tu cintura.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_WAIST );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_WEAR_WRIST:
            if ( get_eq_char( ch, WEAR_WRIST_L ) && get_eq_char( ch, WEAR_WRIST_R )
                 && !remove_obj( ch, WEAR_WRIST_L, fReplace )
                 && !remove_obj( ch, WEAR_WRIST_R, fReplace ) )
                return;

            if ( !get_eq_char( ch, WEAR_WRIST_L ) ) {
                if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                    act( AT_ACTION, "$n se pone $p alrededor de su muñeca izquierda.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "Te pones $p alrededor de tu muñeca izquierda.", ch, obj, NULL, TO_CHAR );
                }
                equip_char( ch, obj, WEAR_WRIST_L );
                oprog_wear_trigger( ch, obj );
                return;
            }

            if ( !get_eq_char( ch, WEAR_WRIST_R ) ) {
                if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                    act( AT_ACTION, "$n se pone $p alrededor de su muñeca derecha.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "Te pones $p alrededor de tu muñeca derecha.", ch, obj, NULL, TO_CHAR );
                }
                equip_char( ch, obj, WEAR_WRIST_R );
                oprog_wear_trigger( ch, obj );
                return;
            }
            bug( "%s", "Wear_obj: no tiene lugar en las muñecas." );
            send_to_char( "Ya llevas dos objetos vestidos en las muñecas.\r\n", ch );
            return;

        case ITEM_WEAR_ANKLE:
            if ( get_eq_char( ch, WEAR_ANKLE_L ) && get_eq_char( ch, WEAR_ANKLE_R )
                 && !remove_obj( ch, WEAR_ANKLE_L, fReplace )
                 && !remove_obj( ch, WEAR_ANKLE_R, fReplace ) )
                return;

            if ( !get_eq_char( ch, WEAR_ANKLE_L ) ) {
                if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                    act( AT_ACTION, "$n se pone $p en su tobillo izquierdo.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "Te pones $p en tu tobillo izquierdo.", ch, obj, NULL, TO_CHAR );
                }
                equip_char( ch, obj, WEAR_ANKLE_L );
                oprog_wear_trigger( ch, obj );
                return;
            }

            if ( !get_eq_char( ch, WEAR_ANKLE_R ) ) {
                if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                    act( AT_ACTION, "$n se pone $p en su tobillo derecho.", ch, obj, NULL, TO_ROOM );
                    act( AT_ACTION, "Te pones $p en tu tobillo derecho.", ch, obj, NULL, TO_CHAR );
                }
                equip_char( ch, obj, WEAR_ANKLE_R );
                oprog_wear_trigger( ch, obj );
                return;
            }

            bug( "%s", "Wear_obj: no hay tobillos libres." );
            send_to_char( "Ya llevas dos objetos vestidos en los tobillos.\r\n", ch );
            return;

        case ITEM_WEAR_SHIELD:
            if ( get_eq_char( ch, WEAR_DUAL_WIELD )
                 || ( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( ch, WEAR_MISSILE_WIELD ) )
                 || ( ( get_eq_char( ch, WEAR_WIELD ) || get_eq_char( ch, WEAR_MISSILE_WIELD ) )
                      && get_eq_char( ch, WEAR_HOLD ) ) ) {
                if ( get_eq_char( ch, WEAR_HOLD ) )
                    send_to_char
                        ( "¡No puedes usar un escudo, mientras estés usando un arma y sosteniendo algo!\r\n",
                          ch );
                else
                    send_to_char( "¡No puedes usar un escudo y dos armas!\r\n", ch );
                return;
            }
            if ( get_eq_char( ch, WEAR_WIELD ) && IS_TWOHANDS( get_eq_char( ch, WEAR_WIELD ) ) ) {
                send_to_char( "¡No puedes usar un escudo y un arma a dos manos!\r\n", ch );
                return;
            }

            if ( ( ch )->race == RACE_DRAGON && !IS_NPC( ch )
                 && !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
                send_to_char( "Los dragones no necesitan usar un escudo.\r\n", ch );
                return;
            }
            if ( ( ch )->race == RACE_ANIMAL && !IS_NPC( ch ) ) {
                send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                return;
            }
             // beastmeld
            if ( ch->desc && ch->desc->original )
            {
                send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                return;
            }

            if ( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
                return;
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n usa $p como escudo.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Usas $p como escudo.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_SHIELD );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_MISSILE_WIELD:
        case ITEM_WIELD:
            if ( ( ch )->race == RACE_DRAGON && !IS_NPC( ch )
                 && !IS_AFFECTED( ch, AFF_DRAGONLORD ) ) {
                send_to_char( "Los Dragones no necesitan usar armas mortales.\r\n", ch );
                return;
            }
            if ( ( ch )->race == RACE_ANIMAL && !IS_NPC( ch ) ) {
                send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                return;
            }
       	     //	beastmeld
            if ( ch->desc && ch->desc->original )
       	    {
                send_to_char( "Los animales no pueden vestir eso.\r\n", ch );
                return;
       	    }
            if ( IS_AFFECTED( ch, AFF_GRAFT ) && obj == get_eq_char( ch, WEAR_WIELD ) ) {
                send_to_char( "Ya llevas un arma aderida a tu mano.\r\n", ch );
                return;
            }
            if ( ch->fighting && ( obj->value[4] == WEP_LANCE ) ) {
                send_to_char( "Estás luchando demasiado cerca de tus enemigos para usar una lanza.\r\n", ch );
                return;
            }

            if ( !could_dual( ch ) ) {
                if ( !remove_obj( ch, WEAR_MISSILE_WIELD, fReplace ) )
                    return;
                if ( !remove_obj( ch, WEAR_WIELD, fReplace ) )
                    return;
                if ( get_eq_char( ch, WEAR_HOLD ) && get_eq_char( ch, WEAR_SHIELD ) ) {
                    send_to_char( "Ya estás usando un escudo y sosteniendo algo.\r\n",
                                  ch );
                    return;
                }
                tmpobj = NULL;
            }
            else {
                OBJ_DATA               *mw,
                                       *dw,
                                       *hd,
                                       *sd;

                tmpobj = get_eq_char( ch, WEAR_WIELD );
                mw = get_eq_char( ch, WEAR_MISSILE_WIELD );
                dw = get_eq_char( ch, WEAR_DUAL_WIELD );
                hd = get_eq_char( ch, WEAR_HOLD );
                sd = get_eq_char( ch, WEAR_SHIELD );

                if ( hd && sd ) {
                    send_to_char( "Ya estás usando un escudo y sosteniendo algo.\r\n",
                                  ch );
                    return;
                }
                /*
                 * If tmpobj or mw exists, then they are already wielding a weapon. -Orion
                 */
                if ( tmpobj ) {
                    if ( !can_dual( ch ) )
                        return;

                    if ( get_obj_weight( obj, FALSE ) + get_obj_weight( tmpobj, FALSE ) >
                         str_app[get_curr_str( ch )].wield ) {
                        send_to_char( "Es demasiado pesado para que puedas empuñar eso.\r\n", ch );
                        return;
                    }
                    if ( mw || dw ) {
                        send_to_char( "Ya estás empuñando dos armas.\r\n", ch );
                        return;
                    }
                    if ( hd || sd ) {
                        send_to_char( "Ya estás empuñando un arma y sosteniendo algo.\r\n",
                                      ch );
                        return;
                    }
                    if ( IS_TWOHANDS( obj ) ) {
                        if ( tmpobj ) {
                            send_to_char
                                ( "¡No puedes usar un arma a dos manos y otra arma!\r\n", ch );
                            return;
                        }
                        if ( mw ) {
                            send_to_char
                                ( "¡No puedes usar un arma a dos manos y un arma arrojadiza!\r\n",
                                  ch );
                            return;
                        }
                        if ( sd ) {
                            send_to_char( "¡No puedes usar un arma a dos manos y un escudo!\r\n",
                                          ch );
                            return;
                        }
                        if ( hd ) {
                            send_to_char
                                ( "No puedes usar un arma a dos manos y sostener algo a la vez.\r\n", ch );
                            return;
                        }
                    }

                    if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                        act( AT_ACTION, "$n empuña $p como arma secundaria.", ch, obj, NULL, TO_ROOM );
                        act( AT_ACTION, "Empuñas $p como arma secundaria.", ch, obj, NULL, TO_CHAR );
                    }
                    if ( 1 << bit == ITEM_MISSILE_WIELD )
                        equip_char( ch, obj, WEAR_MISSILE_WIELD );
                    else
                        equip_char( ch, obj, WEAR_DUAL_WIELD );
                    oprog_wear_trigger( ch, obj );
                    return;
                }
                if ( mw ) {
                    if ( !can_dual( ch ) )
                        return;
                    if ( 1 << bit == ITEM_MISSILE_WIELD ) {
                        send_to_char( "Ya estás empuñando un arma arrojadiza.\r\n", ch );
                        return;
                    }

                    if ( get_obj_weight( obj, FALSE ) + get_obj_weight( mw, FALSE ) >
                         str_app[get_curr_str( ch )].wield ) {
                        send_to_char( "Es ddemasiado pesado para que puedas empuñar eso.\r\n", ch );
                        return;
                    }

                    if ( tmpobj || dw ) {
                        send_to_char( "Ya estás empuñando dos armas.\r\n", ch );
                        return;
                    }

                    if ( IS_TWOHANDS( obj ) ) {
                        if ( sd ) {
                            send_to_char( "¡No puedes usar un arma a dos manos y un escudo!\r\n",
                                          ch );
                            return;
                        }

                        if ( hd ) {
                            send_to_char
                                ( "No puedes usar un arma a dos manos y sostener algo a la vez.\r\n", ch );
                            return;
                        }
                    }

                    if ( hd || sd ) {
                        send_to_char( "Ya estás empuñando un arma y sosteniendo algo.\r\n",
                                      ch );
                        return;
                    }
                    if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                        act( AT_ACTION, "$n empuña $p.", ch, obj, NULL, TO_ROOM );
                        act( AT_ACTION, "Empuñas $p.", ch, obj, NULL, TO_CHAR );
                    }
                    equip_char( ch, obj, WEAR_WIELD );
                    oprog_wear_trigger( ch, obj );
                    return;
                }
            }
            if ( get_obj_weight( obj, FALSE ) > str_app[get_curr_str( ch )].wield ) {
                send_to_char( "Es demasiado pesado para que puedas empuñar eso.\r\n", ch );
                return;
            }

            if ( IS_TWOHANDS( obj ) ) {
                if ( get_eq_char( ch, WEAR_SHIELD ) ) {
                    send_to_char( "¡No puedes usar un arma a dos manos y un escudo!\r\n", ch );
                    return;
                }
                if ( get_eq_char( ch, WEAR_HOLD ) ) {
                    send_to_char( "No puedes usar un arma a dos manos y sostener algo a la vez.\r\n", ch );
                    return;
                }
            }
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n empuña $p.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Empuñas $p.", ch, obj, NULL, TO_CHAR );
            }
            if ( 1 << bit == ITEM_MISSILE_WIELD )
                equip_char( ch, obj, WEAR_MISSILE_WIELD );
            else
                equip_char( ch, obj, WEAR_WIELD );
            oprog_wear_trigger( ch, obj );
            return;

        case ITEM_HOLD:
            if ( get_eq_char( ch, WEAR_DUAL_WIELD )
                 || ( get_eq_char( ch, WEAR_WIELD ) && get_eq_char( ch, WEAR_MISSILE_WIELD ) ) ) {
                send_to_char( "¡No puedes sostener algo y usar dos armas!\r\n", ch );
                return;
            }
            if ( get_eq_char( ch, WEAR_WIELD ) && IS_TWOHANDS( get_eq_char( ch, WEAR_WIELD ) ) ) {
                send_to_char( "¡No puedes sostener algo y usar un arma a dos manos!\r\n", ch );
                return;
            }
            if ( ( get_eq_char( ch, WEAR_WIELD ) || get_eq_char( ch, WEAR_MISSILE_WIELD ) )
                 && get_eq_char( ch, WEAR_SHIELD ) ) {
                send_to_char( "¡No puedes sostener algo mientras uses un arma y un escudo!\r\n",
                              ch );
                return;
            }
            if ( !remove_obj( ch, WEAR_HOLD, fReplace ) )
                return;
            if ( obj->item_type == ITEM_WAND
                 || obj->item_type == ITEM_STAFF
                 || obj->item_type == ITEM_FOOD
                 || obj->item_type == ITEM_COOK
                 || obj->item_type == ITEM_PILL
                 || obj->item_type == ITEM_POTION
                 || obj->item_type == ITEM_SCROLL
                 || obj->item_type == ITEM_DRINK_CON
                 || obj->item_type == ITEM_RAW
                 || obj->item_type == ITEM_BLOOD || obj->item_type == ITEM_PIPE
                 || obj->item_type == ITEM_HERB || obj->item_type == ITEM_KEY
                 || !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n sostiene $p en sus manos.", ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Sostienes $p en tus manos.", ch, obj, NULL, TO_CHAR );
            }
            equip_char( ch, obj, WEAR_HOLD );
            oprog_wear_trigger( ch, obj );
            return;
    }
}

char                   *myobj( OBJ_DATA *obj );

void do_dye( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    OBJ_DATA               *obj,
                           *dye;
    int                     chance;
    const char             *adj;
    char                    newshort[MSL];
    const char             *oldshort;

    chance = ( LEARNED( ch, gsn_dye ) );
    /*
     * Do we have the skill?
     */
    if ( !IS_NPC( ch ) && !( LEARNED( ch, gsn_dye ) ) ) {
        send_to_char( "¡No sabes como hacer eso!.\r\n", ch );
        return;
    }

    /*
     * Do we have the components?
     */

    argument = one_argument( argument, arg1 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "¿Qué objeto quieres teñir?\r\n", ch );
        send_to_char( "Sintaxis: Tintar (objeto) <color>\r\n", ch );
        send_to_char
            ( "Los colores disponibles son: \r\n&CAzul &Cclaro, &ONaranja, &cCyan, &RRojo, &BAzul, &WBlanco, &rRojo &roscuro, &bAzul &boscuro, &wGris, &GVerde, &PRosa, &gVerde &goscuro, &pPúrpura, &zGris &zoscuro, &YAmarillo\r\n",
              ch );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "No tienes ningún objeto para teñir.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg2 );
    if ( arg2[0] == '\0' ) {
        send_to_char( "¿De qué color?\r\n", ch );
        send_to_char
            ( "Los colores disponibles son: \r\n&CAzul &Cclaro, &ONaranja, &cCyan, &RRojo, &BAzul, &WBlanco, &rRojo &roscuro, &bAzul &boscuro, &wGris, &GVerde, &PRosa, &gVerde &goscuro, &pPúrpura, &zGris &zoscuro, &YAmarillo\r\n",
              ch );
        return;
    }

    dye = get_eq_char( ch, WEAR_HOLD );
    if ( dye == NULL || dye->item_type != ITEM_DYE ) {
        send_to_char( "Para teñir un objeto necesitas sostener una botella de tintura.\r\n", ch );
        return;
    }

    /*
     * dye the clothing - success!
     */

     if ( !can_dye_obj( ch, obj ) ) {
		 send_to_char( "Un objeto solo puede ser teñido una vez.\r\n", ch );
return;
}
    if ( number_percent(  ) < chance ) {
        if ( !str_cmp( arg2, "rojo" ) ) {
            obj->color = 3;
                    xSET_BIT( obj->extra_flags, ITEM_ROJO );
        }

        else if ( !str_cmp( arg2, "azul" ) ) {
            obj->color = 4;
                    xSET_BIT( obj->extra_flags, ITEM_AZUL );
        }

        else if ( !str_cmp( arg2, "purpura" ) ) {
            obj->color = 12;
                    xSET_BIT( obj->extra_flags, ITEM_PURPURA );
        }

        else if ( !str_cmp( arg2, "amarillo" ) ) {
            obj->color = 14;
                    xSET_BIT( obj->extra_flags, ITEM_AMARILLO );
        }
        else if ( !str_cmp( arg2, "blanco" ) ) {
            obj->color = 5;
                    xSET_BIT( obj->extra_flags, ITEM_BLANCO );
        }

        else if ( !str_cmp( arg2, "azul-oscuro" ) ) {
            obj->color = 7;
                    xSET_BIT( obj->extra_flags, ITEM_AZULOSCURO );
        }
        else if ( !str_cmp( arg2, "verde" ) ) {
            obj->color = 9;
                    xSET_BIT( obj->extra_flags, ITEM_VERDE );
        }

        else if ( !str_cmp( arg2, "azul-claro" ) ) {
            obj->color = 0;
                    xSET_BIT( obj->extra_flags, ITEM_AZULCLARO );
        }

        else if ( !str_cmp( arg2, "verde-oscuro" ) ) {
            obj->color = 11;
                    xSET_BIT( obj->extra_flags, ITEM_VERDEOSCURO );
        }
        else if ( !str_cmp( arg2, "cyan" ) ) {
            obj->color = 2;
                    xSET_BIT( obj->extra_flags, ITEM_CYAN );
        }
        else if ( !str_cmp( arg2, "rosa" ) ) {
            obj->color = 10;
                    xSET_BIT( obj->extra_flags, ITEM_ROSA );
        }
        else if ( !str_cmp( arg2, "rojo-oscuro" ) ) {
            obj->color = 6;
                    xSET_BIT( obj->extra_flags, ITEM_ROJOOSCURO );
        }
        else if ( !str_cmp( arg2, "gris" ) ) {
            obj->color = 8;
                    xSET_BIT( obj->extra_flags, ITEM_GRIS );
        }
        else if ( !str_cmp( arg2, "gris-oscuro" ) ) {
            obj->color = 13;
                    xSET_BIT( obj->extra_flags, ITEM_GRISOSCURO );
        }
        else if ( !str_cmp( arg2, "naranja" ) ) {
            obj->color = 1;
                    xSET_BIT( obj->extra_flags, ITEM_NARANJA );
        }
        else {
            send_to_char
                ( "Los colores disponibles son: \r\n&CAzul &Cclaro, &ONaranja, &cCyan, &RRojo, &BAzul, &WBlanco, &rRojo &roscuro, &bAzul &boscuro, &wGris, &GVerde, &PRosa, &gVerde &goscuro, &pPúrpura, &zGris &zoscuro, &YAmarillo\r\n",
                  ch );
            return;
        }
        extract_obj( dye );
        send_to_char( "Tiñes el objeto elegantemente.\r\n", ch );
        act( AT_IMMORT, "$n tiñe el objeto elegantemente.", ch, NULL, NULL, TO_ROOM );
        if ( number_bits( 1 ) == 0 )
            learn_from_success( ch, gsn_dye );
        return;
    }
    else {                                             /* failure of skill */

        extract_obj( dye );
        send_to_char( "Intentas teñir el objeto... Pero fallas.\r\n", ch );
        act( AT_IMMORT, "$n intenta teñir un objeto... pero falla.", ch, NULL, NULL, TO_ROOM );
        if ( number_bits( 1 ) == 0 )
            learn_from_failure( ch, gsn_dye );
        return;
    }
}

void do_resize( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *mob;
    OBJ_DATA               *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    for ( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
        if ( IS_NPC( mob ) && ( xIS_SET( mob->act, ACT_RESIZER ) ) )
            break;

    if ( !mob ) {
        send_to_char( "No puedes hacer eso aquí.\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        act( AT_TELL,
             "$n te cuenta 'La adaptación de tamaño de un objeto te costará más caro cuanto más grande hagas el objeto.'",
             mob, NULL, ch, TO_VICT );
        send_to_char( "\r\n&cEscribe adaptar <objeto> <opción>.\r\n", ch );
        send_to_char( "Donde opción puede ser: achicar agrandar encantar\r\n", ch );
        act( AT_TELL,
             "\r\n\r\n$n te cuenta 'Actualmente, mis honorarios son 5 monedas de oro por modificación.\r\nTambién, por 100 monedas de oro puedo intentar encantar el objeto para que se ajuste mágicamente a quien lo use.'",
             mob, NULL, ch, TO_VICT );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "No tienes ese objeto.\r\n", ch );
        return;
    }

    if ( arg1[0] != '\0' && arg2[0] == '\0' ) {
        act( AT_TELL,
             "$n te cuenta '¿Qué quieres hacer con el objeto? achicarlo, agrandarlo o encantarlo.'",
             mob, NULL, ch, TO_VICT );
        send_to_char( "\r\n&cEscribe Adaptar <objeto> <opción>.\r\n", ch );
        return;
    }

    if ( obj->size == 0 ) {
        act( AT_TELL,
             "$n te cuenta 'Ese objeto se ajusta mágicamente al tamaño de quien lo use.'", mob,
             NULL, ch, TO_VICT );
        return;
    }
    if ( GET_MONEY( ch, CURR_GOLD ) < 5 ) {
        ch_printf( ch, "No tienes suficiente oro.\r\n" );
        return;
    }

    if ( !str_cmp( arg2, "magical" ) && GET_MONEY( ch, CURR_GOLD ) < 100 ) {
        ch_printf( ch, "No tienes tanto oro.\r\n" );
        return;
    }

    if ( abs( obj->size - obj->pIndexData->size ) > 3 && str_cmp( arg2, "encantar" ) ) {
        act( AT_CYAN, "\r\n$n pone $p ofrecido por $N sobre la mesa y frunce el ceño.\r\n", mob,
             obj, ch, TO_ROOM );
        act( AT_TELL, "\r\n$n te cuenta 'No puedes adaptar $p, ¡además sería inútil!.'", mob,
             obj, ch, TO_VICT );
        act( AT_CYAN, "$n le devuelve $p a $N.\r\n", mob, obj, ch, TO_ROOM );
        return;
    }

    if ( !str_cmp( arg2, "achicar" ) ) {
        if ( obj->size == 1 ) {
            act( AT_TELL, "$n te cuenta 'Ese objeto ya es lo más pequeño posible.'", mob, NULL, ch,
                 TO_VICT );
            return;
        }
        act( AT_TELL, "$n te cuenta 'Muy bien.'", mob, NULL, ch, TO_VICT );
        act( AT_GREEN, "\r\n$n pone $p ofrecido por $N sobre la mesa y comienza a trabajar.", mob, obj, ch,
             TO_ROOM );
        obj->size = obj->size - 1;
        if ( abs( obj->size - obj->pIndexData->size ) == 3 ) {
            if ( obj->value[0] > 1 )
                obj->value[0] = obj->value[0] / 2;
            act( AT_TELL, "$n te cuenta '$p ya no es tan resistente como solía ser, a causa de su tercera adaptación.'",
                 mob, obj, ch, TO_VICT );
        }
        act( AT_CYAN,
             "\r\n$n sonríe mientras acepta el oro y devuelve $p adaptado a $N.", mob,
             obj, ch, TO_ROOM );
        GET_MONEY( ch, CURR_GOLD ) -= 5;
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg2, "agrandar" ) ) {
        if ( obj->size == 7 ) {
            act( AT_TELL, "$n te cuenta 'Ese objeto ya es lo más grande posible.'", mob, NULL, ch,
                 TO_VICT );
            return;
        }
        act( AT_TELL, "$n te cuenta 'Muy bien.'", mob, NULL, ch, TO_VICT );
        act( AT_GREEN,
             "\r\n$n pone $p ofrecido por $N sobre la mesa, coge algunos materiales y comienza a trabajar.",
             mob, obj, ch, TO_ROOM );
        obj->size = obj->size + 1;
        if ( abs( obj->size - obj->pIndexData->size ) == 3 ) {
            if ( obj->value[0] > 1 )
                obj->value[0] = obj->value[0] / 2;
            act( AT_TELL, "$n te cuenta '$p ya no es tan resistente como solía ser, a causa de su tercera adaptación.'",
                 mob, obj, ch, TO_VICT );
        }
        act( AT_CYAN,
             "\r\n$n sonríe mientras acepta el oro y devuelve $p adaptado a $N.", mob,
             obj, ch, TO_ROOM );
        GET_MONEY( ch, CURR_GOLD ) -= 5;
        save_char_obj( ch );
        return;
    }

    if ( !str_cmp( arg2, "encantar" ) ) {
        act( AT_TELL, "$n te cuenta 'Muy bien.'", mob, NULL, ch, TO_VICT );
        act( AT_GREEN, "\r\n$n pone $p ofrecido por $N en la mesa y comienza a pronunciar palabras arcanas.", mob,
             obj, ch, TO_ROOM );
        obj->size = 0;
        act( AT_CYAN,
             "\r\n$n sonríe mientras acepta el oro y devuelve $p adaptado a $N.", mob,
             obj, ch, TO_ROOM );
        GET_MONEY( ch, CURR_GOLD ) -= 100;
        save_char_obj( ch );
        return;
    }
    save_char_obj( ch );
}

void do_unsheath( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    OBJ_DATA               *obj,
                           *sobj,
                           *wield,
                           *dwield,
                           *mwield;
    int                     count = 0;

    argument = one_argument( argument, arg1 );

    if ( ms_find_obj( ch ) )
        return;

    if ( !arg1 || arg1[0] == '\0' ) {
        if ( !( obj = get_eq_char( ch, WEAR_SHEATH ) ) ) {
            send_to_char( "No estás vistiendo una vaina.\r\n", ch );
            return;
        }
    }
    else {
        if ( !( obj = get_obj_carry( ch, arg1 ) ) ) {
            send_to_char( "No tienes eso en tu inventario.\r\n", ch );
            return;
        }
    }

    if ( obj->item_type != ITEM_SHEATH ) {
        send_to_char( "No es tu vaina actual.\r\n", ch );
        return;
    }

    if ( !obj->first_content ) {
        send_to_char( "No hay nada en la vaina.\r\n", ch );
        return;
    }

    while ( sobj = obj->first_content ) {
        wield = get_eq_char( ch, WEAR_WIELD );
        dwield = get_eq_char( ch, WEAR_DUAL_WIELD );
        mwield = get_eq_char( ch, WEAR_MISSILE_WIELD );

        /*
         * No open place?
         */
        if ( ( wield && dwield ) || mwield ) {
            act( AT_ACTION,
                 "Comienzas a desenvainar tu arma... Pero no tienes manos libres.", ch,
                 NULL, NULL, TO_CHAR );
            return;
        }

        obj_from_obj( sobj );
        obj_to_char( sobj, ch );

        act( AT_ACTION, "Desenvainas $p produciendo un sonido familiar.", ch, sobj, NULL, TO_CHAR );
        act( AT_ACTION, "$n desenvaina $p produciendo un sonido inquietante.", ch, sobj, NULL, TO_ROOM );

        if ( ++count == 1 ) {
            if ( !wield )
                equip_char( ch, sobj, WEAR_WIELD );
        }
        else if ( count == 2 ) {
            if ( !dwield )
                equip_char( ch, sobj, WEAR_DUAL_WIELD );
        }
    }
}

void do_sheath( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    OBJ_DATA               *obj = NULL,
        *obj2 = NULL,
        *sobj,
        *tmpobj;

    argument = one_argument( argument, arg1 );

    if ( ms_find_obj( ch ) )
        return;

    if ( !arg1 || arg1[0] == '\0' ) {
        obj = get_eq_char( ch, WEAR_WIELD );
        obj2 = get_eq_char( ch, WEAR_DUAL_WIELD );
        if ( !obj && !obj2 ) {
            send_to_char( "No estás empuñando nada para envainar.\r\n", ch );
            return;
        }
    }
    else {
        if ( !( obj = get_obj_carry( ch, arg1 ) ) && !( obj = get_obj_wear( ch, arg1 ) ) ) {
            send_to_char( "No tienes ese objeto.\r\n", ch );
            return;
        }
    }

    /*
     * Make sure both are valid
     */
    if ( !obj && obj2 ) {
        obj = obj2;
        obj2 = NULL;
    }

    if ( !obj2 )
        obj2 = obj;

    if ( obj->item_type != ITEM_WEAPON || obj2->item_type != ITEM_WEAPON ) {
        send_to_char( "Eso no es algo que se pueda envainar.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        if ( !( sobj = get_eq_char( ch, WEAR_SHEATH ) ) ) {
            send_to_char( "No estás vistiendo una vaina.\r\n", ch );
            return;
        }
    }
    else {
        if ( !( sobj = get_obj_carry( ch, argument ) ) ) {
            send_to_char( "No tienes eso en tu inventario.\r\n", ch );
            return;
        }
    }

    if ( sobj->item_type != ITEM_SHEATH ) {
        send_to_char( "No es tu vaina actual.\r\n", ch );
        return;
    }

    if ( sobj->first_content ) {
        send_to_char( "Ya hay algo en la vaina.\r\n", ch );
        return;
    }

    if ( obj ) {
        act( AT_ACTION, "Envainas $p.", ch, obj, NULL, TO_CHAR );
        act( AT_ACTION, "$n envaina $p.", ch, obj, NULL, TO_ROOM );
        unequip_char( ch, obj );
        obj_from_char( obj );
        obj_to_obj( obj, sobj );
    }
    if ( obj2 != obj ) {
        act( AT_ACTION, "Envainas $p.", ch, obj2, NULL, TO_CHAR );
        act( AT_ACTION, "$n envaina $p.", ch, obj2, NULL, TO_ROOM );
        unequip_char( ch, obj2 );
        obj_from_char( obj2 );
        obj_to_obj( obj2, sobj );
    }
}

int check_size( CHAR_DATA *ch )
{
    float                   feet = ch->height / 12;

    if ( IS_NPC( ch ) )
        return 0;

    if ( feet <= 2 )
        return 1;
    else if ( feet < 5 )
        return 2;
    else if ( feet < 10 )
        return 3;
    else if ( feet < 20 )
        return 4;
    else if ( feet < 50 )
        return 5;
    else if ( feet < 100 )
        return 6;

    return 7;

}

void do_wear( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    OBJ_DATA               *obj;
    short                   wear_bit;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( ( !str_cmp( arg2, "sobre" ) || !str_cmp( arg2, "en" ) || !str_cmp( arg2, "alrededor" ) )
         && argument[0] != '\0' )

        argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "¿Empuñar, vestir o sostener qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( !str_cmp( arg1, "todo" ) ) {
        OBJ_DATA               *obj_next;

// Let's wear layerable items first, then move on to the rest - Aurin
// 3/1/14 Volk - cool idea but bugged, now fixed
        for ( int i = 1; i < 129; i++ )                // i = layer number
        {
            for ( obj = ch->first_carrying; obj != NULL; obj = obj_next ) {
                obj_next = obj->next_content;
                // Let's ignore those items that aren't to be layered
                if ( obj->pIndexData->layers < 1 )
                    continue;
                if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
                     && obj->pIndexData->layers == i ) {
                    wear_obj( ch, obj, FALSE, -1 );
                    if ( char_died( ch ) )
                        return;
                }
            }
        }

        // And now for the rest of the eq that you can wear - Aurin
        for ( obj = ch->first_carrying; obj != NULL; obj = obj_next ) {
            obj_next = obj->next_content;
            if ( can_see_obj( ch, obj ) && obj->wear_loc == WEAR_NONE ) {
                wear_obj( ch, obj, FALSE, -1 );
                if ( char_died( ch ) )
                    return;
            }
        }
        return;
    }
    else {
        if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
            send_to_char( "No tienes ese objeto.\r\n", ch );
            return;
        }
        if ( arg2[0] != '\0' )
            wear_bit = get_wflag( arg2 );
        else
            wear_bit = -1;
        wear_obj( ch, obj, TRUE, wear_bit );
    }

    return;
}

void do_remove( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj,
                           *obj_next;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Guardar qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( IS_AFFECTED( ch, AFF_DRAGONLORD ) && is_affected( ch, gsn_blindness ) ) {

        if ( !str_cmp( arg, "todo" ) ) {                /* SB Remove all */
            affect_strip( ch, gsn_blindness );
            for ( obj = ch->first_carrying; obj != NULL; obj = obj_next ) {
                obj_next = obj->next_content;
                if ( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj ) )
                    remove_obj( ch, obj->wear_loc, TRUE );
            }
            return;
        }
    }
    else {
        if ( !str_cmp( arg, "todo" ) ) {                /* SB Remove all */
            for ( obj = ch->first_carrying; obj != NULL; obj = obj_next ) {
                obj_next = obj->next_content;
                if ( obj->wear_loc != WEAR_NONE && can_see_obj( ch, obj ) )
                    remove_obj( ch, obj->wear_loc, TRUE );
            }
            return;
        }
    }

    if ( ( obj = get_obj_wear( ch, arg ) ) == NULL ) {
        send_to_char( "No estás usando ese objeto.\r\n", ch );
        return;
    }
    if ( ( obj_next = get_eq_char( ch, obj->wear_loc ) ) != obj ) {
        act( AT_PLAIN, "Primero debes quitarte $p.", ch, obj_next, NULL, TO_CHAR );
        return;
    }

    remove_obj( ch, obj->wear_loc, TRUE );
    return;
}

void do_bury( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    bool                    shovel;
    short                   move;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Qué deseas enterrar?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    shovel = FALSE;
    for ( obj = ch->first_carrying; obj; obj = obj->next_content )
        if ( obj->item_type == ITEM_SHOVEL ) {
            shovel = TRUE;
            break;
        }

    obj = get_obj_list_rev( ch, arg, ch->in_room->last_content );
    if ( !obj ) {
        send_to_char( "No puedes encontrar eso.\r\n", ch );
        return;
    }

    separate_obj( obj );
    if ( !CAN_WEAR( obj, ITEM_TAKE ) ) {
        if ( !IS_OBJ_STAT( obj, ITEM_CLANCORPSE ) || IS_NPC( ch )
             || !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) ) {
            act( AT_PLAIN, "No puedes enterrar $p.", ch, obj, 0, TO_CHAR );
            return;
        }
    }

    switch ( ch->in_room->sector_type ) {
        case SECT_ROAD:
        case SECT_HROAD:
        case SECT_VROAD:
        case SECT_INSIDE:
            send_to_char( "El suelo es demasiado duro para cavar en el.\r\n", ch );
            return;
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_UNDERWATER:
            send_to_char( "No puedes enterrar algo aquí.\r\n", ch );
            return;
        case SECT_AIR:
            send_to_char( "¿Enterrar algo en el aire? ¡No me seas tonto!\r\n", ch );
            return;
    }

    if ( obj->weight > ( UMAX( 5, ( can_carry_w( ch ) / 10 ) ) ) && !shovel ) {
        send_to_char( "Necesitas una pala para enterrar algo tan grande.\r\n", ch );
        return;
    }

    move = ( obj->weight * 50 * ( shovel ? 1 : 5 ) ) / UMAX( 1, can_carry_w( ch ) );
    move = URANGE( 2, move, 1000 );
    if ( move > ch->move ) {
        send_to_char( "No tienes energía para enterrar algo de ese tamaño.\r\n", ch );
        return;
    }
    ch->move -= move;
    if ( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )

        act( AT_ACTION, "Solemnemente entierras $p...", ch, obj, NULL, TO_CHAR );
    act( AT_ACTION, "$n entierra solemnemente $p...", ch, obj, NULL, TO_ROOM );
    xSET_BIT( obj->extra_flags, ITEM_BURIED );
    WAIT_STATE( ch, URANGE( 10, move / 2, 100 ) );
    return;
}

void do_trash( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            name[50];
    OBJ_DATA               *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' || !str_cmp( arg, ch->name ) ) {
        act( AT_ACTION, "Te ofreces a ti mismo como basura para ser reciclado... Pero ni para eso sirves.", ch, NULL, NULL, TO_CHAR );
        act( AT_ACTION, "$n se ofrece a si mismo como basura... evidentemente no se tiene en buen concepto.", ch, NULL, NULL, TO_ROOM );
        return;
    }
    if ( ms_find_obj( ch ) )
        return;
    obj = get_obj_list_rev( ch, arg, ch->in_room->last_content );
    if ( !obj ) {
        send_to_char( "No puedes encontrar eso.\r\n", ch );
        return;
    }
    separate_obj( obj );
    if ( !CAN_WEAR( obj, ITEM_TAKE ) ) {
        act( AT_PLAIN, "$p no puede ser reciclado.", ch, obj, 0, TO_CHAR );
        return;
    }
    if ( xIS_SET( obj->extra_flags, ITEM_PKDISARMED ) && !IS_NPC( ch ) ) {
        if ( CAN_PKILL( ch ) && !get_timer( ch, TIMER_PKILLED ) ) {
            if ( ch->level - obj->value[5] > 5 || obj->value[5] - ch->level > 5 ) {
                send_to_char_color( "\r\n&bLa Administración de 6Dragones detiene tu mano.\r\n",
                                    ch );
                return;
            }
        }
    }

    /*
     * Volk: Stop trashing containers with stuff in it
     */
    if ( obj->item_type == ITEM_CONTAINER && obj->first_content ) {
        send_to_char( "Por favor, vacía el contenedor antes de reciclarlo.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) ) {
        if ( obj->currtype && obj->currtype < MAX_CURR_TYPE && obj->cost > 0
             && obj->item_type != ITEM_CONTAINER ) {
            if ( ch->level < 20 ) {
                GET_MONEY( ch, CURR_COPPER ) += 1;
                ch_printf( ch,
                           "La Administración de 6Dragones te da una moneda de cobre por tu contribución a la limpieza.\r\n" );
            }
            if ( ch->level > 19 ) {
                GET_MONEY( ch, CURR_BRONZE ) += 1;
                ch_printf( ch,
                           "La Administración de 6Dragones te da una moneda de bronce por tu contribución a la limpieza.\r\n" );
            }
        }
        else if ( obj->item_type == ITEM_CORPSE_NPC ) { /* Corpses should at least give 1
                                                         *
                                                         * copper */
            if ( ch->level < 20 ) {
                GET_MONEY( ch, CURR_COPPER ) += 5;
                ch_printf( ch,
                           "La Administración de 6Dragones te da cinco monedas de cobre por tu contribución a la limpieza.\r\n" );
            }
            if ( ch->level > 19 ) {
                GET_MONEY( ch, CURR_BRONZE ) += 5;
                ch_printf( ch,
                           "La Administración de 6Dragones te da cinco monedas de bronce por tu contribución a la limpieza.\r\n" );
            }

        }
        else {
            send_to_char( "No ganas nada por eso.\r\n", ch );
        }

/* Volk - whoops, need to send a message to the player too.. */
        ch_printf( ch, "Tiras a la basura %s.\r\n", obj->short_descr );
        act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n tira a la basura $p." );
    }
    oprog_trash_trigger( ch, obj );
    if ( obj_extracted( obj ) )
        return;
    if ( cur_obj == obj->serial )
        global_objcode = rOBJ_TRASHED;
    separate_obj( obj );
    extract_obj( obj );
}

void do_brandish( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *vch;
    CHAR_DATA              *vch_next;
    OBJ_DATA               *staff;
    ch_ret                  retcode;
    int                     sn;

    if ( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL ) {
        send_to_char( "No tienes un bastón en tus manos.\r\n", ch );
        return;
    }

    if ( staff->item_type != ITEM_STAFF ) {
        send_to_char( "Solo puedes sacudir un bastón.\r\n", ch );
        return;
    }

    if ( ( sn = staff->value[3] ) < 0 || sn >= top_sn || skill_table[sn]->spell_fun == NULL ) {
        bug( "Do_brandish: bad sn %d.", sn );
        return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( staff->value[2] > 0 ) {
        if ( !oprog_use_trigger( ch, staff, NULL, NULL, NULL ) ) {
            act( AT_MAGIC, "$n sacude $p.", ch, staff, NULL, TO_ROOM );
            act( AT_MAGIC, "Sacudes $p.", ch, staff, NULL, TO_CHAR );
        }
        for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
            vch_next = vch->next_in_room;
            if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                 && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;
            else
                switch ( skill_table[sn]->target ) {
                    default:
                        bug( "Do_brandish: bad target for sn %d.", sn );
                        return;

                    case TAR_IGNORE:
                        if ( vch != ch )
                            continue;
                        break;

                    case TAR_CHAR_OFFENSIVE:
                        if ( IS_NPC( ch ) ? IS_NPC( vch ) : !IS_NPC( vch ) )
                            continue;
                        break;

                    case TAR_CHAR_DEFENSIVE:
                        if ( IS_NPC( ch ) ? !IS_NPC( vch ) : IS_NPC( vch ) )
                            continue;
                        break;

                    case TAR_CHAR_SELF:
                        if ( vch != ch )
                            continue;
                        break;
                }

            retcode = obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
            if ( retcode == rCHAR_DIED || retcode == rBOTH_DIED ) {
                bug( "%s", "do_brandish: char died" );
                return;
            }
        }
    }

    if ( --staff->value[2] <= 0 ) {
        act( AT_MAGIC, "¡$p brilla por un instante en las manos de $n y se apaga lentamente!", ch, staff, NULL, TO_ROOM );
        act( AT_MAGIC, "¡$p brilla por un instante pero se apaga lentamente!", ch, staff, NULL, TO_CHAR );
        if ( staff->serial == cur_obj )
            global_objcode = rOBJ_USED;
        extract_obj( staff );
    }

    return;
}

void do_zap( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *wand;
    OBJ_DATA               *obj;
    ch_ret                  retcode;

    one_argument( argument, arg );
    if ( arg[0] == '\0' && !ch->fighting ) {
        send_to_char( "¿Agitar qué o contra quién?\r\n", ch );
        return;
    }

    if ( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL ) {
        send_to_char( "No tienes ninguna varita en tus manos.\r\n", ch );
        return;
    }

    if ( wand->item_type != ITEM_WAND ) {
        send_to_char( "Solo puedes agitar una varita.\r\n", ch );
        return;
    }

    obj = NULL;
    if ( arg[0] == '\0' ) {
        if ( ch->fighting ) {
            victim = who_fighting( ch );
        }
        else {
            send_to_char( "¿Agitar qué o contra quién?\r\n", ch );
            return;
        }
    }
    else {
        if ( ( victim = get_char_room( ch, arg ) ) == NULL
             && ( obj = get_obj_here( ch, arg ) ) == NULL ) {
            send_to_char( "No puedes encontrar eso.\r\n", ch );
            return;
        }
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );

    if ( xIS_SET( ch->act, PLR_BATTLE ) )
        send_to_char( "!!SOUND(sound/wandzap.wav)\r\n", ch );

    if ( wand->value[2] > 0 ) {
        if ( victim ) {
            if ( !oprog_use_trigger( ch, wand, victim, NULL, NULL ) ) {
                act( AT_MAGIC, "$n apunta con $p to $N.", ch, wand, victim, TO_ROOM );
                act( AT_MAGIC, "Apuntas con $p at $N.", ch, wand, victim, TO_CHAR );
            }
        }
        else {
            if ( !oprog_use_trigger( ch, wand, NULL, obj, NULL ) ) {
                act( AT_MAGIC, "$n apunta a $p con $P.", ch, wand, obj, TO_ROOM );
                act( AT_MAGIC, "Apuntas a $p con $P.", ch, wand, obj, TO_CHAR );
            }
        }

        retcode = obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
        if ( retcode == rCHAR_DIED || retcode == rBOTH_DIED ) {
            bug( "%s", "do_zap: char died" );
            return;
        }
    }

    if ( --wand->value[2] <= 0 ) {
        act( AT_MAGIC, "$p explota estrepitosamente.", ch, wand, NULL, TO_ROOM );
        act( AT_MAGIC, "$p explota estrepitosamente.", ch, wand, NULL, TO_CHAR );
        if ( wand->serial == cur_obj )
            global_objcode = rOBJ_USED;
        extract_obj( wand );
    }

    return;
}

/*
 * Save items in a clan storage room   -Scryn & Thoric
 */

void save_clan_storeroom( CHAR_DATA *ch )
{
    FILE                   *fp;
    char                    filename[256];
    short                   templvl;
    OBJ_DATA               *contents;

    if ( !ch ) {
        bug( "%s", "save_clan_storeroom: Null ch pointer!" );
        return;
    }

    snprintf( filename, 256, "%s%d.vault", STORAGE_DIR, ch->in_room->vnum );
    if ( ( fp = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "%s", "save_clan_storeroom: FileOpen" );
        perror( filename );
    }
    else {
        templvl = ch->level;
        ch->level = LEVEL_HERO;                        /* make sure EQ doesn't get lost */
        contents = ch->in_room->last_content;
        fprintf( fp, "#VNUM %d\n", ch->in_room->vnum );
        if ( contents && !IS_OBJ_STAT( contents, ITEM_CLANOBJECT ) )
            fwrite_obj( ch, contents, fp, 0, OS_CARRY, FALSE );
        fprintf( fp, "#END\n" );
        ch->level = templvl;
        FileClose( fp );
        return;
    }
    return;
}

/* Make objects in rooms that are nofloor fall - Scryn 1/23/96 */
void obj_fall( OBJ_DATA *obj, bool through )
{
    EXIT_DATA              *pexit;
    ROOM_INDEX_DATA        *to_room;
    static int              fall_count;
    static bool             is_falling;                /* Stop loops from the call to
                                                        * obj_to_room() -- Altrag */

    if ( !obj->in_room || is_falling )
        return;

    if ( fall_count > 30 ) {
        bug( "%s", "object falling in loop more than 30 times" );
        extract_obj( obj );
        fall_count = 0;
        return;
    }

    if ( IS_SET( obj->in_room->room_flags, ROOM_NOFLOOR ) && CAN_GO( obj, DIR_DOWN )
         && !IS_OBJ_STAT( obj, ITEM_MAGIC ) ) {
        pexit = get_exit( obj->in_room, DIR_DOWN );
        to_room = pexit->to_room;

        if ( through )
            fall_count++;
        else
            fall_count = 0;

        if ( obj->in_room == to_room ) {
            bug( "Object falling into same room, room %d", to_room->vnum );
            extract_obj( obj );
            return;
        }

        if ( obj->in_room->first_person ) {
            act( AT_PLAIN, "$p inicia una larga caída...", obj->in_room->first_person, obj, NULL,
                 TO_ROOM );
            act( AT_PLAIN, "$p inicia una larga caída...", obj->in_room->first_person, obj, NULL,
                 TO_CHAR );
        }
        obj_from_room( obj );
        is_falling = TRUE;
        obj = obj_to_room( obj, to_room );
        is_falling = FALSE;

        if ( obj->in_room->first_person ) {
            act( AT_PLAIN, "$p cae desde arriba...", obj->in_room->first_person, obj, NULL,
                 TO_ROOM );
            act( AT_PLAIN, "$p cae desde arriba...", obj->in_room->first_person, obj, NULL,
                 TO_CHAR );
        }

        if ( !IS_SET( obj->in_room->room_flags, ROOM_NOFLOOR ) && through ) {
/*  int dam = (int)9.81*sqrt(fall_count*2/9.81)*obj->weight/2;
*/ int dam = fall_count * obj->weight / 2;

            /*
             * Damage players
             */
            if ( obj->in_room->first_person && number_percent(  ) > 15 ) {
                CHAR_DATA              *rch;
                CHAR_DATA              *vch = NULL;
                int                     chcnt = 0;

                for ( rch = obj->in_room->first_person; rch; rch = rch->next_in_room, chcnt++ )
                    if ( number_range( 0, chcnt ) == 0 )
                        vch = rch;
                act( AT_WHITE, "¡$p cae sobre $n!", vch, obj, NULL, TO_ROOM );
                act( AT_WHITE, "¡$p cae sobre ti!", vch, obj, NULL, TO_CHAR );

                if ( IS_NPC( vch ) && xIS_SET( vch->act, ACT_HARDHAT ) )
                    act( AT_WHITE, "¡$p pasa rozándote la cabeza!", vch, obj, NULL,
                         TO_CHAR );
                else
                    damage( vch, vch, dam * vch->level, TYPE_UNDEFINED );
            }
            /*
             * Damage objects
             */
            switch ( obj->item_type ) {
                case ITEM_WEAPON:
                case ITEM_ARMOR:
                    if ( ( obj->value[0] - dam ) <= 0 ) {
                        if ( obj->in_room->first_person ) {
                            act( AT_PLAIN, "¡$p se hace añicos por la caída!",
                                 obj->in_room->first_person, obj, NULL, TO_ROOM );
                            act( AT_PLAIN, "¡$p se hace añicos por la caída!",
                                 obj->in_room->first_person, obj, NULL, TO_CHAR );
                        }
                        make_scraps( obj );
                    }
                    else
                        obj->value[0] -= dam;
                    break;
                default:
                    if ( ( dam * 15 ) > get_obj_resistance( obj ) ) {
                        if ( obj->in_room->first_person ) {
                            act( AT_PLAIN, "¡$p se hace añicos por la caída!",
                                 obj->in_room->first_person, obj, NULL, TO_ROOM );
                            act( AT_PLAIN, "¡$p se hace añicos por la caída!",
                                 obj->in_room->first_person, obj, NULL, TO_CHAR );
                        }
                        make_scraps( obj );
                    }
                    break;
            }
        }
        obj_fall( obj, TRUE );
    }
    return;
}

/* Scryn, by request of Darkur, 12/04/98 */
/* Reworked recursive_note_find to fix crash bug when the note was left
 * blank.  7/6/98 -- Shaddai
 */

void do_findnote( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;

    if ( IS_NPC( ch ) ) {
        error( ch );
        return;
    }

    if ( argument[0] == '\0' ) {
        send_to_char( "Debes especificar al menos una palabra clave.\r\n", ch );
        return;
    }

    obj = recursive_note_find( ch->first_carrying, argument );

    if ( obj ) {
        if ( obj->in_obj ) {
            obj_from_obj( obj );
            obj = obj_to_char( obj, ch );
        }
        wear_obj( ch, obj, TRUE, -1 );
    }
    else
        send_to_char( "Nota no encontrada.\r\n", ch );
    return;
}

OBJ_DATA               *recursive_note_find( OBJ_DATA *obj, char *argument )
{
    OBJ_DATA               *returned_obj;
    bool                    match = TRUE;
    char                   *argcopy;
    char                   *subject;
    char                    arg[MIL];
    char                    subj[MSL];

    if ( !obj )
        return NULL;

    switch ( obj->item_type ) {
        case ITEM_PAPER:

            if ( ( subject = get_extra_descr( "_subject_", obj->first_extradesc ) ) == NULL )
                break;
            snprintf( subj, MSL, "%s", strlower( subject ) );
            subject = strlower( subj );

            argcopy = argument;

            while ( match ) {
                argcopy = one_argument( argcopy, arg );

                if ( arg[0] == '\0' )
                    break;

                if ( !strstr( subject, arg ) )
                    match = FALSE;
            }

            if ( match )
                return obj;
            break;

        case ITEM_CONTAINER:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
            if ( obj->first_content ) {
                returned_obj = recursive_note_find( obj->first_content, argument );
                if ( returned_obj )
                    return returned_obj;
            }
            break;

        default:
            break;
    }

    return recursive_note_find( obj->next_content, argument );
}

void do_rolldie( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *die;

    char                    output_string[MAX_STRING_LENGTH];
    char                    roll_string[MAX_STRING_LENGTH];
    char                    total_string[MAX_STRING_LENGTH];

    const char             *verb;

    int                     rollsum = 0;
    int                     roll_count = 0;

    int                     numsides;
    int                     numrolls;

    bool                   *face_seen_table = NULL;

    if ( IS_NPC( ch ) ) {
        send_to_char( "¿Qué?\r\n", ch );
        return;
    }

    if ( ( die = get_eq_char( ch, WEAR_HOLD ) ) == NULL || die->item_type != ITEM_CHANCE ) {
        ch_printf( ch, "¡Necesitas sostener un objeto de posibilidades!\r\n" );
        return;
    }

    numrolls = ( is_number( argument ) ) ? atoi( argument ) : 1;
    verb = get_chance_verb( die );

    if ( numrolls > 100 ) {
        ch_printf( ch, "¡No puedes %s más de 100 veces!\r\n", verb );
        return;
    }

    numsides = die->value[0];

    if ( numsides <= 1 ) {
        ch_printf( ch, "There is no element of chance in this game!\r\n" );
        return;
    }

    if ( die->value[3] == 1 ) {
        if ( numrolls > numsides ) {
            ch_printf( ch, "buen intento, pero solo puedes %s %d veces.\r\n", verb, numsides );
            return;
        }
        face_seen_table = ( bool * ) calloc( numsides, sizeof( bool ) );
        if ( !face_seen_table ) {
            bug( "%s",
                 "do_rolldie: cannot allocate memory for face_seen_table array, terminating.\r\n" );
            return;
        }
    }

    snprintf( roll_string, MAX_STRING_LENGTH, "%s", " " );

    while ( roll_count++ < numrolls ) {
        int                     current_roll;
        char                    current_roll_string[MAX_STRING_LENGTH];

        do {
            current_roll = number_range( 1, numsides );
        }
        while ( die->value[3] == 1 && face_seen_table[current_roll - 1] == TRUE );

        if ( die->value[3] == 1 )
            face_seen_table[current_roll - 1] = TRUE;

        rollsum += current_roll;

        if ( roll_count > 1 )
            mudstrlcat( roll_string, ", ", MAX_STRING_LENGTH );
        if ( numrolls > 1 && roll_count == numrolls )
            mudstrlcat( roll_string, "and ", MAX_STRING_LENGTH );

        if ( die->value[1] == 1 ) {
            char                   *face_name = get_ed_number( die, current_roll );

            if ( face_name ) {
                char                   *face_name_copy = strdup( face_name );   /* Since
                                                                                 * I want
                                                                                 * * * *
                                                                                 * * * *
                                                                                 * to *
                                                                                 * tokenize
                                                                                 * * * *
                                                                                 * * * *
                                                                                 * without
                                                                                 * * * * *
                                                                                 * modifying
                                                                                 * * * * *
                                                                                 * * * the
                                                                                 * *
                                                                                 * original
                                                                                 * * * *
                                                                                 * string */
                snprintf( current_roll_string, MAX_STRING_LENGTH, "%s",
                          strtok( face_name_copy, "\n" ) );
                free( face_name_copy );
            }
            else
                snprintf( current_roll_string, MAX_STRING_LENGTH, "%d", current_roll );
        }
        else
            snprintf( current_roll_string, MAX_STRING_LENGTH, "%d", current_roll );
        mudstrlcat( roll_string, current_roll_string, MAX_STRING_LENGTH );
    }

    if ( numrolls > 1 && die->value[2] == 1 ) {
        snprintf( total_string, MAX_STRING_LENGTH, ", for a total of %d", rollsum );
        mudstrlcat( roll_string, total_string, MAX_STRING_LENGTH );
    }

    mudstrlcat( roll_string, ".\r\n", MAX_STRING_LENGTH );

    snprintf( output_string, MAX_STRING_LENGTH, "You %s%s", verb, roll_string );
    act( AT_GREEN, output_string, ch, NULL, NULL, TO_CHAR );

    snprintf( output_string, MAX_STRING_LENGTH, "$n %s%s", verb, roll_string );
    act( AT_GREEN, output_string, ch, NULL, NULL, TO_ROOM );

    if ( face_seen_table )
        free( face_seen_table );
    return;
}

char                   *get_ed_number( OBJ_DATA *obj, int number )
{
    EXTRA_DESCR_DATA       *ed;
    int                     count;

    for ( ed = obj->first_extradesc, count = 1; ed; ed = ed->next, count++ ) {
        if ( count == number )
            return ed->description;
    }

    return NULL;
}

/*dice chance deal throw*/
const char             *get_chance_verb( OBJ_DATA *obj )
{
    return ( obj->action_desc[0] != '\0' ) ? obj->action_desc : "roll$q";
}

/* Turn an object into scraps. -Thoric */
void make_scraps( OBJ_DATA *obj )
{
    char                    buf[MSL];
    OBJ_DATA               *scraps,
                           *tmpobj;
    CHAR_DATA              *ch = NULL;

    separate_obj( obj );
    scraps = create_object( get_obj_index( OBJ_VNUM_SCRAPS ), 0 );
    scraps->timer = number_range( 5, 15 );
    /*
     * don't make scraps of scraps of scraps of ...
     */
    if ( obj->pIndexData->vnum == OBJ_VNUM_SCRAPS ) {
        if ( VLD_STR( scraps->short_descr ) )
            STRFREE( scraps->short_descr );
        scraps->short_descr = STRALLOC( "algunos escombros" );
        if ( VLD_STR( scraps->description ) )
            STRFREE( scraps->description );
        scraps->description = STRALLOC( "Una pila de escombros tirada por el suelo." );
    }
    else {
        snprintf( buf, MSL, scraps->short_descr, obj->short_descr );
        if ( VLD_STR( scraps->short_descr ) )
            STRFREE( scraps->short_descr );
        scraps->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, scraps->description, obj->short_descr );
        if ( VLD_STR( scraps->description ) )
            STRFREE( scraps->description );
        scraps->description = STRALLOC( buf );
    }
    if ( obj->carried_by ) {
        act( AT_OBJECT, "¡$p cae al suelo, convirtiéndose en escombros!", obj->carried_by, obj, NULL, TO_CHAR );
        if ( obj == get_eq_char( obj->carried_by, WEAR_WIELD )
             && ( tmpobj = get_eq_char( obj->carried_by, WEAR_DUAL_WIELD ) ) != NULL )
            tmpobj->wear_loc = WEAR_WIELD;
        obj_to_room( scraps, obj->carried_by->in_room );
    }
    else if ( obj->in_room ) {
        if ( ( ch = obj->in_room->first_person ) != NULL ) {
            act( AT_OBJECT, "$p se convierte en una pequeña pila de escombros.", ch, obj, NULL, TO_ROOM );
            act( AT_OBJECT, "$p se convierte en una pequeña pila de escombros.", ch, obj, NULL, TO_CHAR );
        }
        obj_to_room( scraps, obj->in_room );
    }
    if ( ( obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_KEYRING
           || obj->item_type == ITEM_QUIVER || obj->item_type == ITEM_CORPSE_PC
           || obj->item_type == ITEM_SHEATH ) && obj->first_content ) {
        if ( ch && ch->in_room ) {
            act( AT_OBJECT, "El contenido de $p se desparrama por el suelo.", ch, obj, NULL, TO_ROOM );
            act( AT_OBJECT, "El contenido de $p se desparrama por el suelo.", ch, obj, NULL, TO_CHAR );
        }
        if ( obj->carried_by )
            empty_obj( obj, NULL, obj->carried_by->in_room, FALSE );
        else if ( obj->in_room )
            empty_obj( obj, NULL, obj->in_room, FALSE );
        else if ( obj->in_obj )
            empty_obj( obj, obj->in_obj, NULL, FALSE );
    }
    extract_obj( obj );
}

/* make some coinage */
OBJ_DATA               *create_money( int amount, int type )
{
    char                    buf[MSL];
    OBJ_DATA               *obj;

    if ( amount <= 0 ) {
        bug( "%s: zero or negative money %d.", __FUNCTION__, amount );
        amount = 1;
    }

    if ( type < FIRST_CURR || type > LAST_CURR ) {
        bug( "Create_money: invalid currency type %d.", type );
        amount = 1;
        type = CURR_GOLD;
    }

    if ( amount == 1 ) {
        if ( !( obj = create_object( get_obj_index( OBJ_VNUM_MONEY_ONE ), 0 ) ) ) {
            bug( "%s: couldn't create object vnum %d", __FUNCTION__, OBJ_VNUM_MONEY_ONE );
            return NULL;
        }
        snprintf( buf, MSL, obj->name, curr_types[type] );
        STRFREE( obj->name );
        obj->name = STRALLOC( buf );
        snprintf( buf, MSL, obj->short_descr, curr_types[type] );
        STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, obj->description, curr_types[type] );
        STRFREE( obj->description );
        obj->description = STRALLOC( buf );
        obj->value[2] = type;
    }
    else {
        if ( amount < 100 )
            obj = create_object( get_obj_index( OBJ_VNUM_MONEY_SOME ), 0 );
        else if ( amount < 500 )
            obj = create_object( get_obj_index( OBJ_VNUM_MONEY_LOTS ), 0 );
        else if ( amount < 2000 )
            obj = create_object( get_obj_index( OBJ_VNUM_MONEY_HEAPS ), 0 );
        else if ( amount < 10000 )
            obj = create_object( get_obj_index( OBJ_VNUM_MONEY_PILE ), 0 );
        else if ( amount < 100000 )
            obj = create_object( get_obj_index( OBJ_VNUM_MONEY_MASS ), 0 );
        else if ( amount < 1000000 )
            obj = create_object( get_obj_index( OBJ_VNUM_MONEY_MILLS ), 0 );
        else
            obj = create_object( get_obj_index( OBJ_VNUM_MONEY_INF ), 0 );
        snprintf( buf, MSL, obj->name, curr_types[type] );
        STRFREE( obj->name );
        obj->name = STRALLOC( buf );
        snprintf( buf, MSL, obj->short_descr, amount, curr_types[type] );
        STRFREE( obj->short_descr );
        obj->short_descr = STRALLOC( buf );
        snprintf( buf, MSL, obj->description, curr_types[type] );
        STRFREE( obj->description );
        obj->description = STRALLOC( buf );
        obj->value[0] = amount;
        obj->value[2] = type;
    }

    return obj;
}

void weapon_type( CHAR_DATA *ch, int value )
{
    bool                    found = FALSE;
    char                    wbuf[MSL];

    /*
     * It's a bludgeon! -Orion
     */
    if ( !found && ( value == 7 || value == 8 ) ) {
        mudstrlcpy( wbuf, "Bludgeon", MSL );
        found = TRUE;
    }

    /*
     * It's a flexible arm! -Orion
     */
    if ( !found && ( value == 4 ) ) {
        mudstrlcpy( wbuf, "Flexible Arm", MSL );
        found = TRUE;
    }

    /*
     * It's a long blade! -Orion
     */
    if ( !found && ( value == 1 || value == 3 ) ) {
        mudstrlcpy( wbuf, "Long Blade", MSL );
        found = TRUE;
    }

    /*
     * It's a missile weapon! -Orion
     */
    if ( !found && ( value == 13 || value == 14 || value == 15 || value == 16 || value == 16 ) ) {
        mudstrlcpy( wbuf, "Missile Weapon", MSL );
        found = TRUE;
    }

    /*
     * It's pugilistic! -Orion
     */
    if ( !found && ( value == 0 || value == 6 || value == 10 || value == 12 ) ) {
        mudstrlcpy( wbuf, "Pugilistic", MSL );
        found = TRUE;
    }

    /*
     * It's a short blade! -Orion
     */
    if ( !found && ( value == 2 || value == 11 ) ) {
        mudstrlcpy( wbuf, "Short Blade", MSL );
        found = TRUE;
    }

    /*
     * It's a talonous arm! -Orion
     */
    if ( !found && ( value == 5 ) ) {
        mudstrlcpy( wbuf, "Talonous Arm", MSL );
        found = TRUE;
    }

    /*
     * Well, we checked all the normal weapon types, and this...
     * just isn't there. -Orion
     */
    if ( !found )
        mudstrlcpy( wbuf, "Not A Clue", MSL );

    ch_printf( ch, "Weapon Type: %s\r\n", wbuf );
    return;
}

/* Connect pieces of an ITEM -- Originally from ACK!  *
 * Modified for Smaug by Zarius 5/19/2000             */
void do_connect( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *first_ob;
    OBJ_DATA               *second_ob;
    OBJ_DATA               *new_ob;
    char                    arg1[MSL],
                            arg2[MSL];

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Sintaxis: Combinar <primer objeto> <segundo objeto>.\r\n", ch );
        return;
    }

    if ( ( first_ob = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "Necesitas tener ambas partes en tu inventario.\r\n", ch );
        send_to_char( "\r\nAsegúrate de tener los objetos necesarios en el inventario.\r\n", ch );
        return;
    }

    if ( ( second_ob = get_obj_carry( ch, arg2 ) ) == NULL ) {
        send_to_char( "Necesitas tener ambas partes en tu inventario.\r\n", ch );
        send_to_char( "\r\nAsegúrate de tener los objetos necesarios en el inventario.\r\n", ch );
        return;
    }

    if ( first_ob->item_type != ITEM_PIECE || second_ob->item_type != ITEM_PIECE ) {
        send_to_char( "¡Ambos objetos deben ser pieza de otro objeto!\r\n", ch );
        return;
    }

    /*
     * check to see if the pieces connect
     */

    if ( ( first_ob->value[0] == second_ob->pIndexData->vnum )
         || ( first_ob->value[1] == second_ob->pIndexData->vnum ) )
        /*
         * good connection
         */
    {
        new_ob = create_object( get_obj_index( first_ob->value[2] ), ch->level );
        extract_obj( first_ob );
        extract_obj( second_ob );
        obj_to_char( new_ob, ch );
        act( AT_ACTION,
             "$n ensambla algunas piezas...\r\n ...¡Creando $p!",
             ch, new_ob, NULL, TO_ROOM );
        act( AT_ACTION,
             "Ensamblas algunas piezas...\r\n ... ¡creando $p!",
             ch, new_ob, NULL, TO_CHAR );

    }
    else {
        act( AT_ACTION, "$n ensambla algunas piezas, pero no logra crear ningún objeto.", ch,
             NULL, NULL, TO_ROOM );
        act( AT_ACTION,
             "Intentas ensamblar algunas piezas para crear un objeto... Pero no logras nada que tenga sentido.",
             ch, NULL, NULL, TO_CHAR );

        return;
    }

    return;
}

//This need to be completely rewrote -Taon.

void update_weapon( CHAR_DATA *ch, OBJ_DATA *obj )
{
    int                     mod,
                            nummod,
                            dammod,
                            damdice;

    if ( !CAN_SHARPEN( obj ) )
        return;

    if ( !obj->value[6] )
        return;

    if ( obj->value[6] > 100 || obj->value[6] < 1 )
        return;

    damdice = obj->value[2];

    if ( obj->value[6] < 30 ) {
        if ( obj->value[1] > 1 && obj->value[2] > 1 ) {
            obj->value[2] = damdice - 1;
            return;
        }
        else
            obj->value[2] = damdice;
        return;
    }
    if ( obj->value[6] >= 30 && obj->value[6] <= 50 ) { /* Weapon is neither sharp nor *
                                                         * blunt * - make it normal and *
                                                         * * * * * * return. */
        obj->value[2] = damdice;
        return;
    }
    if ( obj->value[6] > 50 && obj->value[6] <= 70 ) {
        obj->value[2] = damdice + 2;
        return;
    }
    if ( obj->value[6] > 70 && obj->value[6] <= 90 ) {
        obj->value[2] = damdice + 3;
        return;
    }
    if ( obj->value[6] > 90 && obj->value[6] <= 99 ) {
        obj->value[2] = damdice + 3;
        return;
    }
    if ( obj->value[6] > 90 && obj->value[6] <= 99 ) {
        obj->value[2] = damdice + 3;
        return;
    }
    if ( obj->value[6] == 100 ) {
        obj->value[2] = damdice + 4;
        return;
    }

    return;
}

void identify_object( CHAR_DATA *ch, OBJ_DATA *obj )
{
    AFFECT_DATA            *paf;
    SKILLTYPE              *sktmp;
    char                    buf[MSL];
    EXTRA_DESCR_DATA       *ed;
    int                     count,
                            wflags = 0;

    if ( !obj || !ch ) {
        bug( "%s: NULL ch or NULL obj", __FUNCTION__ );
        return;
    }

    separate_obj( obj );
    set_char_color( AT_LBLUE, ch );
    for ( ed = obj->first_extradesc, count = 1; ed; ed = ed->next, count++ ) {
        if ( count == 1 )
            ch_printf( ch, "%s", ed->description );
    }
    ch_printf( ch, "%s parece ser un(a) %s.\r\n", capitalize( obj->short_descr ),
               aoran( item_type_name( obj ) ) );
    if ( obj->item_type == ITEM_TREASURE )
        send_to_char( "Esto es un tesoro - ¡No puedes vestir un tesoro!\r\n", ch );
    else
        ch_printf( ch, "Este objeto se viste en: %s\r\n", wear_bit_name( obj->wear_flags ) );

    ch_printf( ch, "Propiedades especiales:  %s\r\nsu peso es %d y su nivel es %d.\r\n",
               !xIS_EMPTY( obj->extra_flags ) ? ext_flag_string( &obj->extra_flags,
                                                                 o_flags ) : "Ninguna", obj->weight,
               obj->level );

    /*
     * Only show size needed if can wear it
     */
    wflags = obj->wear_flags;
    if ( IS_SET( wflags, ITEM_TAKE ) )                 /* Strip take first so it just * *
                                                        * checks other wear * locations */
        REMOVE_BIT( wflags, ITEM_TAKE );
    if ( wflags != 0 )
        ch_printf( ch, "Tamaño necesario para poder vestir el objeto: %s\r\n", obj_sizes[obj->size] );

    if ( obj->timer )
        ch_printf( ch, "Este objeto expira en %d ticks.\r\n", obj->timer );

    if ( obj->currtype && obj->currtype < MAX_CURR_TYPE ) {
        if ( obj->cost > 0 ) {
            snprintf( buf, MSL, "Este objeto tiene un valor aproximado de %d monedas de %s .\r\n", obj->cost,
                      curr_types[obj->currtype] );
            act( AT_LBLUE, buf, ch, obj, NULL, TO_CHAR );
        }
        else
            send_to_char( "El valor de este objeto es desconocido.\r\n", ch );

    }
    else {
        bug( "Bad Curr_Type - obj vnum %d [ This is likely a bug with the money code itself, not objects, or the identify spell.  It will be fixed when time permits", obj->pIndexData->vnum );
        return;
    }

    set_char_color( AT_MAGIC, ch );
    switch ( obj->item_type ) {

        default:
            break;
        case ITEM_TREASURE:
            ch_printf( ch, "%s parece ser un(a) %s.\r\n", capitalize( obj->short_descr ),
                       obj->value[2] ==
                       0 ? "que ofrece una leve ayuda a la curación de los dragones." : obj->value[2] ==
                       1 ? "que ofrece una ligera ayuda a la curación de los dragones." : obj->value[2] ==
                       2 ? "que ofrece una excelente ayuda a la curación de los dragones." : obj->value[2] ==
                       3 ? "que ofrece la máxima ayuda a la curación de los dragones." :
                       "no ofrece ayuda a la curación de los dragones." );
            break;
        case ITEM_CONTAINER:
            ch_printf( ch, "%s parece ser un(a) %s.\r\n", capitalize( obj->short_descr ),
                       obj->value[0] < 76 ? "pequeño" :
                       obj->value[0] < 150 ? "de poca capacidad" :
                       obj->value[0] < 300 ? "de media capacidad" : obj->value[0] <
                       550 ? "de gran capacidad" : obj->value[0] <
                       751 ? "de capacidad enorme" : "de enorme capacidad" );
            if ( IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                ch_printf( ch, "Flags actuales: %s\r\n",
                           flag_string( obj->pIndexData->value[1], container_flags ) );
            }
            else if ( !IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) ) {
                ch_printf( ch, "Flags actuales: %s\r\n",
                           flag_string( obj->value[1], container_flags ) );
            }
            break;
        case ITEM_PILL:
        case ITEM_SCROLL:
        case ITEM_POTION:
            ch_printf( ch, "Nivel %d conjuros de:", obj->value[0] );
            if ( obj->value[1] >= 0 && ( sktmp = get_skilltype( obj->value[1] ) ) != NULL ) {
                send_to_char( " '", ch );
                send_to_char( sktmp->name, ch );
                send_to_char( "'", ch );
            }
            if ( obj->value[2] >= 0 && ( sktmp = get_skilltype( obj->value[2] ) ) != NULL ) {
                send_to_char( " '", ch );
                send_to_char( sktmp->name, ch );
                send_to_char( "'", ch );
            }
            if ( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != NULL ) {
                send_to_char( " '", ch );
                send_to_char( sktmp->name, ch );
                send_to_char( "'", ch );
            }
            send_to_char( ".\r\n", ch );
            break;
        case ITEM_SALVE:
            ch_printf( ch, "Tiene %d(%d) aplicaciones de nivel %d", obj->value[1], obj->value[2],
                       obj->value[0] );
            if ( obj->value[4] >= 0 && ( sktmp = get_skilltype( obj->value[4] ) ) != NULL ) {
                send_to_char( " '", ch );
                send_to_char( sktmp->name, ch );
                send_to_char( "'", ch );
            }
            if ( obj->value[6] >= 0 && ( sktmp = get_skilltype( obj->value[6] ) ) != NULL ) {
                send_to_char( " '", ch );
                send_to_char( sktmp->name, ch );
                send_to_char( "'", ch );
            }
            send_to_char( ".\r\n", ch );
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            ch_printf( ch, "Tiene %d(%d) cargas de nivel %d", obj->value[1], obj->value[2],
                       obj->value[0] );
            if ( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != NULL ) {
                send_to_char( " '", ch );
                send_to_char( sktmp->name, ch );
                send_to_char( "'", ch );
            }
            send_to_char( ".\r\n", ch );
            break;
        case ITEM_WEAPON:
            ch_printf( ch, "El daño es entre %d y %d (media de %d)%s\r\n", obj->value[1], obj->value[2],
                       ( obj->value[1] + obj->value[2] ) / 2, IS_OBJ_STAT( obj,
                                                                           ITEM_POISONED ) ?
                       "es un arma venenosa." : "." );
            if ( CAN_SHARPEN( obj ) ) {
                const char             *txt;

                if ( obj->value[6] == 100 )
                    txt = "estar lo más afilado posible";
                else if ( obj->value[6] > 80 )
                    txt = "estar extramadamente afilado";
                else if ( obj->value[6] > 60 )
                    txt = "estar decentemente afilado";
                else if ( obj->value[6] > 40 )
                    txt = "estar afilado";
                else if ( obj->value[6] > 20 )
                    txt = "necesitar una afilada";
                else if ( obj->value[6] > 10 )
                    txt = "estar con poco filo";
                else if ( obj->value[6] > 0 )
                    txt = "estar extremadamente desafilado";
                else
                    txt = "estar sin afilar";
                ch_printf( ch, "Su filo parece &c%s&D%s\r\n", txt,
                           obj->value[6] > 80 ? "!" : "." );

            }
            ch_printf( ch, "Habilidad requerida: %s\r\n", weapon_skills[obj->value[4]] );
            ch_printf( ch, "Tipo de daño:  %s\r\n", capitalize( attack_table[obj->value[3]] ) );
            break;
        case ITEM_MISSILE_WEAPON:
            ch_printf( ch, "La bonificación de daño añadido a los proyectiles es de %d a %d (media de %d).\r\n",
                       obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2 );
            ch_printf( ch, "Rango: %d yardas\r\n", obj->value[3] * 20 );
            ch_printf( ch, "Habilidad requerida:      %s\r\n", weapon_skills[obj->value[4]] );
            ch_printf( ch, "Proyectiles disparados: %s\r\n", projectiles[obj->value[6]] );
            break;
        case ITEM_PROJECTILE:
            ch_printf( ch, "El daño es de %d a %d (media de %d)%s\r\n", obj->value[1], obj->value[2],
                       ( obj->value[1] + obj->value[2] ) / 2, IS_OBJ_STAT( obj,
                                                                           ITEM_POISONED ) ?
                       ", y este proyectil está envenenado." : "." );
            ch_printf( ch, "Tipo de daño: %s\r\n", attack_table[obj->value[3]] );
            ch_printf( ch, "Tipo de proyectil: %s\r\n", projectiles[obj->value[4]] );
            break;
        case ITEM_ARMOR:
            ch_printf( ch, "Protege como armadura en %d.\r\n", obj->value[0] );
            break;
    }
    for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
        showaffect( ch, paf );
    for ( paf = obj->first_affect; paf; paf = paf->next )
        showaffect( ch, paf );
    return;
}

void                    do_mpgenmob( CHAR_DATA *ch, char *argument );

/*
 * Make a trap.
 */
OBJ_DATA               *make_trap( int v0, int v1, int v2, int v3 )
{
    OBJ_DATA               *trap;

    trap = create_object( get_obj_index( OBJ_VNUM_TRAP ), 0 );
    trap->timer = 0;
    trap->value[0] = v0;
    trap->value[1] = v1;
    trap->value[2] = v2;
    trap->value[3] = v3;
    return trap;
}
