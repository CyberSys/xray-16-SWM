#pragma once

//--#SM_TODO+#--

enum
{
    empty_addon_idx = u8(-1)
};
DEFINE_VECTOR(shared_str, ADDONS_VECTOR, ADDONS_VECTOR_IT); //--#SM+#-- moved

struct SAddonData
{
public:
    ADDONS_VECTOR             m_addons_list; //TODO: Переименовать
    ALife::EWeaponAddonStatus m_attach_status;
    shared_str                m_addon_alias;
    shared_str                m_addon_bone;
    bool                      bActive;
    bool                      bHideVis3p;
    u8                        addon_idx;
    float                     m_condition;
    float                     m_battery;

    //IAmmoDepot* m_pAmmo;

    SAddonData();
    ~SAddonData();

    u32 AddonsCount();

    void Initialize(LPCSTR section, LPCSTR sAddonsList, LPCSTR sAddonAlias, LPCSTR sAttachAlias, LPCSTR sAddonBone); // TODO: В конструктор

    const shared_str GetName() const;

    const shared_str GetAddonName() const;
    //TODO: Все inline на ICF

    const shared_str GetNameByIdx(u8 idx) const;

    const shared_str GetAddonNameByIdx(u8 idx) const;

    const shared_str GetVisuals(LPCSTR vis_alias, bool bOnlyFirst = false, u8 idx = empty_addon_idx) const;
};
