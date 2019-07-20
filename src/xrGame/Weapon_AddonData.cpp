#include "stdafx.h"
#include "Weapon_AddonData.h"
#include "Common/object_broker.h"

/***********************************************/
/***** Структура, описывающая аддон оружия *****/ //--#SM+#--
/***********************************************/

SAddonData::SAddonData()
{
    bActive = false;
    bHideVis3p = false;
    addon_idx = empty_addon_idx;

    ResetWeaponKoefs();
}

SAddonData::~SAddonData() { delete_data(m_addons_list); }

/* Инициализировать аддон параметрами из конфига
   section      - секция оружия, которому принадлежит аддон;
   sAddonsList  - название строки из конфига оружия, где перечислены все set-секции адона данного типа;
   sAddonAlias  - название строки из конфига оружия, где прописывается имя предмета-аддона;
   sAttachAlias - название строки из конфига оружия, где прописывается статус возможности установки аддонов; 
   sAddonBone   - название кости в модели, отображение которой привязано к аддону;
*/
void SAddonData::Initialize(LPCSTR section, LPCSTR sAddonsList, LPCSTR sAddonAlias, LPCSTR sAttachAlias, LPCSTR sAddonBone)
{
    bActive = false;
    bHideVis3p = false;
    addon_idx = empty_addon_idx; 

    m_parent_sect = section;

    m_addon_alias = sAddonAlias;
    m_addon_bone = sAddonBone;
    m_attach_status = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_s32, section, sAttachAlias, 0);

    if (m_attach_status != ALife::eAddonDisabled)
    {
        if (pSettings->line_exist(section, sAddonsList))
        {
            LPCSTR str = pSettings->r_string(section, sAddonsList);
            if (xr_strcmp(str, "none") != 0)
            {
                for (int i = 0, count = _GetItemCount(str); i < count; ++i)
                {
                    string128 addon_set_section;
                    _GetItem(str, i, addon_set_section);
                    m_addons_list.push_back(addon_set_section);
                    R_ASSERT4(pSettings->line_exist(addon_set_section, m_addon_alias), section, addon_set_section,
                        m_addon_alias.c_str());
                }
            }
        }

        if (m_attach_status == ALife::eAddonPermanent)
        {
            R_ASSERT4(m_addons_list.size() > 0, section, m_addon_alias.c_str(),
                "attach status = 1 (permanent), but addons list is empty");
        }
    }
}

// Получить список всех визуалов (мировых или худовых) аддона (текущего или по его idx)
const shared_str SAddonData::GetVisuals(LPCSTR vis_alias, bool bOnlyFirst, u8 idx) const
{
    if (idx == empty_addon_idx)
        R_ASSERT(bActive == true);

    const shared_str& addon_set_sect = (idx == empty_addon_idx ? GetName() : m_addons_list[idx]);
    const shared_str& addon_name_sect =
        (idx == empty_addon_idx ? GetAddonName() : pSettings->r_string(addon_set_sect.c_str(), m_addon_alias.c_str()));

    shared_str sRes = READ_IF_EXISTS(pSettings, r_string, addon_set_sect, vis_alias,
        READ_IF_EXISTS(pSettings, r_string, addon_name_sect, vis_alias, "none"));

    if (sRes.equal("none"))
        sRes = NULL;

    if (bOnlyFirst == true && sRes != NULL)
    {
        string128 _itm_name;
        return _GetItem(sRes.c_str(), 0, _itm_name);
    }

    return sRes;
}

// Применить оружейные коэфиценты текущего аддона
void SAddonData::ApplyWeaponKoefs()
{
    ResetWeaponKoefs();

    if (bActive == false)
        return;

    m_kHitPower = READ_ADDON_DATA_NO_WPN(r_float, "addon_hit_power_k", GetName(), GetAddonName(), m_kHitPower);
    m_kImpulse = READ_ADDON_DATA_NO_WPN(r_float, "addon_impulse_k", GetName(), GetAddonName(), m_kImpulse);
    m_kBulletSpeed = READ_ADDON_DATA_NO_WPN(r_float, "addon_bullet_speed_k", GetName(), GetAddonName(), m_kBulletSpeed);
    m_kFireDispersion = READ_ADDON_DATA_NO_WPN(r_float, "addon_fire_disp_k", GetName(), GetAddonName(), m_kFireDispersion);
    m_kCamVDispersion = READ_ADDON_DATA_NO_WPN(r_float, "addon_cam_v_disp_k", GetName(), GetAddonName(), m_kCamVDispersion);
    m_kCamHDispersion = READ_ADDON_DATA_NO_WPN(r_float, "addon_cam_h_disp_k", GetName(), GetAddonName(), m_kCamHDispersion);
    m_kShootingShake = READ_ADDON_DATA_NO_WPN(r_float, "addon_shot_shake_k", GetName(), GetAddonName(), m_kShootingShake);
}

// Сбросить оружейные коэфиценты текущего аддона
void SAddonData::ResetWeaponKoefs()
{
    m_kHitPower = 1.0f;
    m_kImpulse = 1.0f;
    m_kBulletSpeed = 1.0f;
    m_kFireDispersion = 1.0f;
    m_kCamVDispersion = 1.0f;
    m_kCamHDispersion = 1.0f;
    m_kShootingShake = 1.0f;
}
