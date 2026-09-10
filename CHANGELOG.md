# Changelog — LoraSenderAysafi

Versión del canal OTA: `MAJOR.MINOR.PATCH` (semver numérico). El CI la inyecta
desde el tag `vX.Y.Z` (`FW_VERSION_OVERRIDE`); `currentVersion` de `board_def.h`
es el respaldo local.

## Sin publicar

### fix(ota): seguir las redirecciones de GitHub Releases

`getLatestVersion()` / `getLatestFirmwareSha256()` / `updateFirmware()` hacían
`http.begin(url)` sin `setFollowRedirects`: `releases/latest/download/*` responde
**302** al host de assets y el GET volvía vacío — la OTA **nunca descargaba**.
Ahora los tres usan `WiFiClientSecure::setInsecure()` +
`setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS)` + `setTimeout(15000)`.

> ⚠️ Flash al ~84 % del slot OTA de 1.25 MB (`default.csv`). El próximo
> `firmware.bin` casi no puede crecer. Para tener margen: un último flasheo por
> USB con `board_build.partitions = min_spiffs.csv` (slots de 1.9 MB).

## 1.0.23 y anteriores

- Sistema OTA "GitHub Releases pull": CI publica `firmware.bin` + `version.txt` +
  `firmware.sha256` como Release `latest` al empujar un tag `vX.Y.Z` sobre `main`;
  el equipo verifica el SHA-256 y flashea. Ver `README.md`.
- Portal cautivo: alta/baja de tags en tiempo real, log de accesos (CSV),
  configuración de red WiFi del router y contraseña de administrador.
