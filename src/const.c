/* Copyright 2014, 2015 Jes�s Pav�n Abi�n
Programa bajo licencia GPL. Encontrar� una copia de la misma en el archivo COPYING.txt que se encuentra en la ra�z de los fuentes. */
// Constantes de 6 Dragones

#include "h/mud.h"
#include "h/files.h"

/* undef these at EOF */
#define AM 95
#define AC 95
#define AT 85
#define AW 85
#define AV 95
#define AD 95
#define AR 90
#define AA 95

const char             *obj_sizes[] = {
    "magico", "diminuto", "pequenyo", "medio", "grande",
    "enorme", "colosal", "descomunal"
};

#if 0
const struct at_color_type at_color_table[AT_MAXCOLOR] = { {"plain", AT_GREY},
{"action", AT_GREY},
{"say", AT_LBLUE},
{"gossip", AT_LBLUE},
{"yell", AT_WHITE},
{"tell", AT_WHITE},
{"whisper", AT_WHITE},
{"hit", AT_WHITE},
{"hitme", AT_YELLOW},
{"immortal", AT_YELLOW},
{"hurt", AT_RED},
{"falling", AT_WHITE + AT_BLINK},
{"danger", AT_RED + AT_BLINK},
{"magic", AT_BLUE},
{"consider", AT_GREY},
{"report", AT_GREY},
{"poison", AT_GREEN},
{"social", AT_CYAN},
{"dying", AT_YELLOW},
{"dead", AT_RED},
{"skill", AT_GREEN},
{"carnage", AT_BLOOD},
{"damage", AT_WHITE},
{"flee", AT_YELLOW},
{"roomname", AT_WHITE},
{"roomdesc", AT_YELLOW},
{"object", AT_GREEN},
{"person", AT_PINK},
{"list", AT_BLUE},
{"bye", AT_GREEN},
{"gold", AT_YELLOW},
{"gtell", AT_BLUE},
{"note", AT_GREEN},
{"hungry", AT_ORANGE},
{"thirsty", AT_BLUE},
{"fire", AT_RED},
{"sober", AT_WHITE},
{"wearoff", AT_YELLOW},
{"exits", AT_WHITE},
{"score", AT_LBLUE},
{"reset", AT_DGREEN},
{"log", AT_PURPLE},
{"diemsg", AT_WHITE},
{"wartalk", AT_RED},
{"racetalk", AT_DGREEN},
{"ignore", AT_GREEN},
{"divider", AT_PLAIN},
{"morph", AT_GREY},
};
#endif

/*
 * Attribute bonus tables.
 */
const struct str_app_type str_app[26] = {
    {-5, -4, 0, 0},                                    /* 0 */
    {-5, -4, 3, 1},                                    /* 1 */
    {-3, -2, 3, 2},
    {-3, -1, 10, 3},                                   /* 3 */
    {-2, -1, 25, 4},
    {-2, -1, 55, 5},                                   /* 5 */
    {-1, 0, 80, 6},
    {-1, 0, 90, 7},
    {0, 0, 100, 8},
    {0, 0, 100, 9},
    {0, 0, 115, 10},                                   /* 10 */
    {0, 0, 115, 11},
    {0, 0, 140, 12},
    {0, 0, 140, 13},                                   /* 13 */
    {0, 1, 170, 14},
    {1, 1, 170, 15},                                   /* 15 */
    {1, 2, 195, 16},
    {2, 3, 220, 22},
    {2, 4, 250, 25},                                   /* 18 */
    {3, 5, 400, 30},
    {3, 6, 500, 35},                                   /* 20 */
    {4, 7, 600, 40},
    {5, 7, 700, 45},
    {6, 8, 800, 50},
    {8, 10, 900, 55},
    {10, 12, 999, 60}                                  /* 25 */
};

const struct int_app_type int_app[26] = {
    {3},                                               /* 0 */
    {5},                                               /* 1 */
    {7},
    {8},                                               /* 3 */
    {9},
    {10},                                              /* 5 */
    {11},
    {12},
    {13},
    {15},
    {17},                                              /* 10 */
    {19},
    {22},
    {25},
    {28},
    {31},                                              /* 15 */
    {34},
    {37},
    {40},                                              /* 18 */
    {44},
    {49},                                              /* 20 */
    {55},
    {60},
    {70},
    {85},
    {99}                                               /* 25 */
};

const struct wis_app_type wis_app[26] = {
    {1},                                               /* 0 */
    {1},                                               /* 1 */
    {1},
    {1},                                               /* 3 */
    {1},
    {1},                                               /* 5 */
    {2},
    {2},
    {2},
    {3},
    {3},                                               /* 10 */
    {3},
    {3},
    {3},
    {3},
    {4},                                               /* 15 */
    {4},
    {5},
    {6},                                               /* 18 */
    {7},
    {8},                                               /* 20 */
    {8},
    {9},
    {10},
    {11},
    {12}                                               /* 25 */
};

const struct dex_app_type dex_app[26] = {
    {45},                                              /* 0 */
    {40},                                              /* 1 */
    {35},
    {30},
    {25},
    {20},                                              /* 5 */
    {10},
    {9},
    {8},
    {7},
    {6},                                               /* 10 */
    {5},
    {4},
    {2},
    {0},
    {-1},                                              /* 15 */
    {-1},
    {-2},
    {-3},
    {-4},
    {-5},                                              /* 20 */
    {-6},
    {-7},
    {-8},
    {-9},
    {-10}                                              /* 25 */
};

const struct con_app_type con_app[26] = {
    {-4, 20},                                          /* 0 */
    {-3, 25},                                          /* 1 */
    {-2, 30},
    {-2, 35},                                          /* 3 */
    {-1, 40},
    {-1, 45},                                          /* 5 */
    {-1, 50},
    {0, 55},
    {0, 60},
    {0, 65},
    {0, 70},                                           /* 10 */
    {0, 75},
    {0, 80},
    {0, 85},
    {0, 88},
    {1, 90},                                           /* 15 */
    {2, 91},
    {2, 91},
    {3, 92},                                           /* 18 */
    {4, 93},
    {5, 94},                                           /* 20 */
    {6, 95},
    {7, 96},
    {8, 97},
    {9, 98},
    {10, 99}                                           /* 25 */
};

const struct cha_app_type cha_app[26] = {
    {-60},                                             /* 0 */
    {-50},                                             /* 1 */
    {-50},
    {-40},
    {-30},
    {-20},                                             /* 5 */
    {-10},
    {-5},
    {-1},
    {0},
    {0},                                               /* 10 */
    {0},
    {0},
    {0},
    {1},
    {5},                                               /* 15 */
    {10},
    {20},
    {30},
    {40},
    {50},                                              /* 20 */
    {60},
    {70},
    {80},
    {90},
    {99}                                               /* 25 */
};

/* Have to fix this up - not exactly sure how it works (Scryn) */
const struct lck_app_type lck_app[26] = {
    {-30},                                             /* 0 */
    {-20},                                             /* 1 */
    {-15},
    {-12},
    {-10},
    {-8},                                              /* 5 */
    {-5},
    {-4},
    {-3},
    {-2},
    {-1},                                              /* 10 */
    {0},
    {0},
    {0},
    {5},
    {6},                                               /* 15 */
    {7},
    {8},
    {10},
    {13},
    {15},                                              /* 20 */
    {16},
    {18},
    {20},
    {22},
    {25}                                               /* 25 */
};

/*
 * Liquid properties.
 * Used in #OBJECT section of area file.
 */
const struct liq_type   liq_table[LIQ_MAX] = {
    {"agua", "limpia", {0, 1, 10}},                    /* 0 */
    {"cerveza", "�mbar", {3, 2, 5}},
    {"vino", "rosa", {5, 2, 5}},
    {"ale", "marr�n", {2, 2, 5}},
    {"ale oscura", "negro", {1, 2, 5}},

    {"whisky", "dorado", {6, 1, 4}},                   /* 5 */
    {"limonada", "rosa", {0, 1, 8}},
    {"aguardiente", "aguardiente", {10, 0, 0}},
    {"especialidad local", "plateado", {3, 3, 3}},
    {"jugo de moho", "verde", {0, 4, -8}},

    {"leche", "blanco", {0, 3, 6}},                      /* 10 */
    {"t�", "bronceado", {0, 1, 6}},
    {"caf�", "negro", {0, 1, 6}},
    {"sangre", "rojo", {0, 2, -1}},
    {"agua salada", "limpio", {0, 1, -2}},

    {"cola", "cereza", {0, 1, 5}},                     /* 15 */
    {"aguamiel", "miel", {4, 2, 5}},                /* 16 */
    {"grog", "thick brown", {3, 2, 5}},                /* 17 */
    {"Poci�n negra", "negro", {0, 0, 0}},
    {"v�mito", "verde asqueroso", {0, -1, -1}},

    {"agua santa", "limpia", {0, 1, 10}}                /* 20 */
};

/* Working in monks here. -Taon */

const char             *m_attack_table[8] = {
    "golpe de karate", "uppercuts", "patada circular", "gancho de izquierda",
    "gancho de derecha", "jabs", "rodillazo izquierdo",
    "rodillazo derecho"
};

const char             *m_attack_table_plural[8] = {
    "golpe de karate", "uppercuts", "patada circular", "gancho de izquierda",
    "gancho de derecha", "jabs", "rodillazo izquierdo",
    "rodillazo derecho"
};

/* removed "pea" and added chop, spear, smash - Grimm */
/* Removed duplication in damage types - Samson 1-9-00 */
const char             *attack_table[DAM_MAX_TYPE] = {
    "porrazo", "tajo", "apu�alamiento", "hachazo", "aplastamiento", "latigazo", "apu�alamiento",
    "empuj�n"
};

const char             *attack_table_plural[DAM_MAX_TYPE] = {
    "porrazo", "tajo", "apu�alamiento", "hachazo", "aplastamiento", "latigazo", "apu�alamiento",
    "empuj�n"

};

const char             *weapon_skills[WEP_MAX] = {
    "Armas largas mejoradas", "Armas largas", "Armas cortas", "L�tigo",
    "impactos mejorados", "impactos", "arquero", "Cerbatanas", "hachas mejoradas",
    "Hachas", "Lanza", "Dagas", "Lance", "Trituradora", "Garra", "Poleas"
};

const char             *projectiles[PROJ_MAX] = {
    "tornillo", "flecha", "dardo", "piedra"
};

const char             *s_blade_messages[24] = {
    "falla", "rasca", "ara�a", "corta", "corta", "acuchilla", "rasga",
    "rasga", "acuchilla", "acuchilla", "hace sangrar", "mutila", "desgarra", "destripa",
    "_desmiembra_", "_devasta_", "_descuartiza_", "_descuartiza_", "destroza",
    "desfigura", "destruye", "DESTRIPA", "* MASACRA *",
    "&R*** ANIQUILA ***&D"
};

const char             *p_blade_messages[24] = {
    "fallas", "rascas", "ara�as", "cortas", "cortas", "acuchillas", "rasgas",
    "rasgas", "acuchillas", "acuchillas", "haces sangrar", "mutilas", "desgarras", "destripas",
    "_desmiembras_", "_devastas_", "_descuartizas_", "_descuartizas_", "destrozas",
    "desfiguras", "destruyes", "DESTRIPAS", "* MASACRAS *",
    "&R*** ANIQUILAS ***&D"
};

const char             *s_blunt_messages[24] = {
    "falla", "roza", "ara�a", "da�a", "golpea", "golpea", "causa magulladuras",
    "aporrea", "azota", "azota", "azota", "hiere", "aplasta", "desmiembra",
    "_rompe_", "_devasta_", "_mutila_", "_lisia_", "MUTILA", "DESFIGURA",
    "MASACRA", "PULVERIZA", "* ANIQUILA *", "&R*** ANIQUILA ***&D"
};

const char             *p_blunt_messages[24] = {
    "fallas", "rozas", "ara�as", "da�as", "golpeas", "golpeas", "causas magulladuras",
    "aporreas", "azotas", "azotas", "azotas", "hieres", "aplastas", "desmiembras",
    "_rompes_", "_devastas_", "_mutilas_", "_lisias_", "MUTILAS", "DESFIGURAS",
    "MASACRAS", "PULVERIZAS", "* ANIQUILAS *", "&R*** ANIQUILAS ***&D"
};

const char             *s_generic_messages[24] = {
    "falla", "roza", "ara�a", "da�a", "hiere levemente", "hiere", "magulla",
    "lesiona", "golpea", "causa moratones", "causa magulladuras", "hiere gravemente", "destripa", "_traumatiza_",
    "_devasta_", "_destroza_", "_demuele_", "MUTILA", "MASACRA",
    "PULVERIZA", "DESTRUYE", "* ANIQUILA *", "&R*** ANIQUILA ***&D",
    "**** DESINTEGRA ****"
};

const char             *p_generic_messages[24] = {
    "fallas", "rozass", "ara�as", "da�as", "hieres levemente", "hieres", "magullas",
    "lesionas", "golpeas", "causas moratones", "causas magulladuras", "hieres gravemente", "destripas", "_traumatizas_",
    "_devastas_", "_destrozas_", "_demueles_", "MUTILAS", "MASACRAS",
    "PULVERIZAS", "DESTRUYES", "* ANIQUILAS *", "&R*** ANIQUILAS ***&D",
    "**** DESINTEGRAS ****"};

const char            **const s_message_table[DAM_MAX_TYPE] = {
    s_generic_messages,                                /* hit */
    s_blade_messages,                                  /* slash */
    s_blade_messages,                                  /* stab */
    s_blade_messages,                                  /* hack */
    s_blunt_messages,                                  /* crush */
    s_blunt_messages,                                  /* lash */
    s_blade_messages,                                  /* pierce */
    s_blade_messages,                                  /* thrust */
};

const char            **const p_message_table[DAM_MAX_TYPE] = {
    p_generic_messages,                                /* hit */
    s_blade_messages,                                  /* slash */
    s_blade_messages,                                  /* stab */
    s_blade_messages,                                  /* hack */
    s_blunt_messages,                                  /* crush */
    s_blunt_messages,                                  /* lash */
    s_blade_messages,                                  /* pierce */
    s_blade_messages,                                  /* thrust */
};

/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
 */
#define SLOT(n)  n
#define LI LEVEL_IMMORTAL

#undef AM
#undef AC
#undef AT
#undef AW
#undef AV
#undef AD
#undef AR
#undef AA

#undef LI
