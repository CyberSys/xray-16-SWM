#include "pch_script.h"
#include "UIWpnParams.h"
#include "UIXmlInit.h"
#include "Level.h"
#include "game_base_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "inventory_item_object.h"
#include "UIInventoryUtilities.h"
#include "Weapon.h"

struct SLuaWpnParams
{
    luabind::functor<float> m_functorRPM;
    luabind::functor<float> m_functorAccuracy;
    luabind::functor<float> m_functorDamage;
    luabind::functor<float> m_functorDamageMP;
    luabind::functor<float> m_functorHandling;

    SLuaWpnParams();
    ~SLuaWpnParams();
};

static SLuaWpnParams* g_lua_wpn_params = nullptr;

SLuaWpnParams::SLuaWpnParams()
{
    bool functor_exists;
    functor_exists = GEnv.ScriptEngine->functor("ui_wpn_params.GetRPM", m_functorRPM);
    VERIFY(functor_exists);
    functor_exists = GEnv.ScriptEngine->functor("ui_wpn_params.GetDamage", m_functorDamage);
    VERIFY(functor_exists);
    functor_exists = GEnv.ScriptEngine->functor("ui_wpn_params.GetDamageMP", m_functorDamageMP);
    VERIFY(functor_exists);
    functor_exists = GEnv.ScriptEngine->functor("ui_wpn_params.GetHandling", m_functorHandling);
    VERIFY(functor_exists);
    functor_exists = GEnv.ScriptEngine->functor("ui_wpn_params.GetAccuracy", m_functorAccuracy);
    VERIFY(functor_exists);
}

SLuaWpnParams::~SLuaWpnParams() {}

// =====================================================================

CUIWpnParams::CUIWpnParams()
{
    AttachChild(&m_Prop_line);

    AttachChild(&m_icon_acc);
    AttachChild(&m_icon_dam);
    AttachChild(&m_icon_han);
    AttachChild(&m_icon_rpm);

    AttachChild(&m_textAccuracy);
    AttachChild(&m_textDamage);
    AttachChild(&m_textHandling);
    AttachChild(&m_textRPM);

    AttachChild(&m_progressAccuracy);
    AttachChild(&m_progressDamage);
    AttachChild(&m_progressHandling);
    AttachChild(&m_progressRPM);

    AttachChild(&m_stAmmo);
    AttachChild(&m_textAmmoCount);
    AttachChild(&m_textAmmoCount2);
    AttachChild(&m_textAmmoTypes);
    AttachChild(&m_textAmmoUsedType);
    // AttachChild(&m_stAmmoType1); //--#SM+#--
    // AttachChild(&m_stAmmoType2); //--#SM+#--
}

CUIWpnParams::~CUIWpnParams()
{
    delete_data(m_vecStAmmoTypes); //--#SM+#--
}

void CUIWpnParams::InitFromXml(CUIXml& xml_doc)
{
    if (!xml_doc.NavigateToNode("wpn_params", 0))
        return;
    CUIXmlInit::InitWindow(xml_doc, "wpn_params", 0, this);
    CUIXmlInit::InitStatic(xml_doc, "wpn_params:prop_line", 0, &m_Prop_line);

    CUIXmlInit::InitStatic(xml_doc, "wpn_params:static_accuracy", 0, &m_icon_acc);
    CUIXmlInit::InitStatic(xml_doc, "wpn_params:static_damage", 0, &m_icon_dam);
    CUIXmlInit::InitStatic(xml_doc, "wpn_params:static_handling", 0, &m_icon_han);
    CUIXmlInit::InitStatic(xml_doc, "wpn_params:static_rpm", 0, &m_icon_rpm);

    CUIXmlInit::InitTextWnd(xml_doc, "wpn_params:cap_accuracy", 0, &m_textAccuracy);
    CUIXmlInit::InitTextWnd(xml_doc, "wpn_params:cap_damage", 0, &m_textDamage);
    CUIXmlInit::InitTextWnd(xml_doc, "wpn_params:cap_handling", 0, &m_textHandling);
    CUIXmlInit::InitTextWnd(xml_doc, "wpn_params:cap_rpm", 0, &m_textRPM);

    m_progressAccuracy.InitFromXml(xml_doc, "wpn_params:progress_accuracy");
    m_progressDamage.InitFromXml(xml_doc, "wpn_params:progress_damage");
    m_progressHandling.InitFromXml(xml_doc, "wpn_params:progress_handling");
    m_progressRPM.InitFromXml(xml_doc, "wpn_params:progress_rpm");

    if (IsGameTypeSingle())
    {
        CUIXmlInit::InitStatic(xml_doc, "wpn_params:static_ammo", 0, &m_stAmmo);
        CUIXmlInit::InitTextWnd(xml_doc, "wpn_params:cap_ammo_count", 0, &m_textAmmoCount);
        CUIXmlInit::InitTextWnd(xml_doc, "wpn_params:cap_ammo_count2", 0, &m_textAmmoCount2);
        CUIXmlInit::InitTextWnd(xml_doc, "wpn_params:cap_ammo_types", 0, &m_textAmmoTypes);
        CUIXmlInit::InitTextWnd(xml_doc, "wpn_params:cap_ammo_used_type", 0, &m_textAmmoUsedType);
        // CUIXmlInit::InitStatic(xml_doc, "wpn_params:static_ammo_type1", 0, &m_stAmmoType1);  //--#SM+#--
        // CUIXmlInit::InitStatic(xml_doc, "wpn_params:static_ammo_type2", 0, &m_stAmmoType2);  //--#SM+#--

        // ?????????????? ???????????? ?????? ???????????? ??????-???? ?????????? ????????????????, ?????????????????? ?? XML --#SM+#--
        bool bAmmoTypeExistInXML = false;
        string128 str;
        u8 iCnt = 0;
        do //--> ?????????????????? ??????-???? ???????????? ?????? ???????????? ???????????????? ?? XML
        {
            iCnt++;
            xr_sprintf(str, sizeof(str), "wpn_params:static_ammo_type%d", iCnt);

            bAmmoTypeExistInXML = xml_doc.NavigateToNode(str, 0) != nullptr;
            if (bAmmoTypeExistInXML)
            {
                CUIStatic* pStAmmoType = new CUIStatic();
                AttachChild(pStAmmoType);
                CUIXmlInit::InitStatic(xml_doc, str, 0, pStAmmoType);

                m_vecStAmmoTypes.push_back(pStAmmoType);
            }
        } while (bAmmoTypeExistInXML);
    }
}

void CUIWpnParams::SetInfo(CInventoryItem* slot_wpn, CInventoryItem& cur_wpn)
{
    if (!g_lua_wpn_params)
    {
        class CResetEventCb : public CEventNotifierCallbackWithCid
        {
        public:
            CResetEventCb(CID cid) : CEventNotifierCallbackWithCid(cid) {}
            void ProcessEvent() override
            {
                xr_delete(g_lua_wpn_params);
                ai().Unsubscribe(GetCid(), CAI_Space::EVENT_SCRIPT_ENGINE_RESET);
            }
        };

        g_lua_wpn_params = new SLuaWpnParams();
        ai().template Subscribe<CResetEventCb>(CAI_Space::EVENT_SCRIPT_ENGINE_RESET);
    }

    LPCSTR cur_section = cur_wpn.object().cNameSect().c_str();
    string2048 str_upgrades;
    str_upgrades[0] = 0;
    cur_wpn.get_upgrades_str(str_upgrades);

    float cur_rpm = iFloor(g_lua_wpn_params->m_functorRPM(cur_section, str_upgrades) * 53.0f) / 53.0f;
    float cur_accur = iFloor(g_lua_wpn_params->m_functorAccuracy(cur_section, str_upgrades) * 53.0f) / 53.0f;
    float cur_hand = iFloor(g_lua_wpn_params->m_functorHandling(cur_section, str_upgrades) * 53.0f) / 53.0f;
    float cur_damage = (GameID() == eGameIDSingle) ?
        iFloor(g_lua_wpn_params->m_functorDamage(cur_section, str_upgrades) * 53.0f) / 53.0f :
        iFloor(g_lua_wpn_params->m_functorDamageMP(cur_section, str_upgrades) * 53.0f) / 53.0f;

    float slot_rpm = cur_rpm;
    float slot_accur = cur_accur;
    float slot_hand = cur_hand;
    float slot_damage = cur_damage;

    if (slot_wpn && (slot_wpn != &cur_wpn))
    {
        LPCSTR slot_section = slot_wpn->object().cNameSect().c_str();
        str_upgrades[0] = 0;
        slot_wpn->get_upgrades_str(str_upgrades);

        slot_rpm = iFloor(g_lua_wpn_params->m_functorRPM(slot_section, str_upgrades) * 53.0f) / 53.0f;
        slot_accur = iFloor(g_lua_wpn_params->m_functorAccuracy(slot_section, str_upgrades) * 53.0f) / 53.0f;
        slot_hand = iFloor(g_lua_wpn_params->m_functorHandling(slot_section, str_upgrades) * 53.0f) / 53.0f;
        slot_damage = (GameID() == eGameIDSingle) ?
            iFloor(g_lua_wpn_params->m_functorDamage(slot_section, str_upgrades) * 53.0f) / 53.0f :
            iFloor(g_lua_wpn_params->m_functorDamageMP(slot_section, str_upgrades) * 53.0f) / 53.0f;
    }

    m_progressAccuracy.SetTwoPos(cur_accur, slot_accur);
    m_progressDamage.SetTwoPos(cur_damage, slot_damage);
    m_progressHandling.SetTwoPos(cur_hand, slot_hand);
    m_progressRPM.SetTwoPos(cur_rpm, slot_rpm);

    if (IsGameTypeSingle())
    {
        xr_vector<shared_str> ammo_types;

        CWeapon* weapon = cur_wpn.cast_weapon();
        if (!weapon)
            return;

        int ammo_count = weapon->GetMainMagSize(); //--#SM+#--
        int ammo_count2 = ammo_count;

        if (slot_wpn)
        {
            CWeapon* slot_weapon = slot_wpn->cast_weapon();
            if (slot_weapon)
                ammo_count2 = slot_weapon->GetMainMagSize(); //--#SM+#--
        }

        if (ammo_count == ammo_count2)
            m_textAmmoCount2.SetTextColor(color_rgba(170, 170, 170, 255));
        else if (ammo_count < ammo_count2)
            m_textAmmoCount2.SetTextColor(color_rgba(255, 0, 0, 255));
        else
            m_textAmmoCount2.SetTextColor(color_rgba(0, 255, 0, 255));

        string128 str;
        xr_sprintf(str, sizeof(str), "%d", ammo_count);
        m_textAmmoCount2.SetText(str);

        ammo_types = *(weapon->GetMainAmmoTypes()); //--#SM+#--
        if (ammo_types.empty())
            return;

        xr_sprintf(str, sizeof(str), "%s", pSettings->r_string(ammo_types[0].c_str(), "inv_name_short"));
        m_textAmmoUsedType.SetTextST(str);

        // ?????????????? ???????????? ???????????????? --#SM+#--
        bool bShowDetailAmmoCntInMagaz = weapon->InventoryShowAllAmmoCntInMagazine(); //--> ???????????????????? ??????-???? ???????????????? ?????????????? ????????
        bool bShowAmmoIcons = weapon->InventoryShowWeaponAmmo(); //--> ???????????????????? ???????????? ????????????????
        for (u8 i = 0; i < m_vecStAmmoTypes.size(); i++)
        {
            CUIStatic& StAmmoType = *m_vecStAmmoTypes[i];

            StAmmoType.SetShader(InventoryUtilities::GetEquipmentIconsShader(INV_TEXTURE_DEF)); //--#SM+#--

            // ?????????????? ???????????????????? ?? ???????????? ???????? ???????????????? ?? ????????????
            cartridge_info* pCartridgeInfo = nullptr;
            if (bShowDetailAmmoCntInMagaz)
            {
                CartridgesInfoMap AmmoData = weapon->GetAmmoInfo(false);
                for (u8 j = 0; j < AmmoData.size(); j++)
                {
                    if (AmmoData[j].type_idx == i)
                    {
                        pCartridgeInfo = &AmmoData[j];
                        break;
                    }
                }
            }

            Frect tex_rect;
            string128 sAmmoCntText = "";
            bool bColorizeText = false;

            if (i >= ammo_types.size())
            { //--> ???? ?????????????????? ??????-???? ?????????? ???????????????? ?? ???????????????? ???????????? - ???????????????? ???????????? ????????????
                tex_rect.set(0, 0, 1, 1);
                bColorizeText = false;
            }
            else
            { //--> ?????????? ???????????????????????? ????
                tex_rect.x1 = float(pSettings->r_u32(ammo_types[i].c_str(), "inv_grid_x") * INV_GRID_WIDTH);
                tex_rect.y1 = float(pSettings->r_u32(ammo_types[i].c_str(), "inv_grid_y") * INV_GRID_HEIGHT);
                tex_rect.x2 = float(pSettings->r_u32(ammo_types[i].c_str(), "inv_grid_width") * INV_GRID_WIDTH);
                tex_rect.y2 = float(pSettings->r_u32(ammo_types[i].c_str(), "inv_grid_height") * INV_GRID_HEIGHT);
                tex_rect.rb.add(tex_rect.lt);

                xr_string sIconsTexture = READ_IF_EXISTS(
                    pSettings, r_string, ammo_types[i].c_str(), "inv_texture", INV_TEXTURE_DEF); //--#SM+#--
                StAmmoType.SetShader(InventoryUtilities::GetEquipmentIconsShader(sIconsTexture));

                // ?????????????? ?????????????? ???????????????? ?? ???????????? ?????? ???????????? ???????????? ????????????????
                if (bShowDetailAmmoCntInMagaz)
                {
                    u32 iAmmoCnt = 0;
                    if (pCartridgeInfo != nullptr)
                    {
                        iAmmoCnt = pCartridgeInfo->ammo_cnt;
                        bColorizeText = true;
                    }
                    xr_sprintf(sAmmoCntText, sizeof(sAmmoCntText), "%d", iAmmoCnt);
                }
            }

            StAmmoType.SetTextureRect(tex_rect);
            StAmmoType.TextureOn();
            StAmmoType.SetStretchTexture(true);
            StAmmoType.SetWndSize(
                Fvector2().set((tex_rect.x2 - tex_rect.x1) * UI().get_current_kx(), tex_rect.y2 - tex_rect.y1));

            if (bColorizeText)
            {
                bool bIsAmmoEnough = weapon->HaveMinRequiredAmmoInMag();
                StAmmoType.TextItemControl()->SetTextColor(
                    bIsAmmoEnough ? color_rgba(0, 255, 0, 255) : color_rgba(255, 0, 0, 255));
            }
            else
            {
                StAmmoType.TextItemControl()->SetTextColor(color_rgba(170, 170, 170, 255));
            }
            StAmmoType.TextItemControl()->SetTextST(sAmmoCntText);

            StAmmoType.Show(bShowAmmoIcons);
        }

        // ???????????????????????? ?????????????????? ?????????????????? ?????????????????????????? //--#SM+#--
        bool bShowStats = weapon->InventoryShowWeaponStats();

        m_icon_acc.Show(bShowStats);
        m_icon_dam.Show(bShowStats);
        m_icon_han.Show(bShowStats);
        m_icon_rpm.Show(bShowStats);

        m_textAccuracy.Show(bShowStats);
        m_textDamage.Show(bShowStats);
        m_textHandling.Show(bShowStats);
        m_textRPM.Show(bShowStats);

        m_progressAccuracy.Show(bShowStats);
        m_progressDamage.Show(bShowStats);
        m_progressHandling.Show(bShowStats);
        m_progressRPM.Show(bShowStats);

        m_stAmmo.Show(bShowStats);
        m_textAmmoCount.Show(bShowStats);
        m_textAmmoCount2.Show(bShowStats);
        m_textAmmoTypes.Show(bShowStats);
        m_textAmmoUsedType.Show(bShowStats);
    }
}

bool CUIWpnParams::Check(const shared_str& wpn_section)
{
    if (pSettings->line_exist(wpn_section, "fire_dispersion_base"))
    {
        if (0 == xr_strcmp(wpn_section, "wpn_addon_silencer"))
            return false;
        if (0 == xr_strcmp(wpn_section, "wpn_binoc"))
            return false;
        if (0 == xr_strcmp(wpn_section, "mp_wpn_binoc"))
            return false;

        return true;
    }
    return false;
}

// -------------------------------------------------------------------------------------------------

CUIConditionParams::CUIConditionParams()
{
    AttachChild(&m_progress);
    AttachChild(&m_text);
}

CUIConditionParams::~CUIConditionParams() {}
void CUIConditionParams::InitFromXml(CUIXml& xml_doc)
{
    if (!xml_doc.NavigateToNode("condition_params", 0))
        return;
    CUIXmlInit::InitWindow(xml_doc, "condition_params", 0, this);
    CUIXmlInit::InitStatic(xml_doc, "condition_params:caption", 0, &m_text);
    m_progress.InitFromXml(xml_doc, "condition_params:progress_state");
}

void CUIConditionParams::SetInfo(CInventoryItem const* slot_item, CInventoryItem const& cur_item)
{
    float cur_value = cur_item.GetConditionToShow() * 100.0f + 1.0f - EPS;
    float slot_value = cur_value;

    if (slot_item &&
        (slot_item !=
            &cur_item) /*&& (cur_item.object().cNameSect()._get() == slot_item->object().cNameSect()._get())*/)
    {
        slot_value = slot_item->GetConditionToShow() * 100.0f + 1.0f - EPS;
    }
    m_progress.SetTwoPos(cur_value, slot_value);
}
