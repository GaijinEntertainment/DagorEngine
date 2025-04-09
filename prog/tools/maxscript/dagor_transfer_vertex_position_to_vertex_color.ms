--round function
fn round val dp = (local result = ((formattedPrint val format:("0." + (dp as string) + "f"))as float))

try(destroyDialog GJ_VColorPosTransfer)catch()
rollout GJ_VColorPosTransfer "Dagor Vetex Position to VColor" width:200
(
	group "1.Pick Source Object"(
		pickbutton vc_pick_source "Source Object" align:#center width:180 height:25 toolTip:"Select Sourche Object"

	)

	group "2.Transfer Position to VColor"(
		checkbox VColorOn "Show VColor Result" align:#left checked:true enable:true
		dropDownList Chnl "Copy Result to Channel Color" width:175 height:20 align:#left items:#("0", "1", "2", "3","4","5","6","7","8","9") selection:1
		label max_distance "" align:#center style_sunkenedge: true width:175 height:30
		progressBar pbProgressBar "Progress" width:180 height:10 color:(color 100 180 0) align:#center value:70
		button vc_go "Transfer Vetex Position to VColor!" align:#center width:180 height:25 toolTip:"Transfer Vetex Position to VColor"

		--slider MorphPercent "Preview Morph %" width:175 height:10 range: [0,100,100] type: #integer align:#center
	)
	group "3. Restore Position from VColor"
	(
		spinner vc_scale "Global Distance Multiplier" range:[0,100,1] fieldwidth:40
		button vc_res "Restore VertexPos to StartPos!" align:#center width:180 height:25 toolTip:"Transfer Vetex Position to VColor"
	)

	--functions
	--rounding value precession type
	local transferDone = 0
	--learn
	group "Learning Path"
	(
		button btnLearnF "Open Local Documentation" align:#center width:180 height:25
		button btnContactF "Contact with Developer" align:#center width:180 height:25
	)
	on btnLearnF pressed do (
		local pathArr = filterString (pathConfig.removePathLeaf(getSourceFileName()) as string) "\\"
		local finalPath1 = ""
		local finalPath2 = ""
		for t = 1 to pathArr.count do (
			finalPath2 = finalPath2 + pathArr[t] + "\\"
			if t <= pathArr.count-1 then finalPath1 = finalPath1 + pathArr[t] + "\\"
		)
		finalPath1 = finalPath1 + "documentation\\" + "transfer-vertex-position-to-vertex-color-tool.html "
		finalPath2 = finalPath2 + "documentation\\" + "transfer-vertex-position-to-vertex-color-tool.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/transfer_vertex_position_to_vertex_color_tool.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	on vc_pick_source picked obj do
	(
		if obj != undefined do vc_pick_source.text = obj.name
		transferDone = 0
	)
	--main loop vcolor Restore
	on vc_res pressed do
	(
		modalDial = queryBox "Are you sure?" title:"Warning!" icon:#warning
		print modalDial
		if modalDial == true then (
			undo on
			tObj = (selection as array)[1]

			buildVCFaces tObj true

			if (tObj != undefined) then
			(
				local totalSteps = getNumVerts tObj
				local mult = vc_scale.value
				for v = 1 to getNumVerts tObj do
				(
					local vertT = getVert tObj v
					local vertColor = (getVertColor tObj v)

					vert_x = vertT.x +((128 - vertColor.r)*mult)
					vert_y = vertT.y +((128 - vertColor.g)*mult)
					vert_z = vertT.z +((128 - vertColor.b)*mult)

					vertT.x = vert_x
					vertT.y = vert_y
					vertT.z = vert_z

					meshop.setVert tObj v vertT

					pbProgressBar.value = 100.000 / totalSteps * v
				)

				update tObj
				redrawViews
			)
			else
			(
				messageBox "Please select any EditPoly or EditMesh Object." title:"Warning!"
			)



		)
	)
	--main loop vcolor transfer
	on vc_go pressed do
	(
		undo on
		sObj = vc_pick_source.object
		tObj = (selection as array)[1]

		if (sObj != undefined) then
		(
			if (tObj != undefined) then
			(
				if (tObj != sObj) then
				(
					matFr = $.material
					--get class of object material top level material
					matFrClass = classof matFr as string
					if matFrClass != "Dagor_Dagorrat_Material_2" then (
						local script_path = pathConfig.removePathLeaf (getSourceFileName())
						local pFile = script_path + "\DagorLibrary.mat"
						local pFileError = doesFileExist pFile
						if pFileError == false then (
							messageBox "DagorLibrary.mat not found!"
						)
						else
						(
							matLib = loadTempMaterialLibrary pFile
							$.material = matLib[2]--asign second material from lib
						)
					)


					ConvertTo sObj Editable_mesh
					ConvertTo tObj Editable_mesh

					if (getNumVerts sObj) == (getNumVerts tObj) then
					(
						--make "black" all vertex color in target object
						--and find largest distance between vertex
						local distanceMult = 1
						local distanceB = 0

						local distanceX = 0
						local distanceY = 0
						local distanceZ = 0

						--IMPORTANT! we need clear all previos vcolor because it is broken from other software or VertexPaint Modifier
						macros.run "Tools" "Channel_Info"
						channelInfo.ClearChannel tObj 0
						maxOps.CollapseNode tObj true
						ChannelInfo.closeDialog()

						--main vloop
						for v = 1 to getNumVerts tObj do
						(
							--meshop.setVertColor tObj 0 v (color 0 0 0)

							local vertT = getVert tObj v
							local vertS = getVert sObj v
							if distanceB < (distance vertT vertS) then distanceB = (distance vertT vertS)

							if distanceX < (distance [vertT.x,0] [vertS.x,0]) then distanceX = (distance [vertT.x,0] [vertS.x,0])
							if distanceY < (distance [vertT.y,0] [vertS.y,0]) then distanceY = (distance [vertT.y,0] [vertS.y,0])
							if distanceZ < (distance [vertT.z,0] [vertS.z,0]) then distanceZ = (distance [vertT.z,0] [vertS.z,0])
						)

						max_distance.text = ("Maximum distance is: " + (distanceB as string) +"\n" +"Global multiplier is: " + ((round (distanceB/128) 5) as string))
						vc_scale.value = (round (distanceB/128) 5)

						if(distanceB/128 > 0) then (
							local totalSteps = getNumVerts sObj
							for v = 1 to getNumVerts sObj do
							(
								vertS = getVert sObj v
								vertT = getVert tObj v

								if vertT.x < vertS.x then
								(
									vcolor_X = (round (128 - ((distance [vertT.x,0] [vertS.x,0]) / (distanceB/128))) 0)
								)
								else
								(
									vcolor_X = (round (((distance [vertT.x,0] [vertS.x,0]) / (distanceB/128)) + 127) 0)
								)


								if vertT.y < vertS.y then
								(
									vcolor_Y = (round (128 - ((distance [vertT.y,0] [vertS.y,0]) / (distanceB/128))) 0)

								)
								else
								(
									vcolor_Y = (round (((distance [vertT.y,0] [vertS.y,0]) / (distanceB/128)) + 127) 0)
								)

								if vertT.z < vertS.z then
								(
									vcolor_Z = (round (128 - ((distance [vertT.z,0] [vertS.z,0]) / (distanceB/128))) 0)

								)
								else
								(
									vcolor_Z = (round (((distance [vertT.z,0] [vertS.z,0]) / (distanceB/128)) + 127) 0)
								)

								-- tObj 0 v (color vcolor_X vcolor_Y vcolor_Z 0)
								meshop.setVertColor tObj 0 v (color vcolor_X vcolor_Y vcolor_Z 0)
								meshop.setVertColor tObj (Chnl.selection-1) v (color vcolor_X vcolor_Y vcolor_Z 0)

								pbProgressBar.value = 100.000 / totalSteps * v
							)
							if VColorOn.checked == true then
							(
								setCVertMode tObj true
								setShadeCVerts tObj false
							)
							update sObj
							update tObj
						)
						else
						(
							for v = 1 to getNumVerts tObj do
							(
								meshop.setVertColor tObj 0 v (color 127 127 127)
							)
							if VColorOn.checked == true then
							(
								setCVertMode tObj true
								setShadeCVerts tObj false
							)
						)
						undo off
						pbProgressBar.value = 100
						transferDone = 1
					)
					else
					(messageBox "Source object and Target object must have same vertex count!" title:"Warning!" )

				)
				else
				(messageBox "Source object and Target object must be different objects!" title:"Warning!" )
			)
			else
			(messageBox "Please select another one EditPoly or EditMesh object to." title:"Warning!" )
		)
		else
		(messageBox "Please select any EditPoly or EditMesh Object." title:"Warning!" )

	)
	--display morph preview
	on MorphPercent buttondown do
	(
		if transferDone == 1 then
		(
			if (getNumVerts sObj) == (getNumVerts tObj) then
			(
				global tObjCopy = copy tObj
				global sObjCopy = copy sObj
				hide sObjCopy
				hide tObj
				hide sObj

			)
		)
	)
	on MorphPercent buttonup do
	(
		if transferDone == 1 then
		(
			if (getNumVerts sObj) == (getNumVerts tObj) then
			(
				unhide tObj
				unhide sObj
				delete tObjCopy
				delete sObjCopy
			)
		)
	)

	on MorphPercent changed val do
	(
		if tObjCopy != undefined and sObjCopy != undefined and transferDone == 1 then
		(
			for v = 1 to getNumVerts sObjCopy do
			(
				vertS = getVert sObjCopy v
				vertT = getVert tObjCopy v

				vertT.x = (vertT.x +((vertS.x-vertT.x)/100*MorphPercent.value))
				setVert tObjCopy v vertT
				update tObj
			)
			update tObj
		)
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_VColorPosTransfer open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_VColorPosTransfer ((getINISetting INIfile "VColorPosTransfer" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_VColorPosTransfer moved new_pos do
	(
		try(setINISetting INIfile "VColorPosTransfer" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_VColorPosTransfer close do
	(
		try(setINISetting INIfile "VColorPosTransfer" "DialogPos" ((GetDialogPos GJ_VColorPosTransfer) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
CreateDialog GJ_VColorPosTransfer style:#(#style_titlebar, #style_sysmenu, #style_toolwindow)