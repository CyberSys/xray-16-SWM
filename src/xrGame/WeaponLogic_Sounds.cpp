/************************************/
/***** Работа со звуками оружия *****/ //--#SM+#--
/************************************/

#include "StdAfx.h"
#include "Weapon.h"

// Перезагрузка всех звуков, с учётом текущих аддонов
// Если передана секция - то будут загружены только звуки, присутствующие в ней (для апгрейдов)
void CWeapon::ReloadAllSounds(LPCSTR sFromSect)
{
    sReloadSndSectOverride = (sFromSect ? sFromSect : nullptr);

    ReloadAllSounds();

    sReloadSndSectOverride = nullptr;
}

// Перезагрузка всех звуков, с учётом текущих аддонов
void CWeapon::ReloadAllSounds()
{
    // clang-format off
	//		<Имя звука из конфига>		  <Движковый SID звука> <Флаг эксклюзивности>  <Тип звука для НПС>
	//		Одновременно на оружии может играться только один эксклюзивный звук - другие экслюзивные будут остановленны <!>
	ReloadSound( "snd_draw",					"sndShow"				, ESndExcl::eExYes,     SOUND_TYPE_ITEM_TAKING				);
	ReloadSound( "snd_holster",					"sndHide"				, ESndExcl::eExYes,     SOUND_TYPE_ITEM_HIDING				);
	ReloadSound( "snd_shoot",					"sndShot"				, ESndExcl::eExNot,	    SOUND_TYPE_WEAPON_SHOOTING			);
    ReloadSound( "snd_reflection",              "sndReflect"            , ESndExcl::eExNot,	    SOUND_TYPE_WORLD_AMBIENT			);
	ReloadSound( "snd_tape",					"sndTape"				, ESndExcl::eExNot,     SOUND_TYPE_WEAPON_SHOOTING			);
    ReloadSound( "snd_knife",					"sndKnife"				, ESndExcl::eExNot,	    SOUND_TYPE_WEAPON_SHOOTING			);
	ReloadSound( "snd_empty",					"sndEmptyClick"			, ESndExcl::eExNot,	    SOUND_TYPE_WEAPON_EMPTY_CLICKING	);
	ReloadSound( "snd_reload",					"sndReload"				, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_empty",			"sndReloadEmpty"		, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_w_gl",				"sndReloadWGL"			, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_empty_w_gl",		"sndReloadEmptyWGL"		, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_ab",				"sndReloadAB"			, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_ab_empty",			"sndReloadABEmpty"		, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_ab_w_gl",			"sndReloadABWGL"		, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_ab_empty_w_gl",	"sndReloadABEmptyWGL"	, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_fr_ab",			"sndReloadFrAB"			, ESndExcl::eExYes,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_fr_ab_empty",		"sndReloadFrABEmpty"	, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_fr_ab_w_gl",		"sndReloadFrABWGL"		, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_fr_ab_empty_w_gl",	"sndReloadFrABEmptyWGL"	, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_bore",					"sndBore"				, ESndExcl::eExYes,		SOUND_TYPE_IDLE						);
	ReloadSound( "snd_shoot_grenade",			"sndShotG"				, ESndExcl::eExNot,	    SOUND_TYPE_WEAPON_SHOOTING			);
	ReloadSound( "snd_reload_grenade",			"sndReloadG"			, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_switch",					"sndSwitch"				, ESndExcl::eExYes,		SOUND_TYPE_ITEM_USING				);
	ReloadSound( "snd_kick",					"sndKick"				, ESndExcl::eExNot,	    SOUND_TYPE_IDLE						);
	ReloadSound( "snd_zoomin",					"sndZoomIn"				, ESndExcl::eExYes,		SOUND_TYPE_ITEM_USING				);
	ReloadSound( "snd_zoomout",					"sndZoomOut"			, ESndExcl::eExYes,		SOUND_TYPE_ITEM_USING				);
	ReloadSound( "snd_pump",					"sndPump"				, ESndExcl::eExNot,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_pump_w_gl",				"sndPumpWGL"			, ESndExcl::eExNot,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_pump_aim",				"sndPumpAim"			, ESndExcl::eExNot,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_pump_aim_w_gl",			"sndPumpAimWGL"			, ESndExcl::eExNot,     SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_bipods_deploy",			"sndBipodsD"			, ESndExcl::eExOver,    SOUND_TYPE_ITEM_USING				);
	ReloadSound( "snd_bipods_retract",			"sndBipodsU"			, ESndExcl::eExOver,	SOUND_TYPE_ITEM_USING				);

	// Звук глушителя
	if ( get_SilencerStatus() != ALife::eAddonDisabled )
	{
		ReloadSound( "snd_silncer_shot", "sndSilencerShot", ESndExcl::eExNot, SOUND_TYPE_WEAPON_SHOOTING );
	}

	// Звуки перезарядки в три стадии
	if(m_bTriStateReload_main)
	{
		ReloadSound( "snd_open_weapon",						"sndOpen"							, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_weapon_w_gl",				"sndOpenWGL"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_open_empty_weapon",				"sndOpenEmpty"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon_w_gl",			"sndOpenEmptyWGL"					, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_add_cartridge",					"sndAddCartridge"					, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_w_gl",				"sndAddCartridgeWGL"				, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_add_cartridge_empty",				"sndAddCartridgeEmpty"				, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_empty_w_gl",		"sndAddCartridgeEmptyWGL"			, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_lbs",			    "sndAddCartridgeLBS"				, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_add_cartridge_lbs_w_gl",			"sndAddCartridgeLBSWGL"				, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_close_weapon",					"sndClose"							, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_w_gl",				"sndCloseWGL"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_close_empty_weapon",				"sndCloseEmpty"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
     	ReloadSound( "snd_close_empty_weapon_w_gl",			"sndCloseEmptyWGL"					, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_close_fempt_weapon",				"sndCloseFE"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon_w_gl",			"sndCloseFEWGL"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_close_fempt_empty_weapon",		"sndCloseEmptyFE"					, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon_w_gl",	"sndCloseEmptyFEWGL"				, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
    }

	if(m_bTriStateReload_gl)
	{
		ReloadSound( "snd_open_weapon_g",					"sndOpenG"							, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon_g",				"sndOpenEmptyG"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_g",					"sndAddCartridgeG"					, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_empty_g",			"sndAddCartridgeEmptyG"				, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_close_weapon_g",					"sndCloseG"							, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_empty_weapon_g",			"sndCloseEmptyG"					, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon_g",			"sndCloseFEG"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon_g",		"sndCloseEmptyFEG"					, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
	}

	if(m_bTriStateReload_ab)
	{
		ReloadSound( "snd_open_weapon_ab",					"sndOpenAB"							, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_weapon_ab_w_gl",				"sndOpenABWGL"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_ab",				"sndAddCartridgeAB"					, ESndExcl::eExOver,    SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_ab_w_gl",			"sndAddCartridgeABWGL"				, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_ab",					"sndCloseAB"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_ab_w_gl",			"sndCloseABWGL"						, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
	}

	if(m_bTriStateReload_frab)
	{
		ReloadSound( "snd_open_weapon_fr_ab",					"sndOpenFrAB"					, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon_fr_ab",				"sndOpenFrABEmpty"				, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_weapon_fr_ab_w_gl",				"sndOpenFrABWGL"				, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon_fr_ab_w_gl",		"sndOpenFrABEmptyWGL"			, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_fr_ab",					"sndAddCartridgeFrAB"			, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_fr_ab_empty",			"sndAddCartridgeFrABEmpty"		, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_fr_ab_w_gl",			"sndAddCartridgeFrABWGL"		, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_add_cartridge_fr_ab_empty_w_gl",		"sndAddCartridgeFrABEmptyWGL"	, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "anm_add_cartridge_lbs_fr_ab",				"sndAddCartridgeLBSFrAB"		, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_add_cartridge_lbs_fr_ab_w_gl",		"sndAddCartridgeLBSFrABWGL"		, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
        ReloadSound( "snd_close_weapon_fr_ab",					"sndCloseFrAB"					, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_empty_weapon_fr_ab",			"sndCloseFrABEmpty"				, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_fr_ab_w_gl",				"sndCloseFrABWGL"				, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_empty_weapon_fr_ab_w_gl",		"sndCloseFrABEmptyWGL"			, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon_fr_ab",			"sndCloseFrABFE"				, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon_fr_ab",		"sndCloseFrABEmptyFE"			, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon_fr_ab_w_gl",		"sndCloseFrABFEWGL"				, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon_fr_ab_w_gl",	"sndCloseFrABEmptyFEWGL"		, ESndExcl::eExYes,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_switch_add_cartridge",				"sndSwAddCartridge"				, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_switch_add_cartridge_empty",			"sndSwAddCartridgeEmpty"		, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_switch_add_cartridge_w_gl",			"sndSwAddCartridgeWGL"			, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_switch_add_cartridge_empty_w_gl",		"sndSwAddCartridgeEmptyWGL"		, ESndExcl::eExOver,	SOUND_TYPE_WEAPON_RECHARGING	);
	}

    // Звуки перезарядки для НПС / Третьего лица
    ReloadSound("snd_reload_npc", "sndReloadNPC", ESndExcl::eExYes, SOUND_TYPE_WEAPON_RECHARGING);

    // clang-format on

    // Звуки с привязкой к anm_
    CInifile::Sect&   _sect = pSettings->r_section(this->cNameSect());
    auto              _b    = _sect.Data.begin();
    auto              _e    = _sect.Data.end();

    for (; _b != _e; ++_b)
    {
        if (strstr(_b->first.c_str(), "snd_anm_") == _b->first.c_str())
        {
            ReloadSound(_b->first, _b->first, ESndExcl::eExYes, sg_SourceType);
        }
    }
}

// Перезагрузить все звуки с учётом установленных апгрейдов
void CWeapon::ReloadAllSoundsWithUpgrades()
{
    // Грузим оригинальные звуки
    ReloadAllSounds();

    // Перебираем все установленные апгрейды и грузим звуки с них
    CInifile::Sect& _sect = pSettings->r_section(this->cNameSect());
    auto _b = m_upgrades.begin();
    auto _e = m_upgrades.end();

    for (; _b != _e; ++_b)
    {
        LPCSTR sUpgrSect = READ_IF_EXISTS(pSettings, r_string, _b->c_str(), "section", nullptr);
        if (sUpgrSect != nullptr)
            install_upgrade_sounds(sUpgrSect, false);
    }
}

// Перезагрузить конкретный звук, с учётом текущих аддонов
void CWeapon::ReloadSound(shared_str const& strName, shared_str const& strAlias, ESndExcl exMode, int type)
{
    bool exclusive = (exMode != eExNot ? true : false);
    bool overlay = (exMode == eExOver ? true : false);

    shared_str strSection;

    // Специальный аддон 1
    if (IsSpecial_1_Attached() && get_Special_1_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_1_SetSect(), strName))
        {
            strSection = GetSpecial_1_SetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Специальный аддон 2
    if (IsSpecial_2_Attached() && get_Special_2_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_2_SetSect(), strName))
        {
            strSection = GetSpecial_2_SetSect();
            goto label_ReloadSound_reload;
        }
    }
    // Специальный аддон 3
    if (IsSpecial_3_Attached() && get_Special_3_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_3_SetSect(), strName))
        {
            strSection = GetSpecial_3_SetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Специальный аддон 4
    if (IsSpecial_4_Attached() && get_Special_4_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_4_SetSect(), strName))
        {
            strSection = GetSpecial_4_SetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Специальный аддон 5
    if (IsSpecial_5_Attached() && get_Special_5_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_5_SetSect(), strName))
        {
            strSection = GetSpecial_5_SetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Специальный аддон 6
    if (IsSpecial_6_Attached() && get_Special_6_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_6_SetSect(), strName))
        {
            strSection = GetSpecial_6_SetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Магазин
    if (IsMagazineAttached() && get_MagazineStatus() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetMagazineSetSect(), strName))
        {
            strSection = GetMagazineSetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Подствол
    if (IsGrenadeLauncherAttached() && get_GrenadeLauncherStatus() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetGrenadeLauncherSetSect(), strName))
        {
            strSection = GetGrenadeLauncherSetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Глушитель
    if (IsSilencerAttached() && get_SilencerStatus() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSilencerSetSect(), strName))
        {
            strSection = GetSilencerSetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Прицел
    if (IsScopeAttached() && get_ScopeStatus() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetScopeSetSect(), strName))
        {
            strSection = GetScopeSetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Если ничего не нашли - пробуем грузить из секции оружия
    strSection = this->cNameSect();

// Пере-загружаем звук
label_ReloadSound_reload:

    bool bOrigSndType = READ_IF_EXISTS(pSettings, r_bool, strSection.c_str(), make_string("%s_use_src_sndtype", strName.c_str()).c_str(), false);
    if (bOrigSndType == true)
        type = sg_SourceType;

    if (sReloadSndSectOverride != nullptr)
    {
        // Если передана внешняя секция - пробуем взять звуки от туда
        if (pSettings->line_exist(sReloadSndSectOverride, strName.c_str()))
            m_sounds.ReLoadSound(sReloadSndSectOverride, strName.c_str(), strAlias.c_str(), exclusive, type, overlay);
    }
    else
        m_sounds.ReLoadSound(strSection.c_str(), strName.c_str(), strAlias.c_str(), exclusive, type, overlay);
}

// Обновление позиции звуков
void CWeapon::UpdateSounds()
{
    if (Device.dwFrame == dwUpdateSounds_Frame)
        return;

    dwUpdateSounds_Frame = Device.dwFrame;

    Fvector P = get_LastFP();

    m_sounds.UpdateAllSounds(P);
}

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

// Звук входа в прицеливание
void CWeapon::PlaySoundZoomIn()
{
    IGameObject* pParent = H_Parent();
    if (pParent == NULL)
        return;

    m_sounds.StopSound("sndZoomOut");
    PlaySound("sndZoomIn", pParent->Position());
}

// Звук выхода из прицеливания
void CWeapon::PlaySoundZoomOut()
{
    IGameObject* pParent = H_Parent();
    if (pParent == NULL)
        return;

    m_sounds.StopSound("sndZoomIn");
    PlaySound("sndZoomOut", pParent->Position());
}
