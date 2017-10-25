/* Copyright 2014, 2015 Jesús Pavón Abián
Programa bajo licencia GPL. Encontrará una copia de la misma en el archivo COPYING.txt que se encuentra en la raíz de los fuentes. */
/***************************************************************************
 * - Chronicles Copyright 2001, 2002 by Brad Ensley (Orion Elder)          *
 * - SMAUG 1.4  Copyright 1994, 1995, 1996, 1998 by Derek Snider           *
 * - Merc  2.1  Copyright 1992, 1993 by Michael Chastain, Michael Quan,    *
 *   and Mitchell Tse.                                                     *
 * - DikuMud    Copyright 1990, 1991 by Sebastian Hammer, Michael Seifert, *
 *   Hans-Henrik Stærfeldt, Tom Madsen, and Katja Nyboe.                   *
 ***************************************************************************
 * - Miscellaneous module                                                  *
 ***************************************************************************/

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include "h/mud.h"
#include <dirent.h>
#include <stdio.h>
#include "h/files.h"

extern int              top_exit;

/*
 * Local function
 */
void make_bloodstain    args( ( CHAR_DATA *ch ) );

/*
 * Fill a container
 * Many enhancements added by Thoric (ie: filling non-drink containers)
 */
void do_fill( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    OBJ_DATA               *obj;
    OBJ_DATA               *source;
    short                   dest_item,
                            src_item1,
                            src_item2,
                            src_item3;
    int                     diff = 0;
    bool                    all = FALSE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    /*
     * munch optional words 
     */
    if ( ( !str_cmp( arg2, "de" ) || !str_cmp( arg2, "with" ) ) && argument[0] != '\0' )
        argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "¿Llenar qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "No tienes ese objeto.\r\n", ch );
        return;
    }
    else
        dest_item = obj->item_type;

    src_item1 = src_item2 = src_item3 = -1;
    switch ( dest_item ) {
        default:
            act( AT_ACTION, "$n Intenta llenar $p... (No me preguntes como)", ch, obj, NULL, TO_ROOM );
            send_to_char( "No puedes llenar ese recipiente.\r\n", ch );
            return;
            /*
             * place all fillable item types here 
             */
        case ITEM_DRINK_CON:
            src_item1 = ITEM_FOUNTAIN;
            src_item2 = ITEM_BLOOD;
            break;
        case ITEM_HERB_CON:
            src_item1 = ITEM_HERB;
            src_item2 = ITEM_HERB_CON;
            break;
        case ITEM_PIPE:
            src_item1 = ITEM_HERB;
            src_item2 = ITEM_HERB_CON;
            break;
        case ITEM_CONTAINER:
            src_item1 = ITEM_CONTAINER;
            src_item2 = ITEM_CORPSE_NPC;
            src_item3 = ITEM_CORPSE_PC;
            break;
        case ITEM_QUIVER:
            src_item1 = ITEM_CONTAINER;
            src_item2 = ITEM_CORPSE_NPC;
            src_item3 = ITEM_CORPSE_PC;
            break;
    }

    if ( dest_item == ITEM_CONTAINER || dest_item == ITEM_QUIVER ) {
        if ( IS_SET( obj->value[1], CONT_CLOSED ) ) {
            act( AT_PLAIN, "Necesitas abrir $p.", ch, NULL, obj->name, TO_CHAR );
            return;
        }
        if ( get_real_obj_weight( obj ) / obj->count >= obj->value[0] ) {
            send_to_char( "Se ha llenado completamente.\r\n", ch );
            return;
        }
    }
    else {
        diff = obj->value[0] - obj->value[1];
        if ( diff < 1 || obj->value[1] >= obj->value[0] ) {
            send_to_char( "Se ha llenado completamente.\r\n", ch );
            return;
        }
    }

    if ( dest_item == ITEM_PIPE && IS_SET( obj->value[3], PIPE_FULLOFASH ) ) {
        send_to_char( "Se ha llenado completamente. se necesita vaciar primero.\r\n", ch );
        return;
    }

    if ( arg2[0] != '\0' ) {
        if ( ( dest_item == ITEM_CONTAINER || dest_item == ITEM_QUIVER )
             && ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) ) ) {
            all = TRUE;
            source = NULL;
        }
        else
            /*
             * This used to let you fill a pipe from an object on the ground.  Seems
             * to me you should be holding whatever you want to fill a pipe with.
             * It's nitpicking, but I needed to change it to get a mobprog to work
             * right.  Check out Lord Fitzgibbon if you're curious.  -Narn 
             */
        if ( dest_item == ITEM_PIPE ) {
            if ( ( source = get_obj_carry( ch, arg2 ) ) == NULL ) {
                send_to_char( "No tienes ese objeto.\r\n", ch );
                return;
            }
            if ( source->item_type != src_item1 && source->item_type != src_item2
                 && source->item_type != src_item3 ) {
                act( AT_PLAIN, "No puedes llenar $p con $P!", ch, obj, source, TO_CHAR );
                return;
            }
        }
        else {
            if ( ( source = get_obj_here( ch, arg2 ) ) == NULL ) {
                send_to_char( "No encuentras ese objeto.\r\n", ch );
                return;
            }
        }
    }
    else
        source = NULL;

    if ( !source && dest_item == ITEM_PIPE ) {
        send_to_char( "¿Qué llenarás?\r\n", ch );
        return;
    }

    if ( !source ) {
        bool                    found = FALSE;
        OBJ_DATA               *src_next;

        found = FALSE;
        separate_obj( obj );
        for ( source = ch->in_room->first_content; source; source = src_next ) {
            src_next = source->next_content;
            if ( dest_item == ITEM_CONTAINER || dest_item == ITEM_QUIVER ) {
                if ( !CAN_WEAR( source, ITEM_TAKE )
                     || IS_OBJ_STAT( source, ITEM_BURIED )
                     || ( IS_OBJ_STAT( source, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
                     || ch->carry_weight + get_obj_weight( source, FALSE ) > can_carry_w( ch )
                     || ( get_real_obj_weight( source ) +
                          get_real_obj_weight( obj ) / obj->count ) > obj->value[0] )
                    continue;
                if ( all && arg2[3] == '.' && !nifty_is_name( &arg2[4], source->name ) )
                    continue;
                if ( source->pIndexData->vnum == OBJ_VNUM_OPORTAL )
                    continue;

                /*
                 * Only allow projectiles to go in quivers 
                 */
                if ( dest_item == ITEM_QUIVER && source->item_type != ITEM_PROJECTILE )
                    continue;

                obj_from_room( source );
                if ( source->item_type == ITEM_MONEY ) {
                    GET_MONEY( ch, source->value[2] ) += source->value[0];
                    extract_obj( source );
                }
                else
                    obj_to_obj( source, obj );
                found = TRUE;
            }
            else if ( source->item_type == src_item1 || source->item_type == src_item2
                      || source->item_type == src_item3 ) {
                found = TRUE;
                break;
            }
        }
        if ( !found ) {
            switch ( src_item1 ) {
                default:
                    send_to_char( "¡No hay nada apropiado here!\r\n", ch );
                    return;
                case ITEM_FOUNTAIN:
                    send_to_char( "No hay fuente o piscina por aquí here!\r\n", ch );
                    return;
                case ITEM_BLOOD:
                    send_to_char( "No hay ninguna fuente de sangre here!\r\n", ch );
                    return;
                case ITEM_HERB_CON:
                    send_to_char( "No hay hierbas aquí!\r\n", ch );
                    return;
                case ITEM_HERB:
                    send_to_char( "No encuentras hierba para fumar.\r\n", ch );
                    return;
            }
        }
        if ( dest_item == ITEM_CONTAINER || dest_item == ITEM_QUIVER ) {
            act( AT_ACTION, "Llenas  $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n llena $p.", ch, obj, NULL, TO_ROOM );
            return;
        }
    }

    if ( dest_item == ITEM_CONTAINER || dest_item == ITEM_QUIVER ) {
        OBJ_DATA               *otmp,
                               *otmp_next;
        char                    name[MIL];
        CHAR_DATA              *gch;
        char                   *pd;
        bool                    found = FALSE;

        if ( source == obj ) {
            send_to_char( "No se puede llenar por sí mismo!\r\n", ch );
            return;
        }

        switch ( source->item_type ) {
            default:                                  /* put something in container */
                if ( !source->in_room                  /* disallow inventory items */
                     || !CAN_WEAR( source, ITEM_TAKE )
                     || ( IS_OBJ_STAT( source, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
                     || ch->carry_weight + get_obj_weight( source, FALSE ) > can_carry_w( ch )
                     || ( get_real_obj_weight( source ) +
                          get_real_obj_weight( obj ) / obj->count ) > obj->value[0]
                     || ( obj->item_type == ITEM_QUIVER && source->item_type != ITEM_PROJECTILE ) ) {
                    send_to_char( "No puedes hacer eso.\r\n", ch );
                    return;
                }
                separate_obj( obj );
                act( AT_ACTION, "Coges  $P y lo pones en  $p.", ch, obj, source, TO_CHAR );
                act( AT_ACTION, "$n coge $P y lo pone en $p.", ch, obj, source, TO_ROOM );
                obj_from_room( source );
                obj_to_obj( source, obj );
                break;

            case ITEM_MONEY:
                send_to_char( "No puedes hacer eso todavía.\r\n", ch );
                break;

            case ITEM_CORPSE_PC:
                if ( IS_NPC( ch ) ) {
                    send_to_char( "No puedes hacer eso.\r\n", ch );
                    return;
                }
                if ( IS_OBJ_STAT( source, ITEM_CLANCORPSE ) && !IS_IMMORTAL( ch ) ) {
                    send_to_char( "Your hands fumble.  Maybe you better loot a different way.\r\n",
                                  ch );
                    return;
                }
                if ( !IS_OBJ_STAT( source, ITEM_CLANCORPSE )
                     || !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) ) {
                    pd = source->short_descr;
                    pd = one_argument( pd, name );
                    pd = one_argument( pd, name );
                    pd = one_argument( pd, name );
                    pd = one_argument( pd, name );

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
                        if ( !fGroup ) {
                            send_to_char( "Ese es el cuerpo de alguien.\r\n", ch );
                            return;
                        }
                    }
                }
            case ITEM_CONTAINER:
                if ( source->item_type == ITEM_CONTAINER
                     && IS_SET( source->value[1], CONT_CLOSED ) ) {
                    act( AT_PLAIN, "$p se ha cerrado.", ch, NULL, source->name, TO_CHAR );
                    return;
                }

            case ITEM_QUIVER:
                if ( source->item_type == ITEM_QUIVER && IS_SET( source->value[1], CONT_CLOSED ) ) {
                    act( AT_PLAIN, "$p se ha cerrado.", ch, NULL, source->name, TO_CHAR );
                    return;
                }

            case ITEM_CORPSE_NPC:
                if ( ( otmp = source->first_content ) == NULL ) {
                    send_to_char( "Está vacío.\r\n", ch );
                    return;
                }
                separate_obj( obj );
                for ( ; otmp; otmp = otmp_next ) {
                    otmp_next = otmp->next_content;

                    if ( !CAN_WEAR( otmp, ITEM_TAKE )
                         || ( IS_OBJ_STAT( otmp, ITEM_PROTOTYPE ) && !can_take_proto( ch ) )
                         || ch->carry_number + otmp->count > can_carry_n( ch )
                         || ch->carry_weight + get_obj_weight( otmp, FALSE ) > can_carry_w( ch )
                         || ( get_real_obj_weight( source ) +
                              get_real_obj_weight( obj ) / obj->count ) > obj->value[0]
                         || ( obj->item_type == ITEM_QUIVER
                              && otmp->item_type != ITEM_PROJECTILE ) )
                        continue;
                    obj_from_obj( otmp );
                    obj_to_obj( otmp, obj );
                    found = TRUE;
                }
                if ( found ) {
                    act( AT_ACTION, "Llenas  $p de $P.", ch, obj, source, TO_CHAR );
                    act( AT_ACTION, "$n llena $p de $P.", ch, obj, source, TO_ROOM );
                }
                else
                    send_to_char( "No hay nada apropiado ahí.\r\n", ch );
                break;
        }
        return;
    }

    if ( source->count > 1 && source->item_type != ITEM_FOUNTAIN )
        separate_obj( source );

    separate_obj( obj );

    switch ( source->item_type ) {
        default:
            bug( "do_fill: got bad item type: %d", source->item_type );
            send_to_char( "Algo ha salido mal...\r\n", ch );
            return;

        case ITEM_FOUNTAIN:
            if ( obj->value[1] != 0 && obj->value[2] != 0 ) {
                send_to_char( "Eso ya contiene otro líquido.\r\n", ch );
                return;
            }
            obj->value[2] = 0;
            obj->value[1] = obj->value[0];
            act( AT_ACTION, "Llenas $p de $P.", ch, obj, source, TO_CHAR );
            act( AT_ACTION, "$n llena $p de $P.", ch, obj, source, TO_ROOM );
            return;

        case ITEM_BLOOD:
            if ( obj->value[1] != 0 && obj->value[2] != 13 ) {
                send_to_char( "Eso ya contiene otro líquido.\r\n", ch );
                return;
            }
            obj->value[2] = 13;
            if ( source->value[1] < diff )
                diff = source->value[1];
            obj->value[1] += diff;
            act( AT_ACTION, "Llenas $p de $P.", ch, obj, source, TO_CHAR );
            act( AT_ACTION, "$n llena $p de $P.", ch, obj, source, TO_ROOM );
            if ( ( source->value[1] -= diff ) < 1 ) {
                extract_obj( source );
                make_bloodstain( ch );
            }
            return;

        case ITEM_HERB:
            if ( obj->value[1] != 0 && obj->value[2] != source->value[2] ) {
                send_to_char( "Ya contiene otro tipo de hierba.\r\n", ch );
                return;
            }
            obj->value[2] = source->value[2];
            if ( source->value[1] < diff )
                diff = source->value[1];
            obj->value[1] += diff;
            act( AT_ACTION, "Llenas $p con $P.", ch, obj, source, TO_CHAR );
            act( AT_ACTION, "$n llena $p con $P.", ch, obj, source, TO_ROOM );
            if ( ( source->value[1] -= diff ) < 1 )
                extract_obj( source );
            return;

        case ITEM_HERB_CON:
            if ( obj->value[1] != 0 && obj->value[2] != source->value[2] ) {
                send_to_char( "Ya contiene hierba de otro tipo.\r\n", ch );
                return;
            }
            obj->value[2] = source->value[2];
            if ( source->value[1] < diff )
                diff = source->value[1];
            obj->value[1] += diff;
            source->value[1] -= diff;
            act( AT_ACTION, "Llenas $p de $P.", ch, obj, source, TO_CHAR );
            act( AT_ACTION, "$n llena $p de $P.", ch, obj, source, TO_ROOM );
            return;

        case ITEM_DRINK_CON:
            if ( obj->value[1] != 0 && obj->value[2] != source->value[2] ) {
                send_to_char( "Ya contiene líquido.\r\n", ch );
                return;
            }
            obj->value[2] = source->value[2];
            if ( source->value[1] < diff )
                diff = source->value[1];
            obj->value[1] += diff;
            source->value[1] -= diff;
            act( AT_ACTION, "Llenas $p de $P.", ch, obj, source, TO_CHAR );
            act( AT_ACTION, "$n llena $p de $P.", ch, obj, source, TO_ROOM );
            return;
    }
}

void do_drink( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;
    int                     amount;
    int                     liquid;
    AFFECT_DATA             af;

    argument = one_argument( argument, arg );
    /*
     * munch optional words 
     */
    if ( !str_cmp( arg, "de" ) && argument[0] != '\0' )
        argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        for ( obj = ch->in_room->first_content; obj; obj = obj->next_content )
            if ( ( obj->item_type == ITEM_FOUNTAIN ) || ( obj->item_type == ITEM_BLOOD ) )
                break;

        if ( !obj ) {
            send_to_char( "¿Beber qué?\r\n", ch );
            return;
        }
    }
    else {
        if ( ( obj = get_obj_here( ch, arg ) ) == NULL ) {
            send_to_char( "No puedes encontrarlo.\r\n", ch );
            return;
        }
    }

    if ( xIS_SET( ch->act, PLR_COMMUNICATION ) )
        send_to_char( "!!SOUND(sound/drink1.wav)\r\n", ch );

    if ( obj->count > 1 && obj->item_type != ITEM_FOUNTAIN )
        separate_obj( obj );

    if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_DRUNK] > 40 ) {
        send_to_char( "No puedes llegar a tu voca.  *Hic*\r\n", ch );
        return;
    }

    switch ( obj->item_type ) {
        default:
            if ( obj->carried_by == ch ) {
                act( AT_ACTION, "$n lleva $p a su boca e intenta beber...", ch, obj,
                     NULL, TO_ROOM );
                act( AT_ACTION, "Acercas $p a tu boca e intentas beber...", ch,
                     obj, NULL, TO_CHAR );
            }
            else {
                act( AT_ACTION, "$n se agacha e intenta beber de  $p...",
                     ch, obj, NULL, TO_ROOM );
                act( AT_ACTION, "Te agachas en el suelo e intentas beber de $p...", ch, obj,
                     NULL, TO_CHAR );
            }
            break;
        case ITEM_BLOOD:
            if ( IS_BLOODCLASS( ch ) && !IS_NPC( ch ) ) {
                if ( obj->timer > 0                    /* if timer, must be spilled blood 
                                                        */
                     && ch->level > 5 && ch->blood > ( 5 + ch->level / 10 ) ) {
                    if ( IS_VAMPIRE( ch ) ) {
                        send_to_char
                            ( "No puedes beber sangre del suelo!\r\n",
                              ch );
                        send_to_char
                            ( "Salvo en casos de necesidad, sólo prefieres beber sangre del cuello de tus víctimas !\r\n",
                              ch );
                    }
                    else
                        send_to_char( "Sólo beberás sangre derramada cuando lo necesites.\r\n",
                                      ch );
                    return;
                }
                if ( ch->blood < ch->max_blood ) {
                    if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                        act( AT_BLOOD, "$n bebe de la sangre derramada.", ch, NULL, NULL,
                             TO_ROOM );
                        set_char_color( AT_BLOOD, ch );
                        send_to_char( "Saboreas el líquido vital...\r\n",
                                      ch );
                        if ( obj->value[1] <= 1 ) {
                            set_char_color( AT_BLOOD, ch );
                            send_to_char( "Bebes las últimas gotas de sangre.\r\n",
                                          ch );
                            act( AT_BLOOD, "$n bebe las últimas gotas de sangre.", ch,
                                 NULL, NULL, TO_ROOM );
                        }
                    }

                    ch->blood = ch->blood + 1;
                    if ( ch->pcdata->condition[COND_FULL] <= STATED
                         || ch->pcdata->condition[COND_THIRST] <= STATED ) {
                        gain_condition( ch, COND_FULL, 1 );
                        gain_condition( ch, COND_THIRST, 1 );
                    }
                    if ( --obj->value[1] <= 0 ) {
                        if ( obj->serial == cur_obj )
                            global_objcode = rOBJ_DRUNK;
                        extract_obj( obj );
                        make_bloodstain( ch );
                    }
                }
                else
                    send_to_char( "No puedes consumir más sangre.\r\n", ch );
            }
            else
                send_to_char( "No está en tu naturaleza hacer eso.\r\n", ch );
            break;

        case ITEM_FOUNTAIN:
            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n bebe de la fuente.", ch, NULL, NULL, TO_ROOM );
                send_to_char( "Bebes un largo trago. Ya no tienes sed\r\n", ch );
            }

            if ( !IS_NPC( ch ) )
                gain_condition( ch, COND_THIRST, 10 );

            break;

        case ITEM_DRINK_CON:
            if ( obj->value[1] <= 0 ) {
                send_to_char( "Ya está vacío.\r\n", ch );
                return;
            }

            if ( ( liquid = obj->value[2] ) >= LIQ_MAX ) {
                bug( "Do_drink: bad liquid number %d.", liquid );
                liquid = obj->value[2] = 0;
            }

            /*
             * drinking blood 
             */
            if ( obj->value[2] == 13 ) {
                if ( IS_BLOODCLASS( ch ) && !IS_NPC( ch ) ) {
                    if ( ch->blood < ch->max_blood ) {
                        if ( ch->pcdata->condition[COND_FULL] >= STATED
                             || ch->pcdata->condition[COND_THIRST] >= STATED ) {
                            send_to_char( "Estás demasiado lleno para beber más sangre.\r\n", ch );
                            return;
                        }
                        if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                            act( AT_ACTION, "$n bebe $T de $p.", ch, obj,
                                 liq_table[liquid].liq_name, TO_ROOM );
                            act( AT_ACTION, "Bebes $T de $p.", ch, obj,
                                 liq_table[liquid].liq_name, TO_CHAR );

                            set_char_color( AT_BLOOD, ch );
                            send_to_char
                                ( "Saboreas el líquido vital...\r\n",
                                  ch );
                            if ( obj->value[1] <= 1 ) {
                                set_char_color( AT_BLOOD, ch );
                                send_to_char( "Bebes las últimas gotas de sangre.\r\n", ch );
                                act( AT_ACTION, "$n bebe $T de $p.", ch, obj,
                                     liq_table[liquid].liq_name, TO_ROOM );
                                act( AT_ACTION, "You bebes $T de $p.", ch, obj,
                                     liq_table[liquid].liq_name, TO_CHAR );

                            }
                        }

                        ch->blood = ch->blood + 1;
                        gain_condition( ch, COND_FULL, 1 );
                        gain_condition( ch, COND_THIRST, 1 );
                    }
                    else
                        send_to_char( "No puedes beber más sangre.\r\n", ch );
                }
                else
                    send_to_char( "No está en las características de tu raza hacer eso.\r\n", ch );
                break;
            }

            if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
                act( AT_ACTION, "$n bebe $T de $p.", ch, obj, liq_table[liquid].liq_name,
                     TO_ROOM );
                act( AT_ACTION, "Bebes $T de $p.", ch, obj, liq_table[liquid].liq_name,
                     TO_CHAR );
            }

            amount = 1;                                /* UMIN(amount, obj->value[1]); */
            /*
             * what was this? concentrated drinks?  concentrated water
             * too I suppose... sheesh! 
             */

            if ( obj->value[6] == 1 ) {
                act( AT_ACTION, "MmMmh, nada satisface más que una bebida cominada casera.", ch, NULL,
                     NULL, TO_CHAR );
                act( AT_MAGIC, "\r\nTesientes bastante lleno.", ch, NULL, NULL, TO_CHAR );
                af.type = gsn_crafted_drink;
                af.duration = ( obj->level / 2 ) + 100;
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = meb( AFF_CRAFTED_DRINK1 );
                af.level = obj->level;
                affect_join( ch, &af );
            }

            if ( obj->value[6] == 2 ) {
                act( AT_ACTION, "MmMmh, nada satisface más uqe una bebida combinada casera.", ch, NULL,
                     NULL, TO_CHAR );
                act( AT_MAGIC, "\r\nTe sientes bastante lleno.", ch, NULL, NULL, TO_CHAR );
                af.type = gsn_crafted_drink;
                af.duration = ( obj->level / 2 ) + 125;
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = meb( AFF_CRAFTED_DRINK2 );
                af.level = obj->level;
                affect_join( ch, &af );
            }
            if ( obj->value[6] == 3 ) {
                act( AT_ACTION, "MmMmh, nada satisface más que una bebida combinada casera.", ch, NULL,
                     NULL, TO_CHAR );
                act( AT_MAGIC, "\r\nTesientes bastante lleno.", ch, NULL, NULL, TO_CHAR );
                af.type = gsn_crafted_drink;
                af.duration = ( obj->level / 2 ) + 250;
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = meb( AFF_CRAFTED_DRINK3 );
                af.level = obj->level;
                affect_join( ch, &af );
            }
            if ( ch->race != RACE_DWARF ) {
                gain_condition( ch, COND_DRUNK, amount * liq_table[liquid].liq_affect[COND_DRUNK] );
            }
            gain_condition( ch, COND_FULL, amount * liq_table[liquid].liq_affect[COND_FULL] );
            gain_condition( ch, COND_THIRST, amount * liq_table[liquid].liq_affect[COND_THIRST] );

            /*
             * drinking holy water 
             */
            if ( obj->value[2] == 20 ) {
                if ( IS_GOOD( ch ) ) {
                    if ( ch->hit < ch->max_hit ) {
                        send_to_char( "&YTe sientes algo mejor.&D\r\n", ch );
                        ch->hit++;
                    }
                }
                else if ( IS_EVIL( ch ) ) {
                    if ( ch->hit > 1 ) {
                        send_to_char( "&YOuch!&D\r\n", ch );
                        ch->hit--;
                    }
                }
            }

            if ( !IS_NPC( ch ) ) {
                if ( ch->pcdata->condition[COND_DRUNK] > 24 )
                    send_to_char( "Estás borracho.\r\n", ch );
                else if ( ch->pcdata->condition[COND_DRUNK] > 18 )
                    send_to_char( "Estás muy bebido.\r\n", ch );
                else if ( ch->pcdata->condition[COND_DRUNK] > 12 )
                    send_to_char( "Estás bebido.\r\n", ch );
                else if ( ch->pcdata->condition[COND_DRUNK] > 8 )
                    send_to_char( "Estás algo bebido.\r\n", ch );
                else if ( ch->pcdata->condition[COND_DRUNK] > 5 )
                    send_to_char( "Te duele la cabeza.\r\n", ch );

                if ( ch->pcdata->condition[COND_FULL] >= STATED )
                    send_to_char( "Estás lleno.\r\n", ch );

                if ( ch->pcdata->condition[COND_THIRST] >= STATED )
                    send_to_char( "Estás inchado.\r\n", ch );
                else if ( ch->pcdata->condition[COND_THIRST] > 75 )
                    send_to_char( "Tu estómago está revuelto.\r\n", ch );
                else if ( ch->pcdata->condition[COND_THIRST] > 50 )
                    send_to_char( "No tienes sed.\r\n", ch );
            }
            if ( obj->value[4] )

                /*
                 * Volk put this in to allow spells in drink containers. Will
                 * eventually need for alchemy trade.
                 */

                obj_cast_spell( obj->value[4], obj->level, ch, ch, NULL );

            if ( obj->value[3] ) {
                /*
                 * The drink was poisoned! 
                 */

                act( AT_POISON, "$n se retuerce y vomita.", ch, NULL, NULL, TO_ROOM );
                act( AT_POISON, "Te retuerces y vomitas.", ch, NULL, NULL, TO_CHAR );
                ch->mental_state = URANGE( 20, ch->mental_state + 5, 100 );
                af.type = gsn_poison;
                af.duration = 3 * obj->value[3];
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = meb( AFF_POISON );
                af.level = obj->level;
                affect_join( ch, &af );
            }
            if ( obj->value[6] > 0 ) {
                if ( cur_obj == obj->serial )
                    global_objcode = rOBJ_DRUNK;
                extract_obj( obj );
                return;
            }
            obj->value[1] -= amount;
            if ( obj->value[1] <= 0 ) {
                act( AT_ACTION, "No queda nada en $p para beber.", ch, obj, NULL, TO_CHAR );
                return;
            }
            break;
    }


       if ( IS_NPC(ch) && xIS_SET( ch->act, ACT_PET ))
        {
            CHAR_DATA *owner = NULL;
       	    if ( VLD_STR(ch->master->name) && ch->master->Class == CLASS_BEAST )
       	    owner = ch->master; 

       	       	if ( owner )
       	       	{
                if ( owner->pcdata->petthirsty > 800 && owner->pcdata->petthirsty < 1000 )
                {
                act( AT_ACTION, "$n no tiene sed.", ch, NULL,
                 NULL, TO_ROOM );
                }
                else if ( owner->pcdata->petthirsty >= 1000 )
       	        {
                act( AT_ACTION, "$n está hinchado.", ch, NULL,
                 NULL, TO_ROOM );
       	       	 return;
               	}

                if ( owner->pcdata->petthirsty < 1000 )
                {
                  owner->pcdata->petthirsty += 100;
                 if ( owner->pcdata->petaffection < 49 )
                   {
                  owner->pcdata->petaffection += 1;
                   }
                 }
               }
            }
    if ( who_fighting( ch ) && IS_PKILL( ch ) )
        WAIT_STATE( ch, PULSE_PER_SECOND / 3 );
    else
        WAIT_STATE( ch, PULSE_PER_SECOND );
    return;
}

void do_eat( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    OBJ_DATA               *obj;
    ch_ret                  retcode;
    int                     foodcond;
    bool                    hgflag = TRUE;
    AFFECT_DATA             af;

    if ( argument[0] == '\0' ) {
        send_to_char( "¿comer qué??\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) )
        if ( ms_find_obj( ch ) )
            return;

    if ( ( obj = find_obj( ch, argument, TRUE ) ) == NULL )
        return;

    if ( !IS_IMMORTAL( ch ) ) {
        if ( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL
             && obj->item_type != ITEM_COOK ) {
            act( AT_ACTION, "$n empieza a morder $p... ($e debe tener mucha hambre)", ch, obj,
                 NULL, TO_ROOM );
            act( AT_ACTION, "Intentas morder $p...", ch, obj, NULL, TO_CHAR );
            return;
        }

        if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_FULL] >= STATED ) {
            send_to_char( "Estás demasiado lleno para comer más.\r\n", ch );
            return;
        }

        if ( !IS_NPC( ch ) && ch->race == RACE_DRAGON ) {
            separate_obj( obj );
            extract_obj( obj );

            if ( can_use_skill( ch, number_percent(  ), gsn_devour ) ) {
                act( AT_RED, "Comes $p.", ch, obj, NULL, TO_CHAR );   // Aurin
                act( AT_RED, "$n se come $p.", ch, obj, NULL, TO_ROOM );   // Aurin
                ch->pcdata->condition[COND_THIRST] += 2;
                ch->pcdata->condition[COND_FULL] += 2;
                learn_from_success( ch, gsn_devour );
            }
            else {
                send_to_char( "Intentas comer algo pero estás satisfecho.\r\n", ch );  // Aurin
                learn_from_failure( ch, gsn_devour );
            }
            return;
        }
    }

    if ( xIS_SET( ch->act, PLR_COMMUNICATION ) )
        send_to_char( "!!SOUND(sound/eat1.wav)\r\n", ch );

    if ( !IS_NPC( ch )
         && ( !IS_PKILL( ch )
              || ( IS_PKILL( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_HIGHGAG ) ) ) )
        hgflag = FALSE;

    /*
     * required due to object grouping 
     */
    separate_obj( obj );
    if ( obj->in_obj ) {
        if ( !hgflag )
            act( AT_PLAIN, "Coges $p de $P.", ch, obj, obj->in_obj, TO_CHAR );
        act( AT_PLAIN, "$n coge $p de $P.", ch, obj, obj->in_obj, TO_ROOM );
    }
    if ( ch->fighting && number_percent(  ) > ( get_curr_dex( ch ) * 2 + 47 ) ) {
        snprintf( buf, MSL, "%s",
                  ( ch->in_room->sector_type == SECT_UNDERWATER ||
                    ch->in_room->sector_type == SECT_WATER_SWIM ||
                    ch->in_room->sector_type ==
                    SECT_WATER_NOSWIM ) ? "dissolves in the water" : ( ch->in_room->sector_type ==
                                                                       SECT_AIR
                                                                       || IS_SET( ch->in_room->
                                                                                  room_flags,
                                                                                  ROOM_NOFLOOR ) ) ?
                  "falls far below" : "is trampled underfoot" );
        act( AT_MAGIC, "$n tira $p, y $T.", ch, obj, buf, TO_ROOM );
        if ( !hgflag )
            act( AT_MAGIC, "Oops, $p se resbala de tu mano $T!", ch, obj, buf, TO_CHAR );
    }
    else {
        if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
            if ( !obj->action_desc || obj->action_desc[0] == '\0' ) {
                act( AT_ACTION, "$n se come $p.", ch, obj, NULL, TO_ROOM );
                if ( !hgflag )
                    act( AT_ACTION, "Te comes $p.", ch, obj, NULL, TO_CHAR );
            }
            else
                actiondesc( ch, obj, NULL );
        }

        switch ( obj->item_type ) {
            case ITEM_COOK:
            case ITEM_FOOD:
                WAIT_STATE( ch, PULSE_PER_SECOND / 3 );
                if ( obj->timer > 0 && obj->value[1] > 0 )
                    foodcond = ( obj->timer * 10 ) / obj->value[1];
                else
                    foodcond = 10;

                if ( !IS_NPC( ch ) ) {
                    int                     condition;

                    if ( obj->value[6] == 1 ) {
                        act( AT_ACTION, "MmMmh, nada satisface más que una comida casera.", ch, NULL,
                             NULL, TO_CHAR );
                        act( AT_MAGIC, "\r\nEstás lleno .", ch, NULL, NULL,
                             TO_CHAR );
                        af.type = gsn_crafted_food;
                        af.duration = ( obj->level / 2 ) + 100;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = meb( AFF_CRAFTED_FOOD1 );
                        af.level = obj->level;
                        affect_join( ch, &af );
                    }
                    if ( obj->value[6] == 2 ) {
                        act( AT_ACTION, "MmMmh, nada satisface más que una comida casera.", ch, NULL,
                             NULL, TO_CHAR );
                        act( AT_MAGIC, "\r\nEstás lleno.", ch, NULL, NULL,
                             TO_CHAR );
                        af.type = gsn_crafted_food;
                        af.duration = ( obj->level / 2 ) + 125;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = meb( AFF_CRAFTED_FOOD2 );
                        af.level = obj->level;
                        affect_join( ch, &af );
                    }
                    if ( obj->value[6] == 3 ) {
                        act( AT_ACTION, "MmMmh, nada satisface más que una comida casera.", ch, NULL,
                             NULL, TO_CHAR );
                        act( AT_MAGIC, "\r\nEstás lleno.", ch, NULL, NULL,
                             TO_CHAR );
                        af.type = gsn_crafted_food;
                        af.duration = ( obj->level / 2 ) + 250;
                        af.location = APPLY_NONE;
                        af.modifier = 0;
                        af.bitvector = meb( AFF_CRAFTED_FOOD3 );
                        af.level = obj->level;
                        affect_join( ch, &af );
                    }

                    condition = ch->pcdata->condition[COND_FULL];
                    gain_condition( ch, COND_FULL, ( obj->value[0] * foodcond ) / 8 );

                    if ( ch->pcdata->condition[COND_FULL] >= 1
                         && ch->pcdata->condition[COND_FULL] < STATED )
                        send_to_char( "&cNo tienes hambre.\r\n", ch );
                    else if ( ch->pcdata->condition[COND_FULL] >= STATED )
                        send_to_char( "&cEstás lleno.\r\n", ch );
                }

                if ( ( obj->value[3] != 0
                       || ( foodcond < 4 && number_range( 0, foodcond + 1 ) == 0 )
                       || ( obj->item_type == ITEM_COOK && obj->value[2] == 0 ) )
                     && ch->Class != CLASS_HELLSPAWN && ch->race != RACE_DRAGON ) {
                    /*
                     * The food was poisoned! 
                     */

                    if ( obj->value[3] != 0 ) {
                        act( AT_POISON, "$n se ahoga y vomita.", ch, NULL, NULL, TO_ROOM );
                        act( AT_POISON, "Te ahogas y vomitas.", ch, NULL, NULL, TO_CHAR );
                        ch->mental_state = URANGE( 20, ch->mental_state + 5, 100 );
                    }
                    else {
                        act( AT_POISON, "$n vomita en $p.", ch, obj, NULL, TO_ROOM );
                        act( AT_POISON, "Vomitas en $p.", ch, obj, NULL, TO_CHAR );
                        ch->mental_state = URANGE( 15, ch->mental_state + 5, 100 );
                    }

                    af.type = gsn_poison;
                    af.duration = 2 * obj->value[0] * ( obj->value[3] > 0 ? obj->value[3] : 1 );
                    af.location = APPLY_NONE;
                    af.modifier = 0;
                    af.bitvector = meb( AFF_POISON );
                    af.level = obj->level;
                    affect_join( ch, &af );
                }
                break;

            case ITEM_PILL:
                sysdata.upill_val += obj->cost / 100;
                if ( who_fighting( ch ) && IS_PKILL( ch ) )
                    WAIT_STATE( ch, PULSE_PER_SECOND / 4 );
                else
                    WAIT_STATE( ch, PULSE_PER_SECOND / 3 );
                /*
                 * allow pills to fill you, if so desired 
                 */
                if ( !IS_NPC( ch ) && obj->value[4] ) {
                    int                     condition;

                    condition = ch->pcdata->condition[COND_FULL];
                    gain_condition( ch, COND_FULL, ( obj->value[4] * 2 ) );

                    if ( condition <= 1 && ch->pcdata->condition[COND_FULL] > 1 )
                        send_to_char( "No tienes hambre.\r\n", ch );
                    else if ( ch->pcdata->condition[COND_FULL] >= STATED )
                        send_to_char( "Estás lleno.\r\n", ch );
                }
                retcode = obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
                if ( retcode == rNONE )
                    retcode = obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
                if ( retcode == rNONE )
                    retcode = obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
                break;
        }

    }

       if (  IS_NPC(ch) && xIS_SET( ch->act, ACT_PET ))
        {
            CHAR_DATA *owner = NULL;

            if ( VLD_STR(ch->master->name) && ch->master->Class == CLASS_BEAST )
            owner = ch->master;
                
                if ( owner)
                {
                if ( owner->pcdata->pethungry > 800 && owner->pcdata->pethungry < 1000 )
                {
                act( AT_ACTION, "$n ya no tiene hambre.", ch, NULL,
                 NULL, TO_ROOM );
                }
                else if ( owner->pcdata->pethungry >= 1000 )
       	        {
                act( AT_ACTION, "$n está lleno.", ch, NULL,
                 NULL, TO_ROOM );
                 return;
               	}

        	if ( owner->pcdata->pethungry < 1000 )
                {
                  owner->pcdata->pethungry += 100;
                 if ( owner->pcdata->petaffection < 49 )
       	       	   {
                  owner->pcdata->petaffection += 1;
                   }
                 }
                }
       	}


    if ( obj->serial == cur_obj )
        global_objcode = rOBJ_EATEN;
    extract_obj( obj );
    return;
}

void do_quaff( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    OBJ_DATA               *obj;
    ch_ret                  retcode;
    bool                    hgflag = TRUE;

    if ( argument[0] == '\0' || !str_cmp( argument, "" ) || !argument ) {
        send_to_char( "¿Qué quieres tragar??\r\n", ch );
        return;
    }
    if ( ( obj = find_obj( ch, argument, TRUE ) ) == NULL )
        return;

    if ( ch->fighting )
        victim = who_fighting( ch );
    else
        victim = NULL;

    if ( !IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
        return;

    if ( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) ) {
        set_char_color( AT_RED, ch );
        send_to_char( "Tu adrenalina está demasiado alta!\r\n", ch );
        return;
    }

    if ( obj->item_type != ITEM_POTION ) {
        if ( obj->item_type == ITEM_DRINK_CON )
            do_drink( ch, obj->name );
        else {
            act( AT_ACTION, "$n levanta $p a su boca e intenta beber...", ch, obj,
                 NULL, TO_ROOM );
            act( AT_ACTION, "Traes $p hasta tu boca e intentas beber...", ch, obj,
                 NULL, TO_CHAR );
        }
        return;
    }

    /*
     * Empty container check      -Shaddai
     */
    if ( obj->value[1] == -1 && obj->value[2] == -1 && obj->value[3] == -1 ) {
        send_to_char( "Sólo tragas aire.\r\n", ch );
        return;
    }
    /*
     * Fullness checking     -Thoric
     */
    if ( !IS_NPC( ch )
         && ( ch->pcdata->condition[COND_FULL] >= STATED
              || ch->pcdata->condition[COND_THIRST] >= STATED ) ) {
        send_to_char( "Tu estómago no puede con más.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch ) && ch->pcdata->nuisance &&
         ch->pcdata->nuisance->flags > 3
         && ( ch->pcdata->condition[COND_FULL] >=
              ( 100 - ( 3 * ch->pcdata->nuisance->flags ) + ch->pcdata->nuisance->power )
              || ch->pcdata->condition[COND_THIRST] >=
              ( 100 - ( ch->pcdata->nuisance->flags ) + ch->pcdata->nuisance->power ) ) ) {
        send_to_char( "Tu estómago no puede con más.\r\n", ch );
        return;
    }

    if ( !IS_NPC( ch )
         && ( !IS_PKILL( ch )
              || ( IS_PKILL( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_HIGHGAG ) ) ) )
        hgflag = FALSE;

    separate_obj( obj );
    if ( obj->in_obj ) {
        if ( !CAN_PKILL( ch ) ) {
            act( AT_PLAIN, "Coges $p de $P.", ch, obj, obj->in_obj, TO_CHAR );
            act( AT_PLAIN, "$n coges $p de $P.", ch, obj, obj->in_obj, TO_ROOM );
        }
    }

// Volk: If we've reached this stage, check their potion timer first!!
    if ( ch->pcdata ) {
        int                     timer = current_time;

        if ( ch->fighting ) {
            if ( IS_NPC( who_fighting( ch ) ) )        // PvE
            {
                timer -= ch->pcdata->potionspve;
                if ( timer < sysdata.potionspve ) {
                    ch_printf( ch, "Debe esperar %d segundos antes de beber otra poción.\r\n",
                               sysdata.potionspve - timer );
                    return;
                }
            }
            else                                       // PvP
            {
                timer -= ch->pcdata->potionspvp;
                if ( timer < sysdata.potionspvp ) {
                    ch_printf( ch, "Debes esperar  %d segundos antes de beber otra poción.\r\n",
                               sysdata.potionspvp - timer );
                    return;
                }
            }
        }
        else                                           // Out of battle
        {
            timer -= ch->pcdata->potionsoob;
            if ( timer < sysdata.potionsoob ) {
                ch_printf( ch, "Desbes esperar %d segundos antes de beber otra poción.\r\n",
                           sysdata.potionsoob - timer );
                return;
            }
        }
    }

    /*
     * If fighting, chance of dropping potion   -Thoric
     */
    if ( ch->fighting && number_percent(  ) > ( get_curr_dex( ch ) * 2 + 40 ) ) {
        act( AT_MAGIC, "c$n deja caer $p y lo rompe en pedazos.", ch, obj, NULL, TO_ROOM );
        if ( !hgflag )
            act( AT_MAGIC, "¡Oops... $p ha sido golpeado por tu mano y se ha roto!", ch, obj, NULL,
                 TO_CHAR );
    }
    else {
        if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
            if ( !CAN_PKILL( ch ) || !obj->in_obj ) {
                act( AT_ACTION, "$n traga $p.", ch, obj, NULL, TO_ROOM );
                if ( !hgflag )
                    act( AT_ACTION, "Tragas $p.", ch, obj, NULL, TO_CHAR );
            }
            else if ( obj->in_obj ) {
                act( AT_ACTION, "$n traga $p de $P.", ch, obj, obj->in_obj, TO_ROOM );
                if ( !hgflag )
                    act( AT_ACTION, "Tragas $p de $P.", ch, obj, obj->in_obj, TO_CHAR );
            }
        }

        if ( who_fighting( ch ) && IS_PKILL( ch ) )
            WAIT_STATE( ch, PULSE_PER_SECOND / 5 );
        else
            WAIT_STATE( ch, PULSE_PER_SECOND / 3 );

        gain_condition( ch, COND_THIRST, 3 );

        if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] > 36
             && ch->pcdata->condition[COND_THIRST] < 42 )
            act( AT_ACTION, "Ya no tienes sed.", ch, NULL, NULL, TO_CHAR );

        if ( !IS_NPC( ch ) && ch->pcdata->condition[COND_THIRST] > 43 )
            act( AT_ACTION, "Tu estómago está a punto de llenarse.", ch, NULL, NULL, TO_CHAR );
        retcode = obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
        if ( retcode == rNONE )
            retcode = obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
        if ( retcode == rNONE )
            retcode = obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );

// Volk: Put potion timers in here. 
        if ( ch->pcdata ) {
            if ( ch->fighting ) {
                if ( IS_NPC( who_fighting( ch ) ) )    // PvE
                    ch->pcdata->potionspve = current_time;
                else                                   // PvP
                    ch->pcdata->potionspvp = current_time;
            }
            else                                       // Out of battle
                ch->pcdata->potionsoob = current_time;
        }
    }
    if ( obj->pIndexData->vnum == OBJ_VNUM_FLASK_BREWING )
        sysdata.brewed_used++;
    else
        sysdata.upotion_val += obj->cost / 100;
    if ( cur_obj == obj->serial )
        global_objcode = rOBJ_QUAFFED;
    extract_obj( obj );
    return;
}

void do_recite( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *scroll;
    OBJ_DATA               *obj;
    ch_ret                  retcode;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "¿Recitar qué??\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( scroll = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "No tienes que desplazarte.\r\n", ch );
        return;
    }

    if ( scroll->item_type != ITEM_SCROLL ) {
        act( AT_ACTION, "$n sostiene $p como para recitar algo...", ch, scroll, NULL,
             TO_ROOM );
        act( AT_ACTION, "Sostienes  $p y te quedas con la boca abierta.  (¿Ahora qué?)", ch,
             scroll, NULL, TO_CHAR );
        return;
    }

    if ( IS_NPC( ch ) && ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) ) {
        send_to_char( "no conoces ese idioma.\r\n", ch );
        return;
    }

    if ( ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING )
         && ( ch->level + 10 < scroll->value[0] ) ) {
        send_to_char( "Este desplazamiento es demasiado complicado para ti.\r\n", ch );
        return;
    }

    obj = NULL;
    if ( arg2[0] == '\0' )
        victim = ch;
    else {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL
             && ( obj = get_obj_here( ch, arg2 ) ) == NULL ) {
            send_to_char( "No puedes encontrarlo.\r\n", ch );
            return;
        }
    }

    if ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING )
        sysdata.scribed_used++;
    separate_obj( scroll );
    act( AT_MAGIC, "$n recita $p.", ch, scroll, NULL, TO_ROOM );
    act( AT_MAGIC, "Recitas $p.", ch, scroll, NULL, TO_CHAR );

    if ( victim != ch )
        WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
    else
        WAIT_STATE( ch, PULSE_PER_SECOND / 2 );

    retcode = obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj );
    if ( retcode == rNONE )
        retcode = obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj );
    if ( retcode == rNONE )
        retcode = obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );

    if ( scroll->serial == cur_obj )
        global_objcode = rOBJ_USED;
    extract_obj( scroll );
    return;
}

/*
 * Function to handle the state changing of a triggerobject (lever)  -Thoric
 */
void pullorpush( CHAR_DATA *ch, OBJ_DATA *obj, bool pull )
{
    CHAR_DATA              *rch;
    bool                    isup;
    ROOM_INDEX_DATA        *room,
                           *to_room;
    EXIT_DATA              *pexit,
                           *pexit_rev;
    int                     edir;
    const char             *txt;

    if ( IS_SET( obj->value[0], TRIG_UP ) )
        isup = TRUE;
    else
        isup = FALSE;
    switch ( obj->item_type ) {
        default:
            ch_printf( ch, "No puedes  %s éso!\r\n", pull ? "empujar" : "push" );
            return;
            break;
        case ITEM_SWITCH:
        case ITEM_LEVER:
        case ITEM_PULLCHAIN:
            if ( ( !pull && isup ) || ( pull && !isup ) ) {
                ch_printf( ch, "Ya está %s.\r\n", isup ? "arriba" : "down" );
                return;
            }
        case ITEM_BUTTON:
            if ( ( !pull && isup ) || ( pull && !isup ) ) {
                ch_printf( ch, "Ya está %s.\r\n", isup ? "en" : "fuera" );
                return;
            }
            break;
    }
    if ( ( pull ) && HAS_PROG( obj->pIndexData, PULL_PROG ) ) {
        if ( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
            REMOVE_BIT( obj->value[0], TRIG_UP );
        oprog_pull_trigger( ch, obj );
        return;
    }
    if ( ( !pull ) && HAS_PROG( obj->pIndexData, PUSH_PROG ) ) {
        if ( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
            SET_BIT( obj->value[0], TRIG_UP );
        oprog_push_trigger( ch, obj );
        return;
    }

    if ( !oprog_use_trigger( ch, obj, NULL, NULL, NULL ) ) {
        bool                    pSlots = FALSE;

        if ( pSlots ) {
            act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "Tú %s $p palanca.",
                        pull ? "tiras" : "empujas" );
            act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n %s $p palanca.",
                        pull ? "tira" : "empuja" );
        }
        else {
            act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "Tú %s $p.", pull ? "tiras" : "empujas" );
            act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n %s $p.", pull ? "tira" : "empuja" );
        }
    }

    if ( !IS_SET( obj->value[0], TRIG_AUTORETURN ) ) {
        if ( pull )
            REMOVE_BIT( obj->value[0], TRIG_UP );
        else
            SET_BIT( obj->value[0], TRIG_UP );
    }
    if ( IS_SET( obj->value[0], TRIG_TELEPORT ) || IS_SET( obj->value[0], TRIG_TELEPORTALL )
         || IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) ) {
        EXT_BV                  flags;

        if ( ( room = get_room_index( obj->value[1] ) ) == NULL ) {
            bug( "PullOrPush: obj points to invalid room %d", obj->value[1] );
            return;
        }
        xCLEAR_BITS( flags );
        if ( IS_SET( obj->value[0], TRIG_SHOWROOMDESC ) )
            xSET_BIT( flags, TELE_SHOWDESC );
        if ( IS_SET( obj->value[0], TRIG_TELEPORTALL ) )
            xSET_BIT( flags, TELE_TRANSALL );
        if ( IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
            xSET_BIT( flags, TELE_TRANSALLPLUS );

        teleport( ch, obj->value[1], &flags );
        return;
    }

    if ( IS_SET( obj->value[0], TRIG_RAND4 ) || IS_SET( obj->value[0], TRIG_RAND6 ) ) {
        int                     maxd;

        if ( ( room = get_room_index( obj->value[1] ) ) == NULL ) {
            bug( "PullOrPush: obj points to invalid room %d", obj->value[1] );
            return;
        }

        if ( IS_SET( obj->value[0], TRIG_RAND4 ) )
            maxd = 3;
        else
            maxd = 5;

        randomize_exits( room, maxd );
        for ( rch = room->first_person; rch; rch = rch->next_in_room ) {
            send_to_char( "Oyes un sonido retumbante.\r\n", rch );
            send_to_char( "Algo parece diferente...\r\n", rch );
        }
    }
    if ( IS_SET( obj->value[0], TRIG_DOOR ) ) {
        room = get_room_index( obj->value[1] );
        if ( !room )
            room = obj->in_room;
        if ( !room ) {
            bug( "PullOrPush: obj points to invalid room %d", obj->value[1] );
            return;
        }
        if ( IS_SET( obj->value[0], TRIG_D_NORTH ) ) {
            edir = DIR_NORTH;
            txt = "hacia el norte";
        }
        else if ( IS_SET( obj->value[0], TRIG_D_SOUTH ) ) {
            edir = DIR_SOUTH;
            txt = "hacia el sur";
        }
        else if ( IS_SET( obj->value[0], TRIG_D_EAST ) ) {
            edir = DIR_EAST;
            txt = "hacia el este";
        }
        else if ( IS_SET( obj->value[0], TRIG_D_WEST ) ) {
            edir = DIR_WEST;
            txt = "hacia el oeste";
        }
        else if ( IS_SET( obj->value[0], TRIG_D_UP ) ) {
            edir = DIR_UP;
            txt = "desde arriba";
        }
        else if ( IS_SET( obj->value[0], TRIG_D_DOWN ) ) {
            edir = DIR_DOWN;
            txt = "desde abajo";
        }
        else {
            bug( "%s", "PullOrPush: door: no direction flag set." );
            return;
        }
        pexit = get_exit( room, edir );
        if ( !pexit ) {
            if ( !IS_SET( obj->value[0], TRIG_PASSAGE ) ) {
                bug( "PullOrPush: obj points to non-exit %d", obj->value[1] );
                return;
            }
            to_room = get_room_index( obj->value[2] );
            if ( !to_room ) {
                bug( "PullOrPush: dest points to invalid room %d", obj->value[2] );
                return;
            }
            pexit = make_exit( room, to_room, edir );
            pexit->key = -1;
            pexit->exit_info = 0;
            top_exit++;
            act( AT_PLAIN, "¡Un camino se abre!", ch, NULL, NULL, TO_CHAR );
            act( AT_PLAIN, "¡Un camino se abre!", ch, NULL, NULL, TO_ROOM );
            return;
        }
        if ( IS_SET( obj->value[0], TRIG_UNLOCK ) && IS_SET( pexit->exit_info, EX_LOCKED ) ) {
            REMOVE_BIT( pexit->exit_info, EX_LOCKED );
            act( AT_PLAIN, "Escuchas un leve chasquido $T.", ch, NULL, txt, TO_CHAR );
            act( AT_PLAIN, "Escuchas un leve chasquido $T.", ch, NULL, txt, TO_ROOM );
            if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
                REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
            return;
        }
        if ( IS_SET( obj->value[0], TRIG_LOCK ) && !IS_SET( pexit->exit_info, EX_LOCKED ) ) {
            SET_BIT( pexit->exit_info, EX_LOCKED );
            act( AT_PLAIN, "Escuchas un leve chasquido $T.", ch, NULL, txt, TO_CHAR );
            act( AT_PLAIN, "Escuchas un leve chasquido $T.", ch, NULL, txt, TO_ROOM );
            if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
                SET_BIT( pexit_rev->exit_info, EX_LOCKED );
            return;
        }
        if ( IS_SET( obj->value[0], TRIG_OPEN ) && IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            REMOVE_BIT( pexit->exit_info, EX_CLOSED );
            for ( rch = room->first_person; rch; rch = rch->next_in_room )
                act( AT_ACTION, "El $d se abre.", rch, NULL, pexit->keyword, TO_CHAR );
            if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room ) {
                REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
                /*
                 * bug here pointed out by Nick Gammon 
                 */
                for ( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
                    act( AT_ACTION, "El $d se abre.", rch, NULL, pexit_rev->keyword, TO_CHAR );
            }
            check_room_for_traps( ch, trap_door[edir] );
            return;
        }
        if ( IS_SET( obj->value[0], TRIG_CLOSE ) && !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            SET_BIT( pexit->exit_info, EX_CLOSED );
            for ( rch = room->first_person; rch; rch = rch->next_in_room )
                act( AT_ACTION, "El $d se cierra.", rch, NULL, pexit->keyword, TO_CHAR );
            if ( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room ) {
                SET_BIT( pexit_rev->exit_info, EX_CLOSED );
                /*
                 * bug here pointed out by Nick Gammon 
                 */
                for ( rch = pexit->to_room->first_person; rch; rch = rch->next_in_room )
                    act( AT_ACTION, "El $d se cierra.", rch, NULL, pexit_rev->keyword, TO_CHAR );
            }
            check_room_for_traps( ch, trap_door[edir] );
            return;
        }
    }
}

void do_pull( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "¿Tirar de qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( obj = get_obj_here( ch, arg ) ) == NULL ) {
        act( AT_PLAIN, "No veo  $T aquí.", ch, NULL, arg, TO_CHAR );
        return;
    }

    pullorpush( ch, obj, TRUE );
}

void do_push( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    OBJ_DATA               *obj;

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "¿Empujar qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( obj = get_obj_here( ch, arg ) ) == NULL ) {
        act( AT_PLAIN, "No veo  $T aquí.", ch, NULL, arg, TO_CHAR );
        return;
    }

    pullorpush( ch, obj, FALSE );
}

void do_rap( CHAR_DATA *ch, char *argument )
{
    EXIT_DATA              *pexit;
    char                    arg[MIL];

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Golpear en qué?\r\n", ch );
        return;
    }
    if ( ch->fighting ) {
        send_to_char( "Tienes cosas mejores que hacer con tus manos ahora mismo.\r\n", ch );
        return;
    }
    if ( ( pexit = find_door( ch, arg, FALSE ) ) != NULL ) {
        ROOM_INDEX_DATA        *to_room;
        EXIT_DATA              *pexit_rev;
        const char             *keyword;

        if ( !IS_SET( pexit->exit_info, EX_CLOSED ) ) {
            send_to_char( "¡Por qué llamas? ¡Está abierto!\r\n", ch );
            return;
        }
        if ( IS_SET( pexit->exit_info, EX_SECRET ) )
            keyword = "wall";
        else
            keyword = pexit->keyword;
        act( AT_ACTION, "Golpeas fuertemente $d.", ch, NULL, keyword, TO_CHAR );
        act( AT_ACTION, "$n golpea en the $d.", ch, NULL, keyword, TO_ROOM );
        if ( ( to_room = pexit->to_room ) != NULL && ( pexit_rev = pexit->rexit ) != NULL
             && pexit_rev->to_room == ch->in_room ) {
            CHAR_DATA              *rch;

            for ( rch = to_room->first_person; rch; rch = rch->next_in_room ) {
                act( AT_ACTION, "Alguien golpea fuertemente desde el otro lado del $d.", rch, NULL,
                     pexit_rev->keyword, TO_CHAR );
            }
        }
    }
    else {
        act( AT_ACTION, "Haces gestos de llamar en el aire.", ch, NULL, NULL, TO_CHAR );
        act( AT_ACTION, "$n hace gestos de llamar en el aire.", ch, NULL, NULL, TO_ROOM );
    }
    return;
}

/* pipe commands (light, tamp, smoke) by Thoric */
void do_tamp( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *pipe;
    char                    arg[MIL];

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "¿Limpiar qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( pipe = get_obj_carry( ch, arg ) ) == NULL ) {
        send_to_char( "No llevas éso.\r\n", ch );
        return;
    }
    if ( pipe->item_type != ITEM_PIPE ) {
        send_to_char( "No puedes limpiar éso.\r\n", ch );
        return;
    }
    if ( !IS_SET( pipe->value[3], PIPE_TAMPED ) ) {
        act( AT_ACTION, "Gentilmente limpias $p.", ch, pipe, NULL, TO_CHAR );
        act( AT_ACTION, "$n gentilmente limpia $p.", ch, pipe, NULL, TO_ROOM );
        SET_BIT( pipe->value[3], PIPE_TAMPED );
        return;
    }
    send_to_char( "No se necesita limpiar.\r\n", ch );
}

void do_smoke( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *pipe;
    char                    arg[MIL];
    ch_ret                  retcode;

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "¿Fumar qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( pipe = get_obj_carry( ch, arg ) ) == NULL ) {
        send_to_char( "No llevas eso.\r\n", ch );
        return;
    }
    if ( pipe->item_type != ITEM_PIPE ) {
        act( AT_ACTION, "Intentas fumar $p... pero no parece funcionar.", ch, pipe, NULL,
             TO_CHAR );
        act( AT_ACTION, "$n intenta fumar $p... (me pregunto: ¿qué habrá puesto en su pipa?)", ch,
             pipe, NULL, TO_ROOM );
        return;
    }
    if ( !IS_SET( pipe->value[3], PIPE_LIT ) ) {
        act( AT_ACTION, "Intentas fumar  $p, pero no se encendió.", ch, pipe, NULL, TO_CHAR );
        act( AT_ACTION, "$n intenta fumar  $p, pero no se encendió.", ch, pipe, NULL, TO_ROOM );
        return;
    }
    if ( pipe->value[1] > 0 ) {
        if ( !oprog_use_trigger( ch, pipe, NULL, NULL, NULL ) ) {
            act( AT_ACTION, "Dibujas cuidadosamente desde  $p.", ch, pipe, NULL, TO_CHAR );
            act( AT_ACTION, "$n dibuja cuidadosamente desde $p.", ch, pipe, NULL, TO_ROOM );
        }

        if ( IS_VALID_HERB( pipe->value[2] ) && pipe->value[2] < top_herb ) {
            int                     sn = pipe->value[2];
            SKILLTYPE              *skill = get_skilltype( sn );

            WAIT_STATE( ch, 14 );
            if ( sn == 5 ) {
                retcode = spell_refresh( skill_lookup( "refresh" ), ch->level, ch, ch );
            }
            else if ( sn == 4 ) {
                retcode = spell_cure_light( skill_lookup( "cure light" ), ch->level, ch, ch );
            }
            else if ( sn == 3 ) {
                retcode = spell_smaug( skill_lookup( "restore mana" ), 5, ch, ch );
            }
            else if ( sn == 2 ) {
                retcode = spell_smaug( skill_lookup( "detect invis" ), 2, ch, ch );
            }
            else if ( sn == 1 ) {
                retcode = spell_poison( skill_lookup( "poison" ), ch->level, ch, ch );
            }
            if ( obj_extracted( pipe ) )
                return;
        }
        else
            bug( "do_smoke: bad herb sn number %d", pipe->value[2] );

        SET_BIT( pipe->value[3], PIPE_HOT );
        if ( --pipe->value[1] < 1 ) {
            REMOVE_BIT( pipe->value[3], PIPE_LIT );
            SET_BIT( pipe->value[3], PIPE_DIRTY );
            SET_BIT( pipe->value[3], PIPE_FULLOFASH );
        }
    }
}

void do_light( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *pipe;
    char                    arg[MIL];

    one_argument( argument, arg );
    if ( arg[0] == '\0' ) {
        send_to_char( "¿Encender qué?\r\n", ch );
        return;
    }

    if ( ms_find_obj( ch ) )
        return;

    if ( ( pipe = get_obj_carry( ch, arg ) ) == NULL ) {
        send_to_char( "No llevas éso.\r\n", ch );
        return;
    }
    if ( pipe->item_type != ITEM_PIPE ) {
        send_to_char( "No puedes encender éso.\r\n", ch );
        return;
    }
    if ( !IS_SET( pipe->value[3], PIPE_LIT ) ) {
        if ( pipe->value[1] < 1 ) {
            act( AT_ACTION, "Intentas encender $p, but it's empty.", ch, pipe, NULL, TO_CHAR );
            act( AT_ACTION, "$n intenta encender $p, but it's empty.", ch, pipe, NULL, TO_ROOM );
            return;
        }
        act( AT_ACTION, "Enciendes cuidadosamente $p.", ch, pipe, NULL, TO_CHAR );
        act( AT_ACTION, "$n enciende cuidadosamente $p.", ch, pipe, NULL, TO_ROOM );
        SET_BIT( pipe->value[3], PIPE_LIT );
        return;
    }
    send_to_char( "Ya está encendido.\r\n", ch );
}

void do_empty( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA               *obj;
    char                    arg1[MIL];
    char                    arg2[MIL];

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( !str_cmp( arg2, "into" ) && argument[0] != '\0' )
        argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "¿Vaciar qué?\r\n", ch );
        return;
    }
    if ( ms_find_obj( ch ) )
        return;

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "No llevas eso.\r\n", ch );
        return;
    }
    if ( obj->count > 1 )
        separate_obj( obj );

    switch ( obj->item_type ) {
        default:
            act( AT_ACTION, "Sacudes $p en un intento de vaciarlo...", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n empieza a sacudir $p en un intento de vaciarlo...", ch, obj, NULL,
                 TO_ROOM );
            return;
        case ITEM_PIPE:
            act( AT_ACTION, "Tocas suavemente $p y lo vacías.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n gently taps $p and empties it out.", ch, obj, NULL, TO_ROOM );
            REMOVE_BIT( obj->value[3], PIPE_FULLOFASH );
            REMOVE_BIT( obj->value[3], PIPE_LIT );
            obj->value[1] = 0;
            return;
        case ITEM_DRINK_CON:
            if ( obj->value[1] < 1 ) {
                send_to_char( "Ya está vacío.\r\n", ch );
                return;
            }
            act( AT_ACTION, "Vacías $p.", ch, obj, NULL, TO_CHAR );
            act( AT_ACTION, "$n vacía $p.", ch, obj, NULL, TO_ROOM );
            obj->value[1] = 0;
            return;
        case ITEM_CONTAINER:
        case ITEM_QUIVER:
            if ( IS_SET( obj->value[1], CONT_CLOSED ) ) {
                act( AT_PLAIN, "El $p está cerrado.", ch, NULL, obj->name, TO_CHAR );
                return;
            }
        case ITEM_KEYRING:
            if ( !obj->first_content ) {
                send_to_char( "Ya está vacío.\r\n", ch );
                return;
            }
            if ( arg2[0] == '\0' ) {
                if ( IS_SET( ch->in_room->room_flags, ROOM_NODROP )
                     || ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_LITTERBUG ) ) ) {
                    set_char_color( AT_MAGIC, ch );
                    send_to_char( "¡Una fuerza mágica te fuerza a parar!\r\n", ch );
                    set_char_color( AT_TELL, ch );
                    send_to_char( "Alguien te cuenta, '¡No tires basura aquí!'\r\n", ch );
                    return;
                }
                if ( IS_SET( ch->in_room->room_flags, ROOM_NODROPALL )
                     || IS_SET( ch->in_room->room_flags, ROOM_CLANSTOREROOM ) ) {
                    send_to_char( "No parece que puedas hacer éso aquí...\r\n", ch );
                    return;
                }
                if ( empty_obj( obj, NULL, ch->in_room, FALSE ) ) {
                    act( AT_ACTION, "Vacías $p.", ch, obj, NULL, TO_CHAR );
                    act( AT_ACTION, "$n vacía $p.", ch, obj, NULL, TO_ROOM );
                    if ( xIS_SET( sysdata.save_flags, SV_EMPTY ) )
                        save_char_obj( ch );
                }
                else
                    send_to_char( "Hmmm... No funcionó.\r\n", ch );
            }
            else {
                OBJ_DATA               *dest = get_obj_here( ch, arg2 );

                if ( !dest ) {
                    send_to_char( "No puedes encontrarlo.\r\n", ch );
                    return;
                }
                if ( dest == obj ) {
                    send_to_char( "¡No se puede vaciar algo en sí mismo!\r\n", ch );
                    return;
                }
                if ( dest->item_type != ITEM_CONTAINER && dest->item_type != ITEM_KEYRING
                     && dest->item_type != ITEM_QUIVER ) {
                    send_to_char( "¡Éso no es un contenedor!\r\n", ch );
                    return;
                }
                if ( IS_SET( dest->value[1], CONT_CLOSED ) ) {
                    act( AT_PLAIN, "El $p está cerrado.", ch, NULL, dest->name, TO_CHAR );
                    return;
                }
                separate_obj( dest );
                if ( empty_obj( obj, dest, NULL, FALSE ) ) {
                    act( AT_ACTION, "Vacías $p en $P.", ch, obj, dest, TO_CHAR );
                    act( AT_ACTION, "$n vacía $p en $P.", ch, obj, dest, TO_ROOM );
                    if ( !dest->carried_by && xIS_SET( sysdata.save_flags, SV_EMPTY ) )
                        save_char_obj( ch );
                }
                else
                    act( AT_ACTION, "$P está demasiado lleno.", ch, obj, dest, TO_CHAR );
            }
            return;
    }
}

/*
 * Apply a salve/ointment     -Thoric
 * Support for applying to others.  Pkill concerns dealt with elsewhere.
 */
void do_apply( CHAR_DATA *ch, char *argument )
{

    char                    arg1[MIL];
    char                    arg2[MIL];
    CHAR_DATA              *victim;
    OBJ_DATA               *salve;
    OBJ_DATA               *obj;
    ch_ret                  retcode;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "¿Aplicar qué?\r\n", ch );
        return;
    }
    if ( ch->fighting ) {
        send_to_char( "Estás demasiado ocupado luchando ...\r\n", ch );
        return;
    }
    if ( ms_find_obj( ch ) )
        return;
    if ( ( salve = get_obj_carry( ch, arg1 ) ) == NULL ) {
        send_to_char( "No tienes éso.\r\n", ch );
        return;
    }

    obj = NULL;
    if ( arg2[0] == '\0' )
        victim = ch;
    else {
        if ( ( victim = get_char_room( ch, arg2 ) ) == NULL
             && ( obj = get_obj_here( ch, arg2 ) ) == NULL ) {
            send_to_char( "¿Aplicarlo a qué o a quién?\r\n", ch );
            return;
        }
    }

    /*
     * apply salve to another object 
     */
    if ( obj ) {
        send_to_char( "No puedes hacer éso... yet.\r\n", ch );
        return;
    }

    if ( victim->fighting ) {
        send_to_char( "No funcionaría muy bien mientras se está luchando ...\r\n", ch );
        return;
    }

    if ( salve->item_type != ITEM_SALVE ) {
        if ( victim == ch ) {
            act( AT_ACTION, "$n empieza a frotar $p sobre $mself...", ch, salve, NULL, TO_ROOM );
            act( AT_ACTION, "Intentas frotar  $p sobre ti mismo...", ch, salve, NULL, TO_CHAR );
        }
        else {
            act( AT_ACTION, "$n empieza a frotar $p sobre $N...", ch, salve, victim, TO_NOTVICT );
            act( AT_ACTION, "$n empieza a frotar  $p sobre ti...", ch, salve, victim, TO_VICT );
            act( AT_ACTION, "Intentas frotar  $p sobre $N...", ch, salve, victim, TO_CHAR );
        }
        return;
    }
    separate_obj( salve );
    --salve->value[1];

    if ( !oprog_use_trigger( ch, salve, NULL, NULL, NULL ) ) {
        if ( !salve->action_desc || salve->action_desc[0] == '\0' ) {
            if ( salve->value[1] < 1 ) {
                if ( victim != ch ) {
                    act( AT_ACTION, "$n frota lo último de $p sobre $N.", ch, salve, victim,
                         TO_NOTVICT );
                    act( AT_ACTION, "$n frota lo último de $p sobre ti.", ch, salve, victim,
                         TO_VICT );
                    act( AT_ACTION, "Frotas lo último de $p sobre $N.", ch, salve, victim, TO_CHAR );
                }
                else {
                    act( AT_ACTION, "Frotas  lo último de $p en ti.", ch, salve, NULL,
                         TO_CHAR );
                    act( AT_ACTION, "$n frota lo último de  $p en $mself.", ch, salve, NULL,
                         TO_ROOM );
                }
            }
            else {
                if ( victim != ch ) {
                    act( AT_ACTION, "$n frota $p en $N.", ch, salve, victim, TO_NOTVICT );
                    act( AT_ACTION, "$n frota $p en ti.", ch, salve, victim, TO_VICT );
                    act( AT_ACTION, "Frotas $p sobre $N.", ch, salve, victim, TO_CHAR );
                }
                else {
                    act( AT_ACTION, "Frotas $p sobre ti.", ch, salve, NULL, TO_CHAR );
                    act( AT_ACTION, "$n frota $p sobre $mself.", ch, salve, NULL, TO_ROOM );
                }
            }
        }
        else
            actiondesc( ch, salve, NULL );
    }

    WAIT_STATE( ch, salve->value[3] );
    retcode = obj_cast_spell( salve->value[4], salve->value[0], ch, victim, NULL );
    if ( retcode == rNONE )
        retcode = obj_cast_spell( salve->value[6], salve->value[0], ch, victim, NULL );
    if ( retcode == rCHAR_DIED || retcode == rBOTH_DIED ) {
        bug( "%s", "do_apply:  char died" );
        return;
    }

    if ( !obj_extracted( salve ) && salve->value[1] <= 0 )
        extract_obj( salve );
    return;
}

/* generate an action description message */
void actiondesc( CHAR_DATA *ch, OBJ_DATA *obj, void *vo )
{
    char                    charbuf[MSL];
    char                    roombuf[MSL];
    char                   *srcptr = obj->action_desc;
    char                   *charptr = charbuf;
    char                   *roomptr = roombuf;
    const char             *ichar = "You";
    const char             *iroom = "Someone";

    while ( *srcptr != '\0' ) {
        if ( *srcptr == '$' ) {
            srcptr++;
            switch ( *srcptr ) {
                case 'e':
                    ichar = "you";
                    iroom = "$e";
                    break;

                case 'm':
                    ichar = "you";
                    iroom = "$m";
                    break;

                case 'n':
                    ichar = "you";
                    iroom = "$n";
                    break;

                case 's':
                    ichar = "your";
                    iroom = "$s";
                    break;

                    /*
                     * case 'q':
                     * iroom = "s";
                     * break;
                     */

                default:
                    srcptr--;
                    *charptr++ = *srcptr;
                    *roomptr++ = *srcptr;
                    break;
            }
        }
        else if ( *srcptr == '%' && *++srcptr == 's' ) {
            ichar = "You";
            iroom = "$n";
        }
        else {
            *charptr++ = *srcptr;
            *roomptr++ = *srcptr;
            srcptr++;
            continue;
        }

        while ( ( *charptr = *ichar ) != '\0' ) {
            charptr++;
            ichar++;
        }

        while ( ( *roomptr = *iroom ) != '\0' ) {
            roomptr++;
            iroom++;
        }
        srcptr++;
    }

    *charptr = '\0';
    *roomptr = '\0';

    switch ( obj->item_type ) {
        case ITEM_BLOOD:
        case ITEM_FOUNTAIN:
            act( AT_ACTION, charbuf, ch, obj, ch, TO_CHAR );
            act( AT_ACTION, roombuf, ch, obj, ch, TO_ROOM );
            return;

        case ITEM_DRINK_CON:
            act( AT_ACTION, charbuf, ch, obj, liq_table[obj->value[2]].liq_name, TO_CHAR );
            act( AT_ACTION, roombuf, ch, obj, liq_table[obj->value[2]].liq_name, TO_ROOM );
            return;

        case ITEM_PIPE:
            return;

        case ITEM_ARMOR:
        case ITEM_WEAPON:
        case ITEM_LIGHT:
            return;

        case ITEM_COOK:
        case ITEM_FOOD:
        case ITEM_PILL:
            act( AT_ACTION, charbuf, ch, obj, ch, TO_CHAR );
            act( AT_ACTION, roombuf, ch, obj, ch, TO_ROOM );
            return;

        default:
            return;
    }
    return;
}

/*
 * Extended Bitvector Routines     -Thoric
 */

/* check to see if the extended bitvector is completely empty */
bool ext_is_empty( EXT_BV * bits )
{
    int                     x;

    for ( x = 0; x < XBI; x++ )
        if ( bits->bits[x] != 0 )
            return FALSE;

    return TRUE;
}

void ext_clear_bits( EXT_BV * bits )
{
    int                     x;

    for ( x = 0; x < XBI; x++ )
        bits->bits[x] = 0;
}

/* for use by xHAS_BITS() -- works like IS_SET() */
int ext_has_bits( EXT_BV * var, EXT_BV * bits )
{
    int                     x,
                            bit;

    for ( x = 0; x < XBI; x++ )
        if ( ( bit = ( var->bits[x] & bits->bits[x] ) ) != 0 )
            return bit;

    return 0;
}

/* for use by xSAME_BITS() -- works like == */
bool ext_same_bits( EXT_BV * var, EXT_BV * bits )
{
    int                     x;

    for ( x = 0; x < XBI; x++ )
        if ( var->bits[x] != bits->bits[x] )
            return FALSE;

    return TRUE;
}

/* for use by xSET_BITS() -- works like SET_BIT() */
void ext_set_bits( EXT_BV * var, EXT_BV * bits )
{
    int                     x;

    for ( x = 0; x < XBI; x++ )
        var->bits[x] |= bits->bits[x];
}

/* for use by xREMOVE_BITS() -- works like REMOVE_BIT() */
void ext_remove_bits( EXT_BV * var, EXT_BV * bits )
{
    int                     x;

    for ( x = 0; x < XBI; x++ )
        var->bits[x] &= ~( bits->bits[x] );
}

/* for use by xTOGGLE_BITS() -- works like TOGGLE_BIT() */
void ext_toggle_bits( EXT_BV * var, EXT_BV * bits )
{
    int                     x;

    for ( x = 0; x < XBI; x++ )
        var->bits[x] ^= bits->bits[x];
}

/*
 * Read an extended bitvector from a file.   -Thoric
 */
EXT_BV fread_bitvector( FILE * fp )
{
    EXT_BV                  ret;
    int                     c,
                            x = 0;
    int                     num = 0;

    memset( &ret, '\0', sizeof( ret ) );
    for ( ;; ) {
        num = fread_number( fp );
        if ( x < XBI )
            ret.bits[x] = num;
        ++x;
        if ( ( c = getc( fp ) ) != '&' ) {
            ungetc( c, fp );
            break;
        }
    }

    return ret;
}

/* return a string for writing a bitvector to a file */
char                   *print_bitvector( EXT_BV * bits )
{
    static char             buf[XBI * 12];
    char                   *p = buf;
    int                     x,
                            cnt = 0;

    for ( cnt = XBI - 1; cnt > 0; cnt-- )
        if ( bits->bits[cnt] )
            break;
    for ( x = 0; x <= cnt; x++ ) {
        snprintf( p, ( XBI * 12 ) - ( p - buf ), "%d", bits->bits[x] );
        p += strlen( p );
        if ( x < cnt )
            *p++ = '&';
    }
    *p = '\0';

    return buf;
}

/*
 * Write an extended bitvector to a file   -Thoric
 */
void fwrite_bitvector( EXT_BV * bits, FILE * fp )
{
    fputs( print_bitvector( bits ), fp );
}

EXT_BV meb( int bit )
{
    EXT_BV                  bits;

    xCLEAR_BITS( bits );
    if ( bit >= 0 )
        xSET_BIT( bits, bit );

    return bits;
}

EXT_BV multimeb( int bit, ... )
{
    EXT_BV                  bits;
    va_list                 param;
    int                     b;

    xCLEAR_BITS( bits );
    if ( bit < 0 )
        return bits;

    xSET_BIT( bits, bit );

    va_start( param, bit );

    while ( ( b = va_arg( param, int ) ) != -1 )
                                xSET_BIT( bits, b );

    va_end( param );

    return bits;
}

void make_bloodstain( CHAR_DATA *ch )
{
    OBJ_DATA               *obj;

    obj = create_object( get_obj_index( OBJ_VNUM_BLOODSTAIN ), 0 );
    obj->timer = number_range( 1, 2 );
    obj_to_room( obj, ch->in_room );
}

/*
 * Simple wraparound for fprintf, to save troublesome calls. -Orion
 */
bool fptof( FILE * stream, const char *data )
{
    if ( fprintf( stream, "%s", data ) )
        return TRUE;

    return FALSE;
}

#ifdef WIN32

/* routines not in Windows runtime libraries */
#include <time.h>
void gettimeofday( struct timeval *tv, struct timezone *tz )
{
    tv->tv_sec = time( 0 );
    tv->tv_usec = 0;
}

/* directory parsing stuff */

DIR                    *opendir( char *sDirName )
{
    DIR                    *dp = ( DIR * ) malloc( sizeof( DIR ) );

    dp->hDirectory = 0;                                /* if zero, we must do a
                                                        * FindFirstFile */
    strcpy( dp->sDirName, sDirName );                  /* remember for FindFirstFile */
    return dp;
}

struct dirent          *readdir( DIR * dp )
{

    /*
     * either read the first entry, or the next entry 
     */
    do {
        if ( dp->hDirectory == 0 ) {
            dp->hDirectory = FindFirstFile( dp->sDirName, &dp->Win32FindData );
            if ( dp->hDirectory == INVALID_HANDLE_VALUE )
                return NULL;
        }
        else if ( !FindNextFile( dp->hDirectory, &dp->Win32FindData ) )
            return NULL;

        /*
         * skip directories 
         */

    }
    while ( dp->Win32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );

    /*
     * make a copy of the name string 
     */
    dp->dirinfo.d_name = dp->Win32FindData.cFileName;

/* return a pointer to the DIR structure */

    return &dp->dirinfo;
}

void closedir( DIR * dp )
{
    if ( dp->hDirectory )
        FindClose( dp->hDirectory );
    free( dp );
}

#endif
