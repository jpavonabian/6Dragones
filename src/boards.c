/*****************************************************************************
 * DikuMUD (C) 1990, 1991 by:                                                *
 *   Sebastian Hammer, Michael Seifert, Hans Henrik Staefeldt, Tom Madsen,   *
 *   and Katja Nyboe.                                                        *
 *---------------------------------------------------------------------------*
 * MERC 2.1 (C) 1992, 1993 by:                                               *
 *   Michael Chastain, Michael Quan, and Mitchell Tse.                       *
 *---------------------------------------------------------------------------*
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998 by: Derek Snider.                    *
 *   Team: Thoric, Altrag, Blodkai, Narn, Haus, Scryn, Rennard, Swordbearer, *
 *         gorog, Grishnakh, Nivek, Tricops, and Fireblade.                  *
 *---------------------------------------------------------------------------*
 * SMAUG 1.7 FUSS by: Samson and others of the SMAUG community.              *
 *                    Their contributions are greatly appreciated.           *
 *---------------------------------------------------------------------------*
 * LoP (C) 2006 - 2013 by: the LoP team.                                     *
 *---------------------------------------------------------------------------*
 *			     Special boards module			     *
 *****************************************************************************/
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "h/mud.h"
#include "h/clans.h"
#include "h/files.h"
#include "h/key.h"
#include "h/channels.h"

/* Defines for voting on notes. -- Narn */
#define VOTE_NONE 0
#define VOTE_OPEN 1
#define VOTE_CLOSED 2

#define VOTE_YES 0
#define VOTE_NO 1
#define VOTE_ABSTAIN 2
char                   *shorttime( time_t updated );
bool                    valid_pfile( const char *filename );
char                   *distime( time_t updated );

typedef struct board_data BOARD_DATA;
void                    free_note( NOTE_DATA *pnote );

BOARD_DATA             *first_board,
                       *last_board;

typedef struct auction_data AUCTION_DATA;
struct auction_data
{
    AUCTION_DATA           *next,
                           *prev;
    char                   *auctstr;
    int                     count;
    int                     type;
};

AUCTION_DATA           *first_auction,
                       *last_auction;

bool can_read( CHAR_DATA *ch, BOARD_DATA * board )
{
    if ( !board )
        return false;

    if ( get_trust( ch ) >= board->min_read_level )
        return true;

    if ( board->read_group ) {
        if ( ch->pcdata->clan && !str_cmp( ch->pcdata->clan->name, board->read_group ) )
            return true;
        if ( ch->pcdata->council && !str_cmp( ch->pcdata->council->name, board->read_group ) )
            return true;
    }
    if ( board->extra_readers ) {
        if ( is_name( ch->name, board->extra_readers ) )
            return true;
    }
    return false;
}

bool can_remove( CHAR_DATA *ch, BOARD_DATA * board )
{
    if ( get_trust( ch ) >= board->min_remove_level )
        return true;

    if ( board->extra_removers ) {
        if ( is_name( ch->name, board->extra_removers ) )
            return true;
    }
    return false;
}

bool can_post( CHAR_DATA *ch, BOARD_DATA * board )
{
    if ( !board )
        return false;

    if ( get_trust( ch ) >= board->min_post_level )
        return true;

    if ( board->post_group ) {
        if ( ch->pcdata->clan && !str_cmp( ch->pcdata->clan->name, board->post_group ) )
            return true;
        if ( ch->pcdata->council && !str_cmp( ch->pcdata->council->name, board->post_group ) )
            return true;
    }
    return false;
}

BOARD_DATA             *get_board( CHAR_DATA *ch, int bnum )
{
    BOARD_DATA             *board;
    int                     bcount = 0;

    for ( board = first_board; board; board = board->next ) {
        if ( !can_read( ch, board ) && !can_post( ch, board ) && !can_remove( ch, board ) )
            continue;
        if ( ++bcount == bnum )
            return board;
    }
    return NULL;
}

void free_board( BOARD_DATA * board )
{
    NOTE_DATA              *pnote,
                           *next_note;

    STRFREE( board->extra_readers );
    STRFREE( board->extra_removers );
    STRFREE( board->read_group );
    STRFREE( board->post_group );
    STRFREE( board->extra_readers );
    STRFREE( board->extra_removers );
    STRFREE( board->note_file );

    for ( pnote = board->first_note; pnote; pnote = next_note ) {
        next_note = pnote->next;
        UNLINK( pnote, board->first_note, board->last_note, next, prev );
        free_note( pnote );
    }
    UNLINK( board, first_board, last_board, next, prev );
    DISPOSE( board );
}

void free_boards( void )
{
    BOARD_DATA             *board,
                           *board_next;

    for ( board = first_board; board; board = board_next ) {
        board_next = board->next;
        free_board( board );
    }
}

/* board commands. */
void write_boards_txt( void )
{
    BOARD_DATA             *tboard;
    FILE                   *fp;

    if ( !( fp = fopen( BOARD_FILE, "w" ) ) ) {
        bug( "%s: can't open %s for writing!", __FUNCTION__, BOARD_FILE );
        return;
    }
    for ( tboard = first_board; tboard; tboard = tboard->next ) {
        if ( !tboard->note_file )
            continue;
        fprintf( fp, "Filename            %s~\n", tboard->note_file );
        if ( tboard->min_read_level )
            fprintf( fp, "Min_read_level      %d\n", tboard->min_read_level );
        if ( tboard->min_post_level )
            fprintf( fp, "Min_post_level      %d\n", tboard->min_post_level );
        if ( tboard->min_remove_level )
            fprintf( fp, "Min_remove_level    %d\n", tboard->min_remove_level );
        if ( tboard->max_posts )
            fprintf( fp, "Max_posts           %d\n", tboard->max_posts );
        if ( tboard->read_group )
            fprintf( fp, "Read_group          %s~\n", tboard->read_group );
        if ( tboard->post_group )
            fprintf( fp, "Post_group          %s~\n", tboard->post_group );
        if ( tboard->extra_readers )
            fprintf( fp, "Extra_readers       %s~\n", tboard->extra_readers );
        if ( tboard->extra_removers )
            fprintf( fp, "Extra_removers      %s~\n", tboard->extra_removers );
        fprintf( fp, "End\n" );
    }
    fclose( fp );
    fp = NULL;
}

BOARD_DATA             *find_board( CHAR_DATA *ch )
{
    BOARD_DATA             *board;

    if ( ( board = get_board( ch, ch->pcdata->onboard ) ) )
        return board;

    return NULL;
}

bool is_note_to( CHAR_DATA *ch, NOTE_DATA *pnote )
{
    if ( !str_cmp( ch->name, pnote->sender ) )
        return true;

    if ( is_name( "todos", pnote->to_list ) )
        return true;

    if ( IS_IMMORTAL( ch )
         && ( is_name( "imm", pnote->to_list ) || is_name( "immortal", pnote->to_list ) ) )
        return true;

    if ( IS_AVATAR( ch )
         && ( is_name( "av", pnote->to_list ) || is_name( "avatar", pnote->to_list ) ) )
        return true;

    if ( is_name( ch->name, pnote->to_list ) )
        return true;

    if ( IS_CLANNED( ch ) && is_name( ch->pcdata->clan->name, pnote->to_list ) )
        return true;

    if ( IS_COUNCILED( ch ) && is_name( ch->pcdata->council->name, pnote->to_list ) )
        return true;

    return false;
}

void note_attach( CHAR_DATA *ch )
{
    NOTE_DATA              *pnote;

    if ( ch->pnote )
        return;

    CREATE( pnote, NOTE_DATA, 1 );
    pnote->next = pnote->prev = NULL;
    pnote->sender = QUICKLINK( ch->name );
    pnote->to_list = NULL;
    pnote->subject = NULL;
    pnote->text = NULL;
    pnote->first_vote = pnote->last_vote = NULL;
    pnote->obj = NULL;
    pnote->first_bid = pnote->last_bid = NULL;
    pnote->first_read = pnote->last_read = NULL;
    ch->pnote = pnote;
}

void gnote_attach( CHAR_DATA *ch )
{
    NOTE_DATA              *gnote;

    if ( !ch || !ch->pcdata || ch->pcdata->gnote )
        return;

    CREATE( gnote, NOTE_DATA, 1 );
    gnote->next = gnote->prev = NULL;
    gnote->sender = QUICKLINK( ch->name );
    gnote->to_list = NULL;
    gnote->subject = NULL;
    gnote->text = NULL;
    gnote->first_vote = gnote->last_vote = NULL;
    gnote->obj = NULL;
    gnote->first_bid = gnote->last_bid = NULL;
    gnote->first_read = gnote->last_read = NULL;
    ch->pcdata->gnote = gnote;
}

void write_board( BOARD_DATA * board )
{
    FILE                   *fp;
    NOTE_DATA              *pnote;
    VOTE_DATA              *vote;
    BID_DATA               *bid;
    READ_DATA              *read;
    char                    filename[256];

    snprintf( filename, sizeof( filename ), "%s%s", BOARD_DIR, board->note_file );
    if ( !board->first_note ) {
        remove_file( filename );
        return;
    }
    /*
     * Rewrite entire list.
     */
    if ( !( fp = fopen( filename, "w" ) ) ) {
        perror( filename );
        return;
    }
    for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
        if ( !pnote->text || !pnote->sender || !pnote->posttime )
            continue;
        if ( pnote->sender )
            fprintf( fp, "Sender      %s~\n", pnote->sender );
        if ( pnote->posttime )
            fprintf( fp, "PostTime    %ld\n", pnote->posttime );
        if ( pnote->to_list )
            fprintf( fp, "To          %s~\n", pnote->to_list );
        if ( pnote->subject )
            fprintf( fp, "Subject     %s~\n", pnote->subject );
        if ( pnote->voting )
            fprintf( fp, "Voting      %d\n", pnote->voting );
        for ( vote = pnote->first_vote; vote; vote = vote->next )
            fprintf( fp, "Vote        %s~ %d\n", vote->name, vote->vote );
        for ( bid = pnote->first_bid; bid; bid = bid->next )
            fprintf( fp, "Bid         %s~ %d\n", bid->name, bid->bid );
        for ( read = pnote->first_read; read; read = read->next )
            fprintf( fp, "Read        %s~\n", read->name );
        if ( pnote->aclosed )
            fprintf( fp, "%s\n", "AClosed" );
        if ( pnote->acanceled )
            fprintf( fp, "%s\n", "ACanceled" );
        if ( pnote->sfor )
            fprintf( fp, "SFor        %d\n", pnote->sfor );
        /*
         * Save the obj here
         */
        if ( pnote->obj ) {
            if ( pnote->autowin )
                fprintf( fp, "AutoWin     %d\n", pnote->autowin );
            fwrite_obj( NULL, pnote->obj, fp, 0, OS_AUCTION, false );
        }
        fprintf( fp, "Text\n%s~\n", strip_cr( pnote->text ) );
        fprintf( fp, "%s", "End\n\n" );
    }
    fclose( fp );
    fp = NULL;
}

BID_DATA               *has_bidded( NOTE_DATA *pnote, CHAR_DATA *ch )
{
    BID_DATA               *bid;

    if ( !pnote )
        return NULL;
    for ( bid = pnote->first_bid; bid; bid = bid->next ) {
        if ( !str_cmp( bid->name, ch->name ) )
            return bid;
    }
    return NULL;
}

BID_DATA               *check_high_bid( NOTE_DATA *pnote )
{
    BID_DATA               *bid,
                           *ubid = NULL;

    for ( bid = pnote->first_bid; bid; bid = bid->next ) {
        if ( !ubid )
            ubid = bid;
        if ( bid->bid > ubid->bid )
            ubid = bid;
    }
    return ubid;
}

void show_bids( NOTE_DATA *pnote, CHAR_DATA *ch )
{
    BID_DATA               *bid,
                           *hbid = NULL;

    if ( !( hbid = check_high_bid( pnote ) ) ) {
        send_to_char( "Actualmente no hay ofertas.\r\n", ch );
        return;
    }
    ch_printf( ch, "%s tiene la oferta más alta de %d.\r\n", hbid->name, hbid->bid );
    for ( bid = pnote->first_bid; bid; bid = bid->next ) {
        if ( bid == hbid )
            continue;
        ch_printf( ch, "%s ofertó %s.\r\n", bid->name, num_punct( bid->bid ) );
    }
}

READ_DATA              *has_read( NOTE_DATA *pnote, CHAR_DATA *ch )
{
    READ_DATA              *read;

    if ( !pnote )
        return NULL;
    for ( read = pnote->first_read; read; read = read->next ) {
        if ( !str_cmp( read->name, ch->name ) )
            return read;
    }
    return NULL;
}

int get_new_notes( BOARD_DATA * board, CHAR_DATA *ch )
{
    NOTE_DATA              *note;
    READ_DATA              *read;
    int                     nnew = 0;
    bool                    sound = FALSE;

    if ( !ch )
        return nnew;

    /*
     * If NULL check all boards
     */
    if ( !board ) {
        for ( board = first_board; board; board = board->next ) {
            if ( !can_read( ch, board ) )
                continue;

            for ( note = board->first_note; note; note = note->next ) {
                if ( get_trust( ch ) < LEVEL_IMMORTAL && !is_note_to( ch, note ) )
                    continue;
                if ( !( read = has_read( note, ch ) ) )
                    nnew++;
            }
        }
    }
    else {
        if ( can_read( ch, board ) ) {
            for ( note = board->first_note; note; note = note->next ) {
                if ( get_trust( ch ) < LEVEL_IMMORTAL && !is_note_to( ch, note ) )
                    continue;
// sound notification
                if ( !str_cmp( ch->name, note->to_list ) && xIS_SET( ch->act, PLR_COMMUNICATION )
                     && !has_read( note, ch ) )
                    sound = TRUE;

                if ( !( read = has_read( note, ch ) ) )
                    nnew++;
            }
        }
    }
    if ( sound == TRUE )
        send_to_char( "!!SOUND(sound/mail.wav)\r\n", ch );

    return nnew;
}

void show_unread_notes( CHAR_DATA *ch )
{
    int                     nnew = get_new_notes( NULL, ch );

    if ( nnew > 0 )
        ch_printf( ch,
                   "&[board]Hay %s &[board2]%d &[board]nota%s en el buzón sin leer.\r\n",
                   nnew == 1 ? "" : "", nnew, nnew != 1 ? "s" : "" );
}

void add_read( NOTE_DATA *pnote, CHAR_DATA *ch )
{
    READ_DATA              *read;

    if ( !pnote )
        return;
    if ( ( read = has_read( pnote, ch ) ) )
        return;
    CREATE( read, READ_DATA, 1 );
    read->name = STRALLOC( ch->name );
    LINK( read, pnote->first_read, pnote->last_read, next, prev );
}

void add_bid( NOTE_DATA *pnote, CHAR_DATA *ch, int amount )
{
    BID_DATA               *bid;

    if ( !pnote )
        return;
    if ( ( bid = has_bidded( pnote, ch ) ) ) {
        bid->bid = amount;
        return;
    }
    CREATE( bid, BID_DATA, 1 );
    bid->name = STRALLOC( ch->name );
    bid->bid = amount;
    LINK( bid, pnote->first_bid, pnote->last_bid, next, prev );
}

/* Have they voted on it already? */
VOTE_DATA              *has_voted( NOTE_DATA *pnote, CHAR_DATA *ch )
{
    VOTE_DATA              *vote;

    for ( vote = pnote->first_vote; vote; vote = vote->next ) {
        if ( !str_cmp( vote->name, ch->name ) )
            return vote;
    }
    return NULL;
}

void add_vote( NOTE_DATA *pnote, CHAR_DATA *ch, short type )
{
    VOTE_DATA              *vote;

    if ( !pnote )
        return;
    /*
     * See if they already voted if so update the note and the way they voted on it
     */
    if ( ( vote = has_voted( pnote, ch ) ) ) {
        if ( vote->vote == VOTE_YES )
            --pnote->yesvotes;
        if ( vote->vote == VOTE_NO )
            --pnote->novotes;
        if ( vote->vote == VOTE_ABSTAIN )
            --pnote->abstentions;
        vote->vote = type;
        return;
    }
    CREATE( vote, VOTE_DATA, 1 );
    vote->name = STRALLOC( ch->name );
    vote->vote = type;
    LINK( vote, pnote->first_vote, pnote->last_vote, next, prev );
}

void free_vote( VOTE_DATA * vote )
{
    if ( !vote )
        return;
    STRFREE( vote->name );
    DISPOSE( vote );
}

void free_votes( NOTE_DATA *pnote )
{
    VOTE_DATA              *vote,
                           *nextvote;

    if ( !pnote )
        return;
    for ( vote = pnote->first_vote; vote; vote = nextvote ) {
        nextvote = vote->next;
        UNLINK( vote, pnote->first_vote, pnote->last_vote, next, prev );
        free_vote( vote );
    }
}

void free_bid( BID_DATA * bid )
{
    if ( !bid )
        return;
    STRFREE( bid->name );
    DISPOSE( bid );
}

void free_bids( NOTE_DATA *pnote )
{
    BID_DATA               *bid,
                           *nextbid;

    if ( !pnote )
        return;
    for ( bid = pnote->first_bid; bid; bid = nextbid ) {
        nextbid = bid->next;
        UNLINK( bid, pnote->first_bid, pnote->last_bid, next, prev );
        free_bid( bid );
    }
}

void free_read( READ_DATA * read )
{
    if ( !read )
        return;
    STRFREE( read->name );
    DISPOSE( read );
}

void free_reads( NOTE_DATA *pnote )
{
    READ_DATA              *read,
                           *nextread;

    if ( !pnote )
        return;
    for ( read = pnote->first_read; read; read = nextread ) {
        nextread = read->next;
        UNLINK( read, pnote->first_read, pnote->last_read, next, prev );
        free_read( read );
    }
}

void free_note( NOTE_DATA *pnote )
{
    STRFREE( pnote->text );
    STRFREE( pnote->subject );
    STRFREE( pnote->to_list );
    STRFREE( pnote->sender );
    pnote->obj = NULL;
    free_bids( pnote );
    free_votes( pnote );
    free_reads( pnote );
    DISPOSE( pnote );
}

void note_remove( BOARD_DATA * board, NOTE_DATA *pnote )
{
    if ( !board ) {
        bug( "%s: null board", __FUNCTION__ );
        return;
    }

    if ( !pnote ) {
        bug( "%s: null pnote", __FUNCTION__ );
        return;
    }

    UNLINK( pnote, board->first_note, board->last_note, next, prev );
    --board->num_posts;
    free_note( pnote );
    write_board( board );
}

void do_noteroom( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;
    char                    arg[MSL],
                            arg_passed[MSL];

    mudstrlcpy( arg_passed, argument, sizeof( arg_passed ) );

    switch ( ch->substate ) {
        case SUB_WRITING_NOTE:
            do_note( ch, arg_passed );
            break;

        default:
            argument = one_argument( argument, arg );
            if ( !str_cmp( arg, "escribir" ) || !str_cmp( arg, "para" )
                 || !str_cmp( arg, "asunto" ) || !str_cmp( arg, "mostrar" ) ) {
                do_note( ch, arg_passed );
                return;
            }

            if ( !( board = find_board( ch ) ) ) {
                send_to_char( "No hay ningún buzón que mirar.\r\n", ch );
                return;
            }

            do_note( ch, arg_passed );
            return;
    }
}

void read_note( CHAR_DATA *ch, int count, BOARD_DATA * board, NOTE_DATA *pnote )
{
    const char             *color1,
                           *color2;

    if ( !ch || !pnote || !board )
        return;

    color1 = color_str( AT_NOTE, ch );
    color2 = color_str( AT_DGREEN, ch );

    pager_printf( ch, "%s[%s%3d%s] %s%s%s: %s%s\r\n%s\r\n%sPara: %s%s\r\n",
                  color1, color2, count, color1, color2, pnote->sender, color1, color2,
                  pnote->subject, distime( pnote->posttime ), color1, color2, pnote->to_list );
    if ( pnote->yesvotes || pnote->novotes || pnote->abstentions )
        pager_printf( ch, "%sVotos: Si: %s%d %sNo: %s%d %sAbtención: %s%d\r\n",
                      color1, color2, pnote->yesvotes, color1, color2, pnote->novotes, color1,
                      color2, pnote->abstentions );
    if ( !IS_BLIND( ch ) ) {
        pager_printf( ch,
                      "%s~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~`~.~\r\n",
                      color1 );
    }
    pager_printf( ch, "%s%s", color2, pnote->text );
    add_read( pnote, ch );
    write_board( board );
    act( AT_ACTION, "$n lee una nota.", ch, NULL, NULL, TO_CANSEE );
}

void read_next_unread( CHAR_DATA *ch )
{
    BOARD_DATA             *board;
    NOTE_DATA              *note;
    int                     count;

    if ( !ch )
        return;

    for ( board = first_board; board; board = board->next ) {
        if ( !can_read( ch, board ) )
            continue;

        count = 0;
        for ( note = board->first_note; note; note = note->next ) {
            if ( get_trust( ch ) < LEVEL_IMMORTAL && !is_note_to( ch, note ) )
                continue;

            count++;
            if ( has_read( note, ch ) )
                continue;

            /*
             * Ok so we got here so time to show the note
             */
            /*
             * Show a message for what board we are in also
             */
            pager_printf( ch, "leyendo una nota del buzón %s.\r\n",
                          board->note_file ? board->note_file : "(Not Set)" );
            read_note( ch, count, board, note );
            return;
        }
    }

    /*
     * If they got here no new notes to read
     */
    send_to_pager( "ya has leído todas las notas que puedes leer.\r\n", ch );
}

void do_note( CHAR_DATA *ch, char *arg_passed )
{
    BOARD_DATA             *board;
    NOTE_DATA              *pnote = NULL;
    const char             *color1,
                           *color2;
    char                    arg[MIL],
                            buf[MIL];
    int                     vnum,
                            anum,
                            first_list;
    bool                    mfound = false;

    if ( IS_NPC( ch ) )
        return;

    if ( !ch->desc ) {
        bug( "%s: no descriptor", __FUNCTION__ );
        return;
    }

    switch ( ch->substate ) {
        default:
            break;

        case SUB_GNOTE:
            if ( !( pnote = ( NOTE_DATA * ) ch->dest_buf ) ) {
                bug( "%s: NULL ch->dest_buf", __FUNCTION__ );
                return;
            }
            if ( !( board = find_board( ch ) ) ) {
                bug( "%s: NULL BOARD", __FUNCTION__ );
                return;
            }
            STRFREE( pnote->text );
            pnote->text = copy_buffer( ch );
            stop_editing( ch );
            write_board( board );
            return;

        case SUB_WRITING_NOTE:
            if ( ch->dest_buf != ch->pcdata->gnote ) {
                bug( "%s: ch->dest_buf != ch->pcdata->gnote", __FUNCTION__ );
                return;
            }
            STRFREE( ch->pcdata->gnote->text );
            ch->pcdata->gnote->text = copy_buffer( ch );
            stop_editing( ch );
            return;
    }

    color1 = color_str( AT_NOTE, ch );
    color2 = color_str( AT_DGREEN, ch );

    set_char_color( AT_NOTE, ch );
    arg_passed = one_argument( arg_passed, arg );

    if ( arg == NULL || arg[0] == '\0' || !str_cmp( arg, "list" ) ) {
        if ( !( board = find_board( ch ) ) ) {
            send_to_char( "No hay buzones que mirar.\r\n", ch );
            return;
        }

        if ( !can_read( ch, board ) ) {
            send_to_char( "No le ves sentido...\r\n",
                          ch );
            return;
        }

        if ( ( first_list = atoi( arg_passed ) ) ) {
            if ( first_list < 1 ) {
                send_to_char( "¡No puedes leer una nota inferior a la 1!\r\n", ch );
                return;
            }
        }

        set_pager_color( AT_NOTE, ch );
        vnum = 0;
        ch_printf( ch, "Notas en el buzón %s.\r\n",
                   board->note_file ? board->note_file : "(Not Set)" );

        if ( !board->first_note ) {
            ch_printf( ch, "%sNo hay notas en este buzón.\r\n", color2 );
            return;
        }

        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            if ( !is_note_to( ch, pnote ) && get_trust( ch ) < LEVEL_IMMORTAL )
                continue;

            vnum++;
            if ( first_list && vnum < first_list )
                continue;

            if ( vnum > ( first_list + 15 ) ) {
                send_to_pager( "Se muestran 15 notas a la vez.\r\n", ch );
                break;
            }

            mfound = true;
            pager_printf( ch, "%s%3d%s %s%10.10s %s%12.12s %spara %s%-12.12s %s: %s%15.15s%3s\r\n",
                          color2, vnum, color1, color2, shorttime( pnote->posttime ),
                          color2, pnote->sender ? pnote->sender : "(No Sender)", color1,
                          color2, pnote->to_list, color1, color2,
                          pnote->subject ? pnote->subject : "(No Subject)", ( pnote->subject
                                                                              &&
                                                                              strlen
                                                                              ( pnote->subject ) >
                                                                              15 ) ? "..." : "" );
            if ( pnote->voting != VOTE_NONE ) {
                pager_printf( ch, "     %sVotos (%s%s%s):",
                              color1, color2, pnote->voting == VOTE_OPEN ? "abiertos" : "cerrados",
                              color1 );
                if ( pnote->yesvotes )
                    pager_printf( ch, " %sSi: %s%3d", color1, color2, pnote->yesvotes );
                if ( pnote->novotes )
                    pager_printf( ch, " %sNo: %s%3d", color1, color2, pnote->novotes );
                if ( pnote->abstentions )
                    pager_printf( ch, " %sAbtenciones: %s%3d", color1, color2, pnote->abstentions );
                send_to_pager( "\r\n", ch );
            }
        }

        act( AT_ACTION, "$n echa un vistazo a las notas.", ch, NULL, NULL, TO_CANSEE );
        return;
    }

    if ( !str_cmp( arg, "noleidas" ) ) {
        read_next_unread( ch );
        return;
    }

    if ( !str_cmp( arg, "leer" ) ) {
        if ( !( board = find_board( ch ) ) ) {
            send_to_char( "no hay ningún buzón que mirar.\r\n", ch );
            return;
        }

        if ( !can_read( ch, board ) ) {
            send_to_char( "no le ves sentido...\r\n",
                          ch );
            return;
        }

        if ( is_number( arg_passed ) )
            anum = atoi( arg_passed );
        else {
            send_to_char( "¿qué número de nota?\r\n", ch );
            return;
        }

        set_pager_color( AT_NOTE, ch );
        vnum = 0;
        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            if ( get_trust( ch ) < LEVEL_IMMORTAL && !is_note_to( ch, pnote ) )
                continue;

            if ( ++vnum != anum )
                continue;

            read_note( ch, vnum, board, pnote );
            return;
        }
        send_to_char( "No existe esa nota.\r\n", ch );
        return;
    }

    /*
     * Voting added by Narn, June '96
     */
    if ( !str_cmp( arg, "votar" ) ) {
        char                    arg2[MIL];

        arg_passed = one_argument( arg_passed, arg2 );

        if ( !( board = find_board( ch ) ) ) {
            send_to_char( "No hay ningún buzón aquí.\r\n", ch );
            return;
        }
        if ( !can_read( ch, board ) ) {
            send_to_char( "No puedes botar esta nota.\r\n", ch );
            return;
        }

        if ( is_number( arg2 ) )
            anum = atoi( arg2 );
        else {
            send_to_char( "¿por cuál nota quieres botar?\r\n", ch );
            return;
        }

        vnum = 1;
        for ( pnote = board->first_note; pnote && vnum < anum; pnote = pnote->next ) {
            if ( get_trust( ch ) < LEVEL_IMMORTAL && !is_note_to( ch, pnote ) )
                continue;
            vnum++;
        }
        if ( !pnote ) {
            send_to_char( "No existe esa nota.\r\n", ch );
            return;
        }

        /*
         * If you're the author of the note and can read the board you can open
         * and close voting, if you can read it and voting is open you can vote.
         */
        if ( !str_cmp( arg_passed, "abrir" ) ) {
            if ( str_cmp( ch->name, pnote->sender ) ) {
                send_to_char( "No eres el autor de la nota.\r\n", ch );
                return;
            }
            pnote->voting = VOTE_OPEN;
            act( AT_ACTION, "$n abre las votaciones de una nota.", ch, NULL, NULL, TO_CANSEE );
            send_to_char( "Abierta.\r\n", ch );
            write_board( board );
            return;
        }
        if ( !str_cmp( arg_passed, "cerrar" ) ) {
            if ( str_cmp( ch->name, pnote->sender ) ) {
                send_to_char( "No eres el autor de la nota.\r\n", ch );
                return;
            }
            pnote->voting = VOTE_CLOSED;
            act( AT_ACTION, "$n cierra las votaciones de una nota.", ch, NULL, NULL, TO_CANSEE );
            send_to_char( "Cerradas.\r\n", ch );
            write_board( board );
            return;
        }

        if ( pnote->voting != VOTE_OPEN ) {
            send_to_char( "No está abierta.\r\n", ch );
            return;
        }

        if ( !str_cmp( arg_passed, "si" ) ) {
            ++pnote->yesvotes;
            add_vote( pnote, ch, VOTE_YES );
            act( AT_ACTION, "$n vota en una nota.", ch, NULL, NULL, TO_CANSEE );
            send_to_char( "Ok.\r\n", ch );
            write_board( board );
            return;
        }
        if ( !str_cmp( arg_passed, "no" ) ) {
            ++pnote->novotes;
            add_vote( pnote, ch, VOTE_NO );
            act( AT_ACTION, "$n vota en una nota.", ch, NULL, NULL, TO_CANSEE );
            send_to_char( "Ok.\r\n", ch );
            write_board( board );
            return;
        }
        if ( !str_cmp( arg_passed, "abtenerse" ) ) {
            ++pnote->abstentions;
            add_vote( pnote, ch, VOTE_ABSTAIN );
            act( AT_ACTION, "$n vota en una nota.", ch, NULL, NULL, TO_CANSEE );
            send_to_char( "Ok.\r\n", ch );
            write_board( board );
            return;
        }

        /*
         * Lets display the results if we get this far
         */
        {
            VOTE_DATA              *vote;
            int                     voted,
                                    col;

            for ( voted = 0; voted < 3; voted++ ) {
                col = 0;
                pager_printf( ch, "%s votos:\r\n",
                              voted == 0 ? "si" : voted == 1 ? "No" : "Abtención" );
                for ( vote = pnote->first_vote; vote; vote = vote->next ) {
                    if ( vote->vote == voted ) {
                        pager_printf( ch, "   %10.10s", vote->name );
                        if ( ++col == 3 ) {
                            col = 0;
                            send_to_pager( "\r\n", ch );
                        }
                    }
                }
                if ( col != 0 )
                    send_to_pager( "\r\n", ch );
            }
        }
        return;
    }

    if ( !str_cmp( arg, "escribir" ) ) {
        if ( ch->substate == SUB_RESTRICTED ) {
            send_to_char( "No puedes.\r\n", ch );
            return;
        }
        gnote_attach( ch );
        ch->substate = SUB_WRITING_NOTE;
        ch->dest_buf = ch->pcdata->gnote;
        start_editing( ch, ch->pcdata->gnote->text );
        return;
    }

    if ( !str_cmp( arg, "editar" ) ) {
        if ( !( board = find_board( ch ) ) ) {
            send_to_char( "no hay ningún buzón que mirar.\r\n", ch );
            return;
        }

        if ( !can_read( ch, board ) ) {
            send_to_char( "no le ves sentido...\r\n",
                          ch );
            return;
        }

        if ( is_number( arg_passed ) )
            anum = atoi( arg_passed );
        else {
            send_to_char( "¿Editar qué nota?\r\n", ch );
            return;
        }

        set_pager_color( AT_NOTE, ch );
        vnum = 0;
        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            if ( get_trust( ch ) < LEVEL_IMMORTAL && !is_note_to( ch, pnote ) )
                continue;

            if ( ++vnum != anum )
                continue;

            if ( get_trust( ch ) < LEVEL_IMMORTAL && str_cmp( ch->name, pnote->sender ) ) {
                send_to_char( "No puedes editar esa nota.\r\n", ch );
                return;
            }

            ch->substate = SUB_GNOTE;
            ch->dest_buf = pnote;
            start_editing( ch, pnote->text );
            act( AT_ACTION, "$n comienza a editar una nota.", ch, NULL, NULL, TO_CANSEE );
            return;
        }
        send_to_char( "No existe esa nota.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "asunto" ) ) {
        if ( !arg_passed || arg_passed[0] == '\0' ) {
            send_to_char( "¿Y el texto del asunto?\r\n", ch );
            return;
        }
        gnote_attach( ch );
        if ( !ch->pcdata->gnote ) {
            send_to_char( "No tienes ninguna nota empezada\r\n", ch );
            return;
        }
        STRSET( ch->pcdata->gnote->subject, arg_passed );
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "para" ) ) {
        if ( !arg_passed || arg_passed[0] == '\0' ) {
            send_to_char( "Espedifica un destinatario.\r\n", ch );
            return;
        }

        arg_passed[0] = UPPER( arg_passed[0] );
        gnote_attach( ch );
        if ( !ch->pcdata->gnote ) {
            send_to_char( "No tienes notas  que continuar\r\n", ch );
            return;
        }
        STRSET( ch->pcdata->gnote->to_list, arg_passed );
        send_to_char( "Ok.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "mostrar" ) ) {
        if ( !ch->pcdata->gnote ) {
            send_to_char( "No tienes notas que mostrar\r\n", ch );
            return;
        }
        ch_printf( ch, "Para: %s\r\n",
                   ch->pcdata->gnote->to_list ? ch->pcdata->gnote->to_list : "(No especificado)" );
        ch_printf( ch, "Asunto: %s\r\n",
                   ch->pcdata->gnote->subject ? ch->pcdata->gnote->subject : "(No especificado)" );
        ch_printf( ch, "%s\r\n", ch->pcdata->gnote->text ? ch->pcdata->gnote->text : "(Not Set)" );
        return;
    }

    if ( !str_cmp( arg, "enviar" ) ) {
        if ( !ch->pcdata->gnote ) {
            send_to_char( "No tienes notas que enviar\r\n", ch );
            return;
        }

        if ( !( board = find_board( ch ) ) ) {
            send_to_char( "No hay buzones donde enviar tu nota.\r\n", ch );
            return;
        }

        if ( !can_post( ch, board ) ) {
            send_to_char( "Una fuerza mágica te detiene...\r\n", ch );
            return;
        }

        if ( board->num_posts >= board->max_posts ) {
            send_to_char( "No hay lugar aquí para tu nota.\r\n", ch );
            return;
        }

        if ( !ch->pcdata->gnote->to_list ) {
            send_to_char( "Tu nota no va dirigida a nadie.\r\n", ch );
            return;
        }

        if ( !ch->pcdata->gnote->subject ) {
            send_to_char( "Tu nota no tiene asunto.\r\n", ch );
            return;
        }

        if ( !ch->pcdata->gnote->text ) {
            send_to_char( "Tu nota no contiene texto alguno.\r\n", ch );
            return;
        }

        CREATE( pnote, NOTE_DATA, 1 );
        pnote->posttime = current_time;
        pnote->text = STRALLOC( ch->pcdata->gnote->text );
        STRFREE( ch->pcdata->gnote->text );
        pnote->to_list = STRALLOC( ch->pcdata->gnote->to_list );
        STRFREE( ch->pcdata->gnote->to_list );
        pnote->subject = STRALLOC( ch->pcdata->gnote->subject );
        STRFREE( ch->pcdata->gnote->subject );
        pnote->sender = STRALLOC( ch->pcdata->gnote->sender );
        STRFREE( ch->pcdata->gnote->sender );
        DISPOSE( ch->pcdata->gnote );
        ch->pcdata->gnote = NULL;
        pnote->voting = 0;
        pnote->yesvotes = 0;
        pnote->novotes = 0;
        pnote->abstentions = 0;
        pnote->first_vote = pnote->last_vote = NULL;
        pnote->first_bid = pnote->last_bid = NULL;
        pnote->first_read = pnote->last_read = NULL;
        pnote->obj = NULL;
        pnote->autowin = 0;
        pnote->aclosed = false;
        pnote->acanceled = false;
        pnote->sfor = 0;

        LINK( pnote, board->first_note, board->last_note, next, prev );
        board->num_posts++;
        write_board( board );

        /*
         * Send the message about a note being posted to the mud
         */
        snprintf( buf, MIL, "¡%s ha enviado una nota en el buzón %s con asunto %s para %s!&D",
                  ch->name, board->note_file, pnote->subject, pnote->to_list );
        announce( buf );

        send_to_char( "Envías tu nota.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "limpiar" ) ) {
        if ( !ch->pcdata->gnote ) {
            send_to_char( "No tienes notas continuadas\r\n", ch );
            return;
        }
        STRFREE( ch->pcdata->gnote->text );
        STRFREE( ch->pcdata->gnote->subject );
        STRFREE( ch->pcdata->gnote->to_list );
        STRFREE( ch->pcdata->gnote->sender );
        ch->pcdata->gnote->obj = NULL;
        free_votes( ch->pcdata->gnote );
        free_bids( ch->pcdata->gnote );
        free_reads( ch->pcdata->gnote );
        DISPOSE( ch->pcdata->gnote );
        ch->pcdata->gnote = NULL;
        send_to_char( "Limpiada.\r\n", ch );
        return;
    }

    if ( !str_cmp( arg, "eliminar" ) ) {
        if ( !( board = find_board( ch ) ) ) {
            send_to_char( "¡No hay buz´pon del que eliminar nada!\r\n", ch );
            return;
        }

        if ( !is_number( arg_passed ) ) {
            send_to_char( "¿Eliminar que nota?\r\n", ch );
            return;
        }

        if ( !can_read( ch, board ) ) {
            send_to_char
                ( "¡No le ves sentido!\r\n",
                  ch );
            return;
        }

        anum = atoi( arg_passed );
        vnum = 0;
        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            if ( get_trust( ch ) < LEVEL_IMMORTAL && !is_note_to( ch, pnote ) )
                continue;
            if ( ++vnum != anum )
                continue;
            if ( !is_note_to( ch, pnote ) && !can_remove( ch, board ) ) {
                send_to_char( "¡No se te permite borrar esa nota!\r\n", ch );
                return;
            }
            note_remove( board, pnote );
            ch_printf( ch, "Nota %d eliminada.\r\n", vnum );
            /*
             * Send the message to the mud
             */
            act( AT_ACTION, "$n elimina una nota.", ch, NULL, NULL, TO_CANSEE );
            return;
        }

        send_to_char( "No existe esa nota.\r\n", ch );
        return;
    }

    send_to_char( "Huh?  Teclea 'ayuda nota' para saber como usar el comando.\r\n", ch );
}

BOARD_DATA             *read_board( FILE * fp )
{
    BOARD_DATA             *board;
    const char             *word;
    char                    letter;
    bool                    fMatch;

    do {
        letter = getc( fp );
        if ( feof( fp ) ) {
            fclose( fp );
            return NULL;
        }
    }
    while ( isspace( ( int ) letter ) );
    ungetc( letter, fp );

    CREATE( board, BOARD_DATA, 1 );

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = false;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = true;
                fread_to_eol( fp );
                break;

            case 'E':
                KEY( "Extra_readers", board->extra_readers, fread_string( fp ) );
                KEY( "Extra_removers", board->extra_removers, fread_string( fp ) );
                if ( !str_cmp( word, "End" ) ) {
                    board->num_posts = 0;
                    board->first_note = NULL;
                    board->last_note = NULL;
                    board->next = NULL;
                    board->prev = NULL;
                    return board;
                }
                break;

            case 'F':
                KEY( "Filename", board->note_file, fread_string( fp ) );
                break;

            case 'M':
                KEY( "Min_read_level", board->min_read_level, fread_number( fp ) );
                KEY( "Min_post_level", board->min_post_level, fread_number( fp ) );
                KEY( "Min_remove_level", board->min_remove_level, fread_number( fp ) );
                KEY( "Max_posts", board->max_posts, fread_number( fp ) );
                break;

            case 'P':
                KEY( "Post_group", board->post_group, fread_string( fp ) );
                break;

            case 'R':
                KEY( "Read_group", board->read_group, fread_string( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
        }
    }
    free_board( board );
    return NULL;
}

NOTE_DATA              *read_note( FILE * fp )
{
    NOTE_DATA              *pnote;
    VOTE_DATA              *vote;
    BID_DATA               *bid;
    READ_DATA              *read;
    const char             *word;
    char                    letter;
    bool                    fMatch;

    /*
     * Have to see if we are at the end of the file
     */
    do {
        letter = getc( fp );
        if ( feof( fp ) ) {
            fclose( fp );
            return NULL;
        }
    }
    while ( isspace( ( int ) letter ) );
    ungetc( letter, fp );

    CREATE( pnote, NOTE_DATA, 1 );

    pnote->yesvotes = 0;
    pnote->novotes = 0;
    pnote->abstentions = 0;
    pnote->first_vote = pnote->last_vote = NULL;
    pnote->first_bid = pnote->last_bid = NULL;
    pnote->first_read = pnote->last_read = NULL;
    pnote->obj = NULL;
    pnote->autowin = 0;
    pnote->aclosed = false;
    pnote->acanceled = false;
    pnote->sfor = 0;

    for ( ;; ) {
        word = feof( fp ) ? "End" : fread_word( fp );
        fMatch = false;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fMatch = true;
                fread_to_eol( fp );
                break;

            case '#':
                if ( !strcmp( word, "#OBJECT" ) ) {    /* Objects */
                    fread_obj( NULL, pnote, fp, OS_AUCTION );
                    fMatch = true;
                    break;
                }
                break;

            case 'A':
                if ( !str_cmp( word, "AClosed" ) ) {
                    pnote->aclosed = true;
                    fMatch = true;
                    break;
                }
                if ( !str_cmp( word, "ACanceled" ) ) {
                    pnote->acanceled = true;
                    fMatch = true;
                    break;
                }
                KEY( "AutoWin", pnote->autowin, fread_number( fp ) );
                break;

            case 'B':
                if ( !str_cmp( word, "Bid" ) ) {
                    CREATE( bid, BID_DATA, 1 );
                    bid->name = fread_string( fp );
                    bid->bid = fread_number( fp );
                    if ( !valid_pfile( bid->name ) )
                        free_bid( bid );
                    else
                        LINK( bid, pnote->first_bid, pnote->last_bid, next, prev );
                    fMatch = true;
                    break;
                }
                break;

            case 'E':
                if ( !strcmp( word, "End" ) ) {
                    pnote->next = NULL;
                    pnote->prev = NULL;
                    if ( pnote->obj )
                        pnote->obj->auctioned = true;
                    return pnote;
                }
                break;

            case 'N':
                if ( !str_cmp( word, "NSFor" ) ) {
                    fread_number( fp );
                    pnote->sfor = fread_number( fp );
                    fMatch = true;
                    break;
                }
                if ( !str_cmp( word, "NAutoWin" ) ) {
                    fread_number( fp );
                    pnote->autowin = fread_number( fp );
                    fMatch = true;
                    break;
                }
                if ( !str_cmp( word, "NBid" ) ) {
                    CREATE( bid, BID_DATA, 1 );
                    bid->name = fread_string( fp );
                    fread_number( fp );
                    bid->bid = fread_number( fp );
                    if ( !valid_pfile( bid->name ) )
                        free_bid( bid );
                    else
                        LINK( bid, pnote->first_bid, pnote->last_bid, next, prev );
                    fMatch = true;
                    break;
                }
                break;

            case 'P':
                KEY( "PostTime", pnote->posttime, fread_time( fp ) );
                break;

            case 'R':
                if ( !str_cmp( word, "Read" ) ) {
                    CREATE( read, READ_DATA, 1 );
                    read->name = fread_string( fp );
                    if ( !valid_pfile( read->name ) )
                        free_read( read );
                    else
                        LINK( read, pnote->first_read, pnote->last_read, next, prev );
                    fMatch = true;
                    break;
                }
                break;

            case 'S':
                KEY( "SFor", pnote->sfor, fread_number( fp ) );
                KEY( "Sender", pnote->sender, fread_string( fp ) );
                KEY( "Subject", pnote->subject, fread_string( fp ) );
                break;

            case 'T':
                KEY( "To", pnote->to_list, fread_string( fp ) );
                KEY( "Text", pnote->text, fread_string( fp ) );
                break;

            case 'V':
                if ( !str_cmp( word, "Vote" ) ) {
                    CREATE( vote, VOTE_DATA, 1 );
                    vote->name = fread_string( fp );
                    vote->vote = fread_number( fp );
                    if ( !valid_pfile( vote->name ) )
                        free_vote( vote );
                    else {
                        LINK( vote, pnote->first_vote, pnote->last_vote, next, prev );
                        if ( vote->vote == VOTE_NO )
                            pnote->novotes++;
                        if ( vote->vote == VOTE_YES )
                            pnote->yesvotes++;
                        if ( vote->vote == VOTE_ABSTAIN )
                            pnote->abstentions++;
                    }
                    fMatch = true;
                    break;
                }
                KEY( "Voting", pnote->voting, fread_number( fp ) );
                break;
        }
        if ( !fMatch ) {
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
        }
    }
    free_note( pnote );
    return NULL;
}

/* Load boards file. */
void load_boards( void )
{
    FILE                   *board_fp,
                           *note_fp;
    BOARD_DATA             *board;
    NOTE_DATA              *pnote;
    char                    notefile[MIL];

    first_board = last_board = NULL;

    if ( !( board_fp = fopen( BOARD_FILE, "r" ) ) )
        return;

    while ( ( board = read_board( board_fp ) ) ) {
        LINK( board, first_board, last_board, next, prev );
        snprintf( notefile, sizeof( notefile ), "%s%s", BOARD_DIR, board->note_file );
        log_string( notefile );
        if ( ( note_fp = fopen( notefile, "r" ) ) ) {
            while ( ( pnote = read_note( note_fp ) ) ) {
                LINK( pnote, board->first_note, board->last_note, next, prev );
                board->num_posts++;
            }
        }
    }
}

void board_stat( CHAR_DATA *ch, BOARD_DATA * board )
{
    if ( !board ) {
        send_to_char( "No hay más buzones para mostrar.\r\n", ch );
        return;
    }

    ch_printf( ch, "\r\n&GFilename: &W%-15.15s &GRead: &W%d  &GPost: &W%d  &GRemove: &W%d\r\n",
               board->note_file, board->min_read_level, board->min_post_level,
               board->min_remove_level );
    ch_printf( ch, "&GMaxpost:        &W%-3d\r\n", board->max_posts );
    ch_printf( ch, "&GPosts:          &W%d\r\n", board->num_posts );
    ch_printf( ch, "&GRead_group:     &W%s\r\n", board->read_group ? board->read_group : "None" );
    ch_printf( ch, "&GPost_group:     &W%s\r\n", board->post_group ? board->post_group : "None" );
    ch_printf( ch, "&GExtra_readers:  &W%s\r\n",
               board->extra_readers ? board->extra_readers : "None" );
    ch_printf( ch, "&GExtra_removers: &W%s\r\n",
               board->extra_removers ? board->extra_removers : "None" );
}

void do_bset( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;
    char                    arg1[MIL],
                            arg2[MIL],
                            buf[MSL];
    int                     value,
                            bcount = 0;
    bool                    found,
                            create = false;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    set_char_color( AT_NOTE, ch );
    if ( arg1 == NULL || arg1[0] == '\0' ) {
        send_to_char( "Usage: bset <board filename> [create]\r\n", ch );
        send_to_char( "Usage: bset <board filename> <field> <value>\r\n", ch );
        send_to_char( "\r\nField being one of:\r\n", ch );
        send_to_char( "  post     read        remove     post_group     extra_removers\r\n", ch );
        send_to_char( "  maxpost  read_group  filename   extra_readers\r\n", ch );
        return;
    }

    value = atoi( argument );
    found = false;

    if ( !str_cmp( arg2, "create" ) )
        create = true;

    bcount = atoi( arg1 );
    if ( !( board = get_board( ch, bcount ) ) ) {
        bcount = 0;
        for ( board = first_board; board; board = board->next ) {
            if ( !can_read( ch, board ) && !can_post( ch, board ) && !can_remove( ch, board ) )
                continue;
            bcount++;
            if ( !str_cmp( board->note_file, arg1 ) )
                break;
        }
        if ( !board && !create ) {
            send_to_char( "No existe ese buzón.\r\n", ch );
            return;
        }
    }

    if ( arg2 == NULL || arg2[0] == '\0' ) {
        board_stat( ch, board );
        return;
    }

    if ( create ) {
        if ( board ) {
            send_to_char( "El buzón está listo para usarse.\r\n", ch );
            return;
        }

        arg1[0] = UPPER( arg1[0] );
        if ( !can_use_path( ch, BOARD_DIR, arg1 ) )
            return;

        CREATE( board, BOARD_DATA, 1 );
        if ( !board ) {
            bug( "%s: failed to CREATE board.", __FUNCTION__ );
            return;
        }
        LINK( board, first_board, last_board, next, prev );
        board->note_file = STRALLOC( arg1 );
        board->first_note = NULL;
        board->last_note = NULL;
        board->read_group = NULL;
        board->post_group = NULL;
        board->extra_readers = NULL;
        board->extra_removers = NULL;
        write_boards_txt(  );
        ch_printf( ch, "%s board created.\r\n", board->note_file );
        return;
    }

    if ( !str_cmp( arg2, "read" ) ) {
        board->min_read_level = URANGE( 0, value, ( MAX_LEVEL - 1 ) );
        write_boards_txt(  );
        ch_printf( ch, "Read level set to %d.\r\n", board->min_read_level );
        return;
    }

    if ( !str_cmp( arg2, "read_group" ) ) {
        STRSET( board->read_group, argument );
        write_boards_txt(  );
        ch_printf( ch, "Read_group %s.\r\n", board->read_group ? "set" : "cleared" );
        return;
    }

    if ( !str_cmp( arg2, "post_group" ) ) {
        STRSET( board->post_group, argument );
        write_boards_txt(  );
        ch_printf( ch, "Post_group %s.\r\n", board->post_group ? "set" : "cleared" );
        return;
    }

    if ( !str_cmp( arg2, "extra_removers" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "No hay nombre específico.\r\n", ch );
            return;
        }
        if ( !str_cmp( argument, "none" ) )
            STRFREE( board->extra_removers );
        else {
            if ( board->extra_removers )
                snprintf( buf, sizeof( buf ), "%s %s", board->extra_removers, argument );
            else
                snprintf( buf, sizeof( buf ), "%s", argument );
            STRSET( board->extra_removers, buf );
        }
        write_boards_txt(  );
        send_to_char( "Done.  (extra removers set)\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "extra_readers" ) ) {
        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "No hay nombre especificado.\r\n", ch );
            return;
        }
        if ( !str_cmp( argument, "none" ) )
            STRFREE( board->extra_readers );
        else {
            if ( board->extra_readers )
                snprintf( buf, sizeof( buf ), "%s %s", board->extra_readers, argument );
            else
                snprintf( buf, sizeof( buf ), "%s", argument );
            STRSET( board->extra_readers, buf );
        }
        write_boards_txt(  );
        send_to_char( "Done.  (extra readers set)\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "filename" ) ) {
        char                    filename[1024];

        if ( !argument || argument[0] == '\0' ) {
            send_to_char( "No hay un nombre de archivo específico.\r\n", ch );
            return;
        }
        argument = capitalize( argument );
        if ( !can_use_path( ch, BOARD_DIR, argument ) )
            return;
        if ( board->note_file ) {
            snprintf( filename, sizeof( filename ), "%s%s", BOARD_DIR, board->note_file );
            if ( !remove( filename ) )
                send_to_char( "Los archivos más antiguos del buzón, han sido borrados.\r\n", ch );
        }
        STRSET( board->note_file, argument );
        write_boards_txt(  );
        send_to_char( "Done.  (board's filename set)\r\n", ch );
        return;
    }

    if ( !str_cmp( arg2, "post" ) ) {
        board->min_post_level = URANGE( 0, value, ( MAX_LEVEL - 1 ) );
        write_boards_txt(  );
        ch_printf( ch, "Post set to %d. (minimum posting level)\r\n", board->min_post_level );
        return;
    }

    if ( !str_cmp( arg2, "remove" ) ) {
        board->min_remove_level = URANGE( 0, value, ( MAX_LEVEL - 1 ) );
        write_boards_txt(  );
        ch_printf( ch, "Remove set to %d. (minimum remove level)\r\n", board->min_remove_level );
        return;
    }

    if ( !str_cmp( arg2, "maxpost" ) ) {
        board->max_posts = URANGE( 1, value, 999 );
        write_boards_txt(  );
        ch_printf( ch, "Maxpost set to %d. (maximum number of posts)\r\n", board->max_posts );
        return;
    }

    do_bset( ch, ( char * ) "" );
}

void do_boards( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board;
    const char             *b1,
                           *b2;
    int                     bcount = 0,
        nnew = 0;

    if ( !ch )
        return;

    b1 = color_str( AT_GREEN, ch );
    b2 = color_str( AT_GREEN, ch );

    set_char_color( AT_GREEN, ch );
    if ( !first_board ) {
        send_to_char( "No hay buzones todavía.\r\n", ch );
        return;
    }

    if ( !argument || argument[0] == '\0' ) {
        if ( !IS_BLIND( ch ) ) {
            if ( IS_IMMORTAL( ch ) )
                send_to_char
                    ( "&GBoard             Posted  New Posts   Read    Post    Remove    Max\r\n",
                      ch );
            else
                send_to_char( "&GBuzón             Enviada  Nuevas\r\n", ch );
        }
        for ( board = first_board; board; board = board->next ) {
            if ( !can_read( ch, board ) && !can_post( ch, board ) && !can_remove( ch, board ) )
                continue;

            if ( IS_BLIND( ch ) ) {
                nnew = get_new_notes( board, ch );
                pager_printf( ch,
                              "Buzón número %d llamado %s contiene %3d notas totales y %3d notas nuevas\r\n",
                              ++bcount, board->note_file ? board->note_file : "(Not Set)",
                              board->num_posts, nnew );
            }
            if ( !IS_BLIND( ch ) ) {
                pager_printf( ch, "%s%d%s %s%-15.15s %s %s%3d", b2, ++bcount,
                              b1, b2, board->note_file ? board->note_file : "(Not Set)", b1, b2,
                              board->num_posts );
                nnew = get_new_notes( board, ch );
                if ( nnew > 0 )
                    pager_printf( ch, "%s     %s%3d%s", b1, b2, nnew, b1 );
                else
                    send_to_pager( "        ", ch );
                if ( IS_IMMORTAL( ch ) )
                    ch_printf( ch, "         %d        %d       %d     %d",
                               board->min_read_level, board->min_post_level,
                               board->min_remove_level, board->max_posts );
                send_to_pager( "\r\n", ch );
            }
        }
        send_to_char( "Sintaxis: buzones número para escoger un buzón.\r\n", ch );
        send_to_char( "Sintaxis: nota listar/escribir/leer/enviar\r\n", ch );
        return;
    }

    bcount = atoi( argument );
    if ( !( board = get_board( ch, bcount ) ) ) {
        bcount = 0;
        for ( board = first_board; board; board = board->next ) {
            if ( !can_read( ch, board ) && !can_post( ch, board ) && !can_remove( ch, board ) )
                continue;
            bcount++;
            if ( !str_cmp( board->note_file, argument ) )
                break;
        }
        if ( !board ) {
            send_to_char( "No existe ese buzón.\r\n", ch );
            return;
        }
    }

    ch->pcdata->onboard = bcount;
    ch_printf( ch, "Cambiando al buzón %s%d%s %s%s.\r\n",
               b2, ch->pcdata->onboard, b1, b2, board->note_file ? board->note_file : "(No especificado)" );
    ch_printf( ch, "%s%s%s %sleer las notas de este buzón.\r\n",
               b1, b2, can_read( ch, board ) ? "puedes" : "no puedes", b1 );
    ch_printf( ch, "%s%s%s %sescribir notas en este buzón.\r\n",
               b1, b2, can_post( ch, board ) ? "puedes" : "no puedes", b1 );
    ch_printf( ch, "%s%s%s %seliminar notas de este buzón.\r\n",
               b1, b2, can_remove( ch, board ) ? "puedes" : "no puedes", b1 );
}

/* Find the auction board */
BOARD_DATA             *get_auction_board( void )
{
    BOARD_DATA             *board = NULL;

    for ( board = first_board; board; board = board->next ) {
        if ( !str_cmp( board->note_file, "subasta" ) )
            break;
    }
    return board;
}

int count_auctions( CHAR_DATA *ch )
{
    BOARD_DATA             *board = get_auction_board(  );
    NOTE_DATA              *pnote,
                           *pnote_next = NULL;
    int                     count = 0;

    if ( !board || !ch )
        return 0;
    for ( pnote = board->first_note; pnote; pnote = pnote_next ) {
        pnote_next = pnote->next;
        if ( pnote->aclosed )
            continue;
        if ( !str_cmp( pnote->sender, ch->name ) )
            count++;
    }
    return count;
}

void check_auction( CHAR_DATA *ch )
{
    BOARD_DATA             *board = get_auction_board(  );
    NOTE_DATA              *pnote,
                           *pnote_next = NULL;
    BID_DATA               *bid,
                           *bid_next = NULL,
        *chbid = NULL;
    int                     count = 0;
    bool                    asave = false;

    if ( !board || !ch )
        return;

    for ( pnote = board->first_note; pnote; pnote = pnote_next ) {
        pnote_next = pnote->next;
        ++count;
        chbid = check_high_bid( pnote );

        /*
         * Closed and no bets give object back to seller
         */
        if ( pnote->aclosed ) {
            /*
             * If it was canceled return object to seller
             */
            if ( pnote->acanceled ) {
                if ( pnote->obj && !str_cmp( pnote->sender, ch->name ) ) {
                    set_char_color( AT_AUCTION, ch );
                    act( AT_ACTION, "El subastador se materializa ante ti y te entrega $p.", ch,
                         pnote->obj, NULL, TO_CHAR );
                    act( AT_ACTION, "El subastador se materializa ante $n, y le entrega $p.", ch,
                         pnote->obj, NULL, TO_ROOM );
                    pnote->obj->auctioned = false;
                    obj_to_char( pnote->obj, ch );
                    pnote->obj = NULL;
                    pnote->sfor = 0;
                    pnote->acanceled = false;
                    if ( !pnote->first_bid ) {
                        UNLINK( pnote, board->first_note, board->last_note, next, prev );
                        free_note( pnote );
                        --board->num_posts;
                        asave = true;
                        continue;
                    }
                    asave = true;
                }
            }
            if ( pnote->sfor && !str_cmp( pnote->sender, ch->name ) ) {
                set_char_color( AT_AUCTION, ch );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR,
                            ", el subastador se materializa ante ti y te entrega %s monedas de oro.",
                            num_punct( pnote->sfor ) );
                act( AT_ACTION, "El subastador se materializa ante $n, y le entrega algo de oro.",
                     ch, NULL, NULL, TO_ROOM );
                GET_MONEY( ch, CURR_GOLD ) += pnote->sfor;
                pnote->sfor = 0;
                asave = true;
            }
            if ( !pnote->first_bid ) {
                if ( !pnote->obj && !pnote->sfor ) {
                    UNLINK( pnote, board->first_note, board->last_note, next, prev );
                    free_note( pnote );
                    --board->num_posts;
                    asave = true;
                    continue;
                }
                if ( !pnote->acanceled && pnote->obj && !str_cmp( pnote->sender, ch->name ) ) {
                    set_char_color( AT_AUCTION, ch );
                    act( AT_ACTION, "El subastador se meterializa ante ti y te entrega $p.", ch,
                         pnote->obj, NULL, TO_CHAR );
                    act( AT_ACTION, "El subastador se materializa ante $n, y le entrega $p.", ch,
                         pnote->obj, NULL, TO_ROOM );
                    pnote->obj->auctioned = false;
                    obj_to_char( pnote->obj, ch );
                    pnote->obj = NULL;
                    UNLINK( pnote, board->first_note, board->last_note, next, prev );
                    free_note( pnote );
                    --board->num_posts;
                    asave = true;
                    continue;
                }
            }
        }

        for ( bid = pnote->first_bid; bid; bid = bid_next ) {
            bid_next = bid->next;

            if ( str_cmp( bid->name, ch->name ) )
                continue;

            /*
             * Autowin has been reached handle it correctly
             */
            if ( pnote->autowin > 0 && bid->bid >= pnote->autowin ) {
                to_channel_printf( "subasta", 1, "%s ha ganado %s.", ch->name,
                                   pnote->subject );
                pnote->sfor = bid->bid;
                pnote->aclosed = true;
                asave = true;
            }

            /*
             * No more bidding give out what it should to the character
             */
            if ( pnote->aclosed ) {
                if ( chbid && bid == chbid && !pnote->acanceled ) {
                    /*
                     * give the object and remove them from the list
                     */
                    if ( pnote->obj ) {
                        set_char_color( AT_AUCTION, ch );
                        act( AT_ACTION, "El subastador se materializa ante tí y te entrega $p.",
                             ch, pnote->obj, NULL, TO_CHAR );
                        act( AT_ACTION, "El subastador se materializa ante $n, y le entrega $p.",
                             ch, pnote->obj, NULL, TO_ROOM );
                        pnote->obj->auctioned = false;
                        obj_to_char( pnote->obj, ch );
                        pnote->obj = NULL;
                        UNLINK( bid, pnote->first_bid, pnote->last_bid, next, prev );
                        free_bid( bid );
                        asave = true;
                        continue;
                    }
                    else {                             /* object is already gone */

                        set_char_color( AT_AUCTION, ch );
                        act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR,
                                    "El subastador se materializa ante tí, y te entrega %s oro.",
                                    num_punct( bid->bid ) );
                        act( AT_ACTION,
                             "El subastador se materializa ante $n, y le entrega algunas monedas de oro.", ch,
                             NULL, NULL, TO_ROOM );
                        GET_MONEY( ch, CURR_GOLD ) += bid->bid;
                        UNLINK( bid, pnote->first_bid, pnote->last_bid, next, prev );
                        free_bid( bid );
                        asave = true;
                        continue;
                    }
                }
                else {
                    set_char_color( AT_AUCTION, ch );
                    act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR,
                                "El subastador se materializa ante ti y te entrega %s oro.",
                                num_punct( bid->bid ) );
                    act( AT_ACTION,
                         "El subastador se materializa ante $n y le entrega algo de oro.", ch, NULL,
                         NULL, TO_ROOM );
                    GET_MONEY( ch, CURR_GOLD ) += bid->bid;
                    UNLINK( bid, pnote->first_bid, pnote->last_bid, next, prev );
                    free_bid( bid );
                    asave = true;
                    continue;
                }
            }
            /*
             * Not highest bidder so go ahead and refund their bid
             */
            if ( chbid && bid != chbid ) {
                set_char_color( AT_AUCTION, ch );
                act_printf( AT_ACTION, ch, NULL, NULL, TO_CHAR,
                            "el subastador se materializa ante ti y te entrega %s monedas de oro.",
                            num_punct( bid->bid ) );
                act( AT_ACTION, "El subastador se materializa ante $n, y le entrega algo de oro.",
                     ch, NULL, NULL, TO_ROOM );
                GET_MONEY( ch, CURR_GOLD ) += bid->bid;
                UNLINK( bid, pnote->first_bid, pnote->last_bid, next, prev );
                free_bid( bid );
                asave = true;
                continue;
            }
        }
    }

    /*
     * save the auction board if you need to
     */
    if ( asave ) {
        write_board( board );
        save_char_obj( ch );
    }
}

void set_auction_list( OBJ_DATA *list, CHAR_DATA *ch )
{
    AUCTION_DATA           *auction;
    OBJ_DATA               *obj,
                           *tmpobj = NULL;
    char                    astr[MSL * 2];

    for ( obj = list; obj; obj = obj->next_content ) {
        if ( obj->wear_loc == WEAR_NONE && ( can_see_obj( ch, obj )
                                             || ( IS_OBJ_STAT( obj, ITEM_INVIS )
                                                  && IS_OBJ_STAT( obj, ITEM_GLOW )
                                                  && !IS_OBJ_STAT( obj, ITEM_BURIED )
                                                  && !IS_OBJ_STAT( obj, ITEM_HIDDEN ) ) )
             && ( obj->item_type != ITEM_TRAP || IS_AFFECTED( ch, AFF_DETECTTRAPS ) ) ) {
            snprintf( astr, sizeof( astr ), "%s", format_obj_to_char( obj, ch, true ) );

            if ( obj->in_obj ) {
                tmpobj = obj->in_obj;
                mudstrlcpy( astr, "   ", sizeof( astr ) );
                while ( tmpobj->in_obj ) {
                    mudstrlcat( astr, "   ", sizeof( astr ) );
                    tmpobj = tmpobj->in_obj;
                }
                mudstrlcat( astr, format_obj_to_char( obj, ch, true ), sizeof( astr ) );
            }

            for ( auction = first_auction; auction; auction = auction->next ) {
                if ( !strcmp( auction->auctstr, astr ) ) {
                    auction->count += obj->count;
                    break;
                }
            }

            CREATE( auction, AUCTION_DATA, 1 );
            auction->auctstr = STRALLOC( astr );
            auction->type = obj->item_type;
            auction->count = obj->count;
            LINK( auction, first_auction, last_auction, next, prev );

            if ( obj->first_content ) {
                set_auction_list( obj->first_content, ch );
            }
        }
    }
}

void show_auction_list( CHAR_DATA *ch )
{
    AUCTION_DATA           *auction,
                           *auction_next;

    for ( auction = first_auction; auction; auction = auction_next ) {
        auction_next = auction->next;

        switch ( auction->type ) {
            default:
                set_char_color( AT_OBJECT, ch );
                break;

            case ITEM_BLOOD:
                set_char_color( AT_BLOOD, ch );
                break;

            case ITEM_MONEY:
            case ITEM_TREASURE:
                set_char_color( AT_YELLOW, ch );
                break;

            case ITEM_COOK:
            case ITEM_FOOD:
                set_char_color( AT_HUNGRY, ch );
                break;

            case ITEM_DRINK_CON:
            case ITEM_FOUNTAIN:
                set_char_color( AT_THIRSTY, ch );
                break;

            case ITEM_FIRE:
                set_char_color( AT_FIRE, ch );
                break;

            case ITEM_SCROLL:
            case ITEM_WAND:
            case ITEM_STAFF:
                set_char_color( AT_MAGIC, ch );
                break;
        }

        send_to_char( auction->auctstr, ch );

        if ( auction->count != 1 )
            ch_printf( ch, " (%d)", auction->count );
        send_to_char( "\r\n", ch );

        UNLINK( auction, first_auction, last_auction, next, prev );
        STRFREE( auction->auctstr );
        DISPOSE( auction );
    }
}

void auction_update( void )
{
    CHAR_DATA              *ch;

    for ( ch = first_char; ch; ch = ch->next ) {
        if ( IS_NPC( ch ) )
            continue;
        check_auction( ch );
    }
}

void do_auction( CHAR_DATA *ch, char *argument )
{
    BOARD_DATA             *board = NULL;
    NOTE_DATA              *pnote = NULL;
    BID_DATA               *chbid = NULL;
    OBJ_DATA               *obj = NULL;
    char                    arg[MSL];
    int                     count = 0,
        start = 0,
        shown = 0;
    char                    buf[MSL];

    if ( !ch )
        return;

    if ( !( board = get_auction_board(  ) ) ) {
        send_to_char( "No hay buzón de subastas.\r\n", ch );
        return;
    }

    argument = one_argument( argument, arg );
    if ( arg == NULL || arg[0] == '\0' || !str_cmp( arg, "list" ) ) {
        if ( !board->first_note ) {
            send_to_char( "No hay nada subastándose ahora mismo.\r\n", ch );
            return;
        }

        argument = one_argument( argument, arg );
        if ( is_number( arg ) )
            start = atoi( arg );
        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            chbid = check_high_bid( pnote );

            ++count;
            if ( start > 0 && count < start )
                continue;

            if ( shown == 0 )
                pager_printf( ch, "Oferta %12.12s %25.25s %12.12s %25s %15.15s\r\n",
                              "vendedor", "Victoria", "Puja más alta", "puja", "Objeto" );

            pager_printf( ch, "%3d  %12.12s %25s",
                          count, pnote->sender ? pnote->sender : "(No Sender)",
                          num_punct( pnote->autowin ) );

            pager_printf( ch, " %12.12s %25s %15.15s%3s\r\n",
                          chbid ? chbid->name : "(No Bidder)",
                          chbid ? num_punct( chbid->bid ) : "0",
                          pnote->subject ? pnote->subject : "(No Subject)",
                          ( pnote->subject && strlen( pnote->subject ) > 15 ) ? "..." : "" );

            if ( ++shown > 15 ) {
                if ( pnote->next )
                    send_to_pager( "Solo se pueden subastar 15 cosas.\r\n", ch );
                break;
            }
        }
        send_to_char( "uso: subastas\r\n", ch );
        send_to_char( "Uso: subastas Número\r\n", ch );
        send_to_char( "uso: subastas número cerrar\r\n", ch );
                send_to_char( "uso: subastas número cancelar\r\n", ch );
        send_to_char( "Uso: subastas número pujar cantidad\r\n", ch );
        send_to_char( "Uso: subastas objeto victoria\r\n", ch );
        send_to_char( "uso: subastas lista número\r\n", ch );
        return;
    }

    if ( is_number( arg ) ) {
        int                     unote = atoi( arg );

        argument = one_argument( argument, arg );
        for ( pnote = board->first_note; pnote; pnote = pnote->next ) {
            if ( ++count < unote )
                continue;
            if ( !pnote->obj )
                continue;
            if ( !str_cmp( arg, "cerrar" ) ) {
                if ( str_cmp( pnote->sender, ch->name ) && !IS_IMMORTAL( ch ) ) {
                    send_to_char( "Solo el vendedor puede cerrar su subasta.\r\n", ch );
                    return;
                }
                if ( ( chbid = check_high_bid( pnote ) ) )
                    pnote->sfor = chbid->bid;
                to_channel_printf( "subasta", 1, "%s ha cerrado la subasta de %s.", ch->name,
                                   pnote->subject );
                pnote->aclosed = true;
                STRSET( pnote->subject, "Closed" );
                auction_update(  );
                write_board( board );
                return;
            }
            if ( !str_cmp( arg, "cancelar" ) ) {
                if ( str_cmp( pnote->sender, ch->name ) && !IS_IMMORTAL( ch ) ) {
                    send_to_char( "solo el vendedor puede cancelar la subasta.\r\n", ch );
                    return;
                }
                to_channel_printf( "subasta", 1, "%s ha cancelado la subasta de %s.", ch->name,
                                   pnote->subject );
                pnote->aclosed = true;
                pnote->acanceled = true;
                STRSET( pnote->subject, "Canceled" );
                auction_update(  );
                write_board( board );
                return;
            }
            if ( !str_cmp( arg, "pujar" ) ) {
                if ( !str_cmp( pnote->sender, ch->name ) ) {
                    send_to_char( "No puedes pujar por tu propia subasta.\r\n", ch );
                    return;
                }

                chbid = check_high_bid( pnote );
                argument = one_argument( argument, arg );
                if ( !is_number( arg ) ) {
                    send_to_char( "Debes introducir una cantidad a pujar.\r\n", ch );
                    return;
                }

                count = atoi( arg );

                if ( count <= 0 || GET_MONEY( ch, CURR_GOLD ) < count ) {
                    ch_printf( ch, "No puedes pujar %d oro.\r\n", count );
                    return;
                }
                if ( chbid ) {
                    if ( !str_cmp( chbid->name, ch->name ) ) {
                        send_to_char( "ya tienes la puja más alta.\r\n", ch );
                        return;
                    }
                    if ( ( chbid->bid + 100 ) < count ) {
                        ch_printf( ch, "La puja más baja que puedes hacer es %s.\r\n",
                                   num_punct( ( chbid->bid + 100 ) ) );
                        return;
                    }
                }
                add_bid( pnote, ch, count );
                GET_MONEY( ch, CURR_GOLD ) -= count;
                to_channel_printf( "subasta", 1, "%s ha pujado %s monedas de oro para %s.", ch->name,
                                   num_punct( count ), pnote->subject );
                auction_update(  );
                write_board( board );
                return;
            }
            if ( pnote->obj ) {
                identify_object( ch, pnote->obj );
                if ( pnote->obj->first_content ) {
                    set_auction_list( pnote->obj->first_content, ch );
                    show_auction_list( ch );
                }
            }
            else
                send_to_char( "La subasta ha sido cerrada.\r\n", ch );
            show_bids( pnote, ch );
            add_read( pnote, ch );
            return;
        }
        send_to_char( "No existe ese número de subasta.\r\n", ch );
        return;
    }

    if ( !( obj = get_obj_carry( ch, arg ) ) ) {
        send_to_char( "No estás cargando eso.\r\n", ch );
        return;
    }

    if ( obj->timer > 0 ) {
        send_to_char( "No puedes subastar eso.\r\n", ch );
        return;
    }

    if ( count_auctions( ch ) >= sysdata.maxauction ) {
        ch_printf( ch, "Solo puede haber %d subastas a la vez.\r\n",
                   sysdata.maxauction );
        return;
    }

    argument = one_argument( argument, arg );
    count = 0;
    if ( arg != NULL && arg[0] != '\0' && is_number( arg ) ) {
        count = atoi( arg );
        if ( count <= 0 ) {
            send_to_char( "No puedes subastar un objeto con una victoria de cero o menor.\r\n", ch );
            return;
        }
    }
    separate_obj( obj );
    obj_from_char( obj );

    CREATE( pnote, NOTE_DATA, 1 );
    pnote->next = pnote->prev = NULL;
    pnote->sender = QUICKLINK( ch->name );
    pnote->to_list = STRALLOC( "todos" );
    pnote->subject = QUICKLINK( obj->short_descr );
    pnote->text = STRALLOC( "subastando\r\n" );
    pnote->first_vote = pnote->last_vote = NULL;
    pnote->first_bid = pnote->last_bid = NULL;
    pnote->obj = obj;
    obj->auctioned = true;
    pnote->posttime = current_time;
    pnote->voting = 0;
    pnote->yesvotes = 0;
    pnote->novotes = 0;
    pnote->abstentions = 0;
    pnote->first_vote = pnote->last_vote = NULL;
    pnote->first_bid = pnote->last_bid = NULL;
    pnote->first_read = pnote->last_read = NULL;
    pnote->autowin = count;
    board->num_posts++;
    LINK( pnote, board->first_note, board->last_note, next, prev );
    write_board( board );
    snprintf( buf, MSL, "¡%s ha subastado %s!&D", ch->name, pnote->subject );
    announce( buf );
}
