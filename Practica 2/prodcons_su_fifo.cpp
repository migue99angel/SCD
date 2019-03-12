// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_1.cpp
// Ejemplo de un monitor en C++11 con semántica SC, para el problema
// del productor/consumidor, con un único productor y un único consumidor.
// Opcion LIFO (stack)
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <condition_variable>
#include "HoareMonitor.h"
#include <random>

using namespace std ;
using namespace HM ;


constexpr int
   num_items  = 15 ;     // número de items a producir/consumir
int   num_hebras_prod = 5;
int   num_hebras_cons = 3;
int   num_items_prod = num_items/num_hebras_prod;
int   num_items_cons = num_items/num_hebras_cons;


mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items]; // contadores de verificación: consumidos
int    

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int num_prod)
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const int valor_producido = num_prod*num_items_prod +contador[num_prod];
   contador[num_prod]++;
   mtx.lock();
   cout << "Hebra productora: " << num_prod << "produce" << valor_producido<<endl<<flush ;
   mtx.unlock();
   cont_prod[contador] ++ ;
   return valor_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdConsSU
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   primera_libre,  
   primera_ocupada,
   num_celdas_ocupadas;        //  indice de celda de la próxima inserción
 mutex
   cerrojo_monitor ;        // cerrojo del monitor
CondVar        // colas condicion:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdConsSU(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
} ;
// -----------------------------------------------------------------------------

ProdConsSU::ProdConsSU(  )
{
   primera_libre = 0 ;
   primera_ocupada = 0;
   num_celdas_ocupadas = 0;
   ocupadas = newCondVar();
   libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsSU::leer(  )
{
   // esperar bloqueado hasta que 0 < num_celdas_ocupadas
   if ( primera_ocupada == 0 )
      ocupadas.wait( );

   // hacer la operación de lectura, actualizando estado del monitor
   assert( 0 < primera_ocupada  );
   const int valor = buffer[primera_ocupada] ;
   primera_ocupada=(primera_ocupada+1) %num_celdas_total;
   num_celdas_ocupadas--;


   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdConsSU::escribir( int valor )
{

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( num_celdas_ocupadas == num_celdas_total )
      libres.wait(  );

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( primera_libre < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[primera_libre] = valor ;
   primera_libre=(primera_libre+1) %num_celdas_total;
   num_celdas_ocupadas++;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdConsSU> monitor ,int num_prod)
{
   register_thread_name("productor",num_prod);
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int valor = producir_dato(num_prod) ;
      monitor->escribir( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdConsSU> monitor ,int num_prod )
{
   register_thread_name("consumidor",num_prod);

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (1 prod/cons, Monitor SU, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;


   MRef<ProdConsSU> monitor = Create<ProdConsSU>( num_items );


 for (int i=0; i<num_hebras_prod;i++){
   thread hebra_productora ( funcion_hebra_productora, &monitor,i );
}
for (int i = 0; i < num_hebras_cons; ++i){
      thread hebra_consumidora( funcion_hebra_consumidora, &monitor,i );
}   
          
for (int i= 0;i<num_hebras_prod;i++){
   hebra_productora[i].join() ;
}
for (int i= 0;i<num_hebras_cons;i++){
   hebra_consumidora[i].join() ;
}
   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
