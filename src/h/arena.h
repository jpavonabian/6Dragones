/* Copyright 2014, 2015 Jes�s Pav�n Abi�n
Programa bajo licencia GPL. Encontrar� una copia de la misma en el archivo COPYING.txt que se encuentra en la ra�z de los fuentes. */
//This header supports arena system. -Taon

#ifndef arena_h
#define arena_h

extern int              arena_prep_vnum;
extern int              arena_high_vnum;
extern int              arena_low_vnum;
extern int              arena_population;
extern short            prep_length;

extern void             do_game(  );
extern void             run_game(  );
extern short            get_arena_count(  );
extern void arena_chan  args( ( char * ) );

#endif
