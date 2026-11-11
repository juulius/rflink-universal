# RFLink Universal dual-mode release packer. Windows PowerShell 5.1 + Windows tar.exe only.
# No Python, pip, 7-Zip, ESPHome CLI, internet access, or Administrator rights required.
[CmdletBinding()]
param([string]$ProjectRoot = "", [switch]$UseUpdaterOutput)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (-not $ProjectRoot) { $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
$root = (Resolve-Path -LiteralPath $ProjectRoot).Path.TrimEnd('\','/')
# Read the short, authoritative release version from the compiled firmware identity.
$identityPath = Join-Path $root 'rflink_universal/components/rflink_universal/firmware_identity.h'
$identityText = [IO.File]::ReadAllText($identityPath)
$versionMatch = [regex]::Match($identityText, 'RFLINK_FIRMWARE_VERSION\s*=\s*"(v[0-9]+\.[0-9]+(?:\.[0-9]+)?)"')
if (-not $versionMatch.Success) { throw 'Missing valid semantic release version in firmware_identity.h' }
$version = $versionMatch.Groups[1].Value
$dist = Join-Path $root 'dist'
$runtimeRoot = $root
$configSource = Join-Path $root 'rflink-universal.yaml'
$selectedUpdate = $null
$zipOut = $null
$bundleOut = $null
$temp = Join-Path ([IO.Path]::GetTempPath()) ('rfrel-' + [guid]::NewGuid().ToString('N').Substring(0,8))
$srcStage = Join-Path $temp 'source'
$bundleStage = Join-Path $temp 'bundle'
$checkStage = Join-Path $temp 'check'

function Fail([string]$message) { throw $message }
function Assert-Directory([string]$path, [string]$label) {
    if (-not (Test-Path -LiteralPath $path -PathType Container)) { Fail "Required directory missing: $label ($path)" }
}
function Read-Utf8([string]$path) {
    # The release script must be self-contained: updater functions are not imported.
    $bytes = [IO.File]::ReadAllBytes($path)
    $decoder = New-Object Text.UTF8Encoding($false, $true)
    return $decoder.GetString($bytes).TrimStart([char]0xFEFF)
}
function Assert-File([string]$relative) {
    if (-not (Test-Path -LiteralPath (Join-Path $root ($relative.Replace('/',[IO.Path]::DirectorySeparatorChar))) -PathType Leaf)) {
        Fail "Required file missing: $relative"
    }
}
function Relative([string]$parent, [string]$path) {
    return $path.Substring($parent.Length + 1).Replace('\','/')
}
function Copy-Stage([string]$relative, [string]$destination) {
    if ($relative -eq 'rflink-universal.yaml') { $from = $configSource }
    elseif ($relative.StartsWith('rflink_universal/components/') -or
            $relative.StartsWith('rflink_universal/decoders/') -or
            $relative.StartsWith('rflink_universal/options/') -or
            $relative.StartsWith('rflink_universal/packages/') -or
            $relative.StartsWith('rflink_universal/ui/')) {
        $from = Join-Path $runtimeRoot ($relative.Replace('/',[IO.Path]::DirectorySeparatorChar))
    } else { $from = Join-Path $root ($relative.Replace('/',[IO.Path]::DirectorySeparatorChar)) }
    $to = Join-Path $destination ($relative.Replace('/',[IO.Path]::DirectorySeparatorChar))
    if (-not [IO.File]::Exists($from)) { Fail "Packaging source missing while staging '$relative': $from" }
    $dir = [IO.Path]::GetDirectoryName($to)
    try {
        if (-not [IO.Directory]::Exists($dir)) { [IO.Directory]::CreateDirectory($dir) | Out-Null }
        [IO.File]::Copy($from, $to, $true)
    } catch {
        $hint = if ($from.Length -gt 240 -or $to.Length -gt 240) { ' Path is near the classic Windows 260-character limit; extract the source ZIP to a short folder such as C:\RFLink and retry.' } else { '' }
        Fail ("Could not stage '$relative'. FROM='$from' TO='$to'. " + $_.Exception.Message + $hint)
    }
}
function Hash-Same([string]$a, [string]$b) {
    return (Get-FileHash -LiteralPath $a -Algorithm SHA256).Hash -eq (Get-FileHash -LiteralPath $b -Algorithm SHA256).Hash
}
function All-Files([string]$folder) {
    return @(Get-ChildItem -LiteralPath $folder -File -Recurse -Force | ForEach-Object { Relative $folder $_.FullName } | Sort-Object)
}
function Check-Identical([string]$a, [string]$b) {
    $left = All-Files $a
    $right = All-Files $b
    if (($left -join "`n") -cne ($right -join "`n")) { Fail 'Archive file list does not match staging directory.' }
    foreach ($entry in $left) {
        $x = Join-Path $a ($entry.Replace('/',[IO.Path]::DirectorySeparatorChar))
        $y = Join-Path $b ($entry.Replace('/',[IO.Path]::DirectorySeparatorChar))
        if (-not (Hash-Same $x $y)) { Fail "Archive file content mismatch: $entry" }
    }
}
function Forbidden([string]$relative) {
    $parts = $relative.Replace('\','/').Split('/')
    $badDirs = @('dist','build','.esphome','.git','__pycache__','venv','.venv','node_modules','.pio','logs','backup','backups','update_output','output','.bundle_staging')
    foreach ($part in $parts[0..($parts.Length-2)]) {
        if ($part.StartsWith('.') -or $badDirs -contains $part.ToLowerInvariant()) { return $true }
    }
    $file = $parts[-1].ToLowerInvariant()
    if ($file.StartsWith('.') -or $file.Contains('.backup_') -or $file.EndsWith('~')) { return $true }
    if ($file -match '(\.bak|\.tmp|\.sw[a-p]|\.swo|\.log|\.pyc)$') { return $true }
    if ($file -in @('secrets.yaml','.env','.env.local','credentials.json','credentials.yaml','passwords.txt','id_rsa','id_ed25519','private.key')) { return $true }
    if ($file.StartsWith('secrets.') -and $file -ne 'secrets.example.yaml') { return $true }
    if ([IO.Path]::GetExtension($file) -in @('.zip','.gz','.bin','.elf','.o','.obj','.pem','.key','.p12','.pfx','.db','.sqlite','.sqlite3')) { return $true }
    return $false
}

try {
    if ($PSVersionTable.PSVersion.Major -lt 5) { Fail 'Windows PowerShell 5.1 or newer is required.' }
    if (-not (Get-Command tar.exe -ErrorAction SilentlyContinue)) { Fail 'Windows tar.exe not found.' }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $required = @(
        'rflink-universal.yaml','00_RFLink_START.bat',
        'docs/README.md','docs/START_HERE.md','docs/INSTALLEREN_EN_UPDATEN.md',
        'docs/INSTELLINGEN.md','docs/RELEASE_NOTES.md',
        'rflink_universal/tools/maak_release.ps1','rflink_universal/tools/update_rflink.ps1','rflink_universal/tools/test_update_release.ps1',
        'rflink_universal/maintenance/baselines/v0.3.3-RC4-HF4_DECODERAPI2B.yaml',
        'rflink_universal/maintenance/baselines/v0.3.3-RC4-HF4_DECODERAPI2C.yaml',
        'rflink_universal/maintenance/decoder_upgrade_baselines.json',
        'rflink_universal/components/rflink_universal/__init__.py',
        'rflink_universal/components/rflink_universal/rflink_universal.cpp',
        'rflink_universal/components/rflink_universal/rflink_log_store.cpp',
        'rflink_universal/components/rflink_universal/rflink_log_nvs_espidf.cpp',
        'rflink_universal/components/rflink_universal/web_ui.h',
        'rflink_universal/components/rflink_universal/status_led.h',
        'rflink_universal/components/rflink_universal/device_persistence.h',
        'rflink_universal/components/rflink_universal/device_persistence.cpp',
        'rflink_universal/components/rflink_universal/user_vm.cpp',
        'rflink_universal/components/rflink_universal/user_library.cpp',
        'rflink_universal/components/rflink_universal/firmware_user_recipes.h',
        'rflink_universal/tools/promote_user_library.py',
        'rflink_universal/tools/promote_user_library.ps1',
        'rflink_universal/decoders/itho_rft/decoder.cpp',
        'rflink_universal/decoders/itho_rft/manifest.json',
        'rflink_universal/options/rgb_status_ws2812.yaml',
        'rflink_universal/options/ha_mqtt.yaml',
        'rflink_universal/components/rflink_universal/ha_discovery.h',
        'rflink_universal/components/rflink_universal/manifest_schema.py',
        'rflink_universal/components/rflink_universal/capability_catalog.h',
        'rflink_universal/components/rflink_universal/capability_catalog_generated.h',
        'rflink_universal/ui/index.html',
        'rflink_universal/ui/locales.bundle.json',
        'rflink_universal/ui/favicon.svg','rflink_universal/ui/favicon.ico'
    )
    foreach ($relative in $required) { Assert-File $relative }
    # DEFAULT: always package the project source; updater state never changes it.
    # UPDATE: explicit opt-in and fail-closed validation of latest update output.
    # Separate filenames keep default and updated releases from overwriting each other.
    $updatesDir = Join-Path $root 'update_output'
    if ($UseUpdaterOutput) {
        Write-Host 'RELEASE MODE: UPDATE (explicit) - newest verified updater YAML and runtime.'
    if ([IO.File]::Exists((Join-Path $updatesDir 'LAST_UPDATE_FAILED.txt'))) {
        Fail 'Previous updater attempt FAILED before a valid output was created. Read update_output/LAST_UPDATE_FAILED.txt, fix the problem and rerun 00_RFLink_START.bat (update).'
    }
    if ([IO.Directory]::Exists($updatesDir)) {
        $entries = @(Get-ChildItem -LiteralPath $updatesDir -Directory -Force | Where-Object { $_.Name -like 'update_*' })
        if ($entries.Count) {
            $records = New-Object 'System.Collections.Generic.List[object]'
            foreach ($entry in $entries) {
                $metaPath = Join-Path $entry.FullName 'update-manifest.json'
                if (-not [IO.File]::Exists($metaPath)) { Fail "Update folder missing manifest, cannot choose latest safely: $($entry.Name)" }
                $m = Get-Content -LiteralPath $metaPath -Raw -Encoding UTF8 | ConvertFrom-Json
                if ($m.schema -ne 2 -or -not $m.generated_utc -or -not $m.hardware_source -or -not $m.board_revision) { Fail "Bad update metadata: $($entry.Name)" }
                $time = [DateTimeOffset]::Parse($m.generated_utc, [Globalization.CultureInfo]::InvariantCulture)
                $records.Add([pscustomobject]@{ Directory=$entry.FullName; Manifest=$m; When=$time; Name=$entry.Name })
            }
            $latest = @($records | Sort-Object -Property When,Name -Descending)[0]
            $m = $latest.Manifest
            if ($m.status -cne 'READY' -or @($m.conflicts).Count -gt 0) {
                Fail "Newest updater output $($latest.Name) is NOT READY. Fix it; never fall back to an older output."
            }
            if ($m.yaml_filename -cne 'rflink-universal.yaml') { Fail 'Update metadata has an invalid YAML filename.' }
            $configSource = Join-Path $latest.Directory 'rflink-universal.yaml'
            if (-not [IO.File]::Exists($configSource) -or (Get-FileHash -LiteralPath $configSource -Algorithm SHA256).Hash -cne $m.yaml_sha256) {
                Fail 'Newest updater YAML has changed since approval; rerun 00_RFLink_START.bat (update).'
            }
            $runtimeRoot = $latest.Directory
            $listed = @($m.runtime_sha256.PSObject.Properties | ForEach-Object { $_.Name } | Sort-Object)
            $actual = @(Get-ChildItem -LiteralPath (Join-Path $runtimeRoot 'rflink_universal') -File -Recurse -Force |
                ForEach-Object { Relative $runtimeRoot $_.FullName } |
                Where-Object { $_ -notmatch '^rflink_universal/decoders/_[^/]+/' -and ($_ -match '^rflink_universal/(components|decoders)/.*\.(py|cpp|c|h|hpp|inc|tcc|json|yaml)$' -or
                    $_ -match '^rflink_universal/(options|packages)/.*\.yaml$' -or
                    $_ -match '^rflink_universal/ui/.*\.(html|css|js|json|svg|ico)$') } | Sort-Object)
            if (($listed -join "`n") -cne ($actual -join "`n")) { Fail 'Newest update runtime inventory is incomplete or has unexpected files.' }
            foreach ($rel in $listed) {
                $path = Join-Path $runtimeRoot ($rel.Replace('/',[IO.Path]::DirectorySeparatorChar))
                if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -cne $m.runtime_sha256.$rel) { Fail "Update runtime changed: $rel" }
            }
            # The verified pinout release remains HF4 even when upstream update metadata names HF3.
            $selectedUpdate = $latest.Directory
            Write-Host ('SELECTED NEWEST UPDATE: ' + $selectedUpdate)
        }
    }
    if (-not $selectedUpdate) { Fail 'UPDATE mode requested but no verified update_output exists. Run 00_RFLink_START.bat and choose update first, or choose clean release in 00_RFLink_START.bat.' }
    } else { Write-Host 'RELEASE MODE: DEFAULT - using project rflink-universal.yaml and project runtime; update_output ignored.' }
    Assert-Directory (Join-Path $runtimeRoot 'rflink_universal') 'runtime root'
    Assert-Directory (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal') 'RFLink component'
    Assert-Directory (Join-Path $runtimeRoot 'rflink_universal\decoders') 'decoder directory'
    if ($root.Length -gt 140) { Write-Warning "Long project path ($($root.Length) characters). If Windows reports that part of a path cannot be found, extract this SOURCE package to C:\RFLink and rerun." }
    $modeSuffix = if ($UseUpdaterOutput) { '_CUSTOM' } else { '_DEFAULT' }
    $stem = 'RFLink_Universal_' + $version + $modeSuffix
    $zipOut = Join-Path $dist ($stem + '_SOURCE.zip')
    $bundleOut = Join-Path $dist ($stem + '.esphomebundle.tar.gz')
    # Release identity is enforced on the actual selected firmware source, not YAML.
    # This catches a code change that accidentally reuses an older release number.
    $firmwarePath = Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\firmware_identity.h'
    $firmwareIdentity = Read-Utf8 $firmwarePath
    $hashMatch = [regex]::Match($firmwareIdentity, 'RFLINK_FIRMWARE_BUILD_HASH\s*=\s*"([0-9a-f]{12})"')
    $nameMatch = [regex]::Match($firmwareIdentity, 'RFLINK_FIRMWARE_NAME\s*=\s*"([A-Za-z0-9 _-]+)"')
    $runtimeVersionMatch = [regex]::Match($firmwareIdentity, 'RFLINK_FIRMWARE_VERSION\s*=\s*"(v[0-9]+\.[0-9]+(?:\.[0-9]+)?)"')
    if (-not $hashMatch.Success -or -not $nameMatch.Success -or -not $runtimeVersionMatch.Success -or $runtimeVersionMatch.Groups[1].Value -cne $version) { Fail 'Selected firmware identity does not match the release version, name or SHA-256 hash.' }
    $runtimeFolder = Join-Path $runtimeRoot 'rflink_universal'
    $runtimeRecords = New-Object 'System.Collections.Generic.List[string]'
    foreach ($directory in @('components','decoders','profiles','ui','options')) {
        $baseDir = Join-Path $runtimeFolder $directory
        foreach ($f in Get-ChildItem -LiteralPath $baseDir -Recurse -File) {
            if ($f.Name -eq 'firmware_identity.h' -or $f.Extension.ToLowerInvariant() -notin @('.cpp','.h','.json','.html','.yaml','.py','.svg','.ico')) { continue }
            $relativeName = Relative $runtimeFolder $f.FullName
            $fileHash = (Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
            $runtimeRecords.Add(($relativeName + ' ' + $fileHash))
        }
    }
    # IMPORTANT: Sort-Object -CaseSensitive is culture-sensitive, NOT ordinal.
    # Its ordering varies with the user's Windows culture, so the same files
    # produced false "source changed" failures (v0.5.1 on nl-NL Windows).
    # Use an explicit ordinal order, identical to our independent SHA-256 tool.
    $sortedRecords = [string[]]$runtimeRecords.ToArray()
    [Array]::Sort($sortedRecords, [StringComparer]::Ordinal)
    $hashInput = [string]::Join("`n", $sortedRecords) + "`n"
    $digest = [Security.Cryptography.SHA256]::Create()
    try { $computed = -join ($digest.ComputeHash([Text.Encoding]::UTF8.GetBytes($hashInput)) | ForEach-Object { $_.ToString('x2') }) }
    finally { $digest.Dispose() }
    $computed = $computed.Substring(0,12)
    if ($computed -cne $hashMatch.Groups[1].Value) { Fail "Firmware source changed since $version was versioned (expected $($hashMatch.Groups[1].Value), actual $computed). Bump version and update hash before packaging; do not reuse a release number." }
    $decoderRoot = Join-Path $runtimeRoot 'rflink_universal\decoders'
    $packs = @(Get-ChildItem -LiteralPath $decoderRoot -Directory | Where-Object { -not $_.Name.StartsWith('_') })
    if ($packs.Count -lt 1) { Fail 'No decoder packs found.' }
    foreach ($id in @('acurite_609txc','rubicson_temp','fineoffset_wh2')) {
        if (-not (Test-Path -LiteralPath (Join-Path $decoderRoot ($id + '\decoder.cpp')) -PathType Leaf)) { Fail "Weather decoder missing: $id" }
        if (-not (Test-Path -LiteralPath (Join-Path $decoderRoot ($id + '\manifest.json')) -PathType Leaf)) { Fail "Weather manifest missing: $id" }
    }
    foreach ($pack in $packs) {
        $source = Join-Path $pack.FullName 'decoder.cpp'
        $manifestPath = Join-Path $pack.FullName 'manifest.json'
        if (-not [IO.File]::Exists($source) -or -not [IO.File]::Exists($manifestPath)) { Fail "Incomplete decoder pack: $($pack.Name)" }
        $data = Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($data.id -cne $pack.Name -or $data.schema -ne 2) { Fail "Invalid decoder pack manifest: $($pack.Name)" }
        if (@($data.capabilities).Count -eq 0) { Fail "Decoder pack has no capabilities: $($pack.Name)" }
        foreach ($protocol in @($data.capabilities)) {
            if (-not $protocol.protocol -or -not $protocol.transmit -or $protocol.transmit.default_enabled -ne $false) {
                Fail "Unsafe or incomplete decoder capability definition in $($pack.Name)"
            }
            if ($protocol.transmit.encoder -eq 'none' -and (@($protocol.transmit.actions).Count -gt 0 -or $protocol.transmit.status -cne 'disabled')) {
                Fail "Receive-only decoder has TX controls: $($pack.Name)"
            }
        }
    }
    # API v2B: a claimed TX encoder must also exist in its decoder pack and
    # a common dispatch engine must be packaged. Missing code must fail closed.
    $enginePath = Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\tx_engine.cpp'
    $engineHeader = Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\tx_engine.h'
    if (-not [IO.File]::Exists($enginePath) -or -not [IO.File]::Exists($engineHeader)) {
        Fail 'TX2 universal transmitter engine missing from selected runtime.'
    }
    foreach ($pack in $packs) {
        $manifest = Get-Content -LiteralPath (Join-Path $pack.FullName 'manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json
        $claimed = @($manifest.capabilities | Where-Object { $_.transmit.encoder -cne 'none' }).Count
        $encoderFile = Join-Path $pack.FullName 'tx_encoder.cpp'
        $registered = 0
        if ([IO.File]::Exists($encoderFile)) {
            $encoderCode = Get-Content -LiteralPath $encoderFile -Raw -Encoding UTF8
            $registered = [regex]::Matches($encoderCode, 'RFLINK_REGISTER_TX_ENCODER\(\s*[A-Za-z_][A-Za-z0-9_]*\s*\)').Count
        }
        if ($claimed -ne $registered) { Fail "TX encoder manifest/source mismatch: $($pack.Name)" }
    }
    $cpp = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\rflink_universal.cpp') -Raw -Encoding UTF8
    if (-not $cpp.Contains('// RFLINK_WEBHANDLER_LINK_HOTFIX_20260918')) { Fail 'WebHandler fix missing; stop packaging.' }
    $header = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\web_ui.h') -Raw -Encoding UTF8
    $html = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\ui\index.html') -Raw -Encoding UTF8
    $embedded = [regex]::Match($header, 'R"RFHTML\((.*?)\)RFHTML";', [Text.RegularExpressions.RegexOptions]::Singleline)
    $localeBundle = Read-Utf8 (Join-Path $runtimeRoot 'rflink_universal/ui/locales.bundle.json')
    $expectedHtml = $html.Replace('/*__RFLINK_LOCALES__*/', $localeBundle)
    if (-not $embedded.Success -or $embedded.Groups[1].Value -cne $expectedHtml) { Fail 'Embedded web UI differs from editable HTML or its locale bundle.' }
    # INCLUDEPATH1: ESPHome emits both generated radio catalogs into src/rflink_decoders.
    # A header that only includes its sibling by basename passes host tests but
    # fails the real ESP-IDF C++ build. Reject that incomplete release here.
    foreach ($entry in @(
        @('radio_transport_manifest.py', 'radio_transport_catalog_generated.h', 'radio_transport_policy.h'),
        @('radio_contract_v2.py', 'radio_v2_catalog_generated.h', 'radio_contract_v2.h')
    )) {
        $component = Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal'
        $generator = Read-Utf8 (Join-Path $component $entry[0])
        $generated = Read-Utf8 (Join-Path $component $entry[1])
        $expected = '../esphome/components/rflink_universal/' + $entry[2]
        if (-not $generator.Contains($expected) -or -not $generated.Contains($expected)) {
            Fail "Generated radio catalog include path invalid for ESPHome: $($entry[1])"
        }
    }
    # Routes and embedded images are release-critical: never ship a redirect
    # whose favicon was omitted by an updater/CUSTOM runtime.
    $iconSvg = Read-Utf8 (Join-Path $runtimeRoot 'rflink_universal/ui/favicon.svg')
    $iconIco = [IO.File]::ReadAllBytes((Join-Path $runtimeRoot 'rflink_universal/ui/favicon.ico'))
    if ($iconIco.Length -lt 6 -or $iconIco[0] -ne 0 -or $iconIco[1] -ne 0 -or $iconIco[2] -ne 1 -or $iconIco[3] -ne 0 -or
        -not $header.Contains($iconSvg) -or
        -not $header.Contains('RFLINK_FAVICON_ICO_SIZE') -or -not $html.Contains('href="/rflink/favicon.svg"') -or
        -not $cpp.Contains('request->redirect("/rflink")') -or
        -not $cpp.Contains('url == "/favicon.ico"') -or -not $cpp.Contains('url == "/rflink/favicon.svg"')) {
        Fail 'RFLink root redirect or favicon missing/mismatched in selected runtime.'
    }
    # PACKFIX1 / LOGBOOK1: static.038 belonged to the retired monitor title.
    # The current navigation still hosts the radio viewer at data-view="monitor",
    # but uses logbook.title and provides Radio/System/Web/All source filters.
    # Check the *current* UI contract instead of rejecting a valid new release.
    # Keep radio RX/TX engine checks and enforce the new system-trace API too.
    if (-not $html.Contains('data-i18n="logbook.title"') -or
        -not $html.Contains('data-view="monitor"') -or
        -not $html.Contains('id="logbookModes"') -or
        -not $html.Contains('id="systemLogLimit"') -or
        -not $html.Contains('id="webLogLimit"') -or
        -not $html.Contains('rx_ready') -or
        -not $cpp.Contains('record_tx_command') -or
        -not $cpp.Contains('channel_index') -or
        -not $cpp.Contains('/rflink/api/system-trace') -or
        -not $cpp.Contains('/rflink/api/system-trace-config')) {
        Fail 'LOGBOOK1 radio/system/web runtime missing or incomplete; do not package partial UI/API versions.'
    }
    if (-not $cpp.Contains('activity_export_json_') -or -not $cpp.Contains('noise_suspect') -or
        -not $cpp.Contains('full_frame_detail_') -or -not $html.Contains('id="monitorLogType"') -or
        -not $html.Contains('id="monitorCopy"') -or -not $html.Contains('id="monitorExport"') -or
        -not $html.Contains('/rflink/api/rx-log') -or
        -not $cpp.Contains('/rflink/api/rx-log-config')) {
        Fail 'RXLOG1 all-radio diagnostics, runtime limits or export missing; refusing partial release.'
    }
    if (-not $html.Contains('data-i18n="static.097"') -or -not $html.Contains('ledHeartbeat')) { Fail 'LED UI positive checkbox fix missing in selected runtime.' }
    if (-not $cpp.Contains('decoder')) { Fail 'Expected decoder code is missing.' }
    if (-not $cpp.Contains('Restored %u devices from NVS') -or -not $cpp.Contains('storage_ok') -or -not $html.Contains('id="deviceWizard"') -or -not $html.Contains('id="healthCard"')) { Fail 'UX device persistence or wizard/diagnostics missing.' }
    if (-not $cpp.Contains('unlink_identity(') -or -not $cpp.Contains('replace_identity(') -or -not $html.Contains('data-unlink-id') -or -not $html.Contains('data-replace-id')) { Fail 'UX2 signal management incomplete; refusing release.' }
    if (-not $cpp.Contains('ensure_decoder_packs_linked()') -or -not $cpp.Contains('Decoder registration mismatch:')) { Fail 'Decoder registration diagnostics missing.' }
    $linker = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\decoder_registry.cpp') -Raw -Encoding UTF8
    if (-not $linker.Contains('decoder_anchors_generated.h') -or -not $linker.Contains('RFLINK_DECODER_EXPECTED_COUNT')) { Fail 'Decoder linker fail-closed guard missing.' }
    if (-not $cpp.Contains('RFLINK_OPTIONAL_RADIOS_20260918')) { Fail 'Radio-optional fix missing from selected latest runtime. Re-run YAML updater or use clean project folder.' }
    $cppHeader = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\rflink_universal.h') -Raw -Encoding UTF8
    if (-not $cppHeader.Contains('get_setup_priority() const override { return setup_priority::WIFI - 1.0f; }') -or
        -not $cpp.Contains('RFLINK_BOOT_ORDER_FIX_20260918')) {
        Fail 'Boot fix missing: the RFLink web server may crash before WiFi/LwIP starts.'
    }
    $selectedYaml = Get-Content -LiteralPath $configSource -Raw -Encoding UTF8
    # Validate the actual selected YAML in both DEFAULT and CUSTOM modes.
    # WATCHDOG1 keeps the four-argument status call, but CC1101 availability is now
    # gated by the matching software watchdog health bit. Keep this check strict:
    # each radio must reference its own CC1101 and its own watchdog index exactly once.
    # Anchor these checks inside the active interval section, not in a comment.
    $intervalMatch = [regex]::Match($selectedYaml, '(?ms)^interval:[ \t]*\r?\n(.*?)(?=^[A-Za-z_][A-Za-z_0-9]*:|\z)')
    if (-not $intervalMatch.Success) { Fail 'Selected YAML has no active interval section for the missing-radio health poll.' }
    $radioHealthPatterns = @(
        'id\(rflink_core\)\.set_radio_available\(\s*0\s*,\s*!id\(cc1101_433_42\)\.is_failed\(\)\s*&&\s*id\(rflink_core\)\.radio_watchdog_healthy\(\s*0\s*\)\s*,\s*!id\(rx_433_42\)\.is_failed\(\)\s*,\s*!id\(tx_433_42\)\.is_failed\(\)\s*\);',
        'id\(rflink_core\)\.set_radio_available\(\s*1\s*,\s*!id\(cc1101_433_92\)\.is_failed\(\)\s*&&\s*id\(rflink_core\)\.radio_watchdog_healthy\(\s*1\s*\)\s*,\s*!id\(rx_433_92\)\.is_failed\(\)\s*,\s*!id\(tx_433_92\)\.is_failed\(\)\s*\);',
        'id\(rflink_core\)\.set_radio_available\(\s*2\s*,\s*!id\(cc1101_868\)\.is_failed\(\)\s*&&\s*id\(rflink_core\)\.radio_watchdog_healthy\(\s*2\s*\)\s*,\s*true\s*,\s*true\s*\);'
    )
    for ($radioIndex = 0; $radioIndex -lt $radioHealthPatterns.Count; $radioIndex++) {
        $count = [regex]::Matches($intervalMatch.Groups[1].Value, '(?m)^[ \t]+' + $radioHealthPatterns[$radioIndex] + '[ \t]*$').Count
        if ($count -ne 1) {
            Fail "Radio health poll invalid/missing for radio $radioIndex in selected YAML: expected exactly one active WATCHDOG1 four-argument set_radio_available(...) call (CC1101 watchdog/RX/TX)."
        }
    }
    # Optional MQTT must never silently compile with the example broker address.
    if ([regex]::IsMatch($selectedYaml, '(?m)^  ha_mqtt: !include rflink_universal/options/ha_mqtt\.yaml\s*$')) {
        $brokerMatch = [regex]::Match($selectedYaml, '(?m)^  mqtt_broker:\s*[\"'']?([^\"''#\r\n]+)')
        if (-not $brokerMatch.Success -or $brokerMatch.Groups[1].Value.Trim() -in @('VUL_IP_OF_HOSTNAAM_IN','CHANGE_ME','')) {
            Fail 'MQTT-brokeradres ontbreekt: vul mqtt_broker bovenaan in rflink-universal.yaml in voordat je HA/MQTT inschakelt.'
        }
        if (-not [regex]::IsMatch($selectedYaml, '(?m)^  mqtt_port:\s*\d+\s*$') -or
            -not [regex]::IsMatch($selectedYaml, '(?m)^  mqtt_username:\s*\S+')) {
            Fail 'MQTT poort of gebruikersnaam ontbreekt in de centrale substitutions.'
        }
    }
    # Validate the ACTUAL selected YAML, including user-edited or updater-merged GPIOs.
    # No copy of the earlier hardware mapping is used as the source of truth.
    $pinLabels = @('spi_sck','spi_mosi','spi_miso',
                   'radio1_csn','radio1_gdo0','radio1_gdo2',
                   'radio2_csn','radio2_gdo0','radio2_gdo2',
                   'radio3_csn','radio3_gdo0','radio3_gdo2','status_led_gpio')
    $pinValues = @()
    foreach ($key in $pinLabels) {
        $matchesForPin = [regex]::Matches($selectedYaml, '(?m)^  ' + [regex]::Escape($key) + ':\s*(GPIO\d+)(?:\s+#.*)?$')
        if ($matchesForPin.Count -ne 1) { Fail "Missing, duplicated or invalid central pin: $key" }
        $pinValues += $matchesForPin[0].Groups[1].Value
    }
    $selectedRevisionMatch = [regex]::Matches($selectedYaml, '(?m)^  board_revision:\s*([a-z][a-z0-9_]*)\s*$')
    if ($selectedRevisionMatch.Count -ne 1) { Fail 'PCB revision missing or duplicated in YAML.' }
    $selectedRevision = $selectedRevisionMatch[0].Groups[1].Value
    # Known PCB revisions are pinned to the physical GPIO map. Never accept a
    # known revision with arbitrary pins just to make a release pass.
    # custom_existing remains explicitly supported for reviewed user wiring.
    if ($selectedRevision -notin @('eigen_rfid_board_rev1','eigen_rfid_board_rev2_gpio14','custom_existing')) {
        Fail "Unknown PCB revision: $selectedRevision"
    }
    if ($UseUpdaterOutput -and $selectedRevision -cne $m.board_revision) { Fail 'Updater manifest board revision differs from selected YAML.' }
    if ($selectedRevision -eq 'eigen_rfid_board_rev1') {
        $expectedPins = @('GPIO12','GPIO11','GPIO13','GPIO5','GPIO6','GPIO4','GPIO9','GPIO8','GPIO10','GPIO15','GPIO16','GPIO17','GPIO48')
        if (($pinValues -join ',') -cne ($expectedPins -join ',')) { Fail 'Rev1 board label does not match physical HF4 reference pinout.' }
    }
    if ($selectedRevision -eq 'eigen_rfid_board_rev2_gpio14') {
        # RF1 433.42 CS=4 TX=5 RX=6; RF3 433.92 CS=9 TX=8 RX=14;
        # RF2 868 CS=15 IRQ=16 GDO2=17; SPI SCLK=12 MOSI=11 MISO=13.
        $expectedPins = @('GPIO12','GPIO11','GPIO13','GPIO4','GPIO5','GPIO6','GPIO9','GPIO8','GPIO14','GPIO15','GPIO16','GPIO17','GPIO48')
        if (($pinValues -join ',') -cne ($expectedPins -join ',')) {
            Fail 'Rev2 GPIO14 board label does not match the physical 2026-10-11 PCB pinout.'
        }
    }
    if (@($pinValues | Sort-Object -Unique).Count -ne $pinValues.Count) { Fail 'GPIO collision in central PCB pinout (including reserved GDO2 and LED).' }
    if ($pinValues -contains 'GPIO35' -or $pinValues -contains 'GPIO36') { Fail 'GPIO35/36 are unavailable with N16R8 octal PSRAM.' }
    $refs = @(
      '  clk_pin: ${spi_sck}', '  mosi_pin: ${spi_mosi}', '  miso_pin: ${spi_miso}',
      '    cs_pin: ${radio1_csn}', '    cs_pin: ${radio2_csn}', '    cs_pin: ${radio3_csn}',
      '    gdo0_pin: ${radio3_gdo0}',
      '      number: ${radio1_gdo2}', '      number: ${radio2_gdo2}',
      '    pin: ${radio1_gdo0}', '    pin: ${radio2_gdo0}'
    )
    foreach ($expected in $refs) {
        $count = @($selectedYaml -split '\r?\n' | Where-Object { $_ -ceq $expected }).Count
        if ($count -ne 1) { Fail "A radio/SPI connection must reference its central label exactly once: $expected" }
    }
    foreach ($line in ($selectedYaml -split '\r?\n')) {
        if ($line.TrimStart().StartsWith('#')) { continue }
        if ($line -match '(?<![A-Za-z0-9_])GPIO\d+' -and
            $line -notmatch '^  [a-zA-Z][a-zA-Z0-9_]*:\s*GPIO\d+(?:\s+#.*)?$') {
            Fail "Uncentralized GPIO number in YAML: $line"
        }
    }
    # Reject partial/broken YAML updates: count reservations AND verify each
    # individual transmitter, receiver and separate RGB package by ID.
    function Assert-RmtItem([string]$yaml, [string]$section, [string]$id, [int]$symbols) {
        $sectionPattern = '(?ms)^' + [regex]::Escape($section) + ':\r?\n(.*?)(?=^[A-Za-z_][A-Za-z_0-9]*:|\z)'
        $sectionMatch = [regex]::Match($yaml, $sectionPattern)
        if (-not $sectionMatch.Success) { Fail "RMT section missing: $section" }
        $itemPattern = '(?ms)^  - id:\s*' + [regex]::Escape($id) + '\s*\r?\n(.*?)(?=^  - id:|\z)'
        $itemMatch = [regex]::Match($sectionMatch.Groups[1].Value, $itemPattern)
        if (-not $itemMatch.Success -or
            -not [regex]::IsMatch($itemMatch.Groups[1].Value, '(?m)^    use_dma:\s*false\s*$') -or
            -not [regex]::IsMatch($itemMatch.Groups[1].Value, '(?m)^    rmt_symbols:\s*' + $symbols + '\s*$')) {
            Fail "RMT allocation missing or invalid for ${section}/${id}: expected non-DMA $symbols symbols."
        }
    }
    Assert-RmtItem $selectedYaml 'remote_receiver' 'rx_433_42' 96
    Assert-RmtItem $selectedYaml 'remote_receiver' 'rx_433_92' 96
    Assert-RmtItem $selectedYaml 'remote_transmitter' 'tx_433_42' 48
    Assert-RmtItem $selectedYaml 'remote_transmitter' 'tx_433_92' 48
    $rgbYaml = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\options\rgb_status_ws2812.yaml') -Raw -Encoding UTF8
    if (-not [regex]::IsMatch($rgbYaml, '(?m)^  - platform:\s*esp32_rmt_led_strip\s*$') -or
        -not [regex]::IsMatch($rgbYaml, '(?m)^    use_dma:\s*false\s*$') -or
        -not [regex]::IsMatch($rgbYaml, '(?m)^    rmt_symbols:\s*48\s*$')) {
        Fail 'RGB package missing 48-symbol non-DMA RMT TX allocation.'
    }
    # Radio 3 is SPI/packet-capable and must not consume ESP32 RMT resources.
    # Do NOT couple release packaging to an Itho-specific YAML profile here:
    # radio/protocol profiles may evolve independently from the packer. The
    # decoder/runtime tests validate Itho itself; this guard only protects the
    # RMT budget that originally motivated this release check.
    foreach ($rmtSection in @('remote_receiver','remote_transmitter')) {
        $rmtSectionMatch = [regex]::Match($selectedYaml, '(?ms)^' + $rmtSection + ':\r?\n(.*?)(?=^[A-Za-z_][A-Za-z_0-9]*:|\z)')
        if ($rmtSectionMatch.Success -and [regex]::IsMatch($rmtSectionMatch.Groups[1].Value, '(?m)^  - id:\s*(?:rx|tx)_868(?:_|\s|$)')) {
            Fail 'Radio 3/868 must not reserve ESP32 RMT resources; use the SPI packet path/profile instead.'
        }
    }
    # Fail closed if an updater's YAML would regress OTA LED visibility or the
    # selected RFLink runtime no longer surfaces missing radios to the LED.
    # ESPHome 2026.9: AddressableLightState is already a pointer. The older
    # get_output()->write_state(&id(...)) form fails the real C++ compilation.
    if (([regex]::Matches($selectedYaml, 'id\(rflink_status_rgb\)\.loop\(\);')).Count -ne 4 -or
        $selectedYaml.Contains('write_state(&id(')) {
        Fail 'OTA LED compile fix missing: require exactly four LightState::loop() callbacks; reject invalid write_state(&id(...)).'
    }
    $statusLedCode = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\status_led.h') -Raw -Encoding UTF8
    if (-not $statusLedCode.Contains('RADIO_MISSING') -or -not $cpp.Contains('led_.radio_missing(all_checked && any_unavailable)')) {
        Fail 'Missing-radio amber LED integration missing from selected runtime.'
    }
    if ($selectedYaml -match '(?m)^\s*status_led_gpio:\s*GPIO47\s*$') { Write-Warning 'Selected configuration uses GPIO47 for WS2812. The CH343P/N16R8 reference board normally uses GPIO48; keep GPIO47 only if your PCB jumper/wiring confirms it.' }
    # Match ESPHome's real StringRef API: c_str() exists, data() does not.
    if ($cpp.Contains('ref.data()') -or -not $cpp.Contains('std::string(ref.c_str(), ref.size())')) {
        Fail 'WebHandler StringRef compatibility regression: use ref.c_str(), not ref.data().'
    }
    # TX1 never ships remotely operable RF controls by default. A user can
    # enable them only in the central YAML AND per approved RFLink device.
    $txYaml = Read-Utf8 $configSource
    $txCore = Read-Utf8 (Join-Path $runtimeRoot 'rflink_universal/components/rflink_universal/rflink_universal.cpp')
    $txPolicy = Read-Utf8 (Join-Path $runtimeRoot 'rflink_universal/components/rflink_universal/tx_policy.h')
    if (-not $txYaml.Contains('ha_tx_controls:') -or
        -not $txYaml.Contains('tx_controls_enabled: ${ha_tx_controls}') -or
        -not $txCore.Contains('set_transmit_enabled(') -or
        -not $txCore.Contains('transmit_request(') -or
        -not $txPolicy.Contains('tx_lock_channel')) {
        Fail 'TX1 opt-in, frozen binding or MQTT TX control guard is missing.'
    }
    # HA2 must never advertise an observed-but-unapproved function to Home Assistant.
    # Treat the management gates and embedded UI as one release-critical feature.
    $storeCode = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\device_store.cpp') -Raw -Encoding UTF8
    $storeHeader = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\device_store.h') -Raw -Encoding UTF8
    $uiCode = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\ui\index.html') -Raw -Encoding UTF8
    $apiCode = Get-Content -LiteralPath (Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\rflink_universal.cpp') -Raw -Encoding UTF8
    foreach ($requiredText in @('propose_action_(device, result.action)', 'propose_fields_(device, result.fields)',
                                'bool DeviceStore::approve_action', 'bool DeviceStore::remove_field')) {
        if (-not $storeCode.Contains($requiredText)) { Fail "HA2 capability approval/removal guard missing: $requiredText" }
    }
    if (-not $storeHeader.Contains('PendingCapabilities') -or
        -not $apiCode.Contains('Only user-approved capability keys can leave RFLink') -or
        -not $apiCode.Contains('ha_remove_capability_') -or
        -not $uiCode.Contains('data-device-capabilities') -or
        -not $uiCode.Contains('wizardHaWrap') -or
        -not $uiCode.Contains('modalHaWrap')) {
        Fail 'HA2 approval, retained Discovery cleanup or device-wizard interface is incomplete.'
    }
    # COMPILEFIX2: prevent mixed old/new webhandler translation units.
    $obsoleteWeb = Join-Path $runtimeRoot 'rflink_universal\components\rflink_universal\rflink_web.cpp'
    if (Test-Path -LiteralPath $obsoleteWeb -PathType Leaf) {
        Fail 'Obsolete standalone rflink_web.cpp found. Remove the obsolete duplicate translation unit before building the release.'
    }
    [IO.Directory]::CreateDirectory($dist) | Out-Null
    foreach ($dir in @($srcStage,$bundleStage,$checkStage)) { [IO.Directory]::CreateDirectory($dir) | Out-Null }

    # SOURCE archive: only the single launcher, YAML, centrally organised docs, and source runtime.
    foreach ($file in @('rflink-universal.yaml','00_RFLink_START.bat')) { Copy-Stage $file $srcStage }
    $docsDir = Join-Path $root 'docs'
    Assert-Directory $docsDir 'central docs folder'
    foreach ($item in Get-ChildItem -LiteralPath $docsDir -Recurse -File -Force) {
        $relative = Relative $root $item.FullName
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { Fail "Symlink/reparse point forbidden: $relative" }
        if (-not (Forbidden $relative)) { Copy-Stage $relative $srcStage }
    }
    $sourceDir = Join-Path $root 'rflink_universal'
    foreach ($item in Get-ChildItem -LiteralPath $sourceDir -Recurse -File -Force) {
        $subrel = Relative $root $item.FullName
        # The latest update owns ALL runtime files, not a mix with older files.
        if ($selectedUpdate -and ($subrel.StartsWith('rflink_universal/components/') -or
            $subrel.StartsWith('rflink_universal/decoders/') -or
            ($subrel -match '^rflink_universal/(options|packages)/.*\.yaml$') -or
            ($subrel -match '^rflink_universal/ui/.*\.(html|css|js|json|svg|ico)$'))) { continue }
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { Fail "Symlink/reparse point forbidden: $($item.FullName)" }
        $relative = Relative $root $item.FullName
        if (-not (Forbidden $relative)) { Copy-Stage $relative $srcStage }
    }
    if ($selectedUpdate) {
        foreach ($item in Get-ChildItem -LiteralPath (Join-Path $runtimeRoot 'rflink_universal') -Recurse -File -Force) {
            $rel = Relative $runtimeRoot $item.FullName
            if ($rel -in @($m.runtime_sha256.PSObject.Properties | ForEach-Object { $_.Name })) { Copy-Stage $rel $srcStage }
        }
    }
    if ((All-Files $srcStage).Count -lt 60) { Fail 'Too few project files; source package incomplete.' }

    # Bundle files are a strict whitelist; never add real secrets.yaml or a build cache.
    Copy-Stage 'rflink-universal.yaml' $bundleStage
    $runtimePaths = New-Object System.Collections.Generic.List[string]
    $runtimePaths.Add('rflink-universal.yaml')
    $extensions = @('.py','.cpp','.c','.h','.hpp','.inc','.tcc','.json','.yaml')
    foreach ($subdir in @('components','decoders')) {
        $source = Join-Path $runtimeRoot ('rflink_universal\' + $subdir)
        foreach ($item in Get-ChildItem -LiteralPath $source -Recurse -File -Force) {
            $rel = Relative $runtimeRoot $item.FullName
            if (Forbidden $rel -or $rel -match '^rflink_universal/decoders/_[^/]+/') { continue }
            if ($extensions -notcontains $item.Extension.ToLowerInvariant()) { continue }
            if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { Fail "Symlink/reparse point forbidden: $rel" }
            Copy-Stage $rel $bundleStage
            $runtimePaths.Add($rel)
        }
    }
    foreach ($subdir in @('options','packages','ui')) {
        $source = Join-Path $runtimeRoot ('rflink_universal\' + $subdir)
        if (-not (Test-Path -LiteralPath $source -PathType Container)) { continue }
        foreach ($item in Get-ChildItem -LiteralPath $source -Recurse -File -Force) {
            $rel = Relative $runtimeRoot $item.FullName
            if (Forbidden $rel) { continue }
            if ($subdir -eq 'ui' -and $item.Extension.ToLowerInvariant() -notin @('.html','.css','.js','.json','.svg','.ico')) { continue }
            if ($subdir -ne 'ui' -and $item.Extension.ToLowerInvariant() -ne '.yaml') { continue }
            Copy-Stage $rel $bundleStage
            $runtimePaths.Add($rel)
        }
    }
    $manifest = [ordered]@{
        manifest_version = 1
        esphome_version = 'unknown'
        config_filename = 'rflink-universal.yaml'
        config_dir = '.'
        files = @($runtimePaths | Sort-Object)
        has_secrets = $false
    }
    $json = ConvertTo-Json -InputObject $manifest -Depth 10
    [IO.File]::WriteAllText((Join-Path $bundleStage 'manifest.json'), $json + "`n", (New-Object Text.UTF8Encoding($false)))
    if ((All-Files $bundleStage).Count -ne ($runtimePaths.Count + 1)) { Fail 'Bundle file inventory is inconsistent.' }

    $zipTemp = Join-Path $temp 'source.zip'
    [IO.Compression.ZipFile]::CreateFromDirectory($srcStage, $zipTemp, [IO.Compression.CompressionLevel]::Optimal, $false)
    $zipCheck = Join-Path $checkStage 'source'
    [IO.Directory]::CreateDirectory($zipCheck) | Out-Null
    [IO.Compression.ZipFile]::ExtractToDirectory($zipTemp, $zipCheck)
    Check-Identical $srcStage $zipCheck
    $bundleTemp = Join-Path $temp 'device.esphomebundle.tar.gz'
    # List the archive roots explicitly: tar -C DIR . would name ./manifest.json,
    # which fails ESPHome's exact getmember('manifest.json') lookup.
    & tar.exe -czf $bundleTemp -C $bundleStage manifest.json rflink-universal.yaml rflink_universal
    if ($LASTEXITCODE -ne 0) { Fail 'Windows tar.exe failed creating bundle.' }
    $bundleCheck = Join-Path $checkStage 'bundle'
    [IO.Directory]::CreateDirectory($bundleCheck) | Out-Null
    & tar.exe -xzf $bundleTemp -C $bundleCheck
    if ($LASTEXITCODE -ne 0) { Fail 'Windows tar.exe failed verifying bundle.' }
    Check-Identical $bundleStage $bundleCheck
    $parsed = Get-Content -LiteralPath (Join-Path $bundleCheck 'manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($parsed.config_filename -cne 'rflink-universal.yaml' -or $parsed.manifest_version -ne 1 -or $parsed.has_secrets) {
        Fail 'Bundle manifest failed validation.'
    }
    $listed = @($parsed.files | Sort-Object)
    $actual = @(All-Files $bundleCheck | Where-Object { $_ -ne 'manifest.json' } | Sort-Object)
    if (($listed -join "`n") -cne ($actual -join "`n")) { Fail 'Bundle manifest does not match payload.' }
    # Publish only after BOTH archives and manifest passed exhaustive verification.
    Move-Item -LiteralPath $zipTemp -Destination $zipOut -Force
    Move-Item -LiteralPath $bundleTemp -Destination $bundleOut -Force
    if ($selectedUpdate) { Write-Host ('CONFIRMED CUSTOM: newest YAML/runtime came from ' + $selectedUpdate) }
    else { Write-Host 'CONFIRMED DEFAULT: only project YAML and runtime used; updater outputs ignored.' }
    Write-Host ('SUCCESS: source project: ' + $zipOut)
    Write-Host ('SUCCESS: ESPHome initial installation AND updates: ' + $bundleOut)
    Write-Host ('SHA256 ZIP: ' + (Get-FileHash -LiteralPath $zipOut -Algorithm SHA256).Hash)
    Write-Host ('SHA256 BUNDLE: ' + (Get-FileHash -LiteralPath $bundleOut -Algorithm SHA256).Hash)
    Write-Host 'No Python used. No files uploaded. Your existing ESPHome device is not changed.'
    Write-Host 'See docs/INSTALLEREN_EN_UPDATEN.md. Import does not flash firmware automatically.'
    exit 0
}
catch {
    $line = $_.InvocationInfo.ScriptLineNumber
    $kind = $_.Exception.GetType().Name
    [Console]::Error.WriteLine(('PACKAGING FAILED [{0}, line {1}]: {2}' -f $kind,$line,$_.Exception.Message))
    if ($root.Length -gt 140) { [Console]::Error.WriteLine('TIP: project path is long. Extract the SOURCE ZIP to C:\RFLink and rerun 00_RFLink_START.bat (clean release).') }
    exit 1
}
finally {
    if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue }
}
