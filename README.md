# Proyecto PER – Simulador de Carta Náutica

Aplicación de escritorio en Qt 6 que replica las tareas descritas en el caso práctico de Interfaces Humano-Máquina 2025/2026. Permite preparar exámenes del PER gestionando usuarios, sesiones y resolución guiada de problemas sobre una carta digital del Estrecho de Gibraltar.

## Características

- Registro, autenticación, edición de perfil y seguimiento de sesiones de cada alumno.
- Banco de problemas con generación aleatoria u ordenada, respuestas mezcladas y validación inmediata.
- Carta náutica interactiva (QGraphicsScene) para:
  - Marcar puntos, líneas, arcos y anotaciones de texto.
  - Cambiar color y grosor de las marcas, eliminar marcas y limpiar la carta.
  - Realizar mediciones de distancia con cálculo en millas y pixeles.
  - Mostrar y mover transportador y regla virtuales.
  - Mostrar extremos de puntos sobre los bordes de la carta.
  - Zoom progresivo con rueda o controles dedicados.
- Panel lateral con información de usuario, estado del problema y estadísticas de la sesión.
- Estilo visual claro en tonos azules aplicado mediante `styles/lightblue.qss`.

## Requisitos

- Qt 6.5 o superior (Widgets, Gui, Svg).
- CMake 3.21 o superior.
- Compilador C++20 (Clang, GCC o MSVC).

## Construcción

```bash
cmake -S . -B build
cmake --build build
```

En macOS y Linux se creará el ejecutable `ProyectoPER`. En Windows se generará `ProyectoPER.exe`.

## Estructura de datos

- `navdb.sqlite`: base de datos SQLite gestionada por las librerías navdb. Colócala junto al fichero de proyecto (`CMakeLists.txt` / `ProyectoPER.pro`). La aplicación copiará este archivo junto al ejecutable durante la compilación/instalación.
- `question_history` (tabla dentro de `navdb.sqlite`): almacena el detalle de cada intento (pregunta, opciones seleccionadas, respuestas correctas) para reconstruir el historial avanzado.
- `data/avatars/`: directorio local donde se guardan los avatares exportados desde la base de datos o seleccionados por el usuario. El camino almacenado es relativo a esta carpeta.

## Personalización

- Para añadir o modificar problemas actualiza la tabla `problem` dentro de `navdb.sqlite` (puedes usar SQLite Browser o el script que prefieras). Tras los cambios no es necesario recompilar, basta con reiniciar la aplicación para que navegue con los nuevos datos.
- Las imágenes de los instrumentos y la carta se encuentran en `resources/images/` y se empaquetan en el recurso Qt definido en `CMakeLists.txt`.
- El estilo se ajusta en `styles/lightblue.qss`.
