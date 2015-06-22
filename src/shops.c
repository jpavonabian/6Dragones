/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.0 (C) 1994, 1995, 1996 by Derek Snider             |).:.(*
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh and Tricops  |~'~.VxvxV.~'~*
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *    Shop and repair shop module       *
 ****************************************************************************/

/*static char rcsid[] = "$Id: shops.c,v 1.10 2002/10/12 20:06:11 dotd Exp $";*/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "h/mud.h"
#include "h/currency.h"
#include "h/shops.h"
#include "h/clans.h"

/*
 * Local functions
 */

#define  CD  CHAR_DATA
CD                     *find_keeper args( ( CHAR_DATA *ch ) );
CD                     *find_fixer args( ( CHAR_DATA *ch ) );

#define SMITH_PRODUCT    41002
int get_repaircost      args( ( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj ) );

#undef CD
void                    colorize_keeper( OBJ_DATA *obj, CHAR_DATA *ch );

/*
 * Shopping commands.
 */
CHAR_DATA              *find_keeper( CHAR_DATA *ch )
{
    char                    buf[MSL];
    CHAR_DATA              *keeper;
    SHOP_DATA              *pShop;
    int                     speakswell;

    pShop = NULL;
    for ( keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room )
        if ( IS_NPC( keeper ) && ( pShop = keeper->pIndexData->pShop ) != NULL )
            break;

    if ( !pShop ) {
        send_to_char( "No puedes hacer eso aqu�.\r\n", ch );
        return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) ) {
        interpret( keeper, ( char * ) "dice No hago tratos con gente que no veo." );
        return NULL;
    }

    /*
     * Undesirables.
     */
    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_KILLER ) ) {
        interpret( keeper, ( char * ) "dice �Los asesinos no son bienvenidos en este lugar!" );
        snprintf( buf, MSL, "grita %s el ASESINO est� aqu�!\r\n", ch->name );
        interpret( keeper, buf );
        return NULL;
    }

    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_THIEF ) ) {
        interpret( keeper, ( char * ) "dice Los ladrones no son bienvenidos!" );
        snprintf( buf, MSL, "grita  %s el ladr�n est� aqu�!\r\n", ch->name );
        interpret( keeper, buf );
        return NULL;
    }

    /*
     * Shop hours.
     */
    if ( time_info.hour < pShop->open_hour ) {
        interpret( keeper, ( char * ) "dice  Lo siento, est� cerrado, vuelve ma�s tarde." );
        return NULL;
    }

    if ( time_info.hour > pShop->close_hour ) {
        interpret( keeper, ( char * ) "say Lo siento, est� cerrado, vuelve ma�ana." );
        return NULL;
    }

    speakswell =
        UMIN( knows_language( keeper, ch->speaking, ch ),
              knows_language( ch, ch->speaking, keeper ) );

    if ( ( number_percent(  ) % 65 ) > speakswell ) {
        if ( speakswell > 60 )
            snprintf( buf, MSL, "%s Puedes repetir? no te he entendido.", ch->name );
        else if ( speakswell > 50 )
            snprintf( buf, MSL, "%s Podr�as decirlo un poco m�s claramente?", ch->name );
        else if ( speakswell > 40 )
            snprintf( buf, MSL, "%s Disculpa... Que quer�as?", ch->name );
        else
            snprintf( buf, MSL, "%s No te entiendo.", ch->name );
        do_tell( keeper, buf );
        return NULL;
    }

    return keeper;
}

/*
 * repair commands.
 */
CHAR_DATA              *find_fixer( CHAR_DATA *ch )
{
    char                    buf[MSL];
    CHAR_DATA              *keeper;
    REPAIR_DATA            *rShop;
    int                     speakswell;

    rShop = NULL;
    for ( keeper = ch->in_room->first_person; keeper; keeper = keeper->next_in_room )
        if ( IS_NPC( keeper ) && ( rShop = keeper->pIndexData->rShop ) != NULL )
            break;

    if ( !rShop ) {
        send_to_char( "No puedes hacer eso aqu�.\r\n", ch );
        return NULL;
    }

    /*
     * Shop hours.
     */
    if ( time_info.hour < rShop->open_hour ) {
        interpret( keeper, ( char * ) "dice Lo siento, est� cerrado, vuelve m�s tarde." );
        return NULL;
    }

    if ( time_info.hour > rShop->close_hour ) {
        interpret( keeper, ( char * ) "Dice Lo siento, est� cerrado, vuelve ma�ana." );
        return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if ( !can_see( keeper, ch ) ) {
        interpret( keeper, ( char * ) "Dice no hago tratos con gente que no veo." );
        return NULL;
    }

    /*
     * Undesirables.
     */
    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_KILLER ) ) {
        interpret( keeper, ( char * ) "dice Los asesinos no son bienvenidos!" );
        snprintf( buf, MSL, "grita %s el asesino est� aqu�!\r\n", ch->name );
        interpret( keeper, buf );
        return NULL;
    }

    if ( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_THIEF ) ) {
        interpret( keeper, ( char * ) "dice los ladrones no son bienvenidos!" );
        snprintf( buf, MSL, "grita %s el ladr�n est� aqu�!\r\n", ch->name );
        interpret( keeper, buf );
        return NULL;
    }

    speakswell =
        UMIN( knows_language( keeper, ch->speaking, ch ),
              knows_language( ch, ch->speaking, keeper ) );

    if ( ( number_percent(  ) % 65 ) > speakswell ) {
        if ( speakswell > 60 )
            snprintf( buf, MSL, "%s puedes repetirlo? No lo he entendido bien.", ch->name );
        else if ( speakswell > 50 )
            snprintf( buf, MSL, "%s Podr�as decirlo un poco m�s claramente?", ch->name );
        else if ( speakswell > 40 )
            snprintf( buf, MSL, "%s Lo siento... Qu� quer�as?", ch->name );
        else
            snprintf( buf, MSL, "%s No te entiendo.", ch->name );
        do_tell( keeper, buf );
        return NULL;
    }

    return keeper;
}

bool will_buy( CHAR_DATA *keeper, OBJ_DATA *obj )
{
    SHOP_DATA              *pShop;
    OBJ_DATA               *obj2;
    int                     itype;

    if ( !obj || ( pShop = keeper->pIndexData->pShop ) == NULL )
        return FALSE;

    for ( itype = 0; itype < MAX_TRADE; itype++ )
        if ( obj->item_type == pShop->buy_type[itype] )
            break;

    if ( obj->item_type != pShop->buy_type[itype] )
        return FALSE;

    for ( obj2 = keeper->first_carrying; obj2; obj2 = obj2->next_content )
        if ( obj->pIndexData == obj2->pIndexData )
            return FALSE;

    return TRUE;
}

int get_cost( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
    /*
     * fBuy here means the player is buying - ie using profit_buy's margin 
     */
    SHOP_DATA              *pShop;
    int                     cost = 0;

    if ( !obj || ( pShop = keeper->pIndexData->pShop ) == NULL )
        return 0;

    if ( fBuy ) {                                      /* Volk - if fBuy is TRUE, then
                                                        * player is buying - apply buying 
                                                        * * * * * * margin */
        cost = ( obj->pIndexData->cost * pShop->profit_buy / 100 );

        // Make cha have a affect with shops
        if ( get_curr_cha( ch ) < 11 ) {
            cost += 50;
        }
        else if ( get_curr_cha( ch ) > 10 && get_curr_cha( ch ) < 13 ) {
            cost += 40;
        }
        else if ( get_curr_cha( ch ) > 12 && get_curr_cha( ch ) < 14 ) {
            cost += 30;
        }
        else if ( get_curr_cha( ch ) > 13 && get_curr_cha( ch ) < 17 ) {
            cost += 25;
        }
        else if ( get_curr_cha( ch ) > 16 && get_curr_cha( ch ) < 21 ) {
            cost += 10;
        }
        else if ( get_curr_cha( ch ) > 20 && get_curr_cha( ch ) < 24 ) {
            cost += 5;
        }
        else if ( get_curr_cha( ch ) > 23 ) {
            cost -= 1;
        }

        /*
         * Lets allow influence to change cost of items 
         */
        if ( cost > 0 && IS_CLANNED( ch ) && str_cmp( ch->pcdata->clan->name, "Halcyon" ) ) {
            if ( IS_INFLUENCED( ch, ch->in_room->area ) )   /* If their clan is * * * * * 
                                                             * influencing the area * * * 
                                                             * * * * sell it to them *
                                                             * cheaper */
                cost -= ( int ) ( cost * .10 );
            else                                       /* Ok they are clanned and not * * 
                                                        * halcyon so charge more */
                cost += ( int ) ( cost * .10 );
            if ( cost <= 0 )
                cost = 1;
        }
    }
    else                                               /* Volk - selling */
        cost = ( obj->cost * pShop->profit_sell / 100 );

    if ( obj->count > 1 )
        cost *= obj->count;

    return cost;
}

int get_repaircost( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj )
{
    REPAIR_DATA            *rShop;
    int                     cost;
    int                     itype;
    bool                    found;

    if ( !obj || ( rShop = keeper->pIndexData->rShop ) == NULL )
        return 0;

    cost = 0;
    found = FALSE;
    for ( itype = 0; itype < MAX_FIX; itype++ ) {
        if ( obj->item_type == rShop->fix_type[itype] ) {
            cost = ( int ) ( obj->cost * rShop->profit_fix / 1000 );
            found = TRUE;
            break;
        }
    }

    if ( !found )
        cost = -1;

    if ( cost == 0 )
        cost = 1;

    if ( found && cost > 0 ) {
        switch ( obj->item_type ) {
            case ITEM_ARMOR:
                if ( obj->value[0] >= obj->value[1] )
                    cost = -2;
                else
                    cost *= ( obj->value[1] - obj->value[0] );
                break;
            case ITEM_WEAPON:
                if ( INIT_WEAPON_CONDITION == obj->value[0] )
                    cost = -2;
                else
                    cost *= ( INIT_WEAPON_CONDITION - obj->value[0] );
                break;
            case ITEM_WAND:
            case ITEM_STAFF:
                if ( obj->value[2] >= obj->value[1] )
                    cost = -2;
                else
                    cost *= ( obj->value[1] - obj->value[2] );
        }
    }

    /*
     * Lets allow influence to change cost of items 
     */
    if ( cost > 0 && IS_CLANNED( ch ) && str_cmp( ch->pcdata->clan->name, "Halcyon" ) ) {
        if ( IS_INFLUENCED( ch, ch->in_room->area ) )  /* If their clan is influencing *
                                                        * * * * * the area * sell it to * 
                                                        * them * * * * cheaper */
            cost -= ( int ) ( cost * .10 );
        else                                           /* Ok they are clanned and not * * 
                                                        * halcyon so charge more */
            cost += ( int ) ( cost * .10 );
        if ( cost <= 0 )
            cost = 1;
    }

    return cost;
}

/* Lets see if the character is able to afford the object */
bool can_afford( CHAR_DATA *ch, double cost, short fctype )
{
    double                  has;

    if ( fctype == CURR_GOLD )
        cost *= 1000;
    else if ( fctype == CURR_SILVER )
        cost *= 100;
    else if ( fctype == CURR_BRONZE )
        cost *= 10;

    has = ( GET_MONEY( ch, CURR_GOLD ) * 1000 );
    has += ( GET_MONEY( ch, CURR_SILVER ) * 100 );
    has += ( GET_MONEY( ch, CURR_BRONZE ) * 10 );
    has += ( GET_MONEY( ch, CURR_COPPER ) );

    if ( cost > has )
        return FALSE;
    return TRUE;
}

void convert_coins( CHAR_DATA *ch, int needed, short tctype, short fctype, int climit )
{
    while ( GET_MONEY( ch, fctype ) >= climit ) {
        GET_MONEY( ch, tctype ) += 1;
        GET_MONEY( ch, fctype ) -= climit;
        if ( GET_MONEY( ch, tctype ) >= needed )
            return;
    }
}

/*
 * We will start out seeing if they have enough of the specified type if not
 * then we will see if we can convert up, if not then convert down
 * returns true once they have enough
 */
bool spend_coins( CHAR_DATA *ch, CHAR_DATA *keeper, int cost, short ctype )
{
    bool                    converted = FALSE;

    while ( cost > 0 ) {
        if ( ctype == CURR_GOLD ) {
            if ( GET_MONEY( ch, CURR_GOLD ) >= cost ) {
                GET_MONEY( ch, ctype ) -= cost;
                cost -= cost;
                continue;
            }
            converted = TRUE;
            /*
             * Transfer everything up to gold 
             */
            convert_coins( ch, cost, CURR_GOLD, CURR_COPPER, 1000 );
            if ( GET_MONEY( ch, CURR_GOLD ) >= cost )
                continue;
            convert_coins( ch, cost, CURR_GOLD, CURR_BRONZE, 100 );
            if ( GET_MONEY( ch, CURR_GOLD ) >= cost )
                continue;
            convert_coins( ch, cost, CURR_GOLD, CURR_SILVER, 10 );
            if ( GET_MONEY( ch, CURR_GOLD ) >= cost )
                continue;
            /*
             * If we still haven't gotten enough have to transfer up as we go till we do
             * have enough 
             */
            /*
             * First lets transfer copper up as much as possible 
             */
            convert_coins( ch, 1000, CURR_SILVER, CURR_COPPER, 100 );
            convert_coins( ch, 100, CURR_BRONZE, CURR_COPPER, 10 );
            /*
             * Next lets transfer up our bronze 
             */
            convert_coins( ch, 100, CURR_SILVER, CURR_BRONZE, 10 );
            /*
             * Finaly lets transfer up our silver again 
             */
            convert_coins( ch, cost, CURR_GOLD, CURR_SILVER, 10 );
            if ( GET_MONEY( ch, CURR_GOLD ) >= cost )
                continue;
        }
        else if ( ctype == CURR_SILVER ) {
            if ( GET_MONEY( ch, CURR_SILVER ) >= cost ) {
                GET_MONEY( ch, ctype ) -= cost;
                cost -= cost;
                continue;
            }
            converted = TRUE;
            /*
             * Transfer everything to silver 
             */
            convert_coins( ch, cost, CURR_SILVER, CURR_COPPER, 100 );
            if ( GET_MONEY( ch, CURR_SILVER ) >= cost )
                continue;
            convert_coins( ch, cost, CURR_SILVER, CURR_BRONZE, 10 );
            if ( GET_MONEY( ch, CURR_SILVER ) >= cost )
                continue;
            /*
             * If we still haven't gotten enough have to transfer up as we go till we do
             * have enough 
             */
            /*
             * First lets transfer copper up as much as possible 
             */
            convert_coins( ch, 100, CURR_BRONZE, CURR_COPPER, 10 );
            /*
             * Next lets transfer up our bronze 
             */
            convert_coins( ch, 100, CURR_SILVER, CURR_BRONZE, 10 );
            if ( GET_MONEY( ch, CURR_SILVER ) >= cost )
                continue;
            /*
             * If we made it this far we need to bring down gold 
             */
            while ( GET_MONEY( ch, CURR_GOLD ) > 0 ) {
                GET_MONEY( ch, CURR_GOLD ) -= 1;
                GET_MONEY( ch, CURR_SILVER ) += 10;
                if ( GET_MONEY( ch, CURR_SILVER ) >= cost )
                    break;
            }
        }
        else if ( ctype == CURR_BRONZE ) {
            if ( GET_MONEY( ch, CURR_BRONZE ) >= cost ) {
                GET_MONEY( ch, ctype ) -= cost;
                cost -= cost;
                continue;
            }
            converted = TRUE;
            /*
             * Transfer everything to bronze 
             */
            convert_coins( ch, cost, CURR_BRONZE, CURR_COPPER, 10 );
            if ( GET_MONEY( ch, CURR_BRONZE ) >= cost )
                continue;
            /*
             * If we made it this far we need to bring down silver 
             */
            while ( GET_MONEY( ch, CURR_SILVER ) > 0 ) {
                GET_MONEY( ch, CURR_SILVER ) -= 1;
                GET_MONEY( ch, CURR_BRONZE ) += 10;
                if ( GET_MONEY( ch, CURR_BRONZE ) >= cost )
                    break;
            }
            if ( GET_MONEY( ch, CURR_BRONZE ) >= cost )
                continue;
            /*
             * If we made it this far we need to bring down gold 
             */
            while ( GET_MONEY( ch, CURR_GOLD ) > 0 ) {
                GET_MONEY( ch, CURR_GOLD ) -= 1;
                GET_MONEY( ch, CURR_SILVER ) += 10;
                /*
                 * Now as we do this lets break down the silver 
                 */
                while ( GET_MONEY( ch, CURR_SILVER ) > 0 ) {
                    GET_MONEY( ch, CURR_SILVER ) -= 1;
                    GET_MONEY( ch, CURR_BRONZE ) += 10;
                    if ( GET_MONEY( ch, CURR_BRONZE ) >= cost )
                        break;
                }
                if ( GET_MONEY( ch, CURR_BRONZE ) >= cost )
                    break;
            }
        }
        else if ( ctype == CURR_COPPER ) {
            if ( GET_MONEY( ch, CURR_COPPER ) >= cost ) {
                GET_MONEY( ch, ctype ) -= cost;
                cost -= cost;
                continue;
            }
            converted = TRUE;
            /*
             * Transfer everything to copper 
             */
            while ( GET_MONEY( ch, CURR_BRONZE ) > 0 ) {
                GET_MONEY( ch, CURR_BRONZE ) -= 1;
                GET_MONEY( ch, CURR_COPPER ) += 10;
                if ( GET_MONEY( ch, CURR_COPPER ) >= cost )
                    break;
            }
            if ( GET_MONEY( ch, CURR_COPPER ) >= cost )
                continue;
            /*
             * If we made it this far we need to bring down silver 
             */
            while ( GET_MONEY( ch, CURR_SILVER ) > 0 ) {
                GET_MONEY( ch, CURR_SILVER ) -= 1;
                GET_MONEY( ch, CURR_BRONZE ) += 10;
                while ( GET_MONEY( ch, CURR_BRONZE ) > 0 ) {
                    GET_MONEY( ch, CURR_BRONZE ) -= 1;
                    GET_MONEY( ch, CURR_COPPER ) += 10;
                    if ( GET_MONEY( ch, CURR_COPPER ) >= cost )
                        break;
                }
                if ( GET_MONEY( ch, CURR_COPPER ) >= cost )
                    break;
            }
            if ( GET_MONEY( ch, CURR_COPPER ) >= cost )
                continue;
            /*
             * If we made it this far we need to bring down gold 
             */
            while ( GET_MONEY( ch, CURR_GOLD ) > 0 ) {
                GET_MONEY( ch, CURR_GOLD ) -= 1;
                GET_MONEY( ch, CURR_SILVER ) += 10;
                while ( GET_MONEY( ch, CURR_SILVER ) > 0 ) {
                    GET_MONEY( ch, CURR_SILVER ) -= 1;
                    GET_MONEY( ch, CURR_BRONZE ) += 10;
                    while ( GET_MONEY( ch, CURR_BRONZE ) > 0 ) {
                        GET_MONEY( ch, CURR_BRONZE ) -= 1;
                        GET_MONEY( ch, CURR_COPPER ) += 10;
                        if ( GET_MONEY( ch, CURR_COPPER ) >= cost )
                            break;
                    }
                    if ( GET_MONEY( ch, CURR_COPPER ) >= cost )
                        break;
                }
                if ( GET_MONEY( ch, CURR_COPPER ) >= cost )
                    break;
            }
        }
    }
    return converted;
}

bool buy_obj( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj, int noi )
{
    char                    buf[MSL];
    const char             *string;
    int                     cost;
    bool                    converted = FALSE;

    cost = ( get_cost( ch, keeper, obj, TRUE ) * noi );
    if ( !can_afford( ch, cost, obj->currtype ) ) {
        act( AT_TELL, "$n te cuenta 'lo siento, no tienes suficiente dinero para comprar $p.'", keeper, obj, ch,
             TO_VICT );
        ch->reply = keeper;
        return FALSE;
    }
    /*
     * Lets go ahead and handle all the coinage stuff, It doesn't give alot of info but
     * should at least convert them correctly 
     */
    converted = spend_coins( ch, keeper, cost, obj->currtype );
    if ( obj->currtype == CURR_GOLD )
        string = "gold";
    else if ( obj->currtype == CURR_SILVER )
        string = "silver";
    else if ( obj->currtype == CURR_BRONZE )
        string = "bronze";
    else
        string = "copper";

    if ( obj->pIndexData->vnum == 41002 || obj->pIndexData->vnum == 41004 ) {
        cost = obj->cost;
        if ( obj->currtype == CURR_GOLD )
            string = "gold";
        else if ( obj->currtype == CURR_SILVER )
            string = "silver";
        else if ( obj->currtype == CURR_BRONZE )
            string = "bronze";
        else
            string = "copper";

    }

    if ( noi == 1 ) {
        act( AT_ACTION, "$n compra $p.", ch, obj, NULL, TO_ROOM );
        if ( converted )
            snprintf( buf, MSL, "Compras $p. Le das todo tu dinero al tendero." );
        else
            snprintf( buf, MSL, "Compras $p por %d %s moneda%s.", cost, string, cost > 1 ? "s" : "" );
        act( AT_ACTION, buf, ch, obj, NULL, TO_CHAR );
        if ( converted )
            act( AT_ACTION, "El tendero coge lo que $E necesita y te devuelve el resto.",
                 ch, NULL, keeper, TO_CHAR );
    }
    else if ( noi > 1 ) {
        snprintf( buf, MSL, "$n compra %d de $p.", noi );
        act( AT_ACTION, buf, ch, obj, NULL, TO_ROOM );
        if ( converted )
            snprintf( buf, MSL, "Compras %d de $p. Le das todo tu dinero al tendero.",
                      noi );
        else
            snprintf( buf, MSL, "Compras %d de $p por %d %s moneda%s.", noi, cost, string,
                      cost > 1 ? "s" : "" );
        act( AT_ACTION, buf, ch, obj, NULL, TO_CHAR );
        act( AT_ACTION, "El tendero pone todo en una bolsa y te la da.", ch, NULL, NULL,
             TO_CHAR );
        if ( converted )
            act( AT_ACTION, "El tendero coge lo que necesita y te devuelve el resto.",
                 ch, NULL, keeper, TO_CHAR );
    }
    return TRUE;
}

void do_buy( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    int                     maxmoney,
                            cost;
    CHAR_DATA              *keeper;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Comprar qu�?\r\n", ch );
        return;
    }

    if ( IS_SET( ch->in_room->room_flags, ROOM_PET_SHOP ) ) {
        char                    buf[MSL];
        CHAR_DATA              *pet;
        ROOM_INDEX_DATA        *pRoomIndexNext;
        ROOM_INDEX_DATA        *in_room;

        if ( IS_NPC( ch ) )
            return;

        pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
        if ( !pRoomIndexNext ) {
            bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
            send_to_char( "Lo siento, no puedes \r\n", ch );
            return;
        }

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;

        in_room = ch->in_room;
        ch->in_room = pRoomIndexNext;
        pet = get_char_room( ch, arg );
        ch->in_room = in_room;

        if ( pet == NULL || !IS_NPC( pet ) || !xIS_SET( pet->act, ACT_PET ) ) {
            send_to_char( "Lo siento, no puedes comprar eso aqu�\r\n", ch );
            return;
        }

        if ( xIS_SET( ch->act, PLR_BOUGHT_PET ) ) {
            send_to_char( "Ya has comprado una mascota\r\n", ch );
            return;
        }
        if ( pet->level > 100 )
            pet->level = 100;

        cost = ( pet->level * 100 );
        if ( !can_afford( ch, cost, CURR_SILVER ) ) {
            send_to_char( "No te lo puedes permitir\r\n", ch );
            return;
        }

        if ( ch->level < pet->level ) {
            send_to_char( "Are you kidding? That thing will take you for walks!\r\n", ch );
            return;
        }

        spend_coins( ch, keeper, cost, CURR_SILVER );
        GET_MONEY( keeper, CURR_SILVER ) += cost;
        if ( GET_MONEY( keeper, CURR_SILVER ) >= 50000 ) {
            boost_economy( keeper->in_room->area, 25000, CURR_SILVER );
            GET_MONEY( keeper, CURR_SILVER ) -= 25000;
            act( AT_ACTION, "$n pone algo de $T a buen recaudo.", keeper, NULL,
                 curr_types[CURR_SILVER], TO_ROOM );
        }
        pet = create_mobile( pet->pIndexData );
        xSET_BIT( ch->act, PLR_BOUGHT_PET );
        xSET_BIT( pet->act, ACT_PET );
        xSET_BIT( pet->affected_by, AFF_CHARM );
        xSET_BIT( pet->affected_by, AFF_TRUESIGHT );
        argument = one_argument( argument, arg );
        if ( arg[0] != '\0' ) {
            snprintf( buf, MSL, "%s %s", pet->name, arg );
            if ( VLD_STR( pet->name ) )
                STRFREE( pet->name );
            pet->name = STRALLOC( buf );
        }

        snprintf( buf, MSL, "%suna etiqueta en su cuello dice 'Pertenezco a %s'.\r\n", pet->description, ch->name );
        if ( VLD_STR( pet->description ) )
            STRFREE( pet->description );
        pet->description = STRALLOC( buf );

        char_to_room( pet, ch->in_room );
        add_follower( pet, ch );
        send_to_char( "Disfrute su mascota\r\n", ch );
        act( AT_ACTION, "$n compr� $N como mascota.", ch, NULL, pet, TO_ROOM );
        return;
    }
    else {                                             /* not a pet shop - volk */

        OBJ_DATA               *obj;
        int                     cost,
                                noi = 1;
        short                   primcurr,
                                mnoi = 20;

        if ( !( keeper = find_keeper( ch ) ) )
            return;

        maxmoney = keeper->level * 50000;

        if ( is_number( arg ) ) {
            noi = atoi( arg );
            argument = one_argument( argument, arg );
            if ( noi > mnoi ) {
                act( AT_TELL, "$n te cuenta 'No vendo tantos objetos a la vez " "'", keeper,
                     NULL, ch, TO_VICT );
                ch->reply = keeper;
                return;
            }
        }

        obj = get_obj_carry( keeper, arg );
        cost = ( get_cost( ch, keeper, obj, TRUE ) );

        if ( obj && ( obj->pIndexData->vnum == 41002 || obj->pIndexData->vnum == 41004 ) ) {
            cost = obj->cost;
        }

        /*
         * The TRUE part of get_cost means playing is BUYING item, not selling - Volk
         */
        if ( cost <= 0 || !can_see_obj( ch, obj ) ) {
            act( AT_TELL, "$n te cuenta 'No vendo eso -- prueba 'lista'.'", keeper, NULL, ch,
                 TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( obj->level > ch->level ) {
            act( AT_TELL,
                 "$n te cuenta 'Ese objeto es demasiado poderoso para ti. Espero que sepas lo que haces.",
                 keeper, NULL, ch, TO_VICT );
            interpret( keeper, ( char * ) "shrug" );
            ch->reply = keeper;
//         return;
        }

        if ( !IS_OBJ_STAT( obj, ITEM_INVENTORY ) && ( noi > 1 ) ) {
            interpret( keeper, ( char * ) "laugh" );
            act( AT_TELL,
                 "$n te cuenta 'No tengo suficientes objetos de ese tipo a la venta."
                 " a vender mas de uno a la vez.'", keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( xIS_SET( obj->extra_flags, ITEM_PROTOTYPE ) && get_trust( ch ) < LEVEL_IMMORTAL ) {
            act( AT_TELL, "$n te cuenta 'Solo es un prototipo! No puedo venderte eso...'",
                 keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
        }

        if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) ) {
            send_to_char( "No puedes llevar tantos objetos.\r\n", ch );
            return;
        }

        if ( ch->carry_weight + ( get_obj_weight( obj, FALSE ) * noi ) + ( noi > 1 ? 2 : 0 ) >
             can_carry_w( ch ) ) {
            send_to_char( "No puedes llevar tanto peso\r\n", ch );
            return;
        }

        /*
         * New here 
         */
        if ( !buy_obj( ch, keeper, obj, noi ) )
            return;

        primcurr = obj->currtype;
        cost *= noi;
        GET_MONEY( keeper, primcurr ) += cost;

        if ( GET_MONEY( keeper, primcurr ) >= maxmoney ) {
            boost_economy( keeper->in_room->area, ( maxmoney / 2 ), primcurr );
            GET_MONEY( keeper, primcurr ) -= ( maxmoney / 2 );
            act( AT_ACTION, "$n pone algo de $T a buen recaudo.", keeper, NULL,
                 curr_types[primcurr], TO_ROOM );
        }

        if ( IS_OBJ_STAT( obj, ITEM_INVENTORY ) ) {
            OBJ_DATA               *bag;
            OBJ_DATA               *newobj;

            newobj = create_object( obj->pIndexData, obj->level );

            if ( noi > 1 ) {
                bag = create_object( get_obj_index( OBJ_VNUM_SHOPPING_BAG ), 0 );
                /*
                 * perfect size bag ;) 
                 */
                bag->value[0] = bag->weight + ( newobj->weight * noi );
                newobj->count = noi;
                obj->pIndexData->count += ( noi - 1 );
                numobjsloaded += ( noi - 1 );
                obj_to_obj( newobj, bag );
                obj_to_char( bag, ch );
            }
            else
                obj_to_char( newobj, ch );
        }
        else {
            obj_from_char( obj );
            obj_to_char( obj, ch );
        }

        return;
    }
}

void do_list( CHAR_DATA *ch, char *argument )
{
    const char             *divleft = "-----------------------------------[ ";
    const char             *divright = " ]-----------------------------------";
    int                     lower,
                            upper;
    CHAR_DATA              *keeper;

    lower = -2;
    upper = -1;

    if ( IS_SET( ch->in_room->room_flags, ROOM_PET_SHOP ) ) {
        ROOM_INDEX_DATA        *pRoomIndexNext;
        CHAR_DATA              *pet;
        bool                    found;

        pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
        if ( !pRoomIndexNext ) {
            bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
            send_to_char( "No puedes hacer eso aqu�.\r\n", ch );
            return;
        }

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;

        found = FALSE;
        for ( pet = pRoomIndexNext->first_person; pet; pet = pet->next_in_room ) {
            if ( xIS_SET( pet->act, ACT_PET ) && IS_NPC( pet ) ) {
                if ( !found ) {
                    found = TRUE;
                    send_to_char( "Mascotas a la venta :\r\n", ch );
                    ch_printf( ch, "[Lv   Price Currency] Item\r\n" );
                }
                if ( pet->level > 100 )
                    pet->level = 100;
                ch_printf( ch, "[%2d] %4d   plata - %s\r\n", pet->level, 100 * pet->level,
                           pet->short_descr );
            }
        }
        if ( !found )
            send_to_char( "Lo siento, ahora mismo no nos quedan mascotas\r\n", ch );
        return;
    }
    else {
        char                    arg[MIL],
                                arg2[MIL],
                                buf[MSL];
        OBJ_DATA               *obj;
        int                     cost = 0;
        bool                    found,
                                inventory = FALSE;
        AFFECT_DATA            *paf;

        argument = one_argument( argument, arg );
        argument = one_argument( argument, arg2 );

        if ( ( keeper = find_keeper( ch ) ) == NULL )
            return;

        if ( !str_cmp( arg, "stat" ) ) {
            if ( !arg2 || arg2[0] == '\0' ) {
                send_to_char( "Sintaxis: list stat <objeto>\r\n", ch );
                return;
            }
            obj = get_obj_carry( keeper, arg2 );
            if ( !obj || !can_see_obj( ch, obj ) ) {
                act( AT_TELL, "$N te cuenta 'No tengo nada como eso'", ch, NULL, keeper,
                     TO_CHAR );
                ch->reply = keeper;
                return;
            }

            if ( IS_OBJ_STAT( obj, ITEM_INVENTORY ) ) {
                xREMOVE_BIT( obj->extra_flags, ITEM_INVENTORY );
                inventory = TRUE;
            }
            act( AT_CYAN, "$N coge $p de un escaparate.", ch, obj, keeper, TO_CHAR );
            act( AT_TELL, "$N te cuenta 'este objeto de aqu� se usa para......'", ch, NULL, keeper,
                 TO_CHAR );
            snprintf( buf, MSL, "Propiedades especiales: %s",
                      !xIS_EMPTY( obj->extra_flags ) ? ext_flag_string( &obj->extra_flags,
                                                                        o_flags ) : "Nothing" );
            act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
            snprintf( buf, MSL, "Es un %s.", item_type_name( obj ) );
            act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
            snprintf( buf, MSL, "Su peso es %d y su nivel es %d", obj->weight, obj->level );
            act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
            snprintf( buf, MSL, "Las posibles partes del cuerpo para llevar este objeto son %s",
                      flag_string( obj->wear_flags, w_flags ) );
            act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
            snprintf( buf, MSL, "Tama�o necesario para llevarlo: %s\r\n", obj_sizes[obj->size] );
            act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
            if ( inventory )
                xSET_BIT( obj->extra_flags, ITEM_INVENTORY );

            switch ( obj->item_type ) {
                case ITEM_CONTAINER:
                case ITEM_KEYRING:
                case ITEM_QUIVER:
                    snprintf( buf, MSL, "%s aparece en %s.",
                              capitalize( obj->short_descr ),
                              obj->value[0] <
                              76 ? "have a small capacity" : obj->value[0] <
                              150 ? "have a small to medium capacity" : obj->value[0] <
                              300 ? "have a medium capacity" : obj->value[0] <
                              500 ? "have a medium to large capacity" : obj->value[0] <
                              751 ? "have a large capacity" : "have a giant capacity" );
                    act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
                    break;
                case ITEM_PILL:
                case ITEM_SCROLL:
                case ITEM_POTION:
                    snprintf( buf, MSL, "Nivel %d spells of: ", obj->value[0] );
                    if ( obj->value[1] >= 0 && obj->value[1] < top_sn ) {
                        mudstrlcat( buf, "'", MSL );
                        mudstrlcat( buf, skill_table[obj->value[1]]->name, MSL );
                        mudstrlcat( buf, "'", MSL );
                    }
                    if ( obj->value[2] >= 0 && obj->value[2] < top_sn ) {
                        mudstrlcat( buf, "'", MSL );
                        mudstrlcat( buf, skill_table[obj->value[2]]->name, MSL );
                        mudstrlcat( buf, "'", MSL );
                    }
                    if ( obj->value[3] >= 0 && obj->value[3] < top_sn ) {
                        mudstrlcat( buf, "'", MSL );
                        mudstrlcat( buf, skill_table[obj->value[3]]->name, MSL );
                        mudstrlcat( buf, "'", MSL );
                    }
                    act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
                    break;
                case ITEM_SALVE:
                    snprintf( buf, MSL, "Tiene %d(%d) aplicaciones de nivel %d", obj->value[1],
                              obj->value[2], obj->value[0] );
                    if ( obj->value[4] >= 0 && obj->value[4] < top_sn ) {
                        mudstrlcat( buf, " '", MSL );
                        mudstrlcat( buf, skill_table[obj->value[4]]->name, MSL );
                        mudstrlcat( buf, "'", MSL );
                    }
                    act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
                    break;

                case ITEM_WAND:
                case ITEM_STAFF:
                    snprintf( buf, MSL, "Tiene %d(%d) cargas de nivel %d", obj->value[1],
                              obj->value[2], obj->value[0] );
                    if ( obj->value[3] >= 0 && obj->value[3] < top_sn ) {
                        mudstrlcat( buf, " '", MSL );
                        mudstrlcat( buf, skill_table[obj->value[3]]->name, MSL );
                        mudstrlcat( buf, "'", MSL );
                    }
                    act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
                    break;
                case ITEM_MISSILE_WEAPON:
                case ITEM_WEAPON:
                    snprintf( buf, MSL, "El da�o es %d a %d (media %d)%s", obj->value[1],
                              obj->value[2], ( obj->value[1] + obj->value[2] ) / 2,
                              IS_OBJ_STAT( obj, ITEM_POISONED ) ? ", and is poisonous." : "." );
                    act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
                    snprintf( buf, MSL, "Habilidad necesaria: %s", weapon_skills[obj->value[4]] );
                    act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
                    snprintf( buf, MSL, "Tipo de da�o:  %s",
                              capitalize( attack_table[obj->value[3]] ) );
                    act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
                    break;
                case ITEM_ARMOR:
                    snprintf( buf, MSL, "Armadura es %d.", obj->value[0] );
                    act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
                    break;
            }
            bool                    afs = FALSE;

            for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next ) {
                showaffect( ch, paf );
                afs = TRUE;
            }
            for ( paf = obj->first_affect; paf; paf = paf->next ) {
                afs = TRUE;
                showaffect( ch, paf );
            }
            if ( afs ) {
                mudstrlcpy( buf, "'", MSL );
                act( AT_LBLUE, buf, ch, obj, keeper, TO_CHAR );
            }
            ch->reply = keeper;
            return;
        }

        found = FALSE;
        for ( obj = keeper->first_carrying; obj; obj = obj->next_content ) {

            if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
                 && ( cost = get_cost( ch, keeper, obj, TRUE ) ) || obj->pIndexData->vnum == 41002
                 || obj->pIndexData->vnum == 41004 ) {
                if ( !found ) {
                    found = TRUE;
                    send_to_pager
                        ( "&C[&WLv   Price  Currency    Name of Object                                 Size&C]\r\n",
                          ch );
                }
                if ( obj->pIndexData->vnum == 41002 || obj->pIndexData->vnum == 41004 ) {
                    cost = obj->cost;
                }

                if ( obj->level <= upper ) {
                    pager_printf( ch, "%-3s%-11d%-40s\r\n", divleft, upper, divright );
                    upper = -1;
                }

                if ( obj->level < lower ) {
                    pager_printf( ch, "%-3s%-11d%-40s\r\n", divleft, lower, divright );
                    lower = -1;
                }
                pager_printf( ch, "&C[%2d %7d  %-11s %-40s %10s]\r\n", obj->level, cost,
                              curr_types[obj->currtype], capitalize( obj->short_descr ),
                              obj_sizes[obj->size] );
            }
        }

        if ( lower >= 0 ) {
            pager_printf( ch, "%-3s%-7d%-40s\r\n", divleft, lower, divright );
        }

        if ( !found ) {
            if ( arg[0] == '\0' )
                send_to_char( "No puedes comprar nada aqu�\r\n", ch );
            else
                send_to_char( "No puedes comprar eso aqu�.\r\n", ch );
            return;
        }

        send_to_char( "\r\n&CType &WLIST STAT <&Cobject&W>&C - To find out more details.\r\n", ch );
        return;
    }
}

void do_sell( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL];
    CHAR_DATA              *keeper;
    OBJ_DATA               *obj;
    int                     cost = 0,
        sfor = 0;
    short                   primcurr;

    argument = one_argument( argument, arg );

    if ( !arg || arg[0] == '\0' ) {
        send_to_char( "Vender que?\r\n", ch );
        return;
    }

    if ( argument && argument[0] != '\0' && is_number( argument ) ) {
        sfor = atoi( argument );
        if ( sfor < 0 )
            sfor = 0;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL ) {
        act( AT_TELL, "$n te cuenta 'No tienes ese objeto.'", keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if ( !can_see_obj( keeper, obj ) ) {
        send_to_char( "Qu� intentas venderme? No compro aire!\r\n", ch );
        return;
    }

    if ( !can_drop_obj( ch, obj ) ) {
        send_to_char( "No puedes dejar ese objeto.\r\n", ch );
        return;
    }

    if ( obj->timer > 0 ) {
        act( AT_TELL, "$n te cuenta, '$p se deprecia en valor demasiado r�pido...'", keeper, obj, ch,
             TO_VICT );
        return;
    }

    /*
     * Shopkeeper won't buy it or it is worthless 
     */
    if ( !will_buy( keeper, obj ) || obj->pIndexData->cost <= 0 ) {
        /*
         * We will allow one marked as crafthshop to buy object 41002 and 41004 
         */
        if ( !xIS_SET( keeper->act, ACT_CRAFTSHOP )
             || ( obj->pIndexData->vnum != 41002 && obj->pIndexData->vnum != 41004 ) ) {
            act( AT_ACTION, "$n no parece interesado en $p.", keeper, obj, ch, TO_VICT );
            return;
        }
    }

    primcurr = get_primary_curr( keeper->in_room );

    cost = ( get_cost( ch, keeper, obj, FALSE ) );
    if ( sfor != 0 && sfor < cost )
        cost = sfor;

    if ( GET_MONEY( keeper, CURR_GOLD ) < 500 || GET_MONEY( keeper, CURR_SILVER ) < 5000
         || GET_MONEY( keeper, CURR_BRONZE ) < 50000 || GET_MONEY( keeper, CURR_COPPER ) < 50000 ) {
        GET_MONEY( keeper, CURR_GOLD ) = 500;
        GET_MONEY( keeper, CURR_SILVER ) = 5000;
        GET_MONEY( keeper, CURR_BRONZE ) = 50000;
        GET_MONEY( keeper, CURR_COPPER ) = 50000;
    }

    if ( GET_VALUE( obj, type ) == CURR_GOLD ) {
        if ( cost > 5000 ) {
            act( AT_CYAN, "los ojos de $n miran $p.", keeper, obj, ch, TO_VICT );
            act_printf( AT_TELL, keeper, obj, ch, TO_VICT,
                        "$n te cuenta 'No llevo %d de oro encima'", cost );
            ch->reply = keeper;
            return;
        }

        if ( GET_MONEY( keeper, CURR_GOLD ) < cost ) {
            act( AT_TELL, "$n te cuenta 'No llevo suficiente oro para comprar $p.'", keeper, obj, ch,
                 TO_VICT );
            act_printf( AT_TELL, keeper, obj, ch, TO_VICT,
                        "$n te cuenta 'Solo tengo %d de oro int�ntalo m�s tarde'",
                        GET_MONEY( keeper, CURR_GOLD ) );
            ch->reply = keeper;
            return;
        }
    }

    if ( GET_VALUE( obj, type ) == CURR_SILVER ) {
        if ( GET_MONEY( keeper, CURR_SILVER ) < cost ) {
            act( AT_TELL, "$n te cuenta 'No tengo suficiente plata para comprar $p.'", keeper, obj, ch,
                 TO_VICT );
            act_printf( AT_TELL, keeper, obj, ch, TO_VICT,
                        "$n te cuenta 'Lo m�ximo que te puedo dar por esto es %d de plata'",
                        GET_MONEY( keeper, CURR_SILVER ) );
            ch->reply = keeper;
            return;
        }
    }

    if ( GET_VALUE( obj, type ) == CURR_BRONZE ) {
        if ( GET_MONEY( keeper, CURR_BRONZE ) < cost ) {
            act( AT_TELL, "$n te cuenta 'No tengo suficiente bronce para comprar $p.'", keeper, obj, ch,
                 TO_VICT );
            act_printf( AT_TELL, keeper, obj, ch, TO_VICT,
                        "$n te cuenta 'Lo m�ximo que te puedo dar por eso es %d de bronce.'",
                        GET_MONEY( keeper, CURR_BRONZE ) );
            ch->reply = keeper;
            return;
        }
    }

    if ( GET_VALUE( obj, type ) == CURR_COPPER ) {
        if ( GET_MONEY( keeper, CURR_COPPER ) < cost ) {
            act( AT_TELL, "$n te cuenta 'No tengo suficiente cobre para comprar $p.'", keeper, obj, ch,
                 TO_VICT );
            act_printf( AT_TELL, keeper, obj, ch, TO_VICT,
                        "$n te cuenta 'Lo m�ximo que te puedo dar por eso es %d de cobre.'",
                        GET_MONEY( keeper, CURR_COPPER ) );
            ch->reply = keeper;
            return;
        }
    }

    if ( cost == 0 ) {
        cost = 1;
    }

    if ( GET_VALUE( obj, type ) == CURR_SILVER ) {
        act( AT_ACTION, "$n vende $p.", ch, obj, NULL, TO_ROOM );
        snprintf( arg, MIL, "Vendes $p por %d %s moneda%s.", cost, curr_types[CURR_SILVER],
                  cost > 1 ? "s" : "" );
        act( AT_ACTION, arg, ch, obj, NULL, TO_CHAR );
        GET_MONEY( ch, CURR_SILVER ) += cost;
    }

    if ( GET_VALUE( obj, type ) == CURR_GOLD ) {
        act( AT_ACTION, "$n vende $p.", ch, obj, NULL, TO_ROOM );
        snprintf( arg, MIL, "vendes $p por %d %s moneda%s.", cost, curr_types[CURR_GOLD],
                  cost > 1 ? "s" : "" );
        act( AT_ACTION, arg, ch, obj, NULL, TO_CHAR );
        GET_MONEY( ch, CURR_GOLD ) += cost;
    }

    if ( GET_VALUE( obj, type ) == CURR_BRONZE ) {
        act( AT_ACTION, "$n vende $p.", ch, obj, NULL, TO_ROOM );
        snprintf( arg, MIL, "Vendes $p por %d %s moneda%s.", cost, curr_types[CURR_BRONZE],
                  cost > 1 ? "s" : "" );
        act( AT_ACTION, arg, ch, obj, NULL, TO_CHAR );
        GET_MONEY( ch, CURR_BRONZE ) += cost;
    }

    if ( GET_VALUE( obj, type ) == CURR_COPPER ) {
        act( AT_ACTION, "$n vende $p.", ch, obj, NULL, TO_ROOM );
        snprintf( arg, MIL, "Vendes $p por %d %s moneda%s.", cost, curr_types[CURR_COPPER],
                  cost > 1 ? "s" : "" );
        act( AT_ACTION, arg, ch, obj, NULL, TO_CHAR );
        GET_MONEY( ch, CURR_COPPER ) += cost;
    }

    if ( obj->item_type == ITEM_TRASH )
        extract_obj( obj );
    else {
        obj_from_char( obj );
        obj_to_char( obj, keeper );
    }
}

void do_value( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    CHAR_DATA              *keeper;
    OBJ_DATA               *obj;
    int                     cost;
    short                   primcurr;

    if ( argument[0] == '\0' ) {
        send_to_char( "Valorar qu�?\r\n", ch );
        return;
    }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
        return;

    if ( ( obj = get_obj_carry( ch, argument ) ) == NULL ) {
        act( AT_TELL, "$n te cuenta 'No tienes ese objeto.'", keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if ( !can_drop_obj( ch, obj ) ) {
        send_to_char( "No puedes dejar ese objeto!\r\n", ch );
        return;
    }

    if ( !will_buy( keeper, obj ) || ( cost = get_cost( ch, keeper, obj, FALSE ) ) <= 0 ) {
        act( AT_ACTION, "$n no parece interesado en $p.", keeper, obj, ch, TO_VICT );
        return;
    }

    primcurr = get_primary_curr( keeper->in_room );
    if ( GET_VALUE( obj, type ) == CURR_GOLD ) {
        snprintf( buf, MSL, "$n te cuenta 'Te dar� %d %s monedas por $p.'", cost,
                  curr_types[primcurr] );
        act( AT_TELL, buf, keeper, obj, ch, TO_VICT );
        ch->reply = keeper;
    }
    else if ( GET_VALUE( obj, type ) == CURR_SILVER ) {
        snprintf( buf, MSL, "$n te cuenta 'Te dar� %d monedas de plata por $p.'", cost );
        act( AT_TELL, buf, keeper, obj, ch, TO_VICT );
        ch->reply = keeper;
    }
    else if ( GET_VALUE( obj, type ) == CURR_BRONZE ) {
        snprintf( buf, MSL, "$n te cuenta 'Te dar� %d monedas de bronce por $p.'", cost );
        act( AT_TELL, buf, keeper, obj, ch, TO_VICT );
        ch->reply = keeper;
    }
    else if ( GET_VALUE( obj, type ) == CURR_COPPER ) {
        snprintf( buf, MSL, "$n te cuenta 'Te dar� %d monedas de cobre por $p.'", cost );
        act( AT_TELL, buf, keeper, obj, ch, TO_VICT );
        ch->reply = keeper;
    }
    else {
        act( AT_ACTION, "$n no parece interesado en $p.", keeper, obj, ch, TO_VICT );
        return;
    }
}

/*
 * Repair a single object. Used when handling "repair all" - Gorog
 */
void repair_one_obj( CHAR_DATA *ch, CHAR_DATA *keeper, OBJ_DATA *obj, char *arg, int maxmoney,
                     char *fixstr, char *fixstr2 )
{
    char                    buf[MSL];
    int                     cost;
    short                   primcurr;

    primcurr = get_primary_curr( keeper->in_room );

    if ( !can_drop_obj( ch, obj ) )
        ch_printf( ch, "No puedes dejar %s.\r\n", obj->name );
    else if ( ( cost = get_repaircost( ch, keeper, obj ) ) < 0 ) {
        if ( cost != -2 )
            act( AT_TELL, "$n te cuenta, 'Lo siento, no puedo hacer nada con $p.'", keeper, obj, ch,
                 TO_VICT );
        else
            act( AT_TELL, "$n te cuenta, '$p parece estar en perfecto estado!'", keeper, obj, ch, TO_VICT );
    }
    else if ( ( cost = str_cmp( "all", arg ) ? cost : 11 * cost / 10 )
              && !can_afford( ch, cost, CURR_BRONZE ) ) {
        snprintf( buf, MSL, "$N te cuenta, 'Te costar� %d moneda%s de %s a %s %s...'", cost,
                  cost == 1 ? "" : "s", curr_types[CURR_BRONZE], fixstr, obj->name );
        act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
        act( AT_TELL, "$N te cuenta, 'Que veo no puedes pagar.'", ch, NULL, keeper, TO_CHAR );
    }
    else {
        snprintf( buf, MSL, "$n le da $p a $N, que r�pidamente lo %s.", fixstr2 );
        act( AT_ACTION, buf, ch, obj, keeper, TO_ROOM );
        snprintf( buf, MSL, "$N carga %d %s moneda%s a %s $p.", cost, curr_types[CURR_BRONZE],
                  cost == 1 ? "" : "s", fixstr );
        act( AT_ACTION, buf, ch, obj, keeper, TO_CHAR );
        spend_coins( ch, keeper, cost, CURR_BRONZE );
        GET_MONEY( keeper, CURR_BRONZE ) += cost;
        if ( GET_MONEY( keeper, CURR_BRONZE ) < 0 )
            GET_MONEY( keeper, CURR_BRONZE ) = 0;
        else if ( GET_MONEY( keeper, CURR_BRONZE ) >= maxmoney ) {
            boost_economy( keeper->in_room->area, ( maxmoney / 2 ), CURR_BRONZE );
            GET_MONEY( keeper, CURR_BRONZE ) -= ( maxmoney / 2 );
            act( AT_ACTION, "$n pone $T a buen recaudo.", keeper, NULL,
                 curr_types[CURR_BRONZE], TO_ROOM );
        }

        switch ( obj->item_type ) {
            default:
                send_to_char( "Por alg�n motivo, crees que has sido estafado...\r\n", ch );
                break;
            case ITEM_ARMOR:
                obj->value[0] = obj->value[1];
                break;
            case ITEM_WEAPON:
                obj->value[0] = INIT_WEAPON_CONDITION;
                break;
            case ITEM_WAND:
            case ITEM_STAFF:
                obj->value[2] = obj->value[1];
                break;
        }
        oprog_repair_trigger( ch, obj );
    }
}

void do_repair( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *keeper;
    OBJ_DATA               *obj;
    const char             *fixstr;
    const char             *fixstr2;
    int                     maxmoney = 0;

    if ( argument[0] == '\0' ) {
        send_to_char( "Qu� quieres reparar?\r\n", ch );
        return;
    }

    if ( !( keeper = find_fixer( ch ) ) )
        return;

    maxmoney = keeper->level * 100000;
    switch ( keeper->pIndexData->rShop->shop_type ) {
        default:
        case SHOP_FIX:
            fixstr = "repair";
            fixstr2 = "repairs";
            break;
        case SHOP_RECHARGE:
            fixstr = "recharge";
            fixstr2 = "recharges";
            break;
    }

    if ( !str_cmp( argument, "all" ) ) {
        for ( obj = ch->first_carrying; obj; obj = obj->next_content ) {
            if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
                 && can_see_obj( keeper, obj ) && ( obj->item_type == ITEM_ARMOR
                                                    || obj->item_type == ITEM_WEAPON
                                                    || obj->item_type == ITEM_WAND
                                                    || obj->item_type == ITEM_STAFF ) )
                repair_one_obj( ch, keeper, obj, argument, maxmoney, ( char * ) fixstr,
                                ( char * ) fixstr2 );
        }
        return;
    }

    if ( !( obj = get_obj_carry( ch, argument ) ) ) {
        act( AT_TELL, "$n te cuenta 'No tienes ese objeto.'", keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    repair_one_obj( ch, keeper, obj, argument, maxmoney, ( char * ) fixstr, ( char * ) fixstr2 );
}

void appraise_all( CHAR_DATA *ch, CHAR_DATA *keeper, char *fixstr )
{
    OBJ_DATA               *obj;
    char                    buf[MSL];
    int                     cost = 0,
        total = 0;
    short                   primcurr;

    primcurr = get_primary_curr( keeper->in_room );

    for ( obj = ch->first_carrying; obj != NULL; obj = obj->next_content ) {
        if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj )
             && ( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON
                  || obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF ) ) {
            if ( !can_drop_obj( ch, obj ) )
                ch_printf( ch, "No puedes dejar %s.\r\n", obj->name );
            else if ( ( cost = get_repaircost( ch, keeper, obj ) ) < 0 ) {
                if ( cost != -2 )
                    act( AT_TELL, "$n te cuenta, 'Lo siento, no puedo hacer nada con $p.'", keeper,
                         obj, ch, TO_VICT );
                else
                    act( AT_TELL, "$n te cuenta '$p parece estar en perfecto estado!'", keeper, obj, ch,
                         TO_VICT );
            }
            else {
                snprintf( buf, MSL, "$N te cuenta, 'Te costar� %d pieza%s de %s a %s %s'", cost,
                          cost == 1 ? "" : "s", curr_types[primcurr], fixstr, obj->name );
                act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
                total += cost;
            }
        }
    }
    if ( total > 0 ) {
        send_to_char( "\r\n", ch );
        snprintf( buf, MSL, "$N te cuenta, 'Te costar� %d pieza%s de %s en total.'", total,
                  cost == 1 ? "" : "s", curr_types[primcurr] );
        act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
        act( AT_TELL, "$N te cuenta, 'Recuerda que hay un 10% de recargo por reparar todo.'", ch, NULL,
             keeper, TO_CHAR );
    }
}

void do_appraise( CHAR_DATA *ch, char *argument )
{
    char                    buf[MSL];
    char                    arg[MIL];
    CHAR_DATA              *keeper;
    OBJ_DATA               *obj;
    int                     cost;
    const char             *fixstr;

    one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Qu� quieres evaluar?\r\n", ch );
        return;
    }

    if ( ( keeper = find_fixer( ch ) ) == NULL )
        return;

    switch ( keeper->pIndexData->rShop->shop_type ) {
        default:
        case SHOP_FIX:
            fixstr = "repair";
            break;
        case SHOP_RECHARGE:
            fixstr = "recharge";
            break;
    }

    if ( !str_cmp( arg, "all" ) ) {
        appraise_all( ch, keeper, ( char * ) fixstr );
        return;
    }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL ) {
        act( AT_TELL, "$n te cuenta 'No tienes ese objeto.'", keeper, NULL, ch, TO_VICT );
        ch->reply = keeper;
        return;
    }

    if ( !can_drop_obj( ch, obj ) ) {
        send_to_char( "No puedes dejar ese objeto.\r\n", ch );
        return;
    }

    if ( ( cost = get_repaircost( ch, keeper, obj ) ) < 0 ) {
        if ( cost != -2 )
            act( AT_TELL, "$n tells you, 'Lo siento, no puedo hacer nada con $p.'", keeper, obj, ch,
                 TO_VICT );
        else
            act( AT_TELL, "$n te cuenta, '$p parece estar en perfecto estado!'", keeper, obj, ch, TO_VICT );
        return;
    }

    snprintf( buf, MSL, "$N te cuenta, 'Te costar� %d pieza%s de bronce a %s eso...'", cost,
              cost == 1 ? "" : "s", fixstr );
    act( AT_TELL, buf, ch, NULL, keeper, TO_CHAR );
    if ( cost > GET_MONEY( ch, CURR_BRONZE ) )
        act( AT_TELL, "$N te cuenta 'Que veo no puedes pagar.'", ch, NULL, keeper, TO_CHAR );

    return;
}

/* ------------------ Shop Building and Editing Section ----------------- */

void do_makeshop( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA              *shop;
    int                     vnum;
    MOB_INDEX_DATA         *mob;

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Uso:  makeshop <mobvnum>\r\n", ch );
        return;
    }

    vnum = atoi( argument );

    if ( ( mob = get_mob_index( vnum ) ) == NULL ) {
        send_to_char( "Mobile no encontrado.\r\n", ch );
        return;
    }

    if ( !can_medit( ch, mob ) )
        return;

    if ( mob->pShop ) {
        send_to_char( "Este mob ya tiene una tienda.\r\n", ch );
        return;
    }

    CREATE( shop, SHOP_DATA, 1 );

    LINK( shop, first_shop, last_shop, next, prev );
    shop->keeper = vnum;
    shop->profit_buy = 120;
    shop->profit_sell = 90;
    shop->open_hour = 0;
    shop->close_hour = 23;
    mob->pShop = shop;
    send_to_char( "Hecho.\r\n", ch );
    return;
}

void do_shopset( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA              *shop;
    MOB_INDEX_DATA         *mob,
                           *mob2;
    char                    arg1[MIL];
    char                    arg2[MIL];
    int                     vnum,
                            value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Uso: shopset <mob vnum> <campo> valor\r\n", ch );
        send_to_char( "\r\nDonde campo puede ser:\r\n", ch );
        send_to_char( "  buy0 buy1 buy2 buy3 buy4 buy sell open close keeper\r\n", ch );
        return;
    }

    vnum = atoi( arg1 );

    if ( ( mob = get_mob_index( vnum ) ) == NULL ) {
        send_to_char( "Mobile no encontrado.\r\n", ch );
        return;
    }

    if ( !can_medit( ch, mob ) )
        return;

    if ( !mob->pShop ) {
        send_to_char( "Este mob no guarda una tienda.\r\n", ch );
        return;
    }
    shop = mob->pShop;
    value = atoi( argument );

    if ( !str_cmp( arg2, "buy0" ) ) {
        if ( !is_number( argument ) )
            value = get_otype( argument );
        if ( value < 0 || value > MAX_ITEM_TYPE ) {
            send_to_char( "Tipo de objeto no v�lido!\r\n", ch );
            return;
        }
        shop->buy_type[0] = value;
        send_to_char( "Hecho\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy1" ) ) {
        if ( !is_number( argument ) )
            value = get_otype( argument );
        if ( value < 0 || value > MAX_ITEM_TYPE ) {
            send_to_char( "Tipo de objeto no v�lido!\r\n", ch );
            return;
        }
        shop->buy_type[1] = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy2" ) ) {
        if ( !is_number( argument ) )
            value = get_otype( argument );
        if ( value < 0 || value > MAX_ITEM_TYPE ) {
            send_to_char( "Tipo de objeto no v�lido!\r\n", ch );
            return;
        }
        shop->buy_type[2] = value;
        send_to_char( "Hecho\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy3" ) ) {
        if ( !is_number( argument ) )
            value = get_otype( argument );
        if ( value < 0 || value > MAX_ITEM_TYPE ) {
            send_to_char( "Tipo de objeto no v�lido!\r\n", ch );
            return;
        }
        shop->buy_type[3] = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy4" ) ) {
        if ( !is_number( argument ) )
            value = get_otype( argument );
        if ( value < 0 || value > MAX_ITEM_TYPE ) {
            send_to_char( "Tipo de objeto no v�lido!\r\n", ch );
            return;
        }
        shop->buy_type[4] = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "buy" ) ) {
        if ( value <= ( shop->profit_sell + 5 ) || value > 1000 ) {
            send_to_char( "Fuera de rango!\r\n", ch );
            return;
        }
        shop->profit_buy = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "sell" ) ) {
        if ( value < 0 || value >= ( shop->profit_buy - 5 ) ) {
            send_to_char( "Fuera de rango.\r\n", ch );
            return;
        }
        shop->profit_sell = value;
        send_to_char( "Hecho,\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "open" ) ) {
        if ( value < 0 || value > 23 ) {
            send_to_char( "Fuera de rango.\r\n", ch );
            return;
        }
        shop->open_hour = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "close" ) ) {
        if ( value < 0 || value > 23 ) {
            send_to_char( "Fuera de rango.\r\n", ch );
            return;
        }
        shop->close_hour = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "keeper" ) ) {
        if ( ( mob2 = get_mob_index( vnum ) ) == NULL ) {
            send_to_char( "Mobile no encontrado.\r\n", ch );
            return;
        }
        if ( !can_medit( ch, mob ) )
            return;
        if ( mob2->pShop ) {
            send_to_char( "Ese mob ya tiene una tienda.\r\n", ch );
            return;
        }
        mob->pShop = NULL;
        mob2->pShop = shop;
        shop->keeper = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    do_shopset( ch, ( char * ) "" );
    return;
}

void do_shopstat( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA              *shop;
    MOB_INDEX_DATA         *mob;
    int                     vnum;

    if ( argument[0] == '\0' ) {
        send_to_char( "Uso: shopstat <keeper vnum>\r\n", ch );
        return;
    }

    vnum = atoi( argument );

    if ( ( mob = get_mob_index( vnum ) ) == NULL ) {
        send_to_char( "Mobile no encontrado.\r\n", ch );
        return;
    }

    if ( !mob->pShop ) {
        send_to_char( "Ese mob no guarda una tienda.\r\n", ch );
        return;
    }
    shop = mob->pShop;

    ch_printf( ch, "Keeper: %d  %s\r\n", shop->keeper, mob->short_descr );
    ch_printf( ch, "buy0 [%s]  buy1 [%s]  buy2 [%s]  buy3 [%s]  buy4 [%s]\r\n",
               o_types[shop->buy_type[0]], o_types[shop->buy_type[1]], o_types[shop->buy_type[2]],
               o_types[shop->buy_type[3]], o_types[shop->buy_type[4]] );
    ch_printf( ch, "Profit:  buy %3d%%  sell %3d%%\r\n", shop->profit_buy, shop->profit_sell );
    ch_printf( ch, "Horas:   abrir %2d  cerrar %2d\r\n", shop->open_hour, shop->close_hour );
    return;
}

void do_shops( CHAR_DATA *ch, char *argument )
{
    SHOP_DATA              *shop;

    if ( !first_shop ) {
        send_to_char( "No hay tiendas.\r\n", ch );
        return;
    }

    set_pager_color( AT_NOTE, ch );
    for ( shop = first_shop; shop; shop = shop->next )
        pager_printf( ch,
                      "Keeper: %5d Buy: %3d Sell: %3d Open: %2d Close: %2d Buy: %2d %2d %2d %2d %2d\r\n",
                      shop->keeper, shop->profit_buy, shop->profit_sell, shop->open_hour,
                      shop->close_hour, shop->buy_type[0], shop->buy_type[1], shop->buy_type[2],
                      shop->buy_type[3], shop->buy_type[4] );
    return;
}

/* -------------- Repair Shop Building and Editing Section -------------- */

void do_makerepair( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA            *repair;
    int                     vnum;
    MOB_INDEX_DATA         *mob;

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Uso: makerepair <mobvnum>\r\n", ch );
        return;
    }

    vnum = atoi( argument );

    if ( ( mob = get_mob_index( vnum ) ) == NULL ) {
        send_to_char( "Mobile no encontrado.\r\n", ch );
        return;
    }

    if ( !can_medit( ch, mob ) )
        return;

    if ( mob->rShop ) {
        send_to_char( "Este mob ya tiene una tienda de reparaci�n.\r\n", ch );
        return;
    }

    CREATE( repair, REPAIR_DATA, 1 );

    LINK( repair, first_repair, last_repair, next, prev );
    repair->keeper = vnum;
    repair->profit_fix = 100;
    repair->shop_type = SHOP_FIX;
    repair->open_hour = 0;
    repair->close_hour = 23;
    mob->rShop = repair;
    send_to_char( "Hecho.\r\n", ch );
    return;
}

void do_repairset( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA            *repair;
    MOB_INDEX_DATA         *mob,
                           *mob2;
    char                    arg1[MIL];
    char                    arg2[MIL];
    int                     vnum,
                            value;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( arg1[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Uso: repairset <mob vnum> <campo> valor\r\n", ch );
        send_to_char( "\r\nDonde campo puede ser:\r\n", ch );
        send_to_char( "  fix0 fix1 fix2 profit type open close keeper\r\n", ch );
        return;
    }

    vnum = atoi( arg1 );

    if ( ( mob = get_mob_index( vnum ) ) == NULL ) {
        send_to_char( "Mobile no encontrado.\r\n", ch );
        return;
    }

    if ( !can_medit( ch, mob ) )
        return;

    if ( !mob->rShop ) {
        send_to_char( "Este mob no guarda una tienda de reparaci�n.\r\n", ch );
        return;
    }
    repair = mob->rShop;
    value = atoi( argument );

    if ( !str_cmp( arg2, "fix0" ) ) {
        if ( !is_number( argument ) )
            value = get_otype( argument );
        if ( value < 0 || value > MAX_ITEM_TYPE ) {
            send_to_char( "Tipo de objeto no v�lido!\r\n", ch );
            return;
        }
        repair->fix_type[0] = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "fix1" ) ) {
        if ( !is_number( argument ) )
            value = get_otype( argument );
        if ( value < 0 || value > MAX_ITEM_TYPE ) {
            send_to_char( "Tipo de objeto no v�lido.\r\n", ch );
            return;
        }
        repair->fix_type[1] = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "fix2" ) ) {
        if ( !is_number( argument ) )
            value = get_otype( argument );
        if ( value < 0 || value > MAX_ITEM_TYPE ) {
            send_to_char( "Tipo de objeto no va�lido.\r\n", ch );
            return;
        }
        repair->fix_type[2] = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "profit" ) ) {
        if ( value < 1 || value > 1000 ) {
            send_to_char( "Fuera de rango.\r\n", ch );
            return;
        }
        repair->profit_fix = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "type" ) ) {
        if ( value < 1 || value > 2 ) {
            send_to_char( "Fuera de rango.\r\n", ch );
            return;
        }
        repair->shop_type = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "open" ) ) {
        if ( value < 0 || value > 23 ) {
            send_to_char( "Fuera de rango.\r\n", ch );
            return;
        }
        repair->open_hour = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "close" ) ) {
        if ( value < 0 || value > 23 ) {
            send_to_char( "Fuera de rango.\r\n", ch );
            return;
        }
        repair->close_hour = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "keeper" ) ) {
        if ( ( mob2 = get_mob_index( vnum ) ) == NULL ) {
            send_to_char( "Mobile noo encontrado.\r\n", ch );
            return;
        }
        if ( !can_medit( ch, mob ) )
            return;
        if ( mob2->rShop ) {
            send_to_char( "Ese mob ya tiene una tienda de reparaci�n.\r\n", ch );
            return;
        }
        mob->rShop = NULL;
        mob2->rShop = repair;
        repair->keeper = value;
        send_to_char( "Hecho.\r\n", ch );
        return;
    }

    do_repairset( ch, ( char * ) "" );
    return;
}

void do_repairstat( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA            *repair;
    MOB_INDEX_DATA         *mob;
    int                     vnum;

    if ( argument[0] == '\0' ) {
        send_to_char( "Uso:  repairstat <keeper vnum>\r\n", ch );
        return;
    }

    vnum = atoi( argument );

    if ( ( mob = get_mob_index( vnum ) ) == NULL ) {
        send_to_char( "Mobile no encontrado.\r\n", ch );
        return;
    }

    if ( !mob->rShop ) {
        send_to_char( "Este mob no guarda una tienda de reparaci�n.\r\n", ch );
        return;
    }
    repair = mob->rShop;

    ch_printf( ch, "Keeper: %d  %s\r\n", repair->keeper, mob->short_descr );
    ch_printf( ch, "fix0 [%s]  fix1 [%s]  fix2 [%s]\r\n", o_types[repair->fix_type[0]],
               o_types[repair->fix_type[1]], o_types[repair->fix_type[2]] );
    ch_printf( ch, "Profit: %3d%%  Type: %d\r\n", repair->profit_fix, repair->shop_type );
    ch_printf( ch, "Horas:   abrir %2d  cerrar %2d\r\n", repair->open_hour, repair->close_hour );
    return;
}

void do_repairshops( CHAR_DATA *ch, char *argument )
{
    REPAIR_DATA            *repair;

    if ( !first_repair ) {
        send_to_char( "No hay tiendas de reparaci�n.\r\n", ch );
        return;
    }

    set_char_color( AT_NOTE, ch );
    for ( repair = first_repair; repair; repair = repair->next )
        ch_printf( ch,
                   "Keeper: %5d Profit: %3d Type: %d Open: %2d Close: %2d Fix: %2d %2d %2d\r\n",
                   repair->keeper, repair->profit_fix, repair->shop_type, repair->open_hour,
                   repair->close_hour, repair->fix_type[0], repair->fix_type[1],
                   repair->fix_type[2] );
    return;
}
