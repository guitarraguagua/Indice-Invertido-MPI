/*
 * Trabajo: Indice Invertido con MPI
 * Fecha: 22/06/2025
 * Integrantes: Javier Catalán, Martin Ferrada
 *
 * Este programa hace un índice invertido distribuido usando MPI.
 * Cada proceso ayuda a construir el índice a partir de archivos HTML (simulados).
 * Después, se hacen consultas en paralelo para ver en qué archivos aparece cada palabra.
 */

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>

using namespace std;

// Función para separar palabras de un string
vector<string> partir(const string& texto) {
    vector<string> res;
    stringstream ss(texto);
    string temp;
    while (ss >> temp) {
        res.push_back(temp);
    }
    return res;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int mi_rango, total_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &mi_rango);
    MPI_Comm_size(MPI_COMM_WORLD, &total_procs);

    int cant_docs = 0, cant_consultas = 0;
    vector<string> docs_nombres;
    vector<vector<string>> docs_terminos;
    vector<string> consultas;

    // Solo el proceso 0 lee los datos
    if (mi_rango == 0) {
        ifstream arch("input.txt");
        if (!arch.is_open()) {
            cerr << "No se pudo abrir input.txt" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        arch >> cant_docs;
        docs_nombres.resize(cant_docs);
        docs_terminos.resize(cant_docs);

        // Leer nombres y términos
        for (int i = 0; i < cant_docs; ++i) {
            arch >> docs_nombres[i];
            int cuantos;
            arch >> cuantos;
            docs_terminos[i].resize(cuantos);
            for (int j = 0; j < cuantos; ++j) {
                arch >> docs_terminos[i][j];
            }
        }
        // Leer consultas
        arch >> cant_consultas;
        consultas.resize(cant_consultas);
        for (int i = 0; i < cant_consultas; ++i) {
            arch >> consultas[i];
        }
        arch.close();
    }

    // Compartir la cantidad de documentos
    MPI_Bcast(&cant_docs, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calcular qué docs le tocan a cada uno
    int docs_x_proc = cant_docs / total_procs;
    int resto = cant_docs % total_procs;
    int desde = mi_rango * docs_x_proc + min(mi_rango, resto);
    int hasta = desde + docs_x_proc + (mi_rango < resto ? 1 : 0);

    map<string, set<string>> mi_indice;

    // Cada proceso recibe sus documentos
    vector<string> mis_docs;
    vector<vector<string>> mis_terminos;
    if (mi_rango == 0) {
        for (int p = 1; p < total_procs; ++p) {
            int s = p * docs_x_proc + min(p, resto);
            int e = s + docs_x_proc + (p < resto ? 1 : 0);
            int cuantos = e - s;
            MPI_Send(&cuantos, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
            for (int i = s; i < e; ++i) {
                int largo = docs_nombres[i].size();
                MPI_Send(&largo, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                MPI_Send(docs_nombres[i].c_str(), largo, MPI_CHAR, p, 0, MPI_COMM_WORLD);
                int cuantos_terms = docs_terminos[i].size();
                MPI_Send(&cuantos_terms, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                for (int j = 0; j < cuantos_terms; ++j) {
                    int largo_term = docs_terminos[i][j].size();
                    MPI_Send(&largo_term, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
                    MPI_Send(docs_terminos[i][j].c_str(), largo_term, MPI_CHAR, p, 0, MPI_COMM_WORLD);
                }
            }
        }
        for (int i = desde; i < hasta; ++i) {
            mis_docs.push_back(docs_nombres[i]);
            mis_terminos.push_back(docs_terminos[i]);
        }
    } else {
        int cuantos;
        MPI_Recv(&cuantos, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i < cuantos; ++i) {
            int largo;
            MPI_Recv(&largo, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            string nombre(largo, ' ');
            MPI_Recv(&nombre[0], largo, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            mis_docs.push_back(nombre);

            int cuantos_terms;
            MPI_Recv(&cuantos_terms, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            vector<string> terms;
            for (int j = 0; j < cuantos_terms; ++j) {
                int largo_term;
                MPI_Recv(&largo_term, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                string term(largo_term, ' ');
                MPI_Recv(&term[0], largo_term, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                terms.push_back(term);
            }
            mis_terminos.push_back(terms);
        }
    }

    // Construir el índice invertido local
    for (size_t i = 0; i < mis_docs.size(); ++i) {
        for (const string& t : mis_terminos[i]) {
            mi_indice[t].insert(mis_docs[i]);
        }
    }

    // Consultas
    if (mi_rango == 0) {
        for (int q = 0; q < cant_consultas; ++q) {
            string cons = consultas[q];
            int largo_cons = cons.size();
            for (int p = 1; p < total_procs; ++p) {
                MPI_Send(&largo_cons, 1, MPI_INT, p, 1, MPI_COMM_WORLD);
                MPI_Send(cons.c_str(), largo_cons, MPI_CHAR, p, 1, MPI_COMM_WORLD);
            }
            set<string> res;
            if (mi_indice.count(cons)) {
                res = mi_indice[cons];
            }
            for (int p = 1; p < total_procs; ++p) {
                int encontrados;
                MPI_Recv(&encontrados, 1, MPI_INT, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int i = 0; i < encontrados; ++i) {
                    int largo;
                    MPI_Recv(&largo, 1, MPI_INT, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    string nombre(largo, ' ');
                    MPI_Recv(&nombre[0], largo, MPI_CHAR, p, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    res.insert(nombre);
                }
            }
            // Mostrar resultados
            cout << "Resultados para \"" << cons << "\": ";
            if (res.empty()) {
                cout << "No hay resultados." << endl;
            } else {
                bool primero = true;
                for (const string& doc : res) {
                    if (!primero) cout << ", ";
                    cout << doc;
                    primero = false;
                }
                cout << endl;
            }
        }
        // Avisar fin a los otros procesos
        int fin = 0;
        for (int p = 1; p < total_procs; ++p) {
            MPI_Send(&fin, 1, MPI_INT, p, 3, MPI_COMM_WORLD);
        }
    } else {
        while (true) {
            int largo_cons;
            MPI_Recv(&largo_cons, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (largo_cons == 0) break;
            string cons(largo_cons, ' ');
            MPI_Recv(&cons[0], largo_cons, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            set<string> res;
            if (mi_indice.count(cons)) {
                res = mi_indice[cons];
            }
            int encontrados = res.size();
            MPI_Send(&encontrados, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
            for (const string& doc : res) {
                int largo = doc.size();
                MPI_Send(&largo, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
                MPI_Send(doc.c_str(), largo, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
            }
        }
    }

    MPI_Finalize();
    return 0;
}