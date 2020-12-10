/**
 * 
 * Programa que reduce imagenes a formato 480p, de forma paralelizada con POSIX.
 *  
 * Hecho por los estudiantes Juan Guillermo Sierra Agreda y Daniel Angulo Suescun del
 * proyecto curricular Ingeniería de Sistemas de la Universidad Nacional de Colombia, como parte
 * de la practica #1 de la materia Computación Paralela y Distribuida dictada por el profesor
 * Cesar Augusto Pedraza
 * 
 * comando para compilar: g++ reduction.cpp -o testoutput -fopenmp -lm -std=c++11 `pkg-config --cflags --libs opencv`
 * 
 * @author Juan Guillermo Sierra & Daniel Angulo Suescun  
 * */

#include <opencv2/highgui.hpp>
#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include <omp.h>

using namespace cv;
using namespace std;



/**
 * @var coeficientWIDTH coeficiente de la operación widthOriginal/widthReducido
 * */
double coeficientWIDTH;

/**
 * @var coeficientHEIGHT coeficiente de la operación heigthOriginal/heightReducido
 * */
double coeficientHEIGHT;

/**
 * @var WIDTH ancho de la imagen original
 * */
int WIDTH;

/**
 * @var HEIGHT ancho de la imagen original
 * */
int HEIGHT;

/**
 * @const LENGTH cantidad de pixeles de la imagen reducida (852*480)
 * */
const int LENGTH = 408960;

/**
 * @var THREADS número de hilos a utilizar
 * */
int THREADS;

/**
 * Calcula el inicio y final de los calculos a realizar por el hilo, para después definir que pixel de la imagen original
 * corresponde a la reducida.
 * 
 * Los datos de la imagen original y final se aplanaron a 1 dimensión
 * 
 * Para calcular la posición i y j del pixel en la imagen reducida definimos a j
 * como el modulo del número de columnas (852) e i a partir de j, y index es el 
 * valor funcion de i y j, que nos indica la posición del pixel en el arreglo de 
 * 1 dimensión o vector.
 * 
 * 
 * X y Y es la posición del pixel en la imagen original que vamos a tomar, y 
 * correspondiente indexAux es la posición del pixel en el arreglo de 1 dimensión 
 * o vector.
 * 
 * flatResample es el arreglo de la imagen reducida
 * flat es el arreglo de la imagen original
 * @param arg apuntador del hilo (thread) a utlizar.
 * */
// void resample(int ID)
// {
//   int initIteration, endIteration, threadId = ID;
//   initIteration = (LENGTH / THREADS) * threadId;

//   if (threadId == THREADS - 1)
//     endIteration = LENGTH;
//   else
//     endIteration = initIteration + ((LENGTH / THREADS) - 1);

//   int index = 0;

//   for (int aux = initIteration; aux < endIteration; aux++)
//   {
    
//     int j = aux % 852;
//     int i = (aux - j) / 852;
//     index = (j + i * 852) * 3;
//     int x = j * coeficientWIDTH;
//     int y = i * coeficientHEIGHT;

//     int indexAux = (x + y * WIDTH) * 3;
    
//     flatResample.data[index] = flat.data[indexAux];
//     flatResample.data[index + 1] = flat.data[indexAux + 1];
//     flatResample.data[index + 2] = flat.data[indexAux + 2];
//   }
// }

int main(int argc, char *argv[])
{

  /**
 * @var flatResample variable que contendra los valores de la nueva imagen reducida 
 *  
 * */
cv::Mat flatResample;

/**
 * @var flat variable que contendra los valores de la imagen a reducir
 * */
cv::Mat flat;
  // define la cantidad de threads a utilizar dada la entrada del usuario
  THREADS = atoi(argv[3]);
  // define el "canvas" de la imagen reducida
  cv::Mat resampleImage(480, 852, CV_8UC3, Scalar(0, 0, 0));

  // variables del tiempo de ejecucion
  struct timeval tval_before, tval_after, tval_result;

  // arreglo de Id de los threads
  //int threadId[THREADS], i, *retval;

  // crea los hilos
  pthread_t thread[THREADS];

  // lectura imagen original
  cv::Mat image;
  image = cv::imread(argv[1], cv::IMREAD_UNCHANGED);

  // ancho y alto de la imagen original
  WIDTH = image.size().width;
  HEIGHT = image.size().height;

  // numero de pixeles de la imagen original.
  uint totalElements = image.total() * image.channels(); // Note: image.total() == rows*cols.

  // reduce la matriz de datos originales a un arreglo de 1 dimension
  flat = image.reshape(1, totalElements);

  if (!image.isContinuous())
  {
    flat = flat.clone(); // O(N),
  }

  // convertimos el arreglo en un vector, guardado en la variable vec
  std::vector<uchar> vec(flat.data, flat.data + flat.total());

  // realizamos la misma operación ahora para los valores de la imagen a reducir

  uint totalElementsResampleImage = resampleImage.total() * resampleImage.channels();
  flatResample = resampleImage.reshape(1, totalElementsResampleImage);

  if (!resampleImage.isContinuous())
  {
    flatResample = flatResample.clone(); // O(N),
  }
  // flat.data es el puntero del arreglo, guardado ahora en ptr
  auto *ptrResample = flatResample.data;

  unsigned char newImage[408960];
  auto *ptrNewImage = newImage;
  // (el puntero ahora apunta al inicio del vector)
  std::vector<uchar> vecResample(flatResample.data, flatResample.data + flatResample.total());

  // se calculan los coeficientes
  coeficientWIDTH = WIDTH / 852.0;
  coeficientHEIGHT = HEIGHT / 480.0;

  // tiempo del inicio de ejecucion
  gettimeofday(&tval_before, NULL);

  // se genera el flujo de trabajo a los hilos con funcion pragma de OPENMP
  int processId;

  //#pragma omp parallel num_threads(1)
  //  {
  //      int ID = omp_get_thread_num();// id del thread a utilizar
  //      resample(ID);
  //  }

  MPI_Init(NULL, NULL);      // initialize MPI environment
  int world_size; // number of processes
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  int world_rank; // the rank of the process
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  char processor_name[MPI_MAX_PROCESSOR_NAME]; // gets the name of the processor
  int name_len;
  MPI_Get_processor_name(processor_name, &name_len);

  printf("Hello world from processor %s, rank %d out of %d processors\n",
                processor_name, world_rank, world_size);
  int l = 408960;
  int initIteration, endIteration, threadId = world_rank;
  initIteration = (l / world_size) * threadId;

  if (threadId == world_size - 1)
    endIteration = l;
  else
    endIteration = initIteration + ((l / world_size) - 1);

  int index = 0;

  unsigned char data[endIteration-initIteration];

  for (int aux = initIteration; aux < endIteration; aux++)
  {
    
    int j = aux % 852;
    int i = (aux - j) / 852;
    // index = (j + i * 852) * 3;
    index+=3;
    int x = j * coeficientWIDTH;
    int y = i * coeficientHEIGHT;

    int indexAux = (x + y * WIDTH) * 3;
    
    data[index] = flat.data[indexAux];
    data[index + 1] = flat.data[indexAux + 1];
    data[index + 2] = flat.data[indexAux + 2];
  }
  MPI_Gather(&data, 2, MPI_UNSIGNED_CHAR, newImage, 3, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
  MPI_Finalize(); // finish MPI environment
  // tiempo del final de ejecución
  gettimeofday(&tval_after, NULL);
  // se genera la imagen reducida, a partir del puntero del vector con los datos (ptrResample)
  resampleImage = cv::Mat(480, 852, resampleImage.type(), ptrNewImage);

  // se muestra en pantalla el resultado final
  // cv::namedWindow(argv[2], cv::WINDOW_AUTOSIZE);
  // cv::imshow(argv[2], resampleImage);

  // calculo del tiempo tomado
  timersub(&tval_after, &tval_before, &tval_result);
  printf("Time elapsed: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
  // guardado de la imagen resultado
  imwrite(argv[2], resampleImage); 
  cv::waitKey(0);
  return 0;
}
