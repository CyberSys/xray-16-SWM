/**************************************************/
/***** Физика оружия, и всё что с ней связано *****/ //--#SM+#--
/**************************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

// Создать физическую оболочку
void CWeapon::create_physic_shell() { CPhysicsShellHolder::create_physic_shell(); }

// Активировать физическую оболочку
void CWeapon::activate_physic_shell()
{
    UpdateXForm();
    CPhysicsShellHolder::activate_physic_shell();
}

// Настроить физическую оболочку
void CWeapon::setup_physic_shell() { CPhysicsShellHolder::setup_physic_shell(); }
