# Manual de usuario — LoraSenderAysafi

Guía para técnicos de campo: instalación, configuración inicial y administración diaria del equipo (control de portón vehicular y puerta peatonal).

## 1. Contenido del equipo

- Placa TTGO/Heltec LoRa32 V2.0 con pantalla OLED integrada.
- Módulo SD para almacenar la lista blanca de tags y el log de eventos.
- Lector RFID/NFC conectado por puerto serie (USB o UART).
- Dos relés de salida: **PORTON** (vehicular) y **PUERTA** (peatonal).
- Tres entradas de botón: activar portón, activar puerta, parada de emergencia.

## 2. Encendido y pantalla

Al encender, la pantalla OLED muestra en orden:

1. `LoRa Sender <ID único del equipo>` — el ID único (derivado del MAC del chip) identifica a este equipo en la red LoRa/MQTT. Anótalo, lo necesitarás para configurar el receptor remoto.
2. Estado de la SD (`SDCard PASS/FAIL` y tamaño). **Si falla, el equipo se reinicia solo** — revisa que la SD esté bien insertada y formateada (FAT32).
3. `Tags Loaded <n>` o `No Tags` — cuántas credenciales cargó desde `tags.txt`.
4. Estado de WiFi (`IP: x.x.x.x` o `WiFi Connect Fail`).
5. Estado de LoRa (`LoRa Pass`/`LoRa Fail`). Si falla, el equipo espera y se reinicia automáticamente.
6. `LoraSend Ready` — el equipo ya está operativo.

Durante la operación normal, la pantalla muestra el último evento (tag leído, accionamiento remoto, etc.).

## 3. Administrar la lista blanca de tags

> Hoy no existe un portal web ni pantalla de alta de tags: la lista blanca se administra editando el archivo `tags.txt` directamente en la SD. Ver el [Roadmap del README](README.md#roadmap-hacia-un-sistema-de-control-de-acceso-propio-de-aysafi) para el plan de agregar un portal de administración.

### 3.1 Formato del archivo `tags.txt`

Ubicación: raíz de la tarjeta SD, archivo `/tags.txt`. Una credencial por línea, tres campos separados por coma:

```
<ID_DEL_TAG>,<APARTAMENTO_O_NOMBRE>,<FECHA>
```

Ejemplo:

```
04A3B2C1,Apto 101,01/09/2026
80AA8B91,Apto 204,05/09/2026
1F2E3D4C,Visitante Juan Perez,09/09/2026
```

- **`ID_DEL_TAG`**: el identificador que imprime el lector RFID/NFC por el puerto serie al pasar la tarjeta/llavero (visible también en el log si conectas por USB a una PC con monitor serie a 9600 baudios).
- **`APARTAMENTO_O_NOMBRE`**: texto libre, se usa solo para identificar al dueño en logs y MQTT — no puede contener comas.
- **`FECHA`**: texto libre (no se valida ni se usa para expirar el acceso automáticamente); útil para llevar registro de cuándo se dio de alta.
- Máximo **150 tags** por equipo (`maxTags`). Si necesitas más, hay que ampliar el firmware (contactar a soporte técnico).

### 3.2 Cómo agregar o quitar un tag

1. **Apaga el equipo** o espera a que no esté en uso (evita retirar la SD con el equipo energizado).
2. Retira la tarjeta SD e insértala en una computadora.
3. Abre `tags.txt` con un editor de texto simple (Notepad, VS Code, etc. — **no** Word).
4. Para **agregar**: añade una línea nueva al final con el formato de arriba.
   Para **quitar**: borra la línea completa del tag correspondiente.
5. Guarda el archivo en formato texto plano (UTF-8, sin cambiar el nombre).
6. Vuelve a insertar la SD en el equipo y enciende/reinicia. La pantalla mostrará `Tags Loaded <n>` con el nuevo total.

> El cambio solo se aplica al reiniciar (los tags se cargan una vez en el arranque, dentro de `setup()`).

### 3.3 Verificar que un tag fue aceptado

- Al pasar el tag por el lector, si está en la lista blanca el equipo activa el relé correspondiente y el evento se publica por MQTT en `aysafi/mqtt/loraSender/Monjitas1/Tags` (`id,apartamento,fecha_hora`).
- Si el tag no está en la lista, no ocurre ninguna acción (no hay aviso audible; revisa el log serie o MQTT para confirmar lecturas rechazadas).

## 4. Accionamiento manual (botones físicos)

| Botón | Pin | Función |
|---|---|---|
| Activar portón vehicular | `IN1` | Activa el relé `PORTON` |
| Activar puerta peatonal | `IN2` | Activa el relé `PUERTA` |
| Parada de emergencia | `IN3` | Corta el accionamiento en curso |

Los botones se leen por interrupción con antirrebote de software (~2 s de espera entre lecturas).

## 5. Accionamiento y monitoreo remoto (MQTT)

Un operador con acceso al broker MQTT (`emqx.aysafi.com`) puede:

- Publicar `"vehicular"` o `"peatonal"` en `/aysafi/mqtt/loraSender/Monjitas1/accionamientos` para abrir el portón o la puerta a distancia.
- Publicar `"1"` en `/aysafi/mqtt/loraSender/Monjitas1/reset` para reiniciar el equipo remotamente.
- Suscribirse a `aysafi/esp32Lora/Monjitas/Sender` para confirmar que el equipo sigue en línea (heartbeat periódico).

> El broker actual no exige usuario/contraseña — cualquier persona con acceso a esa red puede accionar el portón. Restringe el acceso de red al broker hasta que se implemente autenticación (ver README, sección Roadmap).

## 6. Actualización de firmware (OTA)

El equipo revisa si hay una versión nueva de firmware automáticamente:

- Al encender (dentro de `setup()`).
- Cada 30 minutos, porque el equipo se reinicia solo periódicamente.

No requiere ninguna acción del técnico de campo: si el equipo de desarrollo publicó una versión nueva (ver README, sección "Sistema de publicación de firmware"), el equipo la descarga, verifica su integridad y se reinicia solo con el firmware nuevo — **debe estar conectado a la red WiFi configurada** para poder actualizarse.

Durante la actualización, la pantalla muestra una barra de progreso ("Actualizando... NN%"). No desconectes la alimentación durante este proceso.

## 7. Configuración de red y placa (requiere reflashear)

Estos parámetros están fijados en el firmware y **no se pueden cambiar desde el equipo**; requieren recompilar y volver a flashear (contactar a soporte técnico/desarrollo):

- Red WiFi (SSID/contraseña) — inyectada en tiempo de compilación.
- Broker y puerto MQTT.
- Banda de frecuencia LoRa (433/868/915 MHz) — debe coincidir con el equipo receptor emparejado.
- Variante de placa (V1.0/V1.2/V1.6/V2.0).

## 8. Solución de problemas comunes

| Síntoma en pantalla | Causa probable | Acción |
|---|---|---|
| `SDCard FAIL` (y reinicio en bucle) | SD mal insertada, dañada o sin formatear FAT32 | Reinsertar/reformatear la SD; verificar que `tags.txt` exista |
| `No Tags` | `tags.txt` vacío, ausente o con formato incorrecto | Revisar formato (sección 3.1) |
| `WiFi Connect Fail` | Red fuera de rango, credenciales cambiaron, router caído | Verificar señal WiFi y estado del router; el equipo sigue funcionando localmente (LoRa/botones) sin WiFi, pero no recibirá OTA ni publicará por MQTT |
| `LoRa Fail` (y reinicio tras ~4 min) | Antena desconectada, módulo LoRa dañado, banda mal configurada | Revisar antena y banda (433/868/915) contra el equipo receptor |
| El portón no abre con un tag conocido | Tag no está en `tags.txt` de *este* equipo, o el ID no coincide exactamente | Confirmar el ID exacto leído por el lector serie y compararlo con el archivo |
