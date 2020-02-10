#include "StdAfx.h"
#include "UICellCustomItems.h"
#include "UIInventoryUtilities.h"
#include "Weapon.h"
#include "UIDragDropListEx.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"

#define INV_GRID_WIDTHF 50.0f
#define INV_GRID_HEIGHTF 50.0f
#define INV_BACKGR_ICON_NAME "__bgr_icon"  // Название CUIStatic иконки, которое используется для определения порядка отрисовки аддонов оружия --#SM+#--

namespace detail
{
struct is_helper_pred
{
    bool operator()(CUICellItem* child) { return child->IsHelper(); }
}; // struct is_helper_pred

} // namespace detail

CUIInventoryCellItem::CUIInventoryCellItem(CInventoryItem* itm)
{
    m_pData = (void*)itm;

    inherited::SetShader(InventoryUtilities::GetEquipmentIconsShader(itm->GetInvTexture())); //--#SM+#--

    m_grid_size.set(itm->GetInvGridRect().rb);
    Frect rect;
    rect.lt.set(INV_GRID_WIDTHF * itm->GetInvGridRect().x1, INV_GRID_HEIGHTF * itm->GetInvGridRect().y1);

    rect.rb.set(rect.lt.x + INV_GRID_WIDTHF * m_grid_size.x, rect.lt.y + INV_GRID_HEIGHTF * m_grid_size.y);

    inherited::SetTextureRect(rect);
    inherited::SetStretchTexture(true);
}

bool CUIInventoryCellItem::EqualTo(CUICellItem* itm)
{
    CUIInventoryCellItem* ci = smart_cast<CUIInventoryCellItem*>(itm);
    if (!itm)
    {
        return false;
    }
    if (object()->object().cNameSect() != ci->object()->object().cNameSect())
    {
        return false;
    }
    if (!fsimilar(object()->GetCondition(), ci->object()->GetCondition(), 0.01f))
    {
        return false;
    }
    if (!object()->equal_upgrades(ci->object()->upgardes()))
    {
        return false;
    }
    return true;
}

bool CUIInventoryCellItem::IsHelperOrHasHelperChild()
{
    return std::count_if(m_childs.begin(), m_childs.end(), detail::is_helper_pred()) > 0 || IsHelper();
}

CUIDragItem* CUIInventoryCellItem::CreateDragItem(bool bRotate) //--#SM+#--
{
    return IsHelperOrHasHelperChild() ? NULL : inherited::CreateDragItem(bRotate);
}

bool CUIInventoryCellItem::IsHelper() { return object()->is_helper_item(); }
void CUIInventoryCellItem::SetIsHelper(bool is_helper) { object()->set_is_helper(is_helper); }
void CUIInventoryCellItem::Update()
{
    inherited::Update();
    inherited::UpdateConditionProgressBar(); //Alundaio
    UpdateItemText();

    u32 color = GetTextureColor();
    if (IsHelper() && !ChildsCount())
    {
        color = 0xbbbbbbbb;
    }
    else if (IsHelperOrHasHelperChild())
    {
        color = 0xffffffff;
    }

    SetTextureColor(color);
}

void CUIInventoryCellItem::UpdateItemText()
{
    const u32 helper_count =
        (u32)std::count_if(m_childs.begin(), m_childs.end(), detail::is_helper_pred()) + IsHelper() ? 1 : 0;

    const u32 count = ChildsCount() + 1 - helper_count;

    string32 str;

    if (count > 1 || helper_count)
    {
        xr_sprintf(str, "x%d", count);
        m_text->TextItemControl()->SetText(str);
        m_text->Show(true);
    }
    else
    {
        xr_sprintf(str, "");
        m_text->TextItemControl()->SetText(str);
        m_text->Show(false);
    }
}

CUIAmmoCellItem::CUIAmmoCellItem(CWeaponAmmo* itm) : inherited(itm) {}
bool CUIAmmoCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIAmmoCellItem* ci = smart_cast<CUIAmmoCellItem*>(itm);
    if (!ci)
        return false;

    return ((object()->cNameSect() == ci->object()->cNameSect()));
}

CUIDragItem* CUIAmmoCellItem::CreateDragItem(bool bRotate) { return IsHelper() ? NULL : inherited::CreateDragItem(bRotate); } //--#SM+#--
u32 CUIAmmoCellItem::CalculateAmmoCount()
{
    xr_vector<CUICellItem*>::iterator it = m_childs.begin();
    xr_vector<CUICellItem*>::iterator it_e = m_childs.end();

    u32 total = IsHelper() ? 0 : object()->m_boxCurr;
    for (; it != it_e; ++it)
    {
        CUICellItem* child = *it;

        if (!child->IsHelper())
        {
            total += ((CUIAmmoCellItem*)(*it))->object()->m_boxCurr;
        }
    }

    return total;
}

void CUIAmmoCellItem::UpdateItemText()
{
    m_text->Show(false);
    if (!m_custom_draw)
    {
        const u32 total = CalculateAmmoCount();

        string32 str;
        xr_sprintf(str, "%d", total);
        m_text->TextItemControl()->SetText(str);
        m_text->Show(true);
    }
}

CUIWeaponCellItem::CUIWeaponCellItem(CWeapon* itm) : inherited(itm) //--#SM+#--
{
    m_addons[eDefaultStock] = NULL; 
    m_addons[eDefaultScope] = NULL; 
    m_addons[eSilencer] = NULL;
    m_addons[eScope] = NULL;
    m_addons[eLauncher] = NULL;
    m_addons[eMagazine] = NULL;
    m_addons[eSpecial_1] = NULL;
    m_addons[eSpecial_2] = NULL;
    m_addons[eSpecial_3] = NULL;
    m_addons[eSpecial_4] = NULL; 
    m_addons[eSpecial_5] = NULL;
    m_addons[eSpecial_6] = NULL; 

    if (itm->IsDefaultStockPresent() && itm->IsStockAttached() == false)
        m_addon_offset[eDefaultStock].set(object()->GetDefaultStockX(), object()->GetDefaultStockY());

    if (itm->IsDefaultScopePresent() && itm->CanShowDefaultScope())
        m_addon_offset[eDefaultScope].set(object()->GetDefaultScopeX(), object()->GetDefaultScopeY());

	if (itm->IsSilencerAttached())
        m_addon_offset[eSilencer].set(object()->GetSilencerX(), object()->GetSilencerY());

    if (itm->IsScopeAttached())
        m_addon_offset[eScope].set(object()->GetScopeX(), object()->GetScopeY());

    if (itm->IsGrenadeLauncherAttached())
        m_addon_offset[eLauncher].set(object()->GetGrenadeLauncherX(), object()->GetGrenadeLauncherY());

    if (itm->IsMagazineAttached())
        m_addon_offset[eMagazine].set(object()->GetMagazineX(), object()->GetMagazineY());

    if (itm->IsSpecial_1_Attached())
        m_addon_offset[eSpecial_1].set(object()->GetSpecial_1_X(), object()->GetSpecial_1_Y());

    if (itm->IsSpecial_2_Attached())
        m_addon_offset[eSpecial_2].set(object()->GetSpecial_2_X(), object()->GetSpecial_2_Y());

    if (itm->IsSpecial_3_Attached())
        m_addon_offset[eSpecial_3].set(object()->GetSpecial_3_X(), object()->GetSpecial_3_Y());

    if (itm->IsSpecial_4_Attached())
        m_addon_offset[eSpecial_4].set(object()->GetSpecial_4_X(), object()->GetSpecial_4_Y());

    if (itm->IsSpecial_5_Attached())
        m_addon_offset[eSpecial_5].set(object()->GetSpecial_5_X(), object()->GetSpecial_5_Y());

    if (itm->IsSpecial_6_Attached())
        m_addon_offset[eSpecial_6].set(object()->GetSpecial_6_X(), object()->GetSpecial_6_Y());
}

#include "Common/object_broker.h"
CUIWeaponCellItem::~CUIWeaponCellItem() {}
bool CUIWeaponCellItem::is_scope() { return object()->ScopeAttachable() && object()->IsScopeAttached(); }
bool CUIWeaponCellItem::is_silencer() { return object()->SilencerAttachable() && object()->IsSilencerAttached(); }
bool CUIWeaponCellItem::is_launcher()
{
    return object()->GrenadeLauncherAttachable() && object()->IsGrenadeLauncherAttached();
}
bool CUIWeaponCellItem::is_magazine() //--#SM+#--
{
    return object()->MagazineAttachable() && object()->IsMagazineAttached();
}

bool CUIWeaponCellItem::is_special_1() //--#SM+#--
{
    return object()->Special_1_Attachable() && object()->IsSpecial_1_Attached();
}

bool CUIWeaponCellItem::is_special_2() //--#SM+#--
{
    return object()->Special_2_Attachable() && object()->IsSpecial_2_Attached();
}

bool CUIWeaponCellItem::is_special_3() //--#SM+#--
{
    return object()->Special_3_Attachable() && object()->IsSpecial_3_Attached();
}

bool CUIWeaponCellItem::is_special_4() //--#SM+#--
{
    return object()->Special_4_Attachable() && object()->IsSpecial_4_Attached();
}

bool CUIWeaponCellItem::is_special_5() //--#SM+#--
{
    return object()->Special_5_Attachable() && object()->IsSpecial_5_Attached();
}

bool CUIWeaponCellItem::is_special_6() //--#SM+#--
{
    return object()->Special_6_Attachable() && object()->IsSpecial_6_Attached();
}

void CUIWeaponCellItem::CreateIcon(eAddonType t, const shared_str& sAddonName) //--#SM+#--
{
    if (m_addons[t])
        return;

    m_addons[t] = new CUIStatic();
    m_addons[t]->SetAutoDelete(true);
    AttachChild(m_addons[t]);
    m_addons[t]->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--

    // Регулируем порядок отрисовки иконок аддонов --#SM+#--
    bool bIconToBackground = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "inv_icon_to_back", false);
    if (bIconToBackground)
    {
        m_addons[t]->SetWindowName(INV_BACKGR_ICON_NAME);
    }

    u32 color = GetTextureColor();
    m_addons[t]->SetTextureColor(color);
}

void CUIWeaponCellItem::DestroyIcon(eAddonType t)
{
    DetachChild(m_addons[t]);
    m_addons[t] = NULL;
}

CUIStatic* CUIWeaponCellItem::GetIcon(eAddonType t) { return m_addons[t]; }
void CUIWeaponCellItem::RefreshOffset() //--#SM+#--
{
    if (object()->IsDefaultStockPresent() && object()->IsStockAttached() == false)
        m_addon_offset[eDefaultStock].set(object()->GetDefaultStockX(), object()->GetDefaultStockY());

    if (object()->IsDefaultScopePresent() && object()->CanShowDefaultScope())
        m_addon_offset[eDefaultScope].set(object()->GetDefaultScopeX(), object()->GetDefaultScopeY());

    if (object()->IsSilencerAttached()) 
        m_addon_offset[eSilencer].set(object()->GetSilencerX(), object()->GetSilencerY());

    if (object()->IsScopeAttached())
        m_addon_offset[eScope].set(object()->GetScopeX(), object()->GetScopeY());

    if (object()->IsGrenadeLauncherAttached())
        m_addon_offset[eLauncher].set(object()->GetGrenadeLauncherX(), object()->GetGrenadeLauncherY());

    if (object()->IsMagazineAttached())
        m_addon_offset[eMagazine].set(object()->GetMagazineX(), object()->GetMagazineY());

    if (object()->IsSpecial_1_Attached())
        m_addon_offset[eSpecial_1].set(object()->GetSpecial_1_X(), object()->GetSpecial_1_Y());

    if (object()->IsSpecial_2_Attached())
        m_addon_offset[eSpecial_2].set(object()->GetSpecial_2_X(), object()->GetSpecial_2_Y());

    if (object()->IsSpecial_3_Attached())
        m_addon_offset[eSpecial_3].set(object()->GetSpecial_3_X(), object()->GetSpecial_3_Y());

    if (object()->IsSpecial_4_Attached())
        m_addon_offset[eSpecial_4].set(object()->GetSpecial_4_X(), object()->GetSpecial_4_Y());

    if (object()->IsSpecial_5_Attached())
        m_addon_offset[eSpecial_5].set(object()->GetSpecial_5_X(), object()->GetSpecial_5_Y());

    if (object()->IsSpecial_6_Attached())
        m_addon_offset[eSpecial_6].set(object()->GetSpecial_6_X(), object()->GetSpecial_6_Y());
}

void CUIWeaponCellItem::Draw() //--#SM+#--
{
    // Рисуем только аддоны заднего плана
    bool bBackgrIconsFound = false;
    for (auto it = m_ChildWndList.begin(); m_ChildWndList.end() != it; ++it)
    {
        CUIStatic* pStatic = (CUIStatic*)(*it);
        if (pStatic->WindowName().equal(INV_BACKGR_ICON_NAME))
        {
            bBackgrIconsFound = true;
            pStatic->TextureOn(); //--> Включаем текстуру у аддонов заднего плана
        }
        else
        {
            pStatic->TextureOff(); //--> Отключаем текстуру у аддонов переднего плана
        }
    }
    if (bBackgrIconsFound == true)
    {
        TextureOff(); //--> Отключаем текстуру оружия
        inherited::Draw(); //--> Рисуем иконки заднего плана
        TextureOn(); //--> Включаем текстуру оружия
    }

    // Рисуем только оружие и аддоны переднего плана
    for (auto it = m_ChildWndList.begin(); m_ChildWndList.end() != it; ++it)
    {
        CUIStatic* pStatic = (CUIStatic*)(*it);
        if (pStatic->WindowName().equal(INV_BACKGR_ICON_NAME))
        {
            pStatic->TextureOff(); //--> Отключаем текстуру у аддонов заднего плана
        }
        else
        {
            pStatic->TextureOn(); //--> Включаем текстуру у аддонов переднего плана
        }
    }
    inherited::Draw(); //--> Рисуем оружие и иконки переднего плана

    if (m_upgrade && m_upgrade->IsShown())
        m_upgrade->Draw();
};

void CUIWeaponCellItem::Update()
{
    bool b = Heading();
    inherited::Update();

    bool bForceReInitAddons = (b != Heading()) || object()->Need2UpdateIcon(); //--#SM+#--

    if (object()->IsDefaultStockPresent() && object()->IsStockAttached() == false)
    { //--#SM+#--
        if (!GetIcon(eDefaultStock) || bForceReInitAddons)
        {
            CreateIcon(eDefaultStock, object()->GetDefaultStockSect()); //--#SM+#--
            RefreshOffset();
            InitAddon(GetIcon(eDefaultStock), *object()->GetDefaultStockSect(), m_addon_offset[eDefaultStock], Heading());
        }
    }
    else
    {
        if (m_addons[eDefaultStock])
            DestroyIcon(eDefaultStock);
    }

    if (object()->IsDefaultScopePresent() && object()->CanShowDefaultScope())
    { //--#SM+#--
        if (!GetIcon(eDefaultScope) || bForceReInitAddons)
        {
            CreateIcon(eDefaultScope, object()->GetDefaultScopeSect()); //--#SM+#--
            RefreshOffset();
            InitAddon(GetIcon(eDefaultScope), *object()->GetDefaultScopeSect(), m_addon_offset[eDefaultScope], Heading());
        }
    }
    else
    {
        if (m_addons[eDefaultScope])
            DestroyIcon(eDefaultScope);
    }

    if (object()->SilencerAttachable())
    {
        if (object()->IsSilencerAttached())
        {
            if (!GetIcon(eSilencer) || bForceReInitAddons)
            {
                CreateIcon(eSilencer, object()->GetSilencerName()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eSilencer), *object()->GetSilencerName(), m_addon_offset[eSilencer], Heading());
            }
        }
        else
        {
            if (m_addons[eSilencer])
                DestroyIcon(eSilencer);
        }
    }

    if (object()->ScopeAttachable())
    {
        if (object()->IsScopeAttached())
        {
            if (!GetIcon(eScope) || bForceReInitAddons)
            {
                CreateIcon(eScope, object()->GetScopeName()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eScope), *object()->GetScopeName(), m_addon_offset[eScope], Heading());
            }
        }
        else
        {
            if (m_addons[eScope])
                DestroyIcon(eScope);
        }
    }

    if (object()->GrenadeLauncherAttachable())
    {
        if (object()->IsGrenadeLauncherAttached())
        {
            if (!GetIcon(eLauncher) || bForceReInitAddons)
            {
                CreateIcon(eLauncher, object()->GetGrenadeLauncherName()); //--#SM+#--
                RefreshOffset();
                InitAddon(
                    GetIcon(eLauncher), *object()->GetGrenadeLauncherName(), m_addon_offset[eLauncher], Heading());
            }
        }
        else
        {
            if (m_addons[eLauncher])
                DestroyIcon(eLauncher);
        }
    }

	if (object()->MagazineAttachable())
    { //--#SM+#--
        if (object()->IsMagazineAttached())
        {
            if (!GetIcon(eMagazine) || bForceReInitAddons || object()->GetState() == CWeapon::eSwitchMag)
            {
                CreateIcon(eMagazine, object()->GetMagazineName()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eMagazine), *object()->GetMagazineName(), m_addon_offset[eMagazine], Heading());
            }
        }
        else
        {
            if (m_addons[eMagazine])
                DestroyIcon(eMagazine);
        }
    }

    if (object()->Special_1_Attachable())
    { //--#SM+#--
        if (object()->IsSpecial_1_Attached())
        {
            if (!GetIcon(eSpecial_1) || bForceReInitAddons)
            {
                CreateIcon(eSpecial_1, object()->GetSpecial_1_Name()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eSpecial_1), *object()->GetSpecial_1_Name(), m_addon_offset[eSpecial_1], Heading());
            }
        }
        else
        {
            if (m_addons[eSpecial_1])
                DestroyIcon(eSpecial_1);
        }
    }

    if (object()->Special_2_Attachable())
    { //--#SM+#--
        if (object()->IsSpecial_2_Attached())
        {
            if (!GetIcon(eSpecial_2) || bForceReInitAddons)
            {
                CreateIcon(eSpecial_2, object()->GetSpecial_2_Name()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eSpecial_2), *object()->GetSpecial_2_Name(), m_addon_offset[eSpecial_2], Heading());
            }
        }
        else
        {
            if (m_addons[eSpecial_2])
                DestroyIcon(eSpecial_2);
        }
    }

    if (object()->Special_3_Attachable())
    { //--#SM+#--
        if (object()->IsSpecial_3_Attached())
        {
            if (!GetIcon(eSpecial_3) || bForceReInitAddons)
            {
                CreateIcon(eSpecial_3, object()->GetSpecial_3_Name()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eSpecial_3), *object()->GetSpecial_3_Name(), m_addon_offset[eSpecial_3], Heading());
            }
        }
        else
        {
            if (m_addons[eSpecial_3])
                DestroyIcon(eSpecial_3);
        }
    }

    if (object()->Special_4_Attachable())
    { //--#SM+#--
        if (object()->IsSpecial_4_Attached())
        {
            if (!GetIcon(eSpecial_4) || bForceReInitAddons)
            {
                CreateIcon(eSpecial_4, object()->GetSpecial_4_Name()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eSpecial_4), *object()->GetSpecial_4_Name(), m_addon_offset[eSpecial_4], Heading());
            }
        }
        else
        {
            if (m_addons[eSpecial_4])
                DestroyIcon(eSpecial_4);
        }
    }

    if (object()->Special_5_Attachable())
    { //--#SM+#--
        if (object()->IsSpecial_5_Attached())
        {
            if (!GetIcon(eSpecial_5) || bForceReInitAddons)
            {
                CreateIcon(eSpecial_5, object()->GetSpecial_5_Name()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eSpecial_5), *object()->GetSpecial_5_Name(), m_addon_offset[eSpecial_5], Heading());
            }
        }
        else
        {
            if (m_addons[eSpecial_5])
                DestroyIcon(eSpecial_5);
        }
    }

    if (object()->Special_6_Attachable())
    { //--#SM+#--
        if (object()->IsSpecial_6_Attached())
        {
            if (!GetIcon(eSpecial_6) || bForceReInitAddons)
            {
                CreateIcon(eSpecial_6, object()->GetSpecial_6_Name()); //--#SM+#--
                RefreshOffset();
                InitAddon(GetIcon(eSpecial_6), *object()->GetSpecial_6_Name(), m_addon_offset[eSpecial_6], Heading());
            }
        }
        else
        {
            if (m_addons[eSpecial_6])
                DestroyIcon(eSpecial_6);
        }
    }

    object()->SetUpdateIcon(false); //--#SM+#--
}

void CUIWeaponCellItem::SetTextureColor(u32 color)
{
    inherited::SetTextureColor(color);
    if (m_addons[eDefaultStock]) //--#SM+#--
    {
        m_addons[eDefaultStock]->SetTextureColor(color);
    }
    if (m_addons[eDefaultScope]) //--#SM+#--
    {
        m_addons[eDefaultScope]->SetTextureColor(color);
    }
    if (m_addons[eSilencer])
    {
        m_addons[eSilencer]->SetTextureColor(color);
    }
    if (m_addons[eScope])
    {
        m_addons[eScope]->SetTextureColor(color);
    }
    if (m_addons[eLauncher])
    {
        m_addons[eLauncher]->SetTextureColor(color);
    }
    if (m_addons[eMagazine]) //--#SM+#--
    {
        m_addons[eMagazine]->SetTextureColor(color);
    }
    if (m_addons[eSpecial_1]) //--#SM+#--
    {
        m_addons[eSpecial_1]->SetTextureColor(color);
    }
    if (m_addons[eSpecial_2]) //--#SM+#--
    {
        m_addons[eSpecial_2]->SetTextureColor(color);
    }
    if (m_addons[eSpecial_3]) //--#SM+#--
    {
        m_addons[eSpecial_3]->SetTextureColor(color);
    }
    if (m_addons[eSpecial_4]) //--#SM+#--
    {
        m_addons[eSpecial_4]->SetTextureColor(color);
    }
    if (m_addons[eSpecial_5]) //--#SM+#--
    {
        m_addons[eSpecial_5]->SetTextureColor(color);
    }
    if (m_addons[eSpecial_6]) //--#SM+#--
    {
        m_addons[eSpecial_6]->SetTextureColor(color);
    }
}

void CUIWeaponCellItem::OnAfterChild(CUIDragDropListEx* parent_list)
{
    if (object()->IsDefaultStockPresent() && object()->IsStockAttached() == false && GetIcon(eDefaultStock)) //--#SM+#--
        InitAddon(GetIcon(eDefaultStock), *object()->GetDefaultStockSect(), m_addon_offset[eDefaultStock],
            parent_list->GetVerticalPlacement());

    if (object()->IsDefaultScopePresent() && object()->CanShowDefaultScope() && GetIcon(eDefaultScope)) //--#SM+#--
        InitAddon(GetIcon(eDefaultScope), *object()->GetDefaultScopeSect(), m_addon_offset[eDefaultScope],
            parent_list->GetVerticalPlacement());

    if (is_silencer() && GetIcon(eSilencer))
        InitAddon(GetIcon(eSilencer), *object()->GetSilencerName(), m_addon_offset[eSilencer],
            parent_list->GetVerticalPlacement());

    if (is_scope() && GetIcon(eScope))
        InitAddon(
            GetIcon(eScope), *object()->GetScopeName(), m_addon_offset[eScope], parent_list->GetVerticalPlacement());

    if (is_launcher() && GetIcon(eLauncher))
        InitAddon(GetIcon(eLauncher), *object()->GetGrenadeLauncherName(), m_addon_offset[eLauncher],
            parent_list->GetVerticalPlacement());

	if (is_magazine() && GetIcon(eMagazine)) //--#SM+#--
        InitAddon(GetIcon(eMagazine), *object()->GetMagazineName(), m_addon_offset[eMagazine],
            parent_list->GetVerticalPlacement());

    if (is_special_1() && GetIcon(eSpecial_1)) //--#SM+#--
        InitAddon(GetIcon(eSpecial_1), *object()->GetSpecial_1_Name(), m_addon_offset[eSpecial_1],
            parent_list->GetVerticalPlacement());

    if (is_special_2() && GetIcon(eSpecial_2)) //--#SM+#--
        InitAddon(GetIcon(eSpecial_2), *object()->GetSpecial_2_Name(), m_addon_offset[eSpecial_2],
            parent_list->GetVerticalPlacement());

    if (is_special_3() && GetIcon(eSpecial_3)) //--#SM+#--
        InitAddon(GetIcon(eSpecial_3), *object()->GetSpecial_3_Name(), m_addon_offset[eSpecial_3],
            parent_list->GetVerticalPlacement());

    if (is_special_4() && GetIcon(eSpecial_4)) //--#SM+#--
        InitAddon(GetIcon(eSpecial_4), *object()->GetSpecial_4_Name(), m_addon_offset[eSpecial_4],
            parent_list->GetVerticalPlacement());

    if (is_special_5() && GetIcon(eSpecial_5)) //--#SM+#--
        InitAddon(GetIcon(eSpecial_5), *object()->GetSpecial_5_Name(), m_addon_offset[eSpecial_5],
            parent_list->GetVerticalPlacement());

    if (is_special_6() && GetIcon(eSpecial_6)) //--#SM+#--
        InitAddon(GetIcon(eSpecial_6), *object()->GetSpecial_6_Name(), m_addon_offset[eSpecial_6],
            parent_list->GetVerticalPlacement());
}

void CUIWeaponCellItem::InitAddon(CUIStatic* s, LPCSTR section, Fvector2 addon_offset, bool b_rotate)
{
    Frect tex_rect;
    Fvector2 base_scale;

    if (Heading())
    {
        base_scale.x = GetHeight() / (INV_GRID_WIDTHF * m_grid_size.x);
        base_scale.y = GetWidth() / (INV_GRID_HEIGHTF * m_grid_size.y);
    }
    else
    {
        base_scale.x = GetWidth() / (INV_GRID_WIDTHF * m_grid_size.x);
        base_scale.y = GetHeight() / (INV_GRID_HEIGHTF * m_grid_size.y);
    }

    Fvector2 cell_size;

    cell_size.x = READ_IF_EXISTS(pSettings, r_u32, section, "inv_grid_inst_width",
        READ_IF_EXISTS(pSettings, r_u32, section, "inv_grid_width", 0)); //--#SM+#--
    cell_size.x *= INV_GRID_WIDTHF;

    cell_size.y = READ_IF_EXISTS(pSettings, r_u32, section, "inv_grid_inst_height",
        READ_IF_EXISTS(pSettings, r_u32, section, "inv_grid_height", 0)); //--#SM+#--
    cell_size.y *= INV_GRID_HEIGHTF;

    tex_rect.x1 = READ_IF_EXISTS(pSettings, r_u32, section, "inv_grid_inst_x",
        READ_IF_EXISTS(pSettings, r_u32, section, "inv_grid_x", 0)); //--#SM+#--
    tex_rect.x1 *= INV_GRID_WIDTHF;

    tex_rect.y1 = READ_IF_EXISTS(pSettings, r_u32, section, "inv_grid_inst_y",
        READ_IF_EXISTS(pSettings, r_u32, section, "inv_grid_y", 0)); //--#SM+#--
    tex_rect.y1 *= INV_GRID_HEIGHTF;

    tex_rect.rb.add(tex_rect.lt, cell_size);

    cell_size.mul(base_scale);

    if (b_rotate)
    {
        s->SetWndSize(Fvector2().set(cell_size.y, cell_size.x));
        Fvector2 new_offset;
        new_offset.x = addon_offset.y * base_scale.x;
        new_offset.y = GetHeight() - addon_offset.x * base_scale.x - cell_size.x;
        addon_offset = new_offset;
        addon_offset.x *= UI().get_current_kx();
    }
    else
    {
        s->SetWndSize(cell_size);
        addon_offset.mul(base_scale);
    }

    s->SetWndPos(addon_offset);
    s->SetTextureRect(tex_rect);
    s->SetStretchTexture(true);

    s->EnableHeading(b_rotate);

    if (b_rotate)
    {
        s->SetHeading(GetHeading());
        Fvector2 offs;
        offs.set(0.0f, s->GetWndSize().y);
        s->SetHeadingPivot(Fvector2().set(0.0f, 0.0f), /*Fvector2().set(0.0f,0.0f)*/ offs, true);
    }
}

CUIDragItem* CUIWeaponCellItem::CreateDragItem(bool bRotate) //--#SM+#--
{
    CUIDragItem* i = inherited::CreateDragItem(bRotate); //--#SM+#--
    CUIStatic* s = NULL;

    if (bRotate) //--#SM+#--
    {
        i->wnd()->EnableHeading(true);
        i->wnd()->SetHeading(90.0f * (PI / 180.0f));
        i->wnd()->SetHeadingPivot(Fvector2().set(0.0f, 0.0f), Fvector2().set(0.0f, i->wnd()->GetWndSize().y), true);
    }

    if (GetIcon(eDefaultStock)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetDefaultStockSect(), m_addon_offset[eDefaultStock], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eDefaultScope)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetDefaultScopeSect(), m_addon_offset[eDefaultScope], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eSilencer))
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetSilencerName(), m_addon_offset[eSilencer], bRotate); //--#SM+#--
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eScope))
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetScopeName(), m_addon_offset[eScope], bRotate); //--#SM+#--
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eLauncher))
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetGrenadeLauncherName(), m_addon_offset[eLauncher], bRotate); //--#SM+#--
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

	if (GetIcon(eMagazine)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetMagazineName(), m_addon_offset[eMagazine], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eSpecial_1)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetSpecial_1_Name(), m_addon_offset[eSpecial_1], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eSpecial_2)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetSpecial_2_Name(), m_addon_offset[eSpecial_2], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eSpecial_3)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetSpecial_3_Name(), m_addon_offset[eSpecial_3], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eSpecial_4)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetSpecial_4_Name(), m_addon_offset[eSpecial_4], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eSpecial_5)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetSpecial_5_Name(), m_addon_offset[eSpecial_5], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eSpecial_6)) //--#SM+#--
    {
        s = new CUIStatic();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader(object()->GetInvTexture())); //--#SM+#--
        InitAddon(s, *object()->GetSpecial_6_Name(), m_addon_offset[eSpecial_6], bRotate);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    return i;
}

bool CUIWeaponCellItem::EqualTo(CUICellItem* itm) //--#SM+#--
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIWeaponCellItem* ci = smart_cast<CUIWeaponCellItem*>(itm);
    if (!ci)
        return false;

	CWeapon* pWpn = object()->cast_weapon();
    CWeapon* pWpn2 = ci->object()->cast_weapon();

    if (pWpn != nullptr && pWpn2 != nullptr)
        return pWpn->InventoryEqualTo(pWpn2);

    return true;
}

// Индикатор кол-ва одинаковых предметов, что выводится у иконок в инвентаре
void CUIWeaponCellItem::UpdateItemText() //--#SM+#--
{
    inherited::UpdateItemText();
}

bool CUIWeaponCellItem::GetUtilityBarValue(float& fRetVal) //--#SM+#--
{
    // Для оружейных магазинов рисуем полоску кол-ва оставшихся патронов
    CWeapon* pWpn = object()->cast_weapon();
    if (pWpn && pWpn->IsMagazine())
    {
        fRetVal = (float)pWpn->GetMainAmmoElapsed() / (float)pWpn->GetMainMagSize();
        return true;
    }

    return false;
}

CBuyItemCustomDrawCell::CBuyItemCustomDrawCell(LPCSTR str, CGameFont* pFont)
{
    m_pFont = pFont;
    VERIFY(xr_strlen(str) < 16);
    xr_strcpy(m_string, str);
}

void CBuyItemCustomDrawCell::OnDraw(CUICellItem* cell)
{
    Fvector2 pos;
    cell->GetAbsolutePos(pos);
    UI().ClientToScreenScaled(pos, pos.x, pos.y);
    m_pFont->Out(pos.x, pos.y, m_string);
    m_pFont->OnRender();
}
