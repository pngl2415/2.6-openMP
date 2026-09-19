# Práctica: Algoritmo de búsqueda exhaustiva sobre un espacio de claves de prueba con OpenMP

## Integrantes (Equipo 01)
* **Guzman Lopez Paola Nicole**
* **Mondragon Gambino Juan Sinuhe**
* **Morales Limon Jennifer Nataly**

---

## Descripción de la solución

Este proyecto implementa un algoritmo de búsqueda por fuerza bruta (búsqueda exhaustiva) diseñado para encontrar una clave de prueba dentro de un espacio de búsqueda alfanumérico. La solución está estructurada mediante Programación Orientada a Objetos en C++ dentro de la clase `BuscadorClaves`, integrada por dos enfoques de ejecución:

1. **Búsqueda secuencial (`busquedaSecuencial`):** Recorre de manera lineal el espacio de combinaciones desde la posición $0$ hasta el total calculado, utilizando un único hilo de procesamiento.
2. **Búsqueda paralela (`busquedaParalela`):** Divide de forma equitativa el rango de combinaciones entre los hilos de trabajo configurados con la librería **OpenMP**. Incluye sincronización con secciones críticas y un mecanismo de cancelación temprana para detener los demás hilos en cuanto uno localiza la clave.

> **Nota sobre la gestión de memoria:** Toda la asignación de memoria se realiza mediante arreglos dinámicos en C++ (`new[]` y `delete[]`), tanto para el alfabeto como para las cadenas temporales de cada hilo, evitando el uso de contenedores automáticos como `std::vector` y garantizando la ausencia de fugas de memoria (*memory leaks*).

---

## Espacio de búsqueda y representación en Base 36

### Tamaño del alfabeto
El espacio utiliza un conjunto de $36$ caracteres alfanuméricos ordenados:
* Letras mayúsculas: `A-Z` (26 caracteres)
* Números: `0-9` (10 caracteres)

### Cálculo de combinaciones
Para una clave de longitud $L$, el total de combinaciones posibles se obtiene resolviendo la potencia $36^L$ mediante el método `calcularEspacio()`. El resultado se almacena en un entero de 64 bits sin signo (`unsigned long long`) para evitar desbordamientos numéricos:

* **Longitud 3 (`"Z99"`):** $36^3 = 46,656$ combinaciones.
* **Longitud 6 (`"999999"`):** $36^6 = 2,176,782,336$ combinaciones (más de 2,176 millones).

### Conversión matemática a cadena
En lugar de almacenar las combinaciones en RAM, el método `generarCombinacion()` transforma un índice entero a su representación en texto mediante divisiones y residuos sucesivos entre $36$ (conversión a base 36 en tiempo real).

---

## Estructura de la clase `BuscadorClaves`

| Método / Atributo | Tipo / Retorno | Descripción |
| :--- | :--- | :--- |
| `alfabeto` | `char*` | Puntero al arreglo dinámico que almacena las letras `A-Z` y dígitos `0-9`. |
| `longitud_clave` | `int` | Longitud exacta de la clave a evaluar (configurada en el constructor). |
| `total_combinaciones` | `unsigned long long` | Almacena el número total de estados del espacio de búsqueda ($36^L$). |
| `BuscadorClaves(int longitud)` | Constructor | Reserva memoria para el alfabeto, lo puebla secuencialmente y calcula el espacio. |
| `~BuscadorClaves()` | Destructor | Libera la memoria RAM del alfabeto mediante `delete[]`. |
| `generarCombinacion(...)` | `void` | Mapea una posición numérica entera a su equivalente alfanumérico en base 36. |
| `busquedaSecuencial(...)` | `double` | Realiza la búsqueda iterativa en 1 solo hilo y retorna el tiempo en segundos. |
| `busquedaParalela(...)` | `double` | Divide el rango entre hilos con OpenMP, gestiona la detención anticipada y retorna el tiempo. |

---

## Implementación de paralelismo y sincronización (OpenMP)

### Distribución equitativa de rangos
Para balancear la carga computacional entre todos los hilos, se calcula el bloque base y el residuo del espacio total:

$$\text{bloque\_base} = \lfloor \text{total\_combinaciones} / \text{total\_hilos} \rfloor$$
$$\text{residuo} = \text{total\_combinaciones} \pmod{\text{total\_hilos}}$$

Los primeros hilos asignados (donde $\text{id} < \text{residuo}$) absorben una unidad adicional del residuo para asegurar una repartición justa.

### Control de concurrencia y secciones críticas
* **`#pragma omp parallel shared(...)`**: Crea la región paralela compartiendo variables de control como `encontrada_flag`, `hilo_ganador` y `clave_encontrada`.
* **`#pragma omp critical` (Impresión en consola):** Garantiza que los mensajes de estado y asignación de rangos de cada hilo se desplieguen ordenadamente sin sobreescritura.
* **`#pragma omp critical` (Escritura del resultado):** Protege el registro de la clave hallada y la actualización de `encontrada_flag = true` para evitar condiciones de carrera (*race conditions*).
* **Cancelación temprana:** En cada iteración del bucle, los hilos leen la variable compartida `encontrada_flag`. Si detectan que otro hilo ya encontró la clave, abortan inmediatamente el procesamiento (`break`).

---

## Requisitos e instrucciones de compilación

### Requisitos previos
* Compilador de C++ con soporte para **OpenMP 2.0 o superior** (GCC, Clang o MSVC).

### Compilación en Linux / macOS (GCC)
```bash
g++ -O3 -fopenmp main.cpp -o fuerza_bruta
