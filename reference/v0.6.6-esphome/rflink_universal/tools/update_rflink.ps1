# Safe RC4 YAML update: Windows PowerShell 5.1 and built-in tar.exe/.NET ZIP only.
# This is deliberately NOT a general-purpose YAML parser. It merges complete top-level
# ESPHome blocks and individual substitution keys via a conservative three-way merge.
[CmdletBinding()]
param(
    [string]$ProjectRoot = '',
    [string]$ExistingYaml = '',
    [string]$NewRelease = '',
    [string]$BaseYaml = '',
    [ValidateSet('Normal','Advanced','Automatic')][string]$Mode = 'Normal',
    [string]$HardwareChoice = '',
    [string]$ExistingRuntime = '',
    [switch]$AcceptReleaseFirmwareChanges,
    [switch]$NonInteractive
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Utf8 = New-Object Text.UTF8Encoding($false)
if (-not $ProjectRoot) { $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path }
$root = (Resolve-Path -LiteralPath $ProjectRoot).Path.TrimEnd('\','/')
$temporary = Join-Path ([IO.Path]::GetTempPath()) ('rflink-update-' + [guid]::NewGuid().ToString('N'))
$outputRoot = Join-Path $root 'update_output'
$failureMarker = Join-Path $outputRoot 'LAST_UPDATE_FAILED.txt'
# The current RELEASE hardware profile is rev2; historical rev1 sources are still valid EXISTING boards.
$expectedBoardRevision = 'eigen_rfid_board_rev2_gpio14'
$output = $null

function Fail([string]$message) { throw $message }
function Path-Relative([string]$base, [string]$path) { return $path.Substring($base.Length + 1).Replace('\','/') }
function Read-Utf8([string]$path) {
    $bytes = [IO.File]::ReadAllBytes($path)
    $dec = New-Object Text.UTF8Encoding($false,$true)
    return $dec.GetString($bytes).TrimStart([char]0xFEFF).Replace("`r`n","`n").Replace("`r","`n")
}
function Write-Utf8([string]$path,[string]$contents) {
    $dir = [IO.Path]::GetDirectoryName($path)
    if (-not [IO.Directory]::Exists($dir)) { [IO.Directory]::CreateDirectory($dir) | Out-Null }
    [IO.File]::WriteAllText($path, $contents, $Utf8)
}
function File-Hash([string]$p) { return (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash }
function Version-FromText([string]$text) {
    $m = [regex]::Match($text,'(?im)^#[^\n]*?v([0-9]+\.[0-9]+\.[0-9]+)[ -]RC([0-9]+)-HF([0-9]+)')
    if (-not $m.Success) { return '' }
    return ('v' + $m.Groups[1].Value + '-RC' + $m.Groups[2].Value + '-HF' + $m.Groups[3].Value)
}
# A reviewed, older HF3 radio-optional configuration existed before the final
# UX2 RMT TX + OTA fixes. The exact structural template was reconstructed from
# the user's failing YAML, with all six personal substitution values replaced.
# ONLY name/friendly_name and Itho ID may differ from that exact reference.
# Do not accept a version header or a few feature markers as proof of identity.
function Legacy-HF3-Canonical([string]$text) {
    $cut = $text.IndexOf("`npackages:", [StringComparison]::Ordinal)
    if ($cut -lt 0) { return '' }
    $before = $text.Substring(0,$cut)
    $after = $text.Substring($cut)
    $pattern = '(?m)^(  (?:name|friendly_name|itho_id1|itho_id2|itho_id3|itho_device_type):)[^\n]*$'
    if ([regex]::Matches($before,$pattern).Count -ne 6) { return '' }
    $before = [regex]::Replace($before,$pattern,'$1 __PERSONAL_SETTING__')
    return ($before + $after)
}
function Text-Sha256([string]$text) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [Text.Encoding]::UTF8.GetBytes($text)
        return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','')
    } finally { $sha.Dispose() }
}
function Resolve-KnownBaseline([string]$version,[string]$oldText) {
    # Only baselines copied verbatim from the archived, complete UX2 SOURCE ZIPs.
    # Do not infer a baseline from a version label alone: other HF3 variants exist.
    $catalog = @{
        'v0.3.3-RC4-HF3' = @('v0.3.3-RC4-HF3_UX2.yaml','F1F9174559CFB8863989F0F24923384C55A21DA4562B03E941392B50D0D3DF4E')
        'v0.3.3-RC4-HF4' = @('v0.3.3-RC4-HF4_UX2.yaml','22A4BF525499043A53AF943956BD734277B5A951A650DBE7F60227D481681879')
    }
    if (-not $catalog.ContainsKey($version)) { return '' }
    # Distinguish real archived variants by their COMPLETE structure first.
    # A version string or feature marker is not reliable: partially-updated
    # configurations can contain the newer markers but still have older blocks.
    $legacyName = 'v0.3.3-RC4-HF3_LEGACY_REVIEWED.yaml'
    $legacyHash = '5EEC740606CC9D85D0547F527B16A7B18219BF73AF2E3B8188F8209F36F974FC'
    $legacyFile = Join-Path $root ('rflink_universal\maintenance\baselines\' + $legacyName)
    $canonical = Legacy-HF3-Canonical $oldText
    if ($version -ceq 'v0.3.3-RC4-HF3' -and $canonical -and (Text-Sha256 $canonical) -ceq $legacyHash) {
        if (-not [IO.File]::Exists($legacyFile) -or (File-Hash $legacyFile) -cne $legacyHash) {
            Fail 'Reviewed legacy HF3 reference is missing/modified. Re-extract full SOURCE ZIP.'
        }
        Write-Warning 'Full legacy HF3 profile verified. New RMT/OTA/radio logic comes from HF4; only supported personal settings migrate.'
        return $legacyFile
    }
    # Decoder API v2B/v2C retained the same numeric HF4 header AND TX1 marker,
    # but changed the rflink_universal block. Selecting TX1 as the baseline
    # creates a false three-way conflict on a completely stock v2B->v2C update.
    # Exact historical immutable YAMLs are bundled only in the SOURCE archive.
    if ($version -ceq 'v0.3.3-RC4-HF4' -and $oldText.Contains('raw_transmitter_43342: tx_433_42')) {
        if ($oldText.Contains('# Decoder API v2C: reviewed RAW encoders; opt-in per device')) {
            $file = 'v0.3.3-RC4-HF4_DECODERAPI2C.yaml'
            $hash = '9EDB3EAF8F2DC3867936D8E737037E8153F53151BB388A5D6707F8E9A0D22886'
        } elseif ($oldText.Contains('# TX2: Nexa + reviewed Byron; opt-in per device')) {
            $file = 'v0.3.3-RC4-HF4_DECODERAPI2B.yaml'
            $hash = '3F546C483866A3AC25FB290A1C72F778A8DA2BE4742A3FF5A060634C93CE64BB'
        } else {
            Fail 'Unknown Decoder API v2 profile: supply an exact original YAML baseline instead of assuming TX1.'
        }
        $stockYaml = Join-Path $root ('rflink_universal\maintenance\baselines\' + $file)
        if (-not [IO.File]::Exists($stockYaml) -or (File-Hash $stockYaml) -cne $hash) {
            Fail "Verified Decoder API profile missing/modified: $file. Re-extract the full fixed SOURCE ZIP."
        }
        return $stockYaml
    }
    # TX1 ships under HF4 header too. Never mistake the TX1 option/tx binding
    # for a CONFIG1 personal edit, or a future merge could discard TX logic.
    if ($version -ceq 'v0.3.3-RC4-HF4' -and $oldText.Contains('# TX1_RELEASE_PROFILE: controlled_nexa_43392')) {
        $txBase = Join-Path $root 'rflink_universal\maintenance\baselines\v0.3.3-RC4-HF4_TX1.yaml'
        if (-not [IO.File]::Exists($txBase) -or (File-Hash $txBase) -cne 'A012633BD5E3FDB3AB8424B218958340C9A2A9838D57B5C7BF741702DDE6AEBA') {
            Fail 'Verified TX1 baseline missing/modified. Re-extract full SOURCE ZIP.'
        }
        return $txBase
    }
    # CONFIG1 retains the HF4 numeric header; match this distinct audited YAML
    # BEFORE the generic HA1 marker. Never choose a wrong three-way baseline.
    if ($version -ceq 'v0.3.3-RC4-HF4' -and $oldText.Contains('# CONFIG1: centrale gebruikersinstellingen;')) {
        $configBase = Join-Path $root 'rflink_universal\maintenance\baselines\v0.3.3-RC4-HF4_CONFIG1.yaml'
        if (-not [IO.File]::Exists($configBase) -or (File-Hash $configBase) -cne '5039F2558DB37C6258906461B81E523334681566EB5AB49C5D3B1159EA950DEE') {
            Fail 'Verified CONFIG1 baseline missing/modified. Re-extract full SOURCE ZIP.'
        }
        return $configBase
    }
    # HA1 shares HF4's numeric header but has a DIFFERENT YAML baseline.
    # Version+board revision alone must never be treated as an identity proof.
    if ($version -ceq 'v0.3.3-RC4-HF4' -and $oldText.Contains('# HOME_ASSISTANT_BRIDGE1:')) {
        $haBase = Join-Path $root 'rflink_universal\maintenance\baselines\v0.3.3-RC4-HF4_HA1.yaml'
        if (-not [IO.File]::Exists($haBase) -or (File-Hash $haBase) -cne '7A9D8688292D763FEF6C92D95F214584886966335D8C199CAF2503C57F2E9A9F') {
            Fail 'Verified HA1 YAML baseline missing/modified. Re-extract the full SOURCE ZIP.'
        }
        return $haBase
    }
    # Keep old verified historical baselines immutable. Compilefix1 is a new
    # known YAML variant with a separate pinned checksum. This also lets the
    # updater migrate future installations of this corrected release safely.
    $isCompilefix1 = ($version -ceq 'v0.3.3-RC4-HF4' -and
        $oldText.Contains('# RADIO-OPTIONAL UPDATE:') -and
        $oldText.Contains('id(rflink_status_rgb).loop();') -and
        $oldText.Contains('rmt_symbols: 96') -and
        $oldText.Contains('set_radio_available(0, !id(cc1101_433_42).is_failed(),'))
    if ($isCompilefix1) {
        $fixedFile = Join-Path $root 'rflink_universal\maintenance\baselines\v0.3.3-RC4-HF4_COMPILEFIX1.yaml'
        if (-not [IO.File]::Exists($fixedFile) -or (File-Hash $fixedFile) -cne 'D4BB34C1E23A3E7EBACF31D3F6C26B32DCAAB86426E539DCD575BC01A656B762') {
            Fail 'Verified compilefix1 baseline missing/modified. Re-extract full SOURCE ZIP.'
        }
        return $fixedFile
    }
    $isModernUx2 = ($oldText.Contains('# RADIO-OPTIONAL UPDATE:') -and
        $oldText.Contains('rmt_symbols: 96') -and
        $oldText.Contains('led_ota(true)') -and
        $oldText.Contains('get_output()->write_state(&id(rflink_status_rgb))') -and
        $oldText.Contains('set_radio_available(0, !id(cc1101_433_42).is_failed(),'))
    if (-not $isModernUx2) {
        Fail "Existing $version is not an exact known legacy HF3 profile or recognized UX2 variant. Supply its exact unmodified baseline; do not guess."
    }
    $folder = Join-Path $root 'rflink_universal\maintenance\baselines'
    $candidate = Join-Path $folder $catalog[$version][0]
    if (-not [IO.File]::Exists($candidate)) { Fail "Historical baseline for $version is missing from the SOURCE package. Re-extract the full ZIP." }
    if ((File-Hash $candidate) -cne $catalog[$version][1]) { Fail "Historical baseline for $version differs from the verified original. Re-extract the full SOURCE ZIP." }
    if ((Version-FromText (Read-Utf8 $candidate)) -cne $version) { Fail "Historical baseline for $version has a wrong release header." }
    return $candidate
}
function File-Safe([string]$relative) {
    if ($relative -notmatch '^[a-zA-Z0-9_./-]+$' -or $relative.StartsWith('/') -or $relative.Contains('..') -or $relative.Contains('//')) { return $false }
    return $true
}
function Check-ArchiveEntry([string]$entry) {
    $name = $entry.Replace('\','/').TrimEnd('/')
    if (-not (File-Safe $name)) { Fail "Unsafe archive entry: $entry" }
}
function Extract-Zip([string]$path,[string]$dest) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead($path)
    $seen = New-Object -TypeName 'System.Collections.Generic.HashSet[string]' -ArgumentList ([StringComparer]::OrdinalIgnoreCase)
    try {
        foreach ($entry in $zip.Entries) {
            Check-ArchiveEntry $entry.FullName
            if (-not $seen.Add($entry.FullName)) { Fail "Duplicate archive entry: $($entry.FullName)" }
            $unixMode = ($entry.ExternalAttributes -shr 16) -band 61440
            if ($unixMode -eq 40960) { Fail "Archive contains symlink: $($entry.FullName)" }
            if ($entry.FullName.EndsWith('/')) { continue }
            $target = [IO.Path]::GetFullPath((Join-Path $dest $entry.FullName.Replace('/',[IO.Path]::DirectorySeparatorChar)))
            if (-not $target.StartsWith(($dest.TrimEnd('\','/') + [IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)) { Fail 'ZIP path escaped extraction directory.' }
            $folder = [IO.Path]::GetDirectoryName($target)
            [IO.Directory]::CreateDirectory($folder) | Out-Null
            $stream = $entry.Open()
            $out = [IO.File]::Create($target)
            try { $stream.CopyTo($out) } finally { $out.Dispose(); $stream.Dispose() }
        }
    } finally { $zip.Dispose() }
}
function Extract-Tar([string]$path,[string]$dest) {
    if (-not (Get-Command tar.exe -ErrorAction SilentlyContinue)) { Fail 'Windows tar.exe missing.' }
    $names = @(& tar.exe -tzf $path)
    if ($LASTEXITCODE -ne 0) { Fail 'Invalid tar.gz archive.' }
    foreach ($name in $names) { Check-ArchiveEntry $name }
    $details = @(& tar.exe -tvzf $path)
    if ($LASTEXITCODE -ne 0 -or $details.Count -ne $names.Count) { Fail 'Could not inspect tar.gz entries.' }
    foreach ($detail in $details) { if ($detail -and $detail[0] -notin @('-','d')) { Fail 'Archive contains a link or nonstandard entry.' } }
    & tar.exe -xzf $path -C $dest
    if ($LASTEXITCODE -ne 0) { Fail 'Archive extraction failed.' }
}
# Header regex intentionally requires zero-indent mapping keys. A YAML directive or an
# unsupported unindented line is a hard error, never silently merged/discarded.
function Parse-Blocks([string]$text,[int]$indent) {
    $lines = $text.Replace("`r`n","`n").TrimEnd("`n").Split("`n")
    $re = '^' + (' ' * $indent) + '([a-zA-Z_][a-zA-Z0-9_.-]*):(?:.*)$'
    $preamble = New-Object 'System.Collections.Generic.List[string]'
    $order = New-Object 'System.Collections.Generic.List[string]'
    $blocks = @{}
    $key = $null
    $buffer = New-Object 'System.Collections.Generic.List[string]'
    foreach ($line in $lines) {
        if ($line -match $re) {
            if ($null -ne $key) { $blocks[$key] = ($buffer -join "`n").TrimEnd("`n"); $buffer.Clear() }
            $key = $Matches[1]
            if ($blocks.ContainsKey($key) -or $order.Contains($key)) { Fail "Duplicate YAML section/key: $key" }
            $order.Add($key)
            $buffer.Add($line)
        } elseif ($null -eq $key) {
            if ($line.Trim() -and -not $line.TrimStart().StartsWith('#')) { Fail "Unsupported YAML before first section: $line" }
            $preamble.Add($line)
        } else {
            if ($indent -eq 0 -and $line -match '^[^\s#]' -and $line.Trim()) { Fail "Unsupported top-level YAML syntax: $line" }
            if ($indent -eq 2 -and $line -match '^\s{0,1}[^\s#]' -and $line.Trim()) { Fail "Unsupported substitution syntax: $line" }
            $buffer.Add($line)
        }
    }
    if ($null -ne $key) { $blocks[$key] = ($buffer -join "`n").TrimEnd("`n") }
    return @{ Order = @($order.ToArray()); Blocks = $blocks; Preamble = ($preamble -join "`n") }
}
function Comparable($value) {
    if ($null -eq $value) { return $null }
    return ($value.Replace("`r`n","`n").TrimEnd([char[]]@([char]10,[char]32,[char]9)))
}
# Comments between top-level blocks belong to the preceding block in our
# minimal parser. They must never be mistaken for user firmware changes.
function Executable-Lines($value) {
    if ($null -eq $value) { return $null }
    $lines = @($value.Replace("`r`n","`n").Split("`n") | Where-Object {
        $_.Trim() -and -not $_.TrimStart().StartsWith('#')
    } | ForEach-Object { $_.TrimEnd() })
    return ($lines -join "`n")
}
$conflicts = New-Object 'System.Collections.Generic.List[string]'
$decisions = New-Object 'System.Collections.Generic.List[string]'
$hardwareKeys = @('spi_sck','spi_mosi','spi_miso',
                  'radio1_csn','radio1_gdo0','radio1_gdo2',
                  'radio2_csn','radio2_gdo0','radio2_gdo2',
                  'radio3_csn','radio3_gdo0','radio3_gdo2','status_led_gpio')
$script:selectedHardware = ''
$script:hardwareProfile = ''
$script:preserveIdentity = $true
function Read-YN([string]$message, $defaultChoice) {
    if ($NonInteractive -or $Mode -eq 'Automatic') {
        if ($null -eq $defaultChoice) { Fail "Cannot choose automatically: $message" }
        return [bool]$defaultChoice
    }
    $suffix = if ($null -eq $defaultChoice) { '[y/n]' } elseif ($defaultChoice) { '[Y/n]' } else { '[y/N]' }
    while ($true) {
        $answer = (Read-Host "$message $suffix").Trim().ToLowerInvariant()
        if (-not $answer -and $null -ne $defaultChoice) { return [bool]$defaultChoice }
        if ($answer -in @('y','yes','j','ja')) { return $true }
        if ($answer -in @('n','no','nee')) { return $false }
        Write-Host 'Vul Y of N in. Enter is alleen toegestaan wanneer een standaardkeuze is getoond.'
    }
}
function Sub-Value($section,[string]$key) {
    if (-not $section -or -not $section.Blocks.ContainsKey($key)) { return '' }
    $line = $section.Blocks[$key].Split("`n")[0]
    if ($line -match '^\s+[\w-]+:\s*([^#\r\n]+)') { return $Matches[1].Trim().Trim([char]39,[char]34) }
    return ''
}
function Assert-PinTable($section,[string]$what) {
    $vals = New-Object 'System.Collections.Generic.List[string]'
    foreach ($key in $hardwareKeys) {
        $value = Sub-Value $section $key
        if ($value -notmatch '^GPIO(?:[0-9]|[1-3][0-9]|4[0-8])$') { Fail "$what missing/invalid central pin $key ($value)" }
        if ($value -in @('GPIO35','GPIO36')) { Fail "$what uses N16R8 reserved PSRAM pin $value" }
        if ($vals.Contains($value)) { Fail "$what has duplicate GPIO assignment $value" }
        $vals.Add($value)
    }
}
function Same-Pins($a,$b) {
    foreach ($key in $hardwareKeys) {
        $left=Sub-Value $a $key; $right=Sub-Value $b $key
        if (-not $left -or -not $right -or $left -cne $right) { return $false }
    }
    return $true
}
function Choose-Hardware($old,$new) {
    if ($HardwareChoice -and $HardwareChoice -notin @('Release','Existing','Unknown')) { Fail 'HardwareChoice must be Release, Existing or Unknown.' }
    Assert-PinTable $new 'Release'
    $newRev = Sub-Value $new 'board_revision'
    if ($newRev -cne $expectedBoardRevision) { Fail "Unknown release PCB revision: $newRev. Verify code and board before release." }
    $oldRev = Sub-Value $old 'board_revision'
    $pinsEqual = Same-Pins $old $new
    if (-not $HardwareChoice) {
        if ($pinsEqual) { $script:selectedHardware = 'Release' }
        elseif ($oldRev -ceq $newRev) {
            # Same known PCB revision: release wiring is the default, but display any delta.
            $useNew = Read-YN 'GPIO-pinout uit nieuwe release gebruiken (zelfde PCB-revisie)?' $true
            $script:selectedHardware = if ($useNew) { 'Release' } else { 'Existing' }
        } else {
            # A filename / YAML is not proof of the PHYSICAL board. No unsafe default.
            if ($Mode -eq 'Automatic' -or $NonInteractive) {
                Fail 'Unknown or different PCB revision with changed GPIOs. Specify -HardwareChoice Release or Existing after checking actual wiring.'
            }
            Write-Warning "Bestaande PCB-revisie: $(if ($oldRev) {$oldRev} else {'ONBEKEND'}); nieuwe: $newRev. Controleer fysieke aansluitingen."
            $useNew = Read-YN 'Past de PCB die je echt gebruikt bij de pinout van deze release?' $null
            $script:selectedHardware = if ($useNew) { 'Release' } else { 'Existing' }
        }
    } else {
        if ($HardwareChoice -eq 'Unknown') { Fail 'PCB unknown: no firmware bundle may be built before selecting physical wiring.' }
        $script:selectedHardware = $HardwareChoice
        $decisions.Add("HARDWARE_EXPLICIT: $HardwareChoice (hardware choice supplied by operator)")
    }
    if ($script:selectedHardware -eq 'Existing') {
        Assert-PinTable $old 'Existing hardware'
        if ($pinsEqual) { $script:selectedHardware = 'Release' }
    }
    $script:hardwareProfile = if ($script:selectedHardware -eq 'Release') { $newRev } else { 'custom_existing' }
    $decisions.Add("BOARD_PROFILE $($script:hardwareProfile); hardware_source=$($script:selectedHardware)")
}

function Choose-Block([string]$label,$base,$old,$new) {
    $b = Comparable $base; $o = Comparable $old; $n = Comparable $new
    if ($o -ceq $n) { $decisions.Add("UNCHANGED $label"); return $new }
    if ($o -ceq $b) { $decisions.Add("NEW $label"); return $new }
    if ($n -ceq $b) { $decisions.Add("USER $label"); return $old }
    if ($null -eq $b -and $null -eq $o) { $decisions.Add("ADDED $label"); return $new }
    if ($null -eq $b -and $null -eq $n) { $decisions.Add("USER_ADDED $label"); return $old }
    # The effective external component is identical, only a pinout comment
    # moved between releases. Keep the current comments and path from release.
    if ($label -eq 'external_components' -and (Executable-Lines $old) -ceq (Executable-Lines $new)) {
        $decisions.Add('RELEASE external_components (same actual source/path; comments only differ)')
        return $new
    }
    # Only the exact HA1 opt-in line may be carried from the old packages block.
    # Never accept other custom package sources or uncommented unknown includes.
    if ($label -eq 'packages' -and $null -ne $b -and $null -ne $o -and $null -ne $n) {
        $enabled = '  ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml'
        $disabled = '  # ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml'
        if ($o.Contains($enabled) -and $b.Contains($disabled) -and $n.Contains($disabled)) {
            $disabledOld = $o.Replace($enabled,$disabled)
            if ((Executable-Lines $disabledOld) -ceq (Executable-Lines $b) -and
                (Executable-Lines $disabledOld) -ceq (Executable-Lines $n)) {
                $decisions.Add('MIGRATE_MQTT: operator-enabled HA1 package stays enabled; no additional packages imported')
                return $n.Replace($disabled,$enabled)
            }
        }
    }
    # A documented earlier HF3 transmitter block can occur in partially-updated
    # projects. Accept ONLY this exact audited historical block, not an arbitrary
    # custom transmitter; restore the HF4 RMT budget and monitor hooks.
    if ($label -eq 'remote_transmitter') {
        $legacyFile = Join-Path $root 'rflink_universal\maintenance\baselines\v0.3.3-RC4-HF3_LEGACY_REVIEWED.yaml'
        if ([IO.File]::Exists($legacyFile) -and
            (File-Hash $legacyFile) -ceq '5EEC740606CC9D85D0547F527B16A7B18219BF73AF2E3B8188F8209F36F974FC') {
            $historic = Parse-Blocks (Read-Utf8 $legacyFile) 0
            if ((Executable-Lines $old) -ceq (Executable-Lines $historic.Blocks['remote_transmitter'])) {
                $decisions.Add('KNOWN_LEGACY_TX: restored HF4 transmitter (RMT 48-symbol configuration and radio monitor hooks)')
                return $new
            }
        }
    }
    if ($label -in @('external_components','remote_transmitter')) {
        Write-Warning "Both old installation and new release modified $label relative to historical baseline."
        if ($label -eq 'external_components') {
            Write-Warning 'Using release removes any additional/changed custom external component sources from this YAML.'
        } else {
            Write-Warning 'Using release replaces custom transmitter code with tested HF4 GPIO/RMT/monitor logic.'
        }
        $takeRelease = $false
        if ($AcceptReleaseFirmwareChanges) {
            $takeRelease = $true
            $decisions.Add("EXPLICIT_FIRMWARE_OVERRIDE ${label}: command line -AcceptReleaseFirmwareChanges")
        } elseif (-not $NonInteractive -and $Mode -ne 'Automatic') {
            # No safe default for UNRECOGNIZED code modifications: Enter means STOP.
            $takeRelease = Read-YN "Afwijkende firmware in ${label} vervangen door de nieuwe release (eigen wijzigingen verdwijnen)?" $false
        }
        if ($takeRelease) {
            $decisions.Add("EXPLICIT_RELEASE ${label}: previous custom firmware settings intentionally discarded")
            return $new
        }
        $conflicts.Add($label)
        $decisions.Add("CONFLICT ${label}: existing modifications unrecognized; explicit release choice required (default N / STOP)")
        return $new  # Diagnostic candidate only; no install archive is produced.
    }
    if ($label -in @('logger','wifi','esphome') -and $Mode -eq 'Advanced') {
        $takeExisting = Read-YN "Conflicterend blok ${label}: bestaande configuratie behouden?" $true
        $decisions.Add("MANUAL_CHOICE ${label}: $(if ($takeExisting) {'EXISTING'} else {'RELEASE'})")
        return $(if ($takeExisting) { $old } else { $new })
    }
    $conflicts.Add($label)
    $decisions.Add("CONFLICT $label (candidate keeps existing configuration; manual review required)")
    return $old
}
function Join-Blocks($sections,[string]$preamble) {
    $parts = New-Object 'System.Collections.Generic.List[string]'
    if ($preamble.Trim()) { $parts.Add($preamble.TrimEnd("`n")) }
    foreach ($text in $sections) { if ($null -ne $text) { $parts.Add($text.TrimEnd("`n")) } }
    return (($parts -join "`n`n").TrimEnd("`n") + "`n")
}
function Merge-Substitutions($base,$old,$new) {
    $parsed = @()
    foreach ($v in @($base,$old,$new)) {
        if ($null -eq $v) { $parsed += @{ Order=@(); Blocks=@{}; Preamble='' } }
        else {
            $start = $v.IndexOf("`n")
            $inside = if ($start -lt 0) { '' } else { $v.Substring($start + 1) }
            $parsed += (Parse-Blocks $inside 2)
        }
    }
    $names = New-Object 'System.Collections.Generic.List[string]'
    foreach ($p in @($parsed[2],$parsed[1],$parsed[0])) { foreach ($k in $p.Order) { if (-not $names.Contains($k)) { $names.Add($k) } } }
    $result = New-Object 'System.Collections.Generic.List[string]'
    $identityKeys = @('name','friendly_name','itho_id1','itho_id2','itho_id3','itho_device_type')
    $identityDifferent = $false
    foreach ($idKey in $identityKeys) {
        if ((Sub-Value $parsed[1] $idKey) -and (Sub-Value $parsed[1] $idKey) -cne (Sub-Value $parsed[2] $idKey)) {
            $identityDifferent = $true
        }
    }
    if ($identityDifferent) {
        $script:preserveIdentity = Read-YN 'Bestaande apparaatnaam en Itho-ID behouden (anders kan opnieuw koppelen nodig zijn)?' $true
        $decisions.Add("IDENTITY_SOURCE: $(if ($script:preserveIdentity) {'EXISTING'} else {'RELEASE'})")
    }
    foreach ($key in $hardwareKeys) {
        if (-not $parsed[2].Blocks.ContainsKey($key)) { Fail "Release must define hardware label: $key" }
    }
    Choose-Hardware $parsed[1] $parsed[2]
    if (-not $names.Contains('board_revision')) { $names.Insert(0,'board_revision') }
    foreach ($k in $names) {
        $b=$null; $o=$null; $n=$null
        if ($parsed[0].Blocks.ContainsKey($k)) { $b=$parsed[0].Blocks[$k] }
        if ($parsed[1].Blocks.ContainsKey($k)) { $o=$parsed[1].Blocks[$k] }
        if ($parsed[2].Blocks.ContainsKey($k)) { $n=$parsed[2].Blocks[$k] }
        if ($k -eq 'board_revision') {
            $value = "  board_revision: $($script:hardwareProfile)"
        } elseif ($k -in $hardwareKeys) {
            # GPIOs move as ONE physical hardware profile, never independently.
            $value = if ($script:selectedHardware -eq 'Existing') { $o } else { $n }
            if ((Comparable $o) -cne (Comparable $n)) {
                $oldPin = if ($null -eq $o) { '(missing)' } else { $o.Trim() }
                $decisions.Add("HARDWARE_PIN_$($script:selectedHardware.ToUpperInvariant()) substitutions.${k}: existing [$oldPin] -> selected [$($value.Trim())]")
            }
        } elseif ($k -in @('mqtt_broker','mqtt_username') -and $null -eq $o -and
                  $BaseYaml.EndsWith('v0.3.3-RC4-HF4_HA1.yaml',[StringComparison]::OrdinalIgnoreCase) -and
                  $oldConfig.Blocks.ContainsKey('packages') -and
                  $oldConfig.Blocks['packages'].Contains('  ha_mqtt: !include rflink_universal/options/ha_mqtt.yaml')) {
            # Original HA1 used these two secrets ONLY in the opt-in MQTT file.
            # Carry the existing secret references into the new central labels,
            # never guess an IP or copy actual credentials into release archives.
            $legacyKey = if ($k -eq 'mqtt_broker') { 'rflink_mqtt_broker' } else { 'rflink_mqtt_user' }
            $value = "  $($k): !secret $($legacyKey)"
            $decisions.Add("MIGRATE_MQTT_SECRET substitutions.${k}: existing !secret $legacyKey reference preserved")
        } elseif ($k -in @('name','friendly_name','itho_id1','itho_id2','itho_id3','itho_device_type')) {
            # Device identity and paired virtual Itho ID take priority. If both sides
            # changed, retain existing by default, but record it visibly.
            if ($script:preserveIdentity -and $null -ne $o) {
                $value = $o
                $decisions.Add("PRESERVE_PERSONAL substitutions.$k (existing device identity)")
            } else {
                if ($null -eq $n) { Fail "New release is missing identity key: $k" }
                $value = $n
                $decisions.Add("RELEASE_IDENTITY substitutions.$k")
            }
        } else {
            $value = Choose-Block "substitutions.$k" $b $o $n
        }
        if ($null -ne $value) { $result.Add($value.TrimEnd("`n")) }
    }
    $notes = $parsed[2].Preamble.TrimEnd("`n")
    $head = 'substitutions:'
    if ($notes.Trim()) { $head += "`n" + $notes }
    if ($result.Count) { $head += "`n" + ($result -join "`n") }
    return $head
}
# v2A/B/C stock decoder revisions are identified by TWO hashes from the same
# approved source archive. Older official decoder improvements are not user edits;
# a custom CPP or manifest continues to block a blind migration.
function Known-Stock-Decoder([string]$pack, [string]$oldCpp, [string]$oldManifest) {
    $pathCpp = 'rflink_universal/decoders/' + $pack + '/decoder.cpp'
    $pathManifest = 'rflink_universal/decoders/' + $pack + '/manifest.json'
    $cppHash = File-Hash $oldCpp
    $manifestHash = File-Hash $oldManifest
    foreach ($versionEntry in $stockDecoderRegistry.versions.PSObject.Properties) {
        $files = $versionEntry.Value
        if ($null -ne $files.PSObject.Properties[$pathCpp] -and
            $null -ne $files.PSObject.Properties[$pathManifest] -and
            $cppHash -ceq $files.PSObject.Properties[$pathCpp].Value -and
            $manifestHash -ceq $files.PSObject.Properties[$pathManifest].Value) {
            return $versionEntry.Name
        }
    }
    return ''
}
function Runtime-Relative([string]$rel) {
    if (-not (File-Safe $rel)) { return $false }
    $lower = $rel.ToLowerInvariant()
    if ($lower -match '^rflink_universal/decoders/_[^/]+/') { return $false }
    if ($lower -match '(^|/)(secrets.yaml|\.env|backup|backups|dist|update_output|__pycache__)(/|$)') { return $false }
    if ($lower -match '\.(bat|ps1|md|zip|gz|log|bak|tmp|pyc|bin|elf|pem|key)$') { return $false }
    if ($lower -match '^rflink_universal/(components|decoders)/' -and $lower -match '\.(py|cpp|c|h|hpp|inc|tcc|json|yaml)$') { return $true }
    if ($lower -match '^rflink_universal/(options|packages)/' -and $lower.EndsWith('.yaml')) { return $true }
    return $lower -match '^rflink_universal/ui/[a-z0-9_./-]+\.(html|css|js|json|svg|ico)$'
}
try {
    if ($PSVersionTable.PSVersion.Major -lt 5) { Fail 'Requires Windows PowerShell 5.1.' }
    $interactive = -not $ExistingYaml
    Write-Host ('UPDATE MODE: ' + $Mode + ' - originals are never modified.')
    if ($interactive) {
        $ExistingYaml = Read-Host 'Pad naar je BESTAANDE YAML (sleep bestand hierheen)'
        $chosenNew = Read-Host 'Optioneel pad naar NIEUWE SOURCE ZIP/bundle/map (Enter = huidige projectmap)'
        if ($chosenNew.Trim()) { $NewRelease = $chosenNew }
        $chosenBase = Read-Host 'Optioneel ORIGINELE YAML van oude versie (Enter = automatisch zoeken in bekende UX2-baselines)'
        if ($chosenBase.Trim()) { $BaseYaml = $chosenBase }
        $chosenRuntime = Read-Host 'Optioneel pad naar bestaande SOURCE-map voor eigen decoders (Enter = overslaan)'
        if ($chosenRuntime.Trim()) { $ExistingRuntime = $chosenRuntime }
    }
    $ExistingYaml = $ExistingYaml.Trim().Trim('"')
    if (-not [IO.File]::Exists($ExistingYaml)) { Fail "Existing YAML not found: $ExistingYaml" }
    $ExistingYaml = (Resolve-Path -LiteralPath $ExistingYaml).Path
    if (-not $NewRelease) { $NewRelease = Join-Path $root 'rflink-universal.yaml' }
    $NewRelease = $NewRelease.Trim().Trim('"')
    if (-not (Test-Path -LiteralPath $NewRelease)) { Fail "New release not found: $NewRelease" }
    [IO.Directory]::CreateDirectory($temporary) | Out-Null
    $newRoot = $root
    $ext = $NewRelease.ToLowerInvariant()
    if ($ext.EndsWith('.zip') -or $ext.EndsWith('.tar.gz')) {
        $newRoot = Join-Path $temporary 'new_release'
        [IO.Directory]::CreateDirectory($newRoot) | Out-Null
        if ($ext.EndsWith('.zip')) { Extract-Zip $NewRelease $newRoot } else { Extract-Tar $NewRelease $newRoot }
        $newYaml = Join-Path $newRoot 'rflink-universal.yaml'
    } elseif ([IO.Directory]::Exists($NewRelease)) {
        $newRoot = (Resolve-Path -LiteralPath $NewRelease).Path
        $newYaml = Join-Path $newRoot 'rflink-universal.yaml'
    } else {
        $newYaml = (Resolve-Path -LiteralPath $NewRelease).Path
        $newRoot = [IO.Path]::GetDirectoryName($newYaml)
    }
    if (-not [IO.File]::Exists($newYaml)) { Fail 'New release does not contain rflink-universal.yaml at its root.' }
    if (-not [IO.Directory]::Exists((Join-Path $newRoot 'rflink_universal'))) { Fail 'New release lacks rflink_universal runtime folder; supply a complete source ZIP/bundle or project folder.' }
    $oldText = Read-Utf8 $ExistingYaml
    $newText = Read-Utf8 $newYaml
    $oldRelease = Version-FromText $oldText
    Write-Host ('INPUT YAML: ' + $ExistingYaml)
    Write-Host ('INPUT SHA256: ' + (File-Hash $ExistingYaml))
    Write-Host ('INPUT TYPE: ' + $(if ($oldText.Contains('rmt_symbols: 48')) {'RMT TX fields present'} else {'older RMT TX fields absent'}))
    $newReleaseVersion = Version-FromText $newText
    if (-not $oldRelease -or -not $newReleaseVersion) {
        Fail 'Cannot identify the existing/new YAML release header. Supply the correct original YAML and a supported complete release; automatic migration is not safe.'
    }
    $baseSource = 'explicit'
    if (-not $BaseYaml) {
        # Even when HF version and board_revision agree, the source baseline
        # must be verified: COMPILEFIX1 and HA1 have the SAME numeric HF4 version.
        $BaseYaml = Resolve-KnownBaseline $oldRelease $oldText
        if (-not $BaseYaml) {
            Fail "No verified historical baseline available for $oldRelease. Supply -BaseYaml with the exact original unmodified YAML for this installed version."
        }
        $baseSource = if ($BaseYaml -like '*LEGACY_REVIEWED.yaml') { 'reviewed legacy HF3 variant (personal identity sanitized)' } else { 'verified release-specific baseline' }
    }
    $BaseYaml = $BaseYaml.Trim().Trim('"')
    if (-not [IO.File]::Exists($BaseYaml)) { Fail "Original/base YAML not found: $BaseYaml" }
    $BaseYaml = (Resolve-Path -LiteralPath $BaseYaml).Path
    $baseText = Read-Utf8 $BaseYaml
    $baseRelease = Version-FromText $baseText
    if ($baseRelease -cne $oldRelease) {
        Fail "Wrong baseline: existing YAML is $oldRelease, supplied/selected baseline is $baseRelease. They must be the same release. No merge attempted."
    }
    Write-Host "BASELINE: $baseSource - $oldRelease - $BaseYaml"
    $baseConfig = Parse-Blocks $baseText 0
    $oldConfig = Parse-Blocks $oldText 0
    $newConfig = Parse-Blocks $newText 0
    $keys = New-Object 'System.Collections.Generic.List[string]'
    foreach ($p in @($newConfig,$oldConfig,$baseConfig)) { foreach ($key in $p.Order) { if (-not $keys.Contains($key)) { $keys.Add($key) } } }
    $mergedSections = New-Object 'System.Collections.Generic.List[string]'
    foreach ($key in $keys) {
        $b=$null; $o=$null; $n=$null
        if ($baseConfig.Blocks.ContainsKey($key)) { $b=$baseConfig.Blocks[$key] }
        if ($oldConfig.Blocks.ContainsKey($key)) { $o=$oldConfig.Blocks[$key] }
        if ($newConfig.Blocks.ContainsKey($key)) { $n=$newConfig.Blocks[$key] }
        if ($key -eq 'substitutions' -and ($null -ne $o -or $null -ne $n)) {
            $mergedSections.Add((Merge-Substitutions $b $o $n))
        } else {
            $choice = Choose-Block $key $b $o $n
            if ($null -ne $choice) { $mergedSections.Add($choice) }
        }
    }
    # Device-specific sections must NEVER blindly replace new firmware logic. Other
    # three-way conflicts remain blocked until the human adapts the YAML manually.
    # Validate selected pin table even when hardware is preserved from older config.
    $subSection = @($mergedSections.ToArray() | Where-Object { $_ -match '^substitutions:' } | Select-Object -First 1)[0]
    $chosenSub = Parse-Blocks ($subSection.Substring($subSection.IndexOf("`n") + 1)) 2
    Assert-PinTable $chosenSub 'Selected candidate'
    # Unique, sortable UTC timestamp PLUS random suffix (no overwrite on same millisecond).
    [IO.Directory]::CreateDirectory($outputRoot) | Out-Null
    do {
        $dirName = 'update_' + [DateTime]::UtcNow.ToString('yyyyMMdd_HHmmss_fff') + '_' + [guid]::NewGuid().ToString('N').Substring(0,8)
        $output = Join-Path $outputRoot $dirName
    } while ([IO.Directory]::Exists($output))
    [IO.Directory]::CreateDirectory($output) | Out-Null
    $configOut = Join-Path $output 'rflink-universal.yaml'
    Write-Utf8 $configOut (Join-Blocks $mergedSections.ToArray() $newConfig.Preamble)
    $backups = Join-Path $output 'backups'
    [IO.Directory]::CreateDirectory($backups) | Out-Null
    [IO.File]::Copy($ExistingYaml, (Join-Path $backups 'existing-original.yaml'), $false)
    [IO.File]::Copy($newYaml, (Join-Path $backups 'new-release-original.yaml'), $false)
    [IO.File]::Copy($BaseYaml, (Join-Path $backups 'baseline-original.yaml'), $false)
    $inventory = [ordered]@{}
    foreach ($item in Get-ChildItem -LiteralPath (Join-Path $newRoot 'rflink_universal') -File -Recurse -Force -ErrorAction Stop) {
        if (($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { Fail 'Symlink forbidden in release runtime.' }
        $rel = Path-Relative $newRoot $item.FullName
        if (-not (Runtime-Relative $rel)) { continue }
        $target = Join-Path $output ($rel.Replace('/',[IO.Path]::DirectorySeparatorChar))
        [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
        [IO.File]::Copy($item.FullName, $target, $false)
        $inventory[$rel] = File-Hash $target
    }
    foreach ($needed in @('rflink_universal/components/rflink_universal/__init__.py', 'rflink_universal/components/rflink_universal/rflink_universal.cpp', 'rflink_universal/options/rgb_status_ws2812.yaml','rflink_universal/options/ha_mqtt.yaml','rflink_universal/components/rflink_universal/ha_discovery.h','rflink_universal/ui/index.html','rflink_universal/ui/locales.bundle.json','rflink_universal/ui/locales/en.json','rflink_universal/ui/locales/nl.json','rflink_universal/ui/locales/fr.json','rflink_universal/ui/locales/de.json','rflink_universal/ui/favicon.svg','rflink_universal/ui/favicon.ico')) {
        if (-not $inventory.Contains($needed)) { Fail "Release lacks runtime file: $needed" }
    }
    $stockRegistryPath = Join-Path $root 'rflink_universal/maintenance/decoder_upgrade_baselines.json'
    if (-not [IO.File]::Exists($stockRegistryPath) -or
        (File-Hash $stockRegistryPath) -cne '850CE1A24A9F577419DF53662A98FB117F03F8997185662A5BFA5032CAB9BD3A') {
        Fail 'Verified stock decoder registry missing/modified. Re-extract the full fixed SOURCE ZIP.'
    }
    $stockDecoderRegistry = Get-Content -LiteralPath $stockRegistryPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($stockDecoderRegistry.schema -ne 1) { Fail 'Unsupported stock decoder registry format.' }
    # Own decoder code cannot be safely inferred from an ordinary YAML. An
    # optional existing SOURCE folder can be inspected; differing packs require
    # review and host compilation, never silently replace the new release code.
    if ($ExistingRuntime) {
        $ExistingRuntime = $ExistingRuntime.Trim().Trim('"')
        if (-not [IO.Directory]::Exists($ExistingRuntime)) { Fail 'ExistingRuntime must be an unpacked SOURCE directory.' }
        $oldDecoderRoot = Join-Path $ExistingRuntime 'rflink_universal/decoders'
        if (-not [IO.Directory]::Exists($oldDecoderRoot)) { Fail 'ExistingRuntime lacks a decoders directory.' }
        foreach ($pack in Get-ChildItem -LiteralPath $oldDecoderRoot -Directory | Where-Object { -not $_.Name.StartsWith('_') }) {
            if ($pack.Name -notmatch '^[a-z][a-z0-9_]*$') { Fail "Unsafe decoder name: $($pack.Name)" }
            $newPack = Join-Path (Join-Path $newRoot 'rflink_universal/decoders') $pack.Name
            $oldManifest = Join-Path $pack.FullName 'manifest.json'
            $oldCpp = Join-Path $pack.FullName 'decoder.cpp'
            if (-not [IO.File]::Exists($oldManifest) -or -not [IO.File]::Exists($oldCpp)) { Fail "Incomplete existing decoder: $($pack.Name)" }
            $manifest = Get-Content -LiteralPath $oldManifest -Raw -Encoding UTF8 | ConvertFrom-Json
            if ($manifest.schema -notin @(1,2) -or $manifest.id -cne $pack.Name) { Fail "Invalid existing decoder manifest: $($pack.Name)" }
            if (-not [IO.Directory]::Exists($newPack)) {
                $conflicts.Add("custom decoder $($pack.Name)")
                $decisions.Add("DECODER_REVIEW $($pack.Name): extra decoder detected. NOT copied: add to new source, run tests, then rebuild.")
            } else {
                $newCpp = Join-Path $newPack 'decoder.cpp'
                $newManifest = Join-Path $newPack 'manifest.json'
                if (-not [IO.File]::Exists($newCpp) -or -not [IO.File]::Exists($newManifest)) { Fail "New decoder incomplete: $($pack.Name)" }
                # Schema 1 -> 2 is a REVIEWED metadata migration only if the executable
                # decoder.cpp is byte-for-byte identical. An old pack must never restore
                # an ambiguous TX descriptor or overwrite a schema-2 manifest.
                $stockVersion = Known-Stock-Decoder $pack.Name $oldCpp $oldManifest
                if ($stockVersion) {
                    $decisions.Add("DECODER_STOCK_UPGRADE $($pack.Name): verified unmodified v$stockVersion decoder source+manifest; new release code used.")
                } elseif ((File-Hash $oldCpp) -cne (File-Hash $newCpp)) {
                    $conflicts.Add("modified decoder $($pack.Name)")
                    $decisions.Add("DECODER_REVIEW $($pack.Name): decoder.cpp differs from all approved stock revisions; manual review required.")
                } elseif ($manifest.schema -eq 1) {
                    $decisions.Add("DECODER_SCHEMA2 $($pack.Name): decoder.cpp unchanged; new validated schema-2 manifest from release is used.")
                } elseif ((File-Hash $oldManifest) -cne (File-Hash $newManifest)) {
                    $conflicts.Add("modified decoder manifest $($pack.Name)")
                    $decisions.Add("DECODER_REVIEW $($pack.Name): schema-2 metadata differs; examine capabilities before migrating.")
                }
            }
        }
    } else { $decisions.Add('CUSTOM_DECODERS: existing SOURCE not supplied; decoder migration cannot be verified.') }
    $status = if ($conflicts.Count -eq 0) { 'READY' } else { 'CONFLICT' }
    $report = New-Object 'System.Collections.Generic.List[string]'
    $report.Add('RFLink YAML update report - NO input files were modified.')
    $report.Add('Base: ' + $BaseYaml)
    $report.Add('Baseline selection: ' + $baseSource)
    $report.Add('Existing: ' + $ExistingYaml)
    $report.Add('New: ' + $newYaml)
    $report.Add('Status: ' + $status)
    $report.Add('')
    foreach ($decision in $decisions) { $report.Add($decision) }
    $report.Add('')
    if ($conflicts.Count) {
        $report.Add('DO NOT FLASH: conflicting sections require a conscious manual merge.')
        $report.Add('Resolve conflicts using the backup inputs; rerun the updater with an appropriate baseline or manually adapt.')
    } else { $report.Add('YAML merge is conflict-free. Custom bundle build starts; ESPHome Validate/Compile and physical PCB verification still required.') }
    $report.Add('Hardware revision: ' + $script:hardwareProfile)
    $report.Add('GPIO source: ' + $script:selectedHardware)
    $report.Add('ESPHome Validate/Compile, physical RF TX/RX and OTA: NOT PERFORMED by updater.')
    $report.Add('Additional decoders cannot be recovered from the YAML; provide existing SOURCE directory to check.')
    Write-Utf8 (Join-Path $output 'WIJZIGINGEN.txt') (($report -join "`r`n") + "`r`n")
    $releaseVersion = 'v0.3.3-RC4-HF3'
    if ($newText -match '(?i)v([0-9]+\.[0-9]+\.[0-9]+)[ -]RC([0-9]+)-HF([0-9]+)') { $releaseVersion = ('v' + $Matches[1] + '-RC' + $Matches[2] + '-HF' + $Matches[3]) }
    $meta = [ordered]@{
        release_version = $releaseVersion; schema = 2; board_revision = $script:hardwareProfile; hardware_source = $script:selectedHardware; mode = $Mode; status = $status; generated_utc = [DateTime]::UtcNow.ToString('o');
        yaml_filename = 'rflink-universal.yaml'; yaml_sha256 = File-Hash $configOut;
        original_sha256 = File-Hash $ExistingYaml; new_sha256 = File-Hash $newYaml;
        baseline_sha256 = File-Hash $BaseYaml; conflicts = @($conflicts.ToArray()); runtime_sha256 = $inventory
    }
    Write-Utf8 (Join-Path $output 'update-manifest.json') ((ConvertTo-Json -InputObject $meta -Depth 10) + "`n")
    Write-Host "OUTPUT: $output"
    Write-Host ('STATUS: ' + $status)
    if ($conflicts.Count) {
        Write-Warning ('Conflicts found: ' + ($conflicts -join ', '))
        Write-Warning 'No install bundle created. Resolve conflicts and rerun; no older update is substituted.'
        exit 2
    }
    if (-not (Read-YN 'Geselecteerde instellingen akkoord? Persoonlijke ESPHome-bundle maken?' $true)) {
        Fail 'Bundle creation canceled by operator. No CUSTOM bundle was created.'
    }
    if ([IO.File]::Exists($failureMarker)) { Remove-Item -LiteralPath $failureMarker -Force }
    Write-Host 'Original YAML files unchanged. Building personal ESPHome bundle automatically...'
    $builder = Join-Path $root 'rflink_universal/tools/maak_release.ps1'
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $builder -ProjectRoot $root -UseUpdaterOutput
    if ($LASTEXITCODE -ne 0) { Fail 'Custom bundle build/verification failed. No candidate is approved for flashing.' }
    Write-Host 'CUSTOM bundle is in dist/. In ESPHome run Validate and Compile; flash/OTA is NEVER automatic.'
    exit 0
} catch {
    [Console]::Error.WriteLine('UPDATE FAILED: ' + $_.Exception.Message)
    # A failed attempt that aborts BEFORE making an output must not allow the
    # old READY folder to be silently published as though the update succeeded.
    try {
        [IO.Directory]::CreateDirectory($outputRoot) | Out-Null
        Write-Utf8 $failureMarker ('Last updater attempt FAILED at ' + [DateTime]::UtcNow.ToString('o') + "`r`n" + $_.Exception.Message + "`r`n")
    } catch { [Console]::Error.WriteLine('Could not write updater failure marker; do not use an older bundle; rerun 00_RFLink_START.bat.') }
    exit 1
} finally {
    if ([IO.Directory]::Exists($temporary)) { Remove-Item -LiteralPath $temporary -Recurse -Force -ErrorAction SilentlyContinue }
}
