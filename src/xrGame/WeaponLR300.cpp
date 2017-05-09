#include "pch_script.h"
#include "WeaponLR300.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponLR300::CWeaponLR300() {}
CWeaponLR300::~CWeaponLR300() {}

using namespace luabind;

SCRIPT_EXPORT(CWeaponLR300, (CGameObject), { module(luaState)[class_<CWeaponLR300, CGameObject>("CWeaponLR300").def(constructor<>())]; });
