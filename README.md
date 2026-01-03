# Índice Invertido con MPI

## Descripción

Este proyecto implementa un **índice invertido distribuido** utilizando **MPI (Message Passing Interface)** para computación paralela. El programa permite procesar múltiples documentos de forma distribuida entre varios procesos, construyendo un índice que facilita búsquedas rápidas de palabras en documentos.

## ¿Qué es un índice invertido?

Un índice invertido es una estructura de datos que mapea cada palabra a los documentos que la contienen. Es muy usado en buscadores web y sistemas de recuperación de información.

**Ejemplo:**
`
Palabra "python"  [doc1.txt, doc3.txt, doc5.txt]
Palabra "programación"  [doc2.txt, doc3.txt]
`

## Características

-  **Procesamiento paralelo**: Distribuye documentos entre múltiples procesos MPI
-  **Búsquedas rápidas**: Consultas eficientes sobre el índice invertido
-  **Escalable**: Funciona con cualquier número de procesos
-  **Comunicación MPI**: Utiliza paso de mensajes para sincronización entre procesos

## Cómo funciona

### Fase 1: Distribución de datos
- El proceso 0 lee los documentos desde input.txt
- Distribuye equitativamente los documentos entre todos los procesos

### Fase 2: Construcción del índice
- Cada proceso construye su índice local a partir de sus documentos
- Mapea cada palabra a los documentos que la contienen

### Fase 3: Consultas
- El proceso 0 recibe las consultas
- Envía cada consulta a todos los procesos
- Recopila resultados y muestra los documentos que contienen la palabra

## Requisitos

- C++ 11 o superior
- MPI (OpenMPI o MPICH)
- Compilador compatible (gcc, clang, etc.)

## Compilación

`ash
mpic++ -o trabajo Trabajo.cpp
`

## Ejecución

`ash
mpirun -np 4 ./trabajo
`

Donde -np 4 indica 4 procesos paralelos (ajustable según tus necesidades)

## Formato del archivo de entrada

El archivo input.txt debe tener el siguiente formato:

`
<cantidad_documentos>
<nombre_doc1> <cantidad_palabras> <palabra1> <palabra2> ...
<nombre_doc2> <cantidad_palabras> <palabra1> <palabra2> ...
...
<cantidad_consultas>
<palabra_buscar1>
<palabra_buscar2>
...
`

### Ejemplo:

`
3
articulo1.html 4 python programación desarrollo software
articulo2.html 3 java programación backend
articulo3.html 5 python web framework django fast
2
python
programación
`

## Salida esperada

`
Resultados para "python": articulo1.html, articulo3.html
Resultados para "programación": articulo1.html, articulo2.html
`

## Autores

- Javier Catalán
- Martín Ferrada

## Fecha

22/06/2025

---

**Nota**: Este proyecto es un ejemplo educativo de computación paralela con MPI.
