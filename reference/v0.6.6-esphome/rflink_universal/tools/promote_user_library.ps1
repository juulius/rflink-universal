param(
  [Parameter(Mandatory=$true)][string]$Backup,
  [switch]$Merge,
  [string]$Replace,
  [string]$CandidateOut
)
$ErrorActionPreference = 'Stop'
$Script = Join-Path $PSScriptRoot 'promote_user_library.py'
$ArgsList = @($Script, $Backup)
if ($Merge) { $ArgsList += '--merge' } else { $ArgsList += '--check' }
if ($Replace) { $ArgsList += @('--replace', $Replace) }
if ($CandidateOut) { $ArgsList += @('--candidate-out', $CandidateOut) }

# Promotion is a maintainer/developer action. Prefer an existing Python launcher,
# but also reuse the Python that PlatformIO commonly installs with ESPHome so a
# separate manual Python installation is usually unnecessary.
$pythonExe = $null
$pythonPrefix = @()
$py = Get-Command py -ErrorAction SilentlyContinue
if ($py) {
  $pythonExe = $py.Source
  $pythonPrefix = @('-3')
} else {
  foreach ($name in @('python','python3')) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { $pythonExe = $cmd.Source; break }
  }
}
if (-not $pythonExe -and $env:USERPROFILE) {
  foreach ($candidate in @(
    (Join-Path $env:USERPROFILE '.platformio\penv\Scripts\python.exe'),
    (Join-Path $env:USERPROFILE '.platformio\python3\python.exe')
  )) {
    if (Test-Path -LiteralPath $candidate) { $pythonExe = $candidate; break }
  }
}
if (-not $pythonExe) {
  throw 'Geen bruikbare Python-runtime gevonden. De normale RFLink backup/restore heeft Python niet nodig; alleen maintainer-promotie gebruikt Python + g++ voor hostvalidatie.'
}

& $pythonExe @pythonPrefix @ArgsList
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
