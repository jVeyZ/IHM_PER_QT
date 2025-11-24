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

- `data/users.json`: almacena usuarios, contraseñas con hash y sesiones realizadas.
- `data/problems.json`: catálogo de problemas con respuestas tipo test.
- `data/avatars/`: contiene los avatares personalizados copiados por la aplicación.

## Personalización

- Para añadir problemas basta con extender `data/problems.json` respetando el mismo esquema.
- Las imágenes de los instrumentos y la carta se encuentran en `resources/images/` y se empaquetan en el recurso Qt definido en `CMakeLists.txt`.
- El estilo se ajusta en `styles/lightblue.qss`.
