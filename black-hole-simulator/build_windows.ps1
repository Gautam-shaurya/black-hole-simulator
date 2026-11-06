# One-click builder for Black Hole Simulator (Windows 10/11, 64-bit).
# Downloads: (1) w64devkit = a portable free C compiler, (2) the official GLFW
# Windows binaries. Nothing is installed system-wide; everything stays in this folder.
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'      # makes downloads much faster
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root
$log = Join-Path $root 'build_error.log'
$tools = Join-Path $root 'build_tools'
$glfwDir = Join-Path $root 'third_party\glfw'
$exe = Join-Path $root 'BlackHoleSimulator.exe'
New-Item -ItemType Directory -Force $tools | Out-Null
$manualDir = Join-Path $root 'downloads_here'
New-Item -ItemType Directory -Force $manualDir | Out-Null
try {   # use the Windows proxy settings (work / school networks)
    [Net.WebRequest]::DefaultWebProxy = [Net.WebRequest]::GetSystemWebProxy()
    [Net.WebRequest]::DefaultWebProxy.Credentials = [Net.CredentialCache]::DefaultCredentials
} catch { }

function Say($text) { Write-Host $text -ForegroundColor Cyan }
function Download($url, $file) {
    Write-Host "   downloading $url"
    Invoke-WebRequest -Uri $url -OutFile $file -UseBasicParsing -Headers @{ 'User-Agent' = 'BlackHoleSimulatorBuild' }
}

# Looks in the 'downloads_here' folder for a file the user downloaded by hand.
function Find-Manual($include, $exclude) {
    Get-ChildItem -Path $manualDir -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like $include -and -not ($exclude -and $_.Name -match $exclude) } |
        Select-Object -First 1
}

try {
    if (-not [Environment]::Is64BitOperatingSystem) { throw 'This build needs 64-bit Windows.' }
    foreach ($f in 'src\main.c', 'shaders\blackhole.frag') {
        if (-not (Test-Path (Join-Path $root $f))) {
            throw "The file '$f' is missing. Please extract the WHOLE zip file first (right-click the zip > Extract All), then run this again from the extracted folder."
        }
    }

    # ---- 1. compiler -------------------------------------------------------
    Say '[1/4] Getting the free C compiler (one time only)...'
    $gcc = Get-ChildItem -Path $tools -Recurse -Filter gcc.exe -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $gcc) {
        $file = $null
        $mf = Find-Manual 'w64devkit*' 'fortran|i686|\.7z$'
        if ($mf -and ($mf.Extension -eq '.zip' -or $mf.Extension -eq '.exe')) {
            Say "      using the file you downloaded: $($mf.Name)"
            $file = $mf.FullName
            $keep = $true
        } else {
            try {
                $rel = Invoke-RestMethod -Uri 'https://api.github.com/repos/skeeto/w64devkit/releases/latest' -Headers @{ 'User-Agent' = 'BlackHoleSimulatorBuild' }
                $zipAsset = $rel.assets | Where-Object { $_.name -match '^w64devkit-(x64-)?[0-9.]+\.zip$' } | Select-Object -First 1
                $sfxAsset = $rel.assets | Where-Object { $_.name -match '^w64devkit-x64-[0-9.]+(\.7z)?\.exe$' } | Select-Object -First 1
                $asset = if ($zipAsset) { $zipAsset } else { $sfxAsset }
                if (-not $asset) { throw 'no suitable download found' }
                $file = Join-Path $tools $asset.name
                Download $asset.browser_download_url $file
            } catch {
                throw ("STEP 1 FAILED: could not download the compiler (" + $_.Exception.Message + ").`r`n" +
                       "Your network, firewall, VPN or antivirus is probably blocking PowerShell.`r`n" +
                       "EASY FIX - download it with your web browser instead:`r`n" +
                       "  1. Open  https://github.com/skeeto/w64devkit/releases/latest`r`n" +
                       "  2. Under Assets click the file named like  w64devkit-x64-2.x.x.exe  (or a .zip without 'fortran' or 'i686')`r`n" +
                       "  3. Move that file into this folder:  $manualDir`r`n" +
                       "  4. Run build_windows.bat again.")
            }
        }
        Unblock-File -Path $file -ErrorAction SilentlyContinue
        Say '      unpacking (this can take a minute)...'
        if ($file -like '*.zip') { Expand-Archive -Force -Path $file -DestinationPath $tools }
        else { & $file -y "-o$tools" | Out-Null }
        if (-not $keep) { Remove-Item -Force $file -ErrorAction SilentlyContinue }
        $gcc = Get-ChildItem -Path $tools -Recurse -Filter gcc.exe -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $gcc) { throw 'The compiler was downloaded but gcc.exe was not found after unpacking.' }
    }
    $env:PATH = $gcc.DirectoryName + ';' + $env:PATH
    Say "      compiler ready: $($gcc.FullName)"

    # ---- 2. GLFW -----------------------------------------------------------
    Say '[2/4] Getting GLFW (the window library, official release)...'
    if (-not ((Test-Path (Join-Path $glfwDir 'include\GLFW\glfw3.h')) -and (Test-Path (Join-Path $glfwDir 'lib\libglfw3.a')))) {
        $zip = $null
        $mg = Find-Manual 'glfw*WIN64*.zip' $null
        if ($mg) {
            Say "      using the file you downloaded: $($mg.Name)"
            $zip = $mg.FullName
        } else {
            foreach ($v in '3.4', '3.3.9') {
                try {
                    $zip = Join-Path $tools "glfw-$v.zip"
                    Download "https://github.com/glfw/glfw/releases/download/$v/glfw-$v.bin.WIN64.zip" $zip
                    break
                } catch { $zip = $null; Write-Host "   (GLFW $v failed: $($_.Exception.Message))" -ForegroundColor Yellow }
            }
        }
        if (-not $zip) {
            throw ("STEP 2 FAILED: could not download GLFW.`r`n" +
                   "EASY FIX - download it with your web browser instead:`r`n" +
                   "  1. Open  https://www.glfw.org/download.html`r`n" +
                   "  2. Click  '64-bit Windows binaries'  (a file named like glfw-3.4.bin.WIN64.zip)`r`n" +
                   "  3. Move that file into this folder:  $manualDir`r`n" +
                   "  4. Run build_windows.bat again.")
        }
        Unblock-File -Path $zip -ErrorAction SilentlyContinue
        $tmp = Join-Path $tools 'glfw-unpacked'
        if (Test-Path $tmp) { Remove-Item -Recurse -Force $tmp }
        Expand-Archive -Force -Path $zip -DestinationPath $tmp
        $inner = Get-ChildItem $tmp -Directory | Select-Object -First 1
        New-Item -ItemType Directory -Force (Join-Path $glfwDir 'lib') | Out-Null
        Copy-Item -Recurse -Force (Join-Path $inner.FullName 'include') $glfwDir
        Copy-Item -Force (Join-Path $inner.FullName 'lib-mingw-w64\libglfw3.a') (Join-Path $glfwDir 'lib')
        Remove-Item -Recurse -Force $tmp
        if ($zip -like "$tools*") { Remove-Item -Force $zip -ErrorAction SilentlyContinue }
    }
    Say '      GLFW ready.'

    # ---- 3. compile --------------------------------------------------------
    Say '[3/4] Building BlackHoleSimulator.exe (about 30 seconds)...'
    if (Test-Path $exe) { Remove-Item -Force $exe }
    $sources = Get-ChildItem (Join-Path $root 'src') -Filter *.c | ForEach-Object { $_.FullName }
    $gccArgs = @('-std=gnu99', '-O2', '-s',
                 "-I$root\include", "-I$glfwDir\include") + $sources +
               @('-o', $exe, "-L$glfwDir\lib", '-lglfw3', '-lgdi32', '-luser32', '-lshell32', '-mwindows', '-static')
    $ErrorActionPreference = 'Continue'     # compiler warnings go to stderr; do not treat them as failures
    & $gcc.FullName @gccArgs 2>&1 | ForEach-Object { "$_" } | Tee-Object -FilePath $log | Write-Host
    $gccExit = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($gccExit -ne 0 -or -not (Test-Path $exe)) { throw 'The compiler reported errors (see above).' }

    # ---- 4. check ----------------------------------------------------------
    Say '[4/4] Checking the result...'
    foreach ($f in 'shaders\fullscreen.vert', 'shaders\blackhole.frag', 'shaders\particle.vert', 'shaders\particle.frag',
                   'shaders\post_down.frag', 'shaders\post_blur.frag', 'shaders\post_final.frag', 'shaders\ui.vert', 'shaders\ui.frag') {
        if (-not (Test-Path (Join-Path $root $f))) { throw "Required file missing: $f" }
    }
    Write-Host ("   BlackHoleSimulator.exe created ({0:N0} KB). It needs no extra DLL files." -f ((Get-Item $exe).Length / 1KB)) -ForegroundColor Green
    if (Test-Path $log) { Remove-Item -Force $log }

    $answer = Read-Host 'Start the simulator now? (Y/N)'
    if ($answer -match '^[Yy]') { Start-Process -FilePath $exe -WorkingDirectory $root }
    exit 0
}
catch {
    Write-Host ''
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    ("ERROR: " + $_.Exception.Message) | Add-Content -Path $log
    exit 1
}
