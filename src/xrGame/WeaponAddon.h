#pragma once

/***************************************/
/***** Класс универсального аддона *****/ //--#SM+#--
/***************************************/

#include "inventory_item_object.h"

#include "Scope.h"
#include "Silencer.h"
#include "GrenadeLauncher.h"

class CWeaponAddon : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

public:
    CWeaponAddon();
    virtual ~CWeaponAddon();
};
