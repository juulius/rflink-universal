# Native Windows integration regression: copies the project into a temporary sandbox.
# Creates multiple updater outputs, verifies newest wins, and checks fail-closed.
[CmdletBinding()]
param([string]$ProjectRoot = '')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $ProjectRoot) { $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
$root = (Resolve-Path -LiteralPath $ProjectRoot).Path
$work = Join-Path ([IO.Path]::GetTempPath()) ('rflink-integration-' + [guid]::NewGuid().ToString('N'))
$proj = Join-Path $work 'project with spaces'
$failures = 0
$testCount = 0
$identityText = [IO.File]::ReadAllText((Join-Path $root 'rflink_universal/components/rflink_universal/firmware_identity.h'))
$versionMatch = [regex]::Match($identityText, 'RFLINK_FIRMWARE_VERSION\s*=\s*"(v[0-9]+\.[0-9]+(?:\.[0-9]+)?)"')
if (-not $versionMatch.Success) { throw 'Missing release version in firmware_identity.h' }
$releaseVersion = $versionMatch.Groups[1].Value
function Assert([bool]$condition,[string]$msg) { if (-not $condition) { throw ('ASSERT FAILED: ' + $msg) } }
function Run-PS([string]$script,[string[]]$arguments) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $script @arguments | Out-Host
    return [int]$LASTEXITCODE
}
# Exercise actual Windows BAT launchers (including paths containing spaces).
# Batches suppress PAUSE only during this automated test, never during normal use.
function Run-BAT([string]$batch,[string[]]$arguments) {
    $wasSet = Test-Path Env:RFLINK_TEST_NO_PAUSE
    $previous = if ($wasSet) { $env:RFLINK_TEST_NO_PAUSE } else { $null }
    try {
        $env:RFLINK_TEST_NO_PAUSE = '1'
        & $batch @arguments | Out-Host
        return [int]$LASTEXITCODE
    } finally {
        if ($wasSet) { $env:RFLINK_TEST_NO_PAUSE = $previous } else { Remove-Item Env:RFLINK_TEST_NO_PAUSE -ErrorAction SilentlyContinue }
    }
}
# Fail early on PowerShell grammar errors in the release/updater scripts before
# exercising any actual update mode. The Windows build uses the real PS parser.
foreach ($scriptName in @('update_rflink.ps1','maak_release.ps1','test_update_release.ps1')) {
    $scriptPath = Join-Path (Join-Path $root 'rflink_universal\tools') $scriptName
    $psTokens = $null
    $psParseErrors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$psTokens, [ref]$psParseErrors) | Out-Null
    Assert ($psParseErrors.Count -eq 0) ('PowerShell syntaxfout in ' + $scriptName + ': ' + ($psParseErrors | Out-String))
}
function Sha([string]$file) { return (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash }
function Runtime-Files([string]$folder) {
    return @(Get-ChildItem -LiteralPath (Join-Path $folder 'rflink_universal') -File -Recurse |
        ForEach-Object { $_.FullName.Substring($folder.Length + 1).Replace('\','/') } |
        Where-Object { $_ -notmatch '^rflink_universal/decoders/_[^/]+/' -and ($_ -match '^rflink_universal/(components|decoders)/.*\.(py|cpp|c|h|hpp|inc|tcc|json|yaml)$' -or $_ -match '^rflink_universal/(options|packages)/.*\.yaml$' -or $_ -match '^rflink_universal/ui/.*\.(html|css|js|json|svg|ico)$') })
}
function Mock-Output([string]$folder,[string]$marker,[datetime]$date,[bool]$ready) {
    $dest = Join-Path (Join-Path $proj 'update_output') $folder
    [IO.Directory]::CreateDirectory($dest) | Out-Null
    $text = [IO.File]::ReadAllText((Join-Path $proj 'rflink-universal.yaml'))
    [IO.File]::WriteAllText((Join-Path $dest 'rflink-universal.yaml'), ('# SOURCE_MARKER: ' + $marker + "`n" + $text))
    $inventory = [ordered]@{}
    foreach ($rel in (Runtime-Files $proj)) {
        $in = Join-Path $proj $rel.Replace('/',[IO.Path]::DirectorySeparatorChar)
        $out = Join-Path $dest $rel.Replace('/',[IO.Path]::DirectorySeparatorChar)
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($out)) | Out-Null
        [IO.File]::Copy($in,$out)
        $inventory[$rel] = Sha $out
    }
    $status = if ($ready) { 'READY' } else { 'CONFLICT' }
    $conflicts = if ($ready) { @() } else { @('ota') }
    $json = [ordered]@{ schema=2;board_revision='eigen_rfid_board_rev2_gpio14';hardware_source='Release';status=$status;generated_utc=$date.ToUniversalTime().ToString('o');yaml_filename='rflink-universal.yaml';yaml_sha256=Sha (Join-Path $dest 'rflink-universal.yaml');conflicts=$conflicts;runtime_sha256=$inventory;release_version='v0.3.3-RC4-HF3' }
    [IO.File]::WriteAllText((Join-Path $dest 'update-manifest.json'),(ConvertTo-Json -InputObject $json -Depth 10))
    return $dest
}
try {
    [IO.Directory]::CreateDirectory($proj) | Out-Null
    foreach ($top in @('rflink-universal.yaml','00_RFLink_START.bat')) {
        [IO.File]::Copy((Join-Path $root $top),(Join-Path $proj $top))
    }
    Copy-Item -LiteralPath (Join-Path $root 'docs') -Destination (Join-Path $proj 'docs') -Recurse
    Copy-Item -LiteralPath (Join-Path $root 'rflink_universal') -Destination (Join-Path $proj 'rflink_universal') -Recurse
    $script = Join-Path $proj 'rflink_universal\tools\maak_release.ps1'
    $rc = Run-BAT (Join-Path $proj '00_RFLink_START.bat') @('release','update')
    Assert ($rc -ne 0) 'CUSTOM must reject absence of update output instead of silently packaging DEFAULT.'
    $testCount++
    # Deliberately reverse lexical folder name versus chronological creation timestamp.
    $first = Mock-Output 'update_z_older' 'OLDER' ([datetime]'2026-09-18T10:00:00Z') $true
    $second = Mock-Output 'update_a_newer' 'NEWER' ([datetime]'2026-09-18T11:00:00Z') $true
    $rc = Run-BAT (Join-Path $proj '00_RFLink_START.bat') @('release')
    Assert ($rc -eq 0) 'BAT launcher should build with 2 valid updater outputs, in a path with spaces.'
    $testCount++
    $sourceZip = Join-Path $proj "dist\RFLink_Universal_${releaseVersion}_DEFAULT_SOURCE.zip"
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead($sourceZip)
    try {
        Assert (@($zip.Entries | Where-Object { $_.FullName -eq 'docs/README.md' }).Count -eq 1) 'SOURCE ZIP missing docs/index.'
        Assert (@($zip.Entries | Where-Object { $_.FullName -eq '00_RFLink_START.bat' }).Count -eq 1) 'SOURCE ZIP missing single launcher.'
        Assert (@($zip.Entries | Where-Object { $_.FullName -match '(^|/)(MAAK_RELEASE|UPDATE_RFLINK|TEST_UPDATE_RELEASE|MAAK_UPLOAD)\.bat$' }).Count -eq 0) 'Old root launchers leaked into SOURCE ZIP.'
    } finally { $zip.Dispose() }
    $bundle = Join-Path $proj "dist\RFLink_Universal_${releaseVersion}_DEFAULT.esphomebundle.tar.gz"
    $customBundle = Join-Path $proj "dist\RFLink_Universal_${releaseVersion}_CUSTOM.esphomebundle.tar.gz"
    $unpack = Join-Path $work 'unpack'
    [IO.Directory]::CreateDirectory($unpack) | Out-Null
    & tar.exe -xzf $bundle -C $unpack
    Assert ($LASTEXITCODE -eq 0) 'Could not inspect release tarball.'
    $yaml = [IO.File]::ReadAllText((Join-Path $unpack 'rflink-universal.yaml'))
    Assert (-not $yaml.Contains('SOURCE_MARKER: NEWER') -and -not $yaml.Contains('SOURCE_MARKER: OLDER')) 'DEFAULT incorrectly selected an updater YAML.'
    # Historical bug: an old three-argument release guard rejected the bundled
    # stock YAML, which correctly uses FOUR arguments for each optional radio.
    # A changed/missing health check must still stop packaging and keep dist intact.
    $stockYamlPath = Join-Path $proj 'rflink-universal.yaml'
    $stockYaml = [IO.File]::ReadAllText($stockYamlPath)
    $goodPoll = 'set_radio_available(0, !id(cc1101_433_42).is_failed() && id(rflink_core).radio_watchdog_healthy(0), !id(rx_433_42).is_failed(), !id(tx_433_42).is_failed());'
    Assert ($stockYaml.Contains($goodPoll)) 'Fixture missing exact WATCHDOG1 four-argument radio health poll.'
    $defaultBefore = Sha $bundle
    try {
        [IO.File]::WriteAllText($stockYamlPath, $stockYaml.Replace($goodPoll, 'set_radio_available(0, true, true, true);'))
        Assert ((Run-BAT (Join-Path $proj '00_RFLink_START.bat') @('release')) -ne 0) 'DEFAULT must reject invalid missing-radio poll.'
        Assert ((Sha $bundle) -eq $defaultBefore) 'Rejected default release must not replace previously good archive.'
    } finally {
        [IO.File]::WriteAllText($stockYamlPath, $stockYaml)
    }
    $testCount++
    $rc = Run-PS $script @('-ProjectRoot',$proj,'-UseUpdaterOutput')
    Assert ($rc -eq 0) 'Explicit builder UPDATE mode must succeed with ready updater outputs.'
    $customCheck = Join-Path $work 'custom_unpack'
    [IO.Directory]::CreateDirectory($customCheck) | Out-Null
    & tar.exe -xzf $customBundle -C $customCheck
    Assert ($LASTEXITCODE -eq 0) 'Could not inspect custom release.'
    $customYaml = [IO.File]::ReadAllText((Join-Path $customCheck 'rflink-universal.yaml'))
    Assert ($customYaml.Contains('SOURCE_MARKER: NEWER') -and -not $customYaml.Contains('SOURCE_MARKER: OLDER')) 'Explicit update did not select newest READY output.'
    $testCount++
    $archiveMembers = @(& tar.exe -tzf $bundle)
    Assert (@($archiveMembers | Where-Object { $_ -match '\.bat$|\.ps1$|backups|update-manifest|WIJZIGINGEN|secrets\.yaml|update_output' }).Count -eq 0) 'Developer files or secrets leaked into the bundle.'
    $testCount++
    Assert (@(Get-ChildItem -LiteralPath $proj -Filter '*.bat' -File).Count -eq 1) 'Exactly one BAT launcher must appear in the root.'
    $testCount++
    $third = Mock-Output 'update_b_conflict' 'CONFLICT' ([datetime]'2026-09-18T12:00:00Z') $false
    $previousHash = Sha $bundle
    $rc = Run-PS $script @('-ProjectRoot',$proj,'-UseUpdaterOutput')
    Assert ($rc -ne 0) 'CUSTOM must fail when newest update conflicts.'
    Assert ((Run-PS $script @('-ProjectRoot',$proj)) -eq 0) 'DEFAULT must ignore conflicting updater output.'
    Assert ((Sha $bundle) -eq $previousHash) 'A failed release modified the previously valid archive.'
    $testCount++
    Remove-Item -LiteralPath $third -Recurse -Force
    [IO.File]::WriteAllText((Join-Path $proj 'update_output\LAST_UPDATE_FAILED.txt'),'simulated failed preflight')
    $rc = Run-PS $script @('-ProjectRoot',$proj,'-UseUpdaterOutput')
    Assert ($rc -ne 0) 'Failed updater preflight must block CUSTOM outputs.'
    Assert ((Run-PS $script @('-ProjectRoot',$proj)) -eq 0) 'DEFAULT must ignore failed updater preflight.'
    Assert ((Sha $bundle) -eq $previousHash) 'Blocked preflight modified previously valid bundle.'
    Remove-Item -LiteralPath (Join-Path $proj 'update_output\LAST_UPDATE_FAILED.txt') -Force
    $testCount++
    [IO.File]::AppendAllText((Join-Path $second 'rflink-universal.yaml'),'# TAMPER')
    $rc = Run-PS $script @('-ProjectRoot',$proj,'-UseUpdaterOutput')
    Assert ($rc -ne 0) 'CUSTOM must fail when newest update hash changes.'
    Assert ((Run-PS $script @('-ProjectRoot',$proj)) -eq 0) 'DEFAULT must ignore tampered updater output.'
    $testCount++
    # Separate sandbox for REAL updater execution (not mocked metadata).
    $mergeProj = Join-Path $work 'merge project with spaces'
    [IO.Directory]::CreateDirectory($mergeProj) | Out-Null
    foreach ($top in @('rflink-universal.yaml','00_RFLink_START.bat')) {
        [IO.File]::Copy((Join-Path $root $top),(Join-Path $mergeProj $top))
    }
    Copy-Item -LiteralPath (Join-Path $root 'docs') -Destination (Join-Path $mergeProj 'docs') -Recurse
    Copy-Item -LiteralPath (Join-Path $root 'rflink_universal') -Destination (Join-Path $mergeProj 'rflink_universal') -Recurse
    $newDir = Join-Path $work 'new release with spaces'
    [IO.Directory]::CreateDirectory($newDir) | Out-Null
    Copy-Item -LiteralPath (Join-Path $root 'rflink_universal') -Destination (Join-Path $newDir 'rflink_universal') -Recurse
    $baseText = [IO.File]::ReadAllText((Join-Path $mergeProj 'rflink-universal.yaml'))
    $oldPath = Join-Path $work 'my existing.yaml'
    $newPath = Join-Path $newDir 'rflink-universal.yaml'
    [IO.File]::WriteAllText($oldPath, $baseText.Replace('status_led_gpio: GPIO48','status_led_gpio: GPIO46').Replace('radio1_csn: GPIO5','radio1_csn: GPIO7'))
    [IO.File]::WriteAllText($newPath,$baseText.Replace('  log_level: INFO','  log_level: DEBUG').Replace('  status_led_gpio: GPIO48','  status_led_gpio: GPIO48' + "`n" + '  new_release_option: enabled'))
    $before = Sha $oldPath
    $updater = Join-Path $mergeProj 'rflink_universal\tools\update_rflink.ps1'
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -eq 0) 'Explicit Rev1 hardware selection must build a verified CUSTOM bundle directly.'
    Assert ((Sha $oldPath) -eq $before) 'Updater changed an original YAML.'
    $outputs = @(Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory)
    Assert ($outputs.Count -eq 1) 'First updater run must make exactly one unique folder.'
    $merged = [IO.File]::ReadAllText((Join-Path $outputs[0].FullName 'rflink-universal.yaml'))
    Assert ($merged.Contains('status_led_gpio: GPIO48') -and $merged.Contains('radio1_csn: GPIO5') -and $merged.Contains('  log_level: DEBUG') -and $merged.Contains('new_release_option: enabled')) 'Updater must migrate hardware pins to new release and preserve non-pin edits.'
    $pinReport = [IO.File]::ReadAllText((Join-Path $outputs[0].FullName 'WIJZIGINGEN.txt'))
    Assert ($pinReport.Contains('HARDWARE_PIN_RELEASE substitutions.radio1_csn')) 'Updater must log an overridden old pin.'
    $testCount++
    $rc = Run-BAT (Join-Path $mergeProj '00_RFLink_START.bat') @('release')
    Assert ($rc -eq 0) 'Standalone DEFAULT builder must succeed even after updater.'
    $rc = Run-BAT (Join-Path $mergeProj '00_RFLink_START.bat') @('release','update')
    Assert ($rc -ne 0) 'The clean-release entrypoint must reject extra update arguments.'
    $mergeBundle = Join-Path $mergeProj "dist\RFLink_Universal_${releaseVersion}_CUSTOM.esphomebundle.tar.gz"
    $inspect = Join-Path $work 'merge_unpack'
    [IO.Directory]::CreateDirectory($inspect) | Out-Null
    & tar.exe -xzf $mergeBundle -C $inspect
    Assert ($LASTEXITCODE -eq 0) 'Merged bundle could not be extracted.'
    Assert ([IO.File]::ReadAllText((Join-Path $inspect 'rflink-universal.yaml')).Contains('new_release_option: enabled')) 'Merged YAML not in actual release bundle.'
    $testCount++
    # Different/unknown board + changed pinouts: Automatic must STOP (no guessing).
    [IO.File]::WriteAllText($oldPath, $baseText.Replace('board_revision: eigen_rfid_board_rev2_gpio14','board_revision: other_pcb').Replace('radio1_csn: GPIO4','radio1_csn: GPIO7'))
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-Mode','Automatic')
    Assert ($rc -ne 0) 'Automatic mode must reject uncertain physical board.'
    $testCount++
    # Explicit existing hardware: keep entire pin table, report custom board and
    # distinguish its bundle from the standard reference board profile.
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Existing','-NonInteractive')
    Assert ($rc -eq 0) 'Existing complete safe pinout should generate a custom hardware bundle.'
    $newest = @(Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory | Sort-Object LastWriteTimeUtc -Descending)[0]
    $hardwareYaml = [IO.File]::ReadAllText((Join-Path $newest.FullName 'rflink-universal.yaml'))
    Assert ($hardwareYaml.Contains('board_revision: custom_existing') -and $hardwareYaml.Contains('radio1_csn: GPIO7')) 'Existing hardware must not be overwritten.'
    $testCount++
    # A missing central pin label cannot be preserved as existing PCB hardware.
    [IO.File]::WriteAllText($oldPath, $baseText.Replace('  radio1_csn: GPIO4','  old_radio1_csn: GPIO4'))
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Existing','-NonInteractive')
    Assert ($rc -ne 0) 'Incomplete old pin table must not be used.'
    $testCount++
    # No PCB-pin changes => BAT normal launcher has no interactive hardware question.
    [IO.File]::WriteAllText($oldPath, $baseText)
    $rc = Run-BAT (Join-Path $mergeProj '00_RFLink_START.bat') @('update','automatic',$oldPath,$newDir)
    Assert ($rc -eq 0) 'Single entrypoint should update with unchanged physical pinout.'
    $testCount++
    # Itho identity belongs to the device, not a new default release.
    [IO.File]::WriteAllText($oldPath, $baseText.Replace("  itho_id1: '0x06'","  itho_id1: '0xAA'"))
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-NonInteractive')
    Assert ($rc -eq 0) 'Itho custom ID must survive the update without an interactive question in noninteractive mode.'
    $identityCheck = @((Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory | Sort-Object LastWriteTimeUtc -Descending))[0]
    Assert ([IO.File]::ReadAllText((Join-Path $identityCheck.FullName 'rflink-universal.yaml')).Contains("itho_id1: '0xAA'")) 'Updater silently changed paired Itho ID.'
    $testCount++
    # Regression: upgrade an unmodified v2B SOURCE + matching YAML to this v2C
    # release. The old builtin decoder legitimately changed between releases;
    # neither its CPP nor its schema-2 manifest may be flagged as user edits.
    $v2b = Join-Path $mergeProj 'rflink_universal/maintenance/baselines/v0.3.3-RC4-HF4_DECODERAPI2B.yaml'
    [IO.File]::WriteAllText($oldPath, ([IO.File]::ReadAllText($v2b)).Replace("  itho_id1: '0x06'", "  itho_id1: '0xAA'"))
    $stockOld = Join-Path $work 'stock old v2b with spaces'
    [IO.Directory]::CreateDirectory($stockOld) | Out-Null
    Copy-Item -LiteralPath (Join-Path $root 'rflink_universal') -Destination (Join-Path $stockOld 'rflink_universal') -Recurse
    $fixtureDir = Join-Path $root 'rflink_universal/maintenance/baselines/decoderapi2b_esphome_builtin'
    $oldPack = Join-Path $stockOld 'rflink_universal/decoders/esphome_builtin'
    [IO.File]::Copy((Join-Path $fixtureDir 'decoder.cpp'), (Join-Path $oldPack 'decoder.cpp'), $true)
    [IO.File]::Copy((Join-Path $fixtureDir 'manifest.json'), (Join-Path $oldPack 'manifest.json'), $true)
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-ExistingRuntime',$stockOld,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -eq 0) 'Verified unmodified Decoder API v2B decoder+YAML must migrate to v2C without false firmware conflicts.'
    $v2bReady = @(Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory | Sort-Object LastWriteTimeUtc -Descending)[0]
    $v2bReport = [IO.File]::ReadAllText((Join-Path $v2bReady.FullName 'WIJZIGINGEN.txt'))
    Assert ($v2bReport.Contains('DECODER_STOCK_UPGRADE esphome_builtin') -and -not $v2bReport.Contains('CONFLICT')) 'Stock decoder baseline or migration logging failed.'
    $testCount++
    # A modified CPP from the same historical pack must still STOP.
    [IO.File]::AppendAllText((Join-Path $oldPack 'decoder.cpp'), "`n// USER EDIT`n")
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-ExistingRuntime',$stockOld,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -eq 2) 'Custom changes to a historical decoder must block a blind upgrade.'
    $testCount++
    [IO.File]::WriteAllText($oldPath, $baseText.Replace("  itho_id1: '0x06'", "  itho_id1: '0xAA'"))
    # Own decoder absent from new runtime is a STOP, not a silent deletion.
    $oldSource = Join-Path $work 'old user source'
    [IO.Directory]::CreateDirectory($oldSource) | Out-Null
    Copy-Item -LiteralPath (Join-Path $root 'rflink_universal') -Destination (Join-Path $oldSource 'rflink_universal') -Recurse
    $extra = Join-Path $oldSource 'rflink_universal/decoders/custom_decoder'
    [IO.Directory]::CreateDirectory($extra) | Out-Null
    [IO.File]::WriteAllText((Join-Path $extra 'manifest.json'),'{"schema":1,"id":"custom_decoder"}')
    [IO.File]::WriteAllText((Join-Path $extra 'decoder.cpp'),'// user decoder requires review')
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-ExistingRuntime',$oldSource,'-NonInteractive')
    Assert ($rc -eq 2) 'Custom decoder must block until compatibility review.'
    $testCount++
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Unknown','-NonInteractive')
    Assert ($rc -ne 0) 'Explicit unknown physical PCB must stop.'
    $testCount++
    [IO.File]::WriteAllText($oldPath, $baseText.Replace('  log_level: INFO','  log_level: WARN'))
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-NonInteractive')
    Assert ($rc -eq 2) 'Conflicting same YAML block must yield exit code 2.'
    $rc = Run-PS (Join-Path $mergeProj 'rflink_universal\tools\maak_release.ps1') @('-ProjectRoot',$mergeProj,'-UseUpdaterOutput')
    Assert ($rc -ne 0) 'CUSTOM must block latest actual conflict output.'
    Assert ((Run-PS (Join-Path $mergeProj 'rflink_universal\tools\maak_release.ps1') @('-ProjectRoot',$mergeProj)) -eq 0) 'DEFAULT must keep working despite latest conflict.'
    $testCount++
    # Regression: REAL HF3 UX2 YAML versus HF4 project must auto-select the
    # archived HF3 baseline, instead of comparing it to new project YAML.
    $hf3 = Join-Path $mergeProj 'rflink_universal\maintenance\baselines\v0.3.3-RC4-HF3_UX2.yaml'
    $hf3Text = [IO.File]::ReadAllText($hf3)
    [IO.File]::WriteAllText($oldPath,$hf3Text.Replace("  itho_id1: '0x06'","  itho_id1: '0xAA'"))
    $oldHash = Sha $oldPath
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -eq 0) 'HF3 UX2 must automatically find its original baseline and build HF4 custom.'
    Assert ((Sha $oldPath) -eq $oldHash) 'HF3 input YAML was modified.'
    $hf3Output = @(Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory | Sort-Object LastWriteTimeUtc -Descending)[0]
    $hf3Merged = [IO.File]::ReadAllText((Join-Path $hf3Output.FullName 'rflink-universal.yaml'))
    $hf3Report = [IO.File]::ReadAllText((Join-Path $hf3Output.FullName 'WIJZIGINGEN.txt'))
    Assert ($hf3Merged.Contains("itho_id1: '0xAA'") -and $hf3Merged.Contains('radio3_csn: GPIO15') -and $hf3Merged.Contains('board_revision: eigen_rfid_board_rev2_gpio14')) 'HF3 migration lost paired ID or release pinout.'
    Assert ($hf3Report.Contains('Baseline selection: verified release-specific baseline') -and $hf3Report.Contains('v0.3.3-RC4-HF3_UX2.yaml')) 'HF3 auto baseline was not reported.'
    $testCount++
    # Wizard4: a partly-updated modern HF3 may contain an older *known* TX
    # block and only different comments beside the SAME external component.
    # This is safe to migrate; the new HF4 code must win, without a false conflict.
    $legacyBase = [IO.File]::ReadAllText((Join-Path $mergeProj 'rflink_universal\maintenance\baselines\v0.3.3-RC4-HF3_LEGACY_REVIEWED.yaml'))
    $txPattern = '(?ms)^remote_transmitter:\r?\n.*?(?=^interval:)'
    $modernTx = [regex]::Match($hf3Text,$txPattern)
    $legacyTx = [regex]::Match($legacyBase,$txPattern)
    Assert ($modernTx.Success -and $legacyTx.Success) 'Both historical TX blocks must be present.'
    $mixed = $hf3Text.Substring(0,$modernTx.Index) + $legacyTx.Value + $hf3Text.Substring($modernTx.Index + $modernTx.Length)
    $mixed = $mixed.Replace('Onboard status RGB: WS2812 data GPIO48;', 'Onboard status RGB: WS2812 data GPIO47;')
    [IO.File]::WriteAllText($oldPath,$mixed)
    $mixedHash = Sha $oldPath
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -eq 0) 'Known legacy transmitter plus comment-only external_components must not cause a false conflict.'
    Assert ((Sha $oldPath) -eq $mixedHash) 'Mixed-variant existing YAML was modified.'
    $mixedOutput = @(Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory | Sort-Object LastWriteTimeUtc -Descending)[0]
    $mixedReport = [IO.File]::ReadAllText((Join-Path $mixedOutput.FullName 'WIJZIGINGEN.txt'))
    $mixedYaml = [IO.File]::ReadAllText((Join-Path $mixedOutput.FullName 'rflink-universal.yaml'))
    Assert ($mixedReport.Contains('KNOWN_LEGACY_TX') -and $mixedReport.Contains('comments only differ')) 'Known safe migrations must appear in report.'
    Assert ($mixedYaml.Contains('rmt_symbols: 48') -and $mixedYaml.Contains('record_tx_command(0,')) 'Old transmitter may not overwrite HF4 fixes.'
    $testCount++
    # A *real* unknown transmitter edit is NOT equivalent to the legacy variant.
    # Normal/Automatic with no explicit Y must stop. Explicit command-line opt-in
    # to discarding custom firmware code is separately logged and may then build.
    $unknown = $mixed.Replace('carrier_duty_percent: 100%', 'carrier_duty_percent: 90%')
    [IO.File]::WriteAllText($oldPath,$unknown)
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -eq 2) 'Unknown transmitter changes must stop without an explicit opt-in.'
    $unknownOutput = @(Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory | Sort-Object LastWriteTimeUtc -Descending)[0]
    $unknownReport = [IO.File]::ReadAllText((Join-Path $unknownOutput.FullName 'WIJZIGINGEN.txt'))
    Assert ($unknownReport.Contains('CONFLICT remote_transmitter')) 'Unknown transmitter conflict needs a clear report.'
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Release','-AcceptReleaseFirmwareChanges','-NonInteractive')
    Assert ($rc -eq 0) 'Explicit choice to discard old firmware must build the known HF4 release.'
    $overrideOutput = @(Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory | Sort-Object LastWriteTimeUtc -Descending)[0]
    Assert ([IO.File]::ReadAllText((Join-Path $overrideOutput.FullName 'WIJZIGINGEN.txt')).Contains('EXPLICIT_FIRMWARE_OVERRIDE remote_transmitter')) 'Risky override must be recorded.'
    $testCount++
    # Explicit WRONG baseline must never bypass release checking.
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-BaseYaml',(Join-Path $mergeProj 'rflink-universal.yaml'),'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -ne 0) 'Explicit wrong HF4 baseline for an HF3 YAML must be rejected.'
    $testCount++
    # Header claiming HF2 with no catalog entry: fail, do not pretend HF4 is baseline.
    [IO.File]::WriteAllText($oldPath,$hf3Text.Replace('RC4-HF3','RC4-HF2'))
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -ne 0) 'Unknown old HF2 version must fail closed without exact baseline.'
    $testCount++
    # Regression for the EXACT older HF3 profile that was rejected by Wizard 2.
    # Never distribute the customer's YAML: synthesize a sample from the redacted
    # reference using fake, valid device and Itho identity values.
    $legacyRef = Join-Path $mergeProj 'rflink_universal\maintenance\baselines\v0.3.3-RC4-HF3_LEGACY_REVIEWED.yaml'
    $legacy = [IO.File]::ReadAllText($legacyRef)
    foreach ($kv in @(
        @('name', 'test-legacy-rflink'),
        @('friendly_name', 'Legacy RFLink Test'),
        @('itho_id1', "'0xAA'"),
        @('itho_id2', "'0xBB'"),
        @('itho_id3', "'0xCC'"),
        @('itho_device_type', "'0x16'")
    )) {
        $needle = '  ' + $kv[0] + ': __PERSONAL_SETTING__'
        Assert ($legacy.Contains($needle)) "Legacy reference placeholder missing: $($kv[0])"
        $legacy = $legacy.Replace($needle, ('  ' + $kv[0] + ': ' + $kv[1]))
    }
    [IO.File]::WriteAllText($oldPath, $legacy)
    $legacyOriginalHash = Sha $oldPath
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -eq 0) 'Reviewed older HF3 sample should update cleanly to HF4; Wizard 2 previously rejected it.'
    Assert ((Sha $oldPath) -eq $legacyOriginalHash) 'Legacy input YAML was modified.'
    $latest = @(Get-ChildItem -LiteralPath (Join-Path $mergeProj 'update_output') -Directory | Sort-Object LastWriteTimeUtc -Descending)[0]
    $result = [IO.File]::ReadAllText((Join-Path $latest.FullName 'rflink-universal.yaml'))
    $report = [IO.File]::ReadAllText((Join-Path $latest.FullName 'WIJZIGINGEN.txt'))
    Assert ($result.Contains("itho_id1: '0xAA'") -and $result.Contains("itho_id2: '0xBB'") -and $result.Contains('radio3_csn: GPIO15')) 'Legacy migration lost personal identity or new GPIOs.'
    Assert (($result -split 'rmt_symbols: 48').Count -ge 3 -and $result.Contains('id(rflink_status_rgb).loop();') -and -not $result.Contains('write_state(&id(')) 'Legacy migration lost new RMT TX/OTA fixes.'
    Assert ($report.Contains('reviewed legacy HF3 variant')) 'Legacy baseline selection was not clearly documented.'
    $testCount++
    # Unreviewed changes must NOT be passed off as this known legacy profile.
    [IO.File]::WriteAllText($oldPath, $legacy.Replace('  dedupe_window: 80ms','  dedupe_window: 90ms'))
    $rc = Run-PS $updater @('-ProjectRoot',$mergeProj,'-ExistingYaml',$oldPath,'-NewRelease',$newDir,'-HardwareChoice','Release','-NonInteractive')
    Assert ($rc -ne 0) 'Unknown legacy firmware edits must not bypass strict fingerprint matching.'
    $testCount++
    Write-Host "PASS: $testCount native Windows integration checks."
    exit 0
} catch {
    [Console]::Error.WriteLine('FAIL: ' + $_.Exception.Message)
    exit 1
} finally {
    if ([IO.Directory]::Exists($work)) { Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue }
}
