/* Copyright 2014, 2015 Jesús Pavón Abián
Programa bajo licencia GPL. Encontrará una copia de la misma en el archivo COPYING.txt que se encuentra en la raíz de los fuentes. */
/***************************************************************************  
 *                          SMAUG Banking Support Code                     *
 ***************************************************************************
 *                                                                         *
 * This code may be used freely, as long as credit is given in the help    *
 * file. Thanks.                                                           *
 *                                                                         *
 *                                        -= Minas Ravenblood =-           *
 *                                 Implementor of The Apocalypse Theatre   *
 *                                      (email: krisco7@hotmail.com)       *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 * Taltos's Bank code includes:                                            *
 * Bank account each with a password and a possibility of more than one    *
 * per character and also will allow sharing of money between players      *
 * if wanted.                                                              *
 * Also 2 commands for immortals to view  and edit accounts' details       *
 *                                                                         *
 *                                                 By: Taltos              *
 *                                               AIM: jmoder51             *
 *                                    --|-- email: brainbuster51@yahoo.com *
 *                            O        |||--------------------\            *
 *                          -OO=========||___________________ /            *
 *                            O        |||--------------------\            *
 *                                    --|--                                *
 ***************************************************************************/

#include "h/mud.h"
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "h/files.h"
#include "h/currency.h"

void                    free_bank_to_chars( BANK_DATA * bank );

CHAR_DATA              *find_banker( CHAR_DATA *ch )
{
    CHAR_DATA              *banker = NULL;

    for ( banker = ch->in_room->first_person; banker; banker = banker->next_in_room )
        if ( IS_NPC( banker ) && xIS_SET( banker->act, ACT_BANKER ) )
            break;

    return banker;
}

void unlink_bank( BANK_DATA * bank )
{
    BANK_DATA              *tmp,
                           *tmp_next;
    int                     hash;

    if ( !bank ) {
        bug( "Unlink_bank: NULL bank" );
        return;
    }

    if ( bank->name[0] < 'a' || bank->name[0] > 'z' )
        hash = 0;
    else
        hash = ( bank->name[0] - 'a' ) + 1;

    if ( bank == ( tmp = bank_index[hash] ) ) {
        bank_index[hash] = tmp->next;
        return;
    }
    for ( ; tmp; tmp = tmp_next ) {
        tmp_next = tmp->next;
        if ( bank == tmp_next ) {
            tmp->next = tmp_next->next;
            return;
        }
    }
}

void free_bank( BANK_DATA * bank )
{
    if ( bank->name )
        STRFREE( bank->name );
    if ( bank->password )
        STRFREE( bank->password );
    DISPOSE( bank );
}

void free_banks( void )
{
    BANK_DATA              *bank,
                           *bank_next;
    int                     hash;

    for ( hash = 0; hash < 126; hash++ ) {
        for ( bank = bank_index[hash]; bank; bank = bank_next ) {
            bank_next = bank->next;
            unlink_bank( bank );
            free_bank( bank );
        }
    }
}

void add_bank( BANK_DATA * bank )
{
    int                     hash,
                            x;
    BANK_DATA              *tmp,
                           *prev;

    if ( !bank ) {
        bug( "Add_bank: NULL bank" );
        return;
    }

    if ( !bank->name ) {
        bug( "Add_bank: NULL bank->name" );
        return;
    }

    if ( !bank->password ) {
        bug( "Add_bank: NULL bank->password" );
        return;
    }

    /*
     * make sure the name is all lowercase 
     */
    for ( x = 0; bank->name[x] != '\0'; x++ )
        bank->name[x] = LOWER( bank->name[x] );
    if ( bank->name[0] < 'a' || bank->name[0] > 'z' )
        hash = 0;
    else
        hash = ( bank->name[0] - 'a' ) + 1;

    if ( ( prev = tmp = bank_index[hash] ) == NULL ) {
        bank->next = bank_index[hash];
        bank_index[hash] = bank;
        return;
    }

    for ( ; tmp; tmp = tmp->next ) {
        if ( ( x = strcmp( bank->name, tmp->name ) ) == 0 ) {
            bug( "Add_bank: trying to add duplicate name to bucket %d", hash );
            free_bank( bank );
            return;
        }
        else if ( x < 0 ) {
            if ( tmp == bank_index[hash] ) {
                bank->next = bank_index[hash];
                bank_index[hash] = bank;
                return;
            }
            prev->next = bank;
            bank->next = tmp;
            return;
        }
        prev = tmp;
    }

    /*
     * add to end 
     */
    prev->next = bank;
    bank->next = NULL;
    return;
}

void do_bank( CHAR_DATA *ch, char *argument )
{
    BANK_DATA              *victim_bank;
    BANK_DATA              *bank;
    CHAR_DATA              *banker;
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    char                    arg3[MAX_INPUT_LENGTH];
    char                    arg4[MAX_INPUT_LENGTH];
    char                    arg5[MAX_INPUT_LENGTH];
    char                    buf[MSL],
                           *pwdnew,
                           *p;
    int                     amount = 0,
        currtime = time( 0 );
    int                     type = DEFAULT_CURR;

    /*
     * arg1 == account name
     * arg2 == password (if none, close it)
     */

    if ( !( banker = find_banker( ch ) ) ) {
        send_to_char( "¡No estás en un banco!\r\n", ch );
        return;
    }

    if ( IS_NPC( ch ) ) {
        snprintf( buf, MSL, "decir Lo siento, %s, no negociamos con mobs.", ch->name );
        interpret( banker, buf );
        return;
    }

    if ( argument[0] == '\0' ) {
        // arg1 bank, arg2 password/type, arg3 create/delete/amount, arg4 currency, arg5
        // tbank
        send_to_char( "Uso: banco cuenta contraseña\r\n", ch );
        send_to_char( "Uso: banco cuenta contraseña crear/cancelar\r\n", ch );
        send_to_char( "Uso: banco cuenta saldo\r\n", ch );
        send_to_char( "uso: banco cuenta retirar/ingresar\r\n", ch );
        send_to_char( "Uso: banco cuenta transferir cantidad tipo destino\r\n", ch );
        interpret( banker, ( char * ) "decir Si necesita ayuda teclee ayuda banco." );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );
    argument = one_argument( argument, arg5 );

    bank = find_bank( arg1 );

    if ( is_number( arg3 ) ) {
        amount = atoi( arg3 );
    }

    if ( !str_cmp( arg3, "crear" ) && !is_number( arg3 ) ) {
        if ( strlen( arg1 ) < 4 ) {
            send_to_char( "El nombre de cuenta debe tener mínimo 4 caracteres.\r\n", ch );
            return;
        }

        if ( strlen( arg2 ) < 5 ) {
            send_to_char( "Contraseña inválida.  Mínimo 5 caracteres.\r\n", ch );
            return;
        }
        if ( arg2[0] == '!' ) {
            send_to_char( "La contraseña no puede comenzar por '!'.\r\n", ch );
            return;
        }

        if ( bank ) {
            send_to_char( "¡Ya existe!\r\n", ch );
            return;
        }

        if ( ( currtime - ch->pcdata->lastaccountcreated ) < 3600 ) {
            send_to_char
                ( "Espera al menos una hora para crear una cuenta nueva.\r\n",
                  ch );
            return;
        }

        pwdnew = sha256_crypt( arg2 );
        CREATE( bank, BANK_DATA, 1 );
        bank->lastused = current_time;
        bank->name = STRALLOC( arg1 );
        bank->password = STRALLOC( pwdnew );
        bank->bronze = 0;
        bank->copper = 0;
        bank->gold = 0;
        bank->silver = 0;
        add_bank( bank );
        save_bank(  );
        ch->pcdata->lastaccountcreated = currtime;
        save_char_obj( ch );
        saving_char = NULL;
        send_to_char( "tu cuenta ha sido creada.\r\n", ch );
        return;
    }
    if ( !str_cmp( arg3, "cancelar" ) && !is_number( arg3 ) ) {
        if ( !bank ) {
            send_to_char( "¡No existe!\r\n", ch );
            return;
        }
        if ( strcmp( sha256_crypt( arg2 ), bank->password ) ) {
            send_to_char( "Contraseña inválida.\r\n", ch );
            return;
        }

        GET_MONEY( ch, CURR_GOLD ) += bank->gold;
        GET_MONEY( ch, CURR_SILVER ) += bank->silver;
        GET_MONEY( ch, CURR_BRONZE ) += bank->bronze;
        GET_MONEY( ch, CURR_COPPER ) += bank->copper;
        ch_printf( ch, "Cancelando... (%s)\r\n", bank->name );
        free_bank_to_chars( bank );
        unlink_bank( bank );
        free_bank( bank );
        save_bank(  );
        ch->pcdata->lastaccountcreated = 0;
        save_char_obj( ch );
        saving_char = NULL;
        send_to_char( "Cuenta cancelada.\r\n", ch );
        return;
    }

    if ( !bank ) {
        send_to_char( "¡No existe!\r\n", ch );
        return;
    }
    if ( !str_cmp( arg2, "transferir" ) ) {
        if ( !is_number( arg3 ) ) {
            send_to_char( "Solo puedes transferir monedas.\r\n", ch );
            return;
        }
        else {
            amount = atoi( arg3 );
        }
        if ( arg1 && arg2 )
            type = get_currency_type( arg4 );

        if ( type == CURR_NONE ) {
            send_to_char( "No tienes.\r\n", ch );
            return;
        }
        if ( amount <= 0 ) {
            send_to_char( "No puedes.\r\n", ch );
            return;
        }
        victim_bank = find_bank( arg5 );

        if ( !victim_bank ) {
            sprintf( buf, "%s no tiene una cuenta con ese nombre aquí.", ch->name );
            do_tell( banker, buf );
            return;
        }

        if ( type == CURR_BRONZE ) {
            if ( amount > bank->bronze ) {
                ch_printf( ch, "No tienes tanto %s en el banco.\r\n", curr_types[type] );
                return;
            }

            bank->bronze -= amount;
            victim_bank->bronze += amount;
        }
        else if ( type == CURR_COPPER ) {
            if ( amount > bank->copper ) {
                ch_printf( ch, "No tienes tanto %s en el banco.\r\n", curr_types[type] );
                return;
            }
            bank->copper -= amount;
            victim_bank->copper += amount;
        }
        else if ( type == CURR_GOLD ) {
            if ( amount > bank->gold ) {
                ch_printf( ch, "No tienes tanto %s en el banco.\r\n", curr_types[type] );
                return;
            }
            bank->gold -= amount;
            victim_bank->gold += amount;
        }
        else if ( type == CURR_SILVER ) {
            if ( amount > bank->silver ) {
                ch_printf( ch, "No tienes tanta %s en el banco.\r\n", curr_types[type] );
                return;
            }
            bank->silver -= amount;
            victim_bank->silver += amount;
        }
        ch_printf( ch, "Transfieres %d monedas de %s desde tu cuenta.\r\n", amount,
                   curr_types[type] );
        bank->lastused = current_time;
        save_bank(  );
        return;
    }

    if ( !str_cmp( arg2, "saldo" ) ) {
        if ( ch->pcdata->bank == NULL ) {
            send_to_char( "No tienes una cuenta abierta.\r\n", ch );
        }

        ch_printf( ch, "&CLa cuenta %s tiene:\r\n",
                   bank->name );
        ch_printf( ch, "&Y%d oro.\r\n", bank->gold );
        ch_printf( ch, "&Y%d plata.\r\n", bank->silver );
        ch_printf( ch, "&Y%d bronce.\r\n", bank->bronze );
        ch_printf( ch, "&Y%d cobre.\r\n", bank->copper );
        return;
    }
    if ( !str_cmp( arg2, "retirar" ) ) {
        if ( ch->pcdata->bank == NULL ) {
            send_to_char( "No tienes cuenta en este bancoopen now.\r\n", ch );
            return;
        }
        if ( !is_number( arg3 ) ) {
            send_to_char( "You Solo puedes retirar monedas.\r\n", ch );
            return;
        }
        else {
            amount = atoi( arg3 );
        }
        if ( arg2 && arg3 )
            type = get_currency_type( arg4 );
        if ( type == CURR_NONE ) {
            send_to_char( "ese tipo de moneda no existe.\r\n", ch );
            return;
        }
        if ( amount <= 0 ) {
            send_to_char( "No puedes.\r\n", ch );
            return;
        }
        if ( type == CURR_BRONZE ) {
            if ( amount > bank->bronze ) {
                ch_printf( ch, "No tienes tanto %s en el banco.\r\n", curr_types[type] );
                return;
            }
            bank->bronze -= amount;
            ch_printf( ch, "tu cuenta tiene un saldo de %d monedas de bronce.\r\n", bank->bronze );
        }
        else if ( type == CURR_COPPER ) {
            if ( amount > bank->copper ) {
                ch_printf( ch, "No tienes tanto %s en el banco.\r\n", curr_types[type] );
                return;
            }
            bank->copper -= amount;
            ch_printf( ch, "Tu cuenta tiene un saldo de %d monedas de cobre.\r\n", bank->copper );
        }
        else if ( type == CURR_GOLD ) {
            if ( amount > bank->gold ) {
                ch_printf( ch, "No tienes tanto %s en el banco.\r\n", curr_types[type] );
                return;
            }
            bank->gold -= amount;
            ch_printf( ch, "Tu cuenta tiene un saldo de %d monedas de oro.\r\n", bank->gold );
        }
        else if ( type == CURR_SILVER ) {
            if ( amount > bank->silver ) {
                ch_printf( ch, "No tienes tanta %s en el banco.\r\n", curr_types[type] );
                return;
            }
            bank->silver -= amount;
            ch_printf( ch, "tu cuenta tiene un saldo de %d monedas de plata.\r\n", bank->silver );
        }
        else {
            send_to_char( "No existe el tipo.\r\n", ch );
            return;
        }
        GET_MONEY( ch, type ) += amount;
        ch_printf( ch, "Sacas %d monedas de %s del banco.\r\n", amount, curr_types[type] );
        bank->lastused = current_time;
        save_bank(  );
        save_char_obj( ch );
        return;
    }                                                  // end of withdraw

    // arg1 bank, arg2 password/type, arg3 create/delete/amount, arg4 currency, arg5
    // tbank
    // start of deposit
    if ( !str_cmp( arg2, "ingresar" ) ) {
        if ( ch->pcdata->bank == NULL ) {
            send_to_char( "No tienes cuenta en este banco.\r\n", ch );
            return;
        }
        if ( !is_number( arg3 ) ) {
            send_to_char( "solo puedes ingresar monedas.\r\n", ch );
            return;
        }
        else {
            amount = atoi( arg3 );
        }

        if ( arg2 && arg3 )
            type = get_currency_type( arg4 );
        if ( type == CURR_NONE ) {
            send_to_char( "No existe ese tipo de moneda.\r\n", ch );
            return;
        }

        if ( amount > GET_MONEY( ch, type ) ) {
            send_to_char( "No tienes tantas.\r\n", ch );
            return;
        }

        if ( type != CURR_BRONZE && type != CURR_COPPER && type != CURR_GOLD
             && type != CURR_SILVER ) {
            send_to_char( "No existe.\r\n", ch );
            return;
        }

        GET_MONEY( ch, type ) -= amount;
        ch_printf( ch, "Ingresas %d monedas de %s en el banco.\r\n", amount, curr_types[type] );

        if ( type == CURR_BRONZE ) {
            bank->bronze += amount;
            ch_printf( ch, "tu cuenta tiene un saldo de %d monedas de bronce.\r\n", bank->bronze );
        }
        else if ( type == CURR_COPPER ) {
            bank->copper += amount;
            ch_printf( ch, "tu cuenta tiene un saldo de %d monedas de plata.\r\n", bank->copper );
        }
        else if ( type == CURR_GOLD ) {
            bank->gold += amount;
            ch_printf( ch, "Tu cuenta tiene un saldo de %d monedas de oro.\r\n", bank->gold );
        }
        else if ( type == CURR_SILVER ) {
            bank->silver += amount;
            ch_printf( ch, "tu cuenta tiene un saldo de %d monedas de plata.\r\n", bank->silver );
        }
        save_char_obj( ch );
        bank->lastused = current_time;
        save_bank(  );
        return;
    }
    // end of deposit

    if ( !str_cmp( arg2, "" ) ) {
        if ( ch->pcdata->bank == NULL ) {
            send_to_char( "No tienes cuenta.\r\n", ch );
            return;
        }
        snprintf( buf, MSL, "Has cerrado la cuenta %s.\r\n", ch->pcdata->bank->name );
        send_to_char( buf, ch );
        ch->pcdata->bank = NULL;
        return;
    }
    if ( strcmp( sha256_crypt( arg2 ), bank->password ) ) {
        send_to_char( "Contraseña inválida.\r\n", ch );
        return;
    }

    snprintf( buf, MSL, "Abres la cuenta %s.\r\n", bank->name );
    send_to_char( buf, ch );
    ch->pcdata->bank = bank;
    ch_printf( ch, "&Cpara el banco %s la cuenta tiene:\r\n", bank->name );
    ch_printf( ch, "&Y%d oro.\r\n", bank->gold );
    ch_printf( ch, "&Y%d plata.\r\n", bank->silver );
    ch_printf( ch, "&Y%d bronce.\r\n", bank->bronze );
    ch_printf( ch, "&Y%d cobre.\r\n", bank->copper );

}

void free_bank_to_chars( BANK_DATA * bank )
{
    CHAR_DATA              *ch;

    if ( !bank )
        return;

    for ( ch = first_char; ch; ch = ch->next ) {
        if ( !ch || !ch->pcdata || !ch->pcdata->bank )
            continue;
        if ( ch->pcdata->bank != bank )
            continue;
        ch_printf( ch, "%s cuenta inexistente.\r\n", bank->name );
        ch->pcdata->bank = NULL;
    }
}
