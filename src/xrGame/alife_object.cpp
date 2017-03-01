////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_object.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife object class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife.h"
#include "alife_simulator.h"
#include "xrServer_Objects_ALife_Items.h"

void CSE_ALifeObject::spawn_supplies() { spawn_supplies(*m_ini_string); }
void CSE_ALifeObject::spawn_supplies(LPCSTR ini_string)
{
    if (!ini_string)
        return;

    if (!xr_strlen(ini_string))
        return;

#pragma warning(push)
#pragma warning(disable : 4238)
    CInifile ini(&IReader((void*)(ini_string), xr_strlen(ini_string)), FS.get_path("$game_config$")->m_Path);
#pragma warning(pop)

    if (ini.section_exist("spawn"))
    {
        LPCSTR N, V;
        float p;
        for (u32 k = 0, j; ini.r_line("spawn", k, &N, &V); k++)
        {
            VERIFY(xr_strlen(N));

            float f_cond = 1.0f;
            bool bScope = false;
            bool bSilencer = false;
            bool bLauncher = false;
            bool bMagaz = false; //--#SM+#--
            bool bSpec_1 = false; //--#SM+#--
            bool bSpec_2 = false; //--#SM+#--
            bool bSpec_3 = false; //--#SM+#--
            bool bSpec_4 = false; //--#SM+#--

            j = 1;
            p = 1.f;

            if (V && xr_strlen(V))
            {
                string64 buf;
                j = atoi(_GetItem(V, 0, buf));
                if (!j)
                    j = 1;

                bScope = (NULL != strstr(V, "scope"));
                bSilencer = (NULL != strstr(V, "silencer"));
                bLauncher = (NULL != strstr(V, "launcher"));
                bMagaz = (NULL != strstr(V, "magaz")); //--#SM+#--
                bSpec_1 = (NULL != strstr(V, "special_1")); //--#SM+#--
                bSpec_2 = (NULL != strstr(V, "special_2")); //--#SM+#--
                bSpec_3 = (NULL != strstr(V, "special_3")); //--#SM+#--
                bSpec_4 = (NULL != strstr(V, "special_4")); //--#SM+#--

                // probability
                if (NULL != strstr(V, "prob="))
                    p = (float)atof(strstr(V, "prob=") + 5);
                if (fis_zero(p))
                    p = 1.0f;
                if (NULL != strstr(V, "cond="))
                    f_cond = (float)atof(strstr(V, "cond=") + 5);
            }
            for (u32 i = 0; i < j; ++i)
            {
                if (randF(1.f) < p)
                {
                    CSE_Abstract* E = alife().spawn_item(N, o_Position, m_tNodeID, m_tGraphID, ID);
                    //подсоединить аддоны к оружию, если включены соответствующие флажки
                    CSE_ALifeItemWeapon* W = smart_cast<CSE_ALifeItemWeapon*>(E);
                    if (W)
                    { //--#SM+#--
                      // SM_TODO: Возможность указывать конкретную секцию аддона в xml-е
                        if (W->m_scope_status == ALife::eAddonAttachable && bScope)
                            W->m_scope_idx = 0;
                        if (W->m_silencer_status == ALife::eAddonAttachable && bSilencer)
                            W->m_muzzle_idx = 0;
                        if (W->m_grenade_launcher_status == ALife::eAddonAttachable && bLauncher)
                            W->m_launcher_idx = 0;
                        if (W->m_magazine_status == ALife::eAddonAttachable && bMagaz)
                            W->m_magaz_idx = 0;
                        if (W->m_spec_1_status == ALife::eAddonAttachable && bSpec_1)
                            W->m_spec_1_idx = 0;
                        if (W->m_spec_2_status == ALife::eAddonAttachable && bSpec_2)
                            W->m_spec_2_idx = 0;
                        if (W->m_spec_3_status == ALife::eAddonAttachable && bSpec_3)
                            W->m_spec_3_idx = 0;
                        if (W->m_spec_4_status == ALife::eAddonAttachable && bSpec_4)
                            W->m_spec_4_idx = 0;

                        W->AddonsUpdate();
                    }
                    CSE_ALifeInventoryItem* IItem = smart_cast<CSE_ALifeInventoryItem*>(E);
                    if (IItem)
                        IItem->m_fCondition = f_cond;
                }
            }
        }
    }
}

bool CSE_ALifeObject::keep_saved_data_anyway() const { return (false); }
