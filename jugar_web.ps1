param(
    [int]$Port = 8080
)

$distDir = Join-Path $PSScriptRoot "dist"
if (-not (Test-Path $distDir)) {
    Write-Error "No existe la carpeta 'dist'. Ejecuta primero build_web.bat."
    exit 1
}

$ips = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
    Where-Object { $_.IPAddress -ne "127.0.0.1" -and $_.PrefixOrigin -ne "WellKnown" } |
    Select-Object -ExpandProperty IPAddress

Write-Host "Servidor escuchando en 0.0.0.0:$Port" -ForegroundColor Green
Write-Host "  Local:   http://localhost:$Port" -ForegroundColor Cyan
foreach ($ip in $ips) {
    Write-Host "  Celular: http://${ip}:$Port" -ForegroundColor Yellow
}
Write-Host "Presiona Ctrl+C para detener.`n" -ForegroundColor DarkGray

Start-Process "http://localhost:$Port"
python -m http.server $Port --bind 0.0.0.0 -d $distDir
