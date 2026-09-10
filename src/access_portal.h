#pragma once
// Portal cautivo de administración de la lista blanca de tags RFID.
//
// Al mantener presionado IN3 (parada de emergencia) 5s, el equipo levanta un
// punto de acceso WiFi propio (SoftAP) manteniendo su conexión normal a la
// red del condominio (modo APSTA), y expone una pagina web en 192.168.4.1
// para:
//   - Agregar/eliminar tags RFID en tiempo real (persistidos en /tags.txt).
//   - Consultar el log de accesos (granted/denied) por tag.
//
// Un DNSServer responde cualquier dominio con la IP del portal para que los
// telefonos/laptops detecten automaticamente el "portal cautivo" al conectarse
// al BSSID que levanta el equipo, igual que un router de hotel/cafeteria.
//
// Requiere que board_def.h ya haya sido incluido (usa listaBlanca, countTags,
// maxTags, idUnico, display, saveTagsToFile, getFormattedDateTime).

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SD.h>

static DNSServer portalDns;
static WebServer portalWeb(80);
static bool portalActivo = false;
static const byte PORTAL_DNS_PORT = 53;
static const char *ACCESS_LOG_PATH = "/access_log.csv";
static const uint16_t ACCESS_LOG_MAX_LINES = 500;
static const uint32_t PORTAL_HOLD_MS = 5000;

// SSID/clave derivados del ID único del equipo: evita hardcodear una clave
// fija en el repo público; se muestran en el OLED al activar el portal.
String portalApSsid()
{
    String sufijo = idUnico.length() >= 4 ? idUnico.substring(idUnico.length() - 4) : idUnico;
    return "AYSAFI-Portal-" + sufijo;
}

String portalApPassword()
{
    String sufijo = idUnico.length() >= 6 ? idUnico.substring(idUnico.length() - 6) : ("000000" + idUnico);
    return "aysafi" + sufijo;
}

bool tagIdExiste(const String &id)
{
    for (uint8_t i = 0; i < countTags; i++)
    {
        if (listaBlanca[i].id == id)
        {
            return true;
        }
    }
    return false;
}

bool agregarTag(const String &id, const String &apartamento)
{
    if (id.length() == 0 || countTags >= maxTags || tagIdExiste(id))
    {
        return false;
    }
    listaBlanca[countTags].id = id;
    listaBlanca[countTags].apartamento = apartamento;
    listaBlanca[countTags].fecha = getFormattedDateTime();
    countTags++;
    saveTagsToFile("/tags.txt");
    return true;
}

bool eliminarTag(const String &id)
{
    for (uint8_t i = 0; i < countTags; i++)
    {
        if (listaBlanca[i].id == id)
        {
            for (uint8_t j = i; j < countTags - 1; j++)
            {
                listaBlanca[j] = listaBlanca[j + 1];
            }
            countTags--;
            saveTagsToFile("/tags.txt");
            return true;
        }
    }
    return false;
}

// Registra cada lectura de tag (autorizada o no) para trazabilidad de accesos.
void registrarAcceso(const String &id, const String &apartamento, bool concedido)
{
    if (SD.exists(ACCESS_LOG_PATH))
    {
        File f = SD.open(ACCESS_LOG_PATH, FILE_READ);
        uint16_t lines = 0;
        while (f.available())
        {
            if (f.read() == '\n')
            {
                lines++;
            }
        }
        f.close();
        // Recorta el log al superar el máximo, para no llenar la SD.
        if (lines > ACCESS_LOG_MAX_LINES)
        {
            SD.remove(ACCESS_LOG_PATH);
        }
    }

    File f = SD.open(ACCESS_LOG_PATH, FILE_APPEND);
    if (!f)
    {
        return;
    }
    f.println(getFormattedDateTime() + "," + id + "," + apartamento + "," + (concedido ? "GRANTED" : "DENIED"));
    f.close();
}

static String htmlEscapar(const String &s)
{
    String out = s;
    out.replace("&", "&amp;");
    out.replace("<", "&lt;");
    out.replace(">", "&gt;");
    out.replace("\"", "&quot;");
    return out;
}

static String portalEstilo()
{
    return F("<style>"
              "body{font-family:sans-serif;margin:0;background:#111;color:#eee}"
              "main{max-width:560px;margin:auto;padding:16px}"
              "h1{font-size:19px}h2{font-size:15px;color:#8cf;margin:18px 0 6px}"
              "table{width:100%;border-collapse:collapse;font-size:13px}"
              "td,th{border:1px solid #444;padding:4px}"
              "input{padding:8px;box-sizing:border-box;background:#222;color:#eee;"
              "border:1px solid #555;border-radius:6px}"
              "button{padding:8px 12px;font-size:14px;background:#08f;color:#fff;border:0;border-radius:6px}"
              ".del{background:#a33}a{color:#8cf}</style>");
}

static void portalHandleRoot()
{
    String h;
    h.reserve(4096);
    h += F("<!doctype html><html><head><meta charset=utf-8>"
           "<meta name=viewport content='width=device-width,initial-scale=1'>"
           "<title>AYSAFI - Portal de acceso</title>");
    h += portalEstilo();
    h += F("</head><body><main><h1>AYSAFI &mdash; Lista blanca de tags</h1>");

    h += F("<h2>Agregar tag</h2><form method=post action=/add>"
           "<label>ID del tag</label><input name=id required maxlength=32>"
           "<label>Apartamento / nombre</label><input name=apartamento maxlength=32>"
           "<button style='width:100%;margin-top:8px'>Agregar</button></form>");

    h += "<h2>Tags registrados (" + String(countTags) + "/" + String(maxTags) + ")</h2>";
    h += F("<table><tr><th>ID</th><th>Apartamento</th><th>Alta</th><th></th></tr>");
    for (uint8_t i = 0; i < countTags; i++)
    {
        h += "<tr><td>" + htmlEscapar(listaBlanca[i].id) + "</td><td>" +
             htmlEscapar(listaBlanca[i].apartamento) + "</td><td>" +
             htmlEscapar(listaBlanca[i].fecha) + "</td><td>"
             "<form method=post action=/delete style=display:inline>"
             "<input type=hidden name=id value='" + htmlEscapar(listaBlanca[i].id) + "'>"
             "<button class=del>Eliminar</button></form></td></tr>";
    }
    h += F("</table><p><a href=/log>Ver log de accesos &rarr;</a></p></main></body></html>");

    portalWeb.send(200, "text/html", h);
}

static void portalHandleAdd()
{
    String id = portalWeb.arg("id");
    id.trim();
    String apartamento = portalWeb.arg("apartamento");
    apartamento.trim();
    agregarTag(id, apartamento);
    portalWeb.sendHeader("Location", "/");
    portalWeb.send(303);
}

static void portalHandleDelete()
{
    String id = portalWeb.arg("id");
    eliminarTag(id);
    portalWeb.sendHeader("Location", "/");
    portalWeb.send(303);
}

static void portalHandleLog()
{
    String h;
    h.reserve(4096);
    h += F("<!doctype html><html><head><meta charset=utf-8>"
           "<meta name=viewport content='width=device-width,initial-scale=1'>"
           "<title>AYSAFI - Log de accesos</title>");
    h += portalEstilo();
    h += F("</head><body><main><h1>Log de accesos</h1>"
           "<table><tr><th>Fecha/hora</th><th>ID</th><th>Apartamento</th><th>Resultado</th></tr>");

    if (SD.exists(ACCESS_LOG_PATH))
    {
        File f = SD.open(ACCESS_LOG_PATH, FILE_READ);
        while (f.available())
        {
            String linea = f.readStringUntil('\n');
            linea.trim();
            if (linea.length() == 0)
            {
                continue;
            }
            int c1 = linea.indexOf(',');
            int c2 = linea.indexOf(',', c1 + 1);
            int c3 = linea.lastIndexOf(',');
            if (c1 < 0 || c2 < 0 || c3 < 0)
            {
                continue;
            }
            String fecha = linea.substring(0, c1);
            String id = linea.substring(c1 + 1, c2);
            String apto = linea.substring(c2 + 1, c3);
            String resultado = linea.substring(c3 + 1);
            String claseFila = resultado == "GRANTED" ? "" : " style='color:#f88'";
            h += "<tr" + claseFila + "><td>" + htmlEscapar(fecha) + "</td><td>" + htmlEscapar(id) +
                 "</td><td>" + htmlEscapar(apto) + "</td><td>" + htmlEscapar(resultado) + "</td></tr>";
        }
        f.close();
    }

    h += F("</table><p><a href=/>&larr; Volver</a></p></main></body></html>");
    portalWeb.send(200, "text/html", h);
}

// Cualquier URL desconocida (o de deteccion de portal cautivo de iOS/Android/
// Windows) redirige a la pagina principal, para que el sistema operativo
// muestre automaticamente el aviso de "iniciar sesion en esta red".
static void portalHandleCaptive()
{
    portalWeb.sendHeader("Location", "http://192.168.4.1/", true);
    portalWeb.send(302, "text/plain", "");
}

void iniciarPortalCautivo()
{
    if (portalActivo)
    {
        return;
    }
    portalActivo = true;

    WiFi.mode(WIFI_MODE_APSTA); // conserva la conexión STA (MQTT/OTA) mientras el portal está activo
    IPAddress apIp(192, 168, 4, 1);
    WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
    String ssid = portalApSsid();
    String pass = portalApPassword();
    WiFi.softAP(ssid.c_str(), pass.c_str());

    portalDns.start(PORTAL_DNS_PORT, "*", apIp);

    portalWeb.on("/", HTTP_GET, portalHandleRoot);
    portalWeb.on("/add", HTTP_POST, portalHandleAdd);
    portalWeb.on("/delete", HTTP_POST, portalHandleDelete);
    portalWeb.on("/log", HTTP_GET, portalHandleLog);
    portalWeb.on("/generate_204", portalHandleCaptive);  // Android
    portalWeb.on("/gen_204", portalHandleCaptive);        // Android
    portalWeb.on("/hotspot-detect.html", portalHandleCaptive); // iOS/macOS
    portalWeb.on("/ncsi.txt", portalHandleCaptive);       // Windows
    portalWeb.on("/fwlink", portalHandleCaptive);         // Windows
    portalWeb.onNotFound(portalHandleCaptive);
    portalWeb.begin();

    Serial.println("Portal cautivo activo: " + ssid);

    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(display.getWidth() / 2, 0, "Portal de administracion");
    display.drawString(display.getWidth() / 2, 16, "SSID: " + ssid);
    display.drawString(display.getWidth() / 2, 32, "Clave: " + pass);
    display.drawString(display.getWidth() / 2, 48, "IP: 192.168.4.1");
    display.display();
}

void detenerPortalCautivo()
{
    if (!portalActivo)
    {
        return;
    }
    portalWeb.close();
    portalDns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_MODE_STA);
    portalActivo = false;
    Serial.println("Portal cautivo detenido");
}

void manejarPortalCautivo()
{
    if (!portalActivo)
    {
        return;
    }
    portalDns.processNextRequest();
    portalWeb.handleClient();
}

// Mantener IN3 presionado 5s alterna el portal (independiente del uso normal
// de IN3 como parada de emergencia, que reacciona a la pulsación corta).
void chequearBotonPortal()
{
    static uint32_t desde = 0;
    static bool activado = false;

    if (digitalRead(IN3) == LOW)
    {
        if (desde == 0)
        {
            desde = millis();
        }
        else if (!activado && millis() - desde > PORTAL_HOLD_MS)
        {
            activado = true;
            if (portalActivo)
            {
                detenerPortalCautivo();
            }
            else
            {
                iniciarPortalCautivo();
            }
        }
    }
    else
    {
        desde = 0;
        activado = false;
    }
}
