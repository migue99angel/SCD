/*
  Sistemas Concurrentes y Distribuidos
  Práctica 2: Problema de la barberia con Monitores de Hoare y señales SU
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


const int MAX = 5;
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

// Función que simula que el cliente está waiting fuera de la barbería 
void EsperarFueraBarberia(int cliente)
{
  chrono::milliseconds duracion_espera( aleatorio<20,200>() );

  mtx.lock();
  cout << "Cliente " << cliente << " espera durante ("
       << duracion_espera.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_espera);
}

//Función que simula la acción de cortarle el pelo a un cliente 

void cortarPeloACliente()
{
  chrono::milliseconds duracion_corte( aleatorio<50,500>() );

  mtx.lock();
  cout << "                                     Barbero pela al cliente: ("
       << duracion_corte.count() << " milisegundos)..." << endl;
  mtx.unlock();

  this_thread::sleep_for(duracion_corte);

  mtx.lock();
  cout << "                                     Barbero termina de pelar al cliente " << endl;
  mtx.unlock();
}

//Clase del monitor 

class Barberia : public HoareMonitor {
  private:
    int cliente;
    //variables condicion
    CondVar silla_libre, waiting, pelando;

  public:
    Barberia();
    void cortarPelo(int cliente);
    void siguienteCliente();
    void finCliente();
};

Barberia::Barberia()
{
  silla_libre = newCondVar();
  waiting = newCondVar();
  pelando = newCondVar();
}

void Barberia::cortarPelo(int i)
{

  mtx.lock();
  cout << "Cliente " << i << " entra en la barbería" << endl;
  mtx.unlock();

  
  if(silla_libre.get_nwt() != 0){
    silla_libre.signal();
  }

  else{
    waiting.wait();
  }

  mtx.lock();
  cout << "Cliente " << i << " se está pelando" << endl;
  mtx.unlock();

  
  pelando.wait();
}


void Barberia::siguienteCliente()
{

  if(waiting.get_nwt() == 0){
    silla_libre.wait();
  }


  else{
    waiting.signal();
  }
}

void Barberia::finCliente()
{
  pelando.signal();
}


void funcion_hebra_cliente(MRef<Barberia> monitor, int i)
{
  while(true)
  {
    monitor->cortarPelo(i);
    EsperarFueraBarberia(i);
  }
}

void funcion_hebra_barbero(MRef<Barberia> monitor)
{
   while(true)
   {
     monitor->siguienteCliente();
     cortarPeloACliente();
     monitor->finCliente();
   }
}

int main()
{
  cout << endl << "--------------------------------------------------------" << endl
               << "Práctica 2 Problema de la barberia con Monitores SU" << endl
               << "--------------------------------------------------------" << endl
               << flush ;

  MRef<Barberia> monitor = Create<Barberia>();

  thread hebra_barbero(funcion_hebra_barbero, monitor);
  thread hebra_cliente[MAX];

  for(int i=0; i<MAX; i++){
    hebra_cliente[i] = thread(funcion_hebra_cliente, monitor, i);
  }

  for(int i=0; i<MAX; i++){
    hebra_cliente[i].join();
  }

  hebra_barbero.join();
}