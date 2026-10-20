# Install private housing geometry

The server needs geometry derived from your own supported original RoF2 client.
The public overlay includes hash catalogs and this importer; it includes no client
archives, extracted meshes, captures, fixtures or test files.

Use Python 3.9 or newer. Run the importer as the account that runs the zone server:

```sh
python3 tools/install_original_geometry.py \
  --client /operator/ROF2 \
  --server /operator/server \
  --overlay /export/overlay
```

`--client` is read only. `--server` must be an existing server working directory.
`--overlay` contains `common/housing`; it defaults to this repository's `overlay`
directory when present, otherwise the repository root.

The importer checks the compiled original client/renderer and on-demand/ActorTag
profile hashes, compiled catalogs, external room ZON files, complete EQG
archives, requested MOD/TER members and final generated geometry hashes. A different
client build, unsupported format or missing resource refuses installation. It
requires no external packages, game connection, service restart or database access.

Successful installation creates `housing-original-meshes` inside the server
working directory with mode `0700`; each private mesh has mode `0600`. All output
is validated in staging before individual SHA-named files are installed atomically.
Existing valid files are retained; corrupt files are refused. Interrupted imports
can be rerun. Keep the directory private and readable by the zone server account.

The runtime uses this directory relative to its working directory. To keep it
elsewhere, set `EQEMU_HOUSING_ORIGINAL_ASSETS` in the zone server environment to
the resulting `housing-original-meshes` directory. Do not commit or redistribute
that directory or any original client resources.
