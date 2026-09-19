#include <iostream>
#include <cstring>
#include <omp.h>

class BuscadorClaves {
private:
    //ATRIBUTOS PRIVADOS Y GESTIÓN DE ESPACIO DE BÚSQUEDA
    char* alfabeto; // Puntero para arreglo dinámico del alfabeto
    int tam_alfabeto; //definir el tmaño del conjunto
    int longitud_clave; //largo de la clave a buscar
    unsigned long long total_combinaciones; // espacio de busqueda total es de 64 bits

public:
    // CONSTRUCTOR Y DESTRUCTOR (Reserva y Liberación de Memoria Dinámica)
    BuscadorClaves(int longitud) {
        longitud_clave = longitud;
        tam_alfabeto = 36; //asigna tamaño de la memoria

        alfabeto = new char[tam_alfabeto]; //Asigna espacio dinámico en la memoria RAM para 36 elementos de tipo char

        int indice = 0;
        //El bucle incrementa el valor numérico ASCII del carácter en cada iteración para recorrer y
        // guardar ordenadamente las letras de la A a la Z
        for (char c = 'A'; c <= 'Z'; c++) {
            alfabeto[indice++] = c;
        }
        //0 al 9 para almacenarlos en las posiciones del arreglo justo después de las letras
        for (char c = '0'; c <= '9'; c++) {
            alfabeto[indice++] = c;
        }

        calcularEspacio();
    }

    // destructor que libera la memoria RAM asignada al alfabeto para evitar fugas de memoria (memory leaks)
    ~BuscadorClaves() {
        delete[] alfabeto;
    }

    
    // MÉTODOS AUXILIARES: CÁLCULO MATEMÁTICO Y CONVERSIÓN BASE 36
    
    // calcula el número total de combinaciones posibles resolviendo la potencia 36^longitud_clave
    void calcularEspacio() {
        total_combinaciones = 1;
        for (int i = 0; i < longitud_clave; i++) {
            total_combinaciones *= tam_alfabeto;
        }
    }

    //convierte un índice numérico entero a su representación alfanumerica en base 36
    void generarCombinacion(unsigned long long posicion, char* combinacion_salida) const {
        for (int i = longitud_clave - 1; i >= 0; i--) {
            combinacion_salida[i] = alfabeto[posicion % tam_alfabeto];
            posicion /= tam_alfabeto;
        }
        combinacion_salida[longitud_clave] = '\0'; // terminación de cadena 
    }

    //BÚSQUEDA SECUENCIAL (Evaluación en 1 Solo Núcleo)
    
    double busquedaSecuencial(const char* clave_prueba) {
        std::cout << "\n INICIANDO BUSQUEDA SECUENCIAL" << std::endl;
        std::cout << "Clave a buscar: " << clave_prueba << std::endl;
        // arreglo dinámico temporal para la combinación evaluada (+1 para el fin de cadena '\0')
        char* combinacion_actual = new char[longitud_clave + 1];
        bool encontrada = false; //estado
        unsigned long long combinaciones_revisadas = 0; //contador de combinaciones revisadas

        // registra la estampa de tiempo inicial con alta precisión usando OpenMP
        double inicio_tiempo = omp_get_wtime();

        // recorre el espacio de búsqueda evaluación tras evaluación
        for (unsigned long long i = 0; i < total_combinaciones; i++) {
            generarCombinacion(i, combinacion_actual);
            combinaciones_revisadas++;
            // Compara la combinación generada con la clave buscada
            if (strcmp(combinacion_actual, clave_prueba) == 0) {
                encontrada = true;
                break;
            }
        }
        // Calcula la duración total de la búsqueda restando las estampas de tiempo
        double fin_tiempo = omp_get_wtime();
        double tiempo_ejecucion = fin_tiempo - inicio_tiempo;

        if (encontrada) {
            std::cout << "Estado: Clave encontrada exitosamente" << std::endl;
            std::cout << "Clave encontrada: " << combinacion_actual << std::endl;
        }
        else {
            std::cout << "Estado: Clave no encontrada." << std::endl;
        }

        std::cout << "Combinaciones revisadas: " << combinaciones_revisadas << std::endl;
        std::cout << "Tiempo de ejecucion secuencial: " << tiempo_ejecucion << " segundos." << std::endl;

        delete[] combinacion_actual; // Libera el arreglo dinámico temporal
        return tiempo_ejecucion;
    }

    // BÚSQUEDA PARALELA (Distribución de rangos y exclusión mutua OpenMP)
    
    double busquedaParalela(const char* clave_prueba) {
        std::cout << "\n INICIANDO BUSQUEDA PARALELA " << std::endl;
        std::cout << "Clave a buscar: " << clave_prueba << std::endl;
        //Variables compartidas entre hilos para coordinar
        bool encontrada_flag = false;
        int hilo_ganador = -1;
        char* clave_encontrada = new char[longitud_clave + 1];
        clave_encontrada[0] = '\0';
        //tiempo inicial antes de lanzar los hilos
        double inicio_tiempo = omp_get_wtime();
        //Inicia el bloque paralelo y comparte variables entre todos los hilos
#pragma omp parallel shared(encontrada_flag, hilo_ganador, clave_encontrada)
        {
            int id = omp_get_thread_num(); // identificador único del hilo
            int total_hilos = omp_get_num_threads(); // cantidad total de hilos en ejecución

            // distribución equitativa del espacio de búsqueda
            // Calcula la carga base e identifica sobrantes
            unsigned long long bloque_base = total_combinaciones / total_hilos;
            unsigned long long residuo = total_combinaciones % total_hilos;
            unsigned long long inicio, fin;
            // Distribuye los rangos de manera equitativa incluyendo el residuo
            if (id < residuo) {
                inicio = id * (bloque_base + 1);
                fin = inicio + bloque_base;
            }
            else {
                inicio = id * bloque_base + residuo;
                fin = inicio + bloque_base - 1;
            }
            // Calcula la cantidad de combinaciones asignadas a este hilo
            unsigned long long cantidad = (fin >= inicio) ? (fin - inicio + 1) : 0;

            // Asignación de arreglos locales privados por hilo, evita condiciones de carrera
            char* combinacion_hilo = new char[longitud_clave + 1];
            char* txt_inicio = new char[longitud_clave + 1];
            char* txt_fin = new char[longitud_clave + 1];
            // Obtiene la primera y última combinación en texto del rango asignado
            generarCombinacion(inicio, txt_inicio);
            generarCombinacion(fin, txt_fin);

#pragma omp critical //Bloquea el acceso para que solo un hilo imprima en consola a la vez
            {
                std::cout << "[Hilo " << id << "/" << total_hilos
                    << "] Inicio: " << txt_inicio
                    << " -> Fin: " << txt_fin
                    << " | Cantidad: " << cantidad << std::endl;
            }

            // Búsqueda y mecanismo de sincronización
            bool local_exito = false;
            unsigned long long revisadas_hilo = 0;

            for (unsigned long long i = inicio; i <= fin; i++) {

                if (encontrada_flag) break; //Evalúa la variable compartida en cada iteración; si otro hilo encuentra la clave primero
                //los demás hilos abortan su trabajo inmediatamente ahorrando tiempo.

                generarCombinacion(i, combinacion_hilo);
                revisadas_hilo++;

                if (strcmp(combinacion_hilo, clave_prueba) == 0) {
#pragma omp critical
                    {
                        if (!encontrada_flag) {
                            encontrada_flag = true;
                            hilo_ganador = id;
                            // Copia de la clave hallada al arreglo dinámico compartido
                            for (int k = 0; k <= longitud_clave; k++) {
                                clave_encontrada[k] = combinacion_hilo[k];
                            }
                        }
                    }
                    local_exito = true;
                    break;
                }
            }

            // Reporte de finalización por hilo
#pragma omp critical
            {
                // Libera los arreglos dinámicos locales creados por este hilo en particular
                std::cout << "[Hilo " << id << "] Finalizo. Estado: "
                    << (local_exito ? "GANADOR" : (encontrada_flag ? "CANCELADO (Clave encontrada)" : "AGOTADO"))
                    << " | Revisadas: " << revisadas_hilo << std::endl;
            }

            delete[] combinacion_hilo;
            delete[] txt_inicio;
            delete[] txt_fin;
        }
        // Calcula el tiempo total transcurrido en la ejecución paralela
        double fin_tiempo = omp_get_wtime();
        double tiempo_ejecucion = fin_tiempo - inicio_tiempo;

        std::cout << "\n RESULTADO BUSQUEDA PARALELA" << std::endl;
        if (encontrada_flag) {
            std::cout << "Clave encontrada por el hilo: " << hilo_ganador << std::endl;
            std::cout << "Clave: " << clave_encontrada << std::endl;
        }
        else {
            std::cout << "Clave no encontrada." << std::endl;
        }
        std::cout << "Tiempo de ejecucion paralelo: " << tiempo_ejecucion << " segundos." << std::endl;

        delete[] clave_encontrada; // Libera la memoria compartida del resultado
        // Devuelve el tiempo medido para calcular el Speedup
        return tiempo_ejecucion;
    }

    
    //MÉTODOS DE ACCESO (Getters)
    
    unsigned long long getTotalCombinaciones() const { return total_combinaciones; }
    int getLongitudClave() const { return longitud_clave; }
};

// PRUEBAS DE EXPERIMENTACIÓN Y SPEEDUP
int main() {
    std::cout << " Equipo 01: Guzman Lopez Paola Nicole, Mondragon Gambino Juan Sinuhe y Morales Limon Jennifer Nataly" << std::endl;
    std::cout << "  PRIMERA EJECUCION: CLAVE DE 3 CARACTERES        " << std::endl;
    std::cout << "==================================================" << std::endl;

    BuscadorClaves buscador3(3); // Instancia el buscador para claves de 3 caracteres (36^3 = 46,656 combinaciones)
    const char* clave3 = "Z99"; // Clave cerca del final para probar rango completo

    // Ejecución y toma de tiempos para 3 caracteres
    double t_seq3 = buscador3.busquedaSecuencial(clave3);
    double t_par3 = buscador3.busquedaParalela(clave3);

    // Muestra la ganancia de velocidad (Speedup = T_seq / T_par)
    std::cout << "\n[COMPARACION 3 CARACTERES]" << std::endl;
    std::cout << "Speedup: " << (t_seq3 / t_par3) << "x" << std::endl;

    std::cout << "  SEGUNDA EJECUCION: CLAVE DE 6 CARACTERES       " << std::endl;
    std::cout << "==================================================" << std::endl;
    // Instancia el buscador para 6 caracteres (36^6 = 2,176,782,336 combinaciones)
    BuscadorClaves buscador6(6);
    const char* clave6 = "999999"; // Clave al final del espacio de 2,176 millones

    // Ejecución y toma de tiempos para 6 caracteres
    double t_seq6 = buscador6.busquedaSecuencial(clave6);
    double t_par6 = buscador6.busquedaParalela(clave6);

    // Reporte final de métricas de desempeño y Speedup
    std::cout << "\n[COMPARACION 6 CARACTERES]" << std::endl;
    std::cout << "Tiempo Secuencial: " << t_seq6 << " s" << std::endl;
    std::cout << "Tiempo Paralelo:   " << t_par6 << " s" << std::endl;
    std::cout << "Speedup: " << (t_seq6 / t_par6) << "x" << std::endl;

    return 0;
}