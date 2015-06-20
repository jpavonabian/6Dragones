 /****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 *			     Special clan module			    *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "h/mud.h"
#include "h/files.h"
#include "h/clans.h"
#include "h/key.h"
#include "h/languages.h"
#include <time.h>

#define MAX_NEST	100
static OBJ_DATA        *rgObjNest[MAX_NEST];

CLAN_DATA              *first_clan;
CLAN_DATA              *last_clan;
COUNCIL_DATA           *first_council;
COUNCIL_DATA           *last_council;

bool                    valid_pfile( const char *filename );
void                    influence_areas( CLAN_DATA * clan );

/* local routines */
void fread_clan         args( ( CLAN_DATA * clan, FILE * fp ) );
bool load_clan_file     args( ( const char *clanfile ) );
void write_clan_list    args( ( void ) );
void fread_council      args( ( COUNCIL_DATA * council, FILE * fp ) );
bool load_council_file  args( ( const char *councilfile ) );
void write_council_list args( ( void ) );
void                    check_clan_leaders( CLAN_DATA * clan, char *player );

/* Sorted by level and date updated */
void insert_roster( CLAN_DATA * clan, ROSTER_DATA * roster )
{
    ROSTER_DATA            *uroster;
    int                     daydiff,
                            odaydiff;

    for ( uroster = clan->first_member; uroster; uroster = uroster->next ) {
        odaydiff = ( current_time - uroster->lastupdated ) / 86400;
        daydiff = ( current_time - roster->lastupdated ) / 86400;

        if ( odaydiff < 3 && uroster->level > roster->level )
            continue;
        /*
         * So if level is the same or they havent logged in for 3 days just insert here 
         */
        INSERT( roster, uroster, clan->first_member, next, prev );
        return;
    }
    LINK( roster, clan->first_member, clan->last_member, next, prev );
}

void add_roster( CLAN_DATA * clan, char *name, char *race, int level, int pkills, int kills,
                 int deaths, int tradeclass, int tradelevel )
{
    ROSTER_DATA            *roster;

    CREATE( roster, ROSTER_DATA, 1 );
    roster->name = STRALLOC( name );
    roster->race = STRALLOC( race );
    roster->level = level;
    roster->pkills = pkills;
    roster->kills = kills;
    roster->deaths = deaths;
    roster->joined = current_time;
    roster->lastupdated = current_time;
    roster->tradeclass = tradeclass;
    roster->tradelevel = tradelevel;
    insert_roster( clan, roster );
}

void remove_roster( CLAN_DATA * clan, char *name )
{
    ROSTER_DATA            *roster;

    for ( roster = clan->first_member; roster; roster = roster->next ) {
        if ( !str_cmp( name, roster->name ) ) {
            STRFREE( roster->name );
            STRFREE( roster->race );
            UNLINK( roster, clan->first_member, clan->last_member, next, prev );
            DISPOSE( roster );
            return;
        }
    }
}

/* Remove them from any roster they are on */
void remove_from_rosters( char *name )
{
    CLAN_DATA              *clan;
    ROSTER_DATA            *roster,
                           *roster_next;

    for ( clan = first_clan; clan; clan = clan->next ) {
        for ( roster = clan->first_member; roster; roster = roster_next ) {
            roster_next = roster->next;
            if ( !str_cmp( roster->name, name ) ) {
                STRFREE( roster->name );
                STRFREE( roster->race );
                UNLINK( roster, clan->first_member, clan->last_member, next, prev );
                DISPOSE( roster );
            }
        }
        save_clan( clan );
    }
}

/* When we update the roster move the roster to the first of the list */
void update_roster( CHAR_DATA *ch )
{
    CLAN_DATA              *clan;
    ROSTER_DATA            *roster;

    if ( !ch || !ch->pcdata || !( clan = ch->pcdata->clan ) )
        return;
    for ( roster = clan->first_member; roster; roster = roster->next ) {
        if ( !str_cmp( ch->name, roster->name ) ) {
            roster->race = STRALLOC( capitalize( race_table[ch->race]->race_name ) );
            roster->level = ch->level;
            roster->pkills = ch->pcdata->pkills;
            roster->kills = ch->pcdata->mkills;
            roster->deaths = ch->pcdata->mdeaths;
            roster->lastupdated = current_time;
            roster->tradelevel = ch->pcdata->tradelevel;
            roster->tradeclass = ch->pcdata->tradeclass;
            UNLINK( roster, clan->first_member, clan->last_member, next, prev );
            insert_roster( clan, roster );
            save_clan( ch->pcdata->clan );
            return;
        }
    }

    /*
     * If we make it here, assume they haven't been added previously 
     */
    add_roster( clan, ch->name, capitalize( race_table[ch->race]->race_name ), ch->level,
                ch->pcdata->pkills, ch->pcdata->mkills, ch->pcdata->mdeaths, ch->pcdata->tradeclass,
                ch->pcdata->tradelevel );
    save_clan( clan );
}

/* For use during clan removal and memory cleanup */
void remove_all_rosters( CLAN_DATA * clan )
{
    ROSTER_DATA            *roster,
                           *roster_next;

    for ( roster = clan->first_member; roster; roster = roster_next ) {
        roster_next = roster->next;

        STRFREE( roster->name );
        STRFREE( roster->race );
        UNLINK( roster, clan->first_member, clan->last_member, next, prev );
        DISPOSE( roster );
    }
}

void do_roster( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA              *clan;
    ROSTER_DATA            *roster;
    char                    arg[MAX_INPUT_LENGTH],
                            arg2[MAX_INPUT_LENGTH];
    int                     total = 0;

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mobs no, gracias.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Uso: miembros <clan>\r\n", ch );
        if ( IS_IMMORTAL( ch ) || IS_CHIEFTAIN( ch ) || IS_WARMASTER( ch ) )
            send_to_char( "Uso: miembros <clan> remove <nombre>\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( !( clan = get_clan( arg ) ) ) {
        ch_printf( ch, "No existe ningún %s\r\n", arg );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        ch_printf( ch, "&cMiembros del clan %s\r\r\n\n", clan->name );
        ch_printf( ch, "%-15.15s  %15.15s %6.6s %10.10s %6.6s %6.6s %6.6s %6.6s\r\n", "Nombre",
                   "Raza", "Nivel", "FClase", "FNivel", "PKs", "asesinatos", "Muertes" );
        send_to_char
            ( "------------------------------------------------------------------------------&C\r\n",
              ch );
        for ( roster = clan->first_member; roster; roster = roster->next ) {
            if ( roster->level > 100 )
                continue;
            ch_printf( ch, "%-15.15s  %15.15s %6d %10s %6d %6d %6d %6d\r\n",
                       roster->name, roster->race, roster->level,
                       roster->tradeclass == 20 ? "herrero" : roster->tradeclass ==
                       21 ? "Panadero" : roster->tradeclass == 22 ? "Curtidor" :
                       roster->tradeclass == 24 ? "Joyero" :
                       roster->tradeclass == 25 ? "Carpintero" : "",
                       roster->tradelevel >= 0 ? roster->tradelevel : 0, roster->pkills,
                       roster->kills, roster->deaths );
            total++;
        }
        ch_printf( ch, "\r\n&cHay &C%d &cmiembro%s en &C%s\r\n", total, total == 1 ? "" : "s",
                   clan->name );
        return;
    }

    argument = one_argument( argument, arg2 );

    if ( IS_IMMORTAL( ch )
         || ( VLD_STR( clan->chieftain ) && !str_cmp( ch->name, clan->chieftain ) )
         || ( VLD_STR( clan->warmaster ) && !str_cmp( ch->name, clan->warmaster ) ) ) {
        if ( !str_cmp( arg2, "remove" ) ) {
            if ( !argument || argument[0] == '\0' ) {
                send_to_char( "Remove who from the roster?\r\n", ch );
                return;
            }
            remove_roster( clan, argument );
            save_clan( clan );
            ch_printf( ch, "%s has been removed from the roster for %s\r\n", argument, clan->name );
            return;
        }
    }

    do_roster( ch, ( char * ) "" );
}

void fwrite_memberlist( FILE * fp, CLAN_DATA * clan )
{
    ROSTER_DATA            *roster;

    for ( roster = clan->first_member; roster; roster = roster->next ) {
        if ( !valid_pfile( roster->name ) )
            continue;
        fprintf( fp, "%s", "#ROSTER\n" );
        fprintf( fp, "Name       %s~\n", roster->name );
        fprintf( fp, "Joined     %ld\n", ( time_t ) roster->joined );
        fprintf( fp, "Updated    %ld\n", ( time_t ) roster->lastupdated );
        fprintf( fp, "Race       %s~\n", roster->race );
        fprintf( fp, "Level      %d\n", roster->level );
        fprintf( fp, "PKills     %d\n", roster->pkills );
        fprintf( fp, "Kills      %d\n", roster->kills );
        fprintf( fp, "Deaths     %d\n", roster->deaths );
        fprintf( fp, "TradeClass %d\n", roster->tradeclass );
        fprintf( fp, "TradeLevel %d\n", roster->tradelevel );
        fprintf( fp, "%s", "End\n\n" );
    }
}

void fread_memberlist( CLAN_DATA * clan, FILE * fp )
{
    ROSTER_DATA            *roster;
    const char             *word;
    bool                    fMatch;

    CREATE( roster, ROSTER_DATA, 1 );
    roster->lastupdated = current_time;
    roster->tradeclass = -1;
    roster->tradelevel = -1;
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'D':
                KEY( "Deaths", roster->deaths, fread_number( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    LINK( roster, clan->first_member, clan->last_member, next, prev );
                    return;
                }
                break;

            case 'J':
                KEY( "Joined", roster->joined, fread_number( fp ) );
                break;

            case 'K':
                KEY( "Kills", roster->kills, fread_number( fp ) );
                break;

            case 'L':
                KEY( "Level", roster->level, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", roster->name, fread_string( fp ) );
                break;

            case 'P':
                KEY( "PKills", roster->pkills, fread_number( fp ) );
                break;

            case 'R':
                KEY( "Race", roster->race, fread_string( fp ) );
                break;

            case 'T':
                KEY( "TradeClass", roster->tradeclass, fread_number( fp ) );
                KEY( "TradeLevel", roster->tradelevel, fread_number( fp ) );
                break;

            case 'U':
                KEY( "Updated", roster->lastupdated, fread_number( fp ) );
                break;
        }
        if ( !fMatch )
            bug( "%s: no match: %s", __FUNCTION__, word );
    }
}

void free_pkillarea( PKILLAREA_DATA * pkillarea )
{
    STRFREE( pkillarea->name );
    DISPOSE( pkillarea );
}

void free_one_clan( CLAN_DATA * clan )
{
    PKILLAREA_DATA         *pkillarea,
                           *pkillarea_next;
    AREA_DATA              *tarea;

    UNLINK( clan, first_clan, last_clan, next, prev );
    STRFREE( clan->filename );
    STRFREE( clan->name );
    STRFREE( clan->motto );
    STRFREE( clan->description );
    STRFREE( clan->warmaster );
    STRFREE( clan->chieftain );
    STRFREE( clan->badge );
    STRFREE( clan->intro );
    remove_all_rosters( clan );
    for ( pkillarea = clan->first_pkillarea; pkillarea; pkillarea = pkillarea_next ) {
        pkillarea_next = pkillarea->next;
        UNLINK( pkillarea, clan->first_pkillarea, clan->last_pkillarea, next, prev );
        free_pkillarea( pkillarea );
    }
    for ( tarea = first_area; tarea; tarea = tarea->next ) {
        if ( tarea->influencer == clan )
            tarea->influencer = NULL;
    }
    DISPOSE( clan );
}

void free_clans( void )
{
    CLAN_DATA              *clan,
                           *clan_next;

    for ( clan = first_clan; clan; clan = clan_next ) {
        clan_next = clan->next;
        free_one_clan( clan );
    }
}

void free_one_council( COUNCIL_DATA * council )
{
    UNLINK( council, first_council, last_council, next, prev );
    STRFREE( council->description );
    STRFREE( council->filename );
    STRFREE( council->head );
    STRFREE( council->head2 );
    STRFREE( council->name );
    STRFREE( council->powers );
    DISPOSE( council );
}

void free_councils( void )
{
    COUNCIL_DATA           *council,
                           *council_next;

    for ( council = first_council; council; council = council_next ) {
        council_next = council->next;
        free_one_council( council );
    }
}

/*
 * Get pointer to clan structure from clan name.
 */
CLAN_DATA              *get_clan( const char *name )
{
    CLAN_DATA              *clan;

    for ( clan = first_clan; clan; clan = clan->next )
        if ( !str_cmp( name, clan->name ) )
            return clan;
    return NULL;
}

COUNCIL_DATA           *get_council( const char *name )
{
    COUNCIL_DATA           *council;

    for ( council = first_council; council; council = council->next )
        if ( !str_cmp( name, council->name ) )
            return council;
    return NULL;
}

void write_clan_list(  )
{
    CLAN_DATA              *tclan;
    FILE                   *fpout;
    char                    filename[256];

    snprintf( filename, 256, "%s%s", CLAN_DIR, CLAN_LIST );
    fpout = FileOpen( filename, "w" );
    if ( !fpout ) {
        bug( "FATAL: cannot open %s for writing!\r\n", filename );
        return;
    }
    for ( tclan = first_clan; tclan; tclan = tclan->next )
        fprintf( fpout, "%s\n", tclan->filename );
    fprintf( fpout, "$\n" );
    FileClose( fpout );
}

void write_council_list(  )
{
    COUNCIL_DATA           *tcouncil;
    FILE                   *fpout;
    char                    filename[256];

    snprintf( filename, 256, "%s%s", COUNCIL_DIR, COUNCIL_LIST );
    fpout = FileOpen( filename, "w" );
    if ( !fpout ) {
        bug( "FATAL: cannot open %s for writing!\r\n", filename );
        return;
    }
    for ( tcouncil = first_council; tcouncil; tcouncil = tcouncil->next )
        fprintf( fpout, "%s\n", tcouncil->filename );
    fprintf( fpout, "$\n" );
    FileClose( fpout );
}

/*
 * Save a clan's data to its data file
 */
void save_clan( CLAN_DATA * clan )
{
    FILE                   *fp;
    char                    filename[256];
    PKILLAREA_DATA         *pkillarea = NULL;

    if ( !clan ) {
        bug( "%s", "save_clan: null clan pointer!" );
        return;
    }

    if ( !clan->filename || clan->filename[0] == '\0' ) {
        bug( "save_clan: %s has no filename", clan->name );
        return;
    }

    snprintf( filename, 256, "%s%s", CLAN_DIR, clan->filename );

    if ( ( fp = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "save_clan: can't open %s", filename );
        perror( filename );
    }
    else {
        fprintf( fp, "#CLAN\n" );
        fprintf( fp, "Name         %s~\n", clan->name );
        fprintf( fp, "Filename     %s~\n", clan->filename );
        fprintf( fp, "Intro        %s~\n", clan->intro );
        fprintf( fp, "Motto        %s~\n", clan->motto );
        fprintf( fp, "Description  %s~\n", clan->description );
        fprintf( fp, "Warmaster    %s~\n", clan->warmaster );
        fprintf( fp, "Chieftain    %s~\n", clan->chieftain );
        fprintf( fp, "Badge        %s~\n", clan->badge );
        fprintf( fp, "PKillRangeNew   %d %d %d %d %d %d %d %d %d %d %d\n",
                 clan->pkills[0], clan->pkills[1], clan->pkills[2], clan->pkills[3],
                 clan->pkills[4], clan->pkills[5], clan->pkills[6], clan->pkills[7],
                 clan->pkills[8], clan->pkills[9], clan->pkills[10] );
        fprintf( fp, "PDeathRangeNew  %d %d %d %d %d %d %d %d %d %d %d\n", clan->pdeaths[0],
                 clan->pdeaths[1], clan->pdeaths[2], clan->pdeaths[3], clan->pdeaths[4],
                 clan->pdeaths[5], clan->pdeaths[6], clan->pdeaths[7], clan->pdeaths[8],
                 clan->pdeaths[9], clan->pdeaths[10] );
        fprintf( fp, "MKills       %d\n", clan->mkills );
        fprintf( fp, "MDeaths      %d\n", clan->mdeaths );
        fprintf( fp, "IllegalPK    %d\n", clan->illegal_pk );
        fprintf( fp, "Score        %d\n", clan->score );
        fprintf( fp, "Type         %d\n", clan->clan_type );
        fprintf( fp, "Members      %d\n", clan->members );
        fprintf( fp, "MemLimit     %d\n", clan->mem_limit );
        fprintf( fp, "Alignment    %d\n", clan->alignment );
        fprintf( fp, "Board        %d\n", clan->board );
        fprintf( fp, "ClanObjOne   %d\n", clan->clanobj1 );
        fprintf( fp, "ClanObjTwo   %d\n", clan->clanobj2 );
        fprintf( fp, "ClanObjThree %d\n", clan->clanobj3 );
        fprintf( fp, "ClanObjFour  %d\n", clan->clanobj4 );
        fprintf( fp, "ClanObjFive  %d\n", clan->clanobj5 );
        fprintf( fp, "Clanstatus   %d\n", clan->status );
        fprintf( fp, "Recall       %d\n", clan->recall );
        fprintf( fp, "Totalpoints  %d\n", clan->totalpoints );
        fprintf( fp, "GuardOne     %d\n", clan->guard1 );
        fprintf( fp, "GuardTwo     %d\n", clan->guard2 );
        fprintf( fp, "ArenaVictory %d\n", clan->arena_victory );
        fprintf( fp, "Chieflog     %d\n", clan->chieflog );
        fprintf( fp, "Warlog     %d\n", clan->warlog );
        for ( pkillarea = clan->first_pkillarea; pkillarea; pkillarea = pkillarea->next ) {
            fprintf( fp, "PKILLAREA         %s~\n", pkillarea->name );
        }
        fprintf( fp, "End\n\n" );
        fwrite_memberlist( fp, clan );
        fprintf( fp, "#END\n" );
        FileClose( fp );
        fp = NULL;
    }
    return;
}

/*
 * Save a council's data to its data file
 */
void save_council( COUNCIL_DATA * council )
{
    FILE                   *fp;
    char                    filename[256];

    if ( !council ) {
        bug( "%s", "save_council: null council pointer!" );
        return;
    }

    if ( !council->filename || council->filename[0] == '\0' ) {
        bug( "save_council: %s has no filename", council->name );
        return;
    }

    snprintf( filename, 256, "%s%s", COUNCIL_DIR, council->filename );

    if ( ( fp = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "save_council: can't open %s", filename );
        perror( filename );
    }
    else {
        fprintf( fp, "#COUNCIL\n" );
        if ( council->name )
            fprintf( fp, "Name         %s~\n", council->name );
        if ( council->filename )
            fprintf( fp, "Filename     %s~\n", council->filename );
        if ( council->description )
            fprintf( fp, "Description  %s~\n", council->description );
        if ( council->head )
            fprintf( fp, "Head         %s~\n", council->head );
        if ( council->head2 != NULL )
            fprintf( fp, "Head2        %s~\n", council->head2 );
        fprintf( fp, "Members      %d\n", council->members );
        fprintf( fp, "Board        %d\n", council->board );
        fprintf( fp, "Meeting      %d\n", council->meeting );
        if ( council->powers )
            fprintf( fp, "Powers       %s~\n", council->powers );
        fprintf( fp, "End\n\n" );
        fprintf( fp, "#END\n" );
        FileClose( fp );
        fp = NULL;
    }
}

/*
 * Read in actual clan data.
 */

/*
 * Reads in PKill and PDeath still for backward compatibility but now it
 * should be written to PKillRange and PDeathRange for multiple level pkill
 * tracking support. --Shaddai
 * Added a hardcoded limit memlimit to the amount of members a clan can 
 * have set using setclan.  --Shaddai
 */

void fread_clan( CLAN_DATA * clan, FILE * fp )
{
    const char             *word;
    bool                    fMatch;

    clan->mem_limit = 0;                               /* Set up defaults */
    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'A':
                KEY( "Alignment", clan->alignment, fread_number( fp ) );
                KEY( "ArenaVictory", clan->arena_victory, fread_number( fp ) );
                break;

            case 'B':
                KEY( "Badge", clan->badge, fread_string( fp ) );
                KEY( "Board", clan->board, fread_number( fp ) );
                break;

            case 'C':
                KEY( "Chieftain", clan->chieftain, fread_string( fp ) );
                KEY( "ClanObjOne", clan->clanobj1, fread_number( fp ) );
                KEY( "ClanObjTwo", clan->clanobj2, fread_number( fp ) );
                KEY( "ClanObjThree", clan->clanobj3, fread_number( fp ) );
                KEY( "ClanObjFour", clan->clanobj4, fread_number( fp ) );
                KEY( "ClanObjFive", clan->clanobj5, fread_number( fp ) );
                KEY( "Clanstatus", clan->status, fread_number( fp ) );
                KEY( "Chieflog", clan->chieflog, fread_number( fp ) );
                break;

            case 'D':
                KEY( "Description", clan->description, fread_string( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( !clan->name )
                        clan->name = STRALLOC( "" );
                    if ( !clan->description )
                        clan->description = STRALLOC( "" );
                    if ( !clan->warmaster )
                        clan->warmaster = STRALLOC( "" );
                    if ( !clan->chieftain )
                        clan->chieftain = STRALLOC( "" );
                    if ( !clan->motto )
                        clan->motto = STRALLOC( "" );
                    if ( !clan->badge )
                        clan->badge = STRALLOC( "" );
                    return;
                }
                break;

            case 'F':
                KEY( "Filename", clan->filename, fread_string( fp ) );

            case 'G':
                KEY( "GuardOne", clan->guard1, fread_number( fp ) );
                KEY( "GuardTwo", clan->guard2, fread_number( fp ) );
                break;

            case 'I':
                KEY( "Intro", clan->intro, fread_string( fp ) );
                KEY( "IllegalPK", clan->illegal_pk, fread_number( fp ) );
                break;

            case 'M':
                KEY( "MDeaths", clan->mdeaths, fread_number( fp ) );
                KEY( "Members", clan->members, fread_number( fp ) );
                KEY( "MemLimit", clan->mem_limit, fread_number( fp ) );
                KEY( "MKills", clan->mkills, fread_number( fp ) );
                KEY( "Motto", clan->motto, fread_string( fp ) );
                break;

            case 'N':
                KEY( "Name", clan->name, fread_string( fp ) );
                break;

            case 'O':
                break;

            case 'P':
                KEY( "PDeaths", clan->pdeaths[6], fread_number( fp ) );
                KEY( "PKills", clan->pkills[6], fread_number( fp ) );
                /*
                 * Addition of New Ranges 
                 */
                if ( !str_cmp( word, "PDeathRange" ) ) {
                    fMatch = TRUE;
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                }
                if ( !str_cmp( word, "PDeathRangeNew" ) ) {
                    fMatch = TRUE;
                    clan->pdeaths[0] = fread_number( fp );
                    clan->pdeaths[1] = fread_number( fp );
                    clan->pdeaths[2] = fread_number( fp );
                    clan->pdeaths[3] = fread_number( fp );
                    clan->pdeaths[4] = fread_number( fp );
                    clan->pdeaths[5] = fread_number( fp );
                    clan->pdeaths[6] = fread_number( fp );
                    clan->pdeaths[7] = fread_number( fp );
                    clan->pdeaths[8] = fread_number( fp );
                    clan->pdeaths[9] = fread_number( fp );
                    clan->pdeaths[10] = fread_number( fp );
                }
                if ( !strcmp( word, "PKILLAREA" ) ) {
                    PKILLAREA_DATA         *pkillarea;

                    CREATE( pkillarea, PKILLAREA_DATA, 1 );
                    pkillarea->name = fread_string( fp );
                    LINK( pkillarea, clan->first_pkillarea, clan->last_pkillarea, next, prev );
                    fMatch = TRUE;
                    break;
                }

                if ( !str_cmp( word, "PKillRangeNew" ) ) {
                    fMatch = TRUE;
                    clan->pkills[0] = fread_number( fp );
                    clan->pkills[1] = fread_number( fp );
                    clan->pkills[2] = fread_number( fp );
                    clan->pkills[3] = fread_number( fp );
                    clan->pkills[4] = fread_number( fp );
                    clan->pkills[5] = fread_number( fp );
                    clan->pkills[6] = fread_number( fp );
                    clan->pkills[7] = fread_number( fp );
                    clan->pkills[8] = fread_number( fp );
                    clan->pkills[9] = fread_number( fp );
                    clan->pkills[10] = fread_number( fp );
                }

                if ( !str_cmp( word, "PKillRange" ) ) {
                    fMatch = TRUE;
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                    fread_number( fp );
                }
                break;

            case 'R':
                KEY( "Recall", clan->recall, fread_number( fp ) );
                break;

            case 'S':
                KEY( "Score", clan->score, fread_number( fp ) );
                break;

            case 'T':
                KEY( "Totalpoints", clan->totalpoints, fread_number( fp ) );
                KEY( "Type", clan->clan_type, fread_number( fp ) );
                break;
            case 'W':
                KEY( "Warlog", clan->warlog, fread_number( fp ) );
                KEY( "Warmaster", clan->warmaster, fread_string( fp ) );
                break;
        }

        if ( !fMatch ) {
            bug( "Fread_clan: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

/*
 * Read in actual council data.
 */
void fread_council( COUNCIL_DATA * council, FILE * fp )
{
    const char             *word;
    bool                    fMatch;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;
        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;
            case 'B':
                KEY( "Board", council->board, fread_number( fp ) );
                break;
            case 'D':
                KEY( "Description", council->description, fread_string( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( !council->name )
                        council->name = STRALLOC( "" );
                    if ( !council->description )
                        council->description = STRALLOC( "" );
                    if ( !council->powers )
                        council->powers = STRALLOC( "" );
                    return;
                }
                break;

            case 'F':
                KEY( "Filename", council->filename, fread_string( fp ) );
                break;

            case 'H':
                KEY( "Head", council->head, fread_string( fp ) );
                KEY( "Head2", council->head2, fread_string( fp ) );
                break;

            case 'M':
                KEY( "Members", council->members, fread_number( fp ) );
                KEY( "Meeting", council->meeting, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", council->name, fread_string( fp ) );
                break;

            case 'P':
                KEY( "Powers", council->powers, fread_string( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "Fread_council: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

/*
 * Load a clan file
 */
bool load_clan_file( const char *clanfile )
{
    char                    filename[256];
    CLAN_DATA              *clan;
    FILE                   *fp;
    bool                    found;

    CREATE( clan, CLAN_DATA, 1 );

    /*
     * Make sure we default these to 0 --Shaddai 
     */
    clan->pkills[0] = 0;
    clan->pkills[1] = 0;
    clan->pkills[2] = 0;
    clan->pkills[3] = 0;
    clan->pkills[4] = 0;
    clan->pkills[5] = 0;
    clan->pkills[6] = 0;
    clan->pkills[7] = 0;
    clan->pkills[8] = 0;
    clan->pkills[9] = 0;
    clan->pkills[10] = 0;
    clan->pdeaths[0] = 0;
    clan->pdeaths[1] = 0;
    clan->pdeaths[2] = 0;
    clan->pdeaths[3] = 0;
    clan->pdeaths[4] = 0;
    clan->pdeaths[5] = 0;
    clan->pdeaths[6] = 0;
    clan->pdeaths[7] = 0;
    clan->pdeaths[8] = 0;
    clan->pdeaths[9] = 0;
    clan->pdeaths[10] = 0;
    clan->intro = NULL;

    found = FALSE;
    snprintf( filename, 256, "%s%s", CLAN_DIR, clanfile );

    if ( ( fp = FileOpen( filename, "r" ) ) != NULL ) {
        found = TRUE;
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s: # not found.", __FUNCTION__ );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "CLAN" ) )
                fread_clan( clan, fp );
            else if ( !str_cmp( word, "ROSTER" ) )
                fread_memberlist( clan, fp );
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s: bad section: %s.", __FUNCTION__, word );
                break;
            }
        }
        FileClose( fp );
        fp = NULL;
    }

    if ( found )
        LINK( clan, first_clan, last_clan, next, prev );
    else
        DISPOSE( clan );

    if ( clan )
        influence_areas( clan );

    return found;
}

bool load_council_file( const char *councilfile )
{
    char                    filename[256];
    COUNCIL_DATA           *council;
    FILE                   *fp;
    bool                    found;

    CREATE( council, COUNCIL_DATA, 1 );

    found = FALSE;
    snprintf( filename, 256, "%s%s", COUNCIL_DIR, councilfile );

    if ( ( fp = FileOpen( filename, "r" ) ) != NULL ) {

        found = TRUE;
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s", "Load_council_file: # not found." );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "COUNCIL" ) ) {
                fread_council( council, fp );
                break;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "%s", "Load_council_file: bad section." );
                break;
            }
        }
        FileClose( fp );
    }

    if ( found )
        LINK( council, first_council, last_council, next, prev );

    else
        DISPOSE( council );

    return found;
}

/*
 * Load in all the clan files.
 */
void load_clans(  )
{
    FILE                   *fpList;
    const char             *filename;
    char                    clanlist[256];
    PKILLAREA_DATA         *first_pkillarea;
    PKILLAREA_DATA         *last_pkillarea;

    first_clan = NULL;
    last_clan = NULL;
    first_pkillarea = NULL;
    last_pkillarea = NULL;

    log_string( "Loading clans..." );

    snprintf( clanlist, 256, "%s%s", CLAN_DIR, CLAN_LIST );
    if ( ( fpList = FileOpen( clanlist, "r" ) ) == NULL ) {
        perror( clanlist );
        exit( 1 );
    }

    for ( ;; ) {
        filename = feof( fpList ) ? "$" : fread_word( fpList );
        log_printf( "%s", filename );
        if ( filename[0] == '$' )
            break;

        if ( !load_clan_file( filename ) ) {
            bug( "Cannot load clan file: %s", filename );
        }
    }
    FileClose( fpList );
    log_string( " Done clans " );
    return;
}

/*
 * Load in all the council files.
 */
void load_councils(  )
{
    FILE                   *fpList;
    const char             *filename;
    char                    councillist[256];

    first_council = NULL;
    last_council = NULL;

    log_string( "Loading councils..." );

    snprintf( councillist, 256, "%s%s", COUNCIL_DIR, COUNCIL_LIST );
    if ( ( fpList = FileOpen( councillist, "r" ) ) == NULL ) {
        perror( councillist );
        exit( 1 );
    }

    for ( ;; ) {
        filename = feof( fpList ) ? "$" : fread_word( fpList );
        log_printf( "%s", filename );
        if ( filename[0] == '$' )
            break;

        if ( !load_council_file( filename ) ) {
            bug( "Cannot load council file: %s", filename );
        }
    }
    FileClose( fpList );
    log_string( " Done councils " );
    return;
}

void do_banish( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA              *victim;
    char                    arg1[MIL];
    CLAN_DATA              *clan,
                           *vclan;
    char                    buf[MIL];

    argument = one_argument( argument, arg1 );

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    clan = ch->pcdata->clan;

    if ( ( VLD_STR( clan->chieftain ) && !str_cmp( ch->name, clan->chieftain ) )
         || ( VLD_STR( clan->warmaster ) && !str_cmp( ch->name, clan->warmaster ) ) );
    else {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "uso: Desterrar <Jugador>\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg1 ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "No en mobs.\r\n", ch );
        return;
    }

    if ( !( vclan = victim->pcdata->clan ) || vclan != clan ) {
        send_to_char( "No está en tu clan.\r\n", ch );
        return;
    }

    if ( VLD_STR( clan->chieftain ) && !str_cmp( ch->name, clan->chieftain ) ) {
        if ( victim->pcdata->banish == 1 ) {
            send_to_char( "ya has hecho tu parte del destierro.\r\n", ch );
            return;
        }
        else if ( victim->pcdata->banish == 0 ) {
            act( AT_GREEN, "Das la espalda a $N como primer paso en su destierro de $t.", ch,
                 clan->name, victim, TO_CHAR );
            act( AT_GREEN, "$n da la espalda a $N como primer paso en su destierro de $t.", ch,
                 clan->name, victim, TO_ROOM );
            act( AT_GREEN, "$n te da la espalda como primer paso de tu destierro de $t.", ch,
                 clan->name, victim, TO_VICT );
            victim->pcdata->banish = 1;
            return;
        }
    }
    if ( VLD_STR( clan->warmaster ) && !str_cmp( ch->name, clan->warmaster ) ) {
        if ( victim->pcdata->banish == 0 ) {
            act( AT_GREEN, "Das la espalda a $N como primer paso para el destierro de $t.", ch,
                 clan->name, victim, TO_CHAR );
            act( AT_GREEN, "$n da la espalda a $N como primer paso para el destierro de $t.", ch,
                 clan->name, victim, TO_ROOM );
            act( AT_GREEN, "$n te da la espalda como primer paso del destierro de $t.", ch,
                 clan->name, victim, TO_VICT );
            victim->pcdata->banish = 3;
            return;
        }
        else if ( victim->pcdata->banish == 3 ) {
            send_to_char( "ya has hecho tu parte en su destierro.\r\n", ch );
            return;
        }
    }

// outcast guts
    victim->pcdata->clan_timer = current_time;
    if ( clan->members > 0 )
        --clan->members;
    victim->pcdata->clan = NULL;
    STRFREE( victim->pcdata->clan_name );
    victim->pcdata->clan_name = STRALLOC( "" );
    act( AT_MAGIC, "Destierras a $N de $t", ch, clan->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n destierra a $N de $t", ch, clan->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n te destierra de $t", ch, clan->name, victim, TO_VICT );
    snprintf( buf, sizeof( buf ), "&C¡A %s le han expulsado de %s!", victim->name,
              clan->name );
            announce( buf );
    victim->pcdata->banish = 0;
    save_char_obj( victim );
    save_clan( clan );
// end of outcast guts
    check_clan_leaders( clan, victim->name );

    send_to_char( "Sintaxis: Desterrar <jugador>\r\n", ch );
}

void do_influence( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA              *clan;
    CHAR_DATA              *mob = NULL;
    MOB_INDEX_DATA         *mobvnum = NULL;
    int                     vnum = -1;
    PKILLAREA_DATA         *pkillarea = NULL;

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    clan = ch->pcdata->clan;

    if ( !str_cmp( ch->pcdata->clan_name, "halcyon" ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( str_cmp( ch->name, clan->chieftain ) && str_cmp( ch->name, clan->warmaster ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    /*
     * Lets see Affect price of items that are sold, and areas influenced should show up
     * with clans command 
     */
    // Step 1 generate random clan guards
    AREA_DATA              *tarea;
    bool                    found = FALSE;
    ROOM_INDEX_DATA        *location;
    MOB_INDEX_DATA         *imob = NULL;
    CHAR_DATA              *vch = NULL,
        *vch_next = NULL;
    int                     incost;

    if ( !( tarea = ch->in_room->area ) ) {
        send_to_char( "área no encontrada.\r\n", ch );
        return;
    }

    incost = 200;
    if ( !tarea->influencer )
        incost = 150;

    /*
     * We will use 15 as just normal range, above 15 decrease cost by how much above it is, 
     * below increase cost by how much below it is 
     */
    if ( get_curr_cha( ch ) > 15 )
        incost -= ( get_curr_cha( ch ) - 15 );
    else if ( get_curr_cha( ch ) < 15 )
        incost += ( 15 - get_curr_cha( ch ) );

    if ( clan->totalpoints < incost ) {
        ch_printf( ch, "Necesitas %d para influenciar el área.\r\n",
                   incost );
        return;
    }

    if ( IS_SET( tarea->flags, AFLAG_NOINFLUENCE ) ) {
        send_to_char( "Este área no es influenciable.\r\n", ch );
        return;
    }

    if ( tarea->influencer && tarea->influencer == clan ) {
        send_to_char( "ya has influido en ese área.\r\n", ch );
        return;
    }

    if ( tarea->influencer ) {
        for ( pkillarea = tarea->influencer->first_pkillarea; pkillarea;
              pkillarea = pkillarea->next ) {
            if ( str_cmp( pkillarea->name, tarea->name ) )
                continue;
            UNLINK( pkillarea, tarea->influencer->first_pkillarea,
                    tarea->influencer->last_pkillarea, next, prev );
            if ( VLD_STR( pkillarea->name ) )
                STRFREE( pkillarea->name );
            save_clan( tarea->influencer );
            break;
        }
    }

    clan = ch->pcdata->clan;
    tarea->influencer = clan;

    for ( vch = first_char; vch; vch = vch_next ) {
        vch_next = vch->next;

        if ( vch->in_room->area == ch->in_room->area && IS_NPC( vch ) )
            vch->influence = 20;
    }
    CREATE( pkillarea, PKILLAREA_DATA, 1 );
    LINK( pkillarea, clan->first_pkillarea, clan->last_pkillarea, next, prev );

    pkillarea->name = STRALLOC( tarea->name );
    // 41021 to 41025 for throng
    // 41026 to 41029 for alliance
    ch_printf( ch,
               "&cTe mueves por toda la zona mostrando la insignia del clan y ofreces los servicios del clan.\r\n" );
    act( AT_CYAN,
         "&c$n se mueve por el lugar mostrando la insignia de su clan y ofreciendo servicios.",
         ch, NULL, NULL, TO_NOTVICT );
    if ( !str_cmp( ch->pcdata->clan_name, "throng" ) ) {
        vnum = number_range( 41021, 41025 );

        if ( vnum == -1 )
            return;

        imob = get_mob_index( vnum );
    }
    if ( !str_cmp( ch->pcdata->clan_name, "alliance" ) ) {
        vnum = number_range( 41026, 41029 );
        if ( vnum == -1 )
            return;

        imob = get_mob_index( vnum );
    }

    short                   created;
    short                   count;

    count = number_chance( 6, 12 );

    for ( created = 0; created < count; created++ ) {
        if ( ( location =
               get_room_index( number_range( tarea->low_r_vnum, tarea->hi_r_vnum ) ) ) == NULL ) {
            --created;
            continue;
        }
        if ( IS_SET( location->room_flags, ROOM_SAFE ) ) {
            --created;
            continue;
        }

        if ( !imob ) {
            snprintf( log_buf, MAX_STRING_LENGTH, "influence: Missing mob for vnum %d", vnum );
            log_string( log_buf );
            return;
        }
        mob = create_mobile( imob );
        mob->level = number_chance( 10, 20 );
        STRFREE( mob->clan );
        mob->clan = STRALLOC( clan->name );
        mob->influence = 20;
        char_to_room( mob, location );
    }
    clan->totalpoints -= incost;
    save_clan( clan );
}

void do_declare( CHAR_DATA *ch, char *argument )
{
    char                    buf[MIL];
    char                    arg1[MIL];
    char                    arg2[MIL];
    CLAN_DATA              *clan;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

/* Note to self for this business of clan->status and letting players know when 
   thier clan is at war or whatever.
   1 = war   2 = truce   3 = peace   */

    clan = ch->pcdata->clan;

    if ( !str_cmp( ch->pcdata->clan_name, "halcyon" ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !str_cmp( ch->name, clan->chieftain ) || !str_cmp( ch->name, clan->warmaster ) );
    else {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( arg1[0] == '\0' ) {
        send_to_char( "Uso: Declarar <estado> <clan>\r\n", ch );
        send_to_char( "Estado: guerra, hostilidades, paz\r\n", ch );
        return;
    }

    if ( !str_cmp( ch->pcdata->clan_name, arg2 ) ) {
        send_to_char( "¿Te das cuenta que no tiene sentido ninguno?\r\n", ch );
        return;
    }

    if ( !str_cmp( arg1, "guerra" ) ) {
        if ( !str_cmp( arg2, "alianza" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de la Multitud declara la guerra a la alianza!\r\n",
                      ch->name );
        else if ( !str_cmp( arg2, "multitud" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de la Alianza declara la guerra a la Multitud!\r\n",
                      ch->name );
        else if ( !str_cmp( arg2, "halcyon" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de Halcyon declara la guerra!\r\n",
                      ch->name );
            announce( buf );
        clan->status = 1;
        save_clan( clan );
        return;
    }
    if ( !str_cmp( arg1, "paz" ) ) {
        if ( !str_cmp( arg2, "alianza" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de la Multitud declara la paz a la alianza!\r\n",
                      ch->name );
        else if ( !str_cmp( arg2, "multitud" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de la alianza declara la paz a la Multitud!\r\n",
                      ch->name );
        else if ( !str_cmp( arg2, "halcyon" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de Halcyon declara la paz!\r\n",
                      ch->name );
            announce( buf );
        clan->status = 3;
        save_clan( clan );
        return;
    }
    if ( !str_cmp( arg1, "hostilidades" ) ) {
        if ( !str_cmp( arg2, "alianza" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de la Multitud declara hostilidades  a la alianza!\r\n",
                      ch->name );
        else if ( !str_cmp( arg2, "multitud" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de la alianza declara hostilidades a la Multitud!\r\n",
                      ch->name );
        else if ( !str_cmp( arg2, "halcyon" ) )
            snprintf( buf, MIL,
                      "&C¡%s en representación de Halcyon declara hostilidades!\r\n",
                      ch->name );
            announce( buf );
        clan->status = 2;
        save_clan( clan );
        return;
    }

    send_to_char( "uso: Declarar <estado> <clan>\r\n", ch );
    send_to_char( "Estados: guerra, paz, hostilidades\r\n", ch );
    return;
}

void do_intro( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA              *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    clan = ch->pcdata->clan;

    if ( ( !VLD_STR( clan->chieftain ) || str_cmp( ch->name, clan->chieftain ) )
         && ( !VLD_STR( clan->warmaster ) || str_cmp( ch->name, clan->warmaster ) ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    if ( !VLD_STR( argument ) ) {
        send_to_char( "&cuso: intro <&Cmensaje&c>&D\r\n", ch );
        return;
    }

    STRFREE( clan->intro );
    if ( str_cmp( argument, "limpiar" ) )                /* Allow it so they can clear it * 
                                                        * off */
        clan->intro = STRALLOC( argument );
    send_to_char( "tu mensaje de introducción ha sido ajustado.\r\n", ch );
    save_clan( clan );
}

void do_call( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    OBJ_INDEX_DATA         *pObjIndex;
    OBJ_DATA               *obj;
    CLAN_DATA              *clan;
    int                     level;

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    clan = ch->pcdata->clan;

    if ( !str_cmp( ch->name, clan->chieftain ) || !str_cmp( ch->name, clan->warmaster ) );
    else {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Uso: llamar <número>\r\n", ch );
        send_to_char( "Números disponibles: primero segundo tercero cuarto quinto\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "primero" ) ) {
        pObjIndex = get_obj_index( clan->clanobj1 );
        level = 10;
    }
    if ( !str_cmp( arg, "segundo" ) ) {
        pObjIndex = get_obj_index( clan->clanobj2 );
        level = 20;
    }
    if ( !str_cmp( arg, "tercero" ) ) {
        pObjIndex = get_obj_index( clan->clanobj3 );
        level = 40;
    }
    if ( !str_cmp( arg, "cuarto" ) ) {
        pObjIndex = get_obj_index( clan->clanobj4 );
        level = 80;
    }
    if ( !str_cmp( arg, "quinto" ) ) {
        pObjIndex = get_obj_index( clan->clanobj5 );
        level = 100;
    }

    else if ( str_cmp( arg, "primero" ) && str_cmp( arg, "segundo" ) && str_cmp( arg, "tercero" )
              && str_cmp( arg, "cuarto" ) && str_cmp( arg, "quinto" ) ) {
        send_to_char( "No puedes hacer eso.\r\n", ch );
        return;
    }

    obj = create_object( pObjIndex, level );
    if ( CAN_WEAR( obj, ITEM_TAKE ) )
        obj = obj_to_char( obj, ch );
    else
        obj = obj_to_room( obj, ch->in_room );
    act( AT_MAGIC, "¡$n llama $p desde otra dimensión!", ch, obj, NULL, TO_ROOM );
    act( AT_MAGIC, "¡Llamas $p desde otra dimensión!", ch, obj, NULL, TO_CHAR );
    return;
}

void do_induct( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    CHAR_DATA              *victim;
    CLAN_DATA              *clan;

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    clan = ch->pcdata->clan;

    if ( ( ch->pcdata && ch->pcdata->bestowments
           && is_name( "induct", ch->pcdata->bestowments ) ) );
    else {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Induct whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

    if ( IS_IMMORTAL( victim ) ) {
        send_to_char( "You can't induct such a godly presence.\r\n", ch );
        return;
    }

    if ( victim->level < 10 ) {
        send_to_char( "This player is not worthy of joining yet.\r\n", ch );
        return;
    }

    if ( victim->level > ch->level ) {
        send_to_char( "This player is too powerful for you to induct.\r\n", ch );
        return;
    }

    if ( victim->pcdata->clan ) {
        if ( victim->pcdata->clan == clan )
            send_to_char( "This player already belongs to your clan!\r\n", ch );
        else
            send_to_char( "This player already belongs to a clan!\r\n", ch );
        return;
    }
    if ( clan->mem_limit && clan->members >= clan->mem_limit ) {
        send_to_char( "Your clan is too big to induct anymore players.\r\n", ch );
        return;
    }
    clan->members++;

    if ( clan->clan_type != CLAN_NOKILL )
        SET_BIT( victim->pcdata->flags, PCFLAG_DEADLY );

    victim->pcdata->clan = clan;
    STRFREE( victim->pcdata->clan_name );
    victim->pcdata->clan_name = QUICKLINK( clan->name );
    act( AT_MAGIC, "You induct $N into $t", ch, clan->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n inducts $N into $t", ch, clan->name, victim, TO_NOTVICT );
    act( AT_MAGIC, "$n inducts you into $t", ch, clan->name, victim, TO_VICT );
    save_char_obj( victim );
    save_clan( clan );
    return;
}

void do_council_induct( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    CHAR_DATA              *victim;
    COUNCIL_DATA           *council;

    if ( IS_NPC( ch ) || !ch->pcdata->council ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    council = ch->pcdata->council;

    if ( ( council->head == NULL || str_cmp( ch->name, council->head ) )
         && ( council->head2 == NULL || str_cmp( ch->name, council->head2 ) )
         && str_cmp( council->name, "mortal council" ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Induct whom into your council?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

/*    if( victim->level < LEVEL_IMMORTAL )
    {
	send_to_char( "This player is not worthy of joining any council yet.\r\n", ch );
	return;
    }
*/
    if ( victim->pcdata->council ) {
        send_to_char( "This player already belongs to a council!\r\n", ch );
        return;
    }

    council->members++;
    victim->pcdata->council = council;
    STRFREE( victim->pcdata->council_name );
    victim->pcdata->council_name = QUICKLINK( council->name );
    act( AT_MAGIC, "You induct $N into $t", ch, council->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n inducts $N into $t", ch, council->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n inducts you into $t", ch, council->name, victim, TO_VICT );
    save_char_obj( victim );
    save_council( council );
    return;
}

/* Can the character outcast the victim? */
bool can_outcast( CLAN_DATA * clan, CHAR_DATA *ch, CHAR_DATA *victim )
{
    return FALSE;
}

void do_outcast( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    CHAR_DATA              *victim;
    CLAN_DATA              *clan;
    char                    buf[MAX_STRING_LENGTH];

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    clan = ch->pcdata->clan;

    if ( ( ch->pcdata && ch->pcdata->bestowments
           && is_name( "outcast", ch->pcdata->bestowments ) ) );
    else {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Outcast whom?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "Kick yourself out of your own clan?\r\n", ch );
        return;
    }

    if ( victim->pcdata->clan != ch->pcdata->clan ) {
        send_to_char( "This player does not belong to your clan!\r\n", ch );
        return;
    }

    if ( !can_outcast( clan, ch, victim ) ) {
        send_to_char( "You are not able to outcast them.\r\n", ch );
        return;
    }

    --clan->members;
    if ( clan->members < 0 )
        clan->members = 0;
    victim->pcdata->clan = NULL;
    STRFREE( victim->pcdata->clan_name );
    victim->pcdata->clan_name = STRALLOC( "" );
    act( AT_MAGIC, "You outcast $N from $t", ch, clan->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n outcasts $N from $t", ch, clan->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n outcasts you from $t", ch, clan->name, victim, TO_VICT );

    snprintf( buf, MAX_STRING_LENGTH, "%s has been outcast from %s!", victim->name, clan->name );
    echo_to_all( AT_MAGIC, buf, ECHOTAR_ALL );

    save_char_obj( victim );                           /* clan gets saved when pfile is
                                                        * saved */
    save_clan( clan );
    check_clan_leaders( clan, victim->name );

    return;
}

void do_council_outcast( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    CHAR_DATA              *victim;
    COUNCIL_DATA           *council;

    if ( IS_NPC( ch ) || !ch->pcdata->council ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    council = ch->pcdata->council;

    if ( ( council->head == NULL || str_cmp( ch->name, council->head ) )
         && ( council->head2 == NULL || str_cmp( ch->name, council->head2 ) )
         && str_cmp( council->name, "mortal council" ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' ) {
        send_to_char( "Outcast whom from your council?\r\n", ch );
        return;
    }

    if ( !( victim = get_char_room( ch, arg ) ) ) {
        send_to_char( "That player is not here.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "Not on NPC's.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "Kick yourself out of your own council?\r\n", ch );
        return;
    }

    if ( victim->pcdata->council != ch->pcdata->council ) {
        send_to_char( "This player does not belong to your council!\r\n", ch );
        return;
    }

    if ( council->head2 && !str_cmp( victim->name, ch->pcdata->council->head2 ) ) {
        STRFREE( ch->pcdata->council->head2 );
        ch->pcdata->council->head2 = NULL;
    }

    --council->members;
    if ( council->members < 0 )
        council->members = 0;
    victim->pcdata->council = NULL;
    STRFREE( victim->pcdata->council_name );
    victim->pcdata->council_name = STRALLOC( "" );
    // check_switch( ch, FALSE );
    act( AT_MAGIC, "You outcast $N from $t", ch, council->name, victim, TO_CHAR );
    act( AT_MAGIC, "$n outcasts $N from $t", ch, council->name, victim, TO_ROOM );
    act( AT_MAGIC, "$n outcasts you from $t", ch, council->name, victim, TO_VICT );
    save_char_obj( victim );
    save_council( council );
    return;
}

void do_setclan( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    CLAN_DATA              *clan;

    set_char_color( AT_PLAIN, ch );
    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Usage: setclan <clan> <field> <player>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( " members board recall storage guard1 guard2\r\n", ch );
        send_to_char( " align (not functional) memlimit chieftain warmaster\r\n", ch );
        send_to_char( " obj1 obj2 obj3 obj4 obj5 clanpoints\r\n", ch );
        if ( get_trust( ch ) >= LEVEL_AJ_LT ) {
            send_to_char( " name filename motto desc\r\n", ch );
            send_to_char( " strikes type class\r\n", ch );
        }
        if ( get_trust( ch ) >= LEVEL_AJ_LT )
            send_to_char( " pkill1-7 pdeath1-7\r\n", ch );
        return;
    }

    clan = get_clan( arg1 );
    if ( !clan ) {
        send_to_char( "No such clan.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "badge" ) ) {
        STRFREE( clan->badge );
        clan->badge = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "chieftain" ) ) {
        STRFREE( clan->chieftain );
        clan->chieftain = STRALLOC( argument );
        send_to_char( "Clan chieftain is now set.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "warmaster" ) || !str_cmp( arg2, "ambassador" ) ) {
        STRFREE( clan->warmaster );
        clan->warmaster = STRALLOC( argument );
        ch_printf( ch, "Clan %s is now set.\r\n", arg2 );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "board" ) ) {
        clan->board = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "memlimit" ) ) {
        clan->mem_limit = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "members" ) ) {
        clan->members = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "recall" ) ) {
        clan->recall = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "clanpoints" ) ) {
        clan->totalpoints = atoi( argument );
        send_to_char( "Clan total points set.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "obj1" ) ) {
        clan->clanobj1 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "obj2" ) ) {
        clan->clanobj2 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "obj3" ) ) {
        clan->clanobj3 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "obj4" ) ) {
        clan->clanobj4 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "obj5" ) ) {
        clan->clanobj5 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "status" ) ) {
        clan->status = atoi( argument );
        send_to_char( "Clan status is now set.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "guard1" ) ) {
        clan->guard1 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "guard2" ) ) {
        clan->guard2 = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( get_trust( ch ) < LEVEL_AJ_LT ) {
        do_setclan( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg2, "align" ) ) {
        clan->alignment = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "type" ) ) {
        clan->clan_type = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        CLAN_DATA              *uclan = NULL;

        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "You can't name a clan nothing.\r\n", ch );
            return;
        }
        if ( ( uclan = get_clan( argument ) ) ) {
            send_to_char( "There is already another clan with that name.\r\n", ch );
            return;
        }
        STRFREE( clan->name );
        clan->name = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "filename" ) ) {
        char                    filename[256];

        if ( !is_valid_filename( ch, CLAN_DIR, argument ) )
            return;

        snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clan->filename );
        if ( !remove( filename ) )
            send_to_char( "Old clan file deleted.\r\n", ch );

        STRFREE( clan->filename );
        clan->filename = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        write_clan_list(  );
        return;
    }

    if ( !str_cmp( arg2, "intro" ) ) {
        STRFREE( clan->intro );
        clan->intro = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "motto" ) ) {
        STRFREE( clan->motto );
        clan->motto = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( !str_cmp( arg2, "desc" ) ) {
        STRFREE( clan->description );
        clan->description = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_clan( clan );
        return;
    }

    if ( get_trust( ch ) < LEVEL_AJ_LT ) {
        do_setclan( ch, ( char * ) "" );
        return;
    }

    if ( !str_prefix( "pkill", arg2 ) ) {
        int                     temp_value;

        if ( !str_cmp( arg2, "pkill1" ) )
            temp_value = 0;
        else if ( !str_cmp( arg2, "pkill2" ) )
            temp_value = 1;
        else if ( !str_cmp( arg2, "pkill3" ) )
            temp_value = 2;
        else if ( !str_cmp( arg2, "pkill4" ) )
            temp_value = 3;
        else if ( !str_cmp( arg2, "pkill5" ) )
            temp_value = 4;
        else if ( !str_cmp( arg2, "pkill6" ) )
            temp_value = 5;
        else if ( !str_cmp( arg2, "pkill7" ) )
            temp_value = 6;
        else if ( !str_cmp( arg2, "pkill8" ) )
            temp_value = 7;
        else if ( !str_cmp( arg2, "pkill9" ) )
            temp_value = 8;
        else if ( !str_cmp( arg2, "pkill10" ) )
            temp_value = 9;
        else if ( !str_cmp( arg2, "pkill11" ) )
            temp_value = 10;
        else if ( !str_cmp( arg2, "pkill12" ) )
            temp_value = 11;
        else {
            do_setclan( ch, ( char * ) "" );
            return;
        }
        clan->pkills[temp_value] = atoi( argument );
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_prefix( "pdeath", arg2 ) ) {
        int                     temp_value;

        if ( !str_cmp( arg2, "pdeath1" ) )
            temp_value = 0;
        else if ( !str_cmp( arg2, "pdeath2" ) )
            temp_value = 1;
        else if ( !str_cmp( arg2, "pdeath3" ) )
            temp_value = 2;
        else if ( !str_cmp( arg2, "pdeath4" ) )
            temp_value = 3;
        else if ( !str_cmp( arg2, "pdeath5" ) )
            temp_value = 4;
        else if ( !str_cmp( arg2, "pdeath6" ) )
            temp_value = 5;
        else if ( !str_cmp( arg2, "pdeath7" ) )
            temp_value = 6;
        else if ( !str_cmp( arg2, "pdeath8" ) )
            temp_value = 7;
        else if ( !str_cmp( arg2, "pdeath9" ) )
            temp_value = 8;
        else if ( !str_cmp( arg2, "pdeath10" ) )
            temp_value = 9;
        else if ( !str_cmp( arg2, "pdeath11" ) )
            temp_value = 10;
        else if ( !str_cmp( arg2, "pdeath12" ) )
            temp_value = 11;
        else {
            do_setclan( ch, ( char * ) "" );
            return;
        }
        clan->pdeaths[temp_value] = atoi( argument );
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    do_setclan( ch, ( char * ) "" );
}

void do_setcouncil( CHAR_DATA *ch, char *argument )
{
    char                    arg1[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    COUNCIL_DATA           *council;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if ( arg1[0] == '\0' ) {
        send_to_char( "Usage: setcouncil <council> <field> <value>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( " head head2 members board meeting\r\n", ch );
        if ( get_trust( ch ) >= LEVEL_AJ_LT )
            send_to_char( " name filename desc\r\n", ch );
        if ( get_trust( ch ) >= LEVEL_AJ_LT )
            send_to_char( " powers\r\n", ch );
        return;
    }

    council = get_council( arg1 );
    if ( !council ) {
        send_to_char( "No such council.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "head" ) ) {
        STRFREE( council->head );
        council->head = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        return;
    }

    if ( !str_cmp( arg2, "head2" ) ) {
        if ( council->head2 != NULL )
            STRFREE( council->head2 );
        if ( !str_cmp( argument, "none" ) || !str_cmp( argument, "clear" ) )
            council->head2 = NULL;
        else
            council->head2 = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        return;
    }

    if ( !str_cmp( arg2, "board" ) ) {
        council->board = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        return;
    }

    if ( !str_cmp( arg2, "members" ) ) {
        council->members = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        return;
    }

    if ( !str_cmp( arg2, "meeting" ) ) {
        council->meeting = atoi( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        return;
    }

    if ( get_trust( ch ) < LEVEL_AJ_LT ) {
        do_setcouncil( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg2, "name" ) ) {
        COUNCIL_DATA           *ucouncil;

        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "Can't set a council name to nothing.\r\n", ch );
            return;
        }
        if ( ( ucouncil = get_council( argument ) ) ) {
            send_to_char( "A council is already using that name.\r\n", ch );
            return;
        }
        STRFREE( council->name );
        council->name = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        return;
    }

    if ( !str_cmp( arg2, "filename" ) ) {
        char                    filename[256];

        if ( !is_valid_filename( ch, COUNCIL_DIR, argument ) )
            return;

        snprintf( filename, sizeof( filename ), "%s%s", COUNCIL_DIR, council->filename );
        if ( !remove( filename ) )
            send_to_char( "Old council file deleted.\r\n", ch );

        STRFREE( council->filename );
        council->filename = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        write_council_list(  );
        return;
    }

    if ( !str_cmp( arg2, "desc" ) ) {
        STRFREE( council->description );
        council->description = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        return;
    }

    if ( get_trust( ch ) < LEVEL_AJ_LT ) {
        do_setcouncil( ch, ( char * ) "" );
        return;
    }

    if ( !str_cmp( arg2, "powers" ) ) {
        STRFREE( council->powers );
        council->powers = STRALLOC( argument );
        send_to_char( "Done.\r\n", ch );
        save_council( council );
        return;
    }

    do_setcouncil( ch, ( char * ) "" );
    return;
}

/*
 * Added multiple levels on pkills and pdeaths. -- Shaddai
 */

void do_showclan( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA              *clan;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char( "Usage: showclan <clan>\r\n", ch );
        return;
    }

    clan = get_clan( argument );
    if ( !clan ) {
        send_to_char( "No such clan.\r\n", ch );
        return;
    }

    ch_printf_color( ch,
                     "\r\n&wClan    : &W%s\r\n&wBadge: %s\r\n&wFilename : &W%s\r\n&wMotto    : &W%s\r\n",
                     clan->name, clan->badge ? clan->badge : "(not set)", clan->filename,
                     clan->motto );
    ch_printf_color( ch, "&wDesc     : &W%s\r\n", clan->description );
    ch_printf_color( ch, "&wChieftain: &W%s\r\n&wWar Master: &W%s\r\n", clan->chieftain,
                     clan->warmaster );
    ch_printf_color( ch,
                     "&wPKills   : &w1-9:&W%-3d &w10-19:&W%-3d &w20-29:&W%-3d &w30-39:&W%-3d &w40-49:&W%-3d &w50-59:&W%-3d &w60-69:&W%-3d\r\n",
                     clan->pkills[0], clan->pkills[1], clan->pkills[2], clan->pkills[3],
                     clan->pkills[4], clan->pkills[5], clan->pkills[6] );
    ch_printf_color( ch, "           &w70-79:&W%-3d &w80-89:&W%-3d &w90-99:&W%-3d &w100:&W%-3d\r\n",
                     clan->pkills[7], clan->pkills[8], clan->pkills[9], clan->pkills[10] );
    ch_printf_color( ch,
                     "&wPDeaths  : &w1-9:&W%-3d &w10-19:&W%-3d &w20-29:&W%-3d &w30-39:&W%-3d &w40-49:&W%-3d &w50-59:&W%-3d &w60-69:&W%-3d\r\n",
                     clan->pdeaths[0], clan->pdeaths[1], clan->pdeaths[2], clan->pdeaths[3],
                     clan->pdeaths[4], clan->pdeaths[5], clan->pdeaths[6] );
    ch_printf_color( ch, "           &w70-79:&W%-3d &w80-89:&W%-3d &w90-99:&W%-3d &w100:&W%-3d\r\n",
                     clan->pdeaths[7], clan->pdeaths[8], clan->pdeaths[9], clan->pdeaths[10] );
    ch_printf_color( ch, "&wIllegalPK: &W%-6d\r\n", clan->illegal_pk );
    ch_printf_color( ch, "&wMKills   : &W%-6d   &wMDeaths: &W%-6d   &wScore  : &W%-6d\r\n",
                     clan->mkills, clan->mdeaths, clan->score );
    ch_printf_color( ch, "&wMembers  : &W%-6d  &wMemLimit: &W%-6d   &wAlign  : &W%-6d",
                     clan->members, clan->mem_limit, clan->alignment );
    ch_printf_color( ch,
                     "&wBoard    : &W%-5d    \r\n&wRecall : &W%-5d    &wClan Reputation: &W%-5d\r\n",
                     clan->board, clan->recall, clan->totalpoints );
    ch_printf_color( ch, "&wGuard1   : &W%-5d    &wGuard2 : &W%-5d\r\n", clan->guard1,
                     clan->guard2 );
    ch_printf_color( ch,
                     "&wObj1( &W%d &w)  Obj2( &W%d &w)  Obj3( &W%d &w)  Obj4( &W%d &w)  Obj5( &W%d &w)\r\n",
                     clan->clanobj1, clan->clanobj2, clan->clanobj3, clan->clanobj4,
                     clan->clanobj5 );
    return;
}

void do_showcouncil( CHAR_DATA *ch, char *argument )
{
    COUNCIL_DATA           *council;

    set_char_color( AT_PLAIN, ch );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char( "Usage: showcouncil <council>\r\n", ch );
        return;
    }

    council = get_council( argument );
    if ( !council ) {
        send_to_char( "No such council.\r\n", ch );
        return;
    }

    ch_printf_color( ch, "\r\n&wCouncil :  &W%s\r\n&wFilename:  &W%s\r\n", council->name,
                     council->filename );
    ch_printf_color( ch, "&wHead:      &W%s\r\n", council->head );
    ch_printf_color( ch, "&wHead2:     &W%s\r\n", council->head2 );
    ch_printf_color( ch, "&wMembers:   &W%-d\r\n", council->members );
    ch_printf_color( ch, "&wBoard:     &W%-5d\r\n&wMeeting:   &W%-5d\r\n&wPowers:    &W%s\r\n",
                     council->board, council->meeting, council->powers );
    ch_printf_color( ch, "&wDescription:\r\n&W%s\r\n", council->description );
    return;
}

void do_makeclan( CHAR_DATA *ch, char *argument )
{
    CLAN_DATA              *clan;

    set_char_color( AT_IMMORT, ch );

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Usage: makeclan <clan name>\r\n", ch );
        return;
    }

    set_char_color( AT_PLAIN, ch );
    clan = get_clan( argument );
    if ( clan ) {
        send_to_char( "There is already a clan with that name.\r\n", ch );
        return;
    }

    CREATE( clan, CLAN_DATA, 1 );
    LINK( clan, first_clan, last_clan, next, prev );

    clan->name = STRALLOC( argument );
    /*
     * Let's refix this, STRALLOC shouldn't be used for the 'filename'
     * member without changing load_clan() and do_setclan() to employ hashstrings too... 
     */
    clan->filename = STRALLOC( "" );
    clan->motto = STRALLOC( "" );
    clan->description = STRALLOC( "" );
    clan->warmaster = STRALLOC( "" );
    clan->chieftain = STRALLOC( "" );
    clan->badge = STRALLOC( "" );
}

void do_makecouncil( CHAR_DATA *ch, char *argument )
{
    char                    filename[256];
    COUNCIL_DATA           *council;
    bool                    found;

    set_char_color( AT_IMMORT, ch );

    if ( !argument || argument[0] == '\0' ) {
        send_to_char( "Usage: makecouncil <council name>\r\n", ch );
        return;
    }

    found = FALSE;
    snprintf( filename, 256, "%s%s", COUNCIL_DIR, strlower( argument ) );

    CREATE( council, COUNCIL_DATA, 1 );
    LINK( council, first_council, last_council, next, prev );
    council->name = STRALLOC( argument );
    council->head = STRALLOC( "" );
    council->head2 = NULL;
    council->powers = STRALLOC( "" );
}

/* Added multiple level pkill and pdeath support. --Shaddai */
void do_clans( CHAR_DATA *ch, char *argument )
{
    PKILLAREA_DATA         *pkillarea;
    CLAN_DATA              *clan;
    int                     count = 0;
    bool                    firstdis;

    if ( argument[0] == '\0' ) {
        set_char_color( AT_BLOOD, ch );
        send_to_char
            ( "\r\n         Clan     Cacique     Embajador/Maestro   Pkills: Avatar\r\n", ch );
        send_to_char
            ( "_________________________________________________________________________\r\r\n\n",
              ch );
        for ( clan = first_clan; clan; clan = clan->next ) {
            set_char_color( AT_GREY, ch );
            ch_printf( ch, "%13s %13s %24s", clan->name, clan->chieftain, clan->warmaster );
            set_char_color( AT_BLOOD, ch );
            ch_printf( ch, "            %5d\r\n", clan->pkills[10] );
            count++;
        }
        set_char_color( AT_BLOOD, ch );
        if ( !count )
            send_to_char( "No hay clanes.\r\n", ch );
        else
            send_to_char
                ( "_________________________________________________________________________\r\r\n\nUsa 'clanes <clan>' para más información.\r\n",
                  ch );
        return;
    }

    clan = get_clan( argument );
    if ( !clan ) {
        set_char_color( AT_BLOOD, ch );
        send_to_char( "No existe ese clan.\r\n", ch );
        return;
    }
    set_char_color( AT_BLOOD, ch );
    ch_printf( ch, "\r\n%s, '%s'\r\r\n\n", clan->name, clan->motto );
    if ( VLD_STR( clan->intro ) )
        ch_printf( ch, "Intro: '%s'\r\r\n\n", clan->intro );
    set_char_color( AT_GREY, ch );
    send_to_char_color( "Victorias:&w\r\n", ch );
    ch_printf_color( ch,
                     "    &w 1- 9: &r%-4d &w10-19: &r%-4d &w20-29: &r%-4d &w30-39: &r%-4d &w40-49: &r%-4d\r\n",
                     clan->pkills[0], clan->pkills[1], clan->pkills[2], clan->pkills[3],
                     clan->pkills[4] );
    ch_printf_color( ch,
                     "    &w50-59: &r%-4d &w60-69: &r%-4d &w70-79: &r%-4d &w80-89: &r%-4d &w90-99: &r%-4d\r\n",
                     clan->pkills[5], clan->pkills[6], clan->pkills[7], clan->pkills[8],
                     clan->pkills[9] );
    ch_printf_color( ch, "    &w  100: &r%-4d\r\n", clan->pkills[10] );
    ch_printf_color( ch, "&wVictorias en arena: &r%d&d\r\n", clan->arena_victory );
    set_char_color( AT_GREY, ch );
    ch_printf( ch, "Cacique:  %s\r\n", clan->chieftain );
    if ( !str_cmp( clan->name, "Halcyon" ) )
        send_to_char( "Embajador:  ", ch );
    else
        send_to_char( "Maestro:  ", ch );
    ch_printf( ch, "%s\r\n", clan->warmaster );
    ch_printf( ch, "Miembros: %d\r\n", clan->members );
    ch_printf( ch, "Reputación: %d\r\n\r\n", clan->totalpoints );
    set_char_color( AT_BLOOD, ch );
    ch_printf( ch, "Descripción:  %s\r\n", clan->description );

    firstdis = TRUE;
    int                     acount = 0;

    for ( pkillarea = clan->first_pkillarea; pkillarea; pkillarea = pkillarea->next ) {
        if ( !VLD_STR( pkillarea->name ) )
            continue;
        if ( firstdis )
            ch_printf( ch, "&wÁreas bajo la influencia de &r%s &w:\r\n", clan->name );
        firstdis = FALSE;
        pager_printf( ch, "&G%-25s", capitalize( pkillarea->name ) );
        acount++;
        if ( acount == 3 ) {
            send_to_char( "\r\n", ch );
            acount = 0;
        }
    }
}

void do_councils( CHAR_DATA *ch, char *argument )
{
    COUNCIL_DATA           *council;

    set_char_color( AT_CYAN, ch );
    if ( !first_council ) {
        send_to_char( "There are no councils currently formed.\r\n", ch );
        return;
    }
    if ( argument[0] == '\0' ) {
        send_to_char_color( "\r\n&cTitle                    Head\r\n", ch );
        for ( council = first_council; council; council = council->next ) {
            if ( council->head2 != NULL )
                ch_printf_color( ch, "&w%-24s %s and %s\r\n", council->name, council->head,
                                 council->head2 );
            else
                ch_printf_color( ch, "&w%-24s %-14s\r\n", council->name, council->head );
        }
        send_to_char_color( "&cUse 'councils <name of council>' for more detailed information.\r\n",
                            ch );
        return;
    }
    council = get_council( argument );
    if ( !council ) {
        send_to_char_color( "&cNo such council exists...\r\n", ch );
        return;
    }
    ch_printf_color( ch, "&c\r\n%s\r\n", council->name );
    if ( council->head2 == NULL )
        ch_printf_color( ch, "&cHead:     &w%s\r\n&cMembers:  &w%d\r\n", council->head,
                         council->members );
    else
        ch_printf_color( ch, "&cCo-Heads:     &w%s &cand &w%s\r\n&cMembers:  &w%d\r\n",
                         council->head, council->head2, council->members );
    ch_printf_color( ch, "&cDescription:\r\n&w%s\r\n", council->description );
    return;
}

void do_victories( CHAR_DATA *ch, char *argument )
{
    char                    filename[256];

    if ( IS_NPC( ch ) || !ch->pcdata->clan ) {
        send_to_char( "Huh?\r\n", ch );
        return;
    }

    snprintf( filename, 256, "%s%s.record", CLAN_DIR, ch->pcdata->clan->name );
    set_pager_color( AT_PURPLE, ch );
    if ( IS_IMMORTAL( ch ) && !str_cmp( argument, "clean" ) ) {
        FILE                   *fp = FileOpen( filename, "w" );

        if ( fp )
            FileClose( fp );
        send_to_pager( "\r\nvictorias limpiadas.\r\n", ch );
        return;
    }
    else {
        send_to_pager( "\r\nNIV  Personaje       NIV  Personaje\r\n", ch );
        show_file( ch, filename );
        return;
    }
    return;
}

void do_shove( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    int                     exit_dir;
    EXIT_DATA              *pexit;
    CHAR_DATA              *victim;
    bool                    nogo;
    ROOM_INDEX_DATA        *to_room;
    int                     schance = 0;
    int                     race_bonus = 0;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC( ch ) || !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) ) {
        send_to_char( "Solo personajes pks pueden empujar.\r\n", ch );
        return;
    }

    if ( get_timer( ch, TIMER_PKILLED ) > 0 ) {
        send_to_char( "No puedes empujar jugadores ahora.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Empujar a quién?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }
    if ( victim == ch ) {
        send_to_char( "Tanta violencia te ha dejado el cerebro frito.\r\n", ch );
        return;
    }
    // Only a Dragon is big enough to shove another Dragon. -Taon
    if ( victim->race == RACE_DRAGON && ch->race != RACE_DRAGON ) {
        send_to_char( "¿Intentando empujar un dragón?\r\n", ch );
        return;
    }
    if ( IS_NPC( victim ) || !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ) {
        send_to_char( "No puedes empujar a personajes no pk.\r\n", ch );
        return;
    }

    if ( ch->level - victim->level > 10 || victim->level - ch->level > 10 ) {
        send_to_char( "La diferencia de niveles te lo impide.\r\n",
                      ch );
        return;
    }

    if ( get_timer( victim, TIMER_PKILLED ) > 0 ) {
        send_to_char( "No puedes empujar ese jugador ahora.\r\n", ch );
        return;
    }

    if ( ( victim->position ) != POS_STANDING ) {
        act( AT_PLAIN, "$N no está en pie.", ch, NULL, victim, TO_CHAR );
        return;
    }

    if ( arg2[0] == '\0' ) {
        send_to_char( "¿En qué dirección?\r\n", ch );
        return;
    }

    exit_dir = get_dir( arg2 );
    if ( IS_SET( victim->in_room->room_flags, ROOM_SAFE )
         && get_timer( victim, TIMER_SHOVEDRAG ) <= 0 ) {
        send_to_char( "Ese jugador no puede ser empujado ahora mismo.\r\n", ch );
        return;
    }
    set_position( victim, POS_SHOVE );
    nogo = FALSE;
    if ( ( pexit = get_exit( ch->in_room, exit_dir ) ) == NULL )
        nogo = TRUE;
    else if ( IS_SET( pexit->exit_info, EX_CLOSED )
              && ( !IS_AFFECTED( victim, AFF_PASS_DOOR )
                   || IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
        nogo = TRUE;
    if ( nogo ) {
        send_to_char( "No hay salida en esa dirección.\r\n", ch );
        set_position( victim, POS_STANDING );
        return;
    }
    to_room = pexit->to_room;
    if ( IS_SET( to_room->room_flags, ROOM_DEATH ) ) {
        send_to_char( "No puedes empujar a alguien hacia una trampa mortal.\r\n", ch );
        set_position( victim, POS_STANDING );
        return;
    }

    if ( ch->in_room->area != to_room->area && !in_hard_range( victim, to_room->area ) ) {
        send_to_char( "Ese jugador no puede entrar a ese área.\r\n", ch );
        set_position( victim, POS_STANDING );
        return;
    }

/* Check for class, assign percentage based on that. */
    schance = 35;

/* Add 3 points to chance for every str point above 15, subtract for below 15 */

    schance += ( ( get_curr_str( ch ) - 15 ) * 3 );

    schance += ( ch->level - victim->level );

/* Check for race, adjust percentage based on that. */
    if ( ch->race == RACE_ELF )
        race_bonus = -3;

    if ( ch->race == RACE_DWARF )
        race_bonus = 3;

    if ( ch->race == RACE_HALFLING )
        race_bonus = -5;

    if ( ch->race == RACE_PIXIE )
        race_bonus = -7;

    if ( ch->race == RACE_OGRE )
        race_bonus = 5;

    if ( ch->race == RACE_ORC )
        race_bonus = 7;

    if ( ch->race == RACE_TROLL )
        race_bonus = 10;

    if ( ch->race == RACE_DROW )
        race_bonus = 1;

    if ( ch->race == RACE_GNOME )
        race_bonus = -2;

    schance += race_bonus;

    if ( schance < number_percent(  ) ) {
        send_to_char( "has fallado.\r\n", ch );
        set_position( victim, POS_STANDING );
        return;
    }
    act( AT_ACTION, "Empujas a $N.", ch, NULL, victim, TO_CHAR );
    act( AT_ACTION, "$n te empuja.", ch, NULL, victim, TO_VICT );
    move_char( victim, get_exit( ch->in_room, exit_dir ), 0, FALSE );
    if ( !char_died( victim ) )
        set_position( victim, POS_STANDING );
    WAIT_STATE( ch, 12 );
    /*
     * Remove protection from shove/drag if char shoves -- Blodkai 
     */
    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) && get_timer( ch, TIMER_SHOVEDRAG ) <= 0 )
        add_timer( ch, TIMER_SHOVEDRAG, 10, NULL, 0 );
}

void do_drag( CHAR_DATA *ch, char *argument )
{
    char                    arg[MAX_INPUT_LENGTH];
    char                    arg2[MAX_INPUT_LENGTH];
    int                     exit_dir;
    CHAR_DATA              *victim;
    EXIT_DATA              *pexit;
    ROOM_INDEX_DATA        *to_room;
    bool                    nogo;
    int                     schance = 0;
    int                     race_bonus = 0;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC( ch ) ) {
        send_to_char( "solo los jugadores pueden arrastrar.\r\n", ch );
        return;
    }

    if ( get_timer( ch, TIMER_PKILLED ) > 0 ) {
        send_to_char( "No puedes arrastrar a nadie ahora.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' ) {
        send_to_char( "¿Arrastrar a quién?\r\n", ch );
        return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL ) {
        send_to_char( "No está aquí.\r\n", ch );
        return;
    }

    if ( victim == ch ) {
        send_to_char( "Tanta violencia te ha frito el cerebro.\r\n", ch );
        return;
    }

    if ( IS_NPC( victim ) ) {
        send_to_char( "Solo puedes arrastrar jugadores.\r\n", ch );
        return;
    }

    if ( !xIS_SET( victim->act, PLR_SHOVEDRAG ) && !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ) {
        send_to_char( "Ese jugador no aprecia tus intenciones.\r\n", ch );
        return;
    }

    if ( get_timer( victim, TIMER_PKILLED ) > 0 ) {
        send_to_char( "No puedes arrastrar ese jugador ahora.\r\n", ch );
        return;
    }

    if ( victim->fighting ) {
        send_to_char( "Lo intentas, pero no puedes acercarte lo suficiente.\r\n", ch );
        return;
    }

    if ( !IS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
         && IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ) {
        send_to_char( "No puedes arrastrar a personajes pks.\r\n", ch );
        return;
    }

    if ( !IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) && victim->position > 3 ) {
        send_to_char( "No necesita tu asistencia.\r\n", ch );
        return;
    }

    if ( arg2[0] == '\0' ) {
        send_to_char( "¿En qué dirección?\r\n", ch );
        return;
    }

    if ( ch->level - victim->level > 10 || victim->level - ch->level > 10 ) {
        if ( IS_SET( victim->pcdata->flags, PCFLAG_DEADLY )
             && IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) ) {
            send_to_char( "La diferencia de niveles te lo impide.\r\n",
                          ch );
            return;
        }
    }

    exit_dir = get_dir( arg2 );

    if ( IS_SET( victim->in_room->room_flags, ROOM_SAFE )
         && get_timer( victim, TIMER_SHOVEDRAG ) <= 0 ) {
        send_to_char( "Ese jugador no puede ser arrastrado ahora.\r\n", ch );
        return;
    }

    nogo = FALSE;
    if ( ( pexit = get_exit( ch->in_room, exit_dir ) ) == NULL )
        nogo = TRUE;
    else if ( IS_SET( pexit->exit_info, EX_CLOSED )
              && ( !IS_AFFECTED( victim, AFF_PASS_DOOR )
                   || IS_SET( pexit->exit_info, EX_NOPASSDOOR ) ) )
        nogo = TRUE;
    if ( nogo ) {
        send_to_char( "No hay salida en esa dirección.\r\n", ch );
        return;
    }

    to_room = pexit->to_room;
    if ( IS_SET( to_room->room_flags, ROOM_DEATH ) ) {
        send_to_char( "No puedes arrastrar a alguien a una trampa mortal.\r\n", ch );
        return;
    }

    if ( ch->in_room->area != to_room->area && !in_hard_range( victim, to_room->area ) ) {
        send_to_char( "Ese personaje no puede entrar a ese área.\r\n", ch );
        set_position( victim, POS_STANDING );
        return;
    }

    /*
     * Check for class, assign percentage based on that. 
     */
    schance = 35;

    /*
     * Add 3 points to chance for every str point above 15, subtract for below 15 
     */

    schance += ( ( get_curr_str( ch ) - 15 ) * 3 );

    schance += ( ch->level - victim->level );

    if ( ch->race == 1 )
        race_bonus = -3;

    if ( ch->race == 2 )
        race_bonus = 3;

    if ( ch->race == 3 )
        race_bonus = -5;

    if ( ch->race == 4 )
        race_bonus = -7;

    if ( ch->race == 6 )
        race_bonus = 5;

    if ( ch->race == 7 )
        race_bonus = 7;

    if ( ch->race == 8 )
        race_bonus = 10;

    if ( ch->race == 9 )
        race_bonus = -2;

    schance += race_bonus;

    if ( schance < number_percent(  ) ) {
        send_to_char( "Fallaste.\r\n", ch );
        set_position( victim, POS_STANDING );
        return;
    }

    if ( victim->position < POS_STANDING ) {
        short                   temp;

        temp = victim->position;
        set_position( victim, POS_DRAG );
        act( AT_ACTION, "Arrastras a $N.", ch, NULL, victim, TO_CHAR );
        act( AT_ACTION, "$n te arrastra.", ch, NULL, victim, TO_VICT );
        move_char( victim, get_exit( ch->in_room, exit_dir ), 0, FALSE );
        if ( !char_died( victim ) )
            set_position( victim, temp );
        /*
         * Move ch to the room too.. they are doing dragging - Scryn 
         */
        move_char( ch, get_exit( ch->in_room, exit_dir ), 0, FALSE );
        WAIT_STATE( ch, 12 );
        return;
    }
    send_to_char( "No puedes arrastrar a alguien que está de pié.\r\n", ch );
}

bool is_head_architect( CHAR_DATA *ch )
{
    if ( !ch || !ch->pcdata || !ch->pcdata->council
         || ch->pcdata->council != get_council( "Area Architect" ) )
        return FALSE;
    if ( ch->pcdata->council->head && !str_cmp( ch->pcdata->council->head, ch->name ) )
        return TRUE;
    if ( ch->pcdata->council->head2 && !str_cmp( ch->pcdata->council->head2, ch->name ) )
        return TRUE;
    return FALSE;
}

/* Set all areas the clan is influencing */
void influence_areas( CLAN_DATA * clan )
{
    PKILLAREA_DATA         *pkillarea,
                           *pkillarea_next;
    AREA_DATA              *pkarea;
    bool                    pkremove;                  /* Should we Remove the area or
                                                        * set it as influenced by the
                                                        * clan. */

    for ( pkillarea = clan->first_pkillarea; pkillarea; pkillarea = pkillarea_next ) {
        pkillarea_next = pkillarea->next;

        pkremove = TRUE;                               /* Assume we will remove it */

        for ( pkarea = first_area; pkarea; pkarea = pkarea->next ) {
            if ( !str_cmp( pkillarea->name, pkarea->name ) ) {
                if ( !IS_SET( pkarea->flags, AFLAG_NOINFLUENCE ) ) {
                    pkarea->influencer = clan;
                    pkremove = FALSE;                  /* So should be set and not
                                                        * removed */
                }
                break;
            }
        }

        if ( pkremove ) {
            UNLINK( pkillarea, clan->first_pkillarea, clan->last_pkillarea, next, prev );
            free_pkillarea( pkillarea );
        }
    }
}

/* Used to assign the first person in line for the position */
void assign_clan_leaders( CLAN_DATA * clan )
{
    ROSTER_DATA            *rost;
    char                    buf[MSL];
    bool                    changed = FALSE;

    if ( !clan || ( clan->chieftain && clan->warmaster ) || !clan->first_member )
        return;

    for ( rost = clan->first_member; rost; rost = rost->next ) {
        if ( clan->chieftain && !str_cmp( clan->chieftain, rost->name ) )
            continue;
        if ( clan->warmaster && !str_cmp( clan->warmaster, rost->name ) )
            continue;
        if ( !clan->chieftain ) {
            changed = TRUE;
            clan->chieftain = STRALLOC( rost->name );
            snprintf( buf, MSL,
                      "&C¡%s ahora es el capitán de %s!\r\n",
                      clan->chieftain, clan->name );
            announce( buf );
            continue;
        }
        if ( !clan->warmaster ) {
            changed = TRUE;
            clan->warmaster = STRALLOC( rost->name );
            snprintf( buf, MSL,
                      "&C¡%s ahora es el maestro de guerra de %s!\r\n",
                      clan->warmaster, clan->name );
            announce( buf );

            continue;
        }
    }

    if ( changed )
        save_clan( clan );
}

/* Used if a player is outcast etc... to check and remove them from leader spots */
void check_clan_leaders( CLAN_DATA * clan, char *player )
{
    char                    buf[MSL];

    if ( !clan || !player )
        return;

    if ( clan->chieftain && !str_cmp( clan->chieftain, player ) ) {
        snprintf( buf, MSL,
                  "&C¡%s ha sido eliminado como capitán de  %s!\r\n",
                  clan->chieftain, clan->name );
            announce( buf );

        STRFREE( clan->chieftain );
        clan->chieftain = NULL;
        save_clan( clan );
    }

    if ( clan->warmaster && !str_cmp( clan->warmaster, player ) ) {
        snprintf( buf, MSL,
                  "&C¡%s ha sido eliminado como maestro de %s!\r\n",
                  clan->warmaster, clan->name );
            announce( buf );

        STRFREE( clan->warmaster );
        clan->warmaster = NULL;
        save_clan( clan );
    }

    assign_clan_leaders( clan );
}

void increase_clan_pkills( CLAN_DATA * clan, int level )
{
    if ( !clan )
        return;
    if ( level < 10 )
        clan->pkills[0]++;
    else if ( level < 20 )
        clan->pkills[1]++;
    else if ( level < 30 )
        clan->pkills[2]++;
    else if ( level < 40 )
        clan->pkills[3]++;
    else if ( level < 50 )
        clan->pkills[4]++;
    else if ( level < 60 )
        clan->pkills[5]++;
    else if ( level < 70 )
        clan->pkills[6]++;
    else if ( level < 80 )
        clan->pkills[7]++;
    else if ( level < 90 )
        clan->pkills[8]++;
    else if ( level < 100 )
        clan->pkills[9]++;
    else if ( level == 100 )
        clan->pkills[10]++;
}

void increase_clan_pdeaths( CLAN_DATA * clan, int level )
{
    if ( !clan )
        return;
    if ( level < 10 )
        clan->pdeaths[0]++;
    else if ( level < 20 )
        clan->pdeaths[1]++;
    else if ( level < 30 )
        clan->pdeaths[2]++;
    else if ( level < 40 )
        clan->pdeaths[3]++;
    else if ( level < 50 )
        clan->pdeaths[4]++;
    else if ( level < 60 )
        clan->pdeaths[5]++;
    else if ( level < 70 )
        clan->pdeaths[6]++;
    else if ( level < 80 )
        clan->pdeaths[7]++;
    else if ( level < 90 )
        clan->pdeaths[8]++;
    else if ( level < 100 )
        clan->pdeaths[9]++;
    else if ( level == 100 )
        clan->pdeaths[10]++;
}
