#pragma once

class CUICellItem;
class CInventoryItem;

CUICellItem* create_cell_item(CInventoryItem* itm);
void remake_cell_item(CUICellItem*& old_cell); //--#SM+#--
