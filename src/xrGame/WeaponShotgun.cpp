#include "stdafx.h"
#include "WeaponShotgun.h"
#include "WeaponAutomaticShotgun.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponShotgun::CWeaponShotgun() {}
CWeaponShotgun::~CWeaponShotgun() {}

using namespace luabind;

// clang-format off
SCRIPT_EXPORT(CWeaponShotgun, (CGameObject),
{
	module(luaState)
	[
		class_<CWeaponShotgun,CGameObject>("CWeaponShotgun")
			.def(constructor<>()),
		class_<CWeaponAutomaticShotgun,CGameObject>("CWeaponAutomaticShotgun")
			.def(constructor<>())
	];
});
// clang-format on