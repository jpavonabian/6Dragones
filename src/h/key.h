/* Copyright 2014, 2015 Jes�s Pav�n Abi�n
Programa bajo licencia GPL. Encontrar� una copia de la misma en el archivo COPYING.txt que se encuentra en la ra�z de los fuentes. */
#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value) \
  if(!str_cmp(word, literal)) \
  { \
    field = value; \
    fMatch = TRUE; \
   break; \
   }
