rollout GJ_Toolbar "Gaijin Tools"
(
	local script_path = pathConfig.removePathLeaf (getSourceFileName())
	local btn_img = script_path + "\gj_icons_24i.bmp"
	local btn_size = 28

	--print script_path

	button dmg_build width:btn_size height:btn_size across:5
	button remove_bones width:btn_size height:btn_size
	button fix_normal width:btn_size height:btn_size
	button fix_char width:btn_size height:btn_size
	button gjin_about width:btn_size height:btn_size
	--checkbox chk_toggleEnabled "Toggle Enabled State" checked:true
	local theButtons = #(dmg_build, remove_bones, fix_normal , fix_char, gjin_about)
	fn updateButtons =
		(
			for i = 1 to theButtons.count do
			(
				local theIndex = i
				theButtons[i].images = #(btn_img,undefined,28, theIndex, theIndex, theIndex+1, theIndex+1, false, true)
			)
		)
	--buttons linked to macroscripts
	on dmg_build pressed do (macros.run "GJ_Tools" "GJDamagesSetup")
	on remove_bones pressed do (macros.run "GJ_Tools" "GJRemoveUnusedBones")
	on fix_normal pressed do (macros.run "GJ_Tools" "GJFixNormal")
	on fix_char pressed do (macros.run "GJ_Tools" "GJFiGliff")
	on gjin_about pressed do (macros.run "GJ_Tools" "About_GJ")

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_Toolbar open do
	(
		updateButtons()
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_Toolbar ((getINISetting INIfile "Toolbar" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_Toolbar moved new_pos do
	(
		try(setINISetting INIfile "Toolbar" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_Toolbar close do
	(
		try(setINISetting INIfile "Toolbar" "DialogPos" ((GetDialogPos GJ_Toolbar) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_Toolbar
cui.registerDialogBar GJ_Toolbar
cui.dockDialogBar GJ_Toolbar #cui_dock_all

--colorMan.setColor #text [0, 0, 1]
--colorMan.repaintUI #repaintAll
