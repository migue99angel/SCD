/*
  Sistemas Concurrentes y Distribuidos
  Práctica 2: Problema de los fumadores con Monitores de Hoare y señales SU
  autor: Miguel Ángel Posadas Arráez 2ºB(1)
*/

#include <iostream>
#include <cassert>
#include <iomanip>
#include <thread>
#include <random>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;


const int MAX = 3;
mutex mtx;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max );
  return distribucion_uniforme( generador );
}


void fumar(int num_fumador)
{
  chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

  mtx.lock();
  cout << "\t\tFumador " << num_fumador << ": empieza a fumar ("
       << duracion_fumar.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_fumar);

  mtx.lock();
  cout << "\t\tFumador " << num_fumador << ": termina de fumar, comienza espera de ingrediente "
       << num_fumador << "." << endl;
  mtx.unlock();
}

//Devuelve un entero entre 0y 2 simulando el ingrediente producido

int generar_ingrediente()
{
  this_thread::sleep_for(chrono::milliseconds( aleatorio<20,200>() ));
  return aleatorio<0,MAX-1>();
}

//Clase del monitor 

class Estanco : public HoareMonitor {
  private:
    int ingrediente;
    //variables condicion
    CondVar mostrador_vacio;
    CondVar ingrediente_disponible[MAX];

  public:
    Estanco();
    void obtenerIngrediente(int i);
    void colocar_ingrediente(int i);
    void esperar_recogida();
};

/* Constructor por defecto */
Estanco::Estanco()
{
  ingrediente = NULL;
  mostrador_vacio = newCondVar();

  for (int i=0; i<MAX; i++){
    ingrediente_disponible[i] = newCondVar();
  }
}

//El fumador está bloqueado hasta que el ingrediente esta sobre el mostrador
void Estanco::obtenerIngrediente(int i)
{
  /* Espera a que el ingrediente esté disponible */
  if (ingrediente != i){
    ingrediente_disponible[i].wait();
  }

  /* Vuelve a ponerlo a NULL para indicar que ya no hay ingrediente */
  ingrediente = NULL;

  mtx.lock();
  cout << "\t\tRetirado ingrediente: " << i << endl;
  mtx.unlock();

  mostrador_vacio.signal();
}

/* Función colocar_ingrediente. Pone el ingrediente i en el mostrador */
void Estanco::colocar_ingrediente(int i)
{
  ingrediente = i;

  mtx.lock();
  cout << "Puesto el ingrediente: " << ingrediente << endl;
  mtx.unlock();

  /* Envía una señal de que el ingrediente i está disponible */
  ingrediente_disponible[i].signal();
}

// Función llamada por el estanquero en cada iteración de su bucle infinito. Espera bloqueado hasta que el mostrador esté libre 
void Estanco::esperar_recogida()
{

  if (ingrediente != NULL){
    mostrador_vacio.wait();
  }
}

void funcion_hebra_fumador(MRef<Estanco> monitor, int i)
{
  while(true)
  {
    monitor->obtenerIngrediente(i);
    fumar(i);
  }
}

void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
   while(true)
   {
     int ingred = generar_ingrediente();
     monitor->colocar_ingrediente(ingred);
     monitor->esperar_recogida();
   }
}

//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los fumadores." << endl
        << "--------------------------------------------------------" << endl
        << flush ;
   MRef<Estanco> monitor = Create<Estanco>();
   thread hebra_estanquero( funcion_hebra_estanquero,monitor );
   thread hebra_fumadores[MAX];
      
      for(int i=0;i<MAX;i++){
          hebra_fumadores[i] = thread(funcion_hebra_fumador,monitor,i);
      }
      for(int i=0 ;i<MAX;i++){
        hebra_fumadores[i].join();
      }
  
   hebra_estanquero.join();

}


