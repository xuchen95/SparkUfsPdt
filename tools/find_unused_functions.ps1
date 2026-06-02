# Scan project files for likely function definitions and report those with few references
$root = "D:\Public_Code\GitRepos\SparkUfsPdt"
$extensions = @('*.cpp','*.c','*.h','*.hpp')
$files = Get-ChildItem -Path $root -Recurse -Include $extensions -File
$defs = @{}
# Regex to find potential function definitions (free or member)
$defRegex = '^(\s*(?:[\w\:\<\>\~\*&\s]+)\s+([A-Za-z_][A-Za-z0-9_:\:]*)\s*\([^;]*\)\s*(?:\{|$))'
foreach($f in $files){
	$lines = Get-Content $f.FullName -Raw -ErrorAction SilentlyContinue
	if(-not $lines) { continue }
	$lines -split "\r?\n" | ForEach-Object -Begin {$lineno=0} -Process {
		$lineno++
		$line = $_
		if($line -match $defRegex){
			$full = $matches[2]
			# ignore known patterns that are likely macros or templates
			if($full -match '\b(if|for|while|switch|return)\b') { return }
			$key = $full
			if(-not $defs.ContainsKey($key)) { $defs[$key] = @() }
			$defs[$key] += "${f.FullName}:$lineno"
		}
	}
}

$results = @()
foreach($name in $defs.Keys){
	# search occurrences across files (word boundary)
	$pattern = [regex]::Escape($name)
	$count = (Select-String -Path $files.FullName -Pattern "\b$pattern\b" -SimpleMatch -AllMatches -ErrorAction SilentlyContinue | Measure-Object).Count
	if($count -le 1){
		$results += [PSCustomObject]@{ Name = $name; Definitions = ($defs[$name] -join "; "); Occurrences = $count }
	}
}

# Output to console and to file
$out = "UnusedCandidateFunctions.txt"
"Found $($results.Count) candidate unused functions (approx):" | Tee-Object -FilePath $out
$results | Sort-Object Name | ForEach-Object {
	"$($_.Name)    Occurrences=$($_.Occurrences)    DefinedAt=$($_.Definitions)" | Tee-Object -FilePath $out -Append
}

Write-Output "Wrote results to $out"
