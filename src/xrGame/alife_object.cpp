////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_object.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife object class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife.h"
#include "alife_simulator.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrCommon/xr_vector.h"

void CSE_ALifeObject::spawn_supplies() { spawn_supplies(*m_ini_string); }

void CSE_ALifeObject::spawn_supplies(LPCSTR ini_string)
{
    if (!ini_string)
        return;

    if (!xr_strlen(ini_string))
        return;

#pragma warning(push)
#pragma warning(disable : 4238)
    IReader reader((void*)ini_string, xr_strlen(ini_string));
    CInifile ini(&reader, FS.get_path("$game_config$")->m_Path);
#pragma warning(pop)

    // Alundaio: This will spawn a single random section listed in [spawn_loadout]
    // No need to spawn ammo, this will automatically spawn 1 box for weapon and if ammo_type is specified it will spawn that type
    // count is used only for ammo boxes (ie wpn_pm = 3) will spawn 3 boxes, not 3 wpn_pm
    // Usage: to create random weapon loadouts
    if (ini.section_exist("spawn_loadout"))
    {
        pcstr itmSection, V;
        xr_vector<u32> OnlyOne;

        pcstr lname = *ai().game_graph().header().level(ai().game_graph().vertex(m_tGraphID)->level_id()).name();

        for (u32 k = 0; ini.r_line("spawn_loadout", k, &itmSection, &V); k++)
        {
            // If level=<lname> then only spawn items if object on that level
            if (strstr(V, "level=") != nullptr)
                if (strstr(V, lname) != nullptr)
                    OnlyOne.push_back(k);
            else
                OnlyOne.push_back(k);
        }

        if (!OnlyOne.empty())
        {
            s32 sel = Random.randI(0, OnlyOne.size());
            if (ini.r_line("spawn_loadout", OnlyOne.at(sel), &itmSection, &V))
            {
                VERIFY(xr_strlen(itmSection));
                if (pSettings->section_exist(itmSection))
                {
                    u32 spawn_count = 1;
                    bool bScope = false;
                    bool bSilencer = false;
                    bool bLauncher = false;
                    bool bMagaz = false; //--#SM+#--
                    bool bSpec_1 = false; //--#SM+#--
                    bool bSpec_2 = false; //--#SM+#--
                    bool bSpec_3 = false; //--#SM+#--
                    bool bSpec_4 = false; //--#SM+#--
                    bool bSpec_5 = false; //--#SM+#--
                    bool bSpec_6 = false; //--#SM+#--

                    float f_cond = 1.0f;
                    int i_ammo_type = 0, n = 0;

                    if (V && xr_strlen(V))
                    {
                        n = _GetItemCount(V);
                        if (n > 0)
                        {
                            string64 tmp;
                            spawn_count = atoi(_GetItem(V, 0, tmp)); //count
                        }

                        if (!spawn_count) spawn_count = 1;
                        if (nullptr != strstr(V, "cond="))
                            f_cond = static_cast<float>(atof(strstr(V, "cond=") + 5));
                        
                        bScope = nullptr != strstr(V, "scope");
                        bSilencer = nullptr != strstr(V, "silencer");
                        bLauncher = nullptr != strstr(V, "launcher");
                        bMagaz = (nullptr != strstr(V, "magaz")); //--#SM+#--
                        bSpec_1 = (nullptr != strstr(V, "special_1")); //--#SM+#--
                        bSpec_2 = (nullptr != strstr(V, "special_2")); //--#SM+#--
                        bSpec_3 = (nullptr != strstr(V, "special_3")); //--#SM+#--
                        bSpec_4 = (nullptr != strstr(V, "special_4")); //--#SM+#--
                        bSpec_5 = (nullptr != strstr(V, "special_5")); //--#SM+#--
                        bSpec_6 = (nullptr != strstr(V, "special_6")); //--#SM+#--

                        if (nullptr != strstr(V, "ammo_type="))
                            i_ammo_type = atoi(strstr(V, "ammo_type=") + 10);
                    }

                    CSE_Abstract* E = alife().spawn_item(itmSection, o_Position, m_tNodeID, m_tGraphID, ID);
                    CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);
                    if (W)
                    {
                        // SM_TODO:L ?????????????????????? ?????????????????? ???????????????????? ???????????? ???????????? ?? xml-??
                        Flags16 addon_flags;
                        addon_flags.zero();

                        if (W->m_scope_status == ALife::eAddonAttachable && bScope)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
                        if (W->m_silencer_status == ALife::eAddonAttachable && bSilencer)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
                        if (W->m_grenade_launcher_status == ALife::eAddonAttachable && bLauncher)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);
                        if (W->m_magazine_status == ALife::eAddonAttachable && bMagaz)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonMagazine, bMagaz);
                        if (W->m_spec_1_status == ALife::eAddonAttachable && bSpec_1)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_1, bSpec_1);
                        if (W->m_spec_2_status == ALife::eAddonAttachable && bSpec_2)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_2, bSpec_2);
                        if (W->m_spec_3_status == ALife::eAddonAttachable && bSpec_3)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_3, bSpec_3);
                        if (W->m_spec_4_status == ALife::eAddonAttachable && bSpec_4)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_4, bSpec_4);
                        if (W->m_spec_5_status == ALife::eAddonAttachable && bSpec_5)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_5, bSpec_5);
                        if (W->m_spec_6_status == ALife::eAddonAttachable && bSpec_6)
                            addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_6, bSpec_6);

                        W->SetAddonsState(addon_flags.get(), true);

                        //spawn count box(es) of the correct ammo for weapon
                        if (pSettings->line_exist(itmSection, "ammo_class"))
                        {
                            pcstr ammo_class = pSettings->r_string(itmSection, "ammo_class");
                            pcstr ammoSec = "";
                            for (int i = 0, n = _GetItemCount(ammo_class); i < n; ++i)
                            {
                                string128 tmp;
                                ammoSec = _GetItem(ammo_class, i, tmp);
                                if (i == i_ammo_type)
                                    break;
                            }
                            if (xr_strlen(ammoSec) && pSettings->section_exist(ammoSec))
                                for (u32 i = 1; i <= spawn_count; ++i)
                                    alife().spawn_item(ammoSec, o_Position, m_tNodeID, m_tGraphID, ID);
                        }
                    }
                    CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);
                    if (IItem)
                        IItem->m_fCondition = f_cond;
                }
            }
        }
    }
    //-Alundaio

    if (ini.section_exist("spawn"))
    {
        pcstr N, V;
        float p;
        for (u32 k = 0, j; ini.r_line("spawn", k, &N, &V); k++)
        {
            VERIFY(xr_strlen(N));

            if (pSettings->section_exist(N)) //Alundaio: verify item section exists!
            {
                float f_cond = 1.0f;
                bool bScope = false;
                bool bSilencer = false;
                bool bLauncher = false;
                bool bMagaz = false; //--#SM+#--
                bool bSpec_1 = false; //--#SM+#--
                bool bSpec_2 = false; //--#SM+#--
                bool bSpec_3 = false; //--#SM+#--
                bool bSpec_4 = false; //--#SM+#--
                bool bSpec_5 = false; //--#SM+#--
                bool bSpec_6 = false; //--#SM+#--

                j = 1;
                p = 1.f;

                if (V && xr_strlen(V))
                {
                    string64 buf;
                    j = atoi(_GetItem(V, 0, buf));
                    if (!j)
                        j = 1;

                    bScope = nullptr != strstr(V, "scope");
                    bSilencer = nullptr != strstr(V, "silencer");
                    bLauncher = nullptr != strstr(V, "launcher");
                    bMagaz = (nullptr != strstr(V, "magaz")); //--#SM+#--
                    bSpec_1 = (nullptr != strstr(V, "special_1")); //--#SM+#--
                    bSpec_2 = (nullptr != strstr(V, "special_2")); //--#SM+#--
                    bSpec_3 = (nullptr != strstr(V, "special_3")); //--#SM+#--
                    bSpec_4 = (nullptr != strstr(V, "special_4")); //--#SM+#--
                    bSpec_5 = (nullptr != strstr(V, "special_5")); //--#SM+#--
                    bSpec_6 = (nullptr != strstr(V, "special_6")); //--#SM+#--

                    // probability
                    if (nullptr != strstr(V, "prob="))
                        p = static_cast<float>(atof(strstr(V, "prob=") + 5));
                    if (fis_zero(p))
                        p = 1.0f;
                    if (nullptr != strstr(V, "cond="))
                        f_cond = static_cast<float>(atof(strstr(V, "cond=") + 5));
                }
                for (u32 i = 0; i < j; ++i)
                {
                    if (randF(1.f) < p)
                    {
                        CSE_Abstract* E = alife().spawn_item(N, o_Position, m_tNodeID, m_tGraphID, ID);
                        //???????????????????????? ???????????? ?? ????????????, ???????? ???????????????? ?????????????????????????????? ????????????
                        CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);
                        if (W) //--#SM+#--
                        {
                            // SM_TODO:L ?????????????????????? ?????????????????? ???????????????????? ???????????? ???????????? ?? xml-??
                            Flags16 addon_flags;
                            addon_flags.zero();
                      
                            if (W->m_scope_status == ALife::eAddonAttachable && bScope)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
                            if (W->m_silencer_status == ALife::eAddonAttachable && bSilencer)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
                            if (W->m_grenade_launcher_status == ALife::eAddonAttachable && bLauncher)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);
                            if (W->m_magazine_status == ALife::eAddonAttachable && bMagaz)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonMagazine, bMagaz);
                            if (W->m_spec_1_status == ALife::eAddonAttachable && bSpec_1)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_1, bSpec_1);
                            if (W->m_spec_2_status == ALife::eAddonAttachable && bSpec_2)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_2, bSpec_2);
                            if (W->m_spec_3_status == ALife::eAddonAttachable && bSpec_3)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_3, bSpec_3);
                            if (W->m_spec_4_status == ALife::eAddonAttachable && bSpec_4)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_4, bSpec_4);
                            if (W->m_spec_5_status == ALife::eAddonAttachable && bSpec_5)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_5, bSpec_5);
                            if (W->m_spec_6_status == ALife::eAddonAttachable && bSpec_6)
                                addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSpecial_6, bSpec_6);

                            W->SetAddonsState(addon_flags.get(), true);
                        }
                        CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);
                        if (IItem)
                            IItem->m_fCondition = f_cond;
                    }
                }
            }
        }
    }
}

bool CSE_ALifeObject::keep_saved_data_anyway() const /* noexcept */ { return false; }
