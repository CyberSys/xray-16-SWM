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
