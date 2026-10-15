param(
    [ValidateSet("build", "flash", "monitor", "erase-flash")]
    [string]$Action = "build",
    [string]$Port = "COM7"
)

$ErrorActionPreference = "Stop"
$FirmwareDir = $PSScriptRoot

if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    throw "idf.py ist nicht im PATH. Bitte zuerst das ESP-IDF-PowerShell-Terminal (ESP-IDF 5.5.x) öffnen und dieses Skript erneut starten."
}

$env:IDF_TARGET = "esp32p4"
$env:SDKCONFIG_DEFAULTS = Join-Path $FirmwareDir "sdkconfig.p4.defaults"
$BuildDir = Join-Path $FirmwareDir "build-esp32p4"

Push-Location $FirmwareDir
try {
    if (-not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))) {
        idf.py -B $BuildDir set-target esp32p4
        if ($LASTEXITCODE -ne 0) { throw "idf.py set-target ist fehlgeschlagen ($LASTEXITCODE)." }
    }

    switch ($Action) {
        "build" { idf.py -B $BuildDir build }
        "flash" { idf.py -B $BuildDir -p $Port flash }
        "monitor" { idf.py -B $BuildDir -p $Port monitor }
        "erase-flash" { idf.py -B $BuildDir -p $Port erase-flash }
    }
    if ($LASTEXITCODE -ne 0) { throw "idf.py $Action ist fehlgeschlagen ($LASTEXITCODE)." }
}
finally {
    Pop-Location
}
