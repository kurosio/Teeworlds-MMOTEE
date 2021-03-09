/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include "npcs.h"
#include "../character.h"
#include "../projectile.h"

MACRO_ALLOC_POOL_ID_IMPL(CNpcSold, MAX_CLIENTS)

CNpcSold::CNpcSold(CGameWorld *pWorld)
: CCharacter(pWorld)
{
	m_BotDir = 1;
	m_BotLastPos = m_Pos;
	m_BotLastStuckTime = 0.0f;
	m_BotStuckCount = 0;
	m_BotTimePlayerFound = Server()->Tick();
	m_BotTimeGrounded = Server()->Tick();
	m_BotTimeLastOption = Server()->Tick();
	m_BotTimeLastDamage = 0.0f;
	m_BotClientIDFix = -1;
	m_BotTimeLastSound = Server()->Tick();
	m_BotTimeLastChat = Server()->Tick();
	m_BotJumpTry = false;
}

void CNpcSold::Tick()
{
	TickBotAI();
	CCharacter::Tick();
}

bool CNpcSold::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if (CCharacter::TakeDamage(Force, Dmg, From, Weapon, 0))
	{
		m_BotTimeLastDamage = Server()->Tick();
		return true;
	}

	return false;
}

void CNpcSold::PlaySound()
{

}

void CNpcSold::RunAction()
{
	if (m_BotClientIDFix != -1 && GameServer()->m_apPlayers[m_BotClientIDFix])
	{
		CCharacter *pChar = GameServer()->m_apPlayers[m_BotClientIDFix]->GetCharacter();
		if (!pChar)
		{
			m_BotClientIDFix = -1;
			return;
		}

		if (Server()->Tick() - m_BotTimePlayerFound > Server()->TickSpeed()*0.01f)
		{
			m_LatestInput.m_Fire = m_Input.m_Fire = 1;
			m_BotTimePlayerFound = Server()->Tick();
		}

		m_BotClientIDFix = -1;
	}
}

void CNpcSold::TickBotAI()
{
    // ЗВУКИ
    if (Server()->Tick() - m_BotTimeLastSound > Server()->TickSpeed()*5.0f && !(rand()%50))
    {
        PlaySound();
        m_BotTimeLastSound = Server()->Tick();
    }

    EmoteNormal = EMOTE_HAPPY;

    // ОЧИСТКА ДЕЙСТВИЙ
	m_Input.m_Hook = 0;
	m_Input.m_Fire = 0;
	m_Input.m_Jump = 0;

    // ВЫПОЛНЕНИЕ СЦЕНАРИЯ
    RunAction();

    // ИНТЕРАКТ С ИГРОКАМИ
    bool PlayerClose = false;
    bool PlayerFound = false;
    bool PlayerNFound = false;
    float LessDist = 500.0f;
	int Action = rand()%3;

    m_BotClientIDFix = -1;


	int Dists = distance(LockBotPos, m_Pos);
	if (Dists < LessDist)
		LessDist = Dists;


	if (Dists >= 400.0f)
	{
		vec2 DirPlayer = normalize(LockBotPos - m_Pos);
		if (DirPlayer.x < 0)
			m_BotDir = -1;
		else
			m_BotDir = 1;


		if (Dists >= 430.0f)
		{
		   //Interact with the envirionment
			float radiusZ = ms_PhysSize/2.0f;
			if (distance(m_Pos, m_BotLastPos) < radiusZ || abs(m_Pos.x-m_BotLastPos.x) < radiusZ)
			{
				if (Server()->Tick() - m_BotLastStuckTime > Server()->TickSpeed()*0.5f)
				{
					m_BotStuckCount++;
					if (m_BotStuckCount == 15)
					{
						if (!m_BotJumpTry)
						{
							m_Input.m_Jump = 1;
							m_BotJumpTry = true;
						}
						else
						{
							m_BotDir = (!(rand()%2))?1:-1;
							m_BotJumpTry = false;
						}

						m_BotStuckCount = 0;
						m_BotLastStuckTime = Server()->Tick();
					}
				}
			}
		}
	}
	else if(Dists < 400.0f)
	{
		for (int i=0; i<g_Config.m_SvMaxClients; i++)
		{
			CPlayer *pPlayer = GameServer()->m_apPlayers[i];
			if (!pPlayer || !pPlayer->GetCharacter() || (!pPlayer->m_Search && !pPlayer->IsBot()) || (pPlayer->IsBot() && pPlayer->GetBotType() == BOT_NPC))
				continue;

			int Dist = distance(pPlayer->GetCharacter()->m_Pos, m_Pos);
			if (Dist < LessDist)
				LessDist = Dist;
			else
				continue;

			if (Dist < 400.0f && Dists < 400.0f)
			{

				if (Dist > 200.0f)
				{
					vec2 DirPlayer = normalize(pPlayer->GetCharacter()->m_Pos - m_Pos);
					if (DirPlayer.x < 0)
						m_BotDir = -1;
					else
						m_BotDir = 1;

					m_Input.m_Hook = 1;
				}
				else
				{
					PlayerClose = true;
					m_BotClientIDFix = pPlayer->GetCID();
				}

				m_Input.m_TargetX = static_cast<int>(pPlayer->GetCharacter()->m_Pos.x - m_Pos.x);
				m_Input.m_TargetY = static_cast<int>(pPlayer->GetCharacter()->m_Pos.y - m_Pos.y);

				PlayerFound = true;
			}
		}

		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			CPlayer *pPlayer = GameServer()->m_apPlayers[i];
			if (!pPlayer || !pPlayer->GetCharacter() || pPlayer->m_Search || (pPlayer->IsBot() && pPlayer->GetBotType() == BOT_NPC))
				continue;

			int Dist = distance(pPlayer->GetCharacter()->m_Pos, m_Pos);
			if (Dist < LessDist)
				LessDist = Dist;
			else
				continue;

			if (Dist < 50.0f && Dists < 400.0f)
			{
				if(Server()->Tick()-m_BotTimeLastOption > Server()->TickSpeed()*1.0f)
				{
					GameServer()->SendEmoticon(m_pPlayer->GetCID(), 14);
					m_BotDir = 0;
				}
				// ЧАТ
				if (Server()->Tick()-m_BotTimeLastChat > 20*Server()->TickSpeed())
				{
					GameServer()->SendEmoticon(m_pPlayer->GetCID(), 4);
					if(Server()->GetItemSettings(i, SCHAT)  != 2)
					{
						static const char *pPhrases[3] = {
							"[Guard] Hello {str:name}, today is a good day!!",
							"[Guard] {str:name}. Do you think we'll save?",
							"[Guard] {str:name}. Live in the capital. This bad...",
						};
						GameServer()->SendChatTarget_Localization(i, CHATCATEGORY_ACCUSATION, _(pPhrases[rand()%3]), "name", Server()->ClientName(i), NULL);
					}
					m_BotTimeLastChat = Server()->Tick();
				}

				m_Input.m_TargetX = static_cast<int>(pPlayer->GetCharacter()->m_Pos.x - m_Pos.x);
				m_Input.m_TargetY = static_cast<int>(pPlayer->GetCharacter()->m_Pos.y - m_Pos.y);

				PlayerNFound = true;
			}
		}
	}

	// РАНДОМНЫЙ СЦЕНАРИЙ
	if (!PlayerFound)
	{
		if (Server()->Tick()-m_BotTimeLastOption > Server()->TickSpeed()*1.0f)
		{
			if (Action == 0)
				m_BotDir = -1;
			else if (Action == 1)
				m_BotDir = 1;
			else if (Action == 2)
				m_BotDir = 0;

			m_BotTimeLastOption = Server()->Tick();

			if(Action == 2)
				m_BotTimeLastOption = Server()->Tick();
		}
	}

    //Fix target
    if (!PlayerFound && !PlayerNFound)
    {
        m_Input.m_TargetX = m_BotDir;
        m_Input.m_TargetY = 0;
    }
    else if (m_BotClientIDFix != -1)
    {
    	CPlayer *pPlayer = GameServer()->m_apPlayers[m_BotClientIDFix];
    	if (pPlayer && pPlayer->GetCharacter() && m_Pos.y > pPlayer->GetCharacter()->m_Pos.y) // Jump to player
    		m_Input.m_Jump = 1;
    }

    //Fix Stuck
    if (IsGrounded())
        m_BotTimeGrounded = Server()->Tick();

    //Falls
    if (m_Core.m_Vel.y > GameServer()->Tuning()->m_Gravity)
    {
    	if (m_BotClientIDFix != -1)
    	{
    		CPlayer *pPlayer = GameServer()->m_apPlayers[m_BotClientIDFix];
    		if (pPlayer && pPlayer->GetCharacter() && m_Pos.y > pPlayer->GetCharacter()->m_Pos.y)
    			m_Input.m_Jump = 1;
    		else
    			m_Input.m_Jump = 0;
    	}
    	else
    		m_Input.m_Jump = 1;
    }

    //Limits
    int tx = m_Pos.x+m_BotDir*45.0f;
    if (tx < 0)
        m_BotDir = 1;
    else if (tx >= GameServer()->Collision()->GetWidth()*32.0f)
    	m_BotDir = -1;

    //Delay of actions
    if (!PlayerClose)
        m_BotTimePlayerFound = Server()->Tick();

    //Set data
    m_Input.m_Direction = m_BotDir;
	m_Input.m_PlayerFlags = PLAYERFLAG_PLAYING;
	//Check for legacy input
	if (m_LatestPrevInput.m_Fire && m_Input.m_Fire) m_Input.m_Fire = 0;
	if (m_LatestInput.m_Jump && m_Input.m_Jump) m_Input.m_Jump = 0;
	//Ceck Double Jump
	if (m_Input.m_Jump && (m_Jumped&1) && !(m_Jumped&2) && m_Core.m_Vel.y < GameServer()->Tuning()->m_Gravity)
		m_Input.m_Jump = 0;

	m_LatestPrevInput = m_LatestInput;
	m_LatestInput = m_Input;
	m_BotLastPos = m_Pos;
	CCharacter::FireWeapon();
}
