/* Copyright 2014, 2015 Jesús Pavón Abián
Programa bajo licencia GPL. Encontrará una copia de la misma en el archivo COPYING.txt que se encuentra en la raíz de los fuentes. */
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
