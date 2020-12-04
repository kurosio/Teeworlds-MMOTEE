/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_BOUNCINGBULLET_H
#define GAME_SERVER_ENTITIES_BOUNCINGBULLET_H

class CBouncingBullet : public CEntity
{
public:
	int m_Owner;
	
public:
	CBouncingBullet(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, bool Explosive, int Weapon, int LifeSpan);

	vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj);

	virtual void Reset();
	virtual void Tick();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

private:
	vec2 m_ActualPos;
	vec2 m_ActualDir;
	vec2 m_Direction;
	int m_StartTick;
	int m_LifeSpan;
	int m_BounceLeft;
	float m_DistanceLeft;
	bool m_Explosive;
	int m_Weapon;
};

#endif
