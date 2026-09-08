//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose:
//
//=============================================================================

#ifndef NETMESSAGES_H
#define NETMESSAGES_H
#ifdef _WIN32
#pragma once
#endif

#include "inetchannel.h"
#include "networksystem/netmessage.h"
#include "networksystem/inetworkserializer.h"

#include "netmessages.pb.h"
#include "networkbasetypes.pb.h"
#include "networksystem_protomessages.pb.h"

#undef min
#undef max

/* =========================
Signon groups
========================= */

// hl2sdk-cs2 does not declare these; the values are the engine's own and are
// what every message below is tagged with. Index 12 is a gap in the engine.
enum SignonGroup_t : int32_t
{
    SG_INVALID      = -1,
    SG_GENERIC      = 0,
    SG_LOCALPLAYER  = 1,
    SG_OTHERPLAYER  = 2,
    SG_ENTITIES     = 3,
    SG_SOUNDS       = 4,
    SG_EVENTS       = 5,
    SG_VOICE        = 6,
    SG_STRINGTABLE  = 7,
    SG_MOVE         = 8,
    SG_STRINGCMD    = 9,
    SG_SIGNON       = 10,
    SG_SYSTEM       = 11,
    SG_USERMSG      = 13,
    SG_CLIENTMSG    = 14,
    SG_SPAWNGROUPS  = 15,
    SG_ENGINE       = 16,
    SG_HLTVREPLAY   = 17,
    SG_DECALS       = 18,

    SG_TOTAL        = 19,
};

inline constexpr const char* k_pszNetGroupNames[ SG_TOTAL ] =
{
    "Generic", "Local Player", "Other Players", "Entities", "Sounds", "Events",
    "Voice", "String Table", "Move", "String Command", "Signon", "System",
    "Unknown", "User Messages", "Client Messages", "Spawn Groups",
    "Game Engine", "Hltv Replay", "Decals"
};

/* =========================
Typed message wrappers
========================= */

// hl2sdk-cs2's CNetMessagePB is a one-parameter stub, and a class template
// cannot be redeclared with a different parameter list, so the message id,
// signon group and buffer type ride along here instead.
//
// No data members and no virtuals of its own, so the layout stays
// CNetMessagePB<T>'s. These wrappers are abstract, like the stub they derive
// from -- fine everywhere they are only cast onto engine memory.
template< int32_t ID, typename PROTO_TYPE, SignonGroup_t SIGNON = SG_GENERIC,
          NetChannelBufType_t BUF_TYPE = BUF_RELIABLE, bool IS_FOR_SERVER = false >
class CNetMessagePBTyped : public CNetMessagePB< PROTO_TYPE >
{
public:
    using MyType_t = CNetMessagePBTyped< ID, PROTO_TYPE, SIGNON, BUF_TYPE, IS_FOR_SERVER >;
    using PBType_t = PROTO_TYPE;

    static constexpr int32_t             kMsgId       = ID;
    static constexpr SignonGroup_t       kSignonGroup = SIGNON;
    static constexpr NetChannelBufType_t kBufType     = BUF_TYPE;
    static constexpr bool                kIsForServer = IS_FOR_SERVER;

    static constexpr const char* GetGroupName()
    {
        return ( kSignonGroup >= 0 && kSignonGroup < SG_TOTAL )
            ? k_pszNetGroupNames[ kSignonGroup ] : "Unknown";
    }
};

template< int32_t ID, typename PROTO_TYPE, NetChannelBufType_t BUF_TYPE = BUF_RELIABLE >
class CUserMessagePB : public CNetMessagePBTyped< ID, PROTO_TYPE, SG_USERMSG, BUF_TYPE > {};

/* =========================
System messages
========================= */

class NetMessageSplitscreenUserChanged_t :   public CNetMessagePBTyped< -1, NetMessageSplitscreenUserChanged, SG_SYSTEM > {};
class NetMessagePacketStart_t :              public CNetMessagePBTyped< -1, NetMessagePacketStart, SG_SYSTEM > {};
class NetMessagePacketEnd_t :                public CNetMessagePBTyped< -1, NetMessagePacketEnd, SG_SYSTEM > {};
class NetMessageConnectionClosed_t :         public CNetMessagePBTyped< -1, NetMessageConnectionClosed, SG_SYSTEM > {};
class NetMessageConnectionCrashed_t :        public CNetMessagePBTyped< -1, NetMessageConnectionCrashed, SG_SYSTEM > {};

/* =========================
Engine (net_*)
========================= */

class CNETMsg_Tick_t :                       public CNetMessagePBTyped< net_Tick, CNETMsg_Tick, SG_ENGINE, BUF_UNRELIABLE > {};
class CNETMsg_StringCmd_t :                  public CNetMessagePBTyped< net_StringCmd, CNETMsg_StringCmd, SG_STRINGCMD > {};
class CNETMsg_SetConVar_t :                  public CNetMessagePBTyped< net_SetConVar, CNETMsg_SetConVar, SG_STRINGCMD > {};
class CNETMsg_SignonState_t :                public CNetMessagePBTyped< net_SignonState, CNETMsg_SignonState, SG_SIGNON > {};
class CNETMsg_SpawnGroup_Load_t :            public CNetMessagePBTyped< net_SpawnGroup_Load, CNETMsg_SpawnGroup_Load, SG_SPAWNGROUPS > {};
class CNETMsg_SpawnGroup_ManifestUpdate_t :  public CNetMessagePBTyped< net_SpawnGroup_ManifestUpdate, CNETMsg_SpawnGroup_ManifestUpdate, SG_SPAWNGROUPS > {};
class CNETMsg_SpawnGroup_SetCreationTick_t : public CNetMessagePBTyped< net_SpawnGroup_SetCreationTick, CNETMsg_SpawnGroup_SetCreationTick, SG_SPAWNGROUPS > {};
class CNETMsg_SpawnGroup_Unload_t :          public CNetMessagePBTyped< net_SpawnGroup_Unload, CNETMsg_SpawnGroup_Unload, SG_SPAWNGROUPS > {};
class CNETMsg_SpawnGroup_LoadCompleted_t :   public CNetMessagePBTyped< net_SpawnGroup_LoadCompleted, CNETMsg_SpawnGroup_LoadCompleted, SG_SPAWNGROUPS > {};
class CNETMsg_DebugOverlay_t :               public CNetMessagePBTyped< net_DebugOverlay, CNETMsg_DebugOverlay, SG_ENGINE > {};

/* =========================
Client -> Server (clc_*)
========================= */

class CCLCMsg_ClientInfo_t :                 public CNetMessagePBTyped< clc_ClientInfo, CCLCMsg_ClientInfo, SG_SIGNON > {};
class CCLCMsg_Move_t :                       public CNetMessagePBTyped< clc_Move, CCLCMsg_Move, SG_MOVE, BUF_UNRELIABLE > {};
class CCLCMsg_VoiceData_t :                  public CNetMessagePBTyped< clc_VoiceData, CCLCMsg_VoiceData, SG_VOICE > {};
class CCLCMsg_BaselineAck_t :                public CNetMessagePBTyped< clc_BaselineAck, CCLCMsg_BaselineAck, SG_ENGINE > {};
class CCLCMsg_RespondCvarValue_t :           public CNetMessagePBTyped< clc_RespondCvarValue, CCLCMsg_RespondCvarValue > {};
class CCLCMsg_LoadingProgress_t :            public CNetMessagePBTyped< clc_LoadingProgress, CCLCMsg_LoadingProgress, SG_SIGNON > {};
class CCLCMsg_SplitPlayerConnect_t :         public CNetMessagePBTyped< clc_SplitPlayerConnect, CCLCMsg_SplitPlayerConnect, SG_SIGNON > {};
class CCLCMsg_SplitPlayerDisconnect_t :      public CNetMessagePBTyped< clc_SplitPlayerDisconnect, CCLCMsg_SplitPlayerDisconnect, SG_SIGNON > {};
class CCLCMsg_ServerStatus_t :               public CNetMessagePBTyped< clc_ServerStatus, CCLCMsg_ServerStatus > {};
class CCLCMsg_RequestPause_t :               public CNetMessagePBTyped< clc_RequestPause, CCLCMsg_RequestPause > {};
class CCLCMsg_CmdKeyValues_t :               public CNetMessagePBTyped< clc_CmdKeyValues, CCLCMsg_CmdKeyValues, SG_STRINGCMD > {};
class CCLCMsg_RconServerDetails_t :          public CNetMessagePBTyped< clc_RconServerDetails, CCLCMsg_RconServerDetails > {};
class CCLCMsg_HltvReplay_t :                 public CNetMessagePBTyped< clc_HltvReplay, CCLCMsg_HltvReplay, SG_HLTVREPLAY > {};
class CCLCMsg_Diagnostic_t :                 public CNetMessagePBTyped< clc_Diagnostic, CCLCMsg_Diagnostic > {};

/* =========================
Server -> Client (svc_*)
========================= */

class CSVCMsg_ServerInfo_t :                 public CNetMessagePBTyped< svc_ServerInfo, CSVCMsg_ServerInfo, SG_SIGNON > {};
class CSVCMsg_FlattenedSerializer_t :        public CNetMessagePBTyped< svc_FlattenedSerializer, CSVCMsg_FlattenedSerializer, SG_SIGNON > {};
class CSVCMsg_ClassInfo_t :                  public CNetMessagePBTyped< svc_ClassInfo, CSVCMsg_ClassInfo, SG_SIGNON > {};
class CSVCMsg_SetPause_t :                   public CNetMessagePBTyped< svc_SetPause, CSVCMsg_SetPause > {};
class CSVCMsg_CreateStringTable_t :          public CNetMessagePBTyped< svc_CreateStringTable, CSVCMsg_CreateStringTable, SG_SIGNON > {};
class CSVCMsg_UpdateStringTable_t :          public CNetMessagePBTyped< svc_UpdateStringTable, CSVCMsg_UpdateStringTable, SG_STRINGTABLE > {};
class CSVCMsg_VoiceInit_t :                  public CNetMessagePBTyped< svc_VoiceInit, CSVCMsg_VoiceInit, SG_SIGNON > {};
class CSVCMsg_VoiceData_t :                  public CNetMessagePBTyped< svc_VoiceData, CSVCMsg_VoiceData, SG_VOICE > {};
class CSVCMsg_Print_t :                      public CNetMessagePBTyped< svc_Print, CSVCMsg_Print > {};
class CSVCMsg_Sounds_t :                     public CNetMessagePBTyped< svc_Sounds, CSVCMsg_Sounds, SG_SOUNDS > {};
class CSVCMsg_SetView_t :                    public CNetMessagePBTyped< svc_SetView, CSVCMsg_SetView > {};
class CSVCMsg_ClearAllStringTables_t :       public CNetMessagePBTyped< svc_ClearAllStringTables, CSVCMsg_ClearAllStringTables, SG_SIGNON > {};
class CSVCMsg_CmdKeyValues_t :               public CNetMessagePBTyped< svc_CmdKeyValues, CSVCMsg_CmdKeyValues, SG_STRINGCMD > {};
class CSVCMsg_BSPDecal_t :                   public CNetMessagePBTyped< svc_BSPDecal, CSVCMsg_BSPDecal > {};
class CSVCMsg_SplitScreen_t :                public CNetMessagePBTyped< svc_SplitScreen, CSVCMsg_SplitScreen, SG_SIGNON > {};
// The one wrapper that cannot stay abstract: CServerSideClientBase embeds it by
// value, and CNetMessage leaves AsProto/AsProto2/GetNetMessage/CopyConstruct
// pure. Filling the four slots adds no members and no virtuals of its own, so
// the layout is unchanged; the bodies are never reached, since this is only
// ever cast onto engine memory.
class CSVCMsg_PacketEntities_t : public CNetMessagePBTyped< svc_PacketEntities, CSVCMsg_PacketEntities, SG_INVALID >
{
public:
    void* AsProto() const override { return nullptr; }
    void* AsProto2() const override { return nullptr; }
    INetworkMessageInternal* GetNetMessage() const override { return nullptr; }
    CNetMessage* CopyConstruct( const CNetMessage* other ) const override { return nullptr; }
};
class CSVCMsg_Prefetch_t :                   public CNetMessagePBTyped< svc_Prefetch, CSVCMsg_Prefetch, SG_SOUNDS > {};
class CSVCMsg_Menu_t :                       public CNetMessagePBTyped< svc_Menu, CSVCMsg_Menu > {};
class CSVCMsg_GetCvarValue_t :               public CNetMessagePBTyped< svc_GetCvarValue, CSVCMsg_GetCvarValue > {};
class CSVCMsg_StopSound_t :                  public CNetMessagePBTyped< svc_StopSound, CSVCMsg_StopSound, SG_SOUNDS > {};
class CSVCMsg_PeerList_t :                   public CNetMessagePBTyped< svc_PeerList, CSVCMsg_PeerList > {};
class CSVCMsg_PacketReliable_t :             public CNetMessagePBTyped< svc_PacketReliable, CSVCMsg_PacketReliable > {};
class CSVCMsg_HLTVStatus_t :                 public CNetMessagePBTyped< svc_HLTVStatus, CSVCMsg_HLTVStatus, SG_GENERIC, BUF_RELIABLE, true > {};
class CSVCMsg_ServerSteamID_t :              public CNetMessagePBTyped< svc_ServerSteamID, CSVCMsg_ServerSteamID, SG_GENERIC, BUF_RELIABLE, true > {};
class CSVCMsg_FullFrameSplit_t :             public CNetMessagePBTyped< svc_FullFrameSplit, CSVCMsg_FullFrameSplit > {};
class CSVCMsg_RconServerDetails_t :          public CNetMessagePBTyped< svc_RconServerDetails, CSVCMsg_RconServerDetails > {};
class CSVCMsg_UserMessage_t :                public CNetMessagePBTyped< svc_UserMessage, CSVCMsg_UserMessage, SG_USERMSG > {};
class CSVCMsg_Broadcast_Command_t :          public CNetMessagePBTyped< svc_Broadcast_Command, CSVCMsg_Broadcast_Command, SG_HLTVREPLAY > {};
class CSVCMsg_HltvFixupOperatorStatus_t :    public CNetMessagePBTyped< svc_HltvFixupOperatorStatus, CSVCMsg_HltvFixupOperatorStatus > {};
class CSVCMsg_UserCommands_t :               public CNetMessagePBTyped< svc_UserCmds, CSVCMsg_UserCommands, SG_HLTVREPLAY > {};

#endif // NETMESSAGES_H
