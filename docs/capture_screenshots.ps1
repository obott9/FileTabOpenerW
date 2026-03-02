# FileTabOpenerW screenshot capture script
# Takes 10 screenshots: 5 languages x 2 layouts (Compact/Sidebar)
# Usage: powershell -ExecutionPolicy Bypass -File docs\capture_screenshots.ps1

param(
    [string]$ExePath = "$PSScriptRoot\..\build\Release\FileTabOpenerW.exe",
    [string]$OutputDir = "$PSScriptRoot\images",
    [string]$ConfigDir = "$env:APPDATA\FileTabOpenerW"
)

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$pinvoke = @'
[DllImport("user32.dll")]
public static extern bool MoveWindow(IntPtr hwnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

[DllImport("user32.dll")]
public static extern bool SetForegroundWindow(IntPtr hwnd);
'@

$Win32 = Add-Type -MemberDefinition $pinvoke -Name 'Win32Funcs' -Namespace 'CaptureNS' -PassThru -ErrorAction Stop

# Window dimensions for MoveWindow
$WinX = 200; $WinY = 100; $WinW = 900; $WinH = 680

function New-DemoConfig {
    param([string]$Lang, [string]$Layout, [string[]]$GroupNames)
    $paths = @("C:\Users\user\Documents", "C:\Users\user\Downloads", "C:\Users\user\Desktop")
    $groups = @()
    foreach ($name in $GroupNames) {
        $groups += @{
            name = $name
            paths = $paths
            window_x = $null; window_y = $null
            window_width = $null; window_height = $null
        }
    }
    $cfg = @{
        settings = @{ language = $Lang; layout = $Layout; timeout = 60 }
        tab_groups = $groups
        history = @(
            @{ path = "C:\Users\user\Documents"; pinned = $true; use_count = 5; last_used = "2026-03-01T10:00:00" }
            @{ path = "C:\Users\user\Downloads"; pinned = $false; use_count = 3; last_used = "2026-03-01T09:00:00" }
            @{ path = "C:\Users\user\Desktop"; pinned = $false; use_count = 2; last_used = "2026-03-01T08:00:00" }
        )
    }
    return ($cfg | ConvertTo-Json -Depth 10)
}

function Capture-AtPosition {
    param([string]$OutputPath)
    # CopyFromScreen at known position (MoveWindow coords + DWM shadow margin ~7px)
    $captX = $WinX - 7
    $captY = $WinY
    $captW = $WinW + 14
    $captH = $WinH + 7
    $bmp = New-Object System.Drawing.Bitmap($captW, $captH)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($captX, $captY, 0, 0, (New-Object System.Drawing.Size($captW, $captH)))
    $g.Dispose()
    $bmp.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    $kb = [math]::Round((Get-Item $OutputPath).Length / 1024, 1)
    Write-Host "  Saved: $(Split-Path $OutputPath -Leaf) ($kb KB)"
}

# Language configs using char arrays for CJK
$langData = @(
    @{ code = "en";    names = @("Documents", "Development", "Downloads") }
    @{ code = "ja";    names = @(
        ([char]0x66F8, [char]0x985E -join ''),       # shoruui
        ([char]0x958B, [char]0x767A -join ''),       # kaihatsu
        ([char]0x30C0, [char]0x30A6, [char]0x30F3, [char]0x30ED, [char]0x30FC, [char]0x30C9 -join ''))  # daunroodo
    }
    @{ code = "ko";    names = @(
        ([char]0xBB38, [char]0xC11C -join ''),
        ([char]0xAC1C, [char]0xBC1C -join ''),
        ([char]0xB2E4, [char]0xC6B4, [char]0xB85C, [char]0xB4DC -join ''))
    }
    @{ code = "zh_TW"; names = @(
        ([char]0x6587, [char]0x4EF6 -join ''),
        ([char]0x958B, [char]0x767C -join ''),
        ([char]0x4E0B, [char]0x8F09 -join ''))
    }
    @{ code = "zh_CN"; names = @(
        ([char]0x6587, [char]0x6863 -join ''),
        ([char]0x5F00, [char]0x53D1 -join ''),
        ([char]0x4E0B, [char]0x8F7D -join ''))
    }
)

$layouts = @("compact", "sidebar")

Write-Host "=== FileTabOpenerW Screenshot Capture ==="
Write-Host "Exe: $ExePath"
Write-Host ""

if (-not (Test-Path $ExePath)) {
    Write-Error "Exe not found: $ExePath"
    exit 1
}

New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

# Backup config
$configPath = Join-Path $ConfigDir "config.json"
$backupPath = Join-Path $ConfigDir "config.json.bak"
if ((Test-Path $configPath) -and -not (Test-Path $backupPath)) {
    Copy-Item $configPath $backupPath
    Write-Host "Backed up config"
}

$total = $langData.Count * $layouts.Count
$current = 0

foreach ($ld in $langData) {
    $lang = $ld.code
    $groupNames = $ld.names
    foreach ($layout in $layouts) {
        $current++
        $outFile = Join-Path $OutputDir "${lang}_${layout}.png"
        Write-Host "[$current/$total] Capturing: $lang / $layout ..."

        # Write demo config
        $json = New-DemoConfig -Lang $lang -Layout $layout -GroupNames $groupNames
        [System.IO.File]::WriteAllText($configPath, $json, [System.Text.Encoding]::UTF8)

        # Launch
        $proc = Start-Process -FilePath $ExePath -PassThru
        Start-Sleep -Seconds 2
        $proc.Refresh()
        $hwnd = $proc.MainWindowHandle

        if ($hwnd -eq [IntPtr]::Zero) {
            Start-Sleep -Seconds 2
            $proc.Refresh()
            $hwnd = $proc.MainWindowHandle
        }

        if ($hwnd -eq [IntPtr]::Zero) {
            Write-Warning "  Could not find window handle"
            Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
            continue
        }

        # Position and bring to foreground
        $Win32::MoveWindow($hwnd, $WinX, $WinY, $WinW, $WinH, $true) | Out-Null
        Start-Sleep -Milliseconds 500
        $Win32::SetForegroundWindow($hwnd) | Out-Null
        Start-Sleep -Milliseconds 500

        # Capture
        Capture-AtPosition -OutputPath $outFile

        # Close
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 500
    }
}

# Restore config
if (Test-Path $backupPath) {
    Copy-Item $backupPath $configPath -Force
    Remove-Item $backupPath
    Write-Host ""
    Write-Host "Restored original config"
}

Write-Host ""
Write-Host "=== Done! ==="
$files = Get-ChildItem $OutputDir -Filter "*.png" | Where-Object { $_.Name -ne "test_capture.png" }
$files | ForEach-Object { Write-Host "  $($_.Name) ($([math]::Round($_.Length/1024, 1)) KB)" }
Write-Host "Total: $($files.Count) screenshots"
