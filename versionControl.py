"""Herramienta de release para LoraSenderAysafi.

Reemplaza el antiguo flujo FTP (versionControl.py) por el flujo GitHub:
1. Incrementa `currentVersion` en src/board_def.h.
2. Commitea el cambio en main.
3. Crea un tag anotado vX.Y.Z sobre ese commit.
4. Hace push del commit y del tag a origin.

El push del tag es la señal de "firmware aprobado, listo para publicar":
dispara el workflow .github/workflows/release.yml, que compila y publica
el binario como GitHub Release "latest". Los dispositivos ya en campo lo
recogen automáticamente en su siguiente reinicio (chequearActualizaciones()).

Uso:
    python versionControl.py [--part major|minor|patch]  (default: patch)

Requiere estar parado sobre `main`, sin cambios sin commitear, y tener
`origin` configurado con permisos de push.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent
BOARD_DEF = PROJECT_ROOT / "src" / "board_def.h"


def run(*cmd, check=True):
    result = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True, text=True)
    if check and result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        sys.exit(f"Comando falló: {' '.join(cmd)}")
    return result.stdout.strip()


def leer_version_actual():
    contenido = BOARD_DEF.read_text(encoding="utf-8")
    match = re.search(r'currentVersion\s*=\s*"([^"]+)"', contenido)
    if not match:
        sys.exit("No se encontró currentVersion en board_def.h")
    return match.group(1), contenido


def incrementar_version(version_actual, parte):
    major, minor, patch = (int(n) for n in version_actual.split("."))
    if parte == "major":
        major, minor, patch = major + 1, 0, 0
    elif parte == "minor":
        minor, patch = minor + 1, 0
    else:
        patch += 1
    return f"{major}.{minor}.{patch}"


def actualizar_version_board_def(version_actual, nueva_version, contenido):
    nuevo_contenido = contenido.replace(
        f'currentVersion = "{version_actual}"', f'currentVersion = "{nueva_version}"'
    )
    BOARD_DEF.write_text(nuevo_contenido, encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--part", choices=["major", "minor", "patch"], default="patch")
    args = parser.parse_args()

    rama_actual = run("git", "rev-parse", "--abbrev-ref", "HEAD")
    if rama_actual != "main":
        sys.exit(f"Debes estar en 'main' para crear un release (rama actual: {rama_actual})")

    estado = run("git", "status", "--porcelain")
    if estado:
        sys.exit("Hay cambios sin commitear. Commitea o descarta antes de hacer un release.")

    version_actual, contenido = leer_version_actual()
    nueva_version = incrementar_version(version_actual, args.part)
    actualizar_version_board_def(version_actual, nueva_version, contenido)

    tag = f"v{nueva_version}"
    run("git", "add", str(BOARD_DEF.relative_to(PROJECT_ROOT)))
    run("git", "commit", "-m", f"release: bump version to {nueva_version}")
    run("git", "tag", "-a", tag, "-m", f"Firmware {nueva_version}")
    run("git", "push", "origin", "main")
    run("git", "push", "origin", tag)

    print(f"Release {tag} publicado. GitHub Actions compilará y publicará el firmware.")


if __name__ == "__main__":
    main()
