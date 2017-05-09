#include "pch_script.h"
#include "WeaponSVU.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponSVU::CWeaponSVU() {}
CWeaponSVU::~CWeaponSVU() {}

using namespace luabind;

SCRIPT_EXPORT(CWeaponSVU, (CGameObject), { module(luaState)[class_<CWeaponSVU, CGameObject>("CWeaponSVU").def(constructor<>())]; });
