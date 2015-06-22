/* Copyright 2014, 2015 Jesús Pavón Abián
Programa bajo licencia GPL. Encontrará una copia de la misma en el archivo COPYING.txt que se encuentra en la raíz de los fuentes. */
/* New memory stuff */
typedef struct memory_strings_data MEMORY_DATA;
struct memory_strings_data
{
    MEMORY_DATA            *next;
    MEMORY_DATA            *prev;
    char                   *string;
    unsigned long int       links;
    unsigned long int       length;
    unsigned long int       linenumber;
    char                   *filename;
};
extern MEMORY_DATA     *first_memory;
extern MEMORY_DATA     *last_memory;
