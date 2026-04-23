$ErrorActionPreference = 'Stop'

$projectRoot = 'C:\Users\parke\Documents\Codex\TopDownDarryl\topdown_cpp_shooter'
$godotExe = 'C:\Users\parke\OneDrive\Desktop\Godot\Godot_v4.6.2-stable_win64_console.exe'
$ffmpegExe = 'C:\Program Files\SteelSeries\GG\apps\moments\ffmpeg.exe'
$artifactsDir = Join-Path $projectRoot 'artifacts'
$framesDir = Join-Path $artifactsDir 'quadtree_frames'
$outputMp4 = Join-Path $artifactsDir 'quadtree_demo.mp4'

New-Item -ItemType Directory -Force -Path $artifactsDir | Out-Null
if (Test-Path $framesDir) {
    Remove-Item -Recurse -Force -LiteralPath $framesDir
}
New-Item -ItemType Directory -Force -Path $framesDir | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

Add-Type @"
using System;
using System.Runtime.InteropServices;

public static class Win32Capture {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int X, int Y);

    [DllImport("user32.dll")]
    public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);

    [DllImport("user32.dll")]
    public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
}
"@

$keyDownFlag = 0x0000
$keyUpFlag = 0x0002
$mouseLeftDown = 0x0002
$mouseLeftUp = 0x0004
$vkW = 0x57
$vkA = 0x41
$vkS = 0x53
$vkD = 0x44
$vkR = 0x52

function Invoke-KeyTap {
    param([byte]$VirtualKey)

    [Win32Capture]::keybd_event($VirtualKey, 0, $keyDownFlag, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [Win32Capture]::keybd_event($VirtualKey, 0, $keyUpFlag, [UIntPtr]::Zero)
}

function Set-KeyState {
    param(
        [byte]$VirtualKey,
        [bool]$Pressed
    )

    if ($Pressed) {
        [Win32Capture]::keybd_event($VirtualKey, 0, $keyDownFlag, [UIntPtr]::Zero)
    } else {
        [Win32Capture]::keybd_event($VirtualKey, 0, $keyUpFlag, [UIntPtr]::Zero)
    }
}

function Invoke-LeftClick {
    param(
        [int]$X,
        [int]$Y
    )

    [Win32Capture]::SetCursorPos($X, $Y) | Out-Null
    Start-Sleep -Milliseconds 60
    [Win32Capture]::mouse_event($mouseLeftDown, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 40
    [Win32Capture]::mouse_event($mouseLeftUp, 0, 0, 0, [UIntPtr]::Zero)
}

$process = Start-Process -FilePath $godotExe -ArgumentList '--path', $projectRoot -PassThru

try {
    $windowHandle = [IntPtr]::Zero
    for ($attempt = 0; $attempt -lt 120; $attempt++) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()
        if ($process.MainWindowHandle -ne 0) {
            $windowHandle = [IntPtr]$process.MainWindowHandle
            break
        }
    }

    if ($windowHandle -eq [IntPtr]::Zero) {
        throw 'Godot window did not appear in time.'
    }

    [Win32Capture]::ShowWindow($windowHandle, 9) | Out-Null
    [Win32Capture]::SetForegroundWindow($windowHandle) | Out-Null
    Start-Sleep -Seconds 2

    $rect = New-Object Win32Capture+RECT
    if (-not [Win32Capture]::GetWindowRect($windowHandle, [ref]$rect)) {
        throw 'Unable to get the game window bounds.'
    }

    $captureX = $rect.Left
    $captureY = $rect.Top
    $captureWidth = [Math]::Max(1, $rect.Right - $rect.Left)
    $captureHeight = [Math]::Max(1, $rect.Bottom - $rect.Top)

    $centerX = $captureX + [int]($captureWidth / 2)
    $centerY = $captureY + [int]($captureHeight / 2)
    $aimRightX = $centerX + [int]($captureWidth * 0.28)
    $aimLeftX = $centerX - [int]($captureWidth * 0.22)
    $aimTopY = $centerY - [int]($captureHeight * 0.20)
    $aimBottomY = $centerY + [int]($captureHeight * 0.18)

    $fps = 12
    $durationSeconds = 16
    $totalFrames = $fps * $durationSeconds
    $frameIntervalMs = [int](1000 / $fps)
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

    $events = @(
        @{ Time = 0.4; Action = { Set-KeyState -VirtualKey $vkD -Pressed $true; Set-KeyState -VirtualKey $vkW -Pressed $true } },
        @{ Time = 2.4; Action = { Set-KeyState -VirtualKey $vkW -Pressed $false } },
        @{ Time = 3.0; Action = { Invoke-LeftClick -X $aimRightX -Y $aimTopY } },
        @{ Time = 3.6; Action = { Invoke-LeftClick -X ($centerX + 110) -Y $centerY } },
        @{ Time = 4.2; Action = { Invoke-LeftClick -X $aimRightX -Y $aimBottomY } },
        @{ Time = 4.8; Action = { Set-KeyState -VirtualKey $vkD -Pressed $false } },
        @{ Time = 5.2; Action = { Invoke-KeyTap -VirtualKey $vkR } },
        @{ Time = 6.2; Action = { Set-KeyState -VirtualKey $vkA -Pressed $true; Set-KeyState -VirtualKey $vkS -Pressed $true } },
        @{ Time = 8.4; Action = { Set-KeyState -VirtualKey $vkS -Pressed $false } },
        @{ Time = 9.0; Action = { Invoke-LeftClick -X $aimLeftX -Y $centerY } },
        @{ Time = 10.0; Action = { Set-KeyState -VirtualKey $vkA -Pressed $false; Set-KeyState -VirtualKey $vkD -Pressed $true } },
        @{ Time = 11.3; Action = { Invoke-LeftClick -X $aimRightX -Y $centerY } },
        @{ Time = 12.2; Action = { Set-KeyState -VirtualKey $vkD -Pressed $false; Set-KeyState -VirtualKey $vkW -Pressed $true } },
        @{ Time = 13.7; Action = { Set-KeyState -VirtualKey $vkW -Pressed $false } }
    )

    $eventIndex = 0

    for ($frame = 0; $frame -lt $totalFrames; $frame++) {
        while ($eventIndex -lt $events.Count -and $stopwatch.Elapsed.TotalSeconds -ge $events[$eventIndex].Time) {
            & $events[$eventIndex].Action
            $eventIndex += 1
        }

        $bitmap = New-Object System.Drawing.Bitmap $captureWidth, $captureHeight
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.CopyFromScreen($captureX, $captureY, 0, 0, $bitmap.Size)
            $framePath = Join-Path $framesDir ("frame_{0:D4}.png" -f $frame)
            $bitmap.Save($framePath, [System.Drawing.Imaging.ImageFormat]::Png)
        } finally {
            $graphics.Dispose()
            $bitmap.Dispose()
        }

        $targetMs = ($frame + 1) * $frameIntervalMs
        $remainingMs = $targetMs - $stopwatch.ElapsedMilliseconds
        if ($remainingMs -gt 0) {
            Start-Sleep -Milliseconds $remainingMs
        }
    }

    Set-KeyState -VirtualKey $vkW -Pressed $false
    Set-KeyState -VirtualKey $vkA -Pressed $false
    Set-KeyState -VirtualKey $vkS -Pressed $false
    Set-KeyState -VirtualKey $vkD -Pressed $false

    & $ffmpegExe -hide_banner -y -framerate $fps -i (Join-Path $framesDir 'frame_%04d.png') -c:v libx264 -pix_fmt yuv420p $outputMp4 | Out-Null
} finally {
    Set-KeyState -VirtualKey $vkW -Pressed $false
    Set-KeyState -VirtualKey $vkA -Pressed $false
    Set-KeyState -VirtualKey $vkS -Pressed $false
    Set-KeyState -VirtualKey $vkD -Pressed $false

    if ($process -and -not $process.HasExited) {
        $null = $process.CloseMainWindow()
        Start-Sleep -Seconds 2
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force
        }
    }
}

Write-Output $outputMp4
