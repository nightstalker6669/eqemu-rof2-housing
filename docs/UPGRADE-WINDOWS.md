# Upgrade a native Windows server

This path builds and installs **native Windows `.exe` files**. It does not require WSL or Docker. It uses the same complete housing migrations, content and backup/restore workflow as Linux.

> **Current validation:** Windows-specific installer operations have been exercised with Windows Python under Wine, including original geometry import and `.exe`/DLL backup/restore. Portable arithmetic and process guards have focused checks. A full MSVC build, a real Windows server launch, and NTFS permission/junction behavior have not yet been accepted on a Windows host. Use a separate copy of your server/database for the first Windows deployment. This is an implemented native upgrade path, not a verified prebuilt Windows release.

## 1. Have the build tools ready

Use an existing 64-bit Windows EQEmu server with MariaDB and a working native source-build environment:

- **64-bit Python 3.9 or newer**, Git for Windows and CMake on `PATH`.
- **Visual Studio 2022**, with the **Desktop development with C++** workload and Windows SDK.
- The pinned EQEmu source dependencies: matching x64 database client/development libraries, Boost, TLS/OpenSSL, Lua and your normal scripting dependencies. Follow the [official native Windows build guide](https://docs.eqemu.dev/server/installation/ground-up-windows-install/) for the ordinary EQEmu toolchain; this overlay does not replace it.
- MariaDB client and dump utilities (`mariadb.exe`/`mariadb-dump.exe` or compatible `mysql.exe`/`mysqldump.exe`) on `PATH`.
- Your own complete May 10, 2013 RoF2 client and matching server collision maps.

Use an **x64 Native Tools Command Prompt for VS 2022**, then launch PowerShell from it. Keep Python and all server/runtime libraries on the same x64 architecture. A 32-bit Python environment cannot validate an x64 server's runtime dependencies.

Check that the commands are available:

```powershell
py -3 --version
git --version
cmake --version
where.exe cl
where.exe mariadb
where.exe mariadb-dump
```

If your client tools use the MySQL filenames, check those instead. Do not put database passwords into commands: the installer reads your existing `eqemu_config.json`.

## 2. Download without changing line endings

In PowerShell:

```powershell
Set-Location C:\EQ
git -c core.autocrlf=false clone https://github.com/nightstalker6669/eqemu-rof2-housing.git
Set-Location C:\EQ\eqemu-rof2-housing
```

The package includes `.gitattributes` to preserve exact text bytes. Do not convert the SQL, JSON or patch files to CRLF: their hashes are checked before installation. If an earlier checkout was converted, keep any local edits separately and clone a fresh package with the command above.

Your existing source repository must contain upstream commit `8175ae61874cbf20d7b05102821f6f8a4f4d1c39`. Fetch it if needed, without checking out over your own work:

```powershell
git -C C:\EQ\Source fetch origin 8175ae61874cbf20d7b05102821f6f8a4f4d1c39
```

Read [source compatibility](INSTALL.md#source-compatibility-before-you-upgrade): the installer creates a separate managed checkout at the pinned base. Your original checkout is preserved, but custom C++ features are not automatically merged into the binaries it deploys.

## 3. Set your paths once

Replace the example directories below. `Server` is the folder containing the **active** `eqemu_config.json`; `Client` is the folder containing `eqgame.exe` and the original `.eqg` archives.

```powershell
$HousingArgs = @(
  '--source', 'C:\EQ\Source',
  '--server', 'C:\EQ\Server',
  '--client', 'C:\Games\EverQuest-RoF2',
  '--build', 'C:\EQ\HousingBuild',
  '--backup', 'C:\EQ\HousingBackups\housing-01',
  '--generator', 'Visual Studio 17 2022',
  '--architecture', 'x64',
  '--config', 'Release'
)
```

Paths containing spaces work as individual array entries. Use a **new private backup directory** on each application. Run under the Windows account that owns the server; it must be allowed to read client assets, build source, create a private backup and replace the stopped server binaries.

The server folder must have `world.exe` or `bin\world.exe`. If necessary, add `--bin-dir`, `--opcode-dir`, `--quests-dir` and `--maps-dir` pairs to the array. `Server\maps` must already reach the configured maps folder and contain `base`; a real directory or an existing directory junction works. The installer does not create privileged Windows links for you.

For non-default build dependencies, add the CMake settings your working native build needs, for example:

```powershell
$HousingArgs += @('--cmake-define', 'MYSQL_ROOT=C:/EQ/Dependencies/mysql')
# Or pass your existing dependency toolchain:
# $HousingArgs += @('--toolchain', 'C:\EQ\Dependencies\toolchain.cmake')
```

Do not copy that example blindly: use the actual SDK paths/settings for your build. `--cmake-define NAME=VALUE` is repeatable.

## 4. Supply matching runtime libraries

The installer follows the built executables' actual PE imports, delay imports and forwarded exports. It checks architecture, missing symbols and ambiguous versions; it does not ship an invented list of DLLs.

If your build keeps its runtime DLLs outside the build output directory, add their **matching x64** directories:

```powershell
# Replace with the runtime directory from YOUR native dependency installation.
$HousingArgs += @('--runtime-dir', 'C:\EQ\Dependencies\runtime-x64')
```

Repeat `--runtime-dir` for additional SDK runtime directories. A DLL selected dynamically by a plugin rather than a PE import must be supplied explicitly with `--runtime-dll C:\path\plugin.dll`. Its own dependencies are checked too. Install the Microsoft Visual C++ runtime required by your build. Do not mix DLLs from unrelated server downloads or copy system DLLs into the server directory to bypass a dependency refusal.

Existing adjacent DLLs are included in the private backup. Newly introduced executables/DLLs are recorded for rollback; runtime dependency hashes are recorded with the eight executable hashes.

## 5. Plan, stop and install

First run the read-only plan:

```powershell
py -3 .\tools\install_housing.py @HousingArgs
```

The plan checks the package, database/configuration and source identity. It does not compile or certify the complete native toolchain. Resolve any refusal. MariaDB must allow packets of at least 64MiB; a database administrator should configure `max_allowed_packet=64M` or larger and reconnect before retrying.

Ask players to log out. Stop world, **every zone**, UCS and your launcher/service manager so they cannot respawn. Pause automatic binary updates. Keep MariaDB running. Confirm the game writers are gone:

```powershell
Get-Process world,zone,ucs -ErrorAction SilentlyContinue
```

Then apply:

```powershell
py -3 .\tools\install_housing.py @HousingArgs --apply
```

If the source checkout differs from the pinned base or has local changes, the installer stops before database work. After reviewing the pinned-build consequence, add `--accept-pinned-server-build` to acknowledge that build choice. This flag does not merge custom code or establish compatibility with another EQEmu revision.

Wait for `HOUSING_INSTALL_PASS`. Start your usual server launcher **only after** success. All eight executables, their runtime dependencies, schemas, mappings and shared cache must stay together. A future ordinary upstream binary update can remove housing support; rebuild from the housing package instead.

## 6. Check in game—or restore

Connect using the matching original RoF2 client, visit Sunrise Hills, inspect a plot marker, and try a real personal/guild workflow. Use [current status](STATUS.md) to understand the supported scope.

If any step fails, keep the game server stopped and read the private log named by the installer. Do not restart old executables over a partly upgraded database. To inspect the complete rollback plan:

```powershell
py -3 .\tools\restore_housing.py --server C:\EQ\Server --backup C:\EQ\HousingBackups\housing-01
```

Repeat with `--apply` after reviewing it. This restores the saved database, resolved binaries/DLLs, mappings and cache together and removes unchanged newly installed files. A modified deployment link/file or unresolved inventory application is refused. The restore does not start services for you.
