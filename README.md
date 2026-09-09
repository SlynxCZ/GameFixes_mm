# GameFixes_mm

**One Metamod:Source plugin for the CS2 server fixes that used to be five separate ones (BeamCrashFix, DemoRecordFix, HammerIdFix, SlowAnimationFix, WorkshopVoiceFix), the GC-ban whitelist that used to live in FUNPLAY Core, and a few more (team limit, input activator crash, sv_cheats noclip, server list players).** Every fix is toggled in `game_fixes.ini`.

---

## Fixes

| Block | What it fixes |
|---|---|
| `beam_crash` | `CBeam::SetBeamOrigin` / `SetBeamEndPos` loop forever on a beam without a parent, crashing the server. The unparented path is reimplemented. |
| `demo_record` | With `tv_enable 1`, the GOTV client is never disconnected and gets a slot on top of the session's maxplayers, so demo recording survives match restarts on the same map. |
| `hammer_id` | `SetSchemaHammerUniqueId` skips `m_sUniqueHammerID` behind a `jnz`; patched to `jmp` so every entity gets its id. |
| `slow_animation` | `curtime` is a 32-bit float and loses precision after a day or two on one map (sluggish animations, movement, lag compensation). While the server is empty at a check, the map is reloaded; the remaining `mp_timelimit` is carried over. `reload_interval` sets the check period in seconds. |
| `workshop_voice` | Clients key voice playback off the `xuid` in `svc_VoiceData`; on workshop maps it doesn't identify the speaker uniquely, streams collide and players stop hearing each other. Every speaker gets a distinct xuid per listener, rewritten in place in `CServerSideClient::SendNetMessage` (once per recipient). Seeds reset on every map change. |
| `steam_ban` | The engine's per-frame GC ban / competitive cooldown kick pass (`GameSystem_Think_CheckSteamBan`, hooked by signature). Accounts in `whitelist` (SteamID64s) are stripped out before it runs, so are competitive cooldowns while `sv_kick_players_with_cooldown` is below 2, and with `clear_after_pass` whatever it leaves behind is cleared so a stale entry can't hit the next player to join. |
| `team_limit` | On every `round_start`, `CCSGameRules`' spawnable/max T and CT counts are raised to maxplayers, so a join is never refused because a team is "full". |
| `input_activator_crash` | `CBaseFilter::InputTestActivator` dereferences the input's activator unchecked; a `TestActivator` fired with none crashes the server. Those calls are dropped. |
| `sv_cheats` | Turning `sv_cheats` off leaves anyone in noclip flying. Every such pawn goes back to `MOVETYPE_WALK` the moment the cvar flips to 0. |
| `server_list_players` | Pushes every connected player's SteamID, name and score to the Steam game server API every `update_interval` seconds, so the server browser lists the players. Port of [Source2ZE/ServerListPlayersFix](https://github.com/Source2ZE/ServerListPlayersFix). |

A fix that fails to find what it needs (a signature, a vtable) is logged and left off; the rest of the plugin still loads.

## Configuration

`addons/game_fixes/game_fixes.ini` -- one block per fix, `"enable" "1"` turns it on, a missing block counts as off. Extra options live in the same block:

```
"slow_animation"
{
	"enable"			"1"
	"reload_interval"	"1800"
}

"steam_ban"
{
	"enable"	"1"
	"clear_after_pass"	"1"
	"whitelist"
	{
		"Slynx"	"76561198000000000"
	}
}
```

## Layout

```
src/
  plugin.*          metamod glue: interfaces, ini, the shared GameFrame/StartupServer hooks
  scheduler.*       timers + next-frame queue, ticked from GameFrame
  utils.hpp         WIN_LINUX and WriteCode, the one code-byte patch (on DynLibUtils)
  fixes/fix.h       the CFix interface every fix implements
  fixes/<name>.*    one fix per file pair
  sdk/              schema field access and the few SDK classes the fixes touch
```

Hooks go through KHook, metamod's own detour library (`third_party/khook` in metamod-source, handed to the plugin by `PLUGIN_SAVEVARS()`), so they share one detour backend with every other plugin. Signature scanning and vtable lookup use `vendor/dynlibutils`.

## Building

Requires `HL2SDKCS2`, `MMSOURCE_DEV` and `CSGO_PROTO` in the environment (hl2sdk `cs2` branch, metamod-source, SteamDatabase Protobufs `csgo/`).

CMake, for local development (CLion):

```
git clone --recursive https://github.com/SlynxCZ/GameFixes_mm
cmake -S GameFixes_mm -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

`build/addons/` is the complete install tree (binary, `game_fixes.ini`, metamod `.vdf`); copy it into `game/csgo/`.

AMBuild, what CI uses (`docker/` on Linux, MSVC on Windows), same as the other `*_mm` plugins:

```
mkdir build && cd build
python ../configure.py --enable-optimize --sdks cs2 --mms_path=$MMSOURCE_DEV --hl2sdk-root=$HL2SDKCS2 --hl2sdk-manifests=$MMSOURCE_DEV/hl2sdk-manifests
ambuild
```

The package lands in `build/package/cs2/`; add `configs/addons/game_fixes/game_fixes.ini` next to it.

## Requirements

* Metamod:Source (CS2)

## Author

Slynx (˙·٠● S l y n x ●٠·˙)
[https://slynxdev.cz](https://slynxdev.cz)
