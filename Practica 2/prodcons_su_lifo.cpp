// -----------------------------------------------------------------------------
/*
* Práctica 2 - Sistemas Concurrentes y Distribuidos
* Problema de los productores y consumidores múltiples, versión LIFO
* y señales SU (Señalar y Espera Urgente)
*/
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include "HoareMonitor.h"
#include <condition_variable>
#include <random>

using namespace std ;
using namespace HM ;

constexpr int
   num_items  = 40 ,     // número de items a producir/consumir
   n_productores=8,             // mutex de escritura en pantalla
   n_consumidores=5;

unsigned
   cont_prod[num_items]={0}, // contadores de verificación: producidos
   cont_cons[num_items]={0}; // contadores de verificación: consumidos

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

int producir_dato(int numero_hebra)
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   cout << "producido: " << contador << endl << flush ;
   cont_prod[contador] ++ ;
   return contador++ ;
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
   cout << "                  consumido: " << dato << endl ;
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

class ProdCons1SU : public HoareMonitor
{
 private:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   primera_libre ;          //  indice de celda de la próxima inserción

	CondVar libres;
	CondVar ocupadas; //Variables condición

 public:                    // constructor y métodos públicos
   ProdCons1SU(  ) ;           // constructor
   void  insertar(int dato);
   int extraer();

} ;
// -----------------------------------------------------------------------------

ProdCons1SU::ProdCons1SU(  )
{
   primera_libre = 0 ;
   libres = newCondVar();
   ocupadas = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

void ProdCons1SU::insertar( int dato )
{
	if(primera_libre == num_celdas_total)
		libres.wait();

	primera_libre++;
	int valor = buffer[primera_libre];
	
	ocupadas.signal();

}
// -----------------------------------------------------------------------------

int ProdCons1SU::extraer( )
{
	if(primera_libre == 0)
		ocupadas.wait();

	primera_libre--;
	int valor = buffer[primera_libre];
	
	libres.signal();

	return valor;
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora(MRef<ProdCons1SU> monitor, int numero_hebra)
{
	int max=num_items/n_productores;
   for( unsigned i = 0 ; i < max ; i++ )
   {
      int valor = producir_dato(numero_hebra) ;
      monitor->insertar( valor );
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora(MRef<ProdCons1SU> monitor, int numero_hebra)
{
   int max=num_items/n_consumidores;
   for( unsigned i = 0 ; i < max ; i++ )
   {
      int valor = monitor->extraer();
      consumir_dato( valor ) ;
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores varios productores y consumidores LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdCons1SU> monitor = Create<ProdCons1SU>();
   thread hebra_productora[n_productores];
   thread hebra_consumidora[n_consumidores];

for (int i=0; i<n_productores;i++){
	 hebra_productora[i]=thread ( funcion_hebra_productora, monitor,n_productores );
}
for (int i = 0; i < n_consumidores; ++i){
   	 hebra_consumidora[i] =thread ( funcion_hebra_consumidora, monitor, n_consumidores );
}   
          
for (int i= 0;i<n_productores;i++){
   hebra_productora[i].join() ;
}
for (int i= 0;i<n_consumidores;i++){
   hebra_consumidora[i].join() ;
}
   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
