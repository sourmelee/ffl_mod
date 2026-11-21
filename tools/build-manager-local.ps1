param(
    [Parameter(Mandatory=$true)][string]$Sdk,
    [Parameter(Mandatory=$true)][string]$Jdk,
    [Parameter(Mandatory=$true)][string]$KotlinHome,
    [Parameter(Mandatory=$true)][string]$ApkSigJar,
    [string]$BuildTools = '36.1.0',
    [string]$Platform = 'android-36.1'
)
# Offline fallback when Gradle's daemon/dependency downloads are unavailable.
# Uses the installed SDK and Kotlin compiler; normal CI remains Gradle 8.7.
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$work = Join-Path $project 'out/manager-local'
$dist = Join-Path $project 'out/dist'
$bt = Join-Path $Sdk "build-tools/$BuildTools"
$androidJar = Join-Path $Sdk "platforms/$Platform/android.jar"
$main = Join-Path $project 'manager/app/src/main'
$classes = Join-Path $work 'classes'
$generated = Join-Path $work 'generated'
$dex = Join-Path $work 'dex'
foreach ($dir in @($work,$dist,$classes,$generated,$dex)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
function Invoke-Build([string]$exe, [string[]]$arguments) {
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "Build command failed: $exe ($LASTEXITCODE)" }
}
$java = Join-Path $Jdk 'bin/java.exe'
$aapt = Join-Path $bt 'aapt2.exe'
$config = Get-Content -LiteralPath (Join-Path $project 'manager/app/build.gradle.kts') -Raw
$version = [regex]::Match($config, 'versionName\s*=\s*"([^"]+)"').Groups[1].Value
$code = [regex]::Match($config, 'versionCode\s*=\s*(\d+)').Groups[1].Value
$manifestPath = Join-Path $work 'AndroidManifest.xml'
[xml]$manifest = Get-Content -LiteralPath (Join-Path $main 'AndroidManifest.xml') -Raw
$manifest.manifest.SetAttribute('package', 'io.github.fflmod.fflmod')
[System.IO.File]::WriteAllText($manifestPath,$manifest.OuterXml,[System.Text.UTF8Encoding]::new($false))
Invoke-Build $aapt @('compile','--dir',(Join-Path $main 'res'),'-o',(Join-Path $work 'res.zip'))
Invoke-Build $aapt @('link','-o',(Join-Path $work 'resources.apk'),'--manifest',$manifestPath,'-I',$androidJar,'--java',$generated,'--min-sdk-version','24','--target-sdk-version','34','--version-code',$code,'--version-name',$version,'--debug-mode','--auto-add-overlay','-A',(Join-Path $main 'assets'),(Join-Path $work 'res.zip'))
$javac = @('--release','17','-encoding','UTF-8','-classpath',"$androidJar;$ApkSigJar",'-d',$classes)
$javac += @(Get-ChildItem -LiteralPath (Join-Path $main 'java') -Recurse -Filter '*.java' -File | ForEach-Object {$_.FullName})
$javac += @(Get-ChildItem -LiteralPath $generated -Recurse -Filter '*.java' -File | ForEach-Object {$_.FullName})
Invoke-Build (Join-Path $Jdk 'bin/javac.exe') $javac
$stdlib = Join-Path $KotlinHome 'lib/kotlin-stdlib.jar'
$kotlinc = @('-cp',(Join-Path $KotlinHome 'lib/*'),'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler','-jvm-target','17','-no-stdlib','-no-reflect','-classpath',"$androidJar;$classes;$stdlib;$ApkSigJar",'-d',$classes)
$kotlinc += @(Get-ChildItem -LiteralPath (Join-Path $main 'java') -Recurse -Filter '*.kt' -File | ForEach-Object {$_.FullName})
Invoke-Build $java $kotlinc
Invoke-Build (Join-Path $Jdk 'bin/jar.exe') @('cf',(Join-Path $work 'classes.jar'),'-C',$classes,'.')
Invoke-Build $java @('-cp',(Join-Path $bt 'lib/d8.jar'),'com.android.tools.r8.D8','--min-api','24','--lib',$androidJar,'--output',$dex,(Join-Path $work 'classes.jar'),$stdlib,$ApkSigJar)
$unsigned = Join-Path $work 'unsigned.apk'
Copy-Item -LiteralPath (Join-Path $work 'resources.apk') -Destination $unsigned -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::Open($unsigned, [System.IO.Compression.ZipArchiveMode]::Update)
try {
    foreach ($file in (Get-ChildItem -LiteralPath $dex -Filter '*.dex' -File)) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive,$file.FullName,$file.Name,[System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally { $archive.Dispose() }
$aligned = Join-Path $work 'aligned.apk'
Invoke-Build (Join-Path $bt 'zipalign.exe') @('-f','4',$unsigned,$aligned)
$key = Join-Path $work 'debug.keystore'
if (!(Test-Path -LiteralPath $key)) {
    Invoke-Build (Join-Path $Jdk 'bin/keytool.exe') @('-genkeypair','-keystore',$key,'-storepass','android','-keypass','android','-alias','androiddebugkey','-dname','CN=Android Debug,O=Android,C=US','-keyalg','RSA','-validity','3650')
}
$apk = Join-Path $dist "fflmod-manager-$version-debug.apk"
$signer = Join-Path $bt 'lib/apksigner.jar'
Invoke-Build $java @('-jar',$signer,'sign','--ks',$key,'--ks-pass','pass:android','--key-pass','pass:android','--out',$apk,$aligned)
Invoke-Build $java @('-jar',$signer,'verify','--verbose','--min-sdk-version','24',$apk)
Invoke-Build $aapt @('dump','badging',$apk)
Write-Output "Built $apk using Kotlin, SDK $Platform / build-tools $BuildTools and $ApkSigJar"
