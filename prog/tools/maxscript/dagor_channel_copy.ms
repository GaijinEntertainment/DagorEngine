try(destroyDialog GJ_ChannelTransfer)catch()
rollout GJ_ChannelTransfer "Dagor Channel Copy Tool" width:200 height:218
(
	group "Settings"(
		dropDownList ChnlID "Channel Type" width:175 height:20 align:#left items:#("Geometry", "Selection", "Map") selection:3
		dropDownList ChnlIn "Copy from Channel" width:175 height:20 align:#left items:#("1", "2", "3","4","5","6","7","8","9") selection:3
		dropDownList ChnlOut "Copy to Channel" width:175 height:20 align:#left items:#("1", "2", "3","4","5","6","7","8","9") selection:8
	)
	group "Let`s do it!"(
		button ch_go "Copy Channels!" align:#center width:180 height:25 toolTip:"Transfer Vetex Position to VColor"
	)
	on ch_go pressed do
	(
		local ObjSel = selection as array
		if ObjSel.count > 0 then (
			for obj in ObjSel do (
				if (isValidNode obj) then (
					macros.run "Tools" "Channel_Info"
					channelInfo.CopyChannel obj ChnlID.selection ChnlIn.selection
					if ((ChnlIn.selection) < (ChnlOut.selection)) then
					(
						for i=1 to (abs((ChnlOut.selection - (ChnlIn.selection)))) do channelInfo.addChannel obj
					)
					channelInfo.PasteChannel obj ChnlID.selection ChnlOut.selection
					maxOps.CollapseNode obj true
					ChannelInfo.closeDialog()

					setUserPropBuffer obj ""
					setUserProp obj "renderable:b" "yes"
					setUserProp obj "collidable:b" "no"
				)
				else
				(
					messageBox "Please select only valid Objects! Object skiped..." title:"Warning!"
				)
			)
		)
		else
		(
			messageBox "Please select any valid Objects!" title:"Warning!"
		)
		print (GetDialogPos GJ_ChannelTransfer)
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_ChannelTransfer open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_ChannelTransfer ((getINISetting INIfile "ChnTransfer" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_ChannelTransfer moved new_pos do
	(
		try(setINISetting INIfile "ChnTransfer" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_ChannelTransfer close do
	(
		try(setINISetting INIfile "ChnTransfer" "DialogPos" ((GetDialogPos GJ_ChannelTransfer) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
CreateDialog GJ_ChannelTransfer
