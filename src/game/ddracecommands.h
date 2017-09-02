/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#ifndef GAME_SERVER_DDRACECOMMANDS_H
#define GAME_SERVER_DDRACECOMMANDS_H
#undef GAME_SERVER_DDRACECOMMANDS_H // this file can be included several times
#ifndef CONSOLE_COMMAND
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help)
#endif

//specials
CONSOLE_COMMAND("kill_pl", "v[id]", CFGFLAG_SERVER, ConKillPlayer, this, "Kills player v and announces the kill")
CONSOLE_COMMAND("levelreset_pl", "v[id]", CFGFLAG_SERVER, ConLevelReset, this, "reset player v level")
CONSOLE_COMMAND("endless_pl", "v[id]", CFGFLAG_SERVER, ConEndless, this, "give/remove player v endless")
CONSOLE_COMMAND("epic_circles_pl", "v[id]", CFGFLAG_SERVER, ConEpicCircles, this, "give/remove player v epic circles")
CONSOLE_COMMAND("pullhammer_pl", "v[id]", CFGFLAG_SERVER, ConPullhammer, this, "give/remove player v pullhammer")
CONSOLE_COMMAND("xxl_pl", "v[id]", CFGFLAG_SERVER, ConXXL, this, "give/remove player v xxl")
CONSOLE_COMMAND("smarthammer_pl", "v[id]", CFGFLAG_SERVER, ConSmarthammer, this, "give/remove player v smarthammer")
CONSOLE_COMMAND("bloody_pl", "v[id]", CFGFLAG_SERVER, ConBloody, this, "give/remove player v bloody")
CONSOLE_COMMAND("steamy_pl", "v[id]", CFGFLAG_SERVER, ConSteamy, this, "give/remove player v steamy")
CONSOLE_COMMAND("rainbow_pl", "v[id]", CFGFLAG_SERVER, ConRainbow, this, "give/remove player v rainbow")
CONSOLE_COMMAND("epiletic_rainbow_pl", "v[id]", CFGFLAG_SERVER, ConEpileticRainbow, this, "give/remove player v epiletic rainbow")
CONSOLE_COMMAND("lovely_pl", "v[id]", CFGFLAG_SERVER, ConLovely, this, "give/remove player v lovely")
CONSOLE_COMMAND("rotating_hearts_pl", "v[id]", CFGFLAG_SERVER, ConRotatingHearts, this, "give/remove player v rotating hearts")
CONSOLE_COMMAND("ball_pl", "v[id]", CFGFLAG_SERVER, ConBall, this, "give/remove player v ball")
CONSOLE_COMMAND("heartguns_pl", "v[id]", CFGFLAG_SERVER, ConHeartGuns, this, "give/remove player v heartguns")
CONSOLE_COMMAND("rainbowhook_pl", "v[id]", CFGFLAG_SERVER, ConRainbowHook, this, "give/remove player v rainbow hook")
CONSOLE_COMMAND("invisible_pl", "v[id]", CFGFLAG_SERVER, ConInvisible, this, "give/remove player v invisible")
CONSOLE_COMMAND("vip_pl", "v[id]", CFGFLAG_SERVER, ConVip, this, "give/remove player v vip")
CONSOLE_COMMAND("check_vip_pl", "v[id]", CFGFLAG_SERVER, ConCheckVip, this, "check player v for vip")
CONSOLE_COMMAND("rename_pl", "vr", CFGFLAG_SERVER|CMDFLAG_TEST, ConRename, this, "Renames i name to s")
CONSOLE_COMMAND("hammer_pl", "vr", CFGFLAG_SERVER | CMDFLAG_TEST, ConHL, this, "give player v hammer level x")
CONSOLE_COMMAND("blackhole_pl", "v[id]", CFGFLAG_SERVER | CMDFLAG_TEST, ConBlackhole, this, "give/remove player v blackhole")
CONSOLE_COMMAND("skin_pl", "vs", CFGFLAG_SERVER|CMDFLAG_TEST, ConSkin, this, "Changes the skin from i in s")
CONSOLE_COMMAND("magnetgrocket_pl", "v[id]", CFGFLAG_SERVER, ConRocket, this, "give player v stars")
CONSOLE_COMMAND("stars_pl", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConStars, this, "Changes the skin from i in s")
CONSOLE_COMMAND("clan_pl", "vr", CFGFLAG_SERVER|CMDFLAG_TEST, ConClan, this, "Renames i name to s")
CONSOLE_COMMAND("freeze", "v?i", CFGFLAG_SERVER|CMDFLAG_TEST, ConFreeze, this, "Freezes player v for i seconds (infinite by default)")
CONSOLE_COMMAND("unfreeze", "v", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnFreeze, this, "Unfreezes player v")
CONSOLE_COMMAND("send_sound_pl", "ii", CFGFLAG_SERVER|CMDFLAG_TEST, ConSendSound, this, "send sound i to a player i")
CONSOLE_COMMAND("hook_jetpack_pl", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConHookJetpack, this, "give/remove player v hook jetpack")
CONSOLE_COMMAND("light_saber_pl", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConLightSaber, this, "give/remove player v light saber")
CONSOLE_COMMAND("lightning_laser_pl", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConLightningLaser, this, "give/remove player v lightning laser")

CONSOLE_COMMAND("totele", "i[number]", CFGFLAG_SERVER|CMDFLAG_TEST, ConToTeleporter, this, "Teleports you to teleporter v")
CONSOLE_COMMAND("totelecp", "i[number]", CFGFLAG_SERVER|CMDFLAG_TEST, ConToCheckTeleporter, this, "Teleports you to checkpoint teleporter v")
CONSOLE_COMMAND("tele", "v[id] ?i[number]", CFGFLAG_SERVER|CMDFLAG_TEST, ConTeleport, this, "Teleports you (or player v) to player i")
CONSOLE_COMMAND("addweapon", "v[id] i[weapon-id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConAddWeapon, this, "Gives weapon with id i to you (all = -1, hammer = 0, gun = 1, shotgun = 2, grenade = 3, rifle = 4, ninja = 5)")
CONSOLE_COMMAND("removeweapon", "v[id] i[weapon-id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConRemoveWeapon, this, "removes weapon with id i from you (all = -1, hammer = 0, gun = 1, shotgun = 2, grenade = 3, rifle = 4)")
CONSOLE_COMMAND("shotgun", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConShotgun, this, "Gives a shotgun to you")
CONSOLE_COMMAND("grenade", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConGrenade, this, "Gives a grenade launcher to you")
CONSOLE_COMMAND("rifle", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConRifle, this, "Gives a rifle to you")
CONSOLE_COMMAND("jetpack","", CFGFLAG_SERVER|CMDFLAG_TEST, ConJetpack, this, "Gives jetpack to you")
CONSOLE_COMMAND("weapons", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConWeapons, this, "Gives all weapons to you")
CONSOLE_COMMAND("unshotgun", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnShotgun, this, "Takes the shotgun from you")
CONSOLE_COMMAND("ungrenade", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnGrenade, this, "Takes the grenade launcher you")
CONSOLE_COMMAND("unrifle", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnRifle, this, "Takes the rifle from you")
CONSOLE_COMMAND("unjetpack", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnJetpack, this, "Takes the jetpack from you")
CONSOLE_COMMAND("unweapons", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnWeapons, this, "Takes all weapons from you")
CONSOLE_COMMAND("ninja", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConNinja, this, "Makes you a ninja")
CONSOLE_COMMAND("super", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConSuper, this, "Makes you super")
CONSOLE_COMMAND("unsuper", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnSuper, this, "Removes super from you")
CONSOLE_COMMAND("unsolo", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnSolo, this, "Puts you out of solo part")
CONSOLE_COMMAND("undeep", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnDeep, this, "Puts you out of deep freeze")
CONSOLE_COMMAND("left", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoLeft, this, "Makes you move 1 tile left")
CONSOLE_COMMAND("right", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoRight, this, "Makes you move 1 tile right")
CONSOLE_COMMAND("up", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoUp, this, "Makes you move 1 tile up")
CONSOLE_COMMAND("down", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoDown, this, "Makes you move 1 tile down")

CONSOLE_COMMAND("move", "i[x] i[y]", CFGFLAG_SERVER|CMDFLAG_TEST, ConMove, this, "Moves to the tile with x/y-number ii")
CONSOLE_COMMAND("move_raw", "i[x] i[y]", CFGFLAG_SERVER|CMDFLAG_TEST, ConMoveRaw, this, "Moves to the point with x/y-coordinates ii")
CONSOLE_COMMAND("force_pause", "i[id] i[seconds]", CFGFLAG_SERVER, ConForcePause, this, "Force i to pause for i seconds")
CONSOLE_COMMAND("force_unpause", "i[id]", CFGFLAG_SERVER, ConForcePause, this, "Set force-pause timer of i to 0.")
CONSOLE_COMMAND("showothers", "?i['0'|'1']", CFGFLAG_CHAT, ConShowOthers, this, "Whether to show players from other teams or not (off by default), optional i = 0 for off else for on")
CONSOLE_COMMAND("showall", "?i['0'|'1']", CFGFLAG_CHAT, ConShowAll, this, "Whether to show players at any distance (off by default), optional i = 0 for off else for on")

CONSOLE_COMMAND("list", "?s[filter]", CFGFLAG_CHAT, ConList, this, "List connected players with optional case-insensitive substring matching filter")

CONSOLE_COMMAND("mute", "", CFGFLAG_SERVER, ConMute, this, "")
CONSOLE_COMMAND("muteid", "v[id] i[seconds]", CFGFLAG_SERVER, ConMuteID, this, "")
CONSOLE_COMMAND("silent_muteid", "v[id] i[seconds]", CFGFLAG_SERVER, ConSilentMuteID, this, "")
CONSOLE_COMMAND("muteip", "s[ip] i[seconds]", CFGFLAG_SERVER, ConMuteIP, this, "")
CONSOLE_COMMAND("unmute", "v[id]", CFGFLAG_SERVER, ConUnmute, this, "")
CONSOLE_COMMAND("mutes", "", CFGFLAG_SERVER, ConMutes, this, "")

CONSOLE_COMMAND("freezehammer", "v[id]", CFGFLAG_SERVER, ConFreezeHammer, this, "Gives a player Freeze Hammer")
CONSOLE_COMMAND("unfreezehammer", "v[id]", CFGFLAG_SERVER, ConUnFreezeHammer, this, "Removes Freeze Hammer from a player")

CONSOLE_COMMAND("fixaccounts", "", CFGFLAG_SERVER, ConFixAccounts, this, "allow players to login back.")
CONSOLE_COMMAND("exec_command_by", "ir", CFGFLAG_SERVER, ConPublicExecCommand, this, "run a chat command from every player (public!, they can see it)")
CONSOLE_COMMAND("event_exp", "ii", CFGFLAG_SERVER, ConEventExp, this, "start an even Exp")

#undef CONSOLE_COMMAND

#endif
