/********************************/
/***** Разброс при стрельбе *****/ //--#SM+#--
/********************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

#include "effectorshot.h"
#include "EffectorShotX.h"

// Возвращает 1, если оружие в отличном состоянии и >1 если повреждено
float CWeapon::GetConditionDispersionFactor() const { return (1.f + fireDispersionConditionFactor * (1.f - GetCondition())); }

// Рассчитать базовый разброс самого оружия, используя коэфицент от патрона
float CWeapon::GetBaseDispersion(float cartridge_k)
{
    return fireDispersionBase * cur_silencer_koef.fire_dispersion * cartridge_k * GetConditionDispersionFactor();
}

// Получить текущий разброс
// with_cartridge	- использовать коэфицент разброса от патрона
// for_crosshair	- для отрисовки прицела
float CWeapon::GetFireDispersion(bool with_cartridge, bool for_crosshair)
{
    if (!with_cartridge)
        return GetFireDispersion(1.0f, for_crosshair);

    if (!m_magazine.empty())
        m_fCurrentCartirdgeDisp = m_magazine.back()->param_s.kDisp;

    return GetFireDispersion(m_fCurrentCartirdgeDisp, for_crosshair);
}

// Текущая дисперсия (в радианах) оружия с учетом используемого патрона
float CWeapon::GetFireDispersion(float cartridge_k, bool for_crosshair)
{
    // Учет базовой дисперсии, состояние оружия и влияение патрона
    float fire_disp = GetBaseDispersion(cartridge_k);

    // Учёт дисперсии от сошек
    if (IsBipodsDeployed())
        fire_disp *= (1.f * (1.f - m_bipods.m_translation_factor) + m_bipods.m_fDispersionMod * m_bipods.m_translation_factor);

    // Вычислить дисперсию, вносимую самим стрелком
    if (H_Parent())
    {
        const CInventoryOwner* pOwner      = smart_cast<const CInventoryOwner*>(H_Parent());
        float                  parent_disp = pOwner->GetWeaponAccuracy();
        fire_disp += parent_disp;
    }

    return fire_disp;
}
