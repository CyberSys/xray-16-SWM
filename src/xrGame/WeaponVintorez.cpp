#include "pch_script.h"
#include "WeaponVintorez.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponVintorez::CWeaponVintorez() {}
CWeaponVintorez::~CWeaponVintorez() {}

using namespace luabind;

SCRIPT_EXPORT(CWeaponVintorez, (CGameObject), { module(luaState)[class_<CWeaponVintorez, CGameObject>("CWeaponVintorez").def(constructor<>())]; });
