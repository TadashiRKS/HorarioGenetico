#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cmath>

#define Tcasamiento 0.5
#define Pcasamiento 0.5
#define Tmutacion 0.2
#define IND 20
#define MAX_ITERACIONES 1
#define DIAS 30
#define BITS_POR_TURNO 2

using namespace std;

// Estructuras para almacenar las preferencias de las enfermeras
struct Preferencia{
    int dia;
    int turno;
};

struct Enfermera{
    int cod;
    char *nombre;
    Preferencia arrPref[3];
};

// Vector para almacenar las preferencias de todas las enfermeras
vector<Enfermera> enfermeras;

// Tipos de turnos
enum Turno {LIBRE = 0, T86 = 1, T87 = 2};

// Función para convertir el texto del turno al valor enumerado
int texto_a_turno(char *texto) {
    if (strcmp(texto,"L") == 0) return LIBRE;
    else if (strcmp(texto,"T86") == 0) return T86;
    else if (strcmp(texto,"T87") == 0) return T87;
    return -1; // Error
}

char *leerCadena(ifstream &arch, int num, char car){
    char *aux, nombre[num];
    arch.getline(nombre, num, car);
    aux = new char [strlen(nombre)+1];
    strcpy(aux, nombre);
    return aux;
}

void leerPreferencias(ifstream &arch, struct Enfermera &auxEnf){
    int dia, numTurno;
    char *turno;
    for(int i=0; i<3; i++){
        arch >> dia;
        arch.get();
        if(i == 2)
            turno = leerCadena(arch, 5, '\n');
        else
            turno = leerCadena(arch, 5, ',');
        numTurno = texto_a_turno(turno);
        auxEnf.arrPref[i].dia = dia;
        auxEnf.arrPref[i].turno = numTurno;
        delete turno;
    }
}

// Función para cargar las preferencias desde un archivo CSV
void cargar_preferencias(){
    ifstream arch;
    arch.open("enfermeras.csv", ios::in);
    if(not arch){
        cout << "ERROR al abrir el archivo enfermeras.csv" << endl;
        exit(1);
    }
    char* nombre;
    int cod, dia;
    while(true){
        Enfermera auxEnf;
        arch >> cod;
        if(arch.eof()) break;
        arch.get();
        nombre = leerCadena(arch, 100, ',');
        auxEnf.cod = cod;
        auxEnf.nombre = nombre;
        leerPreferencias(arch, auxEnf);
        enfermeras.push_back(auxEnf);
    }
}

// Función para convertir una combinación de bits a un turno
int bits_a_turno(int bit1, int bit2) {
    if (bit1 == 0 && bit2 == 0) return LIBRE;
    if (bit1 == 0 && bit2 == 1) return T86;
    if (bit1 == 1 && bit2 == 0) return T86;
    if (bit1 == 1 && bit2 == 1) return T87;
    return -1; // Error
}

// Función para calcular el fitness de un cromosoma
double calcular_fitness(const vector<int>& cromosoma) {
    int total_preferencias_cumplidas = 0;
    int total_preferencias = 0;
    
    for (int i=0; i<enfermeras.size(); i++) {
        int enfermera_offset = i * DIAS * BITS_POR_TURNO;
        int preferencias_cumplidas = 0;
        
        for (int j=0; j<3; j++) {
            int dia = (enfermeras[i].arrPref[j].dia - 1) * BITS_POR_TURNO;
            int turno_preferido = enfermeras[i].arrPref[j].turno;
            int turno_asignado = bits_a_turno(cromosoma[enfermera_offset + dia], 
                    cromosoma[enfermera_offset + dia + 1]);
            if (turno_asignado == turno_preferido) {
                preferencias_cumplidas++;
            }
        }
        
        total_preferencias_cumplidas += preferencias_cumplidas;
        total_preferencias += 3;
    }
    
    return (double)(total_preferencias_cumplidas) / total_preferencias;
}

// Función para verificar si un cromosoma es una aberración
bool es_aberracion(const vector<int>& cromosoma) {
    for (int e = 0; e < enfermeras.size(); ++e) {
        int libre_count = 0, t86_count = 0, t87_count = 0;
        for (int d = 0; d < DIAS; ++d) {
            int bit1 = cromosoma[e * DIAS * BITS_POR_TURNO + d * BITS_POR_TURNO];
            int bit2 = cromosoma[e * DIAS * BITS_POR_TURNO + d * BITS_POR_TURNO + 1];
            int turno = bits_a_turno(bit1, bit2);
            if (turno == T86) t86_count++;
            else if (turno == T87) t87_count++;
            else if (turno == LIBRE) libre_count++;
        }
        if (t87_count > 6) return true;
//        if ((double)((t87_count + t86_count)/DIAS) < 0) return true;
        if (((double)(libre_count)/DIAS) > 0.4) return true;
//        if (t87_count*2 > libre_count) return true;
    }
    return false;
}

// Función para mostrar la población
void mostrar_poblacion(vector<vector<int>>& poblacion) {
    for(int i=0; i<poblacion.size(); i++) {
        cout << "Individuo " << i+1 << ": ";
        cout << "fo=" << calcular_fitness(poblacion[i]) << endl;
        for (int j=0; j<poblacion[i].size(); j++) {
            cout << poblacion[i][j] << " ";
            if((j+1)%(BITS_POR_TURNO*DIAS)==0) cout << endl;
        }
    }
}

// Función para generar la población inicial
void generar_poblacion_inicial(vector<vector<int>>& poblacion, int tam_cromosoma) {
    int cont = 0;
    srand(time(NULL));
    while (cont < IND) {
        vector<int> cromosoma;
        for (int j=0;j<tam_cromosoma;j++) {
            cromosoma.push_back(rand()%2); // Generar bits aleatorios entre 0 y 1
        }
        if (!es_aberracion(cromosoma)) {
            poblacion.push_back(cromosoma);
            cont++;
        }
    }
}

// Función para calcular la supervivencia de la población
void calcular_supervivencia(const vector<vector<int>>& poblacion, vector<double>& supervivencia) {
    double suma_fitness = 0;
    for (int i=0; i<poblacion.size(); i++) {
        suma_fitness += calcular_fitness(poblacion[i]);
    }
    for (int i=0; i<poblacion.size(); i++) {
        double fitness = calcular_fitness(poblacion[i]);
        supervivencia.push_back((fitness / suma_fitness) * 100);
    }
}

// Función para cargar la ruleta de selección
void cargar_ruleta(const vector<double>& supervivencia, int *ruleta) {
    int ind = 0;
    for (int i=0; i<supervivencia.size(); i++) {
        for (int j=0; j<(int)supervivencia[i]; j++) {
            ruleta[ind] = i;
            ind++;
        }
    }
}

// Función de selección por ruleta
void seleccion(const vector<vector<int>>& poblacion, vector<vector<int>>& padres) {
    vector<double> supervivencia;
    int ruleta[100]{-1};;
    calcular_supervivencia(poblacion, supervivencia);
    cargar_ruleta(supervivencia, ruleta);
    int npadres = poblacion.size() * Tcasamiento;
    while (padres.size() < npadres) {
        int ind = rand() % 100;
        if (ruleta[ind] != -1) {
            padres.push_back(poblacion[ruleta[ind]]);
        }
    }
}

// Función para generar un hijo a partir de dos padres
void generar_hijo(const vector<int>& padre, const vector<int>& madre, vector<int>& hijo) {
    int pos=round(padre.size()*Pcasamiento);
    for(int i=0;i<pos;i++)
        hijo.push_back(padre[i]);
    for(int i=pos;i<padre.size();i++)
        hijo.push_back(madre[i]);
}

// Función de cruce
void cruce(vector<vector<int>>& poblacion, const vector<vector<int>>& padres) {
    for (int i = 0; i < padres.size(); ++i) {
        for (int j = 0; j < padres.size(); ++j) {
            if (i != j) {
                vector<int> hijo;
                generar_hijo(padres[i], padres[j], hijo);
                if (!es_aberracion(hijo)) {
                    poblacion.push_back(hijo);
                }
            }
        }
    }
}

// Función de mutación
void mutacion(vector<vector<int>>& poblacion, vector<vector<int>> padres) {
    int nmuta=round(padres[0].size()*Tmutacion);
    for (int i=0;i<padres.size();i++){
        for (int j=0; j<nmuta; j++) {
            int ind=rand()%padres[0].size();
            if(padres[i][ind]==0){
                padres[i][ind]=1;
            }
            else
                padres[i][ind]=0;
        }
        if (!es_aberracion(padres[i])) {
            poblacion.push_back(padres[i]);
        }
    }
}

// Función de inversión
void inversion(vector<vector<int>>& poblacion, vector<vector<int>> padres) {
    for (int i=0;i<padres.size();i++) {
        for (int j=0;j<padres[i].size();j++) {
           if(padres[i][j]==0)
                padres[i][j]=1;
            else
                padres[i][j]=0;
        }
        if (!es_aberracion(padres[i])) {
            poblacion.push_back(padres[i]);
        }
    }
}

// Función para eliminar aberraciones
void eliminar_aberraciones(vector<vector<int>>& poblacion) {
    poblacion.erase(remove_if(poblacion.begin(), poblacion.end(), es_aberracion), poblacion.end());
    sort(poblacion.begin(), poblacion.end());
    poblacion.erase(unique(poblacion.begin(), poblacion.end()), poblacion.end());
}

// Función para mostrar el mejor cromosoma
void guerdar_mejor(const vector<vector<int>>& poblacion, vector<int> &mejorCromo,
        double &mejorFitness) {
    int mejor=0;
    for(int i=0;i<poblacion.size();i++)
        if(calcular_fitness(poblacion[mejor])<calcular_fitness(poblacion[i]))
            mejor=i;
    
    if(mejorFitness < calcular_fitness(poblacion[mejor])){
        mejorCromo = poblacion[mejor];
        mejorFitness = calcular_fitness(poblacion[mejor]);
    }
}

void mostrar_mejor(vector<int> mejor_cromo, double mejor_fitness){
    cout << endl<<"La mejor solucion es:" << mejor_fitness <<endl;
    for(int i=0; i<enfermeras.size(); i++){
        cout << left << setw(15) << enfermeras[i].nombre << ": ";
        for(int j=0; j<BITS_POR_TURNO*DIAS; j+=2){
            int bit1 = mejor_cromo[i*BITS_POR_TURNO*DIAS+j];
            int bit2 = mejor_cromo[i*BITS_POR_TURNO*DIAS+j+1];
            int tipo = bits_a_turno(bit1, bit2);
            if(tipo == LIBRE) cout << left << setw(5) <<"L";
            else if(tipo == T86) cout << left << setw(5) << "T86";
            else if(tipo == T87) cout << left << setw(5) << "T87";
        }
        cout << endl;
    }
}

// Función principal del algoritmo genético
void planificacion_ag(int tam_cromosoma) {
    vector<int> mejor_cromo;
    double mejor_fitness = 0;
//    vector<vector<int>> poblacion;
//    generar_poblacion_inicial(poblacion, tam_cromosoma);
//    cout << "POBLACION INICIAL: " << endl;
//    mostrar_poblacion(poblacion);
//    cout << endl;
    
    int iteracion = 0;
    while (iteracion < MAX_ITERACIONES) {
        vector<vector<int>> poblacion;
        generar_poblacion_inicial(poblacion, tam_cromosoma);
        vector<vector<int>> padres;
        seleccion(poblacion, padres);
//        cout << "PADRES: " << endl;
//        mostrar_poblacion(padres);
//        cout << endl;
        cruce(poblacion, padres);
//        cout << "CASAMIENTO: " << endl;
//        mostrar_poblacion(poblacion);
//        cout << endl;
        mutacion(poblacion, padres);
//        cout << "MUTACION: " << endl;
//        mostrar_poblacion(poblacion);
//        cout << endl;
        inversion(poblacion, padres);
//        cout << "INVERSION: " << endl;
//        mostrar_poblacion(poblacion);
//        cout << endl;
        eliminar_aberraciones(poblacion);
//        cout << "POBLACION FINAL: " << endl;
//        mostrar_poblacion(poblacion);
//        cout << endl;
        guerdar_mejor(poblacion, mejor_cromo, mejor_fitness);
        iteracion++;
    }
    mostrar_mejor(mejor_cromo, mejor_fitness);
}

int main() {
    cargar_preferencias();
    int tam_cromosoma = DIAS * enfermeras.size() * BITS_POR_TURNO;
    planificacion_ag(tam_cromosoma);
    return 0;
}
