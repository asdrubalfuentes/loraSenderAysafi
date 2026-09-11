#ifndef C1C60F5C_76C7_44B1_8B8A_E12126045FCD
#define C1C60F5C_76C7_44B1_8B8A_E12126045FCD
#include <Arduino.h>
// #include <HardwareSerial.h>

#define LORA_V1_0_OLED 0
#define LORA_V1_2_OLED 0
#define LORA_V1_6_OLED 0
#define LORA_V2_0_OLED 1

#ifndef LORA_SENDER
#define LORA_SENDER 1
#endif

// #define LORA_PERIOD 868
#define LORA_PERIOD 915
// #define LORA_PERIOD 433

#if LORA_V1_0_OLED
#include <Wire.h>
#include "SSD1306Wire.h"
#define OLED_CLASS_OBJ SSD1306Wire
#define OLED_ADDRESS 0x3C
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define CONFIG_MOSI 27
#define CONFIG_MISO 19
#define CONFIG_CLK 5
#define CONFIG_NSS 18
#define CONFIG_RST 14
#define CONFIG_DIO0 26
// !    There are two versions of TTGO LoRa V1.0,
// !    the 868 version uses the 3D WiFi antenna, and the 433 version uses the PCB antenna.
// !    You need to change the frequency according to the board.

#define SDCARD_MOSI -1
#define SDCARD_MISO -1
#define SDCARD_SCLK -1
#define SDCARD_CS -1

#elif LORA_V1_2_OLED
// Lora V1.2 ds3231
#include <Wire.h>
#include "SSD1306Wire.h"
#define OLED_CLASS_OBJ SSD1306Wire
#define OLED_ADDRESS 0x3C
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST -1
#define CONFIG_MOSI 27
#define CONFIG_MISO 19
#define CONFIG_CLK 5
#define CONFIG_NSS 18
#define CONFIG_RST 23
#define CONFIG_DIO0 26

#define SDCARD_MOSI -1
#define SDCARD_MISO -1
#define SDCARD_SCLK -1
#define SDCARD_CS -1

#define ENABLE_DS3231

#elif LORA_V1_6_OLED
#include <Wire.h>
#include "SSD1306Wire.h"
#define OLED_CLASS_OBJ SSD1306Wire
#define OLED_ADDRESS 0x3C
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST -1

#define CONFIG_MOSI 27
#define CONFIG_MISO 19
#define CONFIG_CLK 5
#define CONFIG_NSS 18
#define CONFIG_RST 23
#define CONFIG_DIO0 26

#define SDCARD_MOSI 15
#define SDCARD_MISO 2
#define SDCARD_SCLK 14
#define SDCARD_CS 13

#elif LORA_V2_0_OLED
#include <Wire.h>
#include "SSD1306Wire.h"
#define OLED_CLASS_OBJ SSD1306Wire
#define OLED_ADDRESS 0x3C
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST -1
#define CONFIG_MOSI 27
#define CONFIG_MISO 19
#define CONFIG_CLK 5
#define CONFIG_NSS 18
#define CONFIG_RST 23
#define CONFIG_DIO0 26

#define SDCARD_MOSI 15
#define SDCARD_MISO 2
#define SDCARD_SCLK 14
#define SDCARD_CS 13
#define DS3231_ONBOARD 0
#define IN1 GPIO_NUM_32    // ACTIVACION PORTÓN VEHICULAR ARRIBA
#define IN2 GPIO_NUM_33    // ACTIVACION PUERTA PEATONAL ARRIBA
#define IN3 GPIO_NUM_0     // PARADA EMERGENCIA PORTON VEHIULAR ARRIBA
#define IN4 GPIO_NUM_34    // ACTIVACION PORTON VEHICULAR ABAJO
#define IN5 GPIO_NUM_36    // ACTIVACION PUERTA PEATONAL ABAJO
#define IN6 GPIO_NUM_39    // PARADA EMERGENCIA PORTON VEHIULAR ABAJO
#define PORTON GPIO_NUM_4  // RELE PORTON VEHICULAR
#define PUERTA GPIO_NUM_25 // RELE PUERTA PEATONAL

// HardwareSerial miModem(1);

#else
#error "please select board"
#endif

#if LORA_PERIOD == 433
#define BAND 433E6
#elif LORA_PERIOD == 868
#define BAND 868E6
#elif LORA_PERIOD == 915
#define BAND 915E6
#else
#error "Please select the correct lora frequency"
#endif

#endif /* C1C60F5C_76C7_44B1_8B8A_E12126045FCD */

// Optionally include custom images
#include "images.h"

#include <SPI.h>
#include <LoRa.h>

#include <WiFi.h>
#include <SD.h>

OLED_CLASS_OBJ display(OLED_ADDRESS, OLED_SDA, OLED_SCL);

// WIFI_SSID/WIFI_PASSWORD deben venir de build_flags (platformio.ini -> sysenv),
// nunca hardcodeados en el repo publico. Ver README para configurar variables locales/CI.
#ifndef WIFI_SSID
#error "Defina WIFI_SSID via build_flags (-D WIFI_SSID=\\\"...\\\"), no lo hardcodee en el codigo."
#endif
#ifndef WIFI_PASSWORD
#error "Defina WIFI_PASSWORD via build_flags (-D WIFI_PASSWORD=\\\"...\\\"), no lo hardcodee en el codigo."
#endif
// #define CORRIENTE_4A20 GPIO_NUM_36
#define MIN_4mA_ZERO 0
#define MAX_20mA_SPAN 60000
#define MIN_COUNTS_4mA 11836
#define MAX_COUNTS_20mA 59578
#define USE_CURRENT_LOOP 0
#define USE_SERIAL_PORT 1

#define WINDOW_SIZE 20
#define NUM_CHANNELS 1

int analogBuffer[NUM_CHANNELS][WINDOW_SIZE];
int analogIndex[NUM_CHANNELS];

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <FS.h>
#include <WebServer.h>
#include <MQTTPubSubClient.h>
#include <CRC32.h>
#include <mbedtls/sha256.h>

SPIClass sdSPI(VSPI);

// Canal de publicacion: GitHub Releases del repo (asset "latest" estable).
// Un tag vX.Y.Z en `main` (release aprobado por el mantenedor) dispara el
// workflow que publica firmware.bin + version.txt + firmware.sha256 como
// "latest", listos para que el dispositivo los descargue en el proximo reinicio.
#define GH_OWNER "asdrubalfuentes"
#define GH_REPO "loraSenderAysafi"
#define GH_RELEASE_BASE "https://github.com/" GH_OWNER "/" GH_REPO "/releases/latest/download/"

#if LORA_SENDER == 1
const char *versionURL = GH_RELEASE_BASE "version.txt";
const char *firmwareURL = GH_RELEASE_BASE "firmware.bin";
const char *firmwareSha256URL = GH_RELEASE_BASE "firmware.sha256";
const String idSlave = "80aa8b910250";
#ifdef FW_VERSION_OVERRIDE
String currentVersion = FW_VERSION_OVERRIDE;
#else
String currentVersion = "1.0.24";
#endif
bool respuesta = false;
#elif LORA_SENDER == 0
const char *versionURL = GH_RELEASE_BASE "version.txt";
const char *firmwareURL = GH_RELEASE_BASE "firmware.bin";
const char *firmwareSha256URL = GH_RELEASE_BASE "firmware.sha256";
#ifdef FW_VERSION_OVERRIDE
String currentVersion = FW_VERSION_OVERRIDE;
#else
String currentVersion = "1.0.24";
#endif
bool vehicularRemoteFlag = false, peatonalRemoteFlag = false;
String idMaster = "80aa8b910250";
#endif

const char *mqttServer = "emqx.aysafi.com";
const uint16_t mqttPort = 1883;
// const char *mqttUser = "lorareSenderM";
// const char *mqttPassword = "loraSenderM";
//  Configuración del servidor NTP
const char *ntpServer = "pool.ntp.org";
// const char *LoRaSSID = "LoRaBal";
// const char *LoRaPassword = "monjitasOriente";
const long gmtOffset_sec = -4 * 3600; // Offset en segundos de GMT (0 para GMT)
const int daylightOffset_sec = 3600;  // Offset de horario de verano (3600 para una hora)
uint16_t comandoBoton = 0;
boolean flagBoton = false, vehicularFlag = false, peatonalFlag = false;

String idUnico = "";
const uint8_t maxTags = 150;
struct tag
{
    String id;
    String apartamento;
    String fecha;
};

WiFiClient espClient;
MQTTPubSubClient mqtt;

uint8_t countTags = 0;
tag listaBlanca[maxTags]; // Lista de dispositivos autorizados
bool flagTagActivado = false;
uint32_t tiempoUltimoCambioTags = 0;

// Versión actual del firmware

String data = "";

unsigned long ota_progress_millis = 0;

void initTime()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Fallo al obtener la hora desde el servidor NTP");
        return;
    }
    Serial.println(&timeinfo, "Hora sincronizada: %A, %B %d %Y %H:%M:%S");
}

void connect()
{
    int retry = 0;
connect_to_wifi:
    Serial.print("connecting to wifi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        WiFi.reconnect();
        delay(1000);
        Serial.print(".");
        if (++retry > 10)
        {
            Serial.println("WiFi connection failed, continue...");
            return;
        }
    }
    Serial.println(" connected!");

connect_to_host:
    Serial.print("connecting to host...");
    espClient.stop();
    while (!espClient.connect(mqttServer, 1883))
    {
        Serial.print(".");
        delay(1000);
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi disconnected");
            goto connect_to_wifi;
        }
    }
    Serial.println(" connected!");

    Serial.print("connecting to mqtt broker...");
    mqtt.disconnect();
    while (!mqtt.connect("LoraSender" + idUnico, "", ""))
    {
        Serial.print(".");
        delay(1000);
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi disconnected");
            goto connect_to_wifi;
        }
        if (espClient.connected() != 1)
        {
            Serial.println("WiFiClient disconnected");
            goto connect_to_host;
        }
    }
    Serial.println(" connected!");
    initTime();
}

void addAnalogValue(int valueInput, int channel)
{
    // Agrega la temperatura al buffer del canal correspondiente
    analogBuffer[channel][analogIndex[channel]] = valueInput;
    analogIndex[channel] = (analogIndex[channel] + 1) % WINDOW_SIZE;
}

int getAnalogFiltered(int channel)
{
    int sum = 0;
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        sum += analogBuffer[channel][i];
    }
    return sum / WINDOW_SIZE;
}

uint32_t tiempoUltimoCambioBotones = 0;

#if LORA_SENDER == 1

void isrBotones()
{
    static uint16_t comandoBotonAnterior = 0;
    // comandoBoton &= 0x7;
    if (digitalRead(IN1) == LOW)
    {
        comandoBoton |= 1;
    }
    else
    {
        comandoBoton &= 6;
    }

    if (digitalRead(IN2) == LOW)
    {
        comandoBoton |= 2;
    }
    else
    {
        comandoBoton &= 5;
    }
    if (digitalRead(IN3) == LOW)
    {
        comandoBoton |= 4;
    }
    else
    {
        comandoBoton &= 0xFB;
    }

    if (comandoBoton != comandoBotonAnterior)
    {
        comandoBotonAnterior = comandoBoton;
        flagBoton = true;
        Serial.printf("comandoBoton: %d\n", comandoBoton);
    }

    tiempoUltimoCambioBotones = millis();
}
#else
#endif

// Función para verificar la versión más reciente
String getLatestVersion()
{
    // releases/latest/download/* responde 302 al host de assets de GitHub:
    // sin seguir redirecciones el GET vuelve vacio y la OTA no baja nada.
    WiFiClientSecure sec;
    sec.setInsecure();
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);
    http.begin(sec, versionURL);
    int httpCode = http.GET();
    String payload = "";

    if (httpCode == 200)
    { // Código 200: OK
        payload = http.getString();
        payload.trim();
    }
    else
    {
        Serial.printf("Error en la consulta de la version: %d\n", httpCode);
    }

    http.end();
    return payload;
}

// Descarga firmware.sha256 (texto hex de 64 chars) publicado junto al binario.
String getLatestFirmwareSha256()
{
    WiFiClientSecure sec;
    sec.setInsecure();
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);
    http.begin(sec, firmwareSha256URL);
    int httpCode = http.GET();
    String payload = "";

    if (httpCode == 200)
    {
        payload = http.getString();
        payload.trim();
        payload.toLowerCase();
    }
    else
    {
        Serial.printf("Error al obtener el checksum del firmware: %d\n", httpCode);
    }

    http.end();
    return payload;
}

// Compara versiones "MAJOR.MINOR.PATCH" numericamente (evita fallas de "1.0.9" vs "1.0.10").
bool isVersionNewer(const String &latest, const String &current)
{
    int la = 0, lb = 0, lc = 0;
    int ca = 0, cb = 0, cc = 0;
    if (sscanf(latest.c_str(), "%d.%d.%d", &la, &lb, &lc) != 3 ||
        sscanf(current.c_str(), "%d.%d.%d", &ca, &cb, &cc) != 3)
    {
        Serial.println("Formato de version invalido, se omite la actualizacion");
        return false;
    }
    if (la != ca) return la > ca;
    if (lb != cb) return lb > cb;
    return lc > cc;
}

// Función para obtener la hora actual desde el servidor NTP


String getFormattedDateTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Fallo al obtener la hora");
        return "00/00/0000 00:00:00";
    }
    char dateTimeString[20];
    strftime(dateTimeString, sizeof(dateTimeString), "%d/%m/%Y %H:%M:%S", &timeinfo);
    return String(dateTimeString);
}

void logMessage(const String &message)
{
    // Obtener la marca de tiempo actual
    String timestamp = getFormattedDateTime();
    String logEntry = "[" + timestamp + "] " + message;

    // Escribir en el archivo log.txt
    if (SD.exists("/log.txt"))
    {
        File logFile = SD.open("/log.txt", FILE_APPEND);
        if (logFile)
        {
            logFile.println(logEntry);
            logFile.close();
        }
        else
        {
            Serial.println("Error al abrir log.txt para escritura.");
        }
    }
    else
    {
        File logFile = SD.open("/log.txt", FILE_WRITE);
        if (logFile)
        {
            logFile.println(logEntry);
            logFile.close();
        }
        else
        {
            Serial.println("Error al crear log.txt.");
        }
    }

    // Publicar en el tópico MQTT
    if (mqtt.isConnected())
    {
#if LORA_SENDER == 1
        mqtt.publish("/aysafi/monjitas1/sender/logs", logEntry);
#else
        mqtt.publish("/aysafi/monjitas1/receiver/logs", logEntry);
#endif
    }
    else
    {
        Serial.println("Error: MQTT no está conectado.");
    }

    // Mostrar en el monitor serial
    Serial.println(logEntry);
}

void saveTagsToFile(const char *filename)
{
    File file = SD.open(filename, FILE_WRITE);
    if (!file)
    {
        Serial.println("Error al abrir el archivo para escribir");
        return;
    }

    for (uint8_t i = 0; i < maxTags; i++)
    {
        if (listaBlanca[i].id != "")
        {
            file.println(listaBlanca[i].id + "," + listaBlanca[i].apartamento + "," + listaBlanca[i].fecha);
        }
    }

    file.close();
    Serial.println("Array de structs guardado en el archivo");
}

void loadTagsFromFile(const char *filename)
{
    File file = SD.open(filename, FILE_READ);
    if (!file)
    {
        Serial.println("Error al abrir el archivo para leer");
        return;
    }

    uint8_t index = 0;
    while (file.available() && index < maxTags)
    {
        String line = file.readStringUntil('\n');
        int commaIndex1 = line.indexOf(',');
        int commaIndex2 = line.lastIndexOf(',');

        if (commaIndex1 > 0 && commaIndex2 > commaIndex1)
        {
            listaBlanca[index].id = line.substring(0, commaIndex1);
            listaBlanca[index].apartamento = line.substring(commaIndex1 + 1, commaIndex2);
            listaBlanca[index].fecha = line.substring(commaIndex2 + 1);
            index++;
            countTags = index;
        }
    }

    file.close();
    Serial.println("Array de structs recuperado del archivo");
}

// Función para descargar y actualizar el firmware
void updateFirmware()
{
    String expectedSha256 = getLatestFirmwareSha256();
    if (expectedSha256.length() != 64)
    {
        Serial.println("Checksum del firmware no disponible o invalido, se aborta la actualizacion");
        return;
    }

    WiFiClientSecure sec;
    sec.setInsecure();
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);
    http.begin(sec, firmwareURL);
    int httpCode = http.GET();

    if (httpCode == 200)
    { // Código 200: OK
        int len = http.getSize();
        if (len > 0)
        {
            WiFiClient *client = http.getStreamPtr();
            if (!Update.begin(len))
            {
                Serial.println("Error al iniciar la actualizacion");
                http.end();
                return;
            }

            // Configurar el callback para el progreso
            Update.onProgress([](size_t written, size_t total)
                              {
                            int progress = (written * 100) / total; // Calcular el porcentaje
                            display.clear();
                            display.setFont(ArialMT_Plain_10);
                            display.setTextAlignment(TEXT_ALIGN_CENTER);
                            display.drawString(display.getWidth() / 2, 10, "Actualizando...");
                            display.drawProgressBar(0, 30, 120, 10, progress); // Dibujar barra de progreso
                            display.drawString(display.getWidth() / 2, 45, String(progress) + "%"); // Mostrar porcentaje
                            display.display(); });

            // Se descarga por bloques calculando el SHA256 en paralelo, para
            // verificar la integridad del binario antes de dar por buena la
            // actualizacion (evita flashear un firmware corrupto o falsificado).
            mbedtls_sha256_context shaCtx;
            mbedtls_sha256_init(&shaCtx);
            mbedtls_sha256_starts(&shaCtx, 0 /* SHA-256, no SHA-224 */);

            uint8_t buf[1024];
            size_t written = 0;
            bool writeError = false;
            while (written < (size_t)len)
            {
                size_t available = client->available();
                if (available == 0)
                {
                    if (!client->connected())
                    {
                        break;
                    }
                    delay(1);
                    continue;
                }
                size_t toRead = min(available, sizeof(buf));
                size_t readBytes = client->readBytes(buf, toRead);
                if (readBytes == 0)
                {
                    break;
                }
                if (Update.write(buf, readBytes) != readBytes)
                {
                    writeError = true;
                    break;
                }
                mbedtls_sha256_update(&shaCtx, buf, readBytes);
                written += readBytes;
            }

            uint8_t digest[32];
            mbedtls_sha256_finish(&shaCtx, digest);
            mbedtls_sha256_free(&shaCtx);

            char digestHex[65];
            for (int i = 0; i < 32; i++)
            {
                sprintf(digestHex + (i * 2), "%02x", digest[i]);
            }
            digestHex[64] = '\0';

            if (writeError || written != (size_t)len)
            {
                Serial.println("Error al escribir el archivo");
                Update.abort();
                http.end();
                return;
            }

            if (expectedSha256 != String(digestHex))
            {
                Serial.printf("Checksum invalido. Esperado: %s Obtenido: %s\n", expectedSha256.c_str(), digestHex);
                Update.abort();
                http.end();
                return;
            }
            Serial.println("Checksum verificado correctamente");

            if (Update.end())
            {
                Serial.println("Actualizacion finalizada correctamente");
                if (Update.isFinished())
                {
                    Serial.println("Actualizacion completada, reiniciando...");
                    ESP.restart();
                }
                else
                {
                    Serial.println("Actualizacion fallida");
                }
            }
            else
            {
                Serial.printf("Error de actualizacion: %s\n", Update.errorString());
            }
        }
        else
        {
            Serial.println("Archivo binario vacio");
        }
    }
    else
    {
        Serial.printf("Error en la descarga del firmware: %d\n", httpCode);
    }

    http.end();
}
uint32_t tiempoUltimo = 0;
uint8_t Bytes_leidos[3] = {0};
uint8_t contadorDeBytes = 0;
bool calcularDatos = false;

void isrSerial(void)
{

    if (Serial.available())
    {
        if (millis() - tiempoUltimo < 300)
        {
            Bytes_leidos[contadorDeBytes] = Serial.read();
            if (++contadorDeBytes > 2)
            {
                calcularDatos = true;
                contadorDeBytes = 0;
            }
            tiempoUltimo = millis();
        }
        else
        {
            Serial.flush();
        }
    }
}

void chequearActualizaciones(void)
{
    if (WiFi.isConnected())
    {
        // Obtener la última versión disponible
        String latestVersion = getLatestVersion();
        // Serial.printf("Version actual: %s, Ultima version: %s\n", currentVersion.c_str(), latestVersion.c_str());
        //  Comparar versiones (numerico, no lexicografico)
        if (latestVersion.length() > 0 && isVersionNewer(latestVersion, currentVersion))
        {
            // Serial.println("Nueva version disponible, iniciando actualizacion...");
            display.setFont(ArialMT_Plain_10);
            display.clear();
            display.printf(" Version actual: %s\n Ultima version: %s\n Aplicando Actualizacion...", currentVersion.c_str(), latestVersion.c_str());
            display.display();
            updateFirmware();
        }
    }
}

#define DEMO_DURATION 3000
typedef void (*Demo)(void);

int demoMode = 0;
int counter = 1;
int count = 0;

void drawFontFaceDemo()
{
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Hello world");
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Hello world");
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 26, "Hello world");
}
volatile uint32_t relojAnterior = 0U;

void onReceive(int packetSize)
{
    relojAnterior = micros();
}

void drawTextFlowDemo()
{
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawStringMaxWidth(0, 0, 128,
                               "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore.");
}

void drawTextAlignmentDemo()
{
    // Text alignment demo
    display.setFont(ArialMT_Plain_10);

    // The coordinates define the left starting point of the text
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 10, "Left aligned (0,10)");

    // The coordinates define the center of the text
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 22, "Center aligned (64,22)");

    // The coordinates define the right end of the text
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 33, "Right aligned (128,33)");
}

void drawRectDemo()
{
    // Draw a pixel at given position
    for (int i = 0; i < 10; i++)
    {
        display.setPixel(i, i);
        display.setPixel(10 - i, i);
    }
    display.drawRect(12, 12, 20, 20);

    // Fill the rectangle
    display.fillRect(14, 14, 17, 17);

    // Draw a line horizontally
    display.drawHorizontalLine(0, 40, 20);

    // Draw a line horizontally
    display.drawVerticalLine(40, 0, 20);
}

void drawCircleDemo()
{
    for (int i = 1; i < 8; i++)
    {
        display.setColor(WHITE);
        display.drawCircle(32, 32, i * 3);
        if (i % 2 == 0)
        {
            display.setColor(BLACK);
        }
        display.fillCircle(96, 32, 32 - i * 3);
    }
}

void drawProgressBarDemo()
{
    int progress = (counter / 5) % 100;
    // draw the progress bar
    display.drawProgressBar(0, 32, 120, 10, progress);

    // draw the percentage as String
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 15, String(progress) + "%");
}

void drawImageDemo()
{
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    display.clear();
    display.drawXbm((128 - aysafi_Logo_width) / 2, (64 - aysafi_Logo_height) / 2, aysafi_Logo_width, aysafi_Logo_height, aysafi_Logo_bits);
    // display.flipScreenVertically();
    // display.drawFastImage(0, 0, 50, 56, WiFi_Logo_bits);
    display.display();
    delay(2000);
    display.clear();
    display.drawXbm(0, (64 - poweredBy_height) / 2, poweredBy_width, poweredBy_height, poweredBy);
    // display.flipScreenVertically();
    // display.drawFastImage(0, 0, 50, 56, WiFi_Logo_bits);
    display.display();
}
int peso = 0;
String trama = "";
//#if LORA_SENDER == 1

String info = "Ofline";

void pintarDatos(void)
{
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_16);

    display.drawString(display.getWidth() / 2, display.getHeight() / 2, data);
    #if LORA_SENDER == 0
    if (vehicularRemoteFlag == true || peatonalRemoteFlag == true)
    {
        info = "Online";
        String mensaje = LoRa.readString();
        tiempoUltimo = millis();
    }
    else
    {
        info = "noCmd";
    }
    #else
    if (respuesta == true)
    {
        info = "Online";
        String mensaje = LoRa.readString();
        tiempoUltimo = millis();
    }
    else
    {
        info = "noCmd";
    }
    #endif
    String informacion = "[" + info + "]" + "RSSI " + String(LoRa.packetRssi());
    display.setFont(ArialMT_Plain_10);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 20, informacion);
    display.drawString(display.getWidth() / 2, 52, "aysafi.com FW Ver: " + currentVersion);
    display.display();
}
//#endif

Demo demos[] = {drawFontFaceDemo, drawTextFlowDemo, drawTextAlignmentDemo, drawRectDemo, drawCircleDemo, drawProgressBarDemo, drawImageDemo};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;
