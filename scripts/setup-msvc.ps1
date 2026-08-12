# Imports an MSVC developer environment into the current GitHub Actions job.
#
# Locates Visual Studio with vswhere, enters its developer shell (optionally
# pinning a specific toolset via -vcvars_ver), exports the resulting
# environment variables to $GITHUB_ENV so later steps can invoke cl.exe
# directly, and writes the compiler banner plus a results-table header to the
# job summary.

param(
	# MSVC toolset to pin, e.g. '14.44'. Empty string selects the newest
	# toolset the image has installed.
	[string]$VcVarsVer = '',
	# Human-readable lane name for the job summary heading.
	[string]$Label = ''
)

$ErrorActionPreference = 'Stop'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = & $vswhere -latest -prerelease -products * `
	-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
	-property installationPath
if (-not $vsPath) { throw 'No Visual Studio installation with C++ build tools found' }
Write-Host "Visual Studio: $vsPath"

$before = @{}
Get-ChildItem env: | ForEach-Object { $before[$_.Name] = $_.Value }

Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
$devArgs = '-arch=x64 -host_arch=x64'
if ($VcVarsVer) { $devArgs += " -vcvars_ver=$VcVarsVer" }
Write-Host "DevCmdArguments: $devArgs"
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments $devArgs

$cl = Get-Command cl.exe -ErrorAction Stop
Write-Host "cl.exe: $($cl.Source)"
$banner = (& cl 2>&1 | Select-Object -First 1) -join ''
Write-Host $banner

# Export everything the dev shell changed or added so later steps see it.
Get-ChildItem env: |
	Where-Object { $_.Name -ne 'GITHUB_ENV' -and $before[$_.Name] -ne $_.Value } |
	ForEach-Object { "$($_.Name)=$($_.Value)" } |
	Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append

if ($env:GITHUB_STEP_SUMMARY) {
	@(
		"## $(if ($Label) { $Label } else { 'MSVC lane' })"
		''
		"``$banner``"
		''
		"cl.exe path: ``$($cl.Source)``"
		''
		'| Source | Flags | Result |'
		'| --- | --- | --- |'
	) | Out-File -FilePath $env:GITHUB_STEP_SUMMARY -Encoding utf8 -Append
}

# The bare `cl` banner invocation above can leave a nonzero $LASTEXITCODE that
# the Actions pwsh shim would otherwise propagate as a step failure.
exit 0
