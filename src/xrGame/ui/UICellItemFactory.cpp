#include "stdafx.h"
#include "UICellItemFactory.h"
#include "UICellCustomItems.h"
#include "UIDragDropListEx.h" //--#SM+#--

CUICellItem* create_cell_item(CInventoryItem* itm)
{
    VERIFY(itm);
    CUICellItem* cell_item;

    CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(itm);
    CWeapon* pWeapon = smart_cast<CWeapon*>(itm);
    if (pAmmo)
    {
        cell_item = new CUIAmmoCellItem(pAmmo);
    }
    else if (pWeapon)
    {
        cell_item = new CUIWeaponCellItem(pWeapon);
    }
    else
    {
        cell_item = new CUIInventoryCellItem(itm);
    }
    return cell_item;
}

// Пересоздать иконку [re-create existed icon] --#SM+#--
void remake_cell_item(CUICellItem*& old_cell)
{
    xr_vector<PIItem> m_items;

    // Сохраняем предмет, которому принадлежит основная иконка
    m_items.push_back((PIItem)old_cell->m_pData);

    // Сохраняем предметы присоединённых к ней иконок
    for (u32 i = 0; i < old_cell->ChildsCount(); ++i)
    {
        CUICellItem* child_cell = old_cell->Child(i);
        m_items.push_back((PIItem)child_cell->m_pData);
    }

    // Сохраняем лист, в котором была наша иконка
    CUIDragDropListEx* owner_list = old_cell->OwnerList();
    R_ASSERT(owner_list != NULL);

    // Удаляем старую иконку
    CUICellItem* dying_cell = owner_list->RemoveItem(old_cell, true);
    dying_cell->m_b_destroy_childs = true;
    xr_delete(dying_cell);

    // Создаём новые
    old_cell = NULL;
    for (int i = 0; i < m_items.size(); i++)
    {
        CUICellItem* new_cell = create_cell_item(m_items[i]);
        owner_list->SetItem(new_cell);

        if (old_cell == NULL)
            old_cell = new_cell;
    }
}