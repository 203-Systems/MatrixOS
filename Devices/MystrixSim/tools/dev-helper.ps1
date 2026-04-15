param(
  [Parameter(Mandatory = $true)]
  [ValidateSet('setup', 'configure', 'build', 'run')]
  [string]$Action,

  [Parameter(Mandatory = $true)]
  [string]$RepoRoot,

  [Parameter(Mandatory = $true)]
  [string]$BuildDir,

  [Parameter(Mandatory = $true)]
  [string]$WebUiDir,

  [Parameter(Mandatory = $true)]
  [string]$Family,

  [Parameter(Mandatory = $true)]
  [string]$Device,

  [Parameter(Mandatory = $true)]
  [string]$Mode,

  [string]$ReleaseVer = '',

  [string]$Generator = 'Unix Makefiles'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
if (-not [System.IO.Path]::IsPathRooted($BuildDir)) {
  $BuildDir = Join-Path $RepoRoot $BuildDir
}
if (-not [System.IO.Path]::IsPathRooted($WebUiDir)) {
  $WebUiDir = Join-Path $RepoRoot $WebUiDir
}
$BuildDir = [System.IO.Path]::GetFullPath($BuildDir)
$WebUiDir = [System.IO.Path]::GetFullPath($WebUiDir)

function Test-CommandAvailable {
  param([string]$Name)
  return $null -ne (Get-Command $Name -ErrorAction SilentlyContinue)
}

function Enable-Emscripten {
  if ((Test-CommandAvailable 'emcmake') -and (Test-CommandAvailable 'emcc')) {
    Write-Host '[MystrixSim] Emscripten toolchain already available in PATH.'
    return
  }

  $roots = @(
    $env:EMSDK,
    'C:\Program Files\emsdk',
    'C:\emsdk',
    (Join-Path $HOME 'emsdk'),
    (Join-Path $HOME 'Documents\emsdk')
  ) | Where-Object { $_ } | Select-Object -Unique

  foreach ($root in $roots) {
    if (Test-Path (Join-Path $root 'emsdk.py')) {
      Write-Host "[MystrixSim] Bootstrapping Emscripten from $root"

      $nodeBin = Get-ChildItem (Join-Path $root 'node') -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        Select-Object -First 1 |
        ForEach-Object { Join-Path $_.FullName 'bin' }

      $pythonBin = Get-ChildItem (Join-Path $root 'python') -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        Select-Object -First 1 |
        ForEach-Object { $_.FullName }

      $pathParts = @(
        $root,
        (Join-Path $root 'upstream\emscripten'),
        $nodeBin,
        $pythonBin
      ) | Where-Object { $_ -and (Test-Path $_) }

      if ($pathParts.Count -gt 0) {
        $env:PATH = (($pathParts -join ';') + ';' + $env:PATH)
      }

      $env:EMSDK = $root
      $env:EM_CONFIG = Join-Path $root '.emscripten'
      $env:EM_CACHE = [System.IO.Path]::GetFullPath((Join-Path $RepoRoot '.emscripten-cache'))
      New-Item -ItemType Directory -Force -Path $env:EM_CACHE | Out-Null

      if ($nodeBin) {
        $nodeExe = Join-Path $nodeBin 'node.exe'
        if (Test-Path $nodeExe) { $env:EMSDK_NODE = $nodeExe }
      }

      if ($pythonBin) {
        $pythonExe = Join-Path $pythonBin 'python.exe'
        if (Test-Path $pythonExe) { $env:EMSDK_PYTHON = $pythonExe }
      }

      if ((Test-CommandAvailable 'emcmake') -and (Test-CommandAvailable 'emcc')) {
        return
      }
    }
  }

  throw @"
MystrixSim requires the Emscripten toolchain, but emcmake/emcc were not found.

Install or activate Emscripten first, for example:
  https://emscripten.org/docs/getting_started/downloads.html

Then re-run:
  make DEVICE=MystrixSim setup
"@
}

function Invoke-Step {
  param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath,

    [Parameter(Mandatory = $true)]
    [string[]]$ArgumentList,

    [string]$WorkingDirectory = $RepoRoot
  )

  Write-Host "[MystrixSim] $FilePath $($ArgumentList -join ' ')"
  Push-Location $WorkingDirectory
  try {
    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
      throw "Command failed with exit code $LASTEXITCODE"
    }
  }
  finally {
    Pop-Location
  }
}

function Get-ConfigureArguments {
  $args = @(
    'cmake',
    '-S', $RepoRoot,
    '-B', $BuildDir,
    "-DFAMILY=$Family",
    "-DDEVICE=$Device",
    "-DMODE=$Mode",
    '-G', $Generator
  )

  if ($ReleaseVer -and $ReleaseVer.Trim().Length -gt 0) {
    $args += "-DMATRIXOS_RELEASE_VER_OVERRIDE=$ReleaseVer"
  }

  return $args
}

Push-Location $RepoRoot
try {
  switch ($Action) {
    'setup' {
      Enable-Emscripten
      Invoke-Step -FilePath 'npm.cmd' -ArgumentList @('install') -WorkingDirectory $WebUiDir
    }

    'configure' {
      Enable-Emscripten
      Invoke-Step -FilePath 'emcmake' -ArgumentList (Get-ConfigureArguments)
    }

    'build' {
      Enable-Emscripten

      & $PSCommandPath configure -RepoRoot $RepoRoot -BuildDir $BuildDir -WebUiDir $WebUiDir -Family $Family -Device $Device -Mode $Mode -ReleaseVer $ReleaseVer -Generator $Generator
      if ($LASTEXITCODE -ne 0) {
        throw "Configure step failed with exit code $LASTEXITCODE"
      }

      Invoke-Step -FilePath 'cmake' -ArgumentList @('--build', $BuildDir)

      $publicDir = Join-Path $WebUiDir 'public'
      $jsSrc = Join-Path $BuildDir 'Devices\MystrixSim\MatrixOSHost.js'
      $wasmSrc = Join-Path $BuildDir 'Devices\MystrixSim\MatrixOSHost.wasm'
      if (-not (Test-Path $jsSrc) -or -not (Test-Path $wasmSrc)) {
        throw "Build completed, but MatrixOSHost.js/wasm were not found under $BuildDir"
      }

      New-Item -ItemType Directory -Force -Path $publicDir | Out-Null

      $packageScript = Join-Path $WebUiDir 'tools\package-runtime.mjs'
      $packageOut = Join-Path $publicDir 'MatrixOS.msfw'
      Invoke-Step -FilePath 'node' -ArgumentList @($packageScript, $jsSrc, $wasmSrc, $packageOut)

      Write-Host "[MystrixSim] Wrote MatrixOS.msfw to $publicDir"
    }

    'run' {
      Invoke-Step -FilePath 'npm.cmd' -ArgumentList @('run', 'dev') -WorkingDirectory $WebUiDir
    }
  }
}
finally {
  Pop-Location
}
