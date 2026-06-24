# Windows ARM64 Local Proof With Parallels

This runbook verifies the library preset inside a native Windows 11 ARM VM on an
Apple Silicon Mac. It is a local Windows/MSVC proof for ARM64. The public CI
`windows-latest` job remains the Windows x64/MSVC proof.

## What This Proves

- The public source tree configures with MSVC.
- `cmake --build --preset library` builds the metering conformance binary and
  the public-surface compile/link smokes.
- `ctest --test-dir build-library --output-on-failure` passes on Windows ARM64.
- The reusable modules do not require Git Bash for the shipped CTest proof.

## Host Requirements

- Apple Silicon Mac.
- Parallels Desktop with a Windows 11 ARM VM.
- Parallels Tools installed in the VM.
- `prlctl` available on macOS.

Check VM visibility:

```sh
prlctl list --all
```

Check command execution inside the VM:

```sh
prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -Command "whoami; [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture"
```

Expected architecture:

```text
Arm64
```

## Windows Toolchain Bootstrap

Run these from macOS. Approve any Windows administrator prompt if one appears.

```sh
prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \
  "winget install --id Git.Git --exact --source winget --accept-package-agreements --accept-source-agreements"

prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \
  "winget install --id Kitware.CMake --exact --source winget --accept-package-agreements --accept-source-agreements"

prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \
  "winget install --id Ninja-build.Ninja --exact --source winget --accept-package-agreements --accept-source-agreements"

prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \
  "winget install --id Python.Python.3.12 --exact --source winget --accept-package-agreements --accept-source-agreements"
```

Install Visual Studio Build Tools with the ARM64 C++ workload. Encoding the
PowerShell command avoids shell-quoting drift between macOS, `prlctl`, and
Windows.

```sh
ENC="$(python3 - <<'PY'
import base64
script = r'''
$wingetArgs = @(
  'install',
  '--id', 'Microsoft.VisualStudio.2022.BuildTools',
  '--exact',
  '--source', 'winget',
  '--accept-package-agreements',
  '--accept-source-agreements',
  '--override', '--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --add Microsoft.VisualStudio.Component.VC.Tools.ARM64 --add Microsoft.VisualStudio.Component.Windows11SDK.26100'
)
& winget @wingetArgs
exit $LASTEXITCODE
'''
print(base64.b64encode(script.encode('utf-16le')).decode('ascii'))
PY
)"
prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -EncodedCommand "$ENC"
```

Verify MSVC:

```sh
ENC="$(python3 - <<'PY'
import base64
script = r'''
Get-ChildItem 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools' -Filter cl.exe -Recurse -ErrorAction SilentlyContinue |
  Select-Object -First 3 -ExpandProperty FullName
& 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64 -property installationPath
'''
print(base64.b64encode(script.encode('utf-16le')).decode('ascii'))
PY
)"
prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -EncodedCommand "$ENC"
```

## Copy The Public Tree Into Windows

Build from `C:\`, not directly from a shared Parallels folder. This avoids
case, path, and file-notification behavior from the host share.

```sh
rm -rf "$HOME/Downloads/bellweather-audio-core-winproof"
rsync -a \
  --exclude 'build-library' \
  --exclude 'build-asan-ubsan' \
  --exclude 'build-tsan' \
  --exclude '.git' \
  $HOME/Code/bellweather-audio-core/ \
  "$HOME/Downloads/bellweather-audio-core-winproof/"

ENC="$(python3 - <<'PY'
import base64
script = r'''
$src = 'Z:\Downloads\bellweather-audio-core-winproof'
$dst = 'C:\bws\bellweather-audio-core'
New-Item -ItemType Directory -Force -Path 'C:\bws' | Out-Null
if (Test-Path $dst) { Remove-Item -Recurse -Force $dst }
robocopy $src $dst /MIR /R:2 /W:1 /NFL /NDL /NP
$code = $LASTEXITCODE
if ($code -le 7) { exit 0 }
exit $code
'''
print(base64.b64encode(script.encode('utf-16le')).decode('ascii'))
PY
)"
prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -EncodedCommand "$ENC"
```

## Run The Proof

Create a stable command file inside the VM:

```sh
ENC="$(python3 - <<'PY'
import base64
script = r'''
$scriptPath = 'C:\bws\run-oss-library-proof.cmd'
$lines = @(
  '@echo on',
  'set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;C:\Program Files\CMake\bin;C:\Program Files\Git\cmd;%LOCALAPPDATA%\Programs\Python\Python312;%LOCALAPPDATA%\Programs\Python\Python312\Scripts;%PATH%"',
  'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=arm64 -host_arch=arm64',
  'if errorlevel 1 exit /b %errorlevel%',
  'cd /d "C:\bws\bellweather-audio-core"',
  'if exist build-library rmdir /s /q build-library',
  'where cl',
  'where cmake',
   'where ninja',
   'where python',
   'python tools\check_manifest_coverage.py',
   'if errorlevel 1 exit /b %errorlevel%',
   'cmake --preset library -DBWS_WARNINGS_AS_ERRORS=ON',
   'if errorlevel 1 exit /b %errorlevel%',
   'cmake --build --preset library',
  'if errorlevel 1 exit /b %errorlevel%',
  'ctest --test-dir build-library --output-on-failure',
  'exit /b %errorlevel%'
)
Set-Content -Path $scriptPath -Value $lines -Encoding ASCII
'''
print(base64.b64encode(script.encode('utf-16le')).decode('ascii'))
PY
)"
prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -EncodedCommand "$ENC"
```

Run it:

```sh
ENC="$(python3 - <<'PY'
import base64
script = r'''
& 'C:\bws\run-oss-library-proof.cmd'
exit $LASTEXITCODE
'''
print(base64.b64encode(script.encode('utf-16le')).decode('ascii'))
PY
)"
prlctl exec "Windows 11" --current-user powershell.exe -NoProfile -ExecutionPolicy Bypass -EncodedCommand "$ENC"
```

Success ends with:

```text
100% tests passed, 0 tests failed out of 161
```

The count can increase as coverage grows. The Windows ARM64 lane is currently
one test lower than macOS/Linux because an Apple-only containment probe is not
registered on Windows.

## Notes From The Windows Lane

- MSVC needs `/Zc:__cplusplus` so `__cplusplus` reports the active C++ standard.
- Avoid non-standard `M_PI`; use `std::numbers::pi_v<T>` or a local constant.
- Do not register CTest entries by Unicode test names on Windows. Stable ASCII
  test names and tags are more portable.
- CTest checks should not require Git Bash. Use portable scripts for public proof
  lanes.
- Public-surface smokes should link only the documented `bws::` target. They are
  meant to catch missing `PUBLIC` include paths, compile definitions, and link
  dependencies.
