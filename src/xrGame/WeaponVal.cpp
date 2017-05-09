#include "pch_script.h"
#include "WeaponVal.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponVal::CWeaponVal() {}
CWeaponVal::~CWeaponVal() {}

using namespace luabind;

SCRIPT_EXPORT(CWeaponVal, (CGameObject), { module(luaState)[class_<CWeaponVal, CGameObject>("CWeaponVal").def(constructor<>())]; });