/* Copyright 2014, 2015 Jes�s Pav�n Abi�n
Programa bajo licencia GPL. Encontrar� una copia de la misma en el archivo COPYING.txt que se encuentra en la ra�z de los fuentes. */
typedef struct landmark_data LANDMARK_DATA;
struct landmark_data
{
    LANDMARK_DATA          *next;
    LANDMARK_DATA          *prev;
    struct area_data       *area;
    char                   *name;
    int                     vnum;
};
