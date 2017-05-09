#include "stdafx.h"
#include "pch_script.h"
#include "WeaponRPG7.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponRPG7::CWeaponRPG7() {}
CWeaponRPG7::~CWeaponRPG7() {}

using namespace luabind;

SCRIPT_EXPORT(CWeaponRPG7, (CGameObject), { module(luaState)[class_<CWeaponRPG7, CGameObject>("CWeaponRPG7").def(constructor<>())]; });
