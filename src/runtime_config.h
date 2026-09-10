#pragma once
// Configuración persistente en NVS (Preferences) para parámetros que hoy son
// build_flags de fábrica pero que un administrador puede necesitar cambiar
// en campo sin recompilar: red WiFi del router y contraseña del portal.
//
// Requiere que board_def.h ya haya sido incluido (usa las macros WIFI_SSID/
// WIFI_PASSWORD como valores de fábrica, e idUnico como semilla del admin
// password por defecto).

#include <Preferences.h>

static Preferences configPrefs;
static const char *CONFIG_NAMESPACE = "aysafi";

String cfgWifiSsid()
{
    configPrefs.begin(CONFIG_NAMESPACE, true);
    String v = configPrefs.getString("wifi_ssid", WIFI_SSID);
    configPrefs.end();
    return v;
}

String cfgWifiPassword()
{
    configPrefs.begin(CONFIG_NAMESPACE, true);
    String v = configPrefs.getString("wifi_pass", WIFI_PASSWORD);
    configPrefs.end();
    return v;
}

void cfgSetWifi(const String &ssid, const String &pass)
{
    configPrefs.begin(CONFIG_NAMESPACE, false);
    configPrefs.putString("wifi_ssid", ssid);
    configPrefs.putString("wifi_pass", pass);
    configPrefs.end();
}

// Password de administrador por defecto: derivado del ID único del equipo
// (igual que la clave del AP), para no fijar una clave adivinable en el repo.
String cfgAdminPasswordDefault()
{
    String sufijo = idUnico.length() >= 6 ? idUnico.substring(idUnico.length() - 6) : ("000000" + idUnico);
    return "admin" + sufijo;
}

String cfgAdminPassword()
{
    configPrefs.begin(CONFIG_NAMESPACE, true);
    String v = configPrefs.getString("admin_pass", "");
    configPrefs.end();
    return v.length() > 0 ? v : cfgAdminPasswordDefault();
}

void cfgSetAdminPassword(const String &pass)
{
    configPrefs.begin(CONFIG_NAMESPACE, false);
    configPrefs.putString("admin_pass", pass);
    configPrefs.end();
}
