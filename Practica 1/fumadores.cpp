#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;

//variables compartidas
const int MAX = 3;
Semaphore mostr_vacio=1;
Semaphore ing_disp[MAX]={0,0,0};
mutex mtx;




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

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
  int ingrediente;
  chrono::milliseconds duracion_fumar( aleatorio<20,200>() );
  this_thread::sleep_for( duracion_fumar );

  while(true){
 
    
    sem_wait(mostr_vacio);
    ingrediente = aleatorio<0,2>();
    mtx.lock();
    cout<<"Puesto ingrediente "<<ingrediente<<"en el mostrador."<<endl;
    mtx.unlock();
    sem_signal(ing_disp[ingrediente]);
 
  }


}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{
   mtx.lock();
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
     mtx.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
     
     sem_wait(ing_disp[num_fumador]);
     mtx.lock();
     cout<<"retirado ingrediente "<<num_fumador<<"."<<endl;
     mtx.unlock();
     sem_signal(mostr_vacio);
     fumar(num_fumador);

   }
}

//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los fumadores." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread hebra_estanquero( funcion_hebra_estanquero );
   thread hebra_fumadores[MAX];
      
      for(int i=0;i<MAX;i++){
          hebra_fumadores[i] = thread(funcion_hebra_fumador,i);
      }
      for(int i=0 ;i<MAX;i++){
        hebra_fumadores[i].join();
      }
  
   hebra_estanquero.join();

}


