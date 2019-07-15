///////////////////////////////////////////////////////////////
// Scope.h
// Scope - апгрейд оружия снайперский прицел
///////////////////////////////////////////////////////////////

#pragma once

#include "inventory_item_object.h"

class CScope : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CScope();
    virtual ~CScope();

    virtual bool item_is_wpn_addon() override { return true; } //--#SM+#--
};
