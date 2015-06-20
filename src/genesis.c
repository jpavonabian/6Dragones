#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>

#ifdef LINUX
#include <time.h>
#endif
#ifndef WIN32
#include <unistd.h>
#include <sys/wait.h>
#endif

#include "h/mud.h"
#include "h/channels.h"
#include "h/auction.h"
#include "h/ban.h"
#include "h/files.h"
#include "h/polymorph.h"
#include "h/hometowns.h"
#include "h/new_auth.h"
#include "h/languages.h"
#include "h/clans.h"
#include "h/city.h"
#include "h/mssp.h"
#include "h/sha256.h"

//Bools for tutorial rooms in use -Taon
bool                    first_tutorial_room = FALSE;
bool                    second_tutorial_room = FALSE;
bool                    third_tutorial_room = FALSE;
bool                    first_etutorial_room = FALSE;
bool                    second_etutorial_room = FALSE;
bool                    third_etutorial_room = FALSE;
bool                    first_dtutorial_room = FALSE;
bool                    second_dtutorial_room = FALSE;
bool                    third_dtutorial_room = FALSE;
void                    fix_city_order( CITY_DATA * city );
bool                    has_city_position( CITY_DATA * city, const char *name );
void                    get_curr_players( void );
bool check_parse_name   args( ( char *name, bool newchar ) );
void checkBuidty        args( ( CHAR_DATA *ch ) );

extern bool             doubleexp;
extern bool             happyhour;
extern bool             cexp;
bool                    check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn );
bool                    check_playing( DESCRIPTOR_DATA *d, char *name, bool kick );
void                    addname( char **list, const char *name );

//int get_adv_class(int race);
bool                    quotes( CHAR_DATA *ch );

void genesis_menu( DESCRIPTOR_DATA *d )
{
    write_to_buffer( d, "\r\n\033[1;40;33m     Consejos clásicos para 6 dragones:\033[0;40;36m\r\n"
                     "    \r\n     Es una forma perfecta para contribuir al desarrollo\r\n"
                     "     que hagas publicidad del juego y ayudes a los nuevos jugadores.  \r\n     http://6dragones.genesismuds.com/\r\n"
                     "     Para ver las novedades del juego teclea \033[1;40;37mNoticias todas\033[0;40;36m.\r\n"
                     "\r\n     ¡Disfruta del juego!\r\n", 0 );
}

int get_adv_class( int race )
{
    if ( race == RACE_CELESTIAL )
        return CLASS_ANGEL;
    if ( race == RACE_DEMON )
        return CLASS_HELLSPAWN;
    if ( race == RACE_VAMPIRE )
        return CLASS_VAMPIRE;
    if ( race == RACE_MINDFLAYER )
        return CLASS_PSIONIC;

    return -1;
}

char                   *all_races( DESCRIPTOR_DATA *d )
{
    int                     iRace;
    char                   *buf;
    char                    thebuf[MSL];

    buf = thebuf;
    buf[0] = '\0';

    for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ ) {
        if ( race_table[iRace]->race_name && VLD_STR( race_table[iRace]->race_name )
             && str_cmp( race_table[iRace]->race_name, "unused" )
             && str_cmp( race_table[iRace]->race_name, "Animal" ) ) {
            if ( iRace > 0 ) {
                if ( strlen( buf ) + strlen( race_table[iRace]->race_name ) > 77 ) {
                    mudstrlcat( buf, "\r\n", MSL );
                    write_to_buffer( d, buf, 0 );
                    buf[0] = '\0';
                }
                else
                    mudstrlcat( buf, " ", MSL );
            }
            mudstrlcat( buf, race_table[iRace]->race_name, MSL );
        }
    }

    mudstrlcat( buf, "]\r\n: ", MSL );

    return buf;
}

char                   *all_classes( DESCRIPTOR_DATA *d )
{
    int                     iClass;
    char                   *buf;
    char                    thebuf[MSL];
    bool                    fclass = TRUE;

    buf = thebuf;
    buf[0] = '\0';

    if ( d->character->race == RACE_DRAGON ) {
        mudstrlcat( buf, "Rojo negro Azul Plateado Dorado]\r\n: ", MSL );
        return buf;
    }

    for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
        if ( class_table[iClass] && VLD_STR( class_table[iClass]->who_name )
             && !IS_SET( class_table[iClass]->race_restriction, 1 << d->character->race )
             && class_table[iClass]->starting ) {
            if ( !fclass ) {
                if ( strlen( buf ) + strlen( class_table[iClass]->who_name ) > 77 ) {
                    mudstrlcat( buf, "\r\n", MSL );
                    write_to_buffer( d, buf, 0 );
                    buf[0] = '\0';
                }
                else
                    mudstrlcat( buf, " ", MSL );
            }
            fclass = FALSE;
            mudstrlcat( buf, class_table[iClass]->who_name, MSL );
        }
    }
    mudstrlcat( buf, "]\r\n: ", MSL );

    return buf;
}

/* Check character to see if they can use the iClass with current classes */
bool can_use_class_multi( CHAR_DATA *ch, int iClass, bool silent )
{
    if ( iClass == CLASS_PRIEST && ( ch->Class == CLASS_DRUID || ch->secondclass == CLASS_DRUID ) ) {
        if ( !silent )
            send_to_char( "\r\nYou can't be a Priest.\r\nPlease select another class.\r\n", ch );
        return FALSE;
    }
    if ( iClass == CLASS_DRUID && ( ch->Class == CLASS_PRIEST || ch->secondclass == CLASS_PRIEST ) ) {
        if ( !silent )
            send_to_char( "\r\nYou can't be a Druid.\r\nPlease select another class.\r\n", ch );
        return FALSE;
    }

    if ( iClass == CLASS_SHADOWKNIGHT
         && ( ch->Class == CLASS_CRUSADER || ch->secondclass == CLASS_CRUSADER
              || ch->Class == CLASS_MONK || ch->secondclass == CLASS_MONK
              || ch->Class == CLASS_WARRIOR || ch->secondclass == CLASS_WARRIOR ) ) {
        if ( !silent )
            send_to_char
                ( "\r\nYou can't be a Shadowknight.\r\nPlease select another class.\r\n", ch );
        return FALSE;
    }
    if ( iClass == CLASS_CRUSADER
         && ( ch->Class == CLASS_SHADOWKNIGHT || ch->secondclass == CLASS_SHADOWKNIGHT
              || ch->Class == CLASS_MONK || ch->secondclass == CLASS_MONK
              || ch->Class == CLASS_WARRIOR || ch->secondclass == CLASS_WARRIOR ) ) {
        if ( !silent )
            send_to_char( "\r\nYou can't be a Crusader.\r\nPlease select another class.\r\n", ch );
        return FALSE;
    }
    if ( iClass == CLASS_MONK
         && ( ch->Class == CLASS_SHADOWKNIGHT || ch->secondclass == CLASS_SHADOWKNIGHT
              || ch->Class == CLASS_CRUSADER || ch->secondclass == CLASS_CRUSADER
              || ch->Class == CLASS_WARRIOR || ch->secondclass == CLASS_WARRIOR ) ) {
        if ( !silent )
            send_to_char( "\r\nYou can't be a Monk.\r\nPlease select another class.\r\n", ch );
        return FALSE;
    }
    if ( iClass == CLASS_WARRIOR
         && ( ch->Class == CLASS_SHADOWKNIGHT || ch->secondclass == CLASS_SHADOWKNIGHT
              || ch->Class == CLASS_CRUSADER || ch->secondclass == CLASS_CRUSADER
              || ch->Class == CLASS_MONK || ch->secondclass == CLASS_MONK ) ) {
        if ( !silent )
            send_to_char( "\r\nYou can't be a Warrior.\r\nPlease select another class.\r\n", ch );
        return FALSE;
    }
    return TRUE;
}

void genesis( DESCRIPTOR_DATA *d, char *argument )
{
    char                    buf[MSL],
                            arg[MSL];
    CHAR_DATA              *ch;
    char                   *pwdnew,
                           *p;
    int                     iClass,
                            iRace;
    bool                    fOld,
                            chk;
    int                     bi;

    while ( isspace( *argument ) )
        argument++;
    ch = d->character;
    set_pager_color( AT_WHITE, ch );
    switch ( d->connected ) {
        default:
            bug( "genesis: bad d->connected %d.", d->connected );
            close_socket( d, TRUE );
            return;

        case CON_OEDIT:
            break;

        case CON_REDIT:
            break;

        case CON_MEDIT:
            break;

        case CON_GET_NAME:
            if ( argument[0] == '\0' ) {
                close_socket( d, FALSE );
                return;
            }
            argument[0] = UPPER( argument[0] );

            if ( !str_cmp( argument, "MSSP-REQUEST" ) ) {
                send_mssp_data( d );
                // Uncomment below if you want to know when an MSSP request occurs
                // log_printf( "IP: %s requested MSSP data!", d->host );
                close_socket( d, FALSE );
                return;
            }
            /*
             * Old players can keep their characters. -- Alty 
             */
            if ( !check_parse_name( argument, ( d->newstate != 0 ) ) ) {
                write_to_buffer( d, "Nombre ilegal, intenta con otro.\r\nNombre: ", 0 );
                return;
            }
            if ( !str_cmp( argument, "Nuevo" ) ) {
                if ( d->newstate == 0 ) {
                    /*
                     * New player 
                     */
                    /*
                     * Don't allow new players if DENY_NEW_PLAYERS is true 
                     */
                    if ( sysdata.DENY_NEW_PLAYERS == TRUE ) {
                        snprintf( buf, MSL, "El mud se está preparando para un reinicio.\r\n" );
                        write_to_buffer( d, buf, 0 );
                        snprintf( buf, MSL, "No aceptamos nuevos jugadores mientras dure este proceso.\r\n" );
                        write_to_buffer( d, buf, 0 );
                        snprintf( buf, MSL, "Por favor intenta de nuevo en algunos minutos.\r\n" );
                        write_to_buffer( d, buf, 0 );
                        close_socket( d, FALSE );
                    }
                    snprintf( buf, MSL,
                              "\r\nEscoger un nombre es una de las partes más importantes del juego...\r\n"
                              "Asegúrate de que tu nombre es apropiado para el tipo de personaje que serás.\r\n"
                              "Recuerda que 6D es un juego de rol ambientado en una época medieval.\r\n"
                              "Si el nombre que escojas no es aceptable, se te pedirá que introduzcas otro\r\n"
                              "hasta encontrar uno apropiado para ti.\r\n\r\nPor favor introduce el nombre de tu personaje: " );
                    write_to_buffer( d, buf, 0 );
                    d->newstate++;
                    d->connected = CON_GET_NAME;
                    return;
                }
                else {
                    write_to_buffer( d, "Nombre ilegal, intenta con otro.\r\nNombre: ", 0 );
                    return;
                }
            }
            if ( check_playing( d, argument, FALSE ) == BERR ) {
                write_to_buffer( d, "Nombre: ", 0 );
                return;
            }
            fOld = load_char_obj( d, argument, TRUE, FALSE, FALSE );
            if ( !d->character ) {
                log_printf( "Bad player file %s@%s.", argument, d->host );
                write_to_buffer( d,
                                 "Tu ficha está corrupta... Avisa a algún inmortal.\r\n",
                                 0 );
                close_socket( d, FALSE );
                return;
            }
            ch = d->character;
            if ( check_bans( ch, BAN_SITE ) ) {
                write_to_buffer( d, "Tu ip ha sido baneada del servidor.\r\n", 0 );
                close_socket( d, FALSE );
                return;
            }
            if ( fOld ) {
                if ( !class_table[ch->Class] ) {
                    write_to_buffer( d,
                                     "Tu clase es inválida y posiblemente produzca un error en el servidor.\r\nPara evitarlo, tu clase ha sido ajustada a guerrero si existe.\r\n",
                                     0 );
                    ch->Class = 3;
                    if ( !class_table[ch->Class] ) {
                        write_to_buffer( d, "Warrior class doesn't exist...closing link.\r\n", 0 );
                        close_socket( d, FALSE );
                        return;
                    }
                }
                if ( !race_table[ch->race] ) {
                    write_to_buffer( d,
                                     "Tu raza es inválida y posiblemente provoque un error en el servidor.\r\nPara evitarlo tu raza ha sido ajustada a humano si es que existe.\r\n",
                                     0 );
                    ch->race = 0;
                    if ( !race_table[ch->race] ) {
                        write_to_buffer( d, "No existe la raza humano. Cerrando la conexión.\r\n", 0 );
                        close_socket( d, FALSE );
                        return;
                    }
                }
/*
                if ( check_bans( ch, BAN_CLASS ) ) {
                    write_to_buffer( d, "Tu clase ha sido baneada del servidor.\r\n", 0 );
                    close_socket( d, FALSE );
                    return;
                }
*/
                if ( check_bans( ch, BAN_RACE ) ) {
                    write_to_buffer( d, "tu raza ha sido baneada del servidor.\r\n", 0 );
                    close_socket( d, FALSE );
                    return;
                }
            }
            if ( xIS_SET( ch->act, PLR_DENY ) ) {
                char                    logbuf[MSL];

                snprintf( logbuf, MSL, "Denying access to %s@%s.", argument, d->host );
                log_string_plus( logbuf, LOG_COMM, sysdata.log_level );
                if ( d->newstate != 0 ) {
                    write_to_buffer( d, "Ese nombre ya existe. Por favor escoge otro: ", 0 );
                    d->connected = CON_GET_NAME;
                    d->character->desc = NULL;
                    free_char( d->character );         /* Big Memory Leak before
                                                        * --Shaddai */
                    d->character = NULL;
                    return;
                }
                write_to_buffer( d, "Acceso denegado.\r\n", 0 );
                close_socket( d, FALSE );
                return;
            }
            /*
             * telnet negotiation to see if they support MXP 
             */
            // write_to_buffer( d, will_mxp_str, 0 );

            chk = check_reconnect( d, argument, FALSE );
            if ( chk == BERR )
                return;
            if ( chk )
                fOld = TRUE;
            else {
                if ( sysdata.wiz_lock && !IS_IMMORTAL( ch ) ) {
                    write_to_buffer( d, "El juego está bloqueado. Solo pueden conectar administradores.\r\n",
                                     0 );
                    write_to_buffer( d, "Por favor inténtalo más tarde.\r\n", 0 );
                    close_socket( d, FALSE );
                    return;
                }
            }
            if ( fOld ) {
                if ( d->newstate != 0 ) {
                    write_to_buffer( d, "Ese nombre ya ha sido escogido. Por favor prueba otro: ", 0 );
                    d->connected = CON_GET_NAME;
                    d->character->desc = NULL;
                    free_char( d->character );         /* Big Memory Leak before
                                                        * --Shaddai */
                    d->character = NULL;
                    return;
                }
                /*
                 * Old player 
                 */
                write_to_buffer( d, "Password: ", 0 );
                write_to_buffer( d, echo_off_str, 0 );
                d->connected = CON_GET_OLD_PASSWORD;
                return;
            }
            else {
                if ( d->newstate == 0 ) {
                    /*
                     * No such player 
                     */
                    write_to_buffer( d,
                                     "\r\nEse jugador no existe.\r\nPor favor revisa tu ortografía o escribe nuevo para crear un nuevo jugador.\r\n\r\nNombre: ",
                                     0 );
                    d->connected = CON_GET_NAME;
                    d->character->desc = NULL;
                    free_char( d->character );         /* Big Memory Leak before
                                                        * --Shaddai */
                    d->character = NULL;
                    return;
                }
                snprintf( buf, MSL, "¿Te he entendido bien, %s (S/N)? ", argument );
                write_to_buffer( d, buf, 0 );
                d->connected = CON_CONFIRM_NEW_NAME;
                return;
            }
            break;
        case CON_GET_OLD_PASSWORD:
            write_to_buffer( d, "\r\n", 2 );

            if ( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) ) {
                write_to_buffer( d, "Contraseña incorrecta, desconectando.\r\n", 0 );
                /*
                 * clear descriptor pointer to get rid of bug message in log 
                 */
                d->character->desc = NULL;
                close_socket( d, false );
                return;
            }

            write_to_buffer( d, ( const char * ) echo_on_str, 0 );

            if ( check_playing( d, ch->pcdata->filename, true ) )
                return;

            if ( ( chk = check_reconnect( d, ch->pcdata->filename, true ) ) )
                return;

            mudstrlcpy( buf, ch->pcdata->filename, sizeof( buf ) );
            d->character->desc = NULL;
            free_char( d->character );
            d->character = NULL;
            fOld = load_char_obj( d, buf, FALSE, FALSE, FALSE );
            ch = d->character;
            if ( ch->fighting )
                ch->position = POS_STANDING;
            if ( !str_cmp( ch->name, "Zlorth" ) ) {
                log_printf_plus( LOG_COMM, get_trust( ch ), "%s (127.0.0.1) has connected.",
                                 ch->name );
            }
            else {
                log_printf_plus( LOG_COMM, get_trust( ch ), "%s (%s) has connected.", ch->name,
                                 d->host );
            }
            struct tm              *tme;
            time_t                  now;
            char                    day[50];

            now = time( 0 );
            tme = localtime( &now );
            strftime( day, 50, "%a %b %d %H:%M:%S %Y", tme );
            if ( xIS_SET( ch->act, PLR_COMMUNICATION ) )
                send_to_char( "!!SOUND(sound/player.wav)\r\n", ch );

            snprintf( buf, MSL, "%20s %24s has logged on.", ch->name, day );
            append_to_file( LAST_FILE, buf );
            auth_update(  );

            write_to_buffer( d, "\r\nPor favor pulsa [ENTER]", 0 );
            d->connected = CON_ENTER_GAME;
            break;

        case CON_CONFIRM_NEW_NAME:
            switch ( *argument ) {
                case 's':
                case 'S':
                    snprintf( buf, MSL,
                              "\r\nAsegúrate de elegir una contraseña que solo tu puedas saber.\r\nTeclea una buena contraseña para %s: %s",
                              ch->name, echo_off_str );
                    write_to_buffer( d, buf, 0 );
                    d->connected = CON_GET_NEW_PASSWORD;
                    break;
                case 'n':
                case 'N':
                    write_to_buffer( d, "¿Entonces como te llamas? ", 0 );
                    /*
                     * clear descriptor pointer to get rid of bug message in log 
                     */
                    d->character->desc = NULL;
                    free_char( d->character );
                    d->character = NULL;
                    d->connected = CON_GET_NAME;
                    break;
                default:
                    write_to_buffer( d, "Por favor escribe si o no. ", 0 );
                    break;
            }
            break;
        case CON_GET_NEW_PASSWORD:
            write_to_buffer( d, "\r\n", 2 );

            if ( strlen( argument ) < 5 ) {
                write_to_buffer( d, "La contraseña debe tener un mínimo de cinco carácteres.\r\nPassword: ",
                                 0 );
                return;
            }

            if ( argument[0] == '!' ) {
                send_to_char( "La contraseña no puede comenzar por '!'.\r\n", ch );
                return;
            }
            pwdnew = sha256_crypt( argument );         /* SHA-256 Encryption */
            STRSET( ch->pcdata->pwd, pwdnew );
            write_to_buffer( d, "\r\nPor favor repite la contraseña para confirmar: ", 0 );
            d->connected = CON_CONFIRM_NEW_PASSWORD;
            break;

        case CON_CONFIRM_NEW_PASSWORD:
            write_to_buffer( d, "\r\n", 2 );

            if ( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) ) {
                write_to_buffer( d, "Las contraseñas no coinciden.\r\nRetype password: ", 0 );
                d->connected = CON_GET_NEW_PASSWORD;
                return;
            }

            write_to_buffer( d, ( const char * ) echo_on_str, 0 );
            write_to_buffer( d, "\r\n¿Cuál es tu sexo, (M/F)? ", 0 );
            d->connected = CON_GET_NEW_SEX;
            break;

            /*
             * Con state for self delete code, added by Samson 1-18-98
             * * Code courtesy of Waldemar Thiel (Swiv) 
             */
        case CON_DELETE:
            write_to_buffer( d, "\r\n", 2 );
            if ( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) ) {
                write_to_buffer( d, "Contraseña incorrecta, borrado cancelado.\r\n", 0 );
                write_to_buffer( d, ( const char * ) echo_on_str, 0 );
                d->connected = CON_PLAYING;
                update_cities(  );
                return;
            }
            else {
                write_to_buffer( d, "\r\n¡has borrado tu personaje!!!\r\n", 0 );
                log_printf( "Player: %s has deleted.", capitalize( ch->name ) );
                do_destroy( ch, ch->name );
                return;
            }
            break;

        case CON_GET_NEW_SEX:
            switch ( argument[0] ) {
                case 'm':
                case 'M':
                    ch->sex = SEX_MALE;
                    break;
                case 'f':
                case 'F':
                    ch->sex = SEX_FEMALE;
                    break;
                default:
                    write_to_buffer( d, "Eso no es un sexo.\r\n¿Cuál es tu sexo? ", 0 );
                    return;
            }
            write_to_buffer( d,
                             "\r\nPuedes elegir una de las siguientes razas, o escribir ayuda [raza] para aprender más:\r\n[",
                             0 );
            write_to_buffer( d, all_races( d ), 0 );
            d->connected = CON_GET_NEW_RACE;
            break;

        case CON_GET_NEW_RACE:
            argument = one_argument( argument, arg );
            if ( !str_cmp( arg, "ayuda" ) ) {
                for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ ) {
                    if ( ( race_table[iRace] && VLD_STR( race_table[iRace]->race_name )
                           && toupper( argument[0] ) == toupper( race_table[iRace]->race_name[0] )
                           && !str_prefix( argument, race_table[iRace]->race_name ) )
                         || !str_cmp( argument, "multiclass" ) || !str_cmp( argument, "races" )
                         || !str_cmp( argument, "multiclass race" ) ) {
                        do_help( ch, argument );
                        write_to_buffer( d, all_races( d ), 0 );
                        write_to_buffer( d, "\r\nPor favor elige una raza: [ ", 0 );
                        return;
                    }
                }
                write_to_buffer( d, "Esa ayuda no existe.  Por favor escoge una raza: [ ", 0 );
                write_to_buffer( d, all_races( d ), 0 );
                return;
            }
            for ( iRace = 0; iRace < MAX_PC_RACE; iRace++ ) {
                if ( race_table[iRace] && VLD_STR( race_table[iRace]->race_name )
                     && toupper( arg[0] ) == toupper( race_table[iRace]->race_name[0] )
                     && !str_prefix( arg, race_table[iRace]->race_name ) ) {
                    ch->race = iRace;
                    break;
                }
            }
            if ( iRace == MAX_PC_RACE || !race_table[iRace] || !race_table[iRace]->race_name
                 || race_table[iRace]->race_name[0] == '\0'
                 || !str_cmp( race_table[iRace]->race_name, "unused" ) ) {
                write_to_buffer( d, "Eso no es una raza.\r\n¿cuál es tu raza? [ ", 0 );
                write_to_buffer( d, all_races( d ), 0 );
                return;
            }
            if ( check_bans( ch, BAN_RACE ) ) {
                write_to_buffer( d,
                                 "Esa raza no está disponible actualmente.\r\n¿Cuál es tu raza? [ ",
                                 0 );
                write_to_buffer( d, all_races( d ), 0 );
                return;
            }

            if ( xIS_SET( race_table[ch->race]->flags, RACE_ADVANCED ) && ch->race != RACE_DRAGON ) {
                ch->Class = get_adv_class( ch->race );
                if ( ch->Class == -1 ) {
                    write_to_buffer( d,
                                     "¡Error desconocido! Por favor escoge otra raza/clase y repórtalo a un inmortal.\r\n [ ",
                                     0 );
                    write_to_buffer( d, all_races( d ), 0 );
                    d->connected = CON_GET_NEW_RACE;
                    break;
                }
                else {
                    ch->secondclass = -1;
                    ch->thirdclass = -1;
                    write_to_buffer( d,
                                     "Puedes usar tus puntos para incrementar tus características.\r\npor ejemplo escribiendo sab\r\naumentarás tu sabiduría en un punto.",
                                     0 );
                    name_stamp_stats( ch );
                    snprintf( buf, MSL,
                              "\r\nFue: %d  Int: %d  Sab: %d  Des: %d  Con: %d  Car: %d  Sue: %d Puntos: %d",
                              ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con,
                              ch->perm_cha, ch->perm_lck, ch->statpoints );
                    write_to_buffer( d, buf, 0 );
                    d->connected = CON_ROLL_STATS;
                    break;
                }
            }
            else {
                write_to_buffer( d,
                                 "\r\nPuedes escoger una de las siguientes clases, o escribir ayuda [clase] para aprender más: \r\n[",
                                 0 );
                write_to_buffer( d, all_classes( d ), 0 );
                d->connected = CON_GET_NEW_CLASS;
                break;
            }
            return;

        case CON_GET_NEW_CLASS:
            argument = one_argument( argument, arg );
            if ( !str_cmp( arg, "ayuda" ) ) {
                for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
                    if ( class_table[iClass] && VLD_STR( class_table[iClass]->who_name )
                         && class_table[iClass]->starting ) {
                        if ( toupper( argument[0] ) == toupper( class_table[iClass]->who_name[0] )
                             && !str_prefix( argument, class_table[iClass]->who_name )
                             || !str_cmp( argument, "multiclass" )
                             || !str_cmp( argument, "classes" )
                             || !str_cmp( argument, "multiclass race" ) ) {
                            do_help( ch, argument );
                            write_to_buffer( d, "Por favor escoge una clase: [ ", 0 );
                            write_to_buffer( d, all_classes( d ), 0 );
                            return;
                        }
                    }
                }
                write_to_buffer( d, "Esa ayuda no existe. Por favor escoge una clase: [ ", 0 );
                write_to_buffer( d, all_classes( d ), 0 );
                return;
            }
            for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
                if ( class_table[iClass] && VLD_STR( class_table[iClass]->who_name )
                     && class_table[iClass]->starting ) {
                    if ( toupper( arg[0] ) == toupper( class_table[iClass]->who_name[0] )
                         && !str_prefix( arg, class_table[iClass]->who_name )
                         && str_cmp( class_table[iClass]->who_name, "Ancient" ) ) {
                        ch->Class = iClass;
                        break;
                    }
                }
            }

            // Monk crap
            if ( iClass == CLASS_MONK || iClass == CLASS_BEAST ) {
                ch->secondclass = -1;
                ch->thirdclass = -1;
                write_to_buffer( d,
                                 "Puedes usar tus puntos para incrementar tus características.\r\nPor ejemplo escribiendo sab\r\nincrementarás tu sabiduría en un punto.",
                                 0 );
                name_stamp_stats( ch );
                snprintf( buf, MSL,
                          "\r\nFue: %d  Int: %d  Sab: %d  Des: %d  Con: %d  Car: %d  Sue: %d Puntos: %d",
                          ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con,
                          ch->perm_cha, ch->perm_lck, ch->statpoints );
                write_to_buffer( d, buf, 0 );
                d->connected = CON_ROLL_STATS;
                return;
            }
            if ( iClass == MAX_PC_CLASS || !class_table[iClass]
                 || !class_table[iClass]->starting || !VLD_STR( class_table[iClass]->who_name )
                 || IS_SET( class_table[iClass]->race_restriction, 1 << ch->race )
                 || !str_cmp( class_table[iClass]->who_name, "unused" )
                 || !str_cmp( class_table[iClass]->who_name, "Ancient" ) ) {
                write_to_buffer( d, "Eso no es una clase.\r\n¿Cuál es tu clase? [ ", 0 );
                write_to_buffer( d, all_classes( d ), 0 );
                return;
            }

            if ( xIS_SET( race_table[ch->race]->flags, RACE_ADVANCED ) ) {
                ch->secondclass = -1;
                ch->thirdclass = -1;
                write_to_buffer( d,
                                 "Puedes usar tus puntos para incrementar tus características.\r\nPor ejemplo escribiendo sab\r\nincrementarás tu sabiduría en un punto.",
                                 0 );
                name_stamp_stats( ch );
                snprintf( buf, MSL,
                          "\r\nFue: %d  Int: %d  Sab: %d  Des: %d  Con: %d  Car: %d  Sue: %d Puntos: %d",
                          ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con,
                          ch->perm_cha, ch->perm_lck, ch->statpoints );
                write_to_buffer( d, buf, 0 );
                d->connected = CON_ROLL_STATS;
            }
            else {
                write_to_buffer( d, "\r\n¿Te gustaría una segunda clase? ¿S/N?\r\n", 0 );
                d->connected = CON_GET_FIRST_CHOICE;
            }
            return;

        case CON_GET_FIRST_CHOICE:
            switch ( argument[0] ) {
                case 's':
                case 'S':
                    {
                        bool                    fclass = TRUE;

                        write_to_buffer( d,
                                         "\r\nPuedes elegir una de las siguientes clases, o teclear ayuda [clase] para aprender más:\r\n[",
                                         0 );
                        buf[0] = '\0';
                        for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
                            if ( d->character
                                 && !can_use_class_multi( d->character, iClass, TRUE ) )
                                continue;
                            if ( class_table[iClass] && VLD_STR( class_table[iClass]->who_name )
                                 && !IS_SET( class_table[iClass]->race_restriction, 1 << ch->race )
                                 && class_table[iClass]->starting && iClass != ch->Class ) {
                                if ( !fclass )
                                    mudstrlcat( buf, " ", MSL );
                                fclass = FALSE;
                                mudstrlcat( buf, class_table[iClass]->who_name, MSL );
                                write_to_buffer( d, buf, 0 );
                                buf[0] = '\0';
                            }
                        }

                    }
                    mudstrlcat( buf, " Druid ]\r\n: ", MSL );
                    write_to_buffer( d, buf, 0 );
                    buf[0] = '\0';
                    d->connected = CON_GET_SECOND_CLASS;
                    return;

                case 'n':
                case 'N':
                    ch->secondclass = -1;
                    ch->thirdclass = -1;
                    write_to_buffer( d,
                                     "Puedes usar tus puntos para incrementar tus características.\r\nPor ejemplo escribiendo sab\r\nincrementarás tu sabiduría en un punto.",
                                     0 );
                    name_stamp_stats( ch );
                    snprintf( buf, MSL,
                              "\r\nFue: %d  Int: %d  Sab: %d  Des: %d  Con: %d  Car: %d  Sue: %d Puntos: %d",
                              ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con,
                              ch->perm_cha, ch->perm_lck, ch->statpoints );
                    write_to_buffer( d, buf, 0 );
                    d->connected = CON_ROLL_STATS;
                    return;

                default:
                    write_to_buffer( d, "¿Si o no? ", 0 );
                    return;
            }
            break;

        case CON_GET_SECOND_CLASS:
            argument = one_argument( argument, arg );
            if ( !str_cmp( arg, "ayuda" ) ) {
                for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
                    if ( d->character && !can_use_class_multi( d->character, iClass, TRUE ) )
                        continue;

                    if ( class_table[iClass] && VLD_STR( class_table[iClass]->who_name )
                         && class_table[iClass]->starting ) {
                        if ( toupper( argument[0] ) == toupper( class_table[iClass]->who_name[0] )
                             && !str_prefix( argument, class_table[iClass]->who_name )
                             || !str_cmp( argument, "multiclass" )
                             || !str_cmp( argument, "classes" )
                             || !str_cmp( argument, "multiclass race" ) ) {
                            do_help( ch, argument );
                            write_to_buffer( d, "Por favor escoge una clase: ", 0 );
                            return;
                        }
                    }
                }
                write_to_buffer( d, "No existe esa ayuda. Por favor escoge una clase: ", 0 );
                return;
            }

            if ( !str_cmp( arg, "Monk" )  || iClass == CLASS_MONK ) {
                write_to_buffer( d, "The Monk Class cannot multiclass, choose another class.\r\n",
                                 0 );
                d->connected = CON_GET_SECOND_CLASS;
                return;
            }
            if ( !str_cmp( arg, "Beast" ) || iClass == CLASS_BEAST ) {
                write_to_buffer( d, "The Beast master Class cannot multiclass, choose another class.\r\n",
                                 0 );
               	d->connected = CON_GET_SECOND_CLASS;
                return;
            }

            if ( !str_cmp( arg, "Druid" ) ) {
                ch->secondclass = 1;
                write_to_buffer( d, "\r\n¿Te gustaría una tercera clase? ¿S/N?\r\n", 0 );
                d->connected = CON_GET_SECOND_CHOICE;
                return;
            }

            for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
                if ( class_table[iClass] && VLD_STR( class_table[iClass]->who_name )
                     && class_table[iClass]->starting ) {
                    if ( d->character && !can_use_class_multi( d->character, iClass, TRUE ) )
                        continue;

                    if ( toupper( arg[0] ) == toupper( class_table[iClass]->who_name[0] )
                         && !str_prefix( arg, class_table[iClass]->who_name )
                         && str_cmp( class_table[iClass]->who_name, "Ancient" ) ) {
                        ch->secondclass = iClass;
                        ch->firstlevel = 1;
                        ch->secondlevel = 1;
                        ch->secondexp = 0;
                        ch->firstexpratio = 50;
                        ch->secondexpratio = 50;
                        break;
                    }
                }
            }
            if ( iClass == MAX_PC_CLASS || !class_table[iClass]
                 || !class_table[iClass]->starting || !VLD_STR( class_table[iClass]->who_name )
                 || IS_SET( class_table[iClass]->race_restriction, 1 << ch->race )
                 || !str_cmp( class_table[iClass]->who_name, "unused" )
                 || !str_cmp( class_table[iClass]->who_name, "Ancient" ) ) {
                write_to_buffer( d, "Eso no es una clase.\r\n¿Cuál es tu clase? ", 0 );
                return;
            }
            if ( ch->Class == iClass ) {
                write_to_buffer( d,
                                 "\r\nNo puedes escoger la misma clase que la primaria.\r\nPor favor escoge otra clase.\r\n",
                                 0 );
                return;
            }
            if ( !can_use_class_multi( ch, iClass, FALSE ) )
                return;
            write_to_buffer( d, "\r\n¿Te gustaría una tercera clase? ¿S/N?\r\n", 0 );
            d->connected = CON_GET_SECOND_CHOICE;
            break;

        case CON_GET_SECOND_CHOICE:
            switch ( argument[0] ) {
                case 's':
                case 'S':
                    {
                        bool                    fclass = TRUE;

                        write_to_buffer( d,
                                         "\r\nPor favor escoge una de las siguientes clases, o teclea ayuda [clase] para aprender más:\r\n[",
                                         0 );
                        buf[0] = '\0';
                        for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
                            if ( d->character
                                 && !can_use_class_multi( d->character, iClass, TRUE ) )
                                continue;

                            if ( class_table[iClass]
                                 && VLD_STR( class_table[iClass]->who_name )
                                 && !IS_SET( class_table[iClass]->race_restriction, 1 << ch->race )
                                 && str_cmp( class_table[iClass]->who_name, "Ancient" )
                                 && class_table[iClass]->starting && iClass != ch->Class
                                 && iClass != ch->secondclass ) {
                                if ( !fclass )
                                    mudstrlcat( buf, " ", MSL );
                                fclass = FALSE;
                                mudstrlcat( buf, class_table[iClass]->who_name, MSL );
                                write_to_buffer( d, buf, 0 );
                                buf[0] = '\0';
                            }
                        }
                        mudstrlcat( buf, " ]\r\n: ", MSL );
                        write_to_buffer( d, buf, 0 );
                        buf[0] = '\0';
                        d->connected = CON_GET_THIRD_CLASS;
                    }
                    return;

                case 'n':
                case 'N':
                    ch->thirdclass = -1;
                    write_to_buffer( d,
                                     "Puedes usar tus puntos para incrementar tus características.\r\nPor ejemplo escribiendo sab\\r\nincrementarás tu sabiduría en un punto.",
                                     0 );
                    name_stamp_stats( ch );
                    snprintf( buf, MSL,
                              "\r\nFue: %d  Int: %d  Sab: %d  Des: %d  Con: %d  Car: %d  Sue: %d Puntos: %d",
                              ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con,
                              ch->perm_cha, ch->perm_lck, ch->statpoints );
                    write_to_buffer( d, buf, 0 );
                    d->connected = CON_ROLL_STATS;
                    return;

                default:
                    write_to_buffer( d, "¿Si o no? ", 0 );
                    return;
            }
            return;

        case CON_GET_THIRD_CLASS:
            argument = one_argument( argument, arg );
            if ( !str_cmp( arg, "ayuda" ) ) {
                for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
                    if ( d->character && !can_use_class_multi( d->character, iClass, TRUE ) )
                        continue;

                    if ( class_table[iClass] && VLD_STR( class_table[iClass]->who_name )
                         && class_table[iClass]->starting ) {
                        if ( toupper( argument[0] ) == toupper( class_table[iClass]->who_name[0] )
                             && !str_prefix( argument, class_table[iClass]->who_name )
                             || !str_cmp( argument, "multiclass" )
                             || !str_cmp( argument, "classes" )
                             || !str_cmp( argument, "multiclass race" ) ) {
                            do_help( ch, argument );
                            write_to_buffer( d, "Por favor escoge una clase: ", 0 );
                            return;
                        }
                    }
                }
                write_to_buffer( d, "No existe esa ayuda. Por favor escoge una clase: ", 0 );
                return;
            }

            if ( !str_cmp( arg, "Monk" )  || iClass == CLASS_MONK ) {
                write_to_buffer( d, "The Monk Class cannot multiclass, choose another class.\r\n",
                                 0 );
                d->connected = CON_GET_THIRD_CLASS;
                return;
            }

            if ( !str_cmp( arg, "Beast" )  || iClass == CLASS_BEAST ) {
                write_to_buffer( d, "The Beast Master Class cannot multiclass, choose another class.\r\n",
                                 0 );
               	d->connected = CON_GET_THIRD_CLASS;
                return;
            }


            for ( iClass = 0; iClass < MAX_PC_CLASS; iClass++ ) {
                if ( d->character && !can_use_class_multi( d->character, iClass, TRUE ) )
                    continue;

                if ( class_table[iClass] && VLD_STR( class_table[iClass]->who_name )
                     && class_table[iClass]->starting ) {
                    if ( toupper( arg[0] ) == toupper( class_table[iClass]->who_name[0] )
                         && !str_prefix( arg, class_table[iClass]->who_name )
                         && str_cmp( class_table[iClass]->who_name, "Ancient" ) ) {
                        ch->thirdclass = iClass;
                        ch->firstlevel = 1;
                        ch->secondlevel = 1;
                        ch->thirdlevel = 1;            // Temporary.
                        ch->thirdexp = 0;
                        ch->firstexpratio = 50;        // Construct expratio defaults.
                        // -Taon
                        ch->secondexpratio = 30;
                        ch->thirdexpratio = 20;
                        break;
                    }
                }
            }
            if ( iClass == MAX_PC_CLASS || !class_table[iClass]
                 || !class_table[iClass]->starting || !VLD_STR( class_table[iClass]->who_name )
                 || IS_SET( class_table[iClass]->race_restriction, 1 << ch->race )
                 || !str_cmp( class_table[iClass]->who_name, "unused" )
                 || !str_cmp( class_table[iClass]->who_name, "Ancient" ) ) {
                write_to_buffer( d, "Eso no es una clase.\r\n¿Cuál es tu clase? ", 0 );
                return;
            }
            if ( ch->Class == iClass || ch->secondclass == iClass ) {
                write_to_buffer( d,
                                 "\r\nNo puedes escoger una clase ya elegida. \r\nPor favor escoge otra.",
                                 0 );
                return;
            }

            if ( !can_use_class_multi( ch, iClass, FALSE ) )
                return;

            /*
             * if (!IS_SET(class_table[iClass]->combo_restriction, 1 << ch->class) ||
             * !IS_SET(class_table[iClass]->combo_restriction, 1 << ch->secondclass)) {
             * write_to_buffer(d, "No puedes combinar tu clase actual con esa clase.\r\n", 0); return; } 
             */
            write_to_buffer( d,
                             "Puedess usar tus puntos para incrementar tus características.\r\nPor ejemplo escribiendo sab\r\nincrementarás tu sabiduría en un punto.",
                             0 );
            name_stamp_stats( ch );
            snprintf( buf, MSL,
                      "\r\nFue: %d  Int: %d  Sab: %d  Des: %d  Con: %d  Car: %d  Sue: %d Puntos: %d",
                      ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con,
                      ch->perm_cha, ch->perm_lck, ch->statpoints );
            write_to_buffer( d, buf, 0 );
            d->connected = CON_ROLL_STATS;
            break;

        case CON_ROLL_STATS:
            if ( !str_cmp( argument, "fue" ) ) {
                if ( get_curr_str( ch ) >= 25 ) {
                    write_to_buffer( d, "Tu fuerza ya está al máximo.\r\n", 0 );
                    return;
                }
                ch->perm_str++;
                write_to_buffer( d, "\r\ntu fuerza se ha incrementado en 1.\r\n", 0 );
            }
            else if ( !str_cmp( argument, "int" ) ) {
                if ( get_curr_int( ch ) >= 25 ) {
                    write_to_buffer( d, "tu inteligencia ya está al máximo.\r\n",
                                     0 );
                    return;
                }
                ch->perm_int++;
                write_to_buffer( d, "\r\nTu inteligencia se ha incrementado en 1.\r\n", 0 );
            }
            else if ( !str_cmp( argument, "sab" ) ) {
                if ( get_curr_wis( ch ) >= 25 ) {
                    write_to_buffer( d, "Tu sabiduría ya está al máximo.\r\n", 0 );
                    return;
                }
                ch->perm_wis++;
                write_to_buffer( d, "\r\ntu sabiduría se ha incrementado en 1.\r\n", 0 );
            }
            else if ( !str_cmp( argument, "des" ) ) {
                if ( get_curr_dex( ch ) >= 25 ) {
                    write_to_buffer( d, "Tu destreza ya está al máximo.\r\n", 0 );
                    return;
                }
                ch->perm_dex++;
                write_to_buffer( d, "\r\ntu destreza se ha incrementado en 1.\r\n", 0 );
            }
            else if ( !str_cmp( argument, "con" ) ) {
                if ( get_curr_con( ch ) >= 25 ) {
                    write_to_buffer( d, "Tu constitución está al máximo.\r\n",
                                     0 );
                    return;
                }
                ch->perm_con++;
                write_to_buffer( d, "\r\nTu constitución se ha incrementado en 1.\r\n", 0 );
            }
            else if ( !str_cmp( argument, "car" ) ) {
                if ( get_curr_cha( ch ) >= 25 ) {
                    write_to_buffer( d, "tu carisma ya está al máximo.\r\n", 0 );
                    return;
                }
                ch->perm_cha++;
                write_to_buffer( d, "\r\ntu carisma se ha incrementado en 1.\r\n", 0 );
            }
            else if ( !str_cmp( argument, "sue" ) ) {
                if ( get_curr_lck( ch ) >= 25 ) {
                    write_to_buffer( d, "tu suerte ya está al máximo.\r\n", 0 );
                    return;
                }
                ch->perm_lck++;
                write_to_buffer( d, "\r\nTu suerte se ha incrementado en 1.\r\n", 0 );
            }
            else {
                write_to_buffer( d,
                                 "\r\nusa \"fue\", \"int\", \"sab\", \"des\", \"con\", \"car\", \"sue\"\r\n",
                                 0 );
                return;
            }

            if ( --ch->statpoints <= 0 ) {
                write_to_buffer( d, "\r\n¿Quieres jugar con ansi o en modo No color, (A/N)? ",
                                 0 );
                d->connected = CON_GET_ANSI;
                return;
            }

            write_to_buffer( d,
                             "Puedes usar tus puntos para incrementar tus características.\r\nPor ejemplo escribiendo sab\r\nincrementarás tu sabiduría en un punto.",
                             0 );
            snprintf( buf, MSL,
                      "\r\nFue: %d  Int: %d  Sab: %d  Des: %d  Con: %d  Car: %d  Sue: %d Points: %d",
                      ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con,
                      ch->perm_cha, ch->perm_lck, ch->statpoints );
            write_to_buffer( d, buf, 0 );
            break;

        case CON_GET_ANSI:
            switch ( argument[0] ) {
                case 'a':
                case 'A':
                    xSET_BIT( ch->act, PLR_ANSI );
                    break;
                case 'n':
                case 'N':
                    break;
                default:
                    write_to_buffer( d, "Selección inválida.\r\nANSI o NADA? ", 0 );
                    return;
            }

            if ( IS_SECONDCLASS( ch ) && !IS_THIRDCLASS( ch ) )
                log_printf( "%s@%s new %s %s/%s.", ch->name, d->host,
                            race_table[ch->race]->race_name, class_table[ch->Class]->who_name,
                            class_table[ch->secondclass]->who_name );
            if ( IS_THIRDCLASS( ch ) )
                log_printf( "%s@%s new %s %s/%s/%s.", ch->name, d->host,
                            race_table[ch->race]->race_name, class_table[ch->Class]->who_name,
                            class_table[ch->secondclass]->who_name,
                            class_table[ch->thirdclass]->who_name );
            if ( !IS_SECONDCLASS( ch ) )
                log_printf( "%s@%s new %s %s.", ch->name, d->host, race_table[ch->race]->race_name,
                            class_table[ch->Class]->who_name );
            write_to_buffer( d, "\r\nPara configurar como el juego te mostrará cierta información, responde a la pregunta. No tendrá otro efecto salvo ese en el juego.\r\n¿tienes problemas de visión? Teclea S o N.", 0 );
            d->connected = CON_GET_BLIND;
            break;

        case CON_GET_BLIND:
// ************ This portion code added by Vladaar to check to see if blind?  ********
            switch ( argument[0] ) {
                case 's':
                case 'S':
                    xSET_BIT( ch->act, PLR_BLIND );
                    xSET_BIT( ch->act, PLR_BRIEF );
                    SET_BIT( ch->pcdata->flags, PCFLAG_GAG );
                    if ( VLD_STR( ch->pcdata->prompt ) )
                        STRFREE( ch->pcdata->prompt );
                    ch->pcdata->prompt = STRALLOC( ">" );
                    break;
                case 'n':
                case 'N':
                    break;
                default:
                    write_to_buffer( d, "Selección inválida.\r\n¿Si o no? ", 0 );
                    return;
            }
// ************ End of Vladaar Mess ********************************
            write_to_buffer( d, "\r\nPulsa [ENTER]", 0 );
            d->connected = CON_ENTER_GAME;
            break;
        case CON_ENTER_MENU:
            write_to_buffer( d, "\r\nPor favor pulsa [ENTER]", 0 );
            d->connected = CON_ENTER_GAME;
            break;
        case CON_ENTER_GAME:
            genesis_menu( d );
            add_char( ch );
            if ( ch->pcdata->clan )
                update_roster( ch );
            if ( ch->pcdata->city )
                update_rollcall( ch );
            d->connected = CON_PLAYING;
            if ( ch->level == 0 ) {
                OBJ_DATA               *obj;
                int                     iLang;
                int                     fakechance;

                reset_colors( ch );
                ch->pcdata->clan = NULL;
                ch->affected_by = race_table[ch->race]->affected;
                ch->armor += race_table[ch->race]->ac_plus;
                ch->alignment += race_table[ch->race]->alignment;
                ch->attacks = race_table[ch->race]->attacks;
                ch->defenses = race_table[ch->race]->defenses;
                ch->saving_poison_death = race_table[ch->race]->saving_poison_death;
                ch->saving_wand = race_table[ch->race]->saving_wand;
                ch->saving_para_petri = race_table[ch->race]->saving_para_petri;
                ch->saving_breath = race_table[ch->race]->saving_breath;
                ch->saving_spell_staff = race_table[ch->race]->saving_spell_staff;
                fakechance = number_range( 1, 10 );
                if ( fakechance < 3 )
                    ch->height = race_table[ch->race]->height - 6;
                if ( fakechance >= 3 && fakechance < 6 )
                    ch->height = race_table[ch->race]->height;
                if ( fakechance >= 6 && fakechance < 11 )
                    ch->height = race_table[ch->race]->height + 6;
                ch->weight =
                    number_range( ( int ) ( race_table[ch->race]->weight * .9 ),
                                  ( int ) ( race_table[ch->race]->weight * 1.1 ) );
                if ( ch->race == RACE_TROLL )
                    ch->alignment = -1000;
                if ( ( iLang = skill_lookup( "common" ) ) < 0 )
                    bug( "genesis: cannot find common language." );
                else
                    ch->pcdata->learned[iLang] = 100;

                if ( race_table[ch->race] ) {
                    int                     uLang;

                    for ( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ ) {
                        if ( IS_SET( race_table[ch->race]->language, 1 << iLang ) ) {
                            if ( ( uLang = skill_lookup( lang_names[iLang] ) ) < 0 )
                                bug( "%s: cannot find racial language [%s].", __FUNCTION__,
                                     lang_names[iLang] );
                            else
                                ch->pcdata->learned[uLang] = 100;
                        }
                    }
                }

                reset_colors( ch );
                addname( &ch->pcdata->chan_listen, "chat" );
                addname( &ch->pcdata->chan_listen, "decir" );
                ch->level = 1;
                ch->exp = 0;
                ch->practice = 4;
                /*
                 * Set htown data 
                 */
                {
                    HTOWN_DATA             *htown = NULL;

                    if ( ( ch )->race == RACE_HUMAN || ( ch )->race == RACE_GNOME
                         || ( ch )->race == RACE_HALFLING || ( ch )->race == RACE_PIXIE )
                        htown = get_htown( "Ciudad Paleon" );
                    if ( ( ch )->race == RACE_ELF )
                        htown = get_htown( "Ciudad Paleon" );
                    if ( ( ch )->race == RACE_DWARF )
                        htown = get_htown( "Ciudad Paleon" );
                    if ( ( ch )->race == RACE_OGRE || ( ch )->race == RACE_ORC
                         || ( ch )->race == RACE_TROLL || ( ch )->race == RACE_GOBLIN )
                        htown = get_htown( "Ciudad Dakar" );
                    if ( ( ch )->race == RACE_MINDFLAYER )
                        htown = get_htown( "ciudad Dakar" );
                    if ( ( ch )->race == RACE_CELESTIAL )
                        htown = get_htown( "Ciudad Paleon" );
                    if ( ( ch )->race == RACE_DEMON || ( ch )->race == RACE_SHADE
                         || ( ch )->race == RACE_DROW )
                        htown = get_htown( "Ciudad Dakar" );
                    if ( ( ch )->race == RACE_CENTAUR )
                        htown = get_htown( "Ciudad Paleon" );
                    if ( ( ch )->race == RACE_DRAGON ) {
                        htown = get_htown( "Ciudad Prohibida" );
                    }
                    if ( ( ch )->race == RACE_VAMPIRE ) {
                        ch->blood = ch->blood + 10;
                        ch->max_blood = ch->max_blood + 10;

                        htown = get_htown( "Ciudad Dakar" );
                    }
                    if ( htown ) {
                        if ( VLD_STR( ch->pcdata->htown_name ) )
                            STRFREE( ch->pcdata->htown_name );
                        ch->pcdata->htown_name = STRALLOC( htown->name );
                        ch->pcdata->htown = htown;
                    }
                }
                ch->max_hit = 80 + get_curr_con( ch );
                ch->max_mana += race_table[ch->race]->mana;
                ch->max_hit += race_table[ch->race]->hit;
                ch->hit = ch->max_hit;
                ch->mana = UMAX( 1, ch->max_mana );
                ch->max_move = ch->max_move + 100;
                ch->move = ch->max_move;
                ch->pcdata->condition[COND_FULL] = STATED;
                ch->pcdata->condition[COND_THIRST] = STATED;
                SET_BIT( ch->pcdata->flags, PCFLAG_DEADLY );

                if ( IS_BLOODCLASS( ch ) ) {
                    ch->max_blood = 10;
                    ch->blood = 10;
                }
                GET_MONEY( ch, CURR_COPPER ) += 1000;
                GET_MONEY( ch, CURR_BRONZE ) += 100;
                // Added a little silver and gold to newbs. -Taon
                GET_MONEY( ch, CURR_SILVER ) += 50;
                GET_MONEY( ch, CURR_GOLD ) += 10;
                /*
                 * Set player birthday to current mud day, -17 years - Samson 10-25-99
                 */
                ch->pcdata->day = time_info.day;
                ch->pcdata->month = time_info.month;
                ch->pcdata->year = time_info.year - 17;
                ch->pcdata->age = 17;
                ch->pcdata->age_bonus = 0;

                snprintf( buf, MSL, "the %s",
                          title_table[ch->Class][ch->level][ch->sex == SEX_FEMALE ? 1 : 0] );
                set_title( ch, buf );
                /*
                 * Added by Narn.  Start new characters with autoexit and autgold
                 * * already turned on.  Very few people don't use those. 
                 */
                xSET_BIT( ch->act, PLR_AUTOMONEY );
                xSET_BIT( ch->act, PLR_AUTOEXIT );
                /*
                 * Added by Brittany, Nov 24/96.  The object is the adventurer's guide
                 * * to the realms of despair, part of Academy.are. 
                 */
                if ( ( ch )->race == RACE_DRAGON ) {
                    OBJ_INDEX_DATA         *obj_ind;

                    if ( ( obj_ind = get_obj_index( 21 ) ) != NULL ) {
                        /*
                         * ball of light 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_LIGHT );
                    }
                    if ( ( obj_ind = get_obj_index( 16038 ) ) != NULL ) {
                        /*
                         *  WATER
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                    }
                    if ( ( obj_ind = get_obj_index( 16040 ) ) != NULL ) {
                        /*
                         *  FOOD
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                    }
                    if ( ( obj_ind = get_obj_index( 68 ) ) != NULL ) {
                        /*
                         * BACKPACK
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_BACK );
                    }
                }

                if ( ( ch )->race != RACE_DRAGON ) {
                    OBJ_INDEX_DATA         *obj_ind;

                    if ( ( obj_ind = get_obj_index( 67 ) ) != NULL ) {
                        /*
                         * hooded lantern 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_LIGHT );
                    }

                    if ( ( obj_ind = get_obj_index( 16038 ) ) != NULL ) {
                        /*
                         *  WATER
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                    }

                    if ( ( obj_ind = get_obj_index( 16040 ) ) != NULL ) {
                        /*
                         *  FOOD
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                    }

                    if ( ( obj_ind = get_obj_index( 4 ) ) != NULL ) {
                        /*
                         * shirt 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_BODY );
                    }
                    if ( ( obj_ind = get_obj_index( 5 ) ) != NULL ) {
                        /*
                         * pants 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_LEGS );
                    }
                    if ( ( obj_ind = get_obj_index( 6 ) ) != NULL ) {
                        /*
                         * cloak 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_ABOUT );
                    }
                    if ( ( obj_ind = get_obj_index( 27 ) ) != NULL ) {
                        /*
                         * vest 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_BODY );
                    }
                    if ( ( obj_ind = get_obj_index( 28 ) ) != NULL ) {
                        /*
                         * boots 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_FEET );
                    }
                    if ( ( obj_ind = get_obj_index( 29 ) ) != NULL ) {
                        /*
                         * BELT 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_WAIST );
                    }
                    if ( ( obj_ind = get_obj_index( 40 ) ) != NULL ) {
                        /*
                         * WRIST 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_WRIST_L );
                    }
                    if ( ( obj_ind = get_obj_index( 40 ) ) != NULL ) {
                        /*
                         * WRIST 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_WRIST_R );
                    }
                    if ( ( obj_ind = get_obj_index( 68 ) ) != NULL ) {
                        /*
                         * BACKPACK 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                        equip_char( ch, obj, WEAR_BACK );
                    }
                }
                {
                    OBJ_INDEX_DATA         *obj_ind;

                    if ( ( obj_ind = get_obj_index( 139 ) ) != NULL ) {
                        /*
                         * book of rules 
                         */
                        obj = create_object( obj_ind, 0 );
                        obj_to_char( obj, ch );
                    }
                }
                ch->pcdata->learned[gsn_slice] = 50;
                if ( ch->race == RACE_DRAGON && ch->pcdata->learned[gsn_devour] == 0 )
                    ch->pcdata->learned[gsn_devour] = 95;

                SET_BIT( d->character->pcdata->flags, PCFLAG_HINTS );
                if ( !sysdata.WAIT_FOR_AUTH )
                    char_to_room( ch, get_room_index( ch->pcdata->htown->startroom ) );
                {

                    // Following code will check through tutorial rooms, placing the 
                    // character in the next empty room "up to 3". Also makes checks
                    // to make sure the priest is in mentioned rooms, if not it will
                    // handle loading them. Bools are reset to false via mpcommand
                    // mpclear. -Taon

                    if ( ( ch )->race != RACE_OGRE && ( ch )->race != RACE_ORC
                         && ( ch )->race != RACE_TROLL && ( ch )->race != RACE_GOBLIN
                         && ( ch )->race != RACE_DEMON && ( ch )->race != RACE_SHADE
                         && ( ch )->race != RACE_DROW && ( ch )->race != RACE_VAMPIRE
                         && ( ch )->race != RACE_MINDFLAYER && ( ch )->race != RACE_DRAGON ) {
                        if ( !first_tutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START ) );
                            first_tutorial_room = TRUE;
                        }
                        else if ( !second_tutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START2 ) );
                            second_tutorial_room = TRUE;
                        }
                        else if ( !third_tutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START3 ) );
                            third_tutorial_room = TRUE;
                        }
                        else {
                            log_string
                                ( "Error: All starting tutorial rooms are currently in use." );
                            log_printf( "Error: Bad tutorial placement reached on %s. ", ch->name );
                            char_to_room( ch, get_room_index( ROOM_AUTH_START ) );
                        }
                    }
                    // EVIL RACE TUTORIAL PLACEMENT
                    else if ( ch->race != RACE_DRAGON ) {
                        if ( !first_etutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START4 ) );
                            first_etutorial_room = TRUE;
                        }
                        else if ( !second_etutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START5 ) );
                            second_etutorial_room = TRUE;
                        }
                        else if ( !third_etutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START6 ) );
                            third_etutorial_room = TRUE;
                        }
                        else {
                            log_string
                                ( "Error: All starting etutorial rooms are currently in use." );
                            log_printf( "Error: Bad etutorial placement reached on %s. ",
                                        ch->name );
                            char_to_room( ch, get_room_index( ROOM_AUTH_START4 ) );
                        }
                    }

                    // dragon race tutorial placement
                    else {
                        if ( !first_dtutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START7 ) );
                            first_dtutorial_room = TRUE;
                        }
                        else if ( !second_dtutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START8 ) );
                            second_dtutorial_room = TRUE;
                        }
                        else if ( !third_dtutorial_room ) {
                            char_to_room( ch, get_room_index( ROOM_AUTH_START9 ) );
                            third_dtutorial_room = TRUE;
                        }
                        else {
                            log_string
                                ( "Error: All starting tutorial rooms are currently in use." );
                            log_printf( "Error: Bad tutorial placement reached on %s. ", ch->name );
                            char_to_room( ch, get_room_index( ROOM_AUTH_START7 ) );
                        }
                    }

                    short                   mob_count = 0;
                    CHAR_DATA              *vch;
                    CHAR_DATA              *vch_next;
                    CHAR_DATA              *priest;

                    for ( vch = ch->in_room->first_person; vch; vch = vch_next ) {
                        vch_next = vch->next_in_room;

                        if ( !vch )
                            break;
                        if ( IS_NPC( vch ) )
                            mob_count++;
                    }
                    if ( mob_count == 0 ) {
                        if ( ( ch )->race == RACE_OGRE || ( ch )->race == RACE_ORC
                             || ( ch )->race == RACE_TROLL || ( ch )->race == RACE_GOBLIN
                             || ( ch )->race == RACE_DEMON || ( ch )->race == RACE_SHADE
                             || ( ch )->race == RACE_DROW || ( ch )->race == RACE_VAMPIRE
                             || ( ch )->race == RACE_MINDFLAYER ) {
                            priest = create_mobile( get_mob_index( 33200 ) );
                            char_to_room( priest, ch->in_room );
                        }
                        else if ( ( ch )->race == RACE_DRAGON ) {
                            priest = create_mobile( get_mob_index( 19007 ) );
                            char_to_room( priest, ch->in_room );
                        }
                        else if ( ( ch )->race == RACE_HUMAN || ( ch )->race == RACE_ELF
                                  || ( ch )->race == RACE_DWARF || ( ch )->race == RACE_HALFLING
                                  || ( ch )->race == RACE_PIXIE || ( ch )->race == RACE_GNOME
                                  || ( ch )->race == RACE_CENTAUR
                                  || ( ch )->race == RACE_CELESTIAL ) {
                            priest = create_mobile( get_mob_index( 5101 ) );
                            char_to_room( priest, ch->in_room );
                        }
                    }
                    ch->pcdata->auth_state = 0;
                    SET_BIT( ch->pcdata->flags, PCFLAG_UNAUTHED );
                    add_to_auth( ch );                 /* new auth */
                }
            }
            else if ( !IS_IMMORTAL( ch ) && ch->pcdata->release_date > 0
                      && ch->pcdata->release_date > current_time ) {
                if ( ch->in_room->vnum == 6 || ch->in_room->vnum == 8 || ch->in_room->vnum == 1206 )
                    char_to_room( ch, ch->in_room );
                else
                    char_to_room( ch, get_room_index( 8 ) );
            }
            else if ( ch->in_room && !IS_IMMORTAL( ch )
                      && ( IS_SET( ch->in_room->room_flags, ROOM_QUEST )
                           || IS_SET( ch->in_room->area->flags, AFLAG_QUEST ) ) ) {
                ROOM_INDEX_DATA        *location = NULL;

                if ( !IS_NPC( ch ) && ch->pcdata->lair )
                    location = get_room_index( ch->pcdata->lair );
                if ( !IS_NPC( ch ) && ch->pcdata->htown )
                    if ( ch->pcdata->lair < 1 && !ch->pcdata->clan )
                        location = get_room_index( ch->pcdata->htown->recall );
                if ( !IS_IMMORTAL( ch ) ) {
                    if ( !IS_NPC( ch ) && ch->pcdata->clan && ch->pcdata->lair < 1 )
                        location = get_room_index( ch->pcdata->clan->recall );
                    if ( !IS_NPC( ch ) && !location && ch->level >= 5
                         && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) && ch->pcdata->lair < 1 )
                        location = get_room_index( ch->pcdata->htown->recall );
                }
                else if ( IS_IMMORTAL( ch ) )
                    location = get_room_index( ROOM_VNUM_CHAT );

                if ( !location && !IS_NPC( ch ) )
                    location = get_room_index( ch->pcdata->htown->recall );
                char_to_room( ch, location );
            }
            else if ( ch->in_room && ( IS_IMMORTAL( ch )
                                       || !IS_SET( ch->in_room->room_flags, ROOM_PROTOTYPE ) ) )
                char_to_room( ch, ch->in_room );
            else if ( ch->in_room && !IS_IMMORTAL( ch )
                      && IS_SET( ch->in_room->room_flags, ROOM_PROTOTYPE ) )
                char_to_room( ch, get_room_index( ch->pcdata->htown->recall ) );
            else if ( IS_IMMORTAL( ch ) )
                char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
            else if ( !IS_IMMORTAL( ch ) && !ch->pcdata->clan )
                char_to_room( ch, get_room_index( ch->pcdata->htown->recall ) );

            if ( get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
                remove_timer( ch, TIMER_SHOVEDRAG );
            if ( get_timer( ch, TIMER_PKILLED ) > 0 )
                remove_timer( ch, TIMER_PKILLED );
            if ( ( ch )->race == RACE_OGRE || ( ch )->race == RACE_ORC || ( ch )->race == RACE_TROLL
                 || ( ch )->race == RACE_GOBLIN || ( ch )->race == RACE_SHADE
                 || ( ch )->race == RACE_DEMON || ( ch )->race == RACE_DROW ) {
                HTOWN_DATA             *htown = NULL;

                htown = get_htown( "Dakar City" );
            }
            if ( ( ch )->race == RACE_DRAGON ) {
                HTOWN_DATA             *htown = NULL;

                htown = get_htown( "Ciudad Prohibida" );
                if ( htown ) {
                    if ( VLD_STR( ch->pcdata->htown_name ) )
                        STRFREE( ch->pcdata->htown_name );
                    ch->pcdata->htown_name = STRALLOC( htown->name );
                    ch->pcdata->htown = htown;
                }

            }

            if ( ch->race == RACE_DRAGON || ch->Class == CLASS_DRAGONLORD ) {
                ch->pcdata->tradeclass = 0;
                ch->pcdata->tradelevel = 0;
                if ( ch->pcdata->learned[gsn_mine] > 0 )
                    ch->pcdata->learned[gsn_mine] = 0;
                if ( ch->pcdata->learned[gsn_gather] > 0 )
                    ch->pcdata->learned[gsn_gather] = 0;
                if ( ch->pcdata->learned[gsn_forge] > 0 )
                    ch->pcdata->learned[gsn_forge] = 0;
                if ( ch->pcdata->learned[gsn_bake] > 0 )
                    ch->pcdata->learned[gsn_bake] = 0;
                if ( ch->pcdata->learned[gsn_mix] > 0 )
                    ch->pcdata->learned[gsn_mix] = 0;
                if ( ch->pcdata->learned[gsn_hunt] > 0 )
                    ch->pcdata->learned[gsn_hunt] = 0;
                if ( ch->pcdata->learned[gsn_tan] > 0 )
                    ch->pcdata->learned[gsn_tan] = 0;
                if ( ch->pcdata->learned[gsn_jewelry] > 0 )
                    ch->pcdata->learned[gsn_jewelry] = 0;
                if ( ch->pcdata->learned[gsn_fell] > 0 )
                    ch->pcdata->learned[gsn_fell] = 0;
                if ( ch->pcdata->learned[gsn_mill] > 0 )
                    ch->pcdata->learned[gsn_mill] = 0;
            }

            if ( ch->level == 1 && ch->Class == CLASS_BEAST && ch->race == RACE_DROW )
              {
                        if ( ch->pcdata->title )
                            STRFREE( ch->pcdata->title );
                        ch->pcdata->title = STRALLOC( " the animal whisperer of Dakar City." );

              }
            if ( ch->level == 1 && ch->Class == CLASS_BEAST && ch->race == RACE_ELF )
              {
                        if ( ch->pcdata->title )
                            STRFREE( ch->pcdata->title );
                        ch->pcdata->title = STRALLOC( "el habitante de ciudad Paleon que susurra a los animales." );

              }

            OBJ_DATA               *deed;

            for ( deed = ch->first_carrying; deed; deed = deed->next_content ) {
                if ( deed->pIndexData->vnum == OBJ_VNUM_DEED && !str_cmp( deed->owner, ch->name ) )
                    break;
            }

            if ( ch->pcdata->ship > 0 && !deed ) {
                ROOM_INDEX_DATA        *location = NULL;

                // Send them back to dock
                ch->pcdata->ship = 0;
                ch->pcdata->watervnum = 0;
                ch->pcdata->boatvnum = 0;
                ch->pcdata->direction = 0;
                location = get_room_index( ROOM_DOCK );
                char_to_room( ch, location );
            }

            act( AT_ACTION, "$n ha entrado al juego.", ch, NULL, NULL, TO_CANSEE );
            send_to_char( "\r\n\r\n", ch );            // a spacer?
            checkBuidty( ch );
            get_curr_players(  );

            if ( IS_SET( ch->pcdata->flags, PCFLAG_UNAUTHED ) ) {
                ch->pcdata->auth_state = 0;
                add_to_auth( ch );                     /* new auth */
                auth_update(  );
            }

            do_news( ch, ( char * ) "last" );
            do_boards( ch, ( char * ) "" );
            check_auction( ch );

            if ( IS_CLANNED( ch ) ) {
                CLAN_DATA              *clan;
                ROSTER_DATA            *ros;
                int                     chieftimer;
                bool                    setchieftain = FALSE,
                    setwarmaster = FALSE;

                clan = ch->pcdata->clan;

                /*
                 * Send the clan intro here since some think they need more spam lol 
                 */
                if ( VLD_STR( clan->intro ) ) {
                    send_to_char( "Mensaje del día del clan...\r\n", ch );
                    ch_printf( ch, "%s\r\n", clan->intro );
                }
                if ( !VLD_STR( clan->chieftain ) )
                    setchieftain = TRUE;
                if ( !VLD_STR( clan->warmaster ) )
                    setwarmaster = TRUE;
                for ( ros = clan->first_member; ros; ros = ros->next ) {
                    if ( !setchieftain && VLD_STR( clan->chieftain )
                         && !str_cmp( clan->chieftain, ros->name ) ) {
                        chieftimer = ( ( current_time - ros->lastupdated ) / 86400 );
                        if ( chieftimer >= sysdata.clanlog )
                            setchieftain = TRUE;
                    }
                    if ( !setwarmaster && VLD_STR( clan->warmaster )
                         && !str_cmp( clan->warmaster, ros->name ) ) {
                        chieftimer = ( ( current_time - ros->lastupdated ) / 86400 );
                        if ( chieftimer >= sysdata.clanlog )
                            setwarmaster = TRUE;
                    }
                }

                /*
                 * Actually take the time to set them 
                 */
                /*
                 * Only set if need to set a new one, ch isn't immortal and not the warmaster 
                 */
                if ( !IS_IMMORTAL( ch ) ) {
                    if ( setchieftain && ( !VLD_STR( clan->warmaster )
                                           || str_cmp( clan->warmaster, ch->name ) ) ) {
                        if ( clan->chieftain )
                            STRFREE( clan->chieftain );
                        clan->chieftain = STRALLOC( ch->name );
                        save_clan( clan );
                        snprintf( buf, MIL,
                                  "&C¡%s ha tomado el mando del clan %s!\r\n",
                                  ch->name, clan->name );
            announce( buf );
                    }
                    /*
                     * Only set if need to set a new one, ch isn't immortal and not the chieftain 
                     */
                    else if ( setwarmaster && ( !VLD_STR( clan->chieftain )
                                                || str_cmp( clan->chieftain, ch->name ) ) ) {
                        if ( clan->warmaster )
                            STRFREE( clan->warmaster );
                        clan->warmaster = STRALLOC( ch->name );
                        save_clan( clan );
                        if ( str_cmp( clan->name, "Halcyon" ) )
                            snprintf( buf, MIL,
                                      "&W[&RAnuncio&W]&C %s ha tomado el mando del maestro de guerra en el clan %s!\r\n",
                                      ch->name, clan->name );
                        else
                            snprintf( buf, MIL,
                                      "&C¡%s ahora es embajador de %s!\r\n",
                                      ch->name, clan->name );
            announce( buf );

                    }
                }

                {
                    CLAN_DATA              *influenced = ch->in_room->area->influencer;

                    if ( influenced && influenced != ch->pcdata->clan ) {
                        if ( !str_cmp( ch->pcdata->clan_name, "throng" ) ) {
                            if ( clan->status == 1 )
                                ch_printf( ch,
                                           "\r\n\r\n&g[Gritar] Barakus, líder de la Multitud '%s a la batalla! ¡Destrozaremos la Alianza!'\r\n\r\n",
                                           ch->name );
                            else if ( clan->status == 2 )
                                ch_printf( ch,
                                           "\r\n\r\n&g[Gritar] Barakus, líder de la Multitud '¡%s odiará a la Alianza sin atacarla mientras maniobra sus fuerzas!'\r\n\r\n",
                                           ch->name );
                            else if ( clan->status == 3 )
                                ch_printf( ch,
                                           "\r\n\r\n&g[Gritar] Barakus, líder de la Multitud '%s no destrozará a la Alianza por ahora!'\r\n\r\n",
                                           ch->name );
                        }
                        else if ( !str_cmp( ch->pcdata->clan_name, "alliance" ) ) {
                            if ( clan->status == 1 )
                                ch_printf( ch,
                                           "\r\n\r\n&g[Gritar] Rodaen, líder de la Alianza '%s a la batalla!  ¡Combatiremos a la Multitud!'\r\n\r\n",
                                           ch->name );
                            else if ( clan->status == 2 )
                                ch_printf( ch,
                                           "\r\n\r\n&g[Gritar] Rodaen, líder de la Alianza '%s a la batalla!  ¡Odiamos a la Alianza!'\r\n\r\n",
                                           ch->name );
                            else if ( clan->status == 3 )
                                ch_printf( ch,
                                           "\r\n\r\n&g[Gritar] Rodaen, líder de la alianza '%s a la batalla!  ¡Hay paz con la Multitud!'\r\n\r\n",
                                           ch->name );
                        }
                    }
                }
            }

            if ( IS_CITY( ch ) ) {
                CITY_DATA              *city;
                ROLLCALL_DATA          *roll;
                int                     duketimer;
                bool                    fixcityorder = FALSE,
                    shouldsavecity = FALSE;

                city = ch->pcdata->city;
                if ( !VLD_STR( city->duke ) || !VLD_STR( city->baron ) || !VLD_STR( city->captain )
                     || !VLD_STR( city->sheriff ) )
                    fixcityorder = TRUE;
                for ( roll = city->first_citizen; roll; roll = roll->next ) {
                    if ( VLD_STR( city->duke ) && !str_cmp( city->duke, roll->name ) ) {
                        duketimer = ( ( current_time - roll->lastupdated ) / 86400 );
                        if ( duketimer >= sysdata.clanlog ) {
                            STRFREE( city->duke );
                            fixcityorder = TRUE;
                        }
                    }
                    if ( VLD_STR( city->baron ) && !str_cmp( city->baron, roll->name ) ) {
                        duketimer = ( ( current_time - roll->lastupdated ) / 86400 );
                        if ( duketimer >= sysdata.clanlog ) {
                            STRFREE( city->baron );
                            fixcityorder = TRUE;
                        }
                    }
                }

                if ( fixcityorder ) {                  /* Go ahead and redo the order */
                    fix_city_order( city );
                    shouldsavecity = TRUE;
                }

                /*
                 * Actually take the time to set them 
                 * Only set if need to set a new one, ch isn't immortal and not the baron 
                 */
                if ( !IS_IMMORTAL( ch ) ) {
                    /*
                     * Fix the order first, to remove any unneeded people, move people up 
                     * etc 
                     */
                    fix_city_order( city );
                    if ( !has_city_position( city, ch->name ) && !VLD_STR( city->knight ) ) {
                        STRFREE( city->knight );
                        city->knight = STRALLOC( ch->name );
                        save_city( city );
                        snprintf( buf, MIL,
                                  "&C¡%s ahora es caballero de %s!",
                                  city->knight, city->name );
announce( buf );
                    }
                    /*
                     * Fix the order again, to remove any unneeded people, move people up 
                     * etc 
                     */
                    fix_city_order( city );
                }
            }

            if ( xIS_SET( d->character->act, PLR_AFK ) )
                xREMOVE_BIT( ch->act, PLR_AFK );

            if ( xIS_SET( ch->act, PLR_TUTORIAL ) && ch->level > 1 ) {
                xREMOVE_BIT( ch->act, PLR_TUTORIAL );
            }

            if ( ch->level < 2 ) {
                xSET_BIT( ch->act, PLR_AUTODOOR );
            }

/* Volk: Set their default text speed, if they don't have one yet. */
            if ( !ch->pcdata->textspeed )
                ch->pcdata->textspeed = 5;

/* Volk: Make sure no skills are higher than they should be! */
            int                     sn;

            for ( sn = 0; sn < top_sn; sn++ ) {
                if ( ch->pcdata->learned[sn] > get_maxadept( ch, sn, TRUE ) )
                    ch->pcdata->learned[sn] = get_maxadept( ch, sn, TRUE );
            }
/* Volk: Bring back pets.. */
            if ( ch->pcdata->pet ) {
                act( AT_ACTION, "$n aparece de la nada con su dueño.", ch->pcdata->pet, NULL, ch,
                     TO_NOTVICT );
                act( AT_ACTION, "$N regresa contigo a los reinos.", ch, NULL, ch->pcdata->pet,
                     TO_CHAR );
            }

            if ( ch->level > 1 ) {
                if ( happyhouron || sysdata.happy ) {
                    send_to_char( "&YEs la hora feliz en 6Dragones!\r\n", ch );
                    send_to_char
                        ( "&RDisfruta de una hora de doble experiencia al luchar, doble experiencia al fabricar o triple experiencia si agrupas con otro jugador.\r\n",
                          ch );
                }
            }

            update_cities(  );

/* Volk: Remove puppet flag if still there  */
            if ( IS_PUPPET( ch ) )
                REMOVE_BIT( ch->pcdata->flags, PCFLAG_PUPPET );

/* Volk: Prevent lowbies from getting yucky colors mixed up. */
            if ( ch->level < 2 )
                interpret( ch, ( char * ) "color _reset_" );

            if ( VLD_STR( ch->pcdata->city_name ) ) {
                SIEGE_DATA             *siege = NULL;
                CITY_DATA              *ucity = NULL;

                if ( ( ucity = get_city( ch->pcdata->city_name ) ) ) {
                    for ( siege = first_siege; siege; siege = siege->next ) {
                        if ( ucity == siege->acity )
                            ch_printf( ch, "Tu ciudad está atqacando %s.\r\n", siege->dcity->name );
                        if ( ucity == siege->dcity )
                            ch_printf( ch, "Tu ciudad está siendo atacada por %s.\r\n",
                                       siege->acity->name );
                    }
                }
            }

            if ( ch->position == POS_MOUNTED )
                ch->position = POS_STANDING;

            do_look( ch, ( char * ) "auto" );
            check_auth_state( ch );                    /* new auth */
            if ( ch->pcdata->htown ) {
                if ( !ch->was_in_room
                     && ch->in_room == get_room_index( ch->pcdata->htown->recall ) )
                    ch->was_in_room = get_room_index( ch->pcdata->htown->recall );
                else if ( ch->was_in_room == get_room_index( ch->pcdata->htown->recall ) )
                    ch->was_in_room = get_room_index( ch->pcdata->htown->recall );
                else if ( !ch->was_in_room )
                    ch->was_in_room = ch->in_room;
            }
            else {
                if ( !ch->was_in_room && ch->in_room == get_room_index( ROOM_VNUM_TEMPLE ) )
                    ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );
                else if ( ch->was_in_room == get_room_index( ROOM_VNUM_TEMPLE ) )
                    ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );
                else if ( !ch->was_in_room )
                    ch->was_in_room = ch->in_room;
            }
            break;

    }
    return;
}

void do_delete( CHAR_DATA *ch, char *argument )
{

    if ( IS_NPC( ch ) ) {
        send_to_char( "Yeah, right. Mobs can't delete themselves.\r\n", ch );
        return;
    }

    if ( ch->fighting != NULL ) {
        send_to_char( "Termina antes de pelear.\r\n", ch );
        return;
    }

    /*
     *    * Reimbursement warning added to code by Samson 1-18-98
     *       */
    set_char_color( AT_YELLOW, ch );
    send_to_char( "Recuerda, la decisión no tiene vuelta atrás. ¡No recuperamos fichas borradas!\r\n", ch );

    /*
     *    * Immortals warning added to code by Samson 1-18-98
     *       */
    if ( IS_IMMORTAL( ch ) ) {
        ch_printf( ch,
                   "Considéralo bien, %s, si te borras, no serás\r\nrestituído en la administración!\r\n",
                   ch->name );
        send_to_char( "Se perderá la área que estás construyendo si procedes.\r\n", ch );
    }
    set_char_color( AT_RED, ch );
    send_to_char( "\r\nTeclea tu contraseña si quieres borrar tu personaje.\r\n", ch );
    send_to_char( "[borrar] contraseña: ", ch );
    write_to_buffer( ch->desc, echo_off_str, 0 );
    ch->desc->connected = CON_DELETE;
}
