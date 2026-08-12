# Compiles one source file with cl.exe, classifies the outcome, logs it, and
# appends a row to the job summary table started by setup-msvc.ps1.
#
# Exits with cl.exe's exit code, so a compile failure (including an internal
# compiler error) fails the workflow step it runs in.

param(
	[Parameter(Mandatory)] [string]$Source,
	# Extra cl.exe flags as one space-separated string, e.g. '/O2 /Zi'.
	[string]$Flags = '',
	[string]$LogDir = 'logs'
)

$ErrorActionPreference = 'Continue'

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

$baseFlags = @('/nologo', '/std:c++20', '/EHsc', '/W4')
$extraFlags = @($Flags -split '\s+' | Where-Object { $_ })
$name = [IO.Path]::GetFileNameWithoutExtension($Source)
$flagTag = if ($extraFlags) { '-' + (($extraFlags -join '') -replace '[^A-Za-z0-9]+', '') } else { '' }
$log = Join-Path $LogDir "$name$flagTag.log"

$clArgs = $baseFlags + $extraFlags + @('/c', "/Fo$LogDir\", "/Fd$LogDir\", $Source)
Write-Host "cl $($clArgs -join ' ')"
& cl @clArgs 2>&1 | Tee-Object -FilePath $log
$code = $LASTEXITCODE

$text = if (Test-Path $log) { Get-Content $log -Raw } else { '' }
$isIce = $text -match 'C1001|C1907|[Ii]nternal [Cc]ompiler [Ee]rror|INTERNAL COMPILER ERROR'

$result = if ($code -eq 0) {
	'compiled OK'
} elseif ($isIce) {
	"INTERNAL COMPILER ERROR (exit $code)"
} else {
	"compile error, not an ICE (exit $code)"
}
Write-Host "RESULT [$Source $Flags]: $result"

if ($env:GITHUB_STEP_SUMMARY) {
	$emoji = if ($code -eq 0) { ':white_check_mark:' } elseif ($isIce) { ':boom:' } else { ':x:' }
	$flagCell = if ($Flags) { $Flags } else { '(default)' }
	"| ``$Source`` | ``$flagCell`` | $emoji $result |" |
		Out-File -FilePath $env:GITHUB_STEP_SUMMARY -Encoding utf8 -Append
}

exit $code
