#include "stdafx.h"
#include "Weapon_AddonData.h"
#include "Common/object_broker.h"
//--#SM_TODO+#--
SAddonData::SAddonData()
{
    bActive   = false;
    addon_idx = empty_addon_idx;
}

SAddonData::~SAddonData() { delete_data(m_addons_list); }

u32 SAddonData::AddonsCount() { return m_addons_list.size(); }

void SAddonData::Initialize(LPCSTR section, LPCSTR sAddonsList, LPCSTR sAddonAlias, LPCSTR sAttachAlias, LPCSTR sAddonBone) // TODO: В конструктор
{
    bActive     = false;
    addon_idx   = empty_addon_idx; // Это индекс аддона в таблице (его номер) а не тип аддона
    m_condition = 0.0f;
    m_battery   = 0.0f;
    //	m_pAmmo				=	NULL;
    m_addon_alias   = sAddonAlias;
    m_addon_bone    = sAddonBone;
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
                    string128 addon_section;
                    _GetItem(str, i, addon_section);
                    m_addons_list.push_back(addon_section);
                    R_ASSERT4(pSettings->line_exist(addon_section, m_addon_alias), section, addon_section, m_addon_alias.c_str());
                }
            }
        }

        if (m_attach_status == ALife::eAddonPermanent)
        {
            R_ASSERT4(m_addons_list.size() > 0, section, m_addon_alias.c_str(), "attach status = 1 (permanent), but addons list is empty");
        }
    }
}

const shared_str SAddonData::GetName() const
{
    /*
	Msg("GetName called");
	Msg("addon_idx = %d", addon_idx);
	Msg("m_addons_list.size = %d", m_addons_list.size());

	for (int i = 0; i < m_addons_list.size(); i++)
	{
		Msg("[%d] = %s", i, m_addons_list[i]);
	}
	*/

    return m_addons_list[addon_idx]; //TODO: !!!
                                     // ABORT() если bActive == false
}

const shared_str SAddonData::GetAddonName() const { return pSettings->r_string(GetName().c_str(), m_addon_alias.c_str()); }
//TODO: Все inline на ICF

const shared_str SAddonData::GetNameByIdx(u8 idx) const
{
    return m_addons_list[idx]; //TODO: !!!
                               // ABORT() если bActive == false
}

const shared_str SAddonData::GetAddonNameByIdx(u8 idx) const { return pSettings->r_string(GetNameByIdx(idx).c_str(), m_addon_alias.c_str()); }

const shared_str SAddonData::GetVisuals(LPCSTR vis_alias, bool bOnlyFirst, u8 idx) const
{
    if (idx == empty_addon_idx)
        R_ASSERT(bActive == true);

    const shared_str& addon_set_sect = (idx == empty_addon_idx ? GetName() : m_addons_list[idx]);
    const shared_str& addon_name_sect =
        (idx == empty_addon_idx ? GetAddonName() : pSettings->r_string(addon_set_sect.c_str(), m_addon_alias.c_str()));

    LPCSTR sRes =
        READ_IF_EXISTS(pSettings, r_string, addon_set_sect, vis_alias, READ_IF_EXISTS(pSettings, r_string, addon_name_sect, vis_alias, NULL));

    if (bOnlyFirst == true && sRes != NULL)
    {
        string128 _itm_name;
        return _GetItem(sRes, 0, _itm_name);
    }

    return sRes;
}