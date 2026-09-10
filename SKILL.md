# Skill: Microcontroladores — Desarrollo de hardware embebido

## Propósito
Proporcionar una guía reutilizable y pasos operativos para desarrollar firmware y prototipos sobre microcontroladores (STM32, ESP32 y otros), usando entornos como PlatformIO, el IDE de Arduino, herramientas HAL/CMSIS, C/C++ y MicroPython.

## Intención
Ayudar a automatizar y estandarizar tareas comunes de desarrollo embebido: configurar proyectos, seleccionar toolchain, estructura de firmware, pruebas en placa, depuración y consejos de despliegue.

## Cuándo usar
- Al comenzar un nuevo proyecto con STM32, ESP32 u otro MCU.
- Al convertir ejemplos de Arduino a PlatformIO o HAL.
- Para crear plantillas reproducibles que sigan buenas prácticas (estructura de carpetas, configuraciones de compilador, tareas de prueba y depuración).

## Fuera de alcance
- Gestión de infraestructura en la nube.
- Diseño de hardware avanzado (PCB de alta velocidad, señales diferenciales complejas).

## Flujo de trabajo paso a paso
1. Definir objetivo y restricciones (MCU, memoria, periféricos, energía, RTOS?).
2. Seleccionar framework: `PlatformIO` (recomendado), `Arduino` core, o `MicroPython` según requisitos.
3. Inicializar proyecto:
   - Para PlatformIO: crear `platformio.ini` con la placa y framework.
   - Para STM32/HAL: generar código base con CubeMX si aplica.
4. Diseñar estructura de carpetas: `src/`, `include/`, `lib/`, `tests/` y `platformio.ini`.
5. Implementar drivers por capas (HAL/CMSIS → drivers de dispositivo → abstracción de hardware → aplicación).
6. Añadir configuración de compilación y opciones de optimización/flags.
7. Escribir pruebas unitarias cuando sea posible (uso de `Unity` en PlatformIO).
8. Flashear y depurar en placa (OpenOCD, ST-Link, esptool, o depurador integrado).
9. Verificar criterios de calidad: consumo, latencia, estabilidad, cobertura mínima de pruebas.
10. Empaquetar y documentar: `README.md`, ejemplo de uso y checklist de pruebas.

## Puntos de decisión y ramas
- RTOS vs bucle principal: elegir si usar `FreeRTOS`/`RT-Thread` según concurrencia y temporización.
- Lenguaje: C/C++ para rendimiento y control; MicroPython para prototipado rápido.
- Método de despliegue: flasheo por USB/ST-Link o actualizaciones OTA (ESP32).

## Criterios de calidad / Checklist
- Proyecto compila con `-Wall` y sin warnings relevantes.
- Tests unitarios básicos pasan en CI/local.
- Recuperación ante fallo (watchdog habilitado si aplica).
- Documentación mínima: cómo compilar, flashear y depurar.

## Comandos útiles (ejemplos)
- Inicializar PlatformIO project:
  - `pio project init --board <board_id> --project-dir .`
- Compilar: `pio run`
- Flashear: `pio run -t upload`
- Depurar: `pio debug` o usar `OpenOCD`/`st-flash` según placa.

## Archivos y plantillas recomendadas
- `platformio.ini` con entornos para debug/release.
- `src/main.cpp`, `include/board_def.h`, `lib/` para drivers.
- `tests/` con casos mínimos para lógica crítica.

## Ejemplos de prompts para usar con este skill
- "Crea un `platformio.ini` para una NUCLEO-F401RE con FreeRTOS y Unity tests."
- "Convierte este sketch de Arduino para ESP32 a PlatformIO y añade OTA."
- "Genera plantilla de drivers HAL para I2C y SPI con pruebas unitarias."

## Iteración y mejoras
1. Guardar el SKILL.md en el repositorio.
2. Recoger feedback del equipo y añadir ejemplos concretos de placas usadas.
3. Añadir snippets o plantillas adicionales (cubeMX export, esptool OTA script).

---
Última actualización: 2026-07-30
