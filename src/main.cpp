/**
 * ING ASDRUBAL FUENTES
 * AYSAFI
 */
#include "board_def.h"
#include "runtime_config.h"
#include "access_portal.h"

void setup()
{
  Serial.begin(9600);

  pinMode(IN1, INPUT_PULLUP);
  pinMode(IN2, INPUT_PULLUP);
  pinMode(IN3, INPUT_PULLUP);
  pinMode(PORTON, OUTPUT);
  pinMode(PUERTA, OUTPUT);

  digitalWrite(PORTON, LOW);
  digitalWrite(PUERTA, LOW);

  if (OLED_RST > 0)
  {
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, HIGH);
    delay(100);
    digitalWrite(OLED_RST, LOW);
    delay(100);
    digitalWrite(OLED_RST, HIGH);
  }
  // pinMode(CORRIENTE_4A20, INPUT);
  // analogReadResolution(16);

  display.init();
  display.flipScreenVertically();
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  idUnico = String(ESP.getEfuseMac(), HEX);
  display.drawString(display.getWidth() / 2, display.getHeight() / 4, LORA_SENDER ? "LoRa Sender\n" + idUnico : "LoRa Receiver\n" + idUnico);
  display.display();
  delay(500);
  drawImageDemo();
  delay(2000);
  if (SDCARD_CS > 0)
  {
    display.clear();
    sdSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
    if (!SD.begin(SDCARD_CS, sdSPI))
    {
      display.drawString(display.getWidth() / 2, display.getHeight() / 2, "SDCard  FAIL");
      esp_restart();
    }
    else
    {
      display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 16, "SDCard  PASS");
      uint32_t cardSize = SD.cardSize() / (1024 * 1024);
      display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Size: " + String(cardSize) + "MB");
      /// Para Probar la SD y leer el contenido de la SD
      /*File root = SD.open("/", FILE_READ);
      Serial.println("Files found on SD card:");
      while (true)
      {
        File entry = root.openNextFile();
        if (!entry)
        {
          break;
        }
        Serial.println(entry.name());
        entry.close();
      }
      root.close();*/
    }
    display.display();
    delay(500);
  }

  loadTagsFromFile("/tags.txt");
  if (countTags == 0)
  {
    display.clear();
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "No Tags");
    display.display();
    delay(2000);
    // esp_restart();
  }
  else
  {
    display.clear();
    String info = "Tags Loaded " + String(countTags);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, info);
    display.display();
    delay(2000);
  }

#if DS3231_ONBOARD
  String info = ds3231_test();
  if (info != "")
  {
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, info);
    display.display();
    delay(2000);
  }
#endif // DS3231_ONBOARD

  WiFi.begin(cfgWifiSsid().c_str(), cfgWifiPassword().c_str());
  delay(2000);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    display.clear();
    // Serial.println("WiFi Connect Fail");
    display.setFont(ArialMT_Plain_10);
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "WiFi Connect Fail");
    // logMessage("WiFi Connect Fail");
    display.display();
    delay(500);
    // esp_restart();
  }
  display.clear();
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "IP:" + WiFi.localIP().toString());
  // logMessage("IP:" + WiFi.localIP().toString());
  display.display();
  delay(1000);

  SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
  LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
  if (!LoRa.begin(BAND))
  {
    display.clear();
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoRa Fail");
    // logMessage("LoRa Fail");
    display.display();
    while (millis() < 240000)
      ;
    esp_restart();
  }
  else
  {
    display.clear();
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoRa Pass");
    // logMessage("LoRa Pass");
    display.display();
    delay(1000);
  }

  chequearActualizaciones();
  mqtt.begin(espClient);
  connect();
// subscribe callback which is called when every packet has come

// subscribe topic and callback which is called when /hello has come
#if LORA_SENDER == 1
  mqtt.subscribe("/aysafi/mqtt/loraSender/Monjitas1/reset", [](const String &payload, const size_t size)
                 {
    Serial.print("/aysafi/mqtt/loraSender/Monjitas1/reset ");
    Serial.println(payload); 
    if (payload == "1")
    {
      esp_restart();
    } });

  mqtt.subscribe("/aysafi/mqtt/loraSender/Monjitas1/accionamientos", [](const String &payload, const size_t size)
                 {
    Serial.print(".../Monjitas1/accionamientos ");
    Serial.println(payload);
    //logMessage("Accionamiento MQTT" + payload);
    if (payload == "vehicular")
    {
      digitalWrite(PORTON, HIGH);
      vehicularFlag = true;
      tiempoUltimoCambioTags = millis();
    }
    else if (payload == "peatonal")
    {
      digitalWrite(PUERTA, HIGH);
      peatonalFlag = true;
      tiempoUltimoCambioTags = millis();
    } });
#else
  mqtt.subscribe("/aysafi/mqtt/loraReceiver/Monjitas1/reset", [](const String &payload, const size_t size)
                 {
Serial.print("/aysafi/mqtt/loraReceiver/Monjitas1/reset ");
Serial.println(payload); 
if (payload == "1")
{
esp_restart();
} });

  mqtt.subscribe("/aysafi/mqtt/loraReceiver/Monjitas1/accionamientos", [](const String &payload, const size_t size)
                 {
Serial.print(".../Monjitas1/accionamientos ");
Serial.println(payload);
//logMessage("Accionamiento MQTT" + payload);
if (payload == "vehicular")
{
digitalWrite(PORTON, HIGH);
vehicularFlag = true;
tiempoUltimoCambioTags = millis();
}
else if (payload == "peatonal")
{
digitalWrite(PUERTA, HIGH);
peatonalFlag = true;
tiempoUltimoCambioTags = millis();
} });
#endif


  delay(500);
  display.setFont(ArialMT_Plain_10);
  Serial.flush();
#if LORA_SENDER == 0
  {
    display.clear();
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoraRecv Ready");
    // logMessage("LoraRecv Ready");
    display.display();
  }
#else
  {
    display.clear();
    display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoraSend Ready");
    // logMessage("LoraSend Ready");
    display.display();
  }
#endif
  pintarDatos();
  // Serial.println(analogReadMilliVolts(CORRIENTE_4A20));

  // attachInterrupt(RX, isrSerial, CHANGE);

  // PARA PRUEBAS DE RELES
  /*
  bool estado = false;
  for(int x = 0; x < 10; x++)
  {
    digitalWrite(PORTON, estado);
    //digitalWrite(PUERTA, estado);
    estado = !estado;
    delay(1000);
  }
    */
  if (mqtt.isConnected() == true)
  {
#if LORA_SENDER == 1
    mqtt.publish("aysafi/mqtt/Monjitas/Sender", "Reinicio del dispositivo");
#else
    mqtt.publish("aysafi/mqtt/Monjitas/Receiver", "Reinicio del dispositivo");
#endif
  }
  delay(200);
}

void loop()
{
  static uint32_t tiempoConsultaActualizaciones = 0;

  static String oldPayLoad;

  if (LoRa.parsePacket())
  {
    String recv = "";
    if (LoRa.available())
    {
      recv = LoRa.readStringUntil('\n');
      // valorOled = recv;
      if (recv.indexOf(idUnico) != -1)
      {
        // Extract the CRC value from the end of the message
        uint32_t receivedCRC;
        int messageLength = recv.length() - sizeof(receivedCRC);
        memcpy(&receivedCRC, recv.c_str() + messageLength, sizeof(receivedCRC));
        recv.remove(messageLength);

        // Calculate the CRC of the received message
        CRC32 crc;
        crc.update(recv.c_str(), recv.length());
        uint32_t calculatedCRC = crc.finalize();

        Serial.print("Received message: ");
        Serial.print(recv);
        Serial.print(" with CRC: ");
        Serial.println(receivedCRC, HEX);

        if (calculatedCRC == receivedCRC)
        {
#if LORA_SENDER == 0
          if (bitRead(recv.toInt(), 0) == 1)
          {
            digitalWrite(PORTON, HIGH);
            vehicularRemoteFlag = true;
            if (mqtt.isConnected() == true)
            {
              mqtt.publish("aysafi/mqtt/loraReceiver/Monjitas1/LoRa", "[" + getFormattedDateTime() + "] Vehicular LoRa");
            }
            tiempoUltimoCambioTags = millis();
          }
          if (bitRead(recv.toInt(), 1) == 1)
          {
            digitalWrite(PUERTA, HIGH);
            peatonalRemoteFlag = true;
            if (mqtt.isConnected() == true)
            {
              mqtt.publish("aysafi/mqtt/loraReceiver/Monjitas1/LoRa", "[" + getFormattedDateTime() + "] Peatonal LoRa");
            }
            tiempoUltimoCambioTags = millis();
          }
          data = recv;
          Serial.println("CRC check passed!");

          String message = idMaster + "," + data;
          crc.update(message.c_str(), message.length());
          uint32_t crcValue = crc.finalize();

          LoRa.beginPacket();
          LoRa.print(message);
          LoRa.write((uint8_t *)&crcValue, sizeof(crcValue));
          LoRa.print("\n");
          LoRa.endPacket();
#else
          respuesta = true;
#endif
        }
        else
        {
          Serial.println("CRC check failed!");
        }
      }
    }
    // TODO: UDP Implementation for display repeater
  }

#if USE_CURRENT_LOOP

  peso = analogRead(CORRIENTE_4A20);
  peso = peso - MIN_COUNTS_4mA;
  peso = peso * 1.26; //(MAX_20mA_SPAN/(MAX_COUNTS_20mA-MIN_COUNTS_4mA));
  addAnalogValue(peso, 0);
  peso = getAnalogFiltered(0);
#elif USE_SERIAL_PORT
  static String tagPass = "";
  static String estacionamiento = "";

  if (Serial.available() > 8)
  {
    trama = Serial.readStringUntil('\n');
    // Serial.print("trama: ");
    // Serial.println(trama);
    bool tagAutorizado = false;
    for (int x = 0; x < countTags; x++)
    {
      if (trama.indexOf(listaBlanca[x].id) != -1 && flagTagActivado == false)
      {
        tagPass = listaBlanca[x].id;
        estacionamiento = listaBlanca[x].apartamento;
        if (mqtt.isConnected() == true)
        {
#if LORA_SENDER == 1
          mqtt.publish("aysafi/mqtt/loraSender/Monjitas1/Tags", listaBlanca[x].id + "," + listaBlanca[x].apartamento + "," + getFormattedDateTime());
#else
          mqtt.publish("aysafi/mqtt/loraReceiver/Monjitas1/Tags", listaBlanca[x].id + "," + listaBlanca[x].apartamento + "," + getFormattedDateTime());
#endif
        }
        // Serial.println("Tag autorizado");
        registrarAcceso(tagPass, estacionamiento, true);
        digitalWrite(PORTON, HIGH);
        flagTagActivado = true;
        tiempoUltimoCambioTags = millis();
        tagAutorizado = true;
        break;
      }
    }
    if (!tagAutorizado && flagTagActivado == false)
    {
      String tramaLimpia = trama;
      tramaLimpia.trim();
      registrarAcceso(tramaLimpia, "", false);
    }
  }

  if (flagTagActivado == true || vehicularFlag == true)
  {
    if (millis() - tiempoUltimoCambioTags > 2000)
    {
      digitalWrite(PORTON, LOW);
      flagTagActivado = false;
      vehicularFlag = false;
    }
  }

#if LORA_SENDER == 0
  if (vehicularRemoteFlag == true && tiempoUltimoCambioTags > 2000)
  {
    digitalWrite(PORTON, LOW);
    vehicularRemoteFlag = false;
  }

  if (peatonalRemoteFlag == true && tiempoUltimoCambioTags > 2000)
  {
    digitalWrite(PUERTA, LOW);
    peatonalRemoteFlag = false;
  }
#endif

  if (peatonalFlag == true)
  {
    if (millis() - tiempoUltimoCambioTags > 2000)
    {
      digitalWrite(PUERTA, LOW);
      peatonalFlag = false;
    }
  }
#if LORA_SENDER == 1
  if (flagBoton == true)
  {
    if (millis() - tiempoUltimoCambioBotones > 2000)
    {
      flagBoton = false;
    }
  }
#endif

  if (flagTagActivado == true)
  {
    data = "Tag " + estacionamiento;
  }
  else if (vehicularFlag == true)
  {
    data = "mqtt vehicular";
  }
  else if (peatonalFlag == true)
  {
    data = "mqtt peatonal";
  }
#if LORA_SENDER == 1
  else if (flagBoton == true)
  {
    CRC32 crc;
    String message = idSlave + "," + data;
    crc.update(message.c_str(), message.length());
    uint32_t crcValue = crc.finalize();

    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.write((uint8_t *)&crcValue, sizeof(crcValue));
    LoRa.print("\n");
    LoRa.endPacket();
    data = String(comandoBoton, HEX) + " Code";
  }
#else
  else if (vehicularRemoteFlag == true)
  {
    data = "Remote vehicular";
  }
  else if (peatonalRemoteFlag == true)
  {
    data = "Remote peatonal";
  }
#endif
  else
  {
    data = "Ready";
  }

#endif
  pintarDatos();
#if LORA_SENDER == 1

  isrBotones();
  mqtt.publish("aysafi/esp32Lora/Monjitas/Sender", "act-" + String(millis() / 1000) + "s");
#else
  mqtt.publish("aysafi/esp32Lora/Monjitas/Recvr", "act-" + String(millis() / 1000) + "s");
#endif
  mqtt.update(); // should be called
  chequearBotonPortal();
  manejarPortalCautivo();

  if ( millis() - tiempoConsultaActualizaciones > 1800000)
  {
    tiempoConsultaActualizaciones = millis();
    //connect();
    esp_restart();
  }
}
