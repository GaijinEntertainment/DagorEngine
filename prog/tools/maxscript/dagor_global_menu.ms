rollout GJ_Buttons2025 "Gaijin Tools" width:200
(
	--create GUI
	button dmg_build "Damages Setup..." align:#center width:180 height:25 toolTip:"Damages Objects Setup"
	button remove_bones "Remove Unused Bones..." align:#center width:180 height:25 toolTip:"Remove Unused Bones"
	button fix_normal "Fix Inverted Normals..." align:#center width:180 height:25 toolTip:"Fix Inverted Normals"
	button fix_char "Check Illegal Characters..." align:#center width:180 height:25 toolTip:"Check Illegal Characters"

	button btn_AO_Baker "Ambient Occlusion Baker..." align:#center width:180 height:25 toolTip:"Ambient Occlusion Baker"
	button btn_CMP_Gen "CMP Generator..." align:#center width:180 height:25 toolTip:"CMP Generator"
	button btn_Dagorppainter "Pivot Painter Vegetetion Tool..." align:#center width:180 height:25 toolTip:"Pivot Painter Vegetetion Tool"
	button btn_DegenerateTrisCheck "Degenerate Tris Checker..." align:#center width:180 height:25 toolTip:"Degenerate Tris Checker"
	button btn_Floater "Gaijin Base Tools Floater..." align:#center width:180 height:25 toolTip:"Gaijin Base Tools Floater"
	button btn_IO "Custom Batch Exporter..." align:#center width:180 height:25 toolTip:"Custom Batch Exporter"
	button btn_LODGenerator "LOD Generator..." align:#center width:180 height:25 toolTip:"LOD Generator"
	button btn_NPuff "Editing Normals Tools..." align:#center width:180 height:25 toolTip:"Editing Normals Tools"
	button btn_VColorPosTransfer "Transfer Vertex Pos to VColor..." align:#center width:180 height:25 toolTip:"Transfer Vertex Pos to VColor"

	button gjin_about "About Tools..." align:#center width:180 height:25 toolTip:"About Tools"

	--buttons linked to macroscripts
	on dmg_build pressed do (macros.run "GJ_Tools" "GJDamagesSetup")
	on remove_bones pressed do (macros.run "GJ_Tools" "GJRemoveUnusedBones")
	on fix_normal pressed do (macros.run "GJ_Tools" "GJFixNormal")
	on fix_char pressed do (macros.run "GJ_Tools" "GJFiGliff")
	on gjin_about pressed do (macros.run "GJ_Tools" "About_GJ")

	on btn_AO_Baker pressed do (macros.run "GJ_Tools" "GJ_AO_Baker")
	on btn_CMP_Gen pressed do (macros.run "GJ_Tools" "GJ_CMP_Gen")
	on btn_Dagorppainter pressed do (macros.run "GJ_Tools" "GJ_Dagorppainter")
	on btn_DegenerateTrisCheck pressed do (macros.run "GJ_Tools" "GJ_DegenerateTrisCheck")
	on btn_Floater pressed do (macros.run "GJ_Tools" "GJ_Floater")
	on btn_IO pressed do (macros.run "GJ_Tools" "GJ_IO")
	on btn_LODGenerator pressed do (macros.run "GJ_Tools" "GJ_LODGenerator")
	on btn_NPuff pressed do (macros.run "GJ_Tools" "GJ_NPuff")
	on btn_VColorPosTransfer pressed do (macros.run "GJ_Tools" "GJ_VColorPosTransfer")

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_Buttons2025 open do
	(
		--updateButtons()
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_Buttons2025 ((getINISetting INIfile "Toolbar" "DialogPos2025")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_Buttons2025 moved new_pos do
	(
		try(setINISetting INIfile "Toolbar" "DialogPos2025" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_Buttons2025 close do
	(
		try(setINISetting INIfile "Toolbar" "DialogPos2025" ((GetDialogPos GJ_Buttons2025) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_Buttons2025 bgcolor:(color 100 100 100) fgcolor:(color 0 255 0) --style:#(#style_titlebar)
