param(
    [Parameter(Mandatory=$true)][string]$Compiler,
    [string]$Rockbox = (Join-Path $PSScriptRoot '../rockbox'),
    [string]$HostCompiler = 'gcc'
)
$ErrorActionPreference = 'Stop'
$build = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force "$build/lang" | Out-Null
Copy-Item -LiteralPath "$PSScriptRoot/autoconf.h" -Destination "$build/autoconf.h"
$version = & git -C $Rockbox describe --tags --exact-match HEAD
if ($LASTEXITCODE -or $version -ne 'v4.0-final') { throw 'Use the unmodified Rockbox v4.0-final source tree.' }
& perl -s "$Rockbox/tools/genlang" "-p=$build/lang" '-t=ipodvideo' "$Rockbox/apps/lang/english.lang"
if ($LASTEXITCODE) { throw 'Language header generation failed' }
& $HostCompiler "$Rockbox/tools/convbdf.c" -o "$build/convbdf.exe"
if ($LASTEXITCODE) { throw 'Font tool compilation failed' }
& "$build/convbdf.exe" -h -o "$build/sysfont.h" "$Rockbox/fonts/08-Schumacher-Clean.bdf"
if ($LASTEXITCODE) { throw 'Font header generation failed' }
$includes = @($build, "$build/lang", "$Rockbox/firmware/target/arm/ipod/video",
    "$Rockbox/firmware/target/arm/ipod", "$Rockbox/firmware/target/arm/pp", "$Rockbox/firmware/target/arm",
    "$Rockbox/apps", "$Rockbox/apps/gui", "$Rockbox/apps/recorder", "$Rockbox/apps/radio",
    "$Rockbox/firmware", "$Rockbox/firmware/export", "$Rockbox/firmware/drivers",
    "$Rockbox/firmware/include", "$Rockbox/firmware/kernel/include", "$Rockbox/firmware/libc/include", "$Rockbox/lib/rbcodec",
    "$Rockbox/lib/rbcodec/codecs", "$Rockbox/lib/rbcodec/dsp", "$Rockbox/lib/rbcodec/metadata", "$Rockbox/lib/skin_parser",
    "$Rockbox/lib/libsetjmp", "$Rockbox/apps/plugins", "$Rockbox/lib") | ForEach-Object { "-I$_" }
$flags = @('-mcpu=arm7tdmi', '-marm', '-std=gnu99', '-Os', '-ffreestanding', '-fno-builtin',
    '-fno-strict-aliasing', '-fomit-frame-pointer', '-ffunction-sections', '-fdata-sections',
    '-DROCKBOX', '-DIPOD_VIDEO', '-DMEMORYSIZE=64', '-DTARGET_ID=15', '-DPLUGIN', '-Wall', '-Wextra', '-Wno-expansion-to-defined', '-Wno-pointer-sign') + $includes
& $Compiler @flags -c "$PSScriptRoot/playlist_builder.c" -o "$build/playlist_builder.o"
if ($LASTEXITCODE) { throw 'Plugin compilation failed' }
& $Compiler @flags -c "$Rockbox/apps/plugins/plugin_crt0.c" -o "$build/plugin_crt0.o"
if ($LASTEXITCODE) { throw 'Startup compilation failed' }
& $Compiler @flags -c "$Rockbox/lib/libsetjmp/arm/setjmp.S" -o "$build/setjmp.o"
if ($LASTEXITCODE) { throw 'setjmp compilation failed' }
& $Compiler @flags -c "$Rockbox/apps/plugins/lib/gcc-support.c" -o "$build/gcc-support.o"
if ($LASTEXITCODE) { throw 'Compiler support compilation failed' }
& $Compiler @flags -E -P -x c "$Rockbox/apps/plugins/plugin.lds" -o "$build/plugin.link"
if ($LASTEXITCODE) { throw 'Linker script preprocessing failed' }
& $Compiler -mcpu=arm7tdmi -marm -nostdlib '-Wl,--gc-sections' "-Wl,-T,$build/plugin.link" "-Wl,-Map,$build/playlist_builder.map" "$build/playlist_builder.o" "$build/plugin_crt0.o" "$build/setjmp.o" "$build/gcc-support.o" -lgcc -o "$build/playlist_builder.elf"
if ($LASTEXITCODE) { throw 'Link failed' }
$objcopy = $Compiler -replace 'gcc(\.exe)?$', 'objcopy.exe'
& $objcopy -O binary "$build/playlist_builder.elf" "$build/playlist_builder.rock"
if ($LASTEXITCODE) { throw 'Binary conversion failed' }
Write-Output "Built $build/playlist_builder.rock"
