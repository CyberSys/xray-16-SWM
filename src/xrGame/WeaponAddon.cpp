/***************************************/
/***** Класс универсального аддона *****/ //--#SM+#--
/***************************************/

#include "stdafx.h"
#include "pch_script.h"
#include "WeaponAddon.h"
#include "xrScriptEngine/ScriptExporter.hpp"

CWeaponAddon::CWeaponAddon() {}

CWeaponAddon::~CWeaponAddon() {}

using namespace luabind;

SCRIPT_EXPORT(CScope, (CGameObject), {
    module(luaState)[class_<CScope, CGameObject>("CScope").def(constructor<>()),

        class_<CSilencer, CGameObject>("CSilencer").def(constructor<>()),

        class_<CGrenadeLauncher, CGameObject>("CGrenadeLauncher").def(constructor<>()),

        class_<CWeaponAddon, CGameObject>("CWeaponAddon").def(constructor<>())];
});