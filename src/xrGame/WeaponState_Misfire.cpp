/******************************/
/***** Состояние "Осечки" *****/ //--#SM+#--
/******************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

// Требуется перейти в состояние осечки
void CWeapon::Need2Misfire() { SwitchState(eMisfire); }

// Переключение стейта на осечку
void CWeapon::switch2_Misfire()
{
    // Alundaio
#ifdef EXTENDED_WEAPON_CALLBACKS
    CGameObject* object = smart_cast<CGameObject*>(H_Parent());
    if (object)
        object->callback(GameObject::eOnWeaponJammed)(object->lua_game_object(), this->lua_game_object());
#endif
    //-Alundaio

    if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
        CurrentGameUI()->AddCustomStatic("gun_jammed", true);

    bMisfire = true;
}

// Переключение на другой стэйт из стэйта "Осечка"
void CWeapon::switchFrom_Misfire(u32 newS) {}

// Обновление оружия в состоянии "Осечка"
void CWeapon::state_Misfire(float dt)
{
    bMisfire = true;

    OnEmptyClick(true);
    UpdateSounds();

    Need2Idle();
}

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

// Проверка на осечку
BOOL CWeapon::IsMisfire() const
{
    if (m_bGrenadeMode)
        return false;
    return bMisfire;
}

// Включаем, с определённой вероятностью, осечку для оружия
BOOL CWeapon::CheckForMisfire()
{
    if (OnClient())
        return FALSE;

    float rnd = ::Random.randF(0.f, 1.f);
    float mp  = GetConditionMisfireProbability();
    if (rnd < mp)
    {
        Need2Misfire();
        return TRUE;
    }

    return FALSE;
}

// Рассчитать вероятность осечки при текущем состоянии оружия
float CWeapon::GetConditionMisfireProbability() const
{
    if (GetCondition() > misfireStartCondition)
        return 0.0f;

    if (GetCondition() < misfireEndCondition)
        return misfireEndProbability;

    float mis = misfireStartProbability + ((misfireStartCondition - GetCondition()) *             // condition goes from 1.f to 0.f
                                              (misfireEndProbability - misfireStartProbability) / // probability goes from 0.f to 1.f
                                              ((misfireStartCondition == misfireEndCondition) ?   // !!!say "No" to devision by zero
                                                      misfireStartCondition :
                                                      (misfireStartCondition - misfireEndCondition)));
    clamp(mis, 0.0f, 0.99f);
    return mis;
}