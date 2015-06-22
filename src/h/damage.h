/* Copyright 2014, 2015 Jesús Pavón Abián
Programa bajo licencia GPL. Encontrará una copia de la misma en el archivo COPYING.txt que se encuentra en la raíz de los fuentes. */
/* Damage allocations for most spells skills in game */

#define  nominal    number_range( 4+ch->level, 20+ch->level )
#define  low        number_range( 10+ch->level, 30+ch->level )  /* 11 - 20 */
#define  medium     number_range( 20+ch->level, 60+ch->level )  /* 21 - 30 */
#define  mediumhigh number_range( 40+ch->level, 120+ch->level ) /* 31 - 40 */
#define  high       number_range( 4*ch->level, 6*ch->level )    /* 41 - 50 */
#define  extrahigh  number_range( 4*ch->level, ch->level*8 )    /* 51 - 60 */
#define  ludicrous  number_range( 6*ch->level, 12*ch->level )   /* 61 - 75 */
#define  insane     number_range( 12*ch->level, 16*ch->level )  /* 76 - 90 */
#define  maximum    number_range( 16*ch->level, 20*ch->level )
