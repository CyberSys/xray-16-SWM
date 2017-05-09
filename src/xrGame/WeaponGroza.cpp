#include "pch_script.h"
#include "WeaponGroza.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponGroza::CWeaponGroza() {}
CWeaponGroza::~CWeaponGroza() {}

using namespace luabind;

SCRIPT_EXPORT(CWeaponGroza, (CGameObject), { module(luaState)[class_<CWeaponGroza, CGameObject>("CWeaponGroza").def(constructor<>())]; });