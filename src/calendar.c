/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 * Registered with the United States Copyright Office: TX 5-877-286         *
 *                                                                          *
 * External contributions from Remcon, Quixadhal, Zarius, and many others.  *
 *                                                                          *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                      Calendar Handler/Seasonal Updates                   *
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
// #include <sys/stat.h>
#if !defined(WIN32)
#include <dirent.h>
#endif
#include "h/mud.h"
#include "h/files.h"
#include "h/key.h"

HOLIDAY_DATA           *first_holiday;
HOLIDAY_DATA           *last_holiday;

#define MAX_TZONE   25

void                    string_stripcolor( char *argument, char *string );
extern time_t           board_expire_time_t;
extern ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];

struct tzone_type
{
    const char             *name;                      /* Name of the time zone */
    const char             *zone;                      /* Cities or Zones in zone
                                                        * crossing */
    int                     gmt_offset;                /* Difference in hours from
                                                        * Greenwich Mean Time */
    int                     dst_offset;                /* Day Light Savings Time offset,
                                                        * Not used but left it in anyway */
};

struct tzone_type       tzone_table[MAX_TZONE] = {
    {"GMT-12", "Eniwetok", -12, 0},
    {"GMT-11", "Samoa", -11, 0},
    {"GMT-10", "Hawaii", -10, 0},
    {"GMT-9", "Alaska", -9, 0},
    {"GMT-8", "Pacific US", -8, -7},
    {"GMT-7", "Mountain US", -7, -6},
    {"GMT-6", "Central US", -6, -5},
    {"GMT-5", "Eastern US", -5, -4},
    {"GMT-4", "Atlantic, Canada", -4, 0},
    {"GMT-3", "Brazilia, Buenos Aires", -3, 0},
    {"GMT-2", "Mid-Atlantic", -2, 0},
    {"GMT-1", "Cape Verdes", -1, 0},
    {"GMT", "Greenwich Mean Time, Greenwich", 0, 0},
    {"GMT+1", "Berlin, Rome", 1, 0},
    {"GMT+2", "Israel, Cairo", 2, 0},
    {"GMT+3", "Moscow, Kuwait", 3, 0},
    {"GMT+4", "Abu Dhabi, Muscat", 4, 0},
    {"GMT+5", "Islamabad, Karachi", 5, 0},
    {"GMT+6", "Almaty, Dhaka", 6, 0},
    {"GMT+7", "Bangkok, Jakarta", 7, 0},
    {"GMT+8", "Perth, Hong Kong, Beijing", 8, 0},
    {"GMT+9", "Tokyo, Osaka", 9, 0},
    {"GMT+10", "Sydney, Melbourne, Guam", 10, 0},
    {"GMT+11", "Magadan, Soloman Is.", 11, 0},
    {"GMT+12", "Fiji, Wellington, Auckland", 12, 0},
};

int tzone_lookup( const char *arg )
{
    int                     i;

    for ( i = 0; i < MAX_TZONE; i++ ) {
        if ( !str_cmp( arg, tzone_table[i].name ) )
            return i;
    }

    for ( i = 0; i < MAX_TZONE; i++ ) {
        if ( is_name( arg, ( char * ) tzone_table[i].zone ) )
            return i;
    }
    return -1;
}

void do_timezone( CHAR_DATA *ch, char *argument )
{
    int                     i;

    if ( IS_NPC( ch ) )
        return;

    if ( !argument || argument[0] == '\0' ) {
        ch_printf( ch, "%-6s %-30s (%s)\r\n", "Nombre", "Ciudad/Zona horaria", "Hora" );
        send_to_char
            ( "-------------------------------------------------------------------------\r\n", ch );
        for ( i = 0; i < MAX_TZONE; i++ ) {
            ch_printf( ch, "%-6s %-30s (%s)\r\n", tzone_table[i].name, tzone_table[i].zone,
                       c_time( current_time, i ) );
        }
        send_to_char
            ( "-------------------------------------------------------------------------\r\n", ch );
        send_to_char( "\r\nSyntaxis: Zona horaria < &CGMT(&Y-&W/&Y+&C)#&W >\r\n", ch );
        return;
    }

    i = tzone_lookup( argument );

    if ( i == -1 ) {
        send_to_char( "Esa zona horaria no existe. Aseg�rate de usar el nombre exacto.\r\n", ch );
        return;
    }

    ch->pcdata->timezone = i;
    ch_printf( ch, "Tu zona horaria actual es %s %s (%s)\r\n", tzone_table[i].name, tzone_table[i].zone,
               c_time( current_time, i ) );
}

/* Ever so slightly modified version of "friendly_ctime" provided by Aurora.
 * Merged with the Timezone snippet by Ryan Jennings (Markanth) r-jenn@shaw.ca
 */
char                   *c_time( time_t curtime, int tz )
{
    const static char      *day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    const static char      *month[] =
        { "Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov",
        "Dic"
    };
    static char             strtime[128];
    struct tm              *ptime;
    char                    tzonename[50];

    if ( curtime <= 0 )
        curtime = current_time;

    if ( tz > -1 && tz < MAX_TZONE ) {
        mudstrlcpy( tzonename, tzone_table[tz].zone, 50 );
#if defined(__CYGWIN__)
        curtime += ( time_t ) timezone;
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
        /*
         * Hopefully this works
         */
        ptime = localtime( &curtime );
        curtime += ptime->tm_gmtoff;
#elif defined(WIN32)
        curtime += _timezone;
#else
        curtime += timezone;                           /* timezone external variable in
                                                        * time.h holds the difference in
                                                        * seconds to GMT. */
#endif
        curtime += ( 60 * 60 * tzone_table[tz].gmt_offset );    /* Add the offset hours */
    }
    ptime = localtime( &curtime );
    if ( tz < 0 || tz >= MAX_TZONE )
#if defined(__CYGWIN__) || defined(WIN32)
        mudstrlcpy( tzonename, tzname[ptime->tm_isdst], 50 );
#else
        mudstrlcpy( tzonename, ptime->tm_zone, 50 );
#endif

    snprintf( strtime, 128, "%3s %3s %d, %d %d:%02d:%02d %cM %s",
              day[ptime->tm_wday], month[ptime->tm_mon], ptime->tm_mday,
              ptime->tm_year + 1900,
              ptime->tm_hour == 0 ? 12 : ptime->tm_hour > 12 ? ptime->tm_hour - 12 : ptime->tm_hour,
              ptime->tm_min, ptime->tm_sec, ptime->tm_hour < 12 ? 'A' : 'P', tzonename );
    return strtime;
}

/* timeZone is not shown as it's a bit .. long.. but it is respected -- Xorith */
char                   *mini_c_time( time_t curtime, int tz )
{
    static char             strtime[128];
    struct tm              *ptime;
    char                    tzonename[50];

    if ( curtime <= 0 )
        curtime = current_time;

    if ( tz > -1 && tz < MAX_TZONE ) {
        mudstrlcpy( tzonename, tzone_table[tz].zone, 50 );
#if defined(__CYGWIN__)
        curtime += ( time_t ) timezone;
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
        /*
         * Hopefully this works
         */
        ptime = localtime( &curtime );
        curtime += ptime->tm_gmtoff;
#elif defined(WIN32)
        curtime += _timezone;
#else
        curtime += timezone;                           /* timezone external variable in
                                                        * time.h holds the difference in
                                                        * seconds to GMT. */
#endif
        curtime += ( 60 * 60 * tzone_table[tz].gmt_offset );    /* Add the offset hours */
    }
    ptime = localtime( &curtime );
    if ( tz < 0 || tz >= MAX_TZONE )
#if defined(__CYGWIN__) || defined(WIN32)
        mudstrlcpy( tzonename, tzname[ptime->tm_isdst], 50 );
#else
        mudstrlcpy( tzonename, ptime->tm_zone, 50 );
#endif

    snprintf( strtime, 128, "%02d/%02d/%02d %02d:%02d%c", ptime->tm_mon + 1,
              ptime->tm_mday, ptime->tm_year - 100,
              ptime->tm_hour == 0 ? 12 : ptime->tm_hour > 12 ? ptime->tm_hour - 12 : ptime->tm_hour,
              ptime->tm_min, ptime->tm_hour < 12 ? 'A' : 'P' );
    return strtime;
}

/* Time values modified to Alsherok calendar - Samson 5-6-99 */
/* Time Values Modified to Smaug Calendar - Kayle 10-17-07 */
const char             *const day_name[] = {
    "de la Luna", "del Toro", "de la Decepci�n", "del Trueno", "de la Libertad",
    "de los Grandes Dioses", "del Sol"
};

/*
char *const month_name[] = {
   "Winter", "the Winter Wolf", "the Frost Giant", "the Old Forces",
   "the Grand Struggle", "the Spring", "Nature", "Futility", "the Dragon",
   "the Sun", "the Heat", "the Battle", "the Dark Shades", "the Shadows",
   "the Long Shadows", "the Ancient Darkness", "the Great Evil"
};
*/

const char             *const month_name[] = {
    "la Gran Guerra", "la Renovaci�n", "la Naturaleza", "la Esperanza", /* primavera */
    "el Drag�n", "el Sol", "el Calor", "la Batalla", /* verano */
    "los Vientos del Oeste", "las Largas Sombras", "la Futilidad", "la Profunda Oscuridad",   /* fall */
    "el Gran Mal", "el Lobo Invernal", "el Gigante de Hielo", "las Fuerzas Antiguas"    /* invierno
                                                                                 */
};

const char             *const season_name[] = {
    "&gprimavera", "&Yverano", "&Ooto�o", "&Cinvierno"
};

/* Calling function must insure tstr buffer is large enough.
 * Returns the address of the buffer passed, allowing things like
 * this printf example: 123456secs = 1day 10hrs 17mins 36secs
 *   time_t tsecs = 123456;
 *   char   buff[ MSL ];
 *
 *   printf( "La duraci�n es %s\n", duration( tsecs, buff ) );
 */

#define  DUR_SCMN       ( 60 )
#define  DUR_MNHR       ( 60 )
#define  DUR_HRDY       ( 24 )
#define  DUR_DYWK       (  7 )
#define  DUR_ADDS( t )  ( (t) == 1 ? '\0' : 's' )

char                   *sec_to_hms( time_t loctime, char *tstr )
{
    time_t                  t_rem;
    int                     sc,
                            mn,
                            hr,
                            dy,
                            wk;
    int                     sflg = 0;
    char                    buff[MSL];

    if ( loctime < 1 ) {
        mudstrlcat( tstr, "no time at all", MSL );
        return ( tstr );
    }

    sc = loctime % DUR_SCMN;
    t_rem = loctime - sc;

    if ( t_rem > 0 ) {
        t_rem /= DUR_SCMN;
        mn = t_rem % DUR_MNHR;
        t_rem -= mn;

        if ( t_rem > 0 ) {
            t_rem /= DUR_MNHR;
            hr = t_rem % DUR_HRDY;
            t_rem -= hr;

            if ( t_rem > 0 ) {
                t_rem /= DUR_HRDY;
                dy = t_rem % DUR_DYWK;
                t_rem -= dy;

                if ( t_rem > 0 ) {
                    wk = t_rem / DUR_DYWK;

                    if ( wk ) {
                        sflg = 1;
                        snprintf( buff, MSL, "%d week%c", wk, DUR_ADDS( wk ) );
                        mudstrlcat( tstr, buff, MSL );
                    }
                }
                if ( dy ) {
                    if ( sflg == 1 )
                        mudstrlcat( tstr, " ", MSL );
                    sflg = 1;
                    snprintf( buff, MSL, "%d day%c", dy, DUR_ADDS( dy ) );
                    mudstrlcat( tstr, buff, MSL );
                }
            }
            if ( hr ) {
                if ( sflg == 1 )
                    mudstrlcat( tstr, " ", MSL );
                sflg = 1;
                snprintf( buff, MSL, "%d hour%c", hr, DUR_ADDS( hr ) );
                mudstrlcat( tstr, buff, MSL );
            }
        }
        if ( mn ) {
            if ( sflg == 1 )
                mudstrlcat( tstr, " ", MSL );
            sflg = 1;
            snprintf( buff, MSL, "%d minute%c", mn, DUR_ADDS( mn ) );
            mudstrlcat( tstr, buff, MSL );
        }
    }
    if ( sc ) {
        if ( sflg == 1 )
            mudstrlcat( tstr, " ", MSL );
        snprintf( buff, MSL, "%d second%c", sc, DUR_ADDS( sc ) );
        mudstrlcat( tstr, buff, MSL );
    }
    return ( tstr );
}

HOLIDAY_DATA           *get_holiday( short month, short day )
{
    HOLIDAY_DATA           *holiday;

    for ( holiday = first_holiday; holiday; holiday = holiday->next )
        if ( month + 1 == holiday->month && day + 1 == holiday->day )
            return holiday;
    return NULL;
}

/* Reads the actual time file from disk - Samson 1-21-99 */
void fread_timedata( FILE * fp )
{
    const char             *word;
    bool                    fMatch = FALSE;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = FALSE;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = TRUE;
                fread_to_eol( fp );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) )
                    return;
                break;

            case 'M':
                KEY( "Mhora", time_info.hour, fread_number( fp ) );
                KEY( "Md�a", time_info.day, fread_number( fp ) );
                KEY( "Mmes", time_info.month, fread_number( fp ) );
                KEY( "Ma�o", time_info.year, fread_number( fp ) );
                break;
        }

        if ( !fMatch ) {
            bug( "Fread_timedata: no match: %s", word );
            fread_to_eol( fp );
        }
    }
}

/* Load time information from saved file - Samson 1-21-99 */
bool load_timedata( void )
{
    char                    filename[256];
    FILE                   *fp;
    bool                    found;

    found = FALSE;
    snprintf( filename, 256, "%stime.dat", SYSTEM_DIR );

    if ( ( fp = FileOpen( filename, "r" ) ) != NULL ) {

        found = TRUE;
        for ( ;; ) {
            char                    letter = '\0';
            char                   *word = NULL;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s", "Load_timedata: # not found." );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "TIME" ) ) {
                fread_timedata( fp );
                break;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "Load_timedata: bad section - %s.", word );
                break;
            }
        }
        FileClose( fp );
    }

    return found;
}

/* Saves the current game world time to disk - Samson 1-21-99 */
void save_timedata( void )
{
    FILE                   *fp;
    char                    filename[256];

    snprintf( filename, 256, "%stime.dat", SYSTEM_DIR );

    if ( ( fp = FileOpen( filename, "w" ) ) == NULL ) {
        bug( "%s", "pcdata->save_timedata: FileOpen" );
        perror( filename );
    }
    else {
        fprintf( fp, "%s", "#TIME\n" );
        fprintf( fp, "Mhora	%d\n", time_info.hour );
        fprintf( fp, "Md�a	%d\n", time_info.day );
        fprintf( fp, "Mmes	%d\n", time_info.month );
        fprintf( fp, "Ma�o	%d\n", time_info.year );
        fprintf( fp, "%s", "End\n\n" );
        fprintf( fp, "%s", "#END\n" );
    }
    FileClose( fp );
    return;
}

void do_time( CHAR_DATA *ch, char *argument )
{
    HOLIDAY_DATA           *holiday;
    extern char             str_boot_time[];

    const char             *suf;
    short                   day;

    day = time_info.day + 1;

    if ( IS_NPC( ch ) )
        return;

    if ( day > 4 && day < 20 )
        suf = "th";
    else if ( day % 10 == 1 )
        suf = "st";
    else if ( day % 10 == 2 )
        suf = "nd";
    else if ( day % 10 == 3 )
        suf = "rd";
    else
        suf = "th";

    ch_printf( ch,
               "&wSon las &W%d&w en punto &W%s&w, D�a de &W%s&w,&W %d%s&w en el Mes de &W%s&w.\r\n"
               "&wLa estaci�n del a�o es %s&w, a�o &W%d&w.\r\n"
               "&wEl mud se inici� a las  :  &W %s\r\n" "&wLa hora del sistema es        :  &W %s\r\n",
               ( time_info.hour % sysdata.hournoon ==
                 0 ) ? sysdata.hournoon : time_info.hour % sysdata.hournoon,
               time_info.hour >= sysdata.hournoon ? "pm" : "am",
               day_name[( time_info.day ) % sysdata.daysperweek], day, suf,
               month_name[time_info.month], season_name[time_info.season], time_info.year,
               str_boot_time, c_time( current_time, -1 ) );

    ch_printf( ch, "&wTu hora local es        :  &W %s&D\r\n",
               c_time( current_time, ch->pcdata->timezone ) );
    holiday = get_holiday( time_info.month, day - 1 );

    if ( holiday != NULL )
        ch_printf( ch, "&wHoy es un d�a de fiesta:&W %s\r\n", holiday->name );

    if ( !IS_NPC( ch ) ) {
        if ( day == ch->pcdata->day + 1 && time_info.month == ch->pcdata->month )
            send_to_char( "&WHoy es tu &Pc&pu&Ym&Opl&Ye&pa&P�os&R!&D\r\n", ch );
    }

    return;
}

void start_winter( void )
{
    ROOM_INDEX_DATA        *room;
    int                     iHash;

    echo_to_all( AT_CYAN, "El aire adquiere un fr�o escalofriante... El invierno ha llegado.", ECHOTAR_ALL );
    echo_to_all( AT_CYAN, "Las aguas del mundo se congelan.\r\n", ECHOTAR_ALL );

    winter_freeze = TRUE;
/*
    for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
        for( room = room_index_hash[iHash]; room; room = room->next )
        {
            switch ( room->sector_type )
            {
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                    room->winter_sector = room->sector_type;
                    room->sector_type = SECT_ARCTIC;
                    break;
            }
        }
    }
*/
    return;
}

void start_spring( void )
{
    ROOM_INDEX_DATA        *room;
    int                     iHash;

    echo_to_all( AT_DGREEN, "El fr�o se aleja del aire mientras la primavera comienza.",
                 ECHOTAR_ALL );
    echo_to_all( AT_BLUE, "Las aguas del mundo se descongelan.\r\n", ECHOTAR_ALL );

    winter_freeze = FALSE;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ ) {
        for ( room = room_index_hash[iHash]; room; room = room->next ) {
            if ( room->sector_type == SECT_ARCTIC && room->winter_sector != -1 ) {
                room->sector_type = room->winter_sector;
                room->winter_sector = -1;
            }
        }
    }
    return;
}

void start_summer( void )
{
    echo_to_all( AT_YELLOW, "Los d�as se alargan y se tornan calurosos con el comienzo del verano.\r\n",
                 ECHOTAR_ALL );
    return;
}

void start_fall( void )
{
    echo_to_all( AT_ORANGE, "Las hojas comienzan a cambiar el color, ha llegado el oto�o.\r\n",
                 ECHOTAR_ALL );
    return;
}

void season_update( void )
{
    HOLIDAY_DATA           *day;

    day = get_holiday( time_info.month, time_info.day );

    if ( day != NULL ) {
        if ( time_info.day + 1 == day->day && time_info.hour == 0 ) {
            echo_to_all( AT_IMMORT, day->announce, ECHOTAR_ALL );
        }
    }

    if ( time_info.season == SEASON_WINTER && winter_freeze == FALSE ) {
        ROOM_INDEX_DATA        *room;
        int                     iHash;

        winter_freeze = TRUE;
/*
        for( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
        {
            for( room = room_index_hash[iHash]; room; room = room->next )
            {
                switch ( room->sector_type )
                {
                    case SECT_WATER_SWIM:
                    case SECT_WATER_NOSWIM:
                        room->winter_sector = room->sector_type;
                        room->sector_type = SECT_ARCTIC;
                        break;
                }
            }
        }
*/
    }
    return;
}

/* PaB: Which season are we in?
 * Notes: Simply figures out which season the current month falls into
 * and returns a proper value.
 */
void calc_season( void )
{
    /*
     * How far along in the year are we, measured in days?
     */
    /*
     * PaB: Am doing this in days to minimize roundoff impact
     */
    int                     day = time_info.month * sysdata.dayspermonth + time_info.day;

    if ( day < ( sysdata.daysperyear / 4 ) ) {
        time_info.season = SEASON_SPRING;
        if ( time_info.hour == 0 && day == 0 )
            start_spring(  );
    }
    else if ( day < ( sysdata.daysperyear / 4 ) * 2 ) {
        time_info.season = SEASON_SUMMER;
        if ( time_info.hour == 0 && day == ( sysdata.daysperyear / 4 ) )
            start_summer(  );
    }
    else if ( day < ( sysdata.daysperyear / 4 ) * 3 ) {
        time_info.season = SEASON_FALL;
        if ( time_info.hour == 0 && day == ( ( sysdata.daysperyear / 4 ) * 2 ) )
            start_fall(  );
    }
    else if ( day < sysdata.daysperyear ) {
        time_info.season = SEASON_WINTER;
        if ( time_info.hour == 0 && day == ( ( sysdata.daysperyear / 4 ) * 3 ) )
            start_winter(  );
    }
    else {
        time_info.season = SEASON_SPRING;
    }

    season_update(  );                                 /* Maintain the season in case of
                                                        * reboot, check for holidays */

    return;
}

void free_holiday( HOLIDAY_DATA * day )
{
    UNLINK( day, first_holiday, last_holiday, next, prev );
    STRFREE( day->announce );
    STRFREE( day->name );
    DISPOSE( day );
}

void free_holidays( void )
{
    HOLIDAY_DATA           *day,
                           *day_next;

    for ( day = first_holiday; day; day = day_next ) {
        day_next = day->next;
        free_holiday( day );
    }
}

void do_holidays( CHAR_DATA *ch, char *argument )
{
    HOLIDAY_DATA           *day;

    send_to_pager( "&RFiesta		       &YMes	        &GD�a\r\n", ch );
    send_to_pager( "&g----------------------+----------------+---------------\r\n", ch );

    for ( day = first_holiday; day; day = day->next )
        pager_printf( ch, "&G%-21s	&g%-11s	%-2d\r\n", day->name, month_name[day->month - 1],
                      day->day );

    return;
}

/* Read in an individual holiday */
void fread_day( HOLIDAY_DATA * day, FILE * fp )
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

            case 'A':
                KEY( "Announce", day->announce, fread_string( fp ) );
                break;

            case 'D':
                KEY( "Day", day->day, fread_number( fp ) );
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) ) {
                    if ( !day->announce )
                        day->announce =
                            STRALLOC( "Today is a holiday, but who the hell knows which one." );
                    return;
                }
                break;

            case 'M':
                KEY( "Month", day->month, fread_number( fp ) );
                break;

            case 'N':
                KEY( "Name", day->name, fread_string( fp ) );
                break;
        }

        if ( !fMatch )
            bug( "fread_day: no match: %s", word );
    }
}

/* Load the holiday file */
void load_holidays( void )
{
    char                    filename[256];
    HOLIDAY_DATA           *day;
    FILE                   *fp;
    short                   daycount;

    first_holiday = NULL;
    last_holiday = NULL;

    snprintf( filename, 256, "%s%s", SYSTEM_DIR, HOLIDAY_FILE );

    if ( ( fp = FileOpen( filename, "r" ) ) != NULL ) {
        daycount = 0;
        for ( ;; ) {
            char                    letter;
            char                   *word;

            letter = fread_letter( fp );
            if ( letter == '*' ) {
                fread_to_eol( fp );
                continue;
            }

            if ( letter != '#' ) {
                bug( "%s", "load_holidays: # not found." );
                break;
            }

            word = fread_word( fp );
            if ( !str_cmp( word, "HOLIDAY" ) ) {
                if ( daycount >= 100 ) {
                    bug( "load_holidays: more holidays than %d, increase Max Holiday in cset.",
                         sysdata.maxholiday );
                    FileClose( fp );
                    return;
                }
                CREATE( day, HOLIDAY_DATA, 1 );
                fread_day( day, fp );
                daycount++;
                LINK( day, first_holiday, last_holiday, next, prev );
                continue;
            }
            else if ( !str_cmp( word, "END" ) )
                break;
            else {
                bug( "load_holidays: bad section: %s.", word );
                continue;
            }
        }
        FileClose( fp );
    }

    return;
}

/* Save the holidays to disk - Samson 5-6-99 */
void save_holidays( void )
{
    HOLIDAY_DATA           *day;
    FILE                   *fp;
    char                    filename[256];

    snprintf( filename, 256, "%s%s", SYSTEM_DIR, HOLIDAY_FILE );

    if ( !( fp = FileOpen( filename, "w" ) ) ) {
        bug( "%s", "save_holidays: FileOpen" );
        perror( filename );
    }
    else {
        for ( day = first_holiday; day; day = day->next ) {
            fprintf( fp, "%s", "#HOLIDAY\n" );
            fprintf( fp, "Name		%s~\n", day->name );
            fprintf( fp, "Announce	%s~\n", day->announce );
            fprintf( fp, "Month		%d\n", day->month );
            fprintf( fp, "Day		%d\n", day->day );
            fprintf( fp, "%s", "End\n\n" );
        }
        fprintf( fp, "%s", "#END\n" );
        FileClose( fp );
    }
    return;
}

void do_saveholiday( CHAR_DATA *ch, char *argument )
{
    save_holidays(  );
    send_to_char( "Holiday chart saved.\r\n", ch );
    return;
}

/* Holiday OLC command - (c)Andrew Wilkie May-20-2005*/
/* Calendar code (c)The Alsherok Team*/
void do_setholiday( CHAR_DATA *ch, char *argument )
{
    HOLIDAY_DATA           *day,
                           *newday;
    int                     count = 0;
    int                     x = 0;
    char                    arg1[MIL],
                            arg2[MIL],
                            arg3[MIL];

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( arg1[0] == '\0' ) {
        send_to_char( "Syntax : setholiday <name> <field> <argument>\r\n", ch );
        send_to_char( "Field can be : day name create announce save delete\r\n", ch );
        return;
    }

/* Save em all.. saves the work of saving individual
   holidays when mass-creating or editing*/
    if ( !str_cmp( arg1, "save" ) ) {
        save_holidays(  );
        send_to_char( "Holiday chart saved.\r\n", ch );
        return;
    }

/* Create a new holiday by name arg1 */
    if ( !str_cmp( arg2, "create" ) ) {
        for ( day = first_holiday; day; day = day->next )
            count++;
        for ( day = first_holiday; day; day = day->next ) {
            if ( !str_cmp( arg1, day->name ) ) {
                send_to_char( "A holiday with that name exists already!\r\n", ch );
                return;
            }
        }

        if ( count >= sysdata.maxholiday ) {
            send_to_char( "There are already too many holidays!\r\n", ch );
            return;
        }

        CREATE( newday, HOLIDAY_DATA, 1 );
        newday->name = STRALLOC( arg1 );
        newday->day = time_info.day;
        newday->month = time_info.month;
        newday->announce =
            STRALLOC
            ( "Today is the holiday of when some moron forgot to set the announcement for this one!" );
        LINK( newday, first_holiday, last_holiday, next, prev );
        send_to_char( "Holiday created.\r\n", ch );
        return;
    }

/* Now... let's find that holiday */

    for ( day = first_holiday; day; day = day->next ) {
        if ( !str_cmp( day->name, arg1 ) )
            break;
    }

/* Anything match? */
    if ( !day ) {
        send_to_char( "Which holiday was that?\r\n", ch );
        return;
    }

/* Set the day */

    if ( !str_cmp( arg2, "day" ) ) {                   // bug fix below. -Taon
        if ( *arg3 == '\0' || !is_number( arg3 ) || atoi( arg3 ) > sysdata.dayspermonth
             || atoi( arg3 ) < 1 ) {
            ch_printf( ch, "You must specify a numeric value : %d - %d", 1, sysdata.dayspermonth );
            return;
        }

/* What day is it?... FRIDAY!.. oh... no... it's.. arg3? */
        day->day = atoi( arg3 );
        send_to_char( "D�a cambiado.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "month" ) ) {
/* Go through the months and find arg3 */

        if ( *arg3 == '\0' || !is_number( arg3 ) || atoi( arg3 ) > sysdata.monthsperyear
             || atoi( arg3 ) <= 1 ) {
            send_to_char( "You must specify a valid month number:\r\n", ch );

/* List all the months with a counter next to them*/
            count = 1;
            while ( month_name[x] != NULL && str_cmp( month_name[x], " " )
                    && x < sysdata.monthsperyear ) {
                ch_printf( ch, "&R(&W%d&R)&Y%s\r\n", count, month_name[x] );
                x++;
                count++;
            }
            return;
        }

/* What's the month? */
        day->month = atoi( arg3 );
        send_to_char( "Mes cambiado.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "announce" ) ) {
        if ( *arg3 == '\0' || !str_cmp( arg3, " " ) || is_number( arg3 ) ) {
            send_to_char( "�Qu� anuncio quieres poner?\r\n", ch );
            return;
        }

        char                    string[MAX_STRING_LENGTH];

/* Set the announcement */
        if ( VLD_STR( day->announce ) )
            STRFREE( day->announce );
        string_stripcolor( arg3, string );
//      day->announce = str_dup( arg3 );
        string[0] = UPPER( string[0] );
        day->announce = STRALLOC( string );

        send_to_char( "Anuncio cambiado.\r\n", ch );
        return;
    }

/* Change the name */
    if ( !str_cmp( arg2, "name" ) ) {
        if ( *arg3 == '\0' || !str_cmp( arg3, " " ) || is_number( arg3 ) ) {
            send_to_char( "�Qu� nombre le quieres poner?\r\n", ch );
            return;
        }

/* Release the good...err... name */
        if ( VLD_STR( day->name ) )
            STRFREE( day->name );
        day->name = STRALLOC( arg3 );
        send_to_char( "Nombre cambiado.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "delete" ) ) {
        if ( str_cmp( arg3, "yes" ) ) {
            send_to_char( "Si est�s seguro, escribe 'delete yes'.\r\n", ch );
            return;
        }

        free_holiday( day );
        send_to_char( "&RFiesta borrada.\r\n", ch );
        return;
    }

    send_to_char( "Syntax: setholiday <name> <field> <argument>\r\n", ch );
    send_to_char( "Field can be: day name create announce save delete\r\n", ch );
    return;
}

void do_showholiday( CHAR_DATA *ch, char *argument )
{
    HOLIDAY_DATA           *day;

    for ( day = first_holiday; day; day = day->next ) {
        if ( !str_cmp( day->name, argument ) )
            break;
    }

/* Anything match? */
    if ( !day ) {
        send_to_char( "Which holiday was that?\r\n", ch );
        return;
    }

    ch_printf( ch, "&RHOLIDAY - &W%s\r\n", day->name );
    ch_printf( ch, "&g--------------------\r\n" );
    ch_printf( ch, "&GMonth &W%s (%d)\t&GDay &W%2d\r\n", month_name[day->month], day->month,
               day->day );
    ch_printf( ch, "&GAnnounce: &W%s\r\n", day->announce == NULL ? "(none!)" : day->announce );

    return;

}
