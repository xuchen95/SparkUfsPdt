param(
	[string]$ProjectPath = "D:\Public_Code\GitRepos\SparkUfsPdt\SparkUfsPdt"
)
$extensions = @('*.cpp','*.c','*.h','*.hpp')
$files = Get-ChildItem -Path $ProjectPath -Recurse -Include $extensions -File -ErrorAction SilentlyContinue
if(-not $files){ Write-Output "No files found under $ProjectPath"; exit 0 }
$defs = @{}
$defRegex = '^(\s*(?:[\w\:\<\>\~\*&\s]+)\s+([A-Za-z_][A-Za-z0-9_:\:]*)\s*\([^;]*\)\s*(?:\{|$))'
foreach($f in $files){
	$lineno=0
	Get-Content $f.FullName -ErrorAction SilentlyContinue | ForEach-Object {
		$lineno++
		$line = $_
		if($line -match $defRegex){
			$full = $matches[2]
			if($full -match '\b(if|for|while|switch|return)\b') { continue }
			if(-not $defs.ContainsKey($full)) { $defs[$full] = @() }
			$defs[$full] += "${f.FullName}:$lineno"
		}
	}
}
$results = @()
foreach($name in $defs.Keys){
	$pattern = [regex]::Escape($name)
	$count = 0
	foreach($f in $files){
		$count += (Select-String -Path $f.FullName -Pattern "\b$pattern\b" -AllMatches -ErrorAction SilentlyContinue | Measure-Object).Count
	}
	if($count -le 1){
		$results += [PSCustomObject]@{ Name = $name; Definitions = ($defs[$name] -join "; "); Occurrences = $count }
	}
}
$out = Join-Path -Path (Get-Location) -ChildPath "SparkUfsPdt_UnusedCandidates.txt"
"Found $($results.Count) candidate unused functions (approx):" | Out-File $out
$results | Sort-Object Name | ForEach-Object { "$($_.Name)    Occurrences=$($_.Occurrences)    DefinedAt=$($_.Definitions)" | Out-File $out -Append }
Write-Output "Wrote results to $out"
