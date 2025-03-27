# Verifica se está sendo executado como administrador
if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell.exe "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    exit
}

Write-Host "Desativando Windows Update (Executando como Administrador)..." -ForegroundColor Yellow

# Parar e desativar o serviço
try {
    Stop-Service -Name wuauserv -Force -ErrorAction Stop
    Set-Service -Name wuauserv -StartupType Disabled -ErrorAction Stop
    Write-Host "Serviço Windows Update desativado com sucesso" -ForegroundColor Green
}
catch {
    Write-Host "Falha ao desativar o serviço: $_" -ForegroundColor Red
}

# Desativar tarefas agendadas (com tratamento de erro)
try {
    $tasks = Get-ScheduledTask -TaskPath "\Microsoft\Windows\WindowsUpdate\" -ErrorAction Stop
    if ($tasks) {
        $tasks | Disable-ScheduledTask -ErrorAction Stop
        Write-Host "Tarefas agendadas desativadas com sucesso" -ForegroundColor Green
    }
}
catch {
    Write-Host "Falha ao desativar tarefas agendadas: $_" -ForegroundColor Red
}

# Modificar registro (parte principal)
try {
    $registryPath = "HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU"
    if (-not (Test-Path $registryPath)) {
        New-Item -Path $registryPath -Force | Out-Null
    }
    Set-ItemProperty -Path $registryPath -Name "NoAutoUpdate" -Value 1 -Type DWord -ErrorAction Stop
    Set-ItemProperty -Path $registryPath -Name "AUOptions" -Value 1 -Type DWord -ErrorAction Stop
    Write-Host "Registro modificado com sucesso" -ForegroundColor Green
}
catch {
    Write-Host "Falha ao modificar o registro: $_" -ForegroundColor Red
}

Write-Host "`nProcesso concluído. Verifique acima se todas as operações foram bem-sucedidas." -ForegroundColor Cyan
Write-Host "Reinicie o computador para aplicar todas as alterações." -ForegroundColor Yellow