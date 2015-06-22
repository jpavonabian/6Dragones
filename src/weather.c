// Archivo del tiempo. Traducido por Lucía Ramos para Felix Felicis. Copiado descaradamente por Zlorth para la traducción de 6 Dragones.
#include <string.h>
#include <stdio.h>
#include "h/mud.h"
#include "h/key.h"
#include "h/files.h"

const char             *const hemisphere_name[] = {
    "northern", "southern"
};

const char             *const climate_names[] = {
    "rainforest", "savanna", "desert", "steppe", "chapparal",
    "grasslands", "deciduous_forest", "taiga", "tundra", "alpine",
    "arctic"
};

int get_hemisphere( char *type )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( hemisphere_name ) / sizeof( hemisphere_name[0] ) ); x++ )
        if ( !str_cmp( type, hemisphere_name[x] ) )
            return x;
    return -1;
}

int get_climate( char *type )
{
    unsigned int            x;

    for ( x = 0; x < ( sizeof( climate_names ) / sizeof( climate_names[0] ) ); x++ )
        if ( !str_cmp( type, climate_names[x] ) )
            return x;
    return -1;
}

struct WeatherCell
{
    int                     climate;                   // Climate flag for the cell
    int                     hemisphere;                // Hemisphere flag for the cell
    int                     temperature;               // Fahrenheit because I'm
    // American, by god
    int                     pressure;                  // 0..100 for now, later change to 
                                                       // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // barometric pressures
    int                     cloudcover;                // 0..100, amount of clouds in the 
                                                       // 
    // 
    // 
    // 
    // 
    // 
    // 
    // 
    // sky
    int                     humidity;                  // 0+
    int                     precipitation;             // 0..100
    int                     energy;                    // 0..100 Storm Energy, chance of
    // storm.
    /*
     *  Instead of a wind direction we use an X/Y speed
     *  It makes the math below much simpler this way.
     *  Its not hard to determine a basic cardinal direction from this
     *  If you want to, a good rule of thumb is that if one directional
     *  speed is more than double that of the other, ignore it; that is
     *  if you have speed X = 15 and speed Y = 3, the wind is obviously
     *  to the east.  If X = 15 and Y = 10, then its a south-east wind. 
     */
    int                     windSpeedX;                // < 0 = west, > 0 = east
    int                     windSpeedY;                // < 0 = north, > 0 = south
};

/*
*	This is the Weather Map.  It is a grid of cells representing X-mile square
*	areas of weather
*/
struct WeatherCell      weatherMap[WEATHER_SIZE_X][WEATHER_SIZE_Y];

/*
*	This is the Weather Delta Map.  It is used to accumulate changes to be
*	applied to the Weather Map.  Why accumulate changes then apply them, rather
*	than just change the Weather Map as we go?
*	Because doing that can mean a change just made to a neighbor can
*	immediately cause ANOTHER change to a neighbor, causing things
*	to get out of control or causing cascading weather, propagating much
*	faster and unpredictably (in a BAD unpredictable way)
*	Instead, we determine all the changes that should occur based on the current
*	'snapshot' of weather, than apply them all at once!
*/
struct WeatherCell      weatherDelta[WEATHER_SIZE_X][WEATHER_SIZE_Y];

//  Set everything up with random non-equal values to prevent equalibrium
void InitializeWeatherMap( void )
{
    int                     x,
                            y;

    for ( y = 0; y < WEATHER_SIZE_Y; y++ ) {
        for ( x = 0; x < WEATHER_SIZE_X; x++ ) {
            struct WeatherCell     *cell = &weatherMap[x][y];

            cell->climate = number_range( 0, 10 );
            cell->hemisphere = number_range( 0, 1 );
            cell->temperature = number_range( -30, 100 );
            cell->pressure = number_range( 0, 100 );
            cell->cloudcover = number_range( 0, 100 );
            cell->humidity = number_range( 0, 100 );
            cell->precipitation = number_range( 0, 100 );
            cell->windSpeedX = number_range( -100, 100 );
            cell->windSpeedY = number_range( -100, 100 );
            cell->energy = number_range( 0, 100 );
        }
    }
}

//Used to determine whether a field exceeds a certain number, see Weather messages for examples
bool ExceedsThreshold( int initial, int delta, int threshold )
{
    return ( ( initial < threshold ) && ( initial + delta >= threshold ) );
}

//Used to determin whether a field drops below a certain point, see Weather messages for examples.
bool DropsBelowThreshold( int initial, int delta, int threshold )
{
    return ( ( initial >= threshold ) && ( initial + delta < threshold ) );
}

//Send a message to a player in the area, assuming they are outside, and awake.
void WeatherMessage( const char *txt, int x, int y )
{
    AREA_DATA              *pArea;
    DESCRIPTOR_DATA        *d = NULL;

    for ( pArea = first_area; pArea; pArea = pArea->next ) {
        if ( pArea->weatherx == x && pArea->weathery == y ) {
            for ( d = first_descriptor; d; d = d->next ) {
                if ( d->connected == CON_PLAYING ) {
                    if ( d->character && ( d->character->in_room->area == pArea )
                         && IS_OUTSIDE( d->character )
                         && !NO_WEATHER_SECT( d->character->in_room->sector_type )
                         && IS_AWAKE( d->character ) && !IS_BLIND( d->character ) )
                        send_to_char( txt, d->character );
                }
            }
        }
    }
}

// This is where we apply all the functions and determine what message to send.
void ApplyDeltaChanges( void )
{
    int                     x,
                            y;

    for ( y = 0; y < WEATHER_SIZE_Y; y++ ) {
        for ( x = 0; x < WEATHER_SIZE_X; x++ ) {
            struct WeatherCell     *cell = &weatherMap[x][y];
            struct WeatherCell     *delta = &weatherDelta[x][y];

            if ( isTorrentialDownpour( getPrecip( cell ) ) ) {
                if ( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
                    WeatherMessage
                        ( "&WThe rain turns to snow as it continues to come down blizzard-like.&D\r\n",
                          x, y );
                else if ( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia cristaliza en abundantes copos que amedrentan al frio con el color del olvido.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa nieve se disuelve en frias gotas de aguacero.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 91 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa lluvia vigoriza su incesante lucha.&D\r\n&YLos truenos hacen temblar la tierra y los relámpagos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa lluvia arremete violenta contra la tierra.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 91 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia se torna  y la intensidad incrementa creando un deslumbrante tapiz nacarado.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 91 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa fuerte nevada se incrementa creando una tormenta heladora.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 91 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa nieve se distrae y la lluvia le gana el pulso.&D\r\n&YEl cielo retumba y se ilumina constantemente.&D\r\n", x, y );
               else
                  WeatherMessage( "&BImpetrante se desprende el agua de las nubes.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WLa nieve cae rápida y constante en forma de tormenta.&D\r\n", x, y );
               else if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BEl aguacero continúa golpeando la tierra.&D\r\n&YTruena y los relámpagos resplandecen como hilos de estrellas.&D\r\n", x, y );
               else
                  WeatherMessage( "&BEl aguacero insiste en forjar la tierra.&D\r\n", x, y );


         }
         else if( isRainingCatsAndDogs( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa intensa lluvia se torna en nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BEl aguacero da paso a una generosa nevada.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 90 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BEl feroz dilubio se amedrenta sin prisa, insistiendo en mojarlo todo.&D\r\n&YEl cielo ruje y centellea.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa fuerte y feroz lluvia amaina un poco, pero continua golpeando.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 90 )
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WEl trágico aguacero empalidece tornándose nieve.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 90 )
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa nieve disminuye y cambia a fuerte lluvia.&D\r\n&YTrueno y relámpago entran en escena subvirtiendo cielo y tierra.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa nieve disminuye y cambia a fuerte lluvia.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 90 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa intensa nieve desciende un poco.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 80 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa lluvia se enfurece.&D\r\n&YLos truenos hacen vibrar la tierra y los relámpagos rasgan el firmamento.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa lluvia se pone seria.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 80 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia se hace nieve y comienza a caer aún más fuerte.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 80 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa nieve cae más fuerte creando un tapiz algodonado.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 80 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa nieve se intensifica un poco y cambia a fuerte lluvia.&D\r\n&YLos truenos hacen temblar la tierra y los relámpagos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa nieve se intensifica un poco y cambia a fuerte lluvia.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WLa nieve cae fuerte formando un mar de nubes de algodon dulce.&D\r\n", x, y );
               else if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa lluvia cae en forma de grandes y abundantes gotas.&D\r\n&YSe escuchan gruñidos en lo alto y los relámpagos chisporrotean iluminando la escena.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLlueve, o quizás sea, la selección de quidditch neozelandesa.&D\r\n", x, y );
         }
         else if( isPouring( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa intensa lluvia se convierte en abundante nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa fuerte nieve se torna en copiosa lluvia. .&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 80 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa fuerte lluvia disminuye un poquito.&D\r\n&YTruena arritmicamente y unos destellos brillantes colman el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa intensa lluvia disminuye pero continúa lloviendo a mares.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 80 )
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa intensa lluvia disminuye y se convierte en abundante nieve.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 80 )
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa cortina de nieve remite mientras comienza la lluvia copiosa.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa cortina de nieve remite mientras comienza la lluvia copiosa.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 80 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa intensa nevada desciende y continúa cayendo fuertemente.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 70 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa lluvia comienza a agudizarse hasta llover a cántaros.&D\r\n&YLos relámpagos destellan en el cielo acompañados brevemente de truenos atronadores.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa constante  lluvia  comienza a caer más fuerte.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 70 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa fuerte lluvia se intensifica y  cambia a nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 70 ) && cell->temperature <= 32 )
               WeatherMessage( "&La nieve comienza a caer con más fuerza  y cubre  el suelo rápidamente.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 70 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa intensa nieve cambia a lluvia y comienza a llover a cántaros.&D\r\n&YLos relámpagos se extienden por el cielo como ramas de luz atronadoras&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa intensa nieve cambia a lluvia y comienza a llover a cántaros.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WLa nieve cae fuertemente.&D\r\n", x, y );
               else if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BDiluvia.&D\r\n&YLos relámpagos y los truenos iluminan el cielo y hacen temblar  la tierra.&D\r\n", x, y );
               else
                  WeatherMessage( "&BDiluvia.&D\r\n", x, y );
         }
         else if( isRaingingHeavily( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa fuerte lluvia cambia a copiosa nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa nieve constante cambia a fuerte lluvia.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 70 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BEl aguacero amaina un poquito..&D\r\n&YLos relámpagos y los truenos comienzan a iluminar el cielo y hacer temblar la tierra.&D\r\n", x, y );
               else
                  WeatherMessage( "&BEl aguacero amaina un poquito..&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 70 )
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WEl aguacero disminuye pero cambia a  abundante nieve.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 70 )
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BEl aguacero disminuye pero cambia a abundante nieve.&D\r\n&YEstalla un  truenos y un relámpago destella en el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa densa nieve remite un poco y cambia a abundante lluvia.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 70 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa abundante nieve disminuye un poco y pasa a ser constante.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 60 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BEl aguacero incrementa su intensidad.  Los relámpagos y los truenos comienzan a iluminar el cielo y hacer temblar la tierra.&D\r\n", x, y );
               else
                  WeatherMessage( "&BEl aguacero comienza a caer más fuerte golpendo la tierra.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 60 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WEl aguacero comienza a intensificarse y  se transforma en abundante nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 60 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa nieve copiosa se intensifica creando un manto blanco en el suelo.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 60 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa nieve constante cambia a  una cada vez más importante y abundante lluvia.&D\r\n&YLos truenos sacuden la tierra y los rayos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa nieve constante cambia a  una cada vez más importante y abundante lluvia.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WLa nieve cae copiosamente sobre la tierra.&D\r\n", x, y );
               else if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa lluvia cae abundantemente sobre la tierra.&D\r\n&YLos truenos sacuden la tierra y los rayos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa lluvia cae abundantemente sobre la tierra.&D\r\n", x, y );
         }
         else if( isDownpour( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WEl aguacero se transforma en una densa nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa nieve constante se transforma en lluvia y disminuye un poco.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 60 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa abundante lluvia disminuye un poco.&D\r\n&YLos truenos sacuden la tierra y los rayos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa abundante lluvia disminuye un poco.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 60 )
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa abundante lluvia disminuye un poco y cambia a nieve cubriendo el suelo  con un manto blanco.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 60 )
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa abundante nieve desciende un poco y cambia a lluvia constante.&D\r\n&YLos truenos restallan y los relámpagos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa abundante nieve desciende un poco y cambia a lluvia constante.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 60 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa abundante nieve  se aligera un poco.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 50 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa abundante nieve se aligera un poco a medida que cambia a una lluvia continua.&D\r\n&YLos truenos sacuden la tierra y los rayos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa abundante nieve se aligera un poco a medida que cambia a una lluvia continua.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 50 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia constante repunta  y se transforma en nieve continua y abundante.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 50 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa nieve constante repunta un poco creando un manto blanco sobre el suelo.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 50 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa nieve constante se intensifica y  se transforma en lluvia torrencial.&D\r\n&YUn rayo atraviesa el cielo y un trueno retumba sacudiendo la tierra.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa nieve constante se intensifica y  se transforma en lluvia torrencial.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WLa nieve cae continuamente.&D\r\n", x, y );
               else if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLlueve a cántaros.&D\r\n&YLos truenos sacuden la tierra y los rayos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLlueve a cántaros.&D\r\n", x, y );
         }
         else if( isRainingSteadily( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia constante se transfoma en nieve continua.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa nieve constante cambia a lluvia continua.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 50 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa intensa lluvia pasa a moderada.&D\r\n&YLos rayos hacen que el cielo resplandezca mientras los truenos retumban constantemente.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa intensa lluvia pasa a moderada.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 50 )
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia torrencial disminuye un poco y cambia a nieve continua.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 50 )
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BEl manto de nieve decrece un poco  a medida que cambia a lluvia moderada.&D\r\n&YLos rayos iluminan el cielo y los truenos sacuden la tierra.&D\r\n", x, y );
               else
                  WeatherMessage( "&BEl manto de nieve decrece un poco  a medida que cambia a lluvia moderada.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 50 ) && cell->temperature <= 32 )
               WeatherMessage( "&WEl manto de nieve decrece un poco pasando a nieve constante.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 40 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa lluvia aumenta en intensidad.&D\r\n&YLos truenos sacuden la tierra y los relámpagos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa lluvia aumenta en intensidad.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 40 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia aumenta y cambia  a copiosa nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 40 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa nieve se intensifica.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 40 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa nieve se intensifica y cambia a una lluvia continua.&D\r\n&YLos relámpagos iluminan el cielo y los truenos hacen temblar la tierra.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa nieve se agudiza un poco  y cambia a lluvia constante.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WNieva constantemente.&D\r\n", x, y );
               else if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLlueve abundantemente.&D\r\n&YLos truenos sacuden la tierra y los rayos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLlueve abundantemente.&D\r\n", x, y );
         }
         else if( isRaining( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia se transforma en nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa nieve cambia a lluvia.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 40 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa continua lluvia disminuye un poco.&D\r\n&YTLos truenos sacuden la tierra y los relámpagos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa  lluvia se agudiza un poco.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 40 )
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia va remitiendo y cambia a nieve.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 40 )
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa nieve disminuye un poco  y va transformándose en lluvia.&D\r\n&YLos truenos sacuden la tierra y los rayos ilumnan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa nieve disminuye un poco  y va transformándose en lluvia.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 40 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa nieve disminuye un poco.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 30 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa ligera lluvia se agudiza un poco.&D\r\n&YLos relámpagos iluminan el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa ligera lluvia se agudiza un poco.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 30 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa ligera lluvia se agudiza un poco y se transforma en nieve.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 30 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa ligera nieve aumenta un poco.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 30 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BLa nieve ligera se intensifica y cambia a lluvia.&D\r\n&YLos  truenos hacen temblar la tierra.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa nieve ligera se intensifica y cambia a lluvia.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WContinúa nevando.&D\r\n", x, y );
               else if( isStormy( getEnergy( cell ) ) )
                  WeatherMessage( "&BContinúa lloviendo.&D\r\n&YTruena, el suelo tiembla y los rayos destellan.&D\r\n", x, y );
               else
                  WeatherMessage( "&BContinúa lloviendo.&D\r\n", x, y );
         }
         else if( isRainingLightly( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia ligera cambia a nieve .&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa ligera nieve cambia a leve lluvia.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 30 ) )
               WeatherMessage( "&BLa lluvia disminuye un poco y  se hace lluvia ligera.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 30 )
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&Wa lluvia disminuye un poco y cambia a ligera nieve.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 30 )
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa nieve remite un poco y se transforma en lluvia ligera.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 30 ) && cell->temperature <= 32 )
               WeatherMessage( "La nieve se hace más ligera.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 20 ) )
               WeatherMessage( "&BLa llovizna se agudiza hasta hacerse lluvia ligera.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 20 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa llovizna se agudiza y se transforma en nieve ligera.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 20 ) && cell->temperature <= 32 )
               WeatherMessage( "&WEl chaparrón se agudiza un poco hasta hacerse nieve ligera.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 20 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BEl chaparrón se agudiza un poco hasta hacerse  lluvia ligera.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WLa nieve ligera continua cayendo suavemente sobre el suelo.&D\r\n", x, y );
               else
                  WeatherMessage( "&BLa lluvia ligera continua cayendo suavemente sobre el suelo.&D\r\n", x, y );
         }
         else if( isDrizzling( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "WLa llovizna cambia a chaparrón.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BEl chaparrón cambia a llovizna.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 20 ) )
               WeatherMessage( "&BLa lluvia ligera aumenta un poco hasta hacerse chaparrón.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 20 )
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa lluvia ligera aumenta un poco y aumenta a chaparrón.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 20 )
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLa nieve ligera se agudiza un poco y cambia a chaparrón.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 20 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLa nieve ligera desciende en forma de ráfagas.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 10 ) )
               WeatherMessage( "&BLa neblina se  hace chaparrón.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 10 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa neblina levanta un poco en forma de ráfagas.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 10 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLos copos aislados aumentan en forma de ráfagas.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->precipitation, delta->precipitation, 10 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLos pocos copos aislados levantan hasta hacerse chaparrón.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WCaen granizos.&D\r\n", x, y );
               else
                  WeatherMessage( "&BUn ligero chaparrón cae sobre el suelo.&D\r\n", x, y );
         }
         else if( isMisting( getPrecip( cell ) ) )
         {
            if( DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WLa niebla meona crea algunos copos de nieve aislado.&D\r\n", x, y );
            else if( ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BAlgunos copos de nieve aislados se hacen niebla meona.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 10 ) )
               WeatherMessage( "&BEl chaparrón amaina un poco hasta hacerse neblina.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 10 ) 
               && DropsBelowThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&WEl chaparrón disminuye y forma algunos copos de nieve aislados.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 10 ) && cell->temperature <= 32 )
               WeatherMessage( "&WLas ráfagas de nieve pasan a ser copos aislados.&D\r\n", x, y );
            else if( DropsBelowThreshold( cell->precipitation, delta->precipitation, 10 ) 
               && ExceedsThreshold( cell->temperature, delta->temperature, 32 ) )
               WeatherMessage( "&BLas ráfagas de nieve remiten y cambian a una ligera neblina.&D\r\n", x, y );
            else
               if( cell->temperature <= 32 )
                  WeatherMessage( "&WCaen algunos copos de nieve aislados.&D\r\n", x, y );
               else
                  WeatherMessage( "&BSe produce una ligera neblina.&D\r\n", x, y );				
         }
         else
         {
            if( isExtremelyCloudy( getCloudCover( cell ) ) )
            {
               if( ExceedsThreshold( cell->cloudcover, delta->cloudcover, 80 ) )
                  WeatherMessage( "&wNubes pasajeras crean un manto en el cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&wLas nubes cubren el cielo como un manto.&D\r\n", x, y );
            }
            else if( isModeratelyCloudy( getCloudCover( cell ) ) )
            {
               if( ExceedsThreshold( cell->cloudcover, delta->cloudcover, 60 ) )
                  WeatherMessage( "&wEl cielo comienza a nublarse más..&D\r\n", x, y );
               else if( DropsBelowThreshold( cell->cloudcover, delta->cloudcover, 80 ) )
                  WeatherMessage( "&wComienzan a despuntar algunas nubes.&D\r\n", x, y );
               else
                  WeatherMessage( "&wMuchas nubes cubren el cielo.&D\r\n", x, y );
            }
            else if( isPartlyCloudy( getCloudCover( cell ) ) )
            {				
               if( ExceedsThreshold( cell->cloudcover, delta->cloudcover, 40 ) )
                  WeatherMessage( "&wMás nubes pasajeras cubren  parcialmente el cielo.&D\r\n", x, y );
               else if( DropsBelowThreshold( cell->cloudcover, delta->cloudcover, 60 ) )
                  WeatherMessage( "&wAlgunas nubes pasan y despejan parte del cielo.&D\r\n", x, y );
               else
                  WeatherMessage( "&wLas nubes cubren parte del cielo.&D\r\n", x, y );
            }
            else if( isCloudy( getCloudCover( cell ) ) )
            {
               if( ExceedsThreshold( cell->cloudcover, delta->cloudcover, 20 ) )
                  WeatherMessage( "&wAlgunas nubes cruzan el cielo.&D\r\n", x, y );
               else if( DropsBelowThreshold( cell->cloudcover, delta->cloudcover, 40 ) )
                  WeatherMessage( "&wAlgunas nubes pasan dejando sólo algunas detrás.&D\r\n", x, y );
               else
                  WeatherMessage( "&wAlgunas nubes se ciernen en el cielo.&D\r\n", x, y );
            }
            else
            {
               if( DropsBelowThreshold( cell->cloudcover, delta->cloudcover, 20 ) )
                  WeatherMessage( "&wLas pocas nubes que quedan comienzan a extenderse.&D\r\n", x, y );
               else
                  if( isSwelteringHeat( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 90 ) )
                        WeatherMessage( "&OEl ya intenso calor se hace casi insoportable.&D\r\n", x, y );
                     else
                        WeatherMessage( "&OEl calor es casi insoportable.&D\r\n", x, y );
                  }
                  else if( isVeryHot( getTemp( cell ) ) )
                  { 
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 80 ) )
                        WeatherMessage( "&OA medida que la temperatura aumenta el calor se hace más intenso.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 90 ) )
                        WeatherMessage( "&OEl insoportable calor disminuye un poco.&D\r\n", x, y );
                     else
                        WeatherMessage( "&OHace mucho calor.&D\r\n", x, y );
                  }
                  else if( isHot( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 70 ) )
                        WeatherMessage( "&OLa temperatura se eleva y hace bastante calor.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 80 ) )
                        WeatherMessage( "&OLa temperatura desciende ligeramente, haciéndola un poquito  más soportable.&D\r\n", x, y );
                     else
                        WeatherMessage( "&OIt is hot.&D\r\n", x, y );
                  }
                  else if( isWarm( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 60 ) )
                        WeatherMessage( "&OLa agradable temperatura se eleva un poco haciéndola cálida.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 70 ) )
                        WeatherMessage( "&OEl calor se hace menos intenso.&D\r\n", x, y );
                     else
                        WeatherMessage( "&OEl tiempo es algo cálido.&D\r\n", x, y );
                  }
                  else if( isTemperate( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 50 ) )
                        WeatherMessage( "&OEl frío se aleja y el tiempo se hace más templado, bueno y agradable.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 60 ) )
                        WeatherMessage( "&OEl calor disminuye y se hace más agradable.&D\r\n", x, y );
                     else
                        WeatherMessage( "&OLa temperatura es buena y  agradable.&D\r\n", x, y );
                  }
                  else if( isCool( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 40 ) )
                        WeatherMessage( "&CEl aire frío se templa quedando fresco.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 50 ) )
                        WeatherMessage( "&CLa temperatura baja y el tiempo queda más fresco.&D\r\n", x, y );
                     else
                        WeatherMessage( "&CThe temperature is cool.&D\r\n", x, y );
                  }
                  else if( isChilly( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 30 ) )
                        WeatherMessage( "&CLa temperatura se eleva un poquito pero el aire aún permanece frío.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 40 ) )
                        WeatherMessage( "&CLa temperatura desciende quedando sensación de frío.&D\r\n", x, y );
                     else
                        WeatherMessage( "&CHace bastante frío.&D\r\n", x, y );
                  }
                  else if( isCold( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 20 ) )
                        WeatherMessage( "&CLa gélida temperatura  se suaviza un poco.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 30 ) )
                        WeatherMessage( "&CLa temperatura desciende y hace frío.&D\r\n", x, y );
                     else
                        WeatherMessage( "&CHace frío.&D\r\n", x, y );
                  }
                  else if( isFrosty( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 10 ) )
                        WeatherMessage( "&CLa gélida temperatura se suaviza un poco.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 20 ) )
                        WeatherMessage( "&La temperatura baja de fría a gélida.&D\r\n", x, y );
                     else
                        WeatherMessage( "&CLa temperatura es muy gélida.&D\r\n", x, y );
                  }
                  else if( isFreezing( getTemp( cell ) ) )
                  {
                     if( ExceedsThreshold( cell->temperature, delta->temperature, 0 ) )
                        WeatherMessage( "&CEl frío helador comienza a templarse ligeramente.&D\r\n", x, y );
                     else if( DropsBelowThreshold( cell->temperature, delta->temperature, 10 ) )
                        WeatherMessage( "&CLa temperatura sube de gélida a muy fría.&D\r\n", x, y );
                     else
                        WeatherMessage( "&CHace un frío helador.&D\r\n", x, y );
                  }
                  else if( isReallyCold( getTemp( cell ) ) )
                  {
                     if( DropsBelowThreshold( cell->temperature, delta->temperature, 0 ) )
                        WeatherMessage( "&CLa temperatura desciende haciendo que el frío helador empeore.&D\r\n", x, y );
                     else if( ExceedsThreshold( cell->temperature, delta->temperature, -10 ) )
                        WeatherMessage( "&CEl aire es muy frío pero la temperatura aumenta suavizándolo.&D\r\n", x, y );
                     else
                        WeatherMessage( "&CHace realmente frío.&D\r\n", x, y );
                  }
                  else if( isVeryCold( getTemp( cell ) ) )
                  {
                     if( DropsBelowThreshold( cell->temperature, delta->temperature, -10 ) )
                        WeatherMessage( "&CLa temperatura desciende haciendo todo más frío.&D\r\n", x, y );
                     else if( ExceedsThreshold( cell->temperature, delta->temperature, -20 ) )
                        WeatherMessage( "&CLa temperatura aumenta mejorando el insoportable frío.&D\r\n", x, y );
                     else
                        WeatherMessage( "&CHace mucho frío.&D\r\n", x, y );
                  }
                  else if( isExtremelyCold( getTemp( cell ) ) )
                  {
                     if( DropsBelowThreshold( cell->temperature, delta->temperature, -20 ) )
                        WeatherMessage( "&CLa temperatura ya de por sí fría desciende y se hace insoportable.&D\r\n", x, y );
                     else
                        WeatherMessage( "&CHace un frío insoportable.&D\r\n", x, y );
                  }
            }
         }
         //Here we actually apply the changes making sure they stay within specific bounds
         cell->temperature	= URANGE( -30, cell->temperature + delta->temperature, 100 );
         cell->pressure		= URANGE( 0, cell->pressure + delta->pressure, 100 );
         cell->cloudcover    = URANGE( 0, cell->cloudcover + delta->cloudcover, 100 );
         cell->energy		= URANGE( 0, cell->energy + delta->energy, 100 );
         cell->humidity		= URANGE( 0, cell->humidity + delta->humidity, 100 );
         cell->precipitation = URANGE( 0, cell->precipitation + delta->precipitation, 100 );
         cell->windSpeedX	= URANGE( -100, cell->windSpeedX + delta->windSpeedX, 100 );
         cell->windSpeedY	= URANGE( -100, cell->windSpeedY + delta->windSpeedY, 100 );
      }
   }
}

void ClearWeatherDeltas( void )
{                                                      // Clear delta map
    memset( weatherDelta, 0, sizeof( weatherDelta ) );
}

void CalculateCellToCellChanges( void )
{
    int                     x,
                            y;
    int                     rand;

    /*
     *  Randomly pick a cell to apply a random change to prevent equilibrium
     */
    x = number_range( 0, WEATHER_SIZE_X );
    y = number_range( 0, WEATHER_SIZE_Y );

    struct WeatherCell     *randcell = &weatherMap[x][y];   // Weather Cell

    rand = number_range( -10, 10 );

    switch ( dice( 1, 8 ) ) {
        case 1:
            randcell->cloudcover += rand;
            break;
        case 2:
            randcell->energy += rand;
            break;
        case 3:
            randcell->humidity += rand;
            break;
        case 4:
            randcell->precipitation += rand;
            break;
        case 5:
            randcell->pressure += rand;
            break;
        case 6:
            randcell->temperature += rand;
            break;
        case 7:
            randcell->windSpeedX += rand;
            break;
        case 8:
            randcell->windSpeedY += rand;
            break;
    }

    /*
     *  Iterate over every cell and set up the changes
     *  that will occur in that cell and it's neighbors
     *  based on the weather
     */
    for ( y = 0; y < WEATHER_SIZE_Y; y++ ) {
        for ( x = 0; x < WEATHER_SIZE_X; x++ ) {

            struct WeatherCell     *cell = &weatherMap[x][y];   // Weather cell
            struct WeatherCell     *delta = &weatherDelta[x][y];    // Where we

            // accumulate the
            // changes to apply

            /*
             *  Here we force the system to take day/night into account
             *  when determining temperature change.
             */
            if ( ( time_info.sunlight == SUN_RISE ) || ( time_info.sunlight == SUN_LIGHT ) )
                delta->temperature +=
                    ( number_range( -1, 2 ) + ( ( ( getCloudCover( cell ) / 10 ) > 5 ) ? -1 : 1 ) );
            if ( ( time_info.sunlight == SUN_SET ) || ( time_info.sunlight == SUN_DARK ) )
                delta->temperature +=
                    ( number_range( -2, 0 ) + ( ( ( getCloudCover( cell ) / 10 ) < 5 ) ? 2 : -3 ) );

            // Precipitation drops humidity by 5% of precip level
            if ( cell->precipitation > 40 )
                delta->humidity -= ( cell->precipitation / 20 );
            else
                delta->humidity += number_range( 0, 3 );

            // Humidity and pressure can affect the precipitation level
            int                     humidityAndPressure = ( cell->humidity + cell->pressure );

            if ( ( humidityAndPressure / 2 ) >= 60 )
                delta->precipitation += ( cell->humidity / 10 );
            else if ( ( humidityAndPressure / 2 ) < 60 && ( humidityAndPressure / 2 ) > 40 )
                delta->precipitation += number_range( -2, 2 );
            else if ( ( humidityAndPressure / 2 ) <= 40 )
                delta->precipitation -= ( cell->humidity / 5 );

            // Humidity and precipitation can affect the cloud cover
            int                     humidityAndPrecip = ( cell->humidity + cell->precipitation );

            if ( ( humidityAndPrecip / 2 ) >= 60 )
                delta->cloudcover -= ( cell->humidity / 10 );
            else if ( ( humidityAndPrecip / 2 ) < 60 && ( humidityAndPrecip / 2 ) > 40 )
                delta->cloudcover += number_range( -2, 2 );
            else if ( ( humidityAndPrecip / 2 ) <= 40 )
                delta->cloudcover += ( cell->humidity / 5 );

            int                     totalPressure = cell->pressure;
            int                     numPressureCells = 1;

            // Changes applied based on what is going on in adjacent cells
            int                     dx,
                                    dy;

            for ( dy = -1; dy <= 1; ++dy ) {
                for ( dx = -1; dx <= 1; ++dx ) {
                    int                     nx = x + dx;
                    int                     ny = y + dy;

                    // Skip THIS cell
                    if ( dx == 0 && dy == 0 )
                        continue;

                    // Prevent array over/underruns
                    if ( nx < 0 || nx >= WEATHER_SIZE_X )
                        continue;
                    if ( ny < 0 || ny >= WEATHER_SIZE_Y )
                        continue;

                    struct WeatherCell     *neighborCell = &weatherMap[nx][ny];
                    struct WeatherCell     *neighborDelta = &weatherDelta[nx][ny];

                    /*
                     *  We'll apply wind changes here
                     *  Wind speeds up in a given direction based on pressure
                     
                     *  1/4 of the pressure difference applied to wind speed
                     
                     *  Wind should move from higher pressure to lower pressure
                     *  and some of our pressure difference should go with it!
                     *  If we are pressure 60, and they are pressure 40
                     *  then with a difference of 20, lets make that a 4 mph
                     *  wind increase towards them!
                     *  So if they are west neighbor (dx < 0)
                     */

                    int                     pressureDelta = cell->pressure - neighborCell->pressure;
                    int                     windSpeedDelta = pressureDelta / 4;

                    if ( dx != 0 )                     // Neighbor to east or west
                        delta->windSpeedX += ( windSpeedDelta * dx );   // dx = -1 or 1
                    if ( dy != 0 )                     // Neighbor to north or south
                        delta->windSpeedY += ( windSpeedDelta * dy );   // dy = -1 or 1

                    totalPressure += neighborCell->pressure;
                    ++numPressureCells;

                    // Now GIVE them a bit of temperature and humidity change
                    // IF our wind is blowing towards them
                    int                     temperatureDelta =
                        ( cell->temperature - neighborCell->temperature );
                    temperatureDelta /= 16;

                    int                     humidityDelta = cell->humidity - neighborCell->humidity;

                    humidityDelta /= 16;

                    if ( ( cell->windSpeedX < 0 && dx < 0 )
                         || ( cell->windSpeedX > 0 && dx > 0 )
                         || ( cell->windSpeedY < 0 && dy < 0 )
                         || ( cell->windSpeedY > 0 && dy > 0 ) ) {
                        neighborDelta->temperature += temperatureDelta;
                        neighborDelta->humidity += humidityDelta;
                        delta->temperature -= temperatureDelta;
                        delta->humidity -= humidityDelta;
                    }
                    // Determine change in the energy of this particular Cell
                    int                     energyDelta = number_range( -10, 10 );

                    delta->energy += energyDelta;
                }
            }
            // Subtract because if positive means we are higher
            delta->pressure = ( ( totalPressure / numPressureCells ) - cell->pressure );

            /*
             * Precipitation should screw with pressure to keep the system from
             * reaching a balancing point.
             */
            if ( cell->precipitation >= 70 )
                delta->pressure -= ( cell->pressure / 2 );
            else if ( cell->precipitation < 70 && cell->precipitation > 30 )
                delta->pressure += number_range( -5, 5 );
            else if ( cell->precipitation <= 30 )
                delta->pressure += ( cell->pressure / 2 );
        }
    }
}

void EnforceClimateConditions( void )
{
    int                     x,
                            y;

    /*
     * This function is used to enforce certain conditions to be upheld
     * within cells.  The cells should have a climate set to them, which
     * tells this function which conditions to enforce. Conditions are pretty
     * straight forward, and tend to mesh with climate conditions around
     * Earth.
     */
    for ( y = 0; y < WEATHER_SIZE_Y; y++ ) {
        for ( x = 0; x < WEATHER_SIZE_X; x++ ) {

            struct WeatherCell     *cell = &weatherMap[x][y];   // Weather cell
            struct WeatherCell     *delta = &weatherDelta[x][y];

            if ( cell->climate == CLIMATE_RAINFOREST ) {
                if ( cell->temperature < 80 )
                    delta->temperature += 3;
                else if ( cell->humidity < 50 )
                    delta->humidity += 2;
            }
            else if ( cell->climate == CLIMATE_SAVANNA ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->humidity > 0 )
                    delta->humidity += -5;
                else if ( cell->temperature < 60 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity < 50 )
                    delta->humidity += 5;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity > 0 )
                    delta->humidity += -5;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity < 50 )
                    delta->humidity += 5;
            }
            else if ( cell->climate == CLIMATE_DESERT ) {
                if ( ( time_info.sunlight == SUN_SET || time_info.sunlight == SUN_DARK )
                     && cell->temperature > 30 )
                    delta->temperature += -5;
                else if ( ( time_info.sunlight == SUN_RISE || time_info.sunlight == SUN_LIGHT )
                          && cell->temperature < 64 )
                    delta->temperature += 2;
                else if ( cell->humidity > 10 )
                    delta->humidity += -2;
                else if ( cell->pressure < 50 )
                    delta->pressure += 2;
            }
            else if ( cell->climate == CLIMATE_STEPPE ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->temperature > 50 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature < 50 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 50 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature < 50 )
                    delta->temperature += 2;
                else if ( cell->humidity > 60 )
                    delta->temperature += -2;
                else if ( cell->humidity < 30 )
                    delta->temperature += 2;
            }
            else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->temperature > 50 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature < 75 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity > 30 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity < 30 )
                    delta->humidity += 2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 50 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature < 75 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity > 30 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity < 30 )
                    delta->humidity += 2;
            }
            else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->temperature > 50 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature < 75 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity > 40 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity < 30 )
                    delta->humidity += 2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 50 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature < 75 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity > 40 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity < 30 )
                    delta->humidity += 2;
            }
            else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->temperature > 40 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature < 65 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 40 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature < 65 )
                    delta->temperature += 2;
                else if ( cell->humidity < 30 )
                    delta->humidity += 2;
            }
            else if ( cell->climate == CLIMATE_TAIGA ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->temperature > 30 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature < 40 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 30 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 30 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity < 50 )
                    delta->humidity += 2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity > 20 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity > 20 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->humidity > 20 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 30 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature < 40 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 30 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 30 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity < 50 )
                    delta->humidity += 2;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity > 20 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity > 20 )
                    delta->humidity += -2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->humidity > 20 )
                    delta->humidity += -2;
            }
            else if ( cell->climate == CLIMATE_TUNDRA ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->temperature > 20 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature < 50 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 25 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 25 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 20 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature < 50 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 25 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 25 )
                    delta->temperature += -2;
            }
            else if ( cell->climate == CLIMATE_ALPINE ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->temperature > 20 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 50 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 25 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 25 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 20 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 50 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 25 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 25 )
                    delta->temperature += -2;
            }
            else if ( cell->climate == CLIMATE_ARCTIC ) {
                if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_NORTH
                     && cell->temperature > -10 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_WINTER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > -10 )
                    delta->temperature += 3;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > -10 )
                    delta->temperature += -3;
                else if ( time_info.season == SEASON_FALL && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > -10 )
                    delta->temperature += 3;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature < -5 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature < -5 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 10 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SUMMER && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 10 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature < -5 )
                    delta->temperature += 2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature < -5 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_NORTH
                          && cell->temperature > 10 )
                    delta->temperature += -2;
                else if ( time_info.season == SEASON_SPRING && cell->hemisphere == HEMISPHERE_SOUTH
                          && cell->temperature > 10 )
                    delta->temperature += 2;

                if ( time_info.season == SEASON_WINTER )
                    delta->humidity += -1;
                else if ( time_info.season == SEASON_FALL )
                    delta->humidity += 1;
                else if ( time_info.season == SEASON_SUMMER )
                    delta->humidity += 2;
                else if ( time_info.season == SEASON_SPRING )
                    delta->humidity += -2;
            }
        }
    }
}

void UpdateWeather( void )
{
    ClearWeatherDeltas(  );
    CalculateCellToCellChanges(  );
    EnforceClimateConditions(  );
    ApplyDeltaChanges(  );
    save_weathermap(  );
}

void RandomizeCells( void )
{
    int                     x,
                            y;

    /*
     * This function came about because of the inexplicable ability
     * of the system, to find its way around anything I coded, and
     * still manage to completely throw itself off based on a single
     * value reaching the Max or Min for that value.
     * What this does:
     * Every night at midnight(as per the single call to this function
     * in time_update()) It will randomize the values in each cell, 
     * based on hemisphere, climate, and season. 
     */
    for ( y = 0; y < WEATHER_SIZE_Y; y++ ) {
        for ( x = 0; x < WEATHER_SIZE_X; x++ ) {

            struct WeatherCell     *cell = &weatherMap[x][y];   // Weather cell

            if ( cell->hemisphere == HEMISPHERE_NORTH ) {
                if ( time_info.season == SEASON_SPRING ) {
                    if ( cell->climate == CLIMATE_RAINFOREST ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 70, 100 );
                        cell->precipitation = number_range( 70, 100 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_SAVANNA ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 20, 40 );
                        cell->humidity = number_range( 60, 80 );
                        cell->precipitation = number_range( 60, 80 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DESERT ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 10 );
                        cell->precipitation = number_range( 0, 10 );
                        cell->windSpeedX = number_range( -10, 10 );
                        cell->windSpeedY = number_range( -10, 10 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_STEPPE ) {
                        cell->temperature = number_range( 40, 70 );
                        cell->pressure = number_range( 20, 40 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -30, 30 );
                        cell->windSpeedY = number_range( -30, 30 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 50, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                        cell->temperature = number_range( 30, 50 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                        cell->temperature = number_range( 45, 65 );
                        cell->pressure = number_range( 20, 40 );
                        cell->cloudcover = number_range( 40, 60 );
                        cell->humidity = number_range( 40, 60 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TAIGA ) {
                        cell->temperature = number_range( 0, 30 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -50, 50 );
                        cell->windSpeedY = number_range( -50, 50 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TUNDRA ) {
                        cell->temperature = number_range( 10, 40 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ALPINE ) {
                        cell->temperature = number_range( 20, 50 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ARCTIC ) {
                        cell->temperature = number_range( 0, 20 );
                        cell->pressure = number_range( 85, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                }
                else if ( time_info.season == SEASON_SUMMER ) {
                    if ( cell->climate == CLIMATE_RAINFOREST ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 70, 100 );
                        cell->precipitation = number_range( 70, 100 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_SAVANNA ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 20, 40 );
                        cell->humidity = number_range( 60, 80 );
                        cell->precipitation = number_range( 60, 80 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DESERT ) {
                        cell->temperature = number_range( 80, 100 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 10 );
                        cell->precipitation = number_range( 0, 10 );
                        cell->windSpeedX = number_range( -10, 10 );
                        cell->windSpeedY = number_range( -10, 10 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_STEPPE ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -30, 30 );
                        cell->windSpeedY = number_range( -30, 30 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                        cell->temperature = number_range( 80, 100 );
                        cell->pressure = number_range( 50, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                        cell->temperature = number_range( 65, 95 );
                        cell->pressure = number_range( 60, 90 );
                        cell->cloudcover = number_range( 10, 30 );
                        cell->humidity = number_range( 10, 30 );
                        cell->precipitation = number_range( 10, 30 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TAIGA ) {
                        cell->temperature = number_range( 30, 70 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 20, 50 );
                        cell->humidity = number_range( 20, 50 );
                        cell->precipitation = number_range( 20, 50 );
                        cell->windSpeedX = number_range( -50, 50 );
                        cell->windSpeedY = number_range( -50, 50 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TUNDRA ) {
                        cell->temperature = number_range( 40, 70 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ALPINE ) {
                        cell->temperature = number_range( 30, 60 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ARCTIC ) {
                        cell->temperature = number_range( -30, -10 );
                        cell->pressure = number_range( 85, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                }
                else if ( time_info.season == SEASON_FALL ) {
                    if ( cell->climate == CLIMATE_RAINFOREST ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 70, 100 );
                        cell->precipitation = number_range( 70, 100 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_SAVANNA ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 60, 80 );
                        cell->cloudcover = number_range( 10, 30 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DESERT ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 10 );
                        cell->precipitation = number_range( 0, 10 );
                        cell->windSpeedX = number_range( -10, 10 );
                        cell->windSpeedY = number_range( -10, 10 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_STEPPE ) {
                        cell->temperature = number_range( 40, 70 );
                        cell->pressure = number_range( 20, 40 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -30, 30 );
                        cell->windSpeedY = number_range( -30, 30 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 30, 70 );
                        cell->cloudcover = number_range( 40, 60 );
                        cell->humidity = number_range( 40, 60 );
                        cell->precipitation = number_range( 20, 40 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                        cell->temperature = number_range( 30, 50 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                        cell->temperature = number_range( 55, 75 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 40, 60 );
                        cell->humidity = number_range( 40, 60 );
                        cell->precipitation = number_range( 40, 60 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TAIGA ) {
                        cell->temperature = number_range( 0, 30 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -50, 50 );
                        cell->windSpeedY = number_range( -50, 50 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TUNDRA ) {
                        cell->temperature = number_range( 10, 40 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 20, 60 );
                        cell->humidity = number_range( 20, 60 );
                        cell->precipitation = number_range( 20, 60 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ALPINE ) {
                        cell->temperature = number_range( 20, 50 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ARCTIC ) {
                        cell->temperature = number_range( 0, 20 );
                        cell->pressure = number_range( 85, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                }
                else if ( time_info.season == SEASON_WINTER ) {
                    if ( cell->climate == CLIMATE_RAINFOREST ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 70, 100 );
                        cell->precipitation = number_range( 70, 100 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_SAVANNA ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 60, 80 );
                        cell->cloudcover = number_range( 10, 30 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DESERT ) {
                        cell->temperature = number_range( 50, 70 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 10 );
                        cell->precipitation = number_range( 0, 10 );
                        cell->windSpeedX = number_range( -10, 10 );
                        cell->windSpeedY = number_range( -10, 10 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_STEPPE ) {
                        cell->temperature = number_range( -10, 20 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -30, 30 );
                        cell->windSpeedY = number_range( -30, 30 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                        cell->temperature = number_range( 40, 60 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 60, 80 );
                        cell->precipitation = number_range( 40, 60 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                        cell->temperature = number_range( -30, 0 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                        cell->temperature = number_range( 10, 30 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 40, 60 );
                        cell->humidity = number_range( 40, 60 );
                        cell->precipitation = number_range( 40, 60 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TAIGA ) {
                        cell->temperature = number_range( -30, 0 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -50, 50 );
                        cell->windSpeedY = number_range( -50, 50 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TUNDRA ) {
                        cell->temperature = number_range( -10, 20 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 20, 60 );
                        cell->humidity = number_range( 20, 60 );
                        cell->precipitation = number_range( 20, 60 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ALPINE ) {
                        cell->temperature = number_range( -30, 10 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ARCTIC ) {
                        cell->temperature = number_range( 30, 60 );
                        cell->pressure = number_range( 85, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                }
            }
            else {
                if ( time_info.season == SEASON_SPRING ) {
                    if ( cell->climate == CLIMATE_RAINFOREST ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 70, 100 );
                        cell->precipitation = number_range( 70, 100 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_SAVANNA ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 60, 80 );
                        cell->cloudcover = number_range( 10, 30 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DESERT ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 10 );
                        cell->precipitation = number_range( 0, 10 );
                        cell->windSpeedX = number_range( -10, 10 );
                        cell->windSpeedY = number_range( -10, 10 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_STEPPE ) {
                        cell->temperature = number_range( 40, 70 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -30, 30 );
                        cell->windSpeedY = number_range( -30, 30 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 30, 70 );
                        cell->cloudcover = number_range( 40, 60 );
                        cell->humidity = number_range( 40, 60 );
                        cell->precipitation = number_range( 20, 40 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                        cell->temperature = number_range( 30, 50 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                        cell->temperature = number_range( 55, 75 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 40, 60 );
                        cell->humidity = number_range( 40, 60 );
                        cell->precipitation = number_range( 40, 60 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TAIGA ) {
                        cell->temperature = number_range( 0, 30 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -50, 50 );
                        cell->windSpeedY = number_range( -50, 50 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TUNDRA ) {
                        cell->temperature = number_range( 10, 40 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 20, 60 );
                        cell->humidity = number_range( 20, 60 );
                        cell->precipitation = number_range( 20, 60 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ALPINE ) {
                        cell->temperature = number_range( 20, 50 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ARCTIC ) {
                        cell->temperature = number_range( 0, 20 );
                        cell->pressure = number_range( 85, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                }
                else if ( time_info.season == SEASON_SUMMER ) {
                    if ( cell->climate == CLIMATE_RAINFOREST ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 70, 100 );
                        cell->precipitation = number_range( 70, 100 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_SAVANNA ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 60, 80 );
                        cell->cloudcover = number_range( 10, 30 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DESERT ) {
                        cell->temperature = number_range( 80, 100 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 10 );
                        cell->precipitation = number_range( 0, 10 );
                        cell->windSpeedX = number_range( -10, 10 );
                        cell->windSpeedY = number_range( -10, 10 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_STEPPE ) {
                        cell->temperature = number_range( 40, 60 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 60, 80 );
                        cell->precipitation = number_range( 40, 60 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 50, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                        cell->temperature = number_range( -30, 0 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                        cell->temperature = number_range( 10, 30 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 40, 60 );
                        cell->humidity = number_range( 40, 60 );
                        cell->precipitation = number_range( 40, 60 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TAIGA ) {
                        cell->temperature = number_range( -30, 0 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -50, 50 );
                        cell->windSpeedY = number_range( -50, 50 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TUNDRA ) {
                        cell->temperature = number_range( -10, 20 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 20, 60 );
                        cell->humidity = number_range( 20, 60 );
                        cell->precipitation = number_range( 20, 60 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ALPINE ) {
                        cell->temperature = number_range( -30, 10 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ARCTIC ) {
                        cell->temperature = number_range( 30, 60 );
                        cell->pressure = number_range( 85, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                }
                else if ( time_info.season == SEASON_FALL ) {
                    if ( cell->climate == CLIMATE_RAINFOREST ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 70, 100 );
                        cell->precipitation = number_range( 70, 100 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_SAVANNA ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 20, 40 );
                        cell->humidity = number_range( 60, 80 );
                        cell->precipitation = number_range( 60, 80 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DESERT ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 10 );
                        cell->precipitation = number_range( 0, 10 );
                        cell->windSpeedX = number_range( -10, 10 );
                        cell->windSpeedY = number_range( -10, 10 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_STEPPE ) {
                        cell->temperature = number_range( 40, 70 );
                        cell->pressure = number_range( 20, 40 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -30, 30 );
                        cell->windSpeedY = number_range( -30, 30 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                        cell->temperature = number_range( -10, 20 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -30, 30 );
                        cell->windSpeedY = number_range( -30, 30 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                        cell->temperature = number_range( 30, 50 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                        cell->temperature = number_range( 45, 65 );
                        cell->pressure = number_range( 20, 40 );
                        cell->cloudcover = number_range( 40, 60 );
                        cell->humidity = number_range( 40, 60 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TAIGA ) {
                        cell->temperature = number_range( 0, 30 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -50, 50 );
                        cell->windSpeedY = number_range( -50, 50 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TUNDRA ) {
                        cell->temperature = number_range( 10, 40 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ALPINE ) {
                        cell->temperature = number_range( 20, 50 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ARCTIC ) {
                        cell->temperature = number_range( 0, 20 );
                        cell->pressure = number_range( 85, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                }
                else if ( time_info.season == SEASON_WINTER ) {
                    if ( cell->climate == CLIMATE_RAINFOREST ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 30, 60 );
                        cell->cloudcover = number_range( 50, 70 );
                        cell->humidity = number_range( 70, 100 );
                        cell->precipitation = number_range( 70, 100 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_SAVANNA ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 20, 40 );
                        cell->humidity = number_range( 60, 80 );
                        cell->precipitation = number_range( 60, 80 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DESERT ) {
                        cell->temperature = number_range( 50, 70 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 10 );
                        cell->precipitation = number_range( 0, 10 );
                        cell->windSpeedX = number_range( -10, 10 );
                        cell->windSpeedY = number_range( -10, 10 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_STEPPE ) {
                        cell->temperature = number_range( 70, 90 );
                        cell->pressure = number_range( 70, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -30, 30 );
                        cell->windSpeedY = number_range( -30, 30 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_CHAPPARAL ) {
                        cell->temperature = number_range( 80, 100 );
                        cell->pressure = number_range( 50, 90 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_GRASSLANDS ) {
                        cell->temperature = number_range( 60, 80 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_DECIDUOUS ) {
                        cell->temperature = number_range( 65, 95 );
                        cell->pressure = number_range( 60, 90 );
                        cell->cloudcover = number_range( 10, 30 );
                        cell->humidity = number_range( 10, 30 );
                        cell->precipitation = number_range( 10, 30 );
                        cell->windSpeedX = number_range( -40, 40 );
                        cell->windSpeedY = number_range( -40, 40 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TAIGA ) {
                        cell->temperature = number_range( 30, 70 );
                        cell->pressure = number_range( 40, 60 );
                        cell->cloudcover = number_range( 20, 50 );
                        cell->humidity = number_range( 20, 50 );
                        cell->precipitation = number_range( 20, 50 );
                        cell->windSpeedX = number_range( -50, 50 );
                        cell->windSpeedY = number_range( -50, 50 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_TUNDRA ) {
                        cell->temperature = number_range( 40, 70 );
                        cell->pressure = number_range( 80, 100 );
                        cell->cloudcover = number_range( 0, 20 );
                        cell->humidity = number_range( 0, 20 );
                        cell->precipitation = number_range( 0, 20 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ALPINE ) {
                        cell->temperature = number_range( 30, 60 );
                        cell->pressure = number_range( 30, 50 );
                        cell->cloudcover = number_range( 60, 80 );
                        cell->humidity = number_range( 50, 70 );
                        cell->precipitation = number_range( 50, 70 );
                        cell->windSpeedX = number_range( -60, 60 );
                        cell->windSpeedY = number_range( -60, 60 );
                        cell->energy = number_range( 0, 100 );
                    }
                    else if ( cell->climate == CLIMATE_ARCTIC ) {
                        cell->temperature = number_range( -30, -10 );
                        cell->pressure = number_range( 85, 100 );
                        cell->cloudcover = number_range( 0, 15 );
                        cell->humidity = number_range( 0, 15 );
                        cell->precipitation = number_range( 0, 15 );
                        cell->windSpeedX = number_range( -20, 20 );
                        cell->windSpeedY = number_range( -20, 20 );
                        cell->energy = number_range( 0, 100 );
                    }
                }
            }
        }
    }
}

const int               weatherVersion = 1;
int                     version;

void save_weathermap( void )
{
    int                     x,
                            y;
    char                    filename[MIL];
    FILE                   *fp;

    snprintf( filename, MIL, "%s%s", SYSTEM_DIR, WEATHER_FILE );
    if ( !( fp = fopen( filename, "w" ) ) ) {
        bug( "%s: fopen", __FUNCTION__ );
        perror( filename );
        return;
    }

    fprintf( fp, "#VERSION %d\n\n", weatherVersion );

    for ( y = 0; y < WEATHER_SIZE_Y; y++ ) {
        for ( x = 0; x < WEATHER_SIZE_X; x++ ) {
            struct WeatherCell     *cell = &weatherMap[x][y];

            fprintf( fp, "#CELL		%d %d\n", x, y );
            fprintf( fp, "Climate     %s~\n", climate_names[cell->climate] );
            fprintf( fp, "Hemisphere  %s~\n", hemisphere_name[cell->hemisphere] );
            fprintf( fp, "State       %d %d %d %d %d %d %d %d\n", cell->cloudcover, cell->energy,
                     cell->humidity, cell->precipitation, cell->pressure, cell->temperature,
                     cell->windSpeedX, cell->windSpeedY );
            fprintf( fp, "End\n\n" );
        }
    }
    fprintf( fp, "\n#END\n\n" );
    fclose( fp );
    fp = NULL;
    return;
}

void fread_cell( FILE * fp, int x, int y )
{
    bool                    fMatch = FALSE;

    struct WeatherCell     *cell = &weatherMap[x][y];

    for ( ;; ) {
        const char             *word = feof( fp ) ? "End" : fread_word( fp );
        char                    flag[MAX_INPUT_LENGTH];
        int                     value = 0;

        switch ( UPPER( word[0] ) ) {
            case '*':
                fread_to_eol( fp );
                break;

            case 'C':
                if ( !str_cmp( word, "Climate" ) ) {
                    if ( version >= 1 ) {
                        char                   *climate = NULL;

                        climate = fread_flagstring( fp );

                        while ( climate[0] != '\0' ) {
                            climate = one_argument( climate, flag );
                            value = get_climate( flag );
                            if ( value < 0 || value >= MAX_CLIMATE )
                                bug( "Unknown climate: %s", flag );
                            else
                                cell->climate = value;
                        }
                        fMatch = TRUE;
                        break;
                    }
                    else
                        cell->climate = fread_number( fp );
                    fMatch = TRUE;
                }
                break;

            case 'E':
                if ( !str_cmp( word, "End" ) )
                    return;
                break;

            case 'H':
                if ( !str_cmp( word, "Hemisphere" ) ) {
                    if ( version >= 1 ) {
                        char                   *hemisphere = NULL;

                        hemisphere = fread_flagstring( fp );

                        while ( hemisphere[0] != '\0' ) {
                            hemisphere = one_argument( hemisphere, flag );
                            value = get_hemisphere( flag );
                            if ( value < 0 || value >= HEMISPHERE_MAX )
                                bug( "Unknown hemisphere: %s", flag );
                            else
                                cell->hemisphere = value;
                        }
                        fMatch = TRUE;
                        break;
                    }
                    else
                        cell->hemisphere = fread_number( fp );
                    fMatch = TRUE;
                }
                break;

            case 'S':
                if ( !str_cmp( word, "State" ) ) {
                    cell->cloudcover = fread_number( fp );
                    cell->energy = fread_number( fp );
                    cell->humidity = fread_number( fp );
                    cell->precipitation = fread_number( fp );
                    cell->pressure = fread_number( fp );
                    cell->temperature = fread_number( fp );
                    cell->windSpeedX = fread_number( fp );
                    cell->windSpeedY = fread_number( fp );
                    fMatch = TRUE;
                    break;
                }
                break;
        }
        if ( !fMatch ) {
            bug( "%s: no match for %s", __FUNCTION__, word );
            fread_to_eol( fp );
        }
    }
}

bool load_weathermap( void )
{
    FILE                   *fp = NULL;
    char                    filename[256];
    int                     x,
                            y;

    version = 0;

    snprintf( filename, 256, "%s%s", SYSTEM_DIR, WEATHER_FILE );
    if ( !( fp = fopen( filename, "r" ) ) ) {
        bug( "load_weathermap(): cannot open %s for reading", filename );
        return FALSE;
    }
    for ( ;; ) {
        char                    letter = fread_letter( fp );
        char                   *word;

        if ( letter == '*' ) {
            fread_to_eol( fp );
            continue;
        }

        if ( letter != '#' ) {
            bug( "%s: # not found (%c)", __FUNCTION__, letter );
            return FALSE;
        }

        word = fread_word( fp );
        if ( !str_cmp( word, "VERSION" ) ) {
            version = fread_number( fp );
            continue;
        }
        if ( !str_cmp( word, "CELL" ) ) {
            x = fread_number( fp );
            y = fread_number( fp );
            fread_cell( fp, x, y );
            continue;
        }
        else if ( !str_cmp( word, "END" ) )
            break;
        else {
            bug( "%s: no match for %s", __FUNCTION__, word );
            continue;
        }
    }
    fclose( fp );
    fp = NULL;
    return TRUE;
}

/*
* Weather Utility Functions
* Designed to attempt to emulate encapsulation.
*/
struct WeatherCell     *getWeatherCell( AREA_DATA *pArea )
{
    return &weatherMap[pArea->weatherx][pArea->weathery];
}

void IncreaseTemp( struct WeatherCell *cell, int change )
{
    cell->temperature += change;
}

void DecreaseTemp( struct WeatherCell *cell, int change )
{
    cell->temperature -= change;
}

void IncreasePrecip( struct WeatherCell *cell, int change )
{
    cell->precipitation += change;
}

void DecreasePrecip( struct WeatherCell *cell, int change )
{
    cell->precipitation -= change;
}

void IncreasePressure( struct WeatherCell *cell, int change )
{
    cell->pressure += change;
}

void DecreasePressure( struct WeatherCell *cell, int change )
{
    cell->pressure -= change;
}

void IncreaseEnergy( struct WeatherCell *cell, int change )
{
    cell->energy += change;
}

void DecreaseEnergy( struct WeatherCell *cell, int change )
{
    cell->energy -= change;
}

void IncreaseCloudCover( struct WeatherCell *cell, int change )
{
    cell->cloudcover += change;
}

void DecreaseCloudCover( struct WeatherCell *cell, int change )
{
    cell->cloudcover -= change;
}

void IncreaseHumidity( struct WeatherCell *cell, int change )
{
    cell->humidity += change;
}

void DecreaseHumidity( struct WeatherCell *cell, int change )
{
    cell->humidity -= change;
}

void IncreaseWindX( struct WeatherCell *cell, int change )
{
    cell->windSpeedX += change;
}

void DecreaseWindX( struct WeatherCell *cell, int change )
{
    cell->windSpeedX -= change;
}

void IncreaseWindY( struct WeatherCell *cell, int change )
{
    cell->windSpeedY += change;
}

void DecreaseWindY( struct WeatherCell *cell, int change )
{
    cell->windSpeedY -= change;
}

/* Cloud cover Information */
int getCloudCover( struct WeatherCell *cell )
{
    return cell->cloudcover;
}

bool isExtremelyCloudy( int cloudCover )
{
    if ( cloudCover > 80 )
        return TRUE;
    else
        return FALSE;
}

bool isModeratelyCloudy( int cloudCover )
{
    if ( cloudCover > 60 && cloudCover <= 80 )
        return TRUE;
    else
        return FALSE;
}

bool isPartlyCloudy( int cloudCover )
{
    if ( cloudCover > 40 && cloudCover <= 60 )
        return TRUE;
    else
        return FALSE;
}

bool isCloudy( int cloudCover )
{
    if ( cloudCover > 20 && cloudCover <= 40 )
        return TRUE;
    else
        return FALSE;
}

/* Temperature Information */
int getTemp( struct WeatherCell *cell )
{
    return cell->temperature;
}

bool isSwelteringHeat( int temp )
{
    if ( temp > 90 )
        return TRUE;
    else
        return FALSE;
}

bool isVeryHot( int temp )
{
    if ( temp > 80 && temp <= 90 )
        return TRUE;
    else
        return FALSE;
}

bool isHot( int temp )
{
    if ( temp > 70 && temp <= 80 )
        return TRUE;
    else
        return FALSE;
}

bool isWarm( int temp )
{
    if ( temp > 60 && temp <= 70 )
        return TRUE;
    else
        return FALSE;
}

bool isTemperate( int temp )
{
    if ( temp > 50 && temp <= 60 )
        return TRUE;
    else
        return FALSE;
}

bool isCool( int temp )
{
    if ( temp > 40 && temp <= 50 )
        return TRUE;
    else
        return FALSE;
}

bool isChilly( int temp )
{
    if ( temp > 30 && temp <= 40 )
        return TRUE;
    else
        return FALSE;
}

bool isCold( int temp )
{
    if ( temp > 20 && temp <= 30 )
        return TRUE;
    else
        return FALSE;
}

bool isFrosty( int temp )
{
    if ( temp > 10 && temp <= 20 )
        return TRUE;
    else
        return FALSE;
}

bool isFreezing( int temp )
{
    if ( temp > 0 && temp <= 10 )
        return TRUE;
    else
        return FALSE;
}

bool isReallyCold( int temp )
{
    if ( temp > -10 && temp <= 0 )
        return TRUE;
    else
        return FALSE;
}

bool isVeryCold( int temp )
{
    if ( temp > -20 && temp <= -10 )
        return TRUE;
    else
        return FALSE;
}

bool isExtremelyCold( int temp )
{
    if ( temp <= -20 )
        return TRUE;
    else
        return FALSE;
}

/* Energy Information */
int getEnergy( struct WeatherCell *cell )
{
    return cell->energy;
}

bool isStormy( int energy )
{
    if ( energy > 50 )
        return TRUE;
    else
        return FALSE;
}

/* Pressure Information */
int getPressure( struct WeatherCell *cell )
{
    return cell->pressure;
}

bool isHighPressure( int pressure )
{
    if ( pressure > 50 )
        return TRUE;
    else
        return FALSE;
}

bool isLowPressure( int pressure )
{
    if ( pressure < 50 )
        return TRUE;
    else
        return FALSE;
}

/* Humidity Information */
int getHumidity( struct WeatherCell *cell )
{
    return cell->humidity;
}

bool isExtremelyHumid( int humidity )
{
    if ( humidity > 80 )
        return TRUE;
    else
        return FALSE;
}

bool isModeratelyHumid( int humidity )
{
    if ( humidity > 60 && humidity < 80 )
        return TRUE;
    else
        return FALSE;
}

bool isMinorlyHumid( int humidity )
{
    if ( humidity > 40 && humidity < 60 )
        return TRUE;
    else
        return FALSE;
}

bool isHumid( int humidity )
{
    if ( humidity > 20 && humidity < 40 )
        return TRUE;
    else
        return FALSE;
}

/* Precipitation Information */
int getPrecip( struct WeatherCell *cell )
{
    return cell->precipitation;
}

bool isTorrentialDownpour( int precip )
{
    if ( precip > 90 )
        return TRUE;
    else
        return FALSE;
}

bool isRainingCatsAndDogs( int precip )
{
    if ( precip > 80 && precip <= 90 )
        return TRUE;
    else
        return FALSE;
}

bool isPouring( int precip )
{
    if ( precip > 70 && precip <= 80 )
        return TRUE;
    else
        return FALSE;
}

bool isRaingingHeavily( int precip )
{
    if ( precip > 60 && precip <= 70 )
        return TRUE;
    else
        return FALSE;
}

bool isDownpour( int precip )
{
    if ( precip > 50 && precip <= 60 )
        return TRUE;
    else
        return FALSE;
}

bool isRainingSteadily( int precip )
{
    if ( precip > 40 && precip <= 50 )
        return TRUE;
    else
        return FALSE;
}

bool isRaining( int precip )
{
    if ( precip > 30 && precip <= 40 )
        return TRUE;
    else
        return FALSE;
}

bool isRainingLightly( int precip )
{
    if ( precip > 20 && precip <= 30 )
        return TRUE;
    else
        return FALSE;
}

bool isDrizzling( int precip )
{
    if ( precip > 10 && precip <= 20 )
        return TRUE;
    else
        return FALSE;
}

bool isMisting( int precip )
{
    if ( precip > 0 && precip <= 10 )
        return TRUE;
    else
        return FALSE;
}

/* WindX Information */
int getWindX( struct WeatherCell *cell )
{
    return cell->windSpeedX;
}

bool isCalmWindE( int windx )
{
    if ( windx > 0 && windx <= 10 )
        return TRUE;
    else
        return FALSE;
}

bool isBreezyWindE( int windx )
{
    if ( windx > 10 && windx <= 20 )
        return TRUE;
    else
        return FALSE;
}

bool isBlusteryWindE( int windx )
{
    if ( windx > 20 && windx <= 40 )
        return TRUE;
    else
        return FALSE;
}

bool isWindyWindE( int windx )
{
    if ( windx > 40 && windx <= 60 )
        return TRUE;
    else
        return FALSE;
}

bool isGustyWindE( int windx )
{
    if ( windx > 60 && windx <= 80 )
        return TRUE;
    else
        return FALSE;
}

bool isGaleForceWindE( int windx )
{
    if ( windx > 80 && windx <= 100 )
        return TRUE;
    else
        return FALSE;
}

bool isCalmWindW( int windx )
{
    if ( windx < 0 && windx >= -10 )
        return TRUE;
    else
        return FALSE;
}

bool isBreezyWindW( int windx )
{
    if ( windx < -10 && windx >= -20 )
        return TRUE;
    else
        return FALSE;
}

bool isBlusteryWindW( int windx )
{
    if ( windx < -20 && windx >= -40 )
        return TRUE;
    else
        return FALSE;
}

bool isWindyWindW( int windx )
{
    if ( windx < -40 && windx >= -60 )
        return TRUE;
    else
        return FALSE;
}

bool isGustyWindW( int windx )
{
    if ( windx < -60 && windx >= -80 )
        return TRUE;
    else
        return FALSE;
}

bool isGaleForceWindW( int windx )
{
    if ( windx < -80 && windx >= -100 )
        return TRUE;
    else
        return FALSE;
}

/* WindY Information */
int getWindY( struct WeatherCell *cell )
{
    return cell->windSpeedY;
}

bool isCalmWindN( int windy )
{
    if ( windy > 0 && windy <= 10 )
        return TRUE;
    else
        return FALSE;
}

bool isBreezyWindN( int windy )
{
    if ( windy > 10 && windy <= 20 )
        return TRUE;
    else
        return FALSE;
}

bool isBlusteryWindN( int windy )
{
    if ( windy > 20 && windy <= 40 )
        return TRUE;
    else
        return FALSE;
}

bool isWindyWindN( int windy )
{
    if ( windy > 40 && windy <= 60 )
        return TRUE;
    else
        return FALSE;
}

bool isGustyWindN( int windy )
{
    if ( windy > 60 && windy <= 80 )
        return TRUE;
    else
        return FALSE;
}

bool isGaleForceWindN( int windy )
{
    if ( windy > 80 && windy <= 100 )
        return TRUE;
    else
        return FALSE;
}

bool isCalmWindS( int windy )
{
    if ( windy < 0 && windy >= -10 )
        return TRUE;
    else
        return FALSE;
}

bool isBreezyWindS( int windy )
{
    if ( windy < -10 && windy >= -20 )
        return TRUE;
    else
        return FALSE;
}

bool isBlusteryWindS( int windy )
{
    if ( windy < -20 && windy >= -40 )
        return TRUE;
    else
        return FALSE;
}

bool isWindyWindS( int windy )
{
    if ( windy < -40 && windy >= -60 )
        return TRUE;
    else
        return FALSE;
}

bool isGustyWindS( int windy )
{
    if ( windy < -60 && windy >= -80 )
        return TRUE;
    else
        return FALSE;
}

bool isGaleForceWindS( int windy )
{
    if ( windy < -80 && windy >= -100 )
        return TRUE;
    else
        return FALSE;
}

void do_setweather( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            arg2[MIL],
                            arg3[MIL],
                            arg4[MIL];
    int                     value,
                            x,
                            y;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );
    argument = one_argument( argument, arg4 );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't setweather.\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "Nice try, but You have no descriptor.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' ) {
        send_to_char( "Syntax: setweather <x> <y> <field> <value>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Field being one of:\r\n", ch );
        send_to_char( "  climate hemisphere\r\n", ch );
        send_to_char( "Climate value being:\r\n", ch );
        send_to_char( "  rainforest savanna desert steppe chapparal arctic\r\n", ch );
        send_to_char( "  grasslands deciduous_forest taiga tundra alpine\r\n", ch );
        send_to_char( " See Help Climates for information on each.\r\n", ch );
        send_to_char( "Hemisphere value being:\r\n", ch );
        send_to_char( "  northern southern\r\n", ch );
        return;
    }

    x = atoi( arg );
    y = atoi( arg2 );

    if ( x < 0 || x > WEATHER_SIZE_X ) {
        ch_printf( ch, "X value must be between 0 and %d.\r\n", WEATHER_SIZE_X );
        return;
    }

    if ( y < 0 || y > WEATHER_SIZE_Y ) {
        ch_printf( ch, "Y value must be between 0 and %d.\r\n", WEATHER_SIZE_Y );
        return;
    }

    struct WeatherCell     *cell = &weatherMap[x][y];

    if ( !str_cmp( arg3, "climate" ) ) {
        if ( arg4[0] == '\0' ) {
            send_to_char( "Usage: setweather <x> <y> climate <flag>\r\n", ch );
            return;
        }

        value = get_climate( arg4 );
        if ( value < 0 || value > MAX_CLIMATE )
            ch_printf( ch, "Unknown flag: %s\r\n", arg4 );
        else {
            cell->climate = value;
            send_to_char( "Cell Climate set.\r\n", ch );
        }
        return;
    }

    if ( !str_cmp( arg3, "hemisphere" ) ) {
        if ( arg4[0] == '\0' ) {
            send_to_char( "Usage: setweather <x> <y> hemisphere <flag>\r\n", ch );
            return;
        }

        value = get_hemisphere( arg4 );
        if ( value < 0 || value > HEMISPHERE_MAX )
            ch_printf( ch, "Unknown flag: %s\r\n", arg4 );
        else {
            cell->hemisphere = value;
            send_to_char( "Cell Hemisphere set.\r\n", ch );
        }
        return;
    }
    else {
        send_to_char( "Syntax: setweather <x> <y> <field> <value>\r\n", ch );
        send_to_char( "\r\n", ch );
        send_to_char( "Field being one of:\r\n", ch );
        send_to_char( "  climate hemisphere\r\n", ch );
        send_to_char( "Climate value being:\r\n", ch );
        send_to_char( "  rainforest savanna desert steppe chapparal arctic\r\n", ch );
        send_to_char( "  grasslands deciduous_forest taiga tundra alpine\r\n", ch );
        send_to_char( " See Help Climates for information on each.\r\n", ch );
        send_to_char( "Hemisphere value being:\r\n", ch );
        send_to_char( "  northern southern\r\n", ch );
        return;
    }
}

void do_showweather( CHAR_DATA *ch, char *argument )
{
    char                    arg[MIL],
                            arg2[MIL];
    int                     x,
                            y;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( IS_NPC( ch ) ) {
        send_to_char( "Mob's can't showweather.\r\n", ch );
        return;
    }

    if ( !ch->desc ) {
        send_to_char( "Nice try, but You have no descriptor.\r\n", ch );
        return;
    }

    if ( arg[0] == '\0' || arg2[0] == '\0' ) {
        send_to_char( "Syntax: showweather <x> <y>\r\n", ch );
        return;
    }

    x = atoi( arg );
    y = atoi( arg2 );

    if ( x < 0 || x > WEATHER_SIZE_X - 1 ) {
        ch_printf( ch, "X value must be between 0 and %d.\r\n", WEATHER_SIZE_X - 1 );
        return;
    }

    if ( y < 0 || y > WEATHER_SIZE_Y - 1 ) {
        ch_printf( ch, "Y value must be between 0 and %d.\r\n", WEATHER_SIZE_Y - 1 );
        return;
    }

    struct WeatherCell     *cell = &weatherMap[x][y];

    ch_printf_color( ch, "Current Weather State for:\r\n" );
    ch_printf_color( ch, "&WCell (&w%d&W, &w%d&W)&D\r\n", x, y );
    ch_printf_color( ch, "&WClimate:           &w%s&D\r\n", climate_names[cell->climate] );
    ch_printf_color( ch, "&WHemispere:         &w%s&D\r\n", hemisphere_name[cell->hemisphere] );
    ch_printf_color( ch, "&WCloud Cover:       &w%d&D\r\n", cell->cloudcover );
    ch_printf_color( ch, "&WEnergy:            &w%d&D\r\n", cell->energy );
    ch_printf_color( ch, "&WTemperature:       &w%d&D\r\n", cell->temperature );
    ch_printf_color( ch, "&WPressure:          &w%d&D\r\n", cell->pressure );
    ch_printf_color( ch, "&WHumidity:          &w%d&D\r\n", cell->humidity );
    ch_printf_color( ch, "&WPrecipitation:     &w%d&D\r\n", cell->precipitation );
    ch_printf_color( ch, "&WWind Speed XAxis:  &w%d&D\r\n", cell->windSpeedX );
    ch_printf_color( ch, "&WWind Speed YAxis:  &w%d&D\r\n", cell->windSpeedY );
}

void do_weather( CHAR_DATA *ch, char *argument )
{
   struct WeatherCell *cell = getWeatherCell( ch->in_room->area );

   if( IS_NPC( ch ) )
   {
      send_to_char( "Mob's no pueden comprobar el tiempo.\r\n", ch );
      return;
   }

   if( !ch->desc )
   {
      send_to_char( "Bonito intento, pero no tienes descriptor.\r\n", ch );
      return;
   }

   if( !IS_OUTSIDE( ch ) && NO_WEATHER_SECT( ch->in_room->sector_type ) )
   {
      send_to_char( "Necesitas salir para hacer eso!\r\n", ch );
      return;
   }

   ch_printf_color( ch, "&wTomas consciencia del tiempo a medida que lo compruebas:&D\r\n" );
   if( getPrecip( cell ) > 0 )
   {
      if( isTorrentialDownpour( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WSe forma una ventisca de nieve de forma que difícilmente puedes ver!&D\r\n" );
         else
            ch_printf_color( ch, "&BLlueve torrencialmente!&D\r\n" );
      }
      else if( isRainingCatsAndDogs( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WLa nieve está formando una sólida pared blanca!&D\r\n" );
         else
            ch_printf_color( ch, "&BLlueve en forma de enormes gotas!&D\r\n" );
      }
      else if( isPouring( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WLa nieve cae fuerte.&D\r\n" );
         else
            ch_printf_color( ch, "&BCae un aguacero.&D\r\n" );
      }
      else if( isRaingingHeavily( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WLa nieve cae fuertemente.&D\r\n" );
         else
            ch_printf_color( ch, "&BLa lluvia cae fuertemente.&D\r\n" );
      }
      else if( isDownpour( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WCae nieve en fuertes oleadas.&D\r\n" );
         else
            ch_printf_color( ch, "&BLlueve a cántaros.&D\r\n" );
      }
      else if( isRainingSteadily( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WLa nieve cae bastante constante.&D\r\n" );
         else
            ch_printf_color( ch, "&BLa lluvia cae bastante continuada.&D\r\n" );
      }
      else if( isRaining( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WCaen montones de copos de nieve.&D\r\n" );
         else
            ch_printf_color( ch, "&BLlueve.&D\r\n" );
      }
      else if( isRainingLightly( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WUna nieve ligera cae a tu alrededor..&D\r\n" );
         else
            ch_printf_color( ch, "&BUna lluvia ligera marca el suelo a tu alrededor.&D\r\n" );
      }
      else if( isDrizzling( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WCaen ráfagas de nieve sobre ti.&D\r\n" );
         else
            ch_printf_color( ch, "&BParece que cae un ligero chaparrón.&D\r\n" );
      }
      else if( isMisting( getPrecip( cell ) ) )
      {
         if( getTemp( cell ) <= 32 )
            ch_printf_color( ch, "&WSe observan algunos copos de nieve dispersos.&D\r\n" );
         else
            ch_printf_color( ch, "&BAparece una ligera neblina.&D\r\n" );
      }
   }
   else
      ch_printf_color( ch, "&BNo parece que allí haya  precipitación alguna.&D\r\n" );

   if( getCloudCover( cell ) > 0 )
   {
      if( isExtremelyCloudy( getCloudCover( cell ) ) )
         ch_printf_color( ch, "&wUn manto de nubes cubre el cielo.&D\r\n" );
      if( isModeratelyCloudy( getCloudCover( cell ) ) )
         ch_printf_color( ch, "&wParece que allá hay un buen cúmulo de nubes.&D\r\n" );
      if( isPartlyCloudy( getCloudCover( cell ) ) )
         ch_printf_color( ch, "&wEl cielo aparece parcialmente nublado.&D\r\n" );
      if( isCloudy( getCloudCover( cell ) ) )
         ch_printf_color( ch, "&wHay algunas nubes dispersas en el cielo.&D\r\n" );
   }
   else
      ch_printf_color( ch, "&wNo parece que allí haya nubes.&D\r\n" );

   if( getHumidity( cell ) > 0 )
   {
      if( isExtremelyHumid( getHumidity( cell ) ) )
         ch_printf_color( ch, "&cSientes la piel horriblemente pegajosa por la extrema humedad. .&D\r\n" );
      else if( isModeratelyHumid( getHumidity( cell ) ) )
         ch_printf_color( ch, "&cTe sientes ligeramente pegajoso por la humedad moderada.&D\r\n" );
      else if( isMinorlyHumid( getHumidity( cell ) ) )
         ch_printf_color( ch, "&cTu piel pegajosa se hace menos evidente  ante el menor grado de humedad.&D\r\n" );
      else if( isHumid( getHumidity( cell ) ) )
         ch_printf_color( ch, "&cEs perfecto sentir el  aire contra tu piel.&D\r\n" );
      else
         ch_printf_color( ch, "&cNo puedes sentir la diferencia de humedad.&D\r\n" );
   }
   else
      ch_printf_color( ch, "&cel aire parece que absorbe la humedad de tu rostro.&D\r\n" );

   if( getWindX( cell ) != 0 && getWindY( cell ) != 0 )
   {
      if( isCalmWindE( getWindX( cell ) ) && isCalmWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento calmado roza tu piel desde el sudeste.&D\r\n" );
      else if( isCalmWindE( getWindX( cell ) ) && isCalmWindN( getWindY( cell ) ) ) 
         ch_printf_color( ch, "&GUn viento calmado roza tu piel desde el nordeste.&D\r\n" );
      else if( isBreezyWindE( getWindX( cell ) ) && isBreezyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento constante procede del sudeste.&D\r\n" );
      else if( isBreezyWindE( getWindX( cell ) ) && isBreezyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento constante procede del nordeste.&D\r\n" );
      else if( isBlusteryWindE( getWindX( cell ) ) && isBlusteryWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GSopla un viento borrascoso desde el sudeste.&D\r\n" );
      else if( isBlusteryWindE( getWindX( cell ) ) && isBlusteryWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GSopla un viento borrascoso desde el nordeste.&D\r\n" );
      else if( isWindyWindE( getWindX( cell ) ) && isWindyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento fuerte y constante aúlla desde el  sudeste.&D\r\n" );
      else if( isWindyWindE( getWindX( cell ) ) && isWindyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento fuerte y constante aúlla desde el nordeste.&D\r\n" );
      else if( isGustyWindE( getWindX( cell ) ) && isGustyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GRáfagas de viento llegan desde el sudeste.&D\r\n" );
      else if( isGustyWindE( getWindX( cell ) ) && isGustyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GRáfagas de viento llegan desde el nordeste.&D\r\n" );
      else if( isGaleForceWindE( getWindX( cell ) ) && isGaleForceWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn temporal arrastra el aire del sudeste.&D\r\n" );
      else if( isGaleForceWindE( getWindX( cell ) ) && isGaleForceWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn temporal arrastra el aire del  nordeste.&D\r\n" );

      else if( isCalmWindW( getWindX( cell ) ) && isCalmWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento calmado roza tu piel desde el suroeste.&D\r\n" );
      else if( isCalmWindW( getWindX( cell ) ) && isCalmWindN( getWindY( cell ) ) ) 
         ch_printf_color( ch, "&GUn viento calmado roza tu piel desde el noroeste.&D\r\n" );
      else if( isBreezyWindW( getWindX( cell ) ) && isBreezyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento constante procede del suroeste.&D\r\n" );
      else if( isBreezyWindW( getWindX( cell ) ) && isBreezyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento constante procede del noroeste.&D\r\n" );
      else if( isBlusteryWindW( getWindX( cell ) ) && isBlusteryWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GSopla un viento borrascoso desde el suroeste.&D\r\n" );
      else if( isBlusteryWindW( getWindX( cell ) ) && isBlusteryWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GSopla un viento borrascoso desde el noroeste.&D\r\n" );
      else if( isWindyWindW( getWindX( cell ) ) && isWindyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento fuerte y constante aúlla desde el suroeste.&D\r\n" );
      else if( isWindyWindW( getWindX( cell ) ) && isWindyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento fuerte y constante aúlla desde el  noroeste.&D\r\n" );
      else if( isGustyWindW( getWindX( cell ) ) && isGustyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GRáfagas de viento llegan desde el suroeste.&D\r\n" );
      else if( isGustyWindW( getWindX( cell ) ) && isGustyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GRáfagas de viento llegan desde el noroeste.&D\r\n" );
      else if( isGaleForceWindW( getWindX( cell ) ) && isGaleForceWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn temporal arrastra el aire del suroeste.&D\r\n" );
      else if( isGaleForceWindW( getWindX( cell ) ) && isGaleForceWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn temporal arrastra el aire del enoroeste.&D\r\n" );

      else
         ch_printf_color( ch, "&GEl viento sopla de forma caótica, ¡No puedes decir de dónde procede!&D\r\n" );
   }
   else if( getWindX( cell ) != 0 && getWindY( cell ) == 0 )
   {
      if( isCalmWindE( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento calmado roza tu piel desde el este.&D\r\n" );
      else if( isBreezyWindE( getWindX( cell ) ) )
         ch_printf_color( ch, "&GUn viento constante procede del este.&D\r\n" );
      else if( isBlusteryWindE( getWindX( cell ) ) )
         ch_printf_color( ch, "&GSopla un viento borrascoso desde el este.&D\r\n" );
      else if( isWindyWindE( getWindX( cell ) ) )
         ch_printf_color( ch, "&GUn viento fuerte y constante aúlla desde el este.&D\r\n" );
      else if( isGustyWindE( getWindX( cell ) ) )
         ch_printf_color( ch, "&GRáfagas de viento llegan desde el este.&D\r\n" );
      else if( isGaleForceWindE( getWindX( cell ) ) )
         ch_printf_color( ch, "&GUn temporal arrastra el aire del sur este.&D\r\n" );

      else if( isCalmWindW( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento calmado roza tu piel desde el oeste.&D\r\n" );
      else if( isBreezyWindW( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento constante procede del oeste.&D\r\n" );
      else if( isBlusteryWindW( getWindY( cell ) ) )
         ch_printf_color( ch, "&GSopla un viento borrascoso desde el oeste.&D\r\n" );
      else if( isWindyWindW( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento fuerte y constante aúlla desde el oeste.&D\r\n" );
      else if( isGustyWindW( getWindY( cell ) ) )
         ch_printf_color( ch, "&GRáfagas de viento llegan desde el oeste.&D\r\n" );
      else if( isGaleForceWindW( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn temporal arrastra el aire del oeste.&D\r\n" );
   }
   else if( getWindX( cell ) == 0 && getWindY( cell ) != 0 )
   {
      if( isCalmWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento calmado roza tu piel desde el sur.&D\r\n" );
      else if( isBreezyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento constante procede del sur.&D\r\n" );
      else if( isBlusteryWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GSopla un viento borrascoso desde el sur.&D\r\n" );
      else if( isWindyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento fuerte y constante aúlla desde el sur.&D\r\n" );
      else if( isGustyWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GRáfagas de viento llegan desde el sur.&D\r\n" );
      else if( isGaleForceWindS( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn temporal arrastra el aire del sur.&D\r\n" );

      else if( isCalmWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento calmado roza tu piel desde el norte.&D\r\n" );
      else if( isBreezyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento constante procede del norte.&D\r\n" );
      else if( isBlusteryWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GSopla un viento borrascoso desde el norte.&D\r\n" );
      else if( isWindyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn viento fuerte y constante aúlla desde el  norte.&D\r\n" );
      else if( isGustyWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GRáfagas de viento llegan desde el norte.&D\r\n" );
      else if( isGaleForceWindN( getWindY( cell ) ) )
         ch_printf_color( ch, "&GUn temporal arrastra el aire del norte.&D\r\n" );
   }
   else
      ch_printf_color( ch, "&GParece que allí no hace viento.\r\n" );

   if( getTemp( cell ) > -30 && getTemp( cell ) < 100 )
   {
      if( isSwelteringHeat( getTemp( cell ) ) )
         ch_printf_color( ch, "&OEl calor es casi insoportable.&D\r\n" );
      else if( isVeryHot( getTemp( cell ) ) )
         ch_printf_color( ch, "&OHace mucho calor...&D\r\n" );
      else if( isHot( getTemp( cell ) ) )
         ch_printf_color( ch, "&OHace calor...&D\r\n" );
      else if( isWarm( getTemp( cell ) ) )
         ch_printf_color( ch, "&OParece que el tiempo es algo cálido.&D\r\n" );
      else if( isTemperate( getTemp( cell ) ) )
         ch_printf_color( ch, "&OLa temperatura es ideal.&D\r\n" );
      else if( isCool( getTemp( cell ) ) )
         ch_printf_color( ch, "&CParece que hace un poco de frío.&D\r\n" );
      else if( isChilly( getTemp( cell ) ) )
         ch_printf_color( ch, "&CParece que hace algo de frío.&D\r\n" );
      else if( isCold( getTemp( cell ) ) )
         ch_printf_color( ch, "&CHace frío.&D\r\n" );
      else if( isFrosty( getTemp( cell ) ) )
         ch_printf_color( ch, "&CHay una visible helada alrededor.&D\r\n" );
      else if( isFreezing( getTemp( cell ) ) )
         ch_printf_color( ch, "&CTu aliento se cristaliza delante de tu rostro.&D\r\n" );
      else if( isReallyCold( getTemp( cell ) ) )
         ch_printf_color( ch, "&CHace realmente frío...&D\r\n" );
      else if( isVeryCold( getTemp( cell ) ) )
         ch_printf_color( ch, "&CCrees que el hielo forma parte de tu ropa.&D\r\n" );
      else if( isExtremelyCold( getTemp( cell ) ) )
         ch_printf_color( ch, "&CSientes el frío adherido a tu cara. Ve dentro!&D\r\n" );
   }
}

