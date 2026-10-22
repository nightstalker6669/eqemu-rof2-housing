# Upgrade an AkkStack server

This runs the **complete housing installer in your existing AkkStack environment**. It keeps the database container running and stops the game server during installation. You do not need to reinstall AkkStack or copy SQL files individually.

## 1. Get the package and identify your folders

Open a terminal on the Docker host. In these examples, AkkStack is `/opt/akk-stack`, the package is `/opt/eqemu-rof2-housing`, and your own matching RoF2 client is `/srv/clients/RoF2`. Replace those three paths with your actual locations.

```sh
git -c core.autocrlf=false clone https://github.com/nightstalker6669/eqemu-rof2-housing.git /opt/eqemu-rof2-housing
cd /opt/akk-stack
export HOUSING_PACKAGE=/opt/eqemu-rof2-housing
export HOUSING_CLIENT=/srv/clients/RoF2
export HOUSING_BACKUPS=/opt/akk-stack/housing-backups
mkdir -p "$HOUSING_BACKUPS"
chmod 700 "$HOUSING_BACKUPS"
```

Keep these variables set in this terminal. The supplied [Compose overlay](../deploy/akk-stack.housing.yml) mounts the package and client read-only and exposes a private backup folder. The usual AkkStack `server`, `code` and `assets` mounts remain in use.

**Source requirement:** `code/EQEmu` must contain upstream commit `8175ae61874cbf20d7b05102821f6f8a4f4d1c39`. If it is missing, fetch it without changing your current checkout:

```sh
git -C code/EQEmu fetch origin 8175ae61874cbf20d7b05102821f6f8a4f4d1c39
```

The installer builds the housing package against that pinned source in a separate managed directory. It preserves your original source files, but does **not** merge custom C++ changes into the new binaries. Review [source compatibility](INSTALL.md#source-compatibility-before-you-upgrade) before proceeding.

## 2. Check the plan

```sh
docker compose -f docker-compose.yml \
  -f "$HOUSING_PACKAGE/deploy/akk-stack.housing.yml" \
  run --rm --no-deps -T --entrypoint python3 eqemu-server \
  /opt/housing-package/tools/install_housing.py \
  --source /home/eqemu/code/EQEmu \
  --server /home/eqemu/server \
  --client /opt/housing-client \
  --build /home/eqemu/code/housing-build \
  --backup /home/eqemu/housing-backups/housing-01
```

This command checks prerequisites and prints a plan. It does not apply the housing update or start a second game server. Read any refusal before continuing. MariaDB needs `max_allowed_packet` of at least 64MiB; configure that in your database service if the plan reports a smaller value.

The installer detects the deployed binaries, opcode directory, quests and maps from the server configuration. Typical AkkStack paths are `/home/eqemu/server/bin`, `/home/eqemu/server/assets/patches`, `/home/eqemu/server/quests`, and `/home/eqemu/server/maps`. Use the directory overrides described in [INSTALL](INSTALL.md) when your layout differs. Paths passed here are **container paths**, not host paths.

## 3. Stop, apply and start

Ask players to log out, disable any external updater/auto-launcher, then stop the normal game container:

```sh
docker compose stop eqemu-server
```

Keep MariaDB running. Repeat the plan command with `--apply` added. If the source warning requires it, add `--accept-pinned-server-build` only after accepting the pinned-build policy above. Use a **new backup name** for each application, such as `housing-02` on a later upgrade.

The command takes private backups, installs/adopts the ordered schemas, builds all eight matching binaries, imports your client geometry, installs house profiles and service content, merges housing opcodes, builds the cache and provisions the neighborhood. Wait for `HOUSING_INSTALL_PASS`.

Only after success:

```sh
docker compose up -d eqemu-server
docker compose logs --tail 80 eqemu-server
```

If installation fails, leave the game container stopped. Read the named private step log. Do not start old binaries against a partly upgraded database or let an ordinary upstream binary updater overwrite the housing build.

## 4. Check in game

Connect with the original May 10, 2013 RoF2 client. Visit Sunrise Hills, inspect a plot marker, and confirm a personal/guild housing workflow. Existing properties and retained item identities are preserved; you do not receive a fabricated owner or free character.

Use the same `--key` on repeat installations so the neighborhood is not duplicated. Choose a new key/name only when deliberately creating another neighborhood. Database-specific initialization and existing-property adoption limits are explained in [INSTALL](INSTALL.md).

## Roll back

With the game container stopped, use the **same Compose overlay and mounts**:

```sh
docker compose -f docker-compose.yml \
  -f "$HOUSING_PACKAGE/deploy/akk-stack.housing.yml" \
  run --rm --no-deps -T --entrypoint python3 eqemu-server \
  /opt/housing-package/tools/restore_housing.py \
  --server /home/eqemu/server \
  --backup /home/eqemu/housing-backups/housing-01
```

Inspect the restore plan, then repeat with `--apply`. Restore the database and its matching binaries/cache together. Start the regular container only after the restore succeeds. A changed deployment link or unresolved inventory application is a refusal to investigate, not a reason to delete custody records.

## Build tools and Compose variations

The container must provide Python 3.9+, Git, CMake, a C++20 compiler, MariaDB client/dump tools and the pinned server build dependencies. If your server image lacks the development tools, use AkkStack's documented development image/override in the one-off installer command. Keep the same server/code/assets mounts and database network; do not substitute an unrelated empty container. Include any Compose override files you normally use **before** `akk-stack.housing.yml` in each command. Older installations may use `docker-compose` instead of `docker compose`.

These commands follow the [official AkkStack volume layout](https://github.com/EQEmu/akk-stack/blob/master/docker-compose.yml) and [development documentation](https://docs.eqemu.dev/akk-stack/operate/development/). Local customizations still need the directory and image checks above.
