-- for suggestions please contact a.vlasov@gaijin.team
-- Dagor scripts toolbox 2.2

--include for all used scripts here
--filein @"..\dagor_fast_prop_edit.ms"
--filein @"..\dagor_tools_floater.ms"
macroScript GJ_CMP_Gen
buttontext:"Dagor CMP Export..."
category:"GJ_Tools"
IconName:"MassFX/MultiObjectEditor"
(
-- Simple CMP file generator from one selected Layer
-- Only for 3dsMax 2021 and above
global INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
global SavePath = (GetDir #export) as string
global new_pos = [0,0]

rollout GJ_CMP_Gen "Dagor CMP Exporter" width:230 height:208
(

	timer CMP_Clock "CMP_Clock" interval:1 -- 1 times a second

	group "CMP Saving Properties"
	(
		dropdownlist edtNodeType "Default Node Type" items:#("", "rendInst", "composit", "gameObj") selection:1
		--editText edtNodeType "Default Node Type" width:188 height:20 text: "rendInst" readonly: false multiLine: false
		checkbox CMP_remSuffix "Remove numeric suffixes if exist" checked:true enable:true
		button btnCMP_Gen "Export CMP" align:#center width:180 height:25
	)

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
		finalPath1 = finalPath1 + "documentation\simple-cmp-editor.html "
		finalPath2 = finalPath2 + "documentation\simple-cmp-editor.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/simple_cmp_editor.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	on btnOutput pressed do (
		DefaultDirS = (getINISetting ((GetDir #userStartupScripts) + "/GJ_WindowsPos.ini") "CMP_Generator" "CMP_Save_Path") as string
		print DefaultDirS
		SavePath = getSavePath caption:"Select Output Directory:" initialDir:(DefaultDirS)
		if (SavePath == undefined) then (
			SavePath = (GetDir #export) as string
			edtOutput.text = "Default 3DSMax Export Directory..."

		)
		else (
			edtOutput.text = SavePath
		)
	)


	fn Matrix3ToDagor3 obj = (
		local R_x_90 = matrix3 [1,0,0] [0,0,-1] [0,1,0] [0,0,0]
		local reflect_z = matrix3 [1,0,0] [0,1,0] [0,0,-1] [0,0,0]

		local transform_matrix = R_x_90 * reflect_z
		local ObjFMatrix = transform_matrix * obj.transform

		ObjFMatrix = replace (ObjFMatrix as string) 1 9 ""
		ObjFMatrix = replace ObjFMatrix ObjFMatrix.count 1 ""

		--make array from matrix
		ObjFMatrix = filterString ObjFMatrix "[ ] , " splitEmptyTokens: false
		local x1 = ObjFMatrix[1] as float
		local y1 = ObjFMatrix[2] as float
		local z1 = ObjFMatrix[3] as float

		local x2 = ObjFMatrix[4] as float
		local y2 = ObjFMatrix[5] as float
		local z2 = ObjFMatrix[6] as float

		local x3 = ObjFMatrix[7] as float
		local y3 = ObjFMatrix[8] as float
		local z3 = ObjFMatrix[9] as float

		local x4 = ObjFMatrix[10] as float
		local y4 = ObjFMatrix[11] as float
		local z4 = ObjFMatrix[12] as float

		--swap YZ
		ObjFMatrix = ("["+x1 as string+", "+z1 as string+", "+y1 as string+"] ["+x2 as string+", "+z2 as string+", "+y2 as string+"] ["+x3 as string+", "+z3 as string+", "+y3 as string+"] ["+x4 as string+", "+z4 as string+", "+y4 as string+"]")
		return ObjFMatrix
	)

	--main CMP start here
	on btnCMP_Gen pressed do (

		local error_select = 0
		local SExplorerOpened = 0
		if SceneExplorerManager.ExplorerIsOpen "Layer Explorer" == true then
		(
			SExplorerOpened = 1
		)
		else
		(
			SceneExplorerManager.ClearAllExplorers()
			SceneExplorerManager.CreateExplorerFromDefault "Layer Explorer"
			SExplorerOpened = 1
		)




		--resSave = (filterString (resSave as string) "\\" splitEmptyTokens: false)
		--resSave = resSave[resSave.count]

		--abort export if layer doesn`t have objects
		if selection.count != 0 then (
			resSave = getSaveFileName caption: "Save your composit file" filename:(SavePath + "\\") types:"Dagor Composit(*composit.blk)|*composit.blk"
			if resSave == undefined then error_select = 2

			if error_select == 0 then (
				--selection ok
				SavePath = resSave
				setINISetting INIfile "CMP_Generator" "CMP_Save_Path" (SavePath as string) forceUTF16:false

				if (findString resSave ".composit.blk") != undefined then(
					CMP_File = createfile (resSave)
				)
				else
				(
					CMP_File = createfile (resSave + ".composit.blk")
				)

				format "%\n" "className:t=\"composit\"" to:CMP_File
				--format "%\n" "" to:CMP_File
				--format "%\n" "node{" to:CMP_File
				--format "%\n" "  tm:m=[[1, 0, 0] [0, 1, 0] [0, 0, 1] [0, 0, 0]]" to:CMP_File

				local selObj = selection
				local selCounter = 1
				for i in selection do
				(
					--object visible
					if i.isHidden == false then (
						format "%\n" "" to:CMP_File
						format "%\n" "  node{" to:CMP_File

						local lastChr = i.name
						if CMP_remSuffix.checked == true then (
							--check name for last tree number if existing and remove if existing
							--for tree number
							local tmp = ((replace lastChr 1 (lastChr.count - 3) "") as float)
							if tmp != undefined then (
								lastChr = (replace lastChr (lastChr.count - 2) 3 "")
							)
							--two number
							local tmp = ((replace lastChr 1 (lastChr.count - 2) "") as float)
							if tmp != undefined then (
								lastChr = (replace lastChr (lastChr.count - 1) 2 "")
							)
							--one number
							local tmp = ((replace lastChr 1 (lastChr.count - 1) "") as float)
							if tmp != undefined then (
								lastChr = (replace lastChr (lastChr.count) 1 "")
							)

							--remove "_" symbol from last if exist
							for i = 1 to 2048 do (
								if lastChr[lastChr.count] == "_" then lastChr = (replace lastChr (lastChr.count) 1 "")
							)
						)

						--fix illegal character inside current object name if present
						lastChr =  (FilterString lastChr ";,. +-!@#$%^&*()|~`\/")

						if edtNodeType.selection == 1 then (
							local ObjName = "    name:t=\""+ (lastChr[1] as string) + "\""
						)
						else
						(
							local ObjName = "    name:t=\""+ (lastChr[1] as string) +":" + edtNodeType.selected + "\""

						)


						format "%\n" ObjName to:CMP_File

						local toDagorProp = Matrix3ToDagor3 selObj[selCounter]

						--CMP Properties doesn`t exist and write default properties
						ObjTranform = "    tm:m=[" + toDagorProp +"]"
						format "%\n" ObjTranform to:CMP_File

						selCounter = selCounter + 1
						format "%\n" "  }" to:CMP_File
					)
				)
				--format "%\n" "}"  to:CMP_File
				close CMP_File

				clearSelection()
			)
			else
			(
				--selection have error
				if error_select == 2 then messageBox "Save composit file aborted!"
			)
		)
		else
		(
			messagebox "Please select any objects!"
		)

		if SExplorerOpened == 0 then SceneExplorerManager.CloseExplorer "Layer Explorer"
	)

	-----------------------------save script window_position on background-----------------------------------------------
  global INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"

	on GJ_CMP_Gen open do
	(

		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_CMP_Gen ((getINISetting INIfile "CMP_Generator" "DialogPos")as Point2)
			SavePath = (getINISetting INIfile "CMP_Generator" "CMP_Save_Path") as string
			edtOutput.text = SavePath
		) catch (
			--our data not found or inin file not exist
			SavePath = (GetDir #export) as string
			setINISetting INIfile "CMP_Generator" "CMP_Save_Path" (SavePath as string) forceUTF16:false
			setINISetting INIfile "CMP_Generator" "DialogPos" ((GetDialogPos GJ_CMP_Gen) as string) forceUTF16:false

		)
    )

	--write to ini file if dialog moved or closed
	on GJ_CMP_Gen moved new_pos do
	(
		setINISetting INIfile "CMP_Generator" "DialogPos" (new_pos as string) forceUTF16:false
	)
    on GJ_CMP_Gen close do
	(
		setINISetting INIfile "CMP_Generator" "DialogPos" ((GetDialogPos GJ_CMP_Gen) as string) forceUTF16:false
		setINISetting INIfile "CMP_Generator" "CMP_Save_Path" (SavePath as string) forceUTF16:false
    )
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_CMP_Gen

)
macroScript GJ_VColorPosTransfer
buttontext:"Dagor Vetex Pos to VColor..."
category:"GJ_Tools"
IconName:"MainUI\ColorClipBoard"
(
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
)
--global GJ_Floater_UD
macroScript GJ_Floater_UD
buttontext:"Fast Properties User Defined..."
category:"GJ_Tools"
IconName:"MainUI\ToggleSceneExplorer"
(
global rolloutArray = #() -- this array holds rollouts for created/existing rollouts and "undefined" for removed rollouts
global rolloutNumber = 1 -- global variable to act as index for the rolloutArray
global selectedPropVar = ""

--Create callback for object info overdraw
fn showObjectNames=
(
	--gw.setTransform(Matrix3 1)
	local ObjSelMaxF = 0
	for o in selection do
		(
		if ObjSelMaxF <= 20 then (
		--compute draw backgroud block size
		local ParamArrY = FilterString (getUserPropBuffer o) "\n"
		local ParamArrX = 0
		for i = 1 to ParamArrY.count do (
			local ParamArrLocalY = ParamArrY[i]
			if(ParamArrLocalY.count > ParamArrX) then ParamArrX = ParamArrLocalY.count
		)

		ParamArrLocalY = o.name
		if(ParamArrLocalY.count > ParamArrX) then ParamArrX = ParamArrLocalY.count

		--draw Rectangle
		local DrawXY = gw.TransPoint o.pos
		local DrawX = DrawXY[1]-10
		local DrawY = DrawXY[2]-15
		local rect1 = (box2 (DrawX-1) (DrawY-1) (((ParamArrX+1)*6) + 12) ((ParamArrY.count+1)*13 + 32))
		local rect2 = (box2 DrawX DrawY (((ParamArrX+1)*6) + 10) ((ParamArrY.count+1)*13 + 30))
		local rect3 = (box2 DrawX (DrawY + 26) (((ParamArrX+1)*6) + 10) ((ParamArrY.count+1)*13 + 30 - 26))

		local aColor = color 50 50 50 1
		local bColor = color 0 0 0 1
		local dColor = color 100 100 100 1

		--gw.clearScreen rect1 useBkg:true
		gw.wrect rect1 bColor
		gw.wrect rect2 aColor
		gw.wrect rect3 dColor

		--draw text
		gw.text o.pos (o.name + "\n" + "\n" + (getUserPropBuffer o)) color:white

		ObjSelMaxF = ObjSelMaxF + 1
		--print (getUserPropBuffer o)
		)
	)
	gw.updateScreen()
)
--registerRedrawViewsCallback showObjectNames
--showObjectNames()
unRegisterRedrawViewsCallback showObjectNames

--main rollout for Fast Properties
try(DestroyDialog ::GJ_Floater_UD)catch()
rollout GJ_Floater_UD "Dagor Fast Properties" width:200 height:600
(

	timer MainLoopClock "MainLoopClock" interval:1 -- 10 times a second

	local isKeyDown = (dotNetClass "managedservices.keyboard").isKeyDown
	local keys = dotNetClass "system.windows.forms.keys"

	group "Selected Objects Parameters"
	(
		button btnCopyF "Copy Obj User Defined" align:#center width:180 height:23
		button btnPasteF "Paste Obj User Defined" align:#center width:180 height:23
		button btnClearF "Clear Obj User Defined" align:#center width:180 height:23
		button btnVColorOn "Vertex Color ON" align:#center width:180 height:23
		button btnVColorOff "Vertex Color OFF" align:#center width:180 height:23
		editText objF_param "" fieldWidth:180 height:0 align:#center text:"Please select any objects" labelOnTop:true
		checkbox ParamFloatingShow "Display Properties on Viewport" align:#left checked:true enable:true
		--checkbox ParamFloatingShowAlways "Display Properties Always" align:#left checked:false enable:true
		--checkbox VColorShow "Viewport Vertex Color" align:#left checked:false enable:true
		--checkbox BBoxShow "Display as Box" align:#left checked:false enable:true
	)

	-- adds a rollout to the rolloutArray
	fn AddRolloutOpen = (
	local ObjSelMax = 0
		for i in selection do
		(	if ObjSelMax <= 9 then(
			global customDefM = getUserPropBuffer i

			--compute block height
			global ParamArrY = FilterString (getUserPropBuffer i) "\n"
			ParamArrY = (ParamArrY.count*13) + 20

			roCreator = rolloutCreator "dynamicRollout" (i.name as string)
			roCreator.begin()
			roCreator.addControl #editText ("CustomDef" + rolloutNumber as string) ("") paramStr:"fieldWidth:180 height:ParamArrY align:#center labelOnTop:true text:customDefM"
			--roCreator.addControl #button #btnRemoveRollout "Copy This Properties"

			roCreator.addLocal "thisRolloutNumber" init:"rolloutNumber"
			roCreator.addHandler #btnRemoveRollout #pressed codeStr:"
			RemoveSubRollout GJ_Floater_UD.theSubRollout rolloutArray[thisRolloutNumber]
			print (@Closed rollout @ + thisRolloutNumber as string)
			rolloutArray[thisRolloutNumber] = undefined
			--print (rolloutArray.count)
			"
			roCreator.end()

			AddSubRollout GJ_Floater_UD.theSubRollout roCreator.def
			rolloutArray[rolloutNumber] = roCreator.def -- adds the created rollout into the array at the last position
			rolloutNumber += 1
			ObjSelMax +=1
			)
		)
	)

	-- shows/hides the created rollouts
	fn HideRollouts =
	(
		-- hide rollouts
		for rolloutIndex = 1 to rolloutArray.count do
		(
			if (rolloutArray[rolloutIndex] != undefined) do
			(
				RemoveSubRollout GJ_Floater_UD.theSubRollout rolloutArray[rolloutIndex]
			)
		)
		rolloutNumber = 1
	)

		fn deleteUserProp node = (
		setUserPropBuffer node ""
	)

	--copy first selected object custopm properties to global variable
	on btnCopyF pressed do (
		if selection.count > 0 then
		(
			local ObjSelF = selection as array
			selectedPropVar = getUserPropBuffer ObjSelF[1]
			setclipboardText selectedPropVar
			--print selectedPropVar
		)
		else
		(
			messageBox "Please Select Any Object"
		)

	)
	--paste custopm properties of first selected object from global var
	on btnPasteF pressed do (

		if selection.count > 0 then
		(
			local ObjSelF = selection as array
			for i in ObjSelF do (
				deleteUserProp i
			)

			selectedPropVar = getclipboardText()
			if selectedPropVar == undefined then selectedPropVar = ""

			for i = 1 to ObjSelF.count do (
				setUserPropBuffer ObjSelF[i] selectedPropVar
			)
			HideRollouts()
			AddRolloutOpen()
		)
		else
		(
			messageBox "Please Select Any Object"
		)
	)

	--clear custopm properties on selected objects
	on btnClearF pressed do (

		if selection.count > 0 then
		(
			local ObjSelF = selection as array
			for i in ObjSelF do (
				deleteUserProp i
			)
			HideRollouts()
			AddRolloutOpen()
		)
		else
		(
			messageBox "Please Select Any Object"
		)
	)

	--show\hide on viewport selected objects user defined properties and objects names
	on ParamFloatingShow changed theState do (
		if ParamFloatingShow.checked == false then (
			unregisterRedrawViewsCallback showObjectNames
			showObjectNames()
		)
		else
		(
			registerRedrawViewsCallback showObjectNames
			showObjectNames()
		)
	)
	--learn
	on btnLearnF pressed do (
		local pathArr = filterString (pathConfig.removePathLeaf(getSourceFileName()) as string) "\\"
		local finalPath1 = ""
		local finalPath2 = ""
		for t = 1 to pathArr.count do (
			finalPath2 = finalPath2 + pathArr[t] + "\\"
			if t <= pathArr.count-1 then finalPath1 = finalPath1 + pathArr[t] + "\\"
		)
		finalPath1 = finalPath1 + "documentation\fast-editor-user-properties.html "
		finalPath2 = finalPath2 + "documentation\fast-editor-user-properties.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/fast_editor_user_properties.html" ""
	)

	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	-- show\hide vertex color on selected objects
	on btnVColorOn pressed do (
		local ObjSelF = selection as array
		for MainObjF in ObjSelF do (
			setCVertMode MainObjF true
			setShadeCVerts MainObjF false
		)
		redrawViews()
	)
	on btnVColorOff pressed do (
		local ObjSelF = selection as array
		for MainObjF in ObjSelF do (
			setCVertMode MainObjF false
			setShadeCVerts MainObjF false
		)
		redrawViews()
	)

	-- show\hide bounding boxes on selected objects
	/*on BBoxShow changed theState do (

		local ObjSelF = selection as array

		if BBoxShow.checked == false then (
			for MainObjF in ObjSelF do (
				MainObjF.boxMode = false
			)
		)
		else
		(
			for MainObjF in ObjSelF do (
				MainObjF.boxMode = true
			)
		)
		redrawViews()
	)
	*/

	global FDialogOpened = 0
	global ShiftTimer = -11
fn MnLoop =
	(
		-- main loop
		if (keyboard.controlPressed == true	and keyboard.shiftPressed == true and FDialogOpened == 0 and ShiftTimer < -10) then
			(
				FDialogOpened = 1

				if ParamFloatingShow.checked == false then (
					unregisterRedrawViewsCallback showObjectNames
					showObjectNames()
				)
				else
				(
					registerRedrawViewsCallback showObjectNames
					showObjectNames()
				)

				HideRollouts()
				AddRolloutOpen()

				local FDPos = mouse.screenpos
				SetDialogPos GJ_Floater_UD [(FDPos[1]-100),(FDPos[2]-35)]

				ShiftTimer = 0
			)
			if (FDialogOpened == 1) then (
				ShiftTimer = ShiftTimer + 1
			)
			if (FDialogOpened == 0) then (
				ShiftTimer = ShiftTimer - 1
			)
		if (keyboard.controlPressed == true	and keyboard.shiftPressed == true and FDialogOpened == 1 and ShiftTimer > 10) then
			(
				FDialogOpened = 0
				unRegisterRedrawViewsCallback showObjectNames
				ShiftTimer = 0
				SetDialogPos GJ_Floater_UD [-1000,-1000]
			)

		-- for any other keys
		--local space_down = isKeyDown keys.Space
		--if space.checked != space_down do space.checked = space_down
	)

	--update main loop only selected times
	local ObjSelA = selection as array
	local SelectionOld = undefined
	local textTick = 0

	--main loop update every tick
	on MainLoopClock tick do
	(
		MnLoop()
		--Update text of user difined properties if selection changed
		if selection.count > 0 then (
			ObjSelA = selection as array
			if (SelectionOld as string) != (ObjSelA as string) then (
				textTick = 1
			)
			if textTick == 1 then (
					SelectionOld = deepcopy ObjSelA
					objF_param.text = (getUserPropBuffer ObjSelA[1])
				textTick = 0

				HideRollouts()
				AddRolloutOpen()
			)
		)
		else
		(
			objF_param.text = ("Please select any Objects" as string)
			HideRollouts()
		)
	)

	--Add\Remove rollouts with objects custom properties
	subRollout theSubRollout width:194 height:315 align:#center

	group "Learning Path"
	(
		button btnLearnF "Open Local Documentation" align:#center width:180 height:25
		button btnContactF "Contact with Developer" align:#center width:180 height:25
	)
	on GJ_Floater_UD close do (
		FDialogOpened = 0
		unRegisterRedrawViewsCallback showObjectNames
	)
)
createDialog GJ_Floater_UD pos:[-1000, -1000] --style:#(#style_border)
)

--Dagor start export fn
fn openMeasureFloater =
	(
		WM_SETREDRAW = 0xB
		WM_LBUTTONUP = 0x202
		WM_LBUTTONDOWN = 0x201

		local result = false
		local oldPannel = getCommandPanelTaskMode()

		commandHWND = windows.getChildHWND #max "Dagor Scene Export"
		if commandHWND == undefined do commandHWND = windows.getChildHWND 0P "Dagor Scene Export"

		windows.sendmessage commandHWND[1] WM_SETREDRAW 0 0

		UtilityPanel.OpenUtility Dagor_Scene_Export

		local theDagorSceneExporter = windows.getChildHWND #max "Dagor Scene Exporter"
		if theDagorSceneExporter == undefined do theDagorSceneExporter = windows.getChildHWND (windows.getDesktopHWND()) "Dagor Scene Export"

		if theDagorSceneExporter != undefined do (
       			local theButton = windows.getChildHWND theDagorSceneExporter[7] "Export DAG"

			if theButton != undefined and theButton.count > 0 do (
				Windows.sendMessage theButton[1] WM_LBUTTONDOWN 0 0
				Windows.sendMessage theButton[1] WM_LBUTTONUP 0 0
				result = true
			)
		)
		setCommandPanelTaskMode oldPannel
		windows.sendmessage commandHWND[1] WM_SETREDRAW 1 0
		return result
	)

--Global Working Parameters
global EscapeEnable = true

--GJ IO Macro
macroScript GJ_IO
buttontext:"Batch DAG Import\Export..."
category:"GJ_Tools"
IconName:"MainUI\Box"
(

try(destroyDialog GJ_Batch_Export)catch()
rollout GJ_Batch_Export "Dagor Custom Batch Exporter" width:200 height:591
(
	local theClasses = exporterPlugin.classes
	local The_INI_Path = ((GetDir #userStartupScripts) + "/gj_group_export_setting.ini")
	local TheChildren = #()
	local CollisionMeshes = #()
	local MeshesToSelect = #()
	local LinkedToDummy = False
	global TheSelection = #()
	local TheExportPath

	group "Geometry"(
		checkbox 'MoveToCenter' "Move to [0,0,0]" height:15 checked:true toolTip:"Move the objects to the origin before export, but keeps their current locations." align:#left
		checkbox 'ResetX' "Reset XForm" height:15 checked:true toolTip:"Reset the objects geometry before export.
		The original objects stay unchanged." align:#left
		checkbox 'ConvertToEditableMesh' "Convert to editable mesh" height:15 checked:true toolTip:"Export objects as editable meshes.
		This avoid some weird bugs sometimes^^
		The original objects stay unchanged." align:#left
		checkbox 'MergeAllNodes' "Merge All Nodes" height:15 checked:true align:#left tooltip:"Merge objects parented to a point helper into a single mesh before exporting."
	)
	group "General"(
		dropdownList 'ExportFormat' "Format" height:40 items:#("Dagor") align:#left
		dropdownList 'ExportPaths' "Path" height:40 toolTip:"Right click to open selected folder ;)" align:#left --enabled:false
		button 'Browse' "... Export PATH" align:#center width:180 height:25 toolTip:"Browse for export folder.
		Right click to purge list" align:#left
		button 'OpenMaxfileLocation' "Max Folder" align:#center width:180 height:25 toolTip:"Open current max file location"
		checkbox 'ExportToMaxfileLocation' "Export To Max File Location" height:15 toolTip:"Export objects into the folder where the current max file is" align:#left
		checkbox 'ShowPrompt' "Show prompt before export" height:15 checked:true toolTip:"Show dialog before export (depend on the format)" align:#left
	)
	group "Names Settings"(
		checkbox 'NameChange' "Change Names" checked:true height:15 toolTip:"Add the following strings to the exported objects name. The names of the objects in the scene stay unchanged." align:#left
		edittext 'prefix' "Prefix" height:17 align:#left
		edittext 'suffix' "Suffix" height:17 text:".lod00" align:#left
	)
	group "Collision Settings"(
		checkbox 'AddCollision' "Add Collision" height:15 toolTip:"Export each object combining all the meshes with the same name plus the following prefix." align:#left
		edittext 'UCXMeshName' "Collision Prefix" height:17 align:#left text:"Cls_"
	)

	button 'Export_Button' "Export Selection" align:#center width:180 height:25 toolTip:"Export selected objects."

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
		finalPath1 = finalPath1 + "documentation\custom-groups-export-tool.html "
		finalPath2 = finalPath2 + "documentation\custom-groups-export-tool.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/custom_groups_export_tool.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	fn GetINISettings = -- Get / save settings from .ini file
	(
		if (getfiles The_INI_Path).count != 0 then
		(
			Global DialogPositionExportTool = execute (getINISetting The_INI_Path "Dialog_Position" "DialogPositionExportTool")

			ExportPaths.items = execute (getINISetting The_INI_Path "Paths" "Export_Paths")
			if ExportPaths.items.count == 0 do
			(
				temp01 = GetDir #export
				insertItem temp01 ExportPaths.items 1
				ExportPaths.items = makeUniqueArray ExportPaths.items
-- 				ExportPathsPersistent = ExportPaths.items
				setINISetting (The_INI_Path) "Paths" "Export_Paths" (ExportPaths.items as string)
			)

			ExportFormat.selection = execute (getINISetting The_INI_Path "Format" "Selected_Format")
			prefix.text = getINISetting The_INI_Path "Texts" "Name_Prefix"
			suffix.text = getINISetting The_INI_Path "Texts" "Name_Suffix"
			UCXMeshName.text = getINISetting The_INI_Path "Texts" "Collision_Prefix"

			MoveToCenter.checked = execute (getINISetting The_INI_Path "Checkboxes" "MoveToCenter")
			ResetX.checked = execute (getINISetting The_INI_Path "Checkboxes" "ResetX")
			ConvertToEditableMesh.checked = execute (getINISetting The_INI_Path "Checkboxes" "ConvertToEditableMesh")
			NameChange.Checked = execute (getINISetting The_INI_Path "Checkboxes" "NameChange")
			AddCollision.checked = execute (getINISetting The_INI_Path "Checkboxes" "AddCollision")
			MergeAllNodes.checked = execute (getINISetting The_INI_Path "Checkboxes" "MergeAllNodes")
			ShowPrompt.checked = execute (getINISetting The_INI_Path "Checkboxes" "Show_Prompt")
			ExportToMaxfileLocation.checked = execute (getINISetting The_INI_Path "Checkboxes" "ExportToMaxfileLocation")
		)
		else
		(
			DialogPositionExportTool = GetDialogPos GJ_Batch_Export
			setINISetting (The_INI_Path) "Dialog_Position" "DialogPositionExportTool" (GetDialogPos GJ_Batch_Export as string)

			temp01 = GetDir #export
			insertItem temp01 ExportPaths.items 1
			ExportPaths.items = makeUniqueArray ExportPaths.items
			setINISetting (The_INI_Path) "Paths" "Export_Paths" (ExportPaths.items as string)

			setINISetting (The_INI_Path) "Format" "Selected_Format" (ExportFormat.selection as string)
			setINISetting (The_INI_Path) "Texts" "Name_Prefix" (prefix.text as string)
			setINISetting (The_INI_Path) "Texts" "Name_Suffix" (suffix.text as string)
			setINISetting (The_INI_Path) "Texts" "Collision_Prefix" (UCXMeshName.text as string)

			setINISetting (The_INI_Path) "Checkboxes" "MoveToCenter" (MoveToCenter.checked as string)
			setINISetting (The_INI_Path) "Checkboxes" "ResetX" (ResetX.checked as string)
			setINISetting (The_INI_Path) "Checkboxes" "ConvertToEditableMesh" (ConvertToEditableMesh.checked as string)
			setINISetting (The_INI_Path) "Checkboxes" "NameChange" (NameChange.checked as string)
			setINISetting (The_INI_Path) "Checkboxes" "AddCollision" (AddCollision.checked as string)
			setINISetting (The_INI_Path) "Checkboxes" "MergeAllNodes" (MergeAllNodes.checked as string)
			setINISetting (The_INI_Path) "Checkboxes" "Show_Prompt" (ShowPrompt.checked as string)
			setINISetting (The_INI_Path) "Checkboxes" "ExportToMaxfileLocation" (ExportToMaxfileLocation.checked as string)
		)
	)


	fn FindExporterClass TheClass =
	(
		Counter = 1
		for i in theClasses do
		(
			if i as string == TheClass then
			(
				return Counter
			)
			Counter += 1
		)
		if Counter == theClasses.count then return 0
	)


	fn GetExporterClass = -- Get exporter class corresponding to the selected format
	(
		case ExportFormat.selection of
		(
			1 : ExporterClass = FindExporterClass "FBXEXP"								--FBX
			2 : ExporterClass = FindExporterClass "ObjExp"								--OBJ
			3 : ExporterClass = FindExporterClass "3D_StudioExporterPlugin"		--3DS
			4 : ExporterClass = FindExporterClass "DWG_ExportExporterPlugin"	--DWG
			5 : ExporterClass = FindExporterClass "DWG_Export"						--DXF
			6 : ExporterClass = FindExporterClass "Alembic_Export"					--ABC
			7 : ExporterClass = FindExporterClass "ACIS_SAT"							--SAT
			8 : ExporterClass = FindExporterClass "DAEEXP"								--DAE
			9 : ExporterClass = FindExporterClass "IGES_Export"						--IGS
			10 : ExporterClass = FindExporterClass "SW3D_Exp"						--W3D
			11 : ExporterClass = FindExporterClass "OpenFltExport"					--FLT
			12 : ExporterClass = FindExporterClass "AsciiExp"							--ASE
			13 : ExporterClass = FindExporterClass "DWF_Exporter"					--DWF
			14 : ExporterClass = (-1)															--VRSCENE
			15 : ExporterClass = (-2)							--MAX
		)
		return ExporterClass
	)

	fn GetRoot obj = -- Get the root of obj
	(
		while obj.parent != undefined do obj = obj.parent
		return obj
	)

	fn SelectObjAndCollisionMeshes obj UcxMeshes InitialUCXPos =
	(
		if LinkedToDummy == True then
		(
			for i in CollisionMeshes do i.pos = obj.pos
			if MergeAllNodes.checked == False then select (CollisionMeshes + MeshesToSelect)
			else select (CollisionMeshes + obj)
		)
		else
		(
			ClearSelection()
			execute("select $" + UCXMeshName.text + obj.name + "*")
			for i in selection do
			(
				append InitialUCXPos i.pos
				i.pos = obj.pos
				append UcxMeshes i
			)
			select (UcxMeshes + obj)
		)
	)

	fn GetAllChildren obj TheChildren = -- Get all the children of obj and append them to TheChildren
	(
		if obj.children != undefined then
		for i in obj.children do
		(
			appendifunique TheChildren i
			GetAllChildren i TheChildren
		)
	)

	fn MergeChildren TheRoot CollisionMeshes = -- Merge all the children of TheRoot and return them as a collapsed mesh,  and store collision meshes in CollisionMeshes (snap collision pivot point on root)
	(
		free TheChildren
		free CollisionMeshes
		free MeshesToSelect
		MeshesToMerge = #()

		GetAllChildren TheRoot TheChildren

		for i in TheChildren do
		(
			if UCXMeshName.text != "" and matchPattern i.name pattern:(UCXMeshName.text + "*") then
			(
				if AddCollision.checked then
				(
					NewObj = snapshot i
					NewObj.name = i.name
					NewObj.parent = TheRoot
					NewObj.pivot = TheRoot.pos
					appendifunique CollisionMeshes NewObj
				)
			)
			else
			(
				if MergeAllNodes.checked == true then
				(
					if superClassOf i != GeometryClass or (superClassOf i == GeometryClass and findString i.name "_" == 1) then -- Ignore meshes with name starting with "_"
					(
						-- nothing ^^
					)
					else
					(
						NewObj = snapshot i
						NewObj.parent = none
						resetxform NewObj
						collapseStack NewObj
						if classOf NewObj != Editable_Poly then convertTo NewObj Editable_Poly
						append MeshesToMerge NewObj
					)
				)
				else
				(
					if findString i.name "_" != 1 then append MeshesToSelect i -- Ignore meshes with name starting with "_"
				)
			)
		)

		if MergeAllNodes.checked == true then
		(
			FirstObj = MeshesToMerge[1]

			for i = 2 to MeshesToMerge.count do
			(
				FirstObj.attach MeshesToMerge[i] FirstObj
			)
			FirstObj.parent = none
			FirstObj.pivot = TheRoot.pos
			FirstObj.transform = TheRoot.transform
			FirstObj.name = TheRoot.name
			return FirstObj
		)
		else
		(
			return TheRoot
		)
	)

	fn SortSelection SelectionList = -- If any of the selected objects is linked to a dummy or a point helper, then keep this helper only in the selection
	( -- if no parent or no helper then keep a copy of the selected objects in the selection
		local NewSelectionList = #()
		for i in SelectionList do
		(
			TheRoot = GetRoot i
			if classOf (TheRoot) == Dummy or classOf (TheRoot) == Point then appendifunique NewSelectionList TheRoot
			else
			(
				NewObj = copy i
				NewObj.name = i.name
				append NewSelectionList NewObj
			)
		)
		return NewSelectionList
	)


	fn AddFormatToName TheName = -- Adds the format at the end of the name to avoid bug when exporting object that contains a '.' in its name
	(
		TheName = (TheName + ".dag")
		return TheName
	)


	fn Export_Stuff obj ExporterClass = -- Make the actual export with the right name, format and collisions
	(
		if ExportToMaxfileLocation.checked then TheExportPath = maxFilePath
		else TheExportPath = ExportPaths.selected

		if AddCollision.checked then
		(
			UcxMeshes = #()
			InitialUCXPos = #()
			SelectObjAndCollisionMeshes obj UcxMeshes InitialUCXPos
		)
		else
		(
			if MergeAllNodes.checked == False and LinkedToDummy == True then
			/*if ExporterClass == -2 then
			(
				free TheChildren
				GetAllChildren obj TheChildren
				appendifunique TheChildren obj
				select TheChildren
			)
			else*/ select MeshesToSelect
			else select obj
		)
		if ExporterClass == -2 and ExportFormat.selection != 2 then
		(
			if NameChange.checked then TheObjName = (TheExportPath + "\\" + Prefix.text + obj.name + Suffix.text)
			else TheObjName = (TheExportPath + "\\" + obj.name)

			TheObjName = AddFormatToName TheObjName
			saveNodes $ TheObjName
		)
		else
		(
			if ExportCounter == 0 and ShowPrompt.checked then
			(
				if NameChange.checked then TheObjName = (TheExportPath + "\\" + Prefix.text + obj.name + Suffix.text)
				else TheObjName = (TheExportPath + "\\" + obj.name)
				TheObjName = AddFormatToName TheObjName
				--exportFile TheObjName selectedOnly:true using:theClasses[ExporterClass]
				UtilityPanel.OpenUtility Dagor_Scene_Export

				--export working only if Dagor Utility Panel opened
				anim_interval = interval 0 0
				dagorExport.Export TheObjName anim_interval selectedOnly:true suppressPrompts:false
				--print TheObjName
			)
			else
			(
				if NameChange.checked then TheObjName = (TheExportPath + "\\" + Prefix.text + obj.name + Suffix.text)
				else TheObjName = (TheExportPath + "\\" + obj.name)
				TheObjName = AddFormatToName TheObjName
				--exportFile TheObjName #noPrompt selectedOnly:true using:theClasses[ExporterClass]

				UtilityPanel.OpenUtility Dagor_Scene_Export
				anim_interval = interval 0 0
				dagorExport.Export TheObjName anim_interval selectedOnly:true suppressPrompts:false
				--print TheObjName
			)
		)
		if AddCollision.checked then
		(
			if LinkedToDummy then
			(
				for i = CollisionMeshes.count to 1 by -1 do delete CollisionMeshes[i]
			)
			else
			(
				for j = 1 to UcxMeshes.count do UcxMeshes[j].pos = InitialUCXPos[j]
			)
		)
		if MergeAllNodes.checked == True or LinkedToDummy == False then delete obj
	)


--Events
	on GJ_Batch_Export open do
	(
		windows.sendmessage ExportPaths.hwnd[1] 352 450 0
		GetINISettings()
		if DialogPositionExportTool != undefined then SetDialogPos GJ_Batch_Export DialogPositionExportTool

		if ExportFormat.selection == 14 then
		(
			AddCollision.enabled = false
			UCXMeshName.enabled = false
		)
		else
		(
			AddCollision.enabled = true
			UCXMeshName.enabled = true
		)

		if MergeAllNodes.checked == False then
		(
			ResetX.enabled = False
			ConvertToEditableMesh.enabled = False
		)
		else if MergeAllNodes.checked == True then
		(
			ResetX.enabled = True
			ConvertToEditableMesh.enabled = True
		)

		if ExportToMaxfileLocation.checked then
		(
			ExportPaths.enabled = false
			Browse.enabled = false
		)
		else
		(
			ExportPaths.enabled = true
			Browse.enabled = true
		)
	)
	on GJ_Batch_Export close do
	(
		setINISetting (The_INI_Path) "Dialog_Position" "DialogPositionExportTool" (GetDialogPos GJ_Batch_Export as string)
	)
	on MoveToCenter changed MoveToCenterState do
		setINISetting (The_INI_Path) "Checkboxes" "MoveToCenter" (MoveToCenterState as string)
	on ResetX changed ResetXState do
		setINISetting (The_INI_Path) "Checkboxes" "ResetX" (ResetXState as string)
	on ConvertToEditableMesh changed ConvertToEditableMeshState do
		setINISetting (The_INI_Path) "Checkboxes" "ConvertToEditableMesh" (ConvertToEditableMeshState as string)
	on MergeAllNodes changed MergeAllNodesState do
	(
		setINISetting (The_INI_Path) "Checkboxes" "MergeAllNodes" (MergeAllNodesState as string)
		if MergeAllNodesState == False then
		(
			ResetX.enabled = False
			ConvertToEditableMesh.enabled = False
		)
		else if MergeAllNodesState == True then
		(
			ResetX.enabled = True
			ConvertToEditableMesh.enabled = True
		)
	)
	on NameChange changed NameChangeState do
		setINISetting (The_INI_Path) "Checkboxes" "NameChange" (NameChangeState as string)
	on prefix changed prefixText do
		setINISetting (The_INI_Path) "Texts" "Name_Prefix" (prefixText as string)
	on suffix changed suffixText do
		setINISetting (The_INI_Path) "Texts" "Name_Suffix" (suffixText as string)
	on AddCollision changed AddCollisionState do
		setINISetting (The_INI_Path) "Checkboxes" "AddCollision" (AddCollisionState as string)
	on UCXMeshName changed UCXMeshNameText do
		setINISetting (The_INI_Path) "Texts" "Collision_Prefix" (UCXMeshNameText as string)
	on ExportFormat selected ExportFormatSelection do
	(
		setINISetting (The_INI_Path) "Format" "Selected_Format" (ExportFormatSelection as string)
		if ExportFormat.selection == 14 then
		(
			AddCollision.enabled = false
			UCXMeshName.enabled = false
		)
		else
		(
			AddCollision.enabled = true
			UCXMeshName.enabled = true
		)
	)
	on ExportPaths selected ExportPathsSelection do
	(
		insertItem ExportPaths.selected ExportPaths.items 1
		ExportPaths.items = makeUniqueArray ExportPaths.items
		ExportPaths.selection = 1
		setINISetting (The_INI_Path) "Paths" "Export_Paths" (ExportPaths.items as string)
	)
	on ExportPaths rightClick do -- Open the selected path in windows explorer
	(
		if ExportPaths.selected != undefined do shellLaunch "explorer.exe" ExportPaths.selected
	)
	on Browse pressed do
	(
		if ExportPaths.selected != undefined then Global ExportPath = getSavepath initialDir:ExportPaths.selected
		else Global ExportPath = getSavepath initialDir:(GetDir #export)
		if ExportPath != undefined do
		(
			insertItem ExportPath ExportPaths.items 1
			ExportPaths.items = makeUniqueArray ExportPaths.items
			setINISetting (The_INI_Path) "Paths" "Export_Paths" (ExportPaths.items as string)
		)
	)
	on Browse rightClick do
	(
		if QueryBox "Are you sure you want to remove the selected path from the list ?" then
		(
			ExportPaths.items = deleteItem ExportPaths.items 1
			setINISetting (The_INI_Path) "Paths" "Export_Paths" (ExportPaths.items as string)
		)
	)
	on ExportToMaxfileLocation changed ExportToMaxfileLocation_State do
	(
		setINISetting (The_INI_Path) "Checkboxes" "ExportToMaxfileLocation" (ExportToMaxfileLocation_State as string)
		if ExportToMaxfileLocation.checked then
		(
			ExportPaths.enabled = false
			Browse.enabled = false
		)
		else
		(
			ExportPaths.enabled = true
			Browse.enabled = true
		)
	)
	on OpenMaxfileLocation pressed do
	(
		if maxFilePath != undefined and maxFilePath != "" then
		(
			TheCompleteMaxFilePath = maxFilePath + maxFileName
			shellLaunch "explorer.exe" ("/select, " + TheCompleteMaxFilePath as string)
		)
		else messageBox "No max file currently opened! Please save scene before use this feature!"
	)
	on Export_Button pressed do
	(
		undo off
		(
			max create mode

			if ExportToMaxfileLocation.checked then if maxFilePath == undefined or maxFilePath == "" then
			(
				messageBox "No max file currently opened! Please save scene before use this feature!"
				return 0
			)
			if selection.count == 0 then
			(
				if TheSelection == undefined or TheSelection.count == 0 then
				(
					MessageBox "Selection is empty! Please select any Object with triangles!"
					return 0
				)
				else
				(
					TheSelection = for node in TheSelection where isvalidnode node collect node -- Remove any deleted scene objects from TheSelection

					local SelectedObj = #()
					if TheSelection.count > 0 then SelectedObj = SortSelection TheSelection
					else
					(
						MessageBox "Selection is empty! Please select any Object with triangles!"
						return 0
					)
				)
			)
			else
			(
				TheSelection = selection as array
				local SelectedObj = SortSelection (selection as array)
			)

			if ExportPaths.selected == undefined then
			(
				if ExportToMaxfileLocation.checked then ( /*nothing ^^*/)
				else
				(
					MessageBox "Please select an export path."
					return 0
				)
			)

			global ExportCounter = 0
			global ExporterClass

			ExporterClass = GetExporterClass()

			if ExporterClass == undefined or ExporterClass == 0 then
			(
				MessageBox "Export plugin not found!"
				return 0
			)

			-- VRSCENE

			if ExporterClass == (-1) then
			(
				if ExportToMaxfileLocation.checked then TheExportPath = maxFilePath
				else TheExportPath = ExportPaths.selected

				local UnHiddenObj = #()

				for o in objects do -- Make any visible object hidden and append them in UnHiddenObj
				(
					if o.isHidden == false then
					(
						append UnHiddenObj o
						o.isHidden = true
					)
				)

				ParentedToDummy = False
				for i in SelectedObj do -- VRSCENE export loop
				(
					if classOf i == Dummy or classOf i == Point then
					(
						ParentedToDummy = True
						free TheChildren
						GetAllChildren i TheChildren
						TheNewRoot = copy i
						TheNewRoot.name = i.name
						i = TheNewRoot
						for j in TheChildren do
						(
							NewObj = copy j
							NewObj.parent = i
						)
						free TheChildren
						GetAllChildren i TheChildren -- Fill TheChildren with the copies
					)
					i.isHidden = false -- Unhide i
					if MoveToCenter.Checked then i.pos = [0,0,0]
					if ResetX.checked then
					(
						if ParentedToDummy then
						(
							for j in TheChildren do
							(
								resetxform j
								collapseStack j
							)
						)
						else
						(
							resetxform i
							collapseStack i
						)
					)
					if ConvertToEditableMesh.Checked then
					(
						if ParentedToDummy then
						(
							for j in TheChildren do convertToMesh j
						)
						else convertToMesh i
					)

					if ParentedToDummy then
					(
						for j in TheChildren do j.isHidden = False
					)

					if NameChange.checked then TheObjName = (TheExportPath + "\\" + Prefix.text + i.name + Suffix.text)
					else TheObjName = (TheExportPath + "\\" + i.name)

					TheObjName = AddFormatToName TheObjName
					vrayExportRTScene TheObjName

					if ParentedToDummy then
					(
						for j in TheChildren do delete j
						delete i
					)
					else delete i

					ExportCounter += 1
				)

				for o in UnHiddenObj do -- Makes everything Unhidden
				(
					if isValidNode o then o.isHidden = false
				)


				max modify mode
				ClearSelection()
				select TheSelection
				return 0
			)
			-- End VRSCENE

			ParentedToDummy = False
			for obj in SelectedObj do -- Main export loop
			(
				if classOf obj == Dummy or classOf obj == Point then
				(
					obj = MergeChildren obj CollisionMeshes
					LinkedToDummy = True
				)
				else LinkedToDummy = False
				if MoveToCenter.Checked then
				(
					if MergeAllNodes.checked then obj.pos = [0,0,0]
					else
					(
						local InitialPos = obj.pos
						obj.pos = [0,0,0]
					)
				)

				if ResetX.checked and (MergeAllNodes.checked or LinkedToDummy == False) then
				(
					resetxform obj
					collapseStack obj
				)
				if ConvertToEditableMesh.Checked and (MergeAllNodes.checked or LinkedToDummy == False) then convertToMesh obj

				Export_Stuff obj ExporterClass
				ExportCounter += 1

				if MoveToCenter.Checked and MergeAllNodes.checked == False and LinkedToDummy then obj.pos = InitialPos
			)

			ClearSelection()
			select TheSelection
			max modify mode
		)
	)
	on ShowPrompt changed ShowPromptState do setINISetting (The_INI_Path) "Checkboxes" "Show_Prompt" (ShowPromptState as string)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_Batch_Export open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_Batch_Export ((getINISetting INIfile "Batch_Export" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_Batch_Export moved new_pos do
	(
		try(setINISetting INIfile "Batch_Export" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_Batch_Export close do
	(
		try(setINISetting INIfile "Batch_Export" "DialogPos" ((GetDialogPos GJ_Batch_Export) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
CreateDialog GJ_Batch_Export style:#(#style_titlebar, #style_sysmenu, #style_toolwindow)
)

--GJ Icons Floater
macroScript GJ_Floater
buttontext:"Icons Floater..."
category:"GJ_Tools"
IconName:"MainUI\Align"
(
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
--cui.dockDialogBar GJ_Toolbar #cui_dock_all

)

--GJ LOD Generator
macroScript GJ_LODGenerator
buttontext:"LOD Generator..."
category:"GJ_Tools"
IconName:"MainUI\ToggleSceneExplorer"
(
try(DestroyDialog ::GJ_LOD_Gen)catch()
rollout GJ_LOD_Gen "Dagor LOD Generator" width:200
(
	group "Engine Optimization Settings"
	(
		dropDownList EngineType "Engine Optimization Type" width:120 height:40 align:#left items:#("MultiRes", "ProOptimizer") selection:2
		checkbox ProtectBorders "Protect Borders" checked:true enabled:true
		checkbox ProtectUV "Keep UV Bondaries" checked:true enabled:true
	)
	group "Selected Objects Parameters"
	(
		spinner LodPercent "Vertex %" range:[1,100,50] fieldwidth:50 scale: 5
		spinner MResIt "Iterations" range:[1,20,10] fieldwidth:50 type:#integer
		spinner LODNum "Number of LODs" range:[0,99,1] fieldwidth:50 type:#integer
		progressBar pbProgressBarLoc "ExportProgress" width:180 height:10 color:(color 0 132 255) align:#center value:50
		progressBar pbProgressBar "ExportProgressTotal" width:180 height:10 color:(color 100 180 0) align:#center value:50
		button btnGo "Build LODs!" align:#center width:180 height:23

	)
	group "Advanced Options"
	(
		checkbox ColOpt "Generate Collision" checked:true enabled:true
		checkbox ColCutOpt "Collision Cut From Top" checked:false enabled:true
		checkbox ColKeepMask "Collision Use Name Mask" checked:true enabled:true
		checkbox ColSmParts "Collision Remove Small Parts" checked:false enabled:true
		spinner ColPercent "Collision Vertex %" range:[1,100,30] fieldwidth:50
		spinner ColSizePercent "Collision Part Size %" range:[1,100,40] fieldwidth:50 enabled:true
		spinner ColCutPercent "Collision Cut Height" range:[0.1,10000,3000] fieldwidth:50
		editText ColMaskProp "Keep Material Name" fieldWidth:180 height:21 align:#center text:"*bark*,*stone,brick*" labelOnTop:true
		editText LODCstProp "LODs Custom Properties" fieldWidth:180 height:40 align:#center text:"renderable:b=yes\ncollidable:b=no\n" labelOnTop:true
		editText ColCstProp "Collision Custom Properties" fieldWidth:180 height:40 align:#center text:"renderable:b=no\ncollidable:b=yes\ncollision:t=\"mesh\"\nphmat:t=\"wood_solid\"\nisPhysCollidable:b=yes\nisTraceable:b=yes" labelOnTop:true
		button btnSaveSettings "Save Settings" align:#center width:180 height:25
	)
	group "Learning Path"
	(
		button btnLearnF "Open Local Documentation" align:#center width:180 height:25
		button btnContactF "Contact with Developer" align:#center width:180 height:25
	)

	fn engine_states = (
		if EngineType.selection == 1 then (
			ProtectBorders.enabled = false
			ProtectUV.enabled = false
		)
		if EngineType.selection == 2 then (
			ProtectBorders.enabled = true
			ProtectUV.enabled = true
		)
	)

	on EngineType selected ret do (
		engine_states()
	)

	--// Top level Locals for proper detach
	local srcObjs = #() -- source objs
	local tmpObjs = #() -- temporary
	local endObjs = #() -- for conv2Mesh
	local eleGroups = #() -- for UnGrouping
	--// Functions
	fn DetachToElements obj cPivot &endObjs num:1 = (
		while obj.getNumFaces() != 0 do (
			polyop.setFaceSelection obj #{1}
			obj.selectElement()
			ele = polyop.getFaceSelection obj
			newName = (num as string)
			num += 1 -- pump up counter
			polyop.detachFaces obj ele asNode:true name:newName
			newObj = getNodeByName newName
			append endObjs newObj
			attachObjects obj newObj
		)
	)
	mapped fn renameChildren ch = (
		ch.name = ch.parent.name + ch.name
	)

	--// this function must be here! (ie after UI def.)
	fn runEntirely = (
		local total = (tmpObjs.count as string) -- total objects
		for i = 1 to tmpObjs.count do (
			DetachToElements tmpObjs[i] 1 endObjs
			grp = group tmpObjs[i].children name:(srcObjs[i].name + "_partagass")
			append eleGroups grp -- for UnGrouping ...
			attachObjects srcObjs[i] grp move:false -- link G to source obj.
			renameChildren grp.children -- rename
		)
		eleGroups = for g in eleGroups where isValidNode g collect g -- important!
		ungroup eleGroups -- now ungroup all at once
		delete srcObjs
	)

	--main loop
	on btnGo pressed do
	(
		--disableSceneRedraw()
		max modify mode

		local p = 1
		local ObjSel = selection as array
		local totalSteps = 0

		--very crappy code but works for total step computation
		local nrm = 0
		for i = 1 to (LODNum.value as integer) do (
			for r = 1 to i do (
				for a = 1 to MResIt.value do (
					nrm = nrm + 1
				)
			)
		)

		totalSteps = MResIt.value
		totalStepsTot = ObjSel.count * LODNum.value
		local u = 0
		pbProgressBar.value = 0

		if ObjSel.count > 0  then ( -- check selection
			for baseObj in ObjSel do (
				if LODNum.value > 0 then (
					for i = 1 to (LODNum.value as integer) do (
						lodObj = instance baseObj -- important for GrowFX because "copy" broken some elements

						-- important because MultiRes working proper only with EditPoly from max 2018
						convertToPoly lodObj

						-- important because work only with selected object in modify pannel
						select lodObj

						local objNFaces = lodObj.mesh.numfaces
						pbProgressBarLoc.value = 0

						local p = 0
						for r = 1 to i do (
							local currentPercent = 0;
							local currentVal  = 100
							local oneStep = 100.0*(100.0 - LodPercent.value)/(100.0*MResIt.value)
							for a = 1 to MResIt.value do (
								max modify mode -- open the Modify panel
								--multiRes mode
								if EngineType.selection == 1 then (
									modPanel.addModToSelection (MultiRes ())
									currentPercent = 100.0*(1 - oneStep / currentVal)
									currentVal = currentPercent * currentVal / 100.0
									lodObj.modifiers[1].vertexPercent = currentPercent
									lodObj.modifiers[1].reqGenerate = true
								)

								--ProOptimizer mode
								if EngineType.selection == 2 then (
									modPanel.addModToSelection (ProOptimizer ())
									currentPercent = 100.0*(1 - oneStep / currentVal)
									currentVal = currentPercent * currentVal / 100.0
									lodObj.modifiers[1].vertexPercent = currentPercent
									lodObj.modifiers[1].KeepUV = true
									lodObj.modifiers[1].KeepVC = true
									if ProtectBorders.checked == true then lodObj.modifiers[1].OptimizationMode = 1 else lodObj.modifiers[1].OptimizationMode = 0
									if ProtectUV.checked == true then lodObj.modifiers[1].LockUV = true else lodObj.modifiers[1].LockUV = false
									lodObj.modifiers[1].Calculate = true
								)
								convertToPoly lodObj

								p = p + 1
								pbProgressBarLoc.value = 100.000 / totalSteps * p
							)
						)

						local addNrm = i as string
						if i < 10 then addNrm = "0" + (i as string)
						lodObj.name = baseObj.name + ".lod" + addNrm

						-- delete objects with zero faces count
						if lodObj.mesh.numFaces == 0 then delete lodObj

					--add custom properties	to current LOD
					setUserPropBuffer lodObj ""
					local ParamArr = FilterString LODCstProp.text "=,\n"
					for i = 1 to ParamArr.count by 2 do (
						setUserProp lodObj ParamArr[i] ParamArr[i+1]
					)

					u = u + 1
					pbProgressBar.value = 100.000 / totalStepsTot * u
					)
				)

				--create collision if checked
				if ColOpt.checked then (

					--create Collision
					collObj = instance baseObj
					convertToPoly collObj
					select collObj

					--optimize collision
					pbProgressBarLoc.value = 0
					p = 0
					local currentVal  = 100
					local oneStep = 100.0*(100.0 - ColPercent.value)/(100.0*MResIt.value)
					for a = 1 to MResIt.value do (
						modPanel.addModToSelection (MultiRes ())
						local currentPercent = 100.0*(1 - oneStep / currentVal)
						currentVal = currentPercent * currentVal / 100.0
						collObj.modifiers[1].vertexPercent = currentPercent
						collObj.modifiers[1].reqGenerate = true
						convertToPoly collObj

						p = p + 1
						pbProgressBarLoc.value = 100.000 / totalSteps * p
					)
					dBox = box length:20 width:20 height:20
					convertToMesh dBox
					for a = 1 to (meshop.getNumVerts dBox)-1 do (
						meshop.deleteVerts dBox #{1}
					)
					meshop.deleteIsoVerts dBox
					convertToPoly dBox
					polyop.attach dBox collObj
					collObj = dBox
					select collObj

					--slice top of collision
					if ColCutOpt.checked == true then (

						--slice top of collision
						modPanel.addModToSelection (sliceModifier ())
						collObj.modifiers[1].Slice_Type = 2
						collObj.modifiers[1].Slice_Plane.pos = [0,0,ColCutPercent.value]
						convertToPoly collObj

						--cap holes of collision
						modPanel.addModToSelection (Cap_Holes ())
						collObj.modifiers[1].smooth = 1
						collObj.modifiers[1].sm_ends  = 1
						collObj.modifiers[1].vis_edges = 1
						convertToPoly collObj
					)

					--add custom properties	to collision
					setUserPropBuffer collObj ""
					local ParamArr = FilterString ColCstProp.text "=,\n"
					for z = 1 to ParamArr.count by 2 do (
						setUserProp collObj ParamArr[z] ParamArr[z+1]
					)

					--remove collision parts by material name mask
					if ColKeepMask.checked == true then (
						matFr = $.material
						--get class of object material top level material
						matFrClass = classof matFr
						--showProperties matFr
						local Ptext = ColMaskProp.text as string
						if matFrClass == Multimaterial then ( -- if Multimaterial we go...
							local paternArr = (FilterString Ptext " ,*,/" splitEmptyTokens:false)

							local matNameArr = matFr.materialList
							convertToPoly $
							local keepArr = #()
							--make array with preserved materials IDs by material patern name
							for a = 1 to paternArr.count do (
									for b = 1 to matNameArr.count do (
									MSubMatName = findString (matNameArr[b] as string) (paternArr[a] as string)
									if MSubMatName != undefined then (
										append keepArr (b as integer)
									)
								)
							)
							local allArr =#()
							for c = 1 to matNameArr.count do (
								append allArr (c as integer)
							)
							local delArr =#()
							for d = 1 to allArr.count do (
								dCand = 0
								for a = 1 to keepArr.count do (
									if keepArr[a] == allArr[d] then dCand = 1
								)
								if dCand == 0 then append delArr d
							)
							--delete face IDs whitout list of generated Array
							for a = 1 to delArr.count do (
								local DDArr =#()
								for f = 1 to (polyOp.getNumFaces $) do (
									local CFaceId = polyop.getFaceMatID $ f
									if CFaceId == delArr[a] then append DDArr f
								)
								polyop.deleteFaces $ DDArr delIsoVerts:false
							)
						)
						else
						(
							print "No Multimaterial on Based Object. Skiped..."
						)
					)

					--assign material
					--if ColdMat.checked == true then(
						local script_path = pathConfig.removePathLeaf (getSourceFileName())
						local pFile = script_path + "\DagorLibrary.mat"
						local pFileError = doesFileExist pFile
						if pFileError == false then (
							messageBox "DagorLibrary.mat not found!"
						)
						else
						(
							matLib = loadTempMaterialLibrary pFile
							$.material = matLib[1]--asign first material from lib
						)
					--)

					--remove small parts
					if ColSmParts.checked == true then (
						srcObjs = for x in selection where canConvertTo x Editable_Poly collect x
						max create mode
						numObjs = objects.count
						snapshot srcObjs -- make copies...
						tmpObjs = ( -- and collect 'em
							for xx = (numObjs + 1) to objects.count collect objects[xx]
						)
						convertTo tmpObjs Editable_Poly

						--// the "core" in Undo context
						runEntirely()
						delete tmpObjs -- KEEP this Out Off Undo context !!!
						endObjs = for x in endObjs where isValidNode x collect x -- important!
						--select endObjs

						--select decomposited objects
						select $*partagass*
						convertToMesh selection

						--find bigest bbox size in objects selection
						pcnBSize = ColSizePercent.value
						BsizeArr = #()
						bigestSize = 0
						for o in selection do (
							currentSize = (o.max - o.min)
							for f = 1 to currentSize.count do (
								if bigestSize < currentSize[f] then bigestSize = currentSize[f]
							)
						)

						--remove parts is smaler the percentage size
						local toDelete = #()
						for o in selection do (
							currentSize = (o.max - o.min)
							bSize = 0
							for f = 1 to currentSize.count do (
								if bSize < currentSize[f] then bSize = currentSize[f]
							)
							if (bigestSize / 100 * pcnBSize) > bSize then o.name = o.name + "_to_delete_thiss_objj"
						)
						--delete small objects
						select $*_to_delete_thiss_objj*
						delete selection

						--attach result into single object
						select $*partagass*
						a = selection as array
						convertToMesh selection
						for k = 2 to a.count do (
							attach a[1] a[k]
						)
						convertToPoly a[1]
						--a[1].name = baseObj.name + "collision"
						--select a[1]
					)

					--rename
					$.name = baseObj.name + ".collision"
				)
			)
		)
		else
		(
			messageBox "Please Select Any Edit Poly\Mesh Object"
		)
		EnableSceneRedraw()
		CompleteRedraw()
	)

	--on ColKeepMask changed ColKeepMask_st do (if ColKeepMask_st == true then ColSmParts.checked = false else ColSmParts.checked = true)

	--on ColSmParts changed ColSmParts_st do (if ColSmParts_st == true then ColKeepMask.checked = false else ColKeepMask.checked = true)

	--learn
	on btnLearnF pressed do (
		local pathArr = filterString (pathConfig.removePathLeaf(getSourceFileName()) as string) "\\"
		local finalPath1 = ""
		local finalPath2 = ""
		for t = 1 to pathArr.count do (
			finalPath2 = finalPath2 + pathArr[t] + "\\"
			if t <= pathArr.count-1 then finalPath1 = finalPath1 + pathArr[t] + "\\"
		)
		finalPath1 = finalPath1 + "documentation\lods-and-collisions-generator.html "
		finalPath2 = finalPath2 + "documentation\lods-and-collisions-generator.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/lods_and_collisions_generator.html" ""
	)

	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	-----------------------------save and load settings-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_LOD_Gen.ini"
	fn encode_buffer txt = (
		local txt_enc = ""

		return txt_enc
	)

	fn load_settings = (
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_LOD_Gen ((getINISetting INIfile "GJ_LOD_Gen" "DialogPos")as Point2)

			EngineType.selection = ((getINISetting INIfile "GJ_LOD_Gen" "EngineType")as integer)

			ProtectBorders.checked = ((getINISetting INIfile "GJ_LOD_Gen" "ProtectBorders")as booleanClass)
			ProtectUV.checked = ((getINISetting INIfile "GJ_LOD_Gen" "ProtectUV")as booleanClass)
			ColOpt.checked = ((getINISetting INIfile "GJ_LOD_Gen" "ColOpt")as booleanClass)
			ColCutOpt.checked = ((getINISetting INIfile "GJ_LOD_Gen" "ColCutOpt")as booleanClass)
			ColKeepMask.checked = ((getINISetting INIfile "GJ_LOD_Gen" "ColKeepMask")as booleanClass)
			ColSmParts.checked = ((getINISetting INIfile "GJ_LOD_Gen" "ColSmParts")as booleanClass)

			ColMaskProp.text = (substituteString (getINISetting INIfile "GJ_LOD_Gen" "ColMaskProp") "|" "\n")
			LODCstProp.text = (substituteString (getINISetting INIfile "GJ_LOD_Gen" "LODCstProp") "|" "\n")
			ColCstProp.text = (substituteString (getINISetting INIfile "GJ_LOD_Gen" "ColCstProp") "|" "\n")

			LodPercent.range = ((getINISetting INIfile "GJ_LOD_Gen" "LodPercent") as point3)
			ColPercent.range = ((getINISetting INIfile "GJ_LOD_Gen" "ColPercent") as point3)
			ColSizePercent.range = ((getINISetting INIfile "GJ_LOD_Gen" "ColSizePercent") as point3)
			ColCutPercent.range = ((getINISetting INIfile "GJ_LOD_Gen" "ColCutPercent") as point3)

			MResIt.range = ((getINISetting INIfile "GJ_LOD_Gen" "MResIt") as point3)
			LODNum.range = ((getINISetting INIfile "GJ_LOD_Gen" "LODNum") as point3)

		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
		engine_states()
	)
	fn save_settings = (
		try(setINISetting INIfile "GJ_LOD_Gen" "EngineType" ((EngineType.selection) as string) forceUTF16:false)catch()

		try(setINISetting INIfile "GJ_LOD_Gen" "ProtectBorders" ((ProtectBorders.checked) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ProtectUV" ((ProtectUV.checked) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ColOpt" ((ColOpt.checked) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ColCutOpt" ((ColCutOpt.checked) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ColKeepMask" ((ColKeepMask.checked) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ColSmParts" ((ColSmParts.checked) as string) forceUTF16:false)catch()

		try(setINISetting INIfile "GJ_LOD_Gen" "ColMaskProp" ((substituteString ColMaskProp.text "\n" "|") as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "LODCstProp" ((substituteString LODCstProp.text "\n" "|") as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ColCstProp" ((substituteString ColCstProp.text "\n" "|") as string) forceUTF16:false)catch()

		try(setINISetting INIfile "GJ_LOD_Gen" "LodPercent" ((LodPercent.range) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ColPercent" ((ColPercent.range) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ColSizePercent" ((ColSizePercent.range) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "ColCutPercent" ((ColCutPercent.range) as string) forceUTF16:false)catch()

		try(setINISetting INIfile "GJ_LOD_Gen" "MResIt" ((MResIt.range) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "GJ_LOD_Gen" "LODNum" ((LODNum.range) as string) forceUTF16:false)catch()
	)

	on GJ_LOD_Gen open do
	(
		load_settings()
    )
 	--write to ini file if dialog moved or closed
	on GJDamagesSetup moved new_pos do
	(
		try(setINISetting INIfile "GJ_LOD_Gen" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
	--write to ini file if dialog moved or closed
    on GJ_LOD_Gen close do
	(
		save_settings()
		try(setINISetting INIfile "GJ_LOD_Gen" "DialogPos" (GetDialogPos GJ_LOD_Gen as string) forceUTF16:false)catch()
    )

	on btnSaveSettings pressed do (
		save_settings()
	)
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_LOD_Gen
)

--Illegal Char Finder
macroScript GJFiGliff
buttontext:"GJFixIllegalCharacters..."
category:"GJ_Tools"
--IconName:"MainUI\ToggleSceneExplorer"
IconName:"MainUI\CurveEditorOpen"
(
	global ObjNameBase
	global ObjCount
	global MainObj

if GJFiGliff !=undefined do destroyDialog GJFiGliff
rollout GJFiGliff "Dagor Fixing Illegal Characters" width:260
	(
		group "1. Illegal Characters List"
		(
			editText ChrBad "" fieldWidth:240 height:100 align:#center text:" :;'@#~[{}]|!£$%^&*()-=+\\,<.>//?\'абвгдеё\nжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗ\nИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" labelOnTop:true
		)
		group "2. Settings"
		(
			editText ChrRepl "Replace with Characters:" fieldWidth:100 align:#left text:"_"
			checkbox RusTrans "Latinization of the Russian" align:#left checked:true enable:true
			checkbox ChPathName "Log Warning Bitmap Path Name" align:#left checked:true enable:true
			checkbox ChTexName "Log Warning Texture Maps Names" align:#left checked:true enable:true
			checkbox FixLayerName "Fix Layers Names" align:#left checked:true enable:true
			checkbox FixObjName "Fix Objects Names" align:#left checked:true enable:true
			checkbox FixMatName "Fix Materials Names" align:#left checked:true enable:true
			checkbox FixToLow "All Changes to Lower Case" align:#left checked:false enable:true
			checkbox DoExport "DAG Export After Fixing Illegal Characters" align:#left checked:false enable:true

			button SaveSet "Save Current Settings" width:200 align:#center
			button LoadSet "Load Default Settings" width:200 align:#center
		)
		group "3. Pick and Fix Illegal Characters"
		(
			button pick_source "FIX ILLEGAL CHARACTERS FROM SELECTION!" width:240 align:#center
		)
		group "Progress Bar"
		(
			--editText ChrLog "" fieldWidth:240 height:100 align:#center text:"Logging started...\n" labelOnTop:true
			progressBar pbProgressBar "TotalExportProgress" width:250 height:16 color:(color 190 0 0) align:#center
		)

		group "Learning Path"
		(
			button btnLearn "Open Local Documentation" width:200 height:30 align:#center
			button btnAut "Get in Touch with the Author" width:200 height:30 align:#center
		)

		--learn
		on btnLearn pressed do (
				local pathArr = filterString (pathConfig.removePathLeaf(getSourceFileName()) as string) "\\"
			local finalPath1 = ""
			local finalPath2 = ""
			for t = 1 to pathArr.count do (
				finalPath2 = finalPath2 + pathArr[t] + "\\"
				if t <= pathArr.count-1 then finalPath1 = finalPath1 + pathArr[t] + "\\"
			)
			finalPath1 = finalPath1 + "documentation\fixing-illegal-characters.html "
			finalPath2 = finalPath2 + "documentation\fixing-illegal-characters.html "

			ShellLaunch finalPath1 ""
			ShellLaunch finalPath2 ""
			ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/fixing_illegal_characters.html" ""
		)

		on btnAut pressed do (
			ShellLaunch "https://github.com/VlasovAlexey " ""
		)

		--Write Ini file function
		fn WriteIni = (
			local IniFileName = (GetDir #userStartupScripts) + "/gj_tools.ini"

			--ini file not exist create default settings

			local IniFile = Createfile IniFileName encoding: #utf8

			format "%\n" (substituteString ChrBad.text "\n" "")  to:IniFile
			format "%\n" (substituteString ChrRepl.text "\n" "")  to:IniFile
			format "%\n" RusTrans.checked  to:IniFile
			format "%\n" RusTrans.enabled  to:IniFile
			format "%\n" ChPathName.checked  to:IniFile
			format "%\n" ChPathName.enabled  to:IniFile
			format "%\n" ChTexName.checked  to:IniFile
			format "%\n" ChTexName.enabled  to:IniFile
			format "%\n" FixLayerName.checked  to:IniFile
			format "%\n" FixLayerName.enabled  to:IniFile
			format "%\n" FixObjName.checked  to:IniFile
			format "%\n" FixObjName.enabled  to:IniFile
			format "%\n" FixMatName.checked  to:IniFile
			format "%\n" FixMatName.enabled  to:IniFile
			format "%\n" FixToLow.checked  to:IniFile
			format "%\n" FixToLow.enabled  to:IniFile
			format "%\n" DoExport.checked  to:IniFile
			format "%\n" DoExport.enabled  to:IniFile

			close IniFile

			return IniFile
		)

		--save settings
		on SaveSet pressed do
		(
			WriteIni()
		)

		--Load default settings
		on LoadSet pressed do
		(
			ChrBad.text = "\"№ :;'@#~[{}]|!£$%^&*()-=+\\,<.>//?\'абвгдеё\nжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗ\nИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
			ChrRepl.text = "_"
			RusTrans.checked = true
			RusTrans.enabled = true
			ChPathName.checked = true
			ChPathName.enabled = true
			ChTexName.checked = true
			ChTexName.enabled = true
			FixLayerName.checked = true
			FixLayerName.enabled = true
			FixObjName.checked = true
			FixObjName.enabled = true
			FixMatName.checked = true
			FixMatName.enabled = true
			FixToLow.checked = false
			FixToLow.enabled = true
			DoExport.checked = false
			DoExport.enabled = true
		)

			--First start save setings procedure after firs open GUI
		on GJFiGliff open do
		(
			local IniFileName = (GetDir #userStartupScripts) + "/gj_tools.ini"
			if ((getfiles IniFilename).count == 0) then
			(
				--ini file not exist create default setting
				WriteIni()
			)
			else
			(
				-- ini file exist and read settings from it
				IniFile = openfile IniFileName

				ChrBad.text = ReadLine IniFile
				ChrRepl.text = ReadLine IniFile
				RusTrans.checked = ReadValue IniFile
				RusTrans.enabled = ReadValue IniFile
				ChPathName.checked = ReadValue IniFile
				ChPathName.enabled = ReadValue IniFile
				ChTexName.checked = ReadValue IniFile
				ChTexName.enabled = ReadValue IniFile
				FixLayerName.checked = ReadValue IniFile
				FixLayerName.enabled = ReadValue IniFile
				FixObjName.checked = ReadValue IniFile
				FixObjName.enabled = ReadValue IniFile
				FixMatName.checked = ReadValue IniFile
				FixMatName.enabled = ReadValue IniFile
				FixToLow.checked = ReadValue IniFile
				FixToLow.enabled = ReadValue IniFile
				DoExport.checked = ReadValue IniFile
				DoExport.enabled = ReadValue IniFile

				close IniFile
			)
		)

		--to lower case fn
		fn LowerCaseNm ObjNm = (
			local ObjNewNm = ObjNm
			ObjNewNm = toLower ObjNm

			return ObjNewNm
		)

		--replace char fn
		fn RepChar ObjNm illegalChr RChar = (

			local ObjNewNm = ObjNm
			for f = 1 to ObjNm.count do (
				for z = 1 to illegalChr.count do (
					local result = findString ObjNm illegalChr[z]
					if result != undefined do(
						ObjNewNm = replace ObjNm result 1 RChar
						if ObjNewNm != ObjNm then ObjNm = ObjNewNm
					)
				)
			)

			--lower case
			if FixToLow.checked == true  do(
				ObjNewNm = LowerCaseNm ObjNewNm
			)
			return ObjNewNm
		)

		--make latinic all russian chars fn
		fn LatinChar ObjNm = (
			local ObjNewNm = ObjNm
			local RusLng = #("а","б","в","г","д","е","ё","ж","з","и","й","к","л","м","н","о","п","р","с","т","у","ф","х","ц","ч","ш","щ","ъ","ы","ь","э","ю","я",
			"А","Б","В","Г","Д","Е","Ё","Ж","З","И","Й","К","Л","М","Н","О","П","Р","С","Т","У","Ф","Х","Ц","Ч","Ш","Щ","Ъ","Ы","Ь","Э","Ю","Я")
			local EngLng = #("a","b","v","g","d","e","ye","zh","z","i","y","k","l","m","n","o","p","r","s","t","u","f","kh","ts","ch","sh","shch","\"","y","`","e","yu","ya",
			"A","B","V","G","D","E","Ye","Zh","Z","I","Y","K","L","M","N","O","P","R","S","T","U","F","Kh","Ts","Ch","Sh","Shch","\"","y","`","E","Yu","Ya")

			local illegalChr = RusLng

			for f = 1 to ObjNm.count do (
				for z = 1 to illegalChr.count do (

					local result = undefined
					local ChrToCompareIndex = undefined

					ChrToCompareIndex = findString ObjNm illegalChr[z]
					if ChrToCompareIndex != undefined do(
						if (matchPattern ObjNm[ChrToCompareIndex] pattern:(illegalChr[z] as string) ignoreCase:false) == true do (
							result = findString ObjNm illegalChr[z]
						)
					)

					local ReplaceCharIndex = undefined
					if result != undefined do (
						--find illegalChars in russian array
						for r = 1 to RusLng.count do (
							if (matchPattern (illegalChr[z] as string) pattern:(RusLng[r] as string) ignoreCase:false) == true do (
							--if illegalChr[z] == RusLng[r] then (
								ReplaceCharIndex = r
								--exit
							)
						)

						ObjNewNm = replace ObjNm result 1 EngLng[ReplaceCharIndex]
						if ObjNewNm != ObjNm then ObjNm = ObjNewNm
					)
				)
			)

			--lower case
			if FixToLow.checked == true  do(
				ObjNewNm = LowerCaseNm ObjNewNm
			)
			return ObjNewNm
		)

		-- Main loop
		on pick_source pressed do (

			--disableSceneRedraw()

			local ObjSel = selection as array

			local i = 1
			local ObjCount = 0

			local ObjError = false
			ChrLog = "Logging started...\n"

			--fix layers names
			if FixLayerName.checked == true do(
				ChrLog = ChrLog + "Check layers names...\n"
				local illegalChars = ChrBad.text
				local ReplaceChar = ChrRepl.text
				for i = 0 to layerManager.count-1 do (
					ilayer = layerManager.getLayer i
					LayerNm = ilayer.name
					local LayerNewNm = LayerNm

					if RusTrans.checked == false then(
						-- latinisation is off
						LayerNewNm = RepChar LayerNm illegalChars ReplaceChar
					)
					else
					(
						LayerNewNm = LatinChar LayerNm
						LayerNewNm = RepChar LayerNewNm illegalChars ReplaceChar
					)

					--print LayerNm
					--print LayerNewNm

					--replace new name
					if LayerNm != LayerNewNm and LayerNm != "0" then (
						ObjError = true
						RenResult = (layerManager.getLayer i).setName LayerNewNm
						--print RenResult
						if RenResult == false then (
							ChrLog = ChrLog + "| ERROR! This layer name already exist! Layer name: " + LayerNm + " can`t be changed to: " + LayerNewNm + " Please change it manual! |\n"
						)
						else(
							ChrLog = ChrLog + "| ERROR! | Layer name: | " + LayerNm + "| changed to: | " + LayerNewNm + "| \n"
						)
						--ilayer.name = LayerNewNm
						--RNameLayer = LayerManager.getlayer i
						--RNameLayer.setName = (LayerNewNm as string)
					)

					--format "Layer: %; nodes: %\n" layerName layerNodes

					pbProgressBar.value = 100.000 / sceneMaterials.count * i
					i = i + 1
					)
				)
				i = 1


			--fix objects naming
			if FixObjName.checked == true do(
				ChrLog = ChrLog + "Checking object names...\n"
				for MainObj in ObjSel do (
					windows.processPostedMessages()
					local ObjName = MainObj.name as string
					local illegalChars = ChrBad.text
					local ReplaceChar = ChrRepl.text
					local ObjNewName = ObjName

					if RusTrans.checked == false then(
						-- latinisation is off
						ObjNewName = RepChar ObjName illegalChars ReplaceChar
					)
					else
					(
						ObjNewName = LatinChar ObjName
						ObjNewName = RepChar ObjNewName illegalChars ReplaceChar
					)

					--replace new name
					if MainObj.name != ObjNewName then(
						ObjError = true
						--ChrLog.text = ChrLog.text + "ERROR! | Object name: " + MainObj.name + " | changed to: " + ObjNewName + "\n"
						ChrLog = ChrLog + "| ERROR! | Object name: " + MainObj.name + " | changed to: | " + ObjNewName + "\n"

						MainObj.name = ObjNewName
					)
					pbProgressBar.value = 100.000 / ObjSel.count * i
					i = i + 1
				)
				i = 1
			)

			--check and warning if need Materials names
			if FixMatName.checked == true do(
				ChrLog = ChrLog + "Check material names...\n"

				local illegalChars = ChrBad.text
				local ReplaceChar = ChrRepl.text

				for mat in sceneMaterials do (
					windows.processPostedMessages()

					local NMat = mat.name as string

					local illegalChars = ChrBad.text
					local ReplaceChar = ChrRepl.text
					local NMatNew = NMat

					if RusTrans.checked == false then(
						-- latinisation is off
						NMatNew = RepChar NMat illegalChars ReplaceChar
					)
					else
					(
						NMatNew = LatinChar NMat
						NMatNew = RepChar NMatNew illegalChars ReplaceChar
					)
					--replace new name
					if mat.name != NMatNew then(
						ObjError = true
						ChrLog = ChrLog + "| ERROR! | Material name: " + mat.name + " | changed to: | " + NMatNew + "\n"

						mat.name = NMatNew
					)

					pbProgressBar.value = 100.000 / sceneMaterials.count * i
					i = i + 1
				)
				i = 1
			)
			--check and warning if need Bitmap paths
			if ChPathName.checked == true do(
				ChrLog = ChrLog + "Check bitmaps paths names...\n"

				local illegalChars = ChrBad.text
				local ReplaceChar = ChrRepl.text

				--remove from searching char some one like \./: because is present in normal path
				illegalChars = RepChar illegalChars " :;'@#~[{}]|!£$%^&*()-=+\,<.>//?\," "~"

				for mat in sceneMaterials do (

					windows.processPostedMessages()
					local NewNPath =undefined

					numTex = getNumSubTexmaps mat

					--print numTex
					local findPatern
					if numTex != undefined do (
						for i = 1 to numTex do (
							currentFilename = getSubTexmap mat i
							--FPath = getSubTexmap mat i.GetFullFilePath()
							--print currentFilename
							findPatern = matchPattern (currentFilename as string) pattern:"*:Bitmap"ignoreCase:false
							--print(currentFilename as string)
							--print findPatern
							if findPatern == true then (
								--texFileName = filenameFromPath currentFilename.filename

								if getFilenamePath currentFilename.filename != undefined then (
									Npath = getFilenamePath currentFilename.filename

									NewNPath = RepChar NPath illegalChars ReplaceChar
									if NPath != NewNPath then (
										ObjError = true
										ChrLog = ChrLog + "| WARNING! | Path: " + NPath + " | in texture | " + currentFilename as string + " | of material | " + mat.name + " | contains bad characters! |\n"
										--for d in deleteTextures do (
										--if d == texFileName then (
										--setSubTexmap mat i undefined
									)
								)
							)
						)
					)
					pbProgressBar.value = 100.000 / sceneMaterials.count * i
					i = i + 1
				)
				i = 1
			)

			--check and warning if need Bitmap name
			if ChTexName.checked == true do(
				ChrLog = ChrLog + "Check textures names...\n"
				local illegalChars = ChrBad.text
				local ReplaceChar = ChrRepl.text
				for mat in sceneMaterials do (
					windows.processPostedMessages()
					local NewNPath =undefined
					numTex = getNumSubTexmaps mat
					for i = 1 to numTex do (
						currentFilename = getSubTexmap mat i
						findPatern = matchPattern (currentFilename as string) pattern:"*:Bitmap"ignoreCase:false

						if findPatern == true then (
							if getFilenameFile currentFilename.filename != undefined then (
								Npath = getFilenameFile currentFilename.filename
								NewNPath = RepChar NPath illegalChars ReplaceChar
								if NPath != NewNPath then (
									ObjError = true
									ChrLog = ChrLog + "| WARNING! | Bitmap file name: " + NPath + " | contains bad characters!  |\n"
								)
							)
						)
					)
					pbProgressBar.value = 100.000 / sceneMaterials.count * i
					i = i + 1
				)
				i = 1
			)


			--Dagor Export
			fn checkPlugsVersion dir: plugsFilenames: ext: = (
				local sioDir = dotNetClass "System.IO.Directory"
				local sioFile = dotNetClass "System.IO.File"
				local sioPath = dotNetClass "System.IO.Path"
				local sioSearchOptAll = (dotNetClass "System.IO.SearchOption").AllDirectories
				if dir == unsupplied or not sioDir.exists dir do dir = getDir #maxroot

				local allFiles = sioDir.GetFiles dir "*.*" sioSearchOptAll
				if allFiles.count == 0 do return (messageBox "This directory not contains any file")

				if plugsFilenames != unsupplied then (
					local plugNames = #()
					ext = for p in plugsFilenames collect (
						append plugNames (tolower (sioPath.GetFileNameWithoutExtension p))
						sioPath.getExtension p
					)
					allFiles = for f in allFiles where findItem ext (tolower (sioPath.getExtension f)) > 0 collect (
						(tolower (sioPath.GetFileNameWithoutExtension f))
					)
				for n = 1 to plugNames.count do (
					if findItem allFiles plugNames[n] == 0 then return false
					else return true
					)
				)
				else -- with this statement you can print all installed plugins by given extensions
				(
					if ext == unsupplied do (
						ext = #(".bmi", ".bmf", ".bms", ".dlb", ".dlc", ".dle", ".dlf", ".dlh", ".dli", ".dlk", ".dlm", \
						".dlo", ".dlr", ".dls", ".dlt", ".dlu", ".dlv", ".dlx", ".flt", ".gup", ".ms", ".mse")
					)
					for f in allFiles where findItem ext (tolower (getfilenametype f)) > 0 do print f
				)
				free allFiles
			)

			if DoExport.checked == true do(
				ChrLog = ChrLog + "Check Dagor plugins files exist...\n"

				--Check by name existing plugin Files
				local PPcount = pluginPaths.count()
				local res1 = false
				local res2 = false
				for j = 1 to PPcount do (
					folder = pluginPaths.get j
					plug1 = #("dagor.dlu")
					plug2 = #("dagor_imp.dlu")
					if checkPlugsVersion dir:folder plugsFilenames:plug1 == true then res1 = true
					if checkPlugsVersion dir:folder plugsFilenames:plug2 == true then res2 = true
				)

				if res1 == true and res2 == true then (
					--Dagor plugs installed and everything is ok
					openMeasureFloater()
					ChrLog = ChrLog + "EXPORT...Done!\n"
				)
				else(
					--or you need instal plugs before start export
					ObjError = true
					ChrLog = ChrLog + "| ERROR! Dagor plugins is not installed! |\n"
				)

			)
			ChrLog = ChrLog + "Log finished\n"

			EnableSceneRedraw()
			CompleteRedraw()

			--we have errors and need open window with log

			try(form.close()) catch()
			form = dotnetobject "MaxCustomControls.Maxform"
			form.Text = "Errors Log"
			form.StartPosition = form.StartPosition.Manual
			form.Location = dotnetobject "System.Drawing.Point" 900 500
			form.Size = dotnetobject "System.Drawing.Size" 800 500
			tb = dotnetobject "RichTextBox"
			tb.Height = 800
			tb.Dock = tb.Dock.Top
			form.Controls.add tb
			form.showmodeless() (
			c = dotnetclass "System.Drawing.Color"
			fn printColored text tb colors:#(c.Red, c.Green) tokens:"	 " = (
				tb.SuspendLayout()
				txt = filterstring text tokens splitEmptyTokens:on
				tb.ForeColor = tb.ForeColor.Black
				tb.Text = text
				tb.SelectionStart = 0
				for k=0 to txt.count-1 do (
					s = txt[k+1].count
					tb.SelectionLength = s
					tb.selectionColor = colors[(mod k colors.count) + 1]
					tb.SelectionStart += s+1
						)
							tb.SelectionLength = 0
							tb.ResumeLayout()
						)
				--take text from ChrLog stored early
				printColored ChrLog tb colors:#(c.Blue, c.Red) tokens:"*|"
			)
		)

		-----------------------------save script window_position on background-----------------------------------------------
		local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
		on GJFiGliff open do
		(
			try (
				--ini file alredy exist and have our data
				SetDialogPos GJFiGliff ((getINISetting INIfile "GJFiGliff" "DialogPos")as Point2)
			) catch (
				--our data not found or inin file not exist and we nothing change in dialog pos
			)
		)

		--write to ini file if dialog moved or closed
		on GJFiGliff moved new_pos do
		(
			try(setINISetting INIfile "GJFiGliff" "DialogPos" (new_pos as string) forceUTF16:false)catch()
		)
		on GJFiGliff close do
		(
			try(setINISetting INIfile "GJFiGliff" "DialogPos" ((GetDialogPos GJ_AO_Baker) as string) forceUTF16:false)catch()
		)
		-------------------------------------------------------------------------------------------------------------------
	)
createDialog GJFiGliff style:#(#style_toolwindow, #style_sysmenu)
)

--Fixing rotated normals script
macroScript GJFixNormal
buttontext:"GJFixNormal..."
category:"GJ_Tools"
IconName:"Layers/CreateNewLayer"
(
	global ObjNameBase
	global ObjCount
	global MainObj

if GJFixNormal !=undefined do destroyDialog GJFixNormal
rollout GJFixNormal "Dagor Fix Normals Orientation" width:200
	(
		group "1. Pick and Fix Normals Orientation"
		(
			button pick_source "FIX NORMALS FROM SELECTION!" width:180 height:25 align:#center
		)
		group "Progress Bar"
		(
			progressBar pbProgressBar "TotalExportProgress" width:180 height:10 color:(color 190 0 0) align:#center
		)

		group "Learning Path"
		(
			button btnLearn "Open Local Documentation" width:180 height:25 align:#center
			button btnAut "Get in Touch with the Author" width:180 height:25 align:#center
		)

		--learn
		on btnLearn pressed do (
			local pathArr = filterString (pathConfig.removePathLeaf(getSourceFileName()) as string) "\\"
			local finalPath1 = ""
			local finalPath2 = ""
			for t = 1 to pathArr.count do (
				finalPath2 = finalPath2 + pathArr[t] + "\\"
				if t <= pathArr.count-1 then finalPath1 = finalPath1 + pathArr[t] + "\\"
			)
			finalPath1 = finalPath1 + "documentation\fix-normal-orientation-tool.html "
			finalPath2 = finalPath2 + "documentation\fix-normal-orientation-tool.html "

			ShellLaunch finalPath1 ""
			ShellLaunch finalPath2 ""
			ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/fix_normal_orientation_tool.html" ""
		)

		on btnAut pressed do (
			ShellLaunch "https://github.com/VlasovAlexey " ""
		)


		-- Main loop
		on pick_source pressed do (
			disableSceneRedraw()

			local ObjSel = selection as array
			local i = 1
			ObjCount = 0
			for MainObj in ObjSel do (

				PivotPosition = MainObj.pivot
				ResetPivot MainObj
				CenterPivot MainObj
				MainObj.pivot = PivotPosition

				pbProgressBar.value = 100.000 / ObjSel.count * i
				i = i + 1
			)
			i = 1

			EnableSceneRedraw()
			CompleteRedraw()
		)

		-----------------------------save script window_position on background-----------------------------------------------
		local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
		on GJFixNormal open do
		(
			try (
				--ini file alredy exist and have our data
				SetDialogPos GJFixNormal ((getINISetting INIfile "GJFixNormal" "DialogPos")as Point2)
			) catch (
				--our data not found or inin file not exist and we nothing change in dialog pos
			)
		)

		--write to ini file if dialog moved or closed
		on GJFixNormal moved new_pos do
		(
			try(setINISetting INIfile "GJFixNormal" "DialogPos" (new_pos as string) forceUTF16:false)catch()
		)
		on GJFixNormal close do
		(
			try(setINISetting INIfile "GJFixNormal" "DialogPos" ((GetDialogPos GJFixNormal) as string) forceUTF16:false)catch()
		)
		-------------------------------------------------------------------------------------------------------------------
	)
	createDialog GJFixNormal style:#(#style_toolwindow, #style_sysmenu)

)

--GJDamagesSetup
macroScript GJDamagesSetup
buttontext:"GJDamagesSetup..."
category:"GJ_Tools"
IconName:"MassFX/CreateHingeConstraint"
(
	global ObjNameBase
	global ObjCount
	global MainObj

if GJDamagesSetup !=undefined do destroyDialog GJDamagesSetup
rollout GJDamagesSetup "Dagor Damages Setup" width:200
	(
		group "1. Visible Objects Parameters"
		(
			editText obj_param "" fieldWidth:180 height:150 align:#center text:"animated_node:b=yes\nphysObj:b=yes\ncollidable:b=no\nmassType:t=\"none\"" labelOnTop:true
		)
		group "2. Collision Objects Parameters"
		(
			editText obj_col_param "" fieldWidth:180 height:150 align:#center text:"materialName:t=\"wood_solid\"\nmassType:t=\"box\"\ndensity:r=150\nrenderable:b=no\ncollType:t=\"box\"" labelOnTop:true

		)
		group "Collision Generation Settings" (
			spinner RndRotSpn "Random Collision Rotation" range:[0,10,2.5] fieldwidth:40 type:#float tooltip:"Range of Random Collision Rotation" align:#center
			spinner maxVertSpn "Convex Maximum Verts" range:[12,2048,64] fieldwidth:60 type:#integer tooltip:"Maximum number of vertices to use for convex hull" align:#center
			spinner offsetSpn "Convex Surface Offset" range:[-1e10,1e10,0.01] fieldwidth:60 type:#worldunits tooltip:"Offset of convex hull to surface" align:#center

			editText postfix_name "Collision Name Postfix" fieldWidth:60 align:#center text:"_dmg"
			dropDownList CollisionType "Collision Type" width:120 height:40 align:#center items:#("Sphere", "Box", "Capsule", "Convex Hull") selection:2
			checkbox PrOldUserProp "Preserve Old User Parameters" align:#center checked:true enable:true
		)
		group "3. Pick and create Damaged Parts"
		(
			button pick_source "CREATE FROM SELECTION!" width:180 height:25 align:#center
		)
		group "Progress"
		(
			progressBar pbProgressBar "ExportProgress" width:180 height:10 color:(color 0 132 255) align:#center value:100
		)

		group "Learning Path"
		(
			button btnLearn "Open Local Documentation" width:180 height:25 align:#center
			button btnAut "Get in Touch with the Author" width:180 height:25 align:#center
		)

		--learn
		on btnLearn pressed do (
			local pathArr = filterString (pathConfig.removePathLeaf(getSourceFileName()) as string) "\\"
			local finalPath1 = ""
			local finalPath2 = ""
			for t = 1 to pathArr.count do (
				finalPath2 = finalPath2 + pathArr[t] + "\\"
				if t <= pathArr.count-1 then finalPath1 = finalPath1 + pathArr[t] + "\\"
			)
			finalPath1 = finalPath1 + "documentation\damages-setup-tool.html "
			finalPath2 = finalPath2 + "documentation\damages-setup-tool.html "

			ShellLaunch finalPath1 ""
			ShellLaunch finalPath2 ""
			ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/damages_setup_tool.html" ""
		)

		on btnAut pressed do (
			ShellLaunch "https://github.com/VlasovAlexey " ""
		)

		fn deleteUserProp node = (
			setUserPropBuffer node ""
		)

		-- Main loop DamagesSetup
		on pick_source pressed do (
			disableSceneRedraw()

			local ObjSel = selection as array
			local i = 1
			ObjCount = 0
			for MainObj in ObjSel do (
				--Add custom property
				if PrOldUserProp.checked == false then (
					deleteUserProp MainObj
				)
				local ParamArr = FilterString obj_param.text "=,\n"
				for i = 1 to ParamArr.count by 2 do (
					setUserProp MainObj ParamArr[i] ParamArr[i+1]
				)

				windows.processPostedMessages()
				ObjNameBase = MainObj.name
				with redraw off (
					local rb = MassFX_RBody()
					--ResetPivot MainObj
					select MainObj
					local cmdMode = getCommandPanelTaskMode()		-- store previous mode
					setCommandPanelTaskMode #modify
					addModifier MainObj rb
					rb.meshVerticesLimit  = maxVertSpn.value
					rb.meshType = CollisionType.selection  		-- 3 = capsule, 4 = convex
					rb.meshInflation = offsetSpn.value
					rb.RBMeshRegenerate 1
					local hull = rb.RBMeshConvertToCustomMesh 1
					if isValidNode hull then (
						hull.name = ObjNameBase + postfix_name.text -- + "_0" + (ObjCount as string)
						ObjCount = ObjCount + 1
					)
					deleteModifier MainObj 1
					setCommandPanelTaskMode cmdMode		-- restore previous mode
				)
				convertToMesh hull
				hull.pivot = hull.center
				ResetScale hull

				--move collision to same layer
				layer = MainObj.layer
				layer.addNode hull

				--random rotation collision
				myRotate = eulerangles (random 0 RndRotSpn.value) (random 0 RndRotSpn.value) (random 0 RndRotSpn.value)
				rotate hull myRotate

				--assign Dagor collision material
				local pFile = (GetDir #userStartupScripts) + "/DagorLibrary.mat"
				local pFileError = doesFileExist pFile
				if pFileError == false then (
					messageBox "DagorLibrary.mat not found! PLease put DagorLibrary.mat to C:\Users\$YOUR_USER_NAME$\AppData\Local\Autodesk\3dsMax_$YOUR_3DSMAX_VERSION$\ENU\scripts\startup"
				)
				else
				(
					matLib = loadTempMaterialLibrary pFile
					hull.material = matLib[1]--asign first material from lib
				)

				--Add custom property
				local ParamArr = FilterString obj_col_param.text "=,\n"
				for i = 1 to ParamArr.count by 2 do (
					setUserProp hull ParamArr[i] ParamArr[i+1]
				)

				--parrenting collision to selected object
				hull.parent = MainObj

				pbProgressBar.value = 100.000 / ObjSel.count * i
				i = i + 1
			)
			i = 1

			EnableSceneRedraw()
			CompleteRedraw()
		)

		on pick_source picked obj do
		(

		)

		on bt_ pressed  do
		(

		)

		-----------------------------save script window_position on background-----------------------------------------------
		local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
		on GJDamagesSetup open do
		(
			try (
				--ini file alredy exist and have our data
				SetDialogPos GJDamagesSetup ((getINISetting INIfile "DamagesSetup" "DialogPos")as Point2)
			) catch (
				--our data not found or inin file not exist and we nothing change in dialog pos
			)
		)

		--write to ini file if dialog moved or closed
		on GJDamagesSetup moved new_pos do
		(
			try(setINISetting INIfile "DamagesSetup" "DialogPos" (new_pos as string) forceUTF16:false)catch()
		)
		on GJDamagesSetup close do
		(
			try(setINISetting INIfile "DamagesSetup" "DialogPos" ((GetDialogPos GJDamagesSetup) as string) forceUTF16:false)catch()
		)
		-------------------------------------------------------------------------------------------------------------------
	)
	createDialog GJDamagesSetup style:#(#style_toolwindow, #style_sysmenu)

)

--GJRemove Unused Bones
macroScript GJRemoveUnusedBones
buttontext:"GJRemoveUnusedBones..."
category:"GJ_Tools"
IconName:"MassFX/RemoveRagdoll"
(
if GJRemoveUnusedBones !=undefined do destroyDialog GJRemoveUnusedBones
rollout GJRemoveUnusedBones "GJRemoveUnusedBones" width:200
(
	group "1. Pick Skined Object"
	(
		pickbutton pick_source_obj "Pick Skined Object" width:180 height:25 align:#center
	)

	group "2. ROOT Bone Name"
	(
		--pickbutton pick_source_bone "Pick ROOT Bone" width:250 align:#center
		edittext source_bone_name "" fieldWidth:180 height:25 align:#center text:"ROOT_" labelOnTop:true
	)

	group "3. Delete Unused Bones"
	(
		button bt_remove "DELETE UNUSED BONES!" width:180 height:25 align:#center
		checkbox DltAfterRoot "Del All Parent Bones after ROOT" align:#center checked:true enable:true
	)
	group "Progress Bar"
	(
		progressBar PBProgress "TotalExportProgress" width:180 height:10 color:(color 190 0 0) align:#center
	)
	group "Learning Path"
		(
			button btnAut "Get in Touch with the Author" width:180 height:25 align:#center
		)

		--learn
		on btnAut pressed do (
			ShellLaunch "https://github.com/VlasovAlexey " ""
		)

	--select all parrent objects single branch
	fn addParentArr obj arra=(
		if isValidNode obj and isValidNode obj.parent do(
			append arra obj.parent
			addParentArr obj.parent arra
		)
	)

	--select all children objects in selection
	fn get_all_nodes nodes:#()=(
		allnodes = #()
		for n in nodes do (
			append allnodes n
			join allnodes (get_all_nodes nodes:(n.children))
		)
		allnodes
	)

	-- Main Loop
		on bt_remove pressed do
		(	--reset progress bar
			PBProgress.value = 0.0

			source_obj = pick_source_obj.object
			--source_bone = pick_source_bone.object
			source_bone = getNodeByName source_bone_name.text
			--source_bone = getNodeByName "ROOT_"
			--print(source_bone_name.text)

			--IMPORTANT! You need go to to modify mode before work with any modifiers property
			max modify mode
			select source_obj

				if (source_obj != undefined) then
				(

						if (source_bone != undefined) then
						(
							--all property is checked and ready to prepare

							--Remove unused bones from selected object
							fn selectSkinBonesFn obj =
							(
								local weightThresh = 0.0001
								local objs = selection as array
								for obj in objs do (
									select obj
									for m in obj.modifiers where classof m == Skin do (
										modPanel.setCurrentObject m
										local vertCount = skinOps.GetNumberVertices m
										local bonesCount = skinOps.GetNumberBones m
										local unusedBones = #{1..bonesCount}
										for v = 1 to vertCount do (
											local vertWeightCount = skinOps.GetVertexWeightCount m v
											for i = 1 to vertWeightCount do (
												local weight = skinOps.GetVertexWeight m v i
												if weight >= weightThresh then (
													local boneID = skinOps.GetVertexWeightBoneID m v i
													unusedBones[boneID] = false
												)
											)
											PBProgress.value = 100.*v
										)
										for i = bonesCount to 1 by -1 where unusedBones[i] do (
											skinOps.SelectBone m i
											skinOps.RemoveBone m
											PBProgress.value = 100.*i
										)
										--format "% Bones where removed from %.%\n" unusedBones.numberSet obj.name m.name
									)
								)
							)

						selectSkinBonesFn source_obj
							--Return bones list in selected object
fn selectSkinBonesFn obj =
(
	max modify mode
	select obj

	if obj.modifiers[#Skin] == undefined do
	(
		messageBox "This object doesn`t have SKIN Modifier!" title:"Error!"
		local skinBones = #()
		return skinBones
	)

	if obj.modifiers[#Skin] != undefined do
	(
		skinBones = #()
		skinMods = getclassinstances Skin target:obj

		for s = 1 to skinMods.count where skinMods.count > 0 do
		(
			skel = skinOps.getnumberbones skinMods[s]
			for x = 1 to skel do
			(
				myBone = (skinOps.getBoneName skinMods[s] x 0)
				append skinBones (getNodeByName myBone)
			)
		)

		clearSelection()
		return skinBones
	)

)




						--collect all selected bones to one system
						ParentArr = #()
						theobj = #()
						theobj = selectSkinBonesFn source_obj

						for n = 1 to theobj.count do(
							addParentArr theobj[n] ParentArr
						)

						--Select ROOT Bone
						rootBone = source_bone
						select rootBone

						--Select all bones from root
						ChildrenArr = get_all_nodes nodes:(selection as array)
						selectmore ChildrenArr

						--select all used bones
						deselect ParentArr
						deselect theobj

						--delete unused bones
						delete selection

							if DltAfterRoot.checked == true then(
								ParentArr = #()
								theobj = #()
								select rootBone
								theobj = selection
								addParentArr theobj[1] ParentArr
								deselect theobj
								select ParentArr
								delete selection
							)

						)
						else
						(
							messageBox "You must set a ROOT Bone first!" title:"Error!"
						)

				)
				else
				(
					messageBox "You must set a Skined Object first!" title:"Error!"
				)

		)

		--Pick to ROOT Bone
		on pick_source_bone picked Obj do
		(
			if Obj != undefined then
			(
				pick_source_bone.text = Obj.name
			)
		)
		--Pick to Skined Object
		on pick_source_obj picked Obj do
		(
		if Obj != undefined then
		(
			select Obj
			for mesh in selection do
			(
				if superclassof mesh == GeometryClass and ClassOf mesh == Editable_mesh then
				(
					if Obj.modifiers[#skin] != undefined then
					(
						pick_source_obj.text = Obj.name
					)
					else
					(
						messagebox ("Please Add Skin Modyfier to Source Object!") title: "Warning!"
					)
				)
				else
				(
					messagebox ("Please convert to Editable mesh or use Editable Mesh for Source Skined Object!") title: "Warning!"
				)
			)
		)
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
	on GJRemoveUnusedBones open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJRemoveUnusedBones ((getINISetting INIfile "RemoveUnusedBones" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
	)

	--write to ini file if dialog moved or closed
	on GJRemoveUnusedBones moved new_pos do
	(
		try(setINISetting INIfile "RemoveUnusedBones" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
	on GJRemoveUnusedBones close do
	(
		try(setINISetting INIfile "RemoveUnusedBones" "DialogPos" ((GetDialogPos GJRemoveUnusedBones) as string) forceUTF16:false)catch()
	)
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJRemoveUnusedBones style:#(#style_toolwindow, #style_sysmenu)
)

-- Global About Window
macroScript About_GJ
buttontext:"About..."
category:"GJ_Tools"
IconName:"MassFX/WorldParameters"

(
	rollout AboutGJ "About" width:296 height:150
	(
		bitmap bmp2 "Bitmap" pos:[8,8] width:280 height:80 fileName: "gj_logo.bmp"
		label label_text2 "© 2024 Gaijin Network Ltd. All rights reserved. v2.2" align:#center
		button btn1 "Ok" width:88 height:24
		on btn1 pressed do destroyDialog(AboutGJ)
	)
	createDialog AboutGJ 296 150
)

--main rollout for AO Baker
macroScript GJ_AO_Baker
buttontext:"Ambient Occlusion Baker..."
category:"GJ_Tools"
IconName:"MainUI\Array"
(

try(DestroyDialog ::GJ_AO_Baker)catch()
rollout GJ_AO_Baker "Dagor AO Baker" width:200 height:398
(
	group "Setup AO"
	(
		colorpicker theAOColor "AO Tint Color:       " color:[255,255,255] modal:false enabled:true
		colorpicker AmbColor "AO Ambient Color:" color:[45,45,45] modal:false
		checkbox DisplayAO "Display AO on Viewport" align:#left checked:true enable:true
		checkbox beautifierAO "Use Beautifier(experemental)" align:#left checked:true enable:true
		dropdownlist MapSh "Transfer AO to Map Channel" items:#("1", "2", "3", "4", "5", "6", "7", "8", "9", "10") selection: 8

		checkbox GradEnable "Enable Gradient" align:#left checked:true enable:true
		spinner GradPow "Gradient Blend Percentages" fieldwidth:30 range:[0,100,80] type:#integer tooltip:"Gradient Blend %" align:#left
		dropdownlist GradBlend "Gradiend Blend Type" items:#("Normal", "Multiply", "Add", "Overlay") selection: 1

		progressBar pbProgressBar "Progress" width:180 height:10 color:(color 0 132 255) align:#center value:50
		button btnBakeAO "Bake Selected Object" align:#center width:180 height:25
	)


	on theAOColor changed nColor do (
		theAOColor.color = nColor
	)

	on AmbColor changed nnColor do (
		AmbColor.color = nnColor
		print (filterString (AmbColor.color as string) "() ")
	)

	on GradEnable changed GradEnable_st do (
		if GradEnable_st == true then (
			GradPow.enabled = true
			GradBlend.enabled = true
		)
		if GradEnable_st == false then (
			GradPow.enabled = false
			GradBlend.enabled = false
		)
	)

	--start AO baking
	on btnBakeAO pressed do (

		local obj = selection as array

		--fixing problem with inverted/broken normals by clear properties by attach to box
		local objNewList = #()
		for f in obj do (
			--first make active layer from selected objects
			local aLayer = f.layer.name
			for i = 0 to (LayerManager.count - 1) do
			(
				CLayerProp = layermanager.getlayer i
				if CLayerProp.name == aLayer then CLayerProp.current = true
			)
			--get properties for save transfer to new object
			local oldObjPos = f.pos
			local oldObjName = f.name
			local oldObjWireC = f.wireColor
			local oldObjProp = getUserPropBuffer f
			local oldObjMat = f.material
			f.pos = [0,0,0]

			--reset any transforms to default. It is important for many strange imported objects transforms
			resetBox = box length:2 width:2 height:2
			ConvertTo resetBox Editable_Poly
			polyop.attach resetBox f

			faceBoxList = #(1,2,3,4,5,6)
			polyop.deleteFaces resetBox faceBoxList

			--restore all properties from old object to new created
			resetBox.name = oldObjName
			resetBox.pos = oldObjPos
			resetBox.wireColor = oldObjWireC
			resetBox.material = oldObjMat
			setUserPropBuffer resetBox oldObjProp

			join objNewList #(resetBox)
		)
		obj = objNewList

		pbProgressBar.value = 0
		local totalSteps = Obj.count
		local currentSteps = 0
		for d in obj do (
			if d.isHidden == false then (
				local env_ambient =  ambientColor
				max select all
				--Hide Selection for GI computation
				for o in selection do
				(
					o.isGIExcluded = true
				)
				obj.isGIExcluded = false
				select d

				--setup AO material
				global AOmat = StandardMaterial()
				AOmat.name = ("AO_mat")
				AOmat.diffuse = theAOColor.color
				AOmat.ambient = theAOColor.color

				--back up material from selection
				global backMat = copy d.material
				d.material = AOmat

				--Create a shadow casting white sky light
				theLight = Skylight pos:[0,0,0] rgb:white
				theLight.multiplier = 1.0
				theLight.color = (color 255 255 255)
				theLight.on = true
				theLight.sky_mode = 1
				rays_per_sample = 20
				ray_bias = 0.005
				castShadows = true

				--setup default radiosity
				SceneRadiosity.radiosity = Radiosity();

				sceneRadiosity.radiosity.Reset true true
				sceneRadiosity.radiosity.lightUnitsUsed = 0
				sceneRadiosity.radiosity.radProcessInRenderOnly = false
				sceneRadiosity.radiosity.radInitialQuality = 85.0
				sceneRadiosity.radiosity.radGlobalRefineSteps = 0
				sceneRadiosity.radiosity.radSelectionRefineSteps = 0
				sceneRadiosity.radiosity.radFiltering = 0
				sceneRadiosity.radiosity.radDirectFiltering = 0

				sceneRadiosity.radiosity.radDisplayInViewport = true

				sceneRadiosity.radiosity.meshingEnabled = false
				sceneRadiosity.radiosity.shootDirectLights = true
				sceneRadiosity.radiosity.includePointLights = false
				sceneRadiosity.radiosity.includeLinearLights = false
				sceneRadiosity.radiosity.includeAreaLights = false
				sceneRadiosity.radiosity.includeSkylight = true
				sceneRadiosity.radiosity.includeSelfEmittingLights = true
				sceneRadiosity.radiosity.minimumSelfEmittingSize = 0.01

				sceneRadiosity.radiosity.rmReuseDirectIllumination = true
				sceneRadiosity.radiosity.rmRegather = false

				--setup radiosity render properties

				RadiosityPreferences.useAdvancedLighting = true
				RadiosityPreferences.computeAdvancedLighting = true

				--setup fastest render
				renderWidth = 1
				renderHeight = 1
				rendTimeType = 1
				rendShowVFB = false
				rendHidden = false
				ambientColor = AmbColor.color

				renderers.target = "local"
				renderers.current = RendererClass.classes[1]()

				setRenderType #selection

				--star compute radiosity
				renderSceneDialog.close()
				max quick render

				--asign lighting to map shannel 8
				utilitypanel.openutility Assign_Vertex_Colors
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Map Channel" do exit with i[1])
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Lighting &Only" do exit with i[1])
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Radiosity, Reuse Direct" do exit with i[1])
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Color by Vertex" do exit with i[1])
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Assign to Selected" do exit with i[1])

				--transfer color info from ch3 to ch8
				macros.run "Tools" "Channel_Info"
				if (isValidNode d) then (
					channelInfo.CopyChannel d 3 3
					for r = 1 to 5 do channelInfo.addChannel d
					channelInfo.PasteChannel d 3 (MapSh.selected as integer)
					maxOps.CollapseNode d true
					setUserPropBuffer d ""
					setUserProp d "renderable:b" "yes"
					setUserProp d "collidable:b" "no"
				)
				global hwnd = windows.getChildHWND 0 "Map Channel Info"
				windows.sendMessage hwnd[1] 0x0010 0 0
				max modify mode

				--display AO
				if DisplayAO.checked == true then (
					setCVertMode d true
					d.vertexColorMapChannel = MapSh.selected as integer
					d.vertexColorType = #map_channel
				)

				--remove special created objects and lights
				delete theLight
				d.material = backMat
				ambientColor = env_ambient
				currentSteps = currentSteps + 1
				pbProgressBar.value = 100.000 / totalSteps * currentSteps

				--unhide all objects in scene one command
				--objects.isHidden = false
				max modify mode


				local ao_mult_color = (filterString (theAOColor.color as string) "() ")


				--add AO tint color
				VPaintMod = PaintLayerMod()
				addModifier d VPaintMod
				d.modifiers[1].mapChannel = (MapSh.selected as integer)
				d.modifiers[1].layerMode = "Multiply"
				d.modifiers[1].layerOpacity = 100

				VPaintState = VPaintMod.AcquirePaintState d
				for y = 1 to getNumVerts d do (
					VPaintState.SetVertColor y [(ao_mult_color[2] as float)/255, (ao_mult_color[3] as float)/255, (ao_mult_color[4] as float)/255, 1]
				)
				VPaintMod.ApplyPaintState d VPaintState
				convertToPoly d

				-- beautifierAO
				if beautifierAO.checked == true then (
					addModifier d VPaintMod
					d.modifiers[1].mapChannel = (MapSh.selected as integer)
					d.modifiers[1].layerMode = "Soft Light"
					d.modifiers[1].layerOpacity = 100

					VPaintState = VPaintMod.AcquirePaintState d
					for y = 1 to getNumVerts d do (
						VPaintState.SetVertColor y [(ao_mult_color[2] as float)/255, (ao_mult_color[3] as float)/255, (ao_mult_color[4] as float)/255, 1]
					)
					VPaintMod.ApplyPaintState d VPaintState
					convertToPoly d

					--second pass
					addModifier d VPaintMod
					d.modifiers[1].mapChannel = (MapSh.selected as integer)
					d.modifiers[1].layerMode = "Soft Light"
					d.modifiers[1].layerOpacity = 50

					VPaintState = VPaintMod.AcquirePaintState d
					for y = 1 to getNumVerts d do (
						VPaintState.SetVertColor y [(ao_mult_color[2] as float)/255, (ao_mult_color[3] as float)/255, (ao_mult_color[4] as float)/255, 1]
					)
					VPaintMod.ApplyPaintState d VPaintState
					convertToPoly d
				)

				--Ambient coloring
				ao_amb_color = (filterString (AmbColor.color as string) "() ")

				addModifier d VPaintMod
				d.modifiers[1].mapChannel = (MapSh.selected as integer)
				d.modifiers[1].layerMode = "Screen"
				d.modifiers[1].layerOpacity = 100

				VPaintState = VPaintMod.AcquirePaintState d
				for y = 1 to getNumVerts d do (
					VPaintState.SetVertColor y [(ao_amb_color[2] as float)/255, (ao_amb_color[3] as float)/255, (ao_amb_color[4] as float)/255, 1]
				)
				VPaintMod.ApplyPaintState d VPaintState
				convertToPoly d

				--adding gradient if enabled
				if GradEnable.checked == true then (
					d = $
					convertToPoly d
					obj_copy = copy d

					--make gradient on copied object
					nHeight = for i=1 to obj_copy.numVerts collect (polyop.getVert obj_copy i)[3]
					maxH = amax nHeight
					minH = amin nHeight

					for i=1 to obj_copy.numVerts do (
						colorVal = ( ( ( (polyop.getVert obj_copy i)[3] - 0 ) / (maxH - 0) )*255)
						if colorVal < 0.0 do (colorVal = 0.0)
						polyop.setVertSelection obj_copy i
						polyop.setvertcolor obj_copy 0 obj_copy.selectedVerts (color colorVal colorVal colorVal)
					)

					--second copy from vcolor to mapPaintState data
					VPaintMod = PaintLayerMod()
					addModifier obj_copy VPaintMod

					--create copy of paint state
					VPaintMod.mapChannel = 0
					VPaintMod.MergeVertexColors obj_copy
					VPaintState = VPaintMod.AcquirePaintState obj_copy

					--we have copied gradient state and don`t need any more copied object
					delete obj_copy

					--add same modifier to final object
					VPaintModF = PaintLayerMod()
					addModifier d VPaintModF
					VPaintModF.mapChannel = (MapSh.selected as integer)
					VPaintModF.layerMode = GradBlend.selected
					VPaintModF.layerOpacity = (GradPow.value as integer)

					--apply copied gradient data to final object
					VPaintModF.ApplyPaintState d VPaintState
				)
			)
		)
	)

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
		finalPath1 = finalPath1 + "documentation\ambient-occlusion-baker.html "
		finalPath2 = finalPath2 + "documentation\ambient-occlusion-baker.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/ambient_occlusion_baker.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_AO_Baker open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_AO_Baker ((getINISetting INIfile "AO_Baker" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_AO_Baker moved new_pos do
	(
		try(setINISetting INIfile "AO_Baker" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_AO_Baker close do
	(
		try(setINISetting INIfile "AO_Baker" "DialogPos" ((GetDialogPos GJ_AO_Baker) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_AO_Baker
)

--main rollout
macroScript GJ_Dagorppainter
buttontext:"Pivot Painter Vegetation Tool..."
category:"GJ_Tools"
IconName:"MainUI\Group"
(
try(DestroyDialog ::GJ_Dagorppainter)catch()
rollout GJ_Dagorppainter "Dagor Pivot Painter Tool" width:200
(
	fn tree_filt obj = ((classof obj == GrowFX2 or classof obj == GrowFX))
	group "Settings"
	(

		--spinner spnScaleFactor "Scale Factor" range:[0.001, 1000.0, 1.0] fieldwidth:50 type: #float
		editText edtFinalObjectName "" fieldWidth:178 text:"FINAL OBJECT NAME HERE" multiLine: false align:#center
		pickbutton btnPick "Pick GrowFX Object" align:#center width:180 height:23 tooltip: "Pick Root object" filter: tree_filt
		checkbox grpByPathColor "Group by Path Color" checked:false enabled:true
		editText edtTextureSavePath "" fieldWidth:141 height:21 align:#left text:"TEXTURE SAVE PATH" readOnly: true across: 2 --labelOnTop:true
		button btnTextureSavePath "..." width:25 height:21 align: #right tooltip: "Pick Folder to store .DDS textures"
		button btnSaveSettings "Save Settings" align:#center width:180 height:25
		button btnGo "START CREATION" align:#center width:180 height:23 tooltip: "Create DDS-textures and merge hierarchy into single object"
		progressBar pbProgressBar "TotalExportProgress" width:180 height:10 color:(color 0 132 255) align:#center value:50
	)

	group "Learning Path"
	(
		button btnLearnF "Open Local Documentation" align:#center width:180 height:25
		button btnContactF "Contact with Developer" align:#center width:180 height:25
	)

	local X_RES = 32.0, Y_RES = 64.0, rootObj = undefined, texturesavepath = "", ScaleFactor = 1.0

	local inipath = getFilenamePath (getSourceFileName()), inifile = "dagor_ppainter.ini"


	fn compareFN v1 v2 valArray: centerpoint: =
	(
	local v1i = valArray[v1]
	local v2i = valArray[v2]
	local d = (distance v1i centerpoint) - (distance v2i centerpoint)
	case of
	(
	(d < 0.): 1
	(d > 0.): -1
	default: 0
	)
	)

	fn adjustWorldDirectionForUnreal myVector = (
			[myVector[1], myVector[3], myVector[2]]*ScaleFactor --1.0
		)

	fn adjustDir myVector = (
			--myVector = [myVector[1], myVector[3], myVector[2]]
			myVector = (normalize myVector)*.5+.5
			myVector = [myVector[1], myVector[3], myVector[2]]
			--[myVector[1], myVector[2], 1.0-myVector[3]]*255.0 --255.0*100 --1.0
		)

	fn findHierarchyData obj =
			(
				select obj
				finalSortedArray=for i in $ collect i
				for obj in finalSortedArray collect #(obj, (adjustWorldDirectionForUnreal obj.pivot), 0, 0, 0, 0, 0, 0) --obj.transform.row1
			)

--	fn convertIndexToUV idx = (
--		uv2 = ([mod idx X_RES, idx/X_RES]+0.5)/[X_RES, Y_RES]
--		)

	fn findXYLocationOfIndex index xy = (
			index=(index-1)
			newXY=[(mod (index) xy[1]), floor (index/xy[1])]
			newXY+=[1,1]
		)

	fn getArrayOfPixels inputarr num = (
		ArrayOfPixels = #()

		localMaxY=-1
		for i=1 to inputarr.count do (
			newXY=findXYLocationOfIndex i [X_RES, Y_RES]
			if newXY[2]>localMaxY do append ArrayOfPixels #()
			localMaxY=newXY[2]
			--format "-len-%\n" inputarr[i][5]
			append ArrayOfPixels[newXY[2]] [inputarr[i][num][1], inputarr[i][num][2], inputarr[i][num][3], inputarr[i][num+2] as float]
		)
		ArrayOfPixels
	)


	fn saveDDS pixelsarray f meta ishdr = (

		bmapTexture = bitmap (X_RES as integer) (Y_RES as integer) filename: (texturesavepath + edtFinalObjectName.text + f) hdr: ishdr --(maxFilePath+"\\test\\"+(getFilenameFile maxfilename)
		for i=0 to (pixelsarray.count-1) do (
			setPixels bmapTexture [0, i] pixelsarray[(i+1)]
			)

		save bmapTexture gamma: 1.0 metadata: meta quiet: true
		close bmapTexture
		format "tex: %\n" bmapTexture.filename

		freeSceneBitmaps()
	)

	on GJ_Dagorppainter open do (

		if doesFileExist (inipath+inifile) then texturesavepath = edtTextureSavePath.text = getINISetting (inipath+inifile) "Directories" "texturesavepath"
		--setINISetting <filename_string> <section_string> <key_string> <key_value_string>

		--if selection.count==1 then if queryBox "Get Final Object Name from Selected object?" then edtFinalObjectName.Text = selection[1].name
	)

	on btnPick picked obj do if obj != undefined do (
		unhide obj
		select obj
		--max modify mode
		edtFinalObjectName.Text = obj.name
		rootObj = obj
	)

	on btnTextureSavePath pressed do (
		texturesavepath = getSavePath  caption:"Select Your Image Directory" initialDir: (if (isDirectoryWriteable texturesavepath) then texturesavepath else maxfilepath)
		if texturesavepath==undefined then texturesavepath=""
		if isDirectoryWriteable texturesavepath then (
			if (substring texturesavepath texturesavepath.count 1)!="\\" then texturesavepath+="\\"
			edtTextureSavePath.text=texturesavepath
			setINISetting (inipath+inifile) "Directories" "texturesavepath" texturesavepath
			)
	)

	--find duplicate objects names in scene
	fn find_dup_names base_name = (
		local result = 0
		--max modify mode
		for o in objects do
		(
			if o.name == base_name then (
				result = result + 1
				if result > 1 then (
					return true
				)
			)
		)
		return false
	)

	on btnGo pressed do if isDirectoryWriteable texturesavepath and edtFinalObjectName.text != "FINAL OBJECT NAME HERE" then (
		--rootObj = for i in selection where i.parent==undefined and (classof i) == Editable_Poly collect i
		--if rootObj.count!=1 then messagebox "Select single Epoly root-object!"
		if (find_dup_names rootObj.name) == false then (
			local tmp_mat = rootObj.material
			local aLayer = rootObj.layer.name
			for i = 0 to (LayerManager.count - 1) do (
				CLayerProp = layermanager.getlayer i
				if CLayerProp.name == aLayer then CLayerProp.current = true
			)

			max modify mode
			local grp_c_checker = 0
			if grpByPathColor.checked == true then grp_c_checker = 1

			--decomposite object to parts
			select rootObj
			for c in windows.getChildrenHWND #max where c[4] == "Button" and c[5] == "Group by Path Color" do UIAccessor.sendMessage c[1] 0x00F1 grp_c_checker 0
			UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Create Hierarchy of Meshes" do exit with i[1])

			print (getNodeByName((rootObj.name as string) + "_1_001"))
			select (getNodeByName((rootObj.name as string) + "_1_001"))
			$.name = rootObj.name
			rootObj = $

			ScaleFactor = case units.SystemType of (
			#Centimeters : .01
			#Meters : 1.0
			#millimeters : 0.001
			default: 1.0
			)

			--FIND, REPARENT AND DELETE EMPTY OBJECTS
			--select rootObj

			emptyObjectsArray=for i in $ where i.numVerts==0 collect i
			if emptyObjectsArray.count>0 then for obj in emptyObjectsArray do (obj.children.parent = obj.parent; obj.parent=undefined; delete obj)

			arr = findHierarchyData rootObj
			objArr = for i in arr collect i[1]

			--COLLECT DDS-DATA AND SET UV
			for i=1 to arr.count do (
			--uv = convertIndexToUV i

			xIncrement=(1.0/X_RES) -- half pixel
			yIncrement=(1.0/Y_RES) -- half pixel

			newXY=(findXYLocationOfIndex i [X_RES, Y_RES])-[.5,.5]
			mX=newXY[1]
			mY=newXY[2]
			mX*=xIncrement
			mY*=yIncrement
			mY=1.0-mY
			mX*=255.0
			mY*=255.0
			if classof arr[i][1] != Editable_Poly do convertToPoly arr[i][1]


			positions=#()
			indexArray=#()

			if arr[i][1].numVerts>0 then (
				positions = polyop.getVerts arr[i][1] #all
				indexArray = for j = 1 to positions.count collect j
				qsort indexArray compareFN valArray: positions centerpoint: (arr[i][1].pivot)

				--if i==1 then (dummy pos: positions[indexArray[1]])
				polyop.setVertColor arr[i][1] 2 #all  [mX, mY, 0] --([uv.x, -uv.y, 0]*255.0)
				)
			else (
				indexArray[1] = 1
				positions[indexArray[1]] = arr[i][1].pivot
				)

			for m = 1 to (polyop.getNumMaps arr[i][1]) do (ChannelInfo.NameChannel arr[i][1] 3 m ("UVChannel_" + m as string))
			--dummy pos: positions[indexArray[1]]
			arr[i][3] = adjustDir (positions[indexArray[1]] - arr[i][1].pivot)
			arr[i][4] = if i==1 then 0.0 else ((findItem objArr arr[i][1].parent)-1)
			arr[i][5] = ScaleFactor * (distance positions[indexArray[1]] arr[i][1].pivot)/20.48
			arr[i][6] = i-1
			arr[i][7] = (findItem objArr arr[i][1].parent)-1
			arr[i][8] = if isValidNode arr[i][1].parent then objArr[findItem objArr arr[i][1].parent].name else undefined
			pbProgressBar.value = 100.*i/arr.count

			)

			--PIVOT
			saveDDS (getArrayOfPixels arr 2) "_pivot_pos.dds" #(113, 0, 0, 0, 14, 4, 0, 0, 0, 0, 0, 0) true

			--DIR
			saveDDS (getArrayOfPixels arr 3) "_pivot_dir.dds" #(21, 0, 0, 0, 247, 3, 0, 0, 0, 0, 0, 0) true

			--for i in arr do print i
			lay = if (LayerManager.getLayerFromName "WIND")==undefined then LayerManager.newLayerFromName "WIND" else LayerManager.getLayerFromName "WIND"
			lay.current = true
			obj = copy rootObj

			deleteItem arr 1
			k=0

			--MERGE HIERARCHY INTO SINGLE OBJECT
			for i in arr do (
				k+=1
				n = snapshot i[1]
				obj.attach n obj
				--deleteItem arr i
				pbProgressBar.value = 100.*k/arr.count
				)

			obj.name = edtFinalObjectName.text + ".lod00"

				--remove decomposited objects
				local base_name = edtFinalObjectName.text
				local all_to_dell = #()
				for o in objects do (
					if (findString o.name base_name) == undefined then (
					) else (
						if (classof o == Editable_Poly) and (classof o != GrowFX2) then (
							if (o.name != obj.name) then (
								append all_to_dell o
							)

						)
					)
				)
				delete all_to_dell

			obj.material = tmp_mat
			select obj
			)
			else (
				messagebox ("The scene contains objects with the same names!\n\n" + (rootObj.name as string) + "\n\nPlease give all objects in the scene unique names!")
			)
		)
		else messagebox "Pick Root, set Texture Save Path and Final Object Name!"

	--learn
	on btnLearnF pressed do (
		local pathArr = filterString (pathConfig.removePathLeaf(getSourceFileName()) as string) "\\"
		local finalPath1 = ""
		local finalPath2 = ""
		for t = 1 to pathArr.count do (
			finalPath2 = finalPath2 + pathArr[t] + "\\"
			if t <= pathArr.count-1 then finalPath1 = finalPath1 + pathArr[t] + "\\"
		)
		finalPath1 = finalPath1 + "documentation\pivot-painter-vegetation-tool.html "
		finalPath2 = finalPath2 + "documentation\pivot-painter-vegetation-tool.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/pivot_painter_vegetation_tool.html" ""
	)

	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_Dagorppainter.ini"
	fn load_settings = (
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_Dagorppainter ((getINISetting INIfile "Dagorppainter" "DialogPos")as Point2)
			grpByPathColor.checked = ((getINISetting INIfile "Dagorppainter" "grpByPathColor")as booleanClass)
			edtTextureSavePath.text = (substituteString (getINISetting INIfile "Dagorppainter" "edtTextureSavePath") "|" "\n")
			texturesavepath = edtTextureSavePath.text
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
	)
	fn save_settings = (
		try(setINISetting INIfile "Dagorppainter" "grpByPathColor" ((grpByPathColor.checked) as string) forceUTF16:false)catch()
		try(setINISetting INIfile "Dagorppainter" "edtTextureSavePath" ((substituteString edtTextureSavePath.text "\n" "|") as string) forceUTF16:false)catch()
		try(setINISetting INIfile "Dagorppainter" "DialogPos" ((GetDialogPos GJ_Dagorppainter) as string) forceUTF16:false)catch()
	)

    on GJ_Dagorppainter open do
	(
		load_settings()
    )

	--write to ini file if dialog moved or closed
	on GJ_Dagorppainter moved new_pos do
	(
		try(setINISetting INIfile "Dagorppainter" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_Dagorppainter close do
	(
		save_settings()
		try(setINISetting INIfile "Dagorppainter" "DialogPos" ((GetDialogPos GJ_Dagorppainter) as string) forceUTF16:false)catch()
    )

	on btnSaveSettings pressed do (
		save_settings()
	)
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_Dagorppainter
)



--Script for simulation Normals Puffiness like Speed Tree v7.0 and above
--WARNING! Minimum 3ds Max 2023!
macroScript GJ_NPuff
buttontext:"Dagor Editing Normals Tool..."
category:"GJ_Tools"
IconName:"MainUI\ArrowHelper"
(
try(destroyDialog GJ_NPuff)catch()
rollout GJ_NPuff "Dagor Editing Normals Tool" width:200 height:520
(
	group "Puffiness Settings"(
		spinner Center_Puff_Pos "From Ground Center Offset " range:[0,10000,1] fieldwidth:30 scale:0.1
		button Puff_Start "Start Puffiness!" align:#center width:180 height:25 toolTip:"Starting Puffiness process"
	)
	group "Polysurface Settings"(
		spinner PSurface_size "Polysurface Size" range:[0.1,10000,3.0] fieldwidth:30 scale:0.1
		spinner PSurface_tension "Polysurface Tension" range:[0,1,0.5] fieldwidth:30 scale:0.1
		spinner PSurface_accuracy "Polysurface Accuracy" range:[0.1,10000,1.0] fieldwidth:30 scale:0.1
		checkbox PSurface_delete "Delete PSurface after Transfer" align:#left checked:true enable:true
		button PSurface_Start "Transfer Polysurface!" align:#center width:180 height:25 toolTip:"Starting generation polysurface and normal transfer from polysurface to selected objects"
	)
	group "NormalThief"(
		pickbutton pick_source "Source Object" align:#center width:180 height:25 toolTip:"Select Sourche Object with Normals"
		button bt_steal "Steal !" align:#center width:180 height:25 toolTip:"Select Target Object and Press Steal !"
	)
	group "General" (
		spinner Strn_Percent "Strength %" range:[0,100,50] fieldwidth:50 scale:5
		spinner PuffNormalLen "Display Normals Length" range:[0,100,1] fieldwidth:50 scale:0.1
		editText PuffMaskProp "Keep Normals on Material Name" fieldWidth:180 height:21 align:#center text:"*bark*,*stone,brick*,*root*" labelOnTop:true multiLine:false
		progressBar PFProgressBar "TotalExportProgress" width:180 height:16 color:(color 190 0 0) align:#center
	)
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
		finalPath1 = finalPath1 + "documentation\editing-normals-tool.html "
		finalPath2 = finalPath2 + "documentation\editing-normals-tool.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/editing_normals_tool.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)



	--main Puffiness
	on Puff_Start pressed do (
		local ObjSel = selection as array
		local Strength = (Strn_Percent.value/100) -- when set to 1.0, entirely replaces existing normal direction. at 0.0, does not adjust normal at all

		max modify mode
		if ObjSel.count > 0 then ( -- check selection
			for obj in ObjSel do (
				--obj = $

				ConvertTo obj Editable_Poly

				--get properties for save transfer to new object
				local oldObjPos = obj.pos
				local oldObjName = obj.name
				local oldObjWireC = obj.wireColor
				local oldObjProp = getUserPropBuffer obj

				obj.pos = [0,0,0]

				--reset any transforms to default. It is important for many strange imported objects transforms
				resetBox = box length:2 width:2 height:2
				resetBox.name = oldObjName
				ConvertTo resetBox Editable_Poly
				polyop.attach resetBox obj

				faceBoxList = #(1,2,3,4,5,6)
				polyop.deleteFaces resetBox faceBoxList
				select resetBox
				obj = $

				matFr = $.material

				--get class of object material top level material
				matFrClass = classof matFr
				--showProperties matFr
				local Ptext = PuffMaskProp.text as string
				if matFrClass == Multimaterial then ( -- if Multimaterial we go...
					local paternArr = (FilterString Ptext " ,*,/" splitEmptyTokens:false)
					local matNameArr = matFr.materialList
					local keepArr = #()

					--make array with preserved materials IDs by material patern name
					for a = 1 to paternArr.count do (
						for i = 1 to matNameArr.count do (
							MSubMatName = findString (matNameArr[i] as string) (paternArr[a] as string)
							if MSubMatName != undefined then (
								append keepArr (i as integer)
							)
						)
						PFProgressBar.value = 100.*a/paternArr.count
					)

					--sort only keeped array mat id and create new array with this id`s
					local allArr =#()
					for i = 1 to matNameArr.count do (
						append allArr (i as integer)
					)
					local delArr =#()
					for i = 1 to allArr.count do (
						dCand = 0
						for a = 1 to keepArr.count do (
							if keepArr[a] == allArr[i] then dCand = 1
						)
						if dCand == 0 then append delArr i

						PFProgressBar.value = 100.*i/allArr.count
					)

					--select vertex with keepd previos id`d array
					chArr = #()
					for a = 1 to delArr.count do (
						join chArr (polyop.getVertsByMatId $ delArr[a])

						PFProgressBar.value = 100.*a/delArr.count
					)
					polyop.setVertSelection $ chArr
				)
				else
				(
					--messageBox "No Multimaterial on Based Object. Used all Normals..."
				)

				modPanel.setCurrentObject obj
				enMods = (for m in obj.modifiers where classof m == editnormals collect m)
				enMod = enMods[1]

				--select all vertex if none vertex selected or keep selection
				subobjectLevel = 1 --verts
				vertarray = #{}
				vertarray = polyop.getVertSelection obj
				if vertarray.numberSet == 0 then (
					-- no selection, select all
					polyop.setVertSelection obj #all
					vertarray = polyop.getVertSelection obj
				)
				subobjectLevel = 0
				--added edit normals modifier to top of level
				if enMod == undefined then (
					enMod = editnormals()
					modPanel.addModToSelection (enMod) ui:on
					enMod.displayLength = PuffNormalLen.value
				)

				--get working pivot ifo and///not use in this moment :)
				WPTM = WorkingPivot.getTM()
				splayfrom = (WPTM[4])
				splayfrom = [0,0,Center_Puff_Pos.value]

				modPanel.setCurrentObject obj

				normarray = enMod.GetSelection()
				--enMod.MakeExplicit ()
				objpos = obj.pos
				modPanel.setCurrentObject enMod

				--progress bar values

				for vert in vertarray do (
					vertpos = (enMod.GetVertex vert node:obj) + objpos
					newnormangle = normalize (vertpos - splayfrom)
					tempbitarray = #{vert}
					norms = #{}
					enMod.ConvertVertexSelection tempbitarray norms
					enMod.MakeExplicit selection:norms

					--pbar values


					for norm in norms do (
						existingnormangle = normalize (enMod.GetNormal (norm))
						finalangle = normalize ( newnormangle * Strength + existingnormangle * (1 - Strength))
						enMod.SetNormal norm finalangle
					)
					PFProgressBar.value = 100.*vert/vertarray.count
				)

				PFProgressBar.value = 0

				--restore to new object old object custom properties
				obj.pos = oldObjPos
				obj.wireColor = oldObjWireC
				setUserPropBuffer obj oldObjProp

				subobjectLevel = 0
			)
		)
		else
		(
			messageBox "Please Select Any Edit Poly\Mesh Object"
		)
		clearNodeSelection redraw:true
	)

	--main NormalThief
	on pick_source picked obj do
	(
		if obj != undefined do pick_source.text = obj.name
	)

	--go steal!
	on bt_steal pressed do
	(
		local Strength = (Strn_Percent.value/100)
		sObj = pick_source.object
		tObj = (selection as array)[1]

		if (sObj != undefined) and (tObj != undefined) then
		(
			if (superclassof sObj == geometryclass) and (superclassof tObj == geometryclass) then
			(
				--format "Source: %\n" source
				--format "Target: %\n" target
				ConvertTo $ Editable_Poly
				matFr = $.material
				--get class of object material top level material
				matFrClass = classof matFr
				--showProperties matFr

				if matFrClass == Multimaterial then ( -- if Multimaterial we go...
					local Ptext = PuffMaskProp.text as string
					local paternArr = (FilterString Ptext " ,*,/" splitEmptyTokens:false)
					local matNameArr = matFr.materialList
					local keepArr = #()

					--make array with preserved materials IDs by material patern name
					for a = 1 to paternArr.count do (
						for i = 1 to matNameArr.count do (
							MSubMatName = findString (matNameArr[i] as string) (paternArr[a] as string)
							if MSubMatName != undefined then (
								append keepArr (i as integer)
							)
						)
						PFProgressBar.value = 100.*a/paternArr.count
					)

					--sort only keeped array mat id and create new array with this id`s
					local allArr =#()
					for i = 1 to matNameArr.count do (
						append allArr (i as integer)
					)
					local delArr =#()
					for i = 1 to allArr.count do (
						dCand = 0
						for a = 1 to keepArr.count do (
							if keepArr[a] == allArr[i] then dCand = 1
						)
						if dCand == 0 then append delArr i
						PFProgressBar.value = 100.*i/allArr.count
					)

					--select vertex with keepd previos id`d array
					chArr = #()
					for a = 1 to delArr.count do (
						join chArr (polyop.getFacesByMatId $ delArr[a])
						PFProgressBar.value = 100.*a/delArr.count
					)
					polyop.setFaceSelection $ chArr
				)
				else
				(
					--messageBox "No Multimaterial on Based Object. Skiped..."
				)

				start = timestamp()

				--get selected faces using copy instead of snapshot to keep poly :[
				selFaces = #{}
				disableRefMsgs()
				tObjCopy = copy tObj
				if (classof tObjCopy != editable_poly) and (classof tObjCopy != editable_mesh) do converttopoly tObjCopy
				selFaces = getFaceSelection tObjCopy

				--if no faces selected, select all
				if selFaces.numberset == 0 do selFaces = #{1..tObjCopy.numfaces}
				delete tObjCopy
				enableRefMsgs()

				--snapshot/xform the source object so we don't mess with it
				sMesh = snapshotasmesh sObj
				sObjSnap = editable_mesh()
				sObjSnap.mesh = sMesh
				mod_tObj = Edit_Normals ()
				mod_tObj.displayLength = PuffNormalLen.value
				mod_sObjSnap = Edit_Normals ()
				mod_sObjSnap.displayLength = 0

				addmodifier tObj mod_tObj
				addmodifier sObjSnap mod_sObjSnap

				--cache
				_getNormalID = mod_sObjSnap.GetNormalID
				_getNormal = mod_sObjSnap.GetNormal
				_getFaceDegree = mod_tObj.GetFaceDegree
				_getVertexID = mod_tObj.GetVertexID
				_getVertex = mod_tObj.GetVertex
				_getNormalIDt = mod_tObj.getNormalID
				_ConvertVertexSelection = mod_tObj.ConvertVertexSelection
				_setSelection = mod_tObj.SetSelection
				_makeExplicit = mod_tObj.MakeExplicit
				_setNormal = mod_tObj.Setnormal
				_getNormal = mod_tObj.Getnormal
				nID_Arr = #()
				nVal_Arr = #()
				hitFaceBary_Arr = #()
				faceID_Arr = #()
				done = #()

				mpi = MeshProjIntersect()
				mpi.setNode sObjSnap
				mpi.build()

				--! editNormals has to be the current selection, with modify panel on !
				select tObj --should be already selected but just in case
				tObjTransform = tObj.transform

				max modify mode

				--for each selected face...
				for f in selFaces do (
					corners = _getFaceDegree f

					--for each face corner...
					for c=1 to corners do (
						--get vertex ID
						v = _getVertexID f c
						if finditem done v == 0 do (
							try (
								--get vert closest face barycenter in source mesh
								--get vert normal ID
								pos = (_getVertex v)*tObjTransform --world pos
								mpi.closestFace pos doubleSided:true
								hitFace = mpi.GetHitFace()+1 --zero based
								bary = mpi.GetHitBary()
								nID = _getNormalIDt f c

								--? should we break the id if only 1 smoothing group ?
								append hitFaceBary_Arr #(hitFace,bary)
								append nID_Arr nID
							)
							catch (format "Error on vert:%\n" v)
							sharedNorm = #{}
							_ConvertVertexSelection #{v} sharedNorm

							--if the vertex has only 1 normal, we're done with it
							--? could be more optimized ?
							if sharedNorm.numberset == 1 do append done v
						)
					)
				)

				--get normal from source faces barycenters
				select sObjSnap
				--for each hitFace...
				for faceBary in hitFaceBary_Arr do (
					f = faceBary[1]
					bary = faceBary[2]

					--get its vertex normals
					n1 = _getNormal (_getNormalID f 1)
					n2 = _getNormal (_getNormalID f 2)
					n3 = _getNormal (_getNormalID f 3)

					--get barycenter normal
					n= (bary.x*n1)+(bary.y*n2)+(bary.z*n3)
					append nVal_Arr n
				)

				--set normals on target object
				select tObj
				subobjectLevel = 1
				disableRefMsgs()

				for i=1 to nID_Arr.count do (
					nID = nID_Arr[i]
					n = nVal_Arr[i]
					_setSelection #{nID}
					_MakeExplicit()
					oldNrm = (_getNormal nID)
					finalangle = normalize ( n * Strength + oldNrm * (1 - Strength))
					_setNormal nID finalangle
					PFProgressBar.value = 100.*i/nID_Arr.count
				)
				enableRefMsgs()

				--clean
				mpi.Free()
				delete sObjSnap
				gc light:true
				select tObj
				PFProgressBar.value = 0

			)
			else(messageBox "Souce or Target are not valid geometry objects." title:"Oops !" )
		)
		else(messageBox "Please set a Source Object and select a Target Object ." title:"Oops !" )
	)

	--go polysurface!
	on PSurface_Start pressed do
	(
		--tObj = (selection as array)[1]
		ObjSel = selection as array
		if (ObjSel.count != 0) then (
			for tObj in ObjSel do (

				select tObj

				--get properties for save transfer to new object
				local oldObjPos = tObj.pos
				local oldObjName = tObj.name
				local oldObjWireC = tObj.wireColor
				local oldObjProp = getUserPropBuffer tObj

				tObj.pos = [0,0,0]

				--reset any transforms to default. It is important for many strange imported objects transforms
				resetBox = box length:2 width:2 height:2
				resetBox.name = oldObjName
				ConvertTo resetBox Editable_Poly
				polyop.attach resetBox tObj

				faceBoxList = #(1,2,3,4,5,6)
				polyop.deleteFaces resetBox faceBoxList
				select resetBox
				tObj = $

				if (superclassof tObj == geometryclass) then
				(
					local Strength = (Strn_Percent.value/100)
					ConvertTo $ Editable_Poly

					--get class of object material top level material
					matFr = $.material
					matFrClass = classof matFr
					chArr = #()

					if matFrClass == Multimaterial then ( -- if Multimaterial we go...
						local Ptext = PuffMaskProp.text as string
						local paternArr = (FilterString Ptext " ,*,/" splitEmptyTokens:false)
						local matNameArr = matFr.materialList
						local keepArr = #()

						--make array with preserved materials IDs by material patern name
						for a = 1 to paternArr.count do (
							for i = 1 to matNameArr.count do (
								MSubMatName = findString (matNameArr[i] as string) (paternArr[a] as string)
								if MSubMatName != undefined then (
									append keepArr (i as integer)
								)
							)
							PFProgressBar.value = 100.*a/paternArr.count
						)

						--sort only keeped array mat id and create new array with this id`s
						local allArr =#()
						for i = 1 to matNameArr.count do (
							append allArr (i as integer)
						)
						local delArr =#()
						for i = 1 to allArr.count do (
							dCand = 0
							for a = 1 to keepArr.count do (
								if keepArr[a] == allArr[i] then dCand = 1
							)
							if dCand == 0 then append delArr i
							PFProgressBar.value = 100.*i/allArr.count
						)

						--select vertex with keepd previos id`d array
						for a = 1 to delArr.count do (
							join chArr (polyop.getFacesByMatId $ delArr[a])
							PFProgressBar.value = 100.*a/delArr.count
						)
						polyop.setFaceSelection $ chArr
					)
					else
					(
						--select all faces in the selected object
						chArr = #{1..$.numfaces}
						polyop.setFaceSelection $ chArr
						--messageBox "No Multimaterial on Based Object. Used all Normals..."
					)
					polyop.detachFaces $ chArr delete:false asNode: true name:"PSurface"

					--create polysurface object from selected facelist
					isoSurface = BlobMesh ()
					isoSurface.name = ("PSurface_" + tObj.name)
					isoSurface.size = PSurface_size.value
					isoSurface.tension = PSurface_tension.value
					isoSurface.viewport = PSurface_accuracy.value
					isoSurface.addBlob (getNodeByName "PSurface")
					modPanel.setCurrentObject isoSurface

					mSmooth = meshsmooth()
					addModifier isoSurface mSmooth
					mSmooth.subdivMethod = 1
					mSmooth.strength = 1
					mSmooth.Relax = 1

					ConvertTo isoSurface Editable_Poly
					isoSurface.xray = true
					isoSurface.backFaceCull = true

					--delete object created temporary from face selection
					delete (getNodeByName "PSurface")

					select tObj
					sObj = isoSurface

					--get selected faces using copy instead of snapshot to keep poly :[
					selFaces = #{}
					disableRefMsgs()
					tObjCopy = copy tObj
					if (classof tObjCopy != editable_poly) and (classof tObjCopy != editable_mesh) do converttopoly tObjCopy
					selFaces = getFaceSelection tObjCopy

					--if no faces selected, select all
					if selFaces.numberset == 0 do selFaces = #{1..tObjCopy.numfaces}
					delete tObjCopy
					enableRefMsgs()

					--snapshot/xform the source object so we don't mess with it
					sMesh = snapshotasmesh sObj
					sObjSnap = editable_mesh()
					sObjSnap.mesh = sMesh
					mod_tObj = Edit_Normals ()
					mod_tObj.displayLength = PuffNormalLen.value
					mod_sObjSnap = Edit_Normals ()
					mod_sObjSnap.displayLength = 0
					addmodifier tObj mod_tObj
					addmodifier sObjSnap mod_sObjSnap

					--cache
					_getNormalID = mod_sObjSnap.GetNormalID
					_getNormal = mod_sObjSnap.GetNormal
					_getFaceDegree = mod_tObj.GetFaceDegree
					_getVertexID = mod_tObj.GetVertexID
					_getVertex = mod_tObj.GetVertex
					_getNormalIDt = mod_tObj.getNormalID
					_ConvertVertexSelection = mod_tObj.ConvertVertexSelection
					_setSelection = mod_tObj.SetSelection
					_makeExplicit = mod_tObj.MakeExplicit
					_setNormal = mod_tObj.Setnormal
					_getNormal = mod_tObj.Getnormal
					nID_Arr = #()
					nVal_Arr = #()
					hitFaceBary_Arr = #()
					faceID_Arr = #()
					done = #()
					mpi = MeshProjIntersect()
					mpi.setNode sObjSnap
					mpi.build()

					--! editNormals has to be the current selection, with modify panel on !
					select tObj --should be already selected but just in case
					tObjTransform = tObj.transform
					max modify mode

					--for each selected face...
					for f in selFaces do (
						corners = _getFaceDegree f

						--for each face corner...
						for c=1 to corners do (
							--get vertex ID
							v = _getVertexID f c
							if finditem done v == 0 do (
								try (
									--get vert closest face barycenter in source mesh
									--get vert normal ID
									pos = (_getVertex v)*tObjTransform --world pos
									mpi.closestFace pos doubleSided:true
									hitFace = mpi.GetHitFace()+1 --zero based
									bary = mpi.GetHitBary()
									nID = _getNormalIDt f c

									--? should we break the id if only 1 smoothing group ?
									append hitFaceBary_Arr #(hitFace,bary)
									append nID_Arr nID
								)
								catch (format "Error on vert:%\n" v)
								sharedNorm = #{}
								_ConvertVertexSelection #{v} sharedNorm

								--if the vertex has only 1 normal, we're done with it
								--? could be more optimized ?
								if sharedNorm.numberset == 1 do append done v
							)
						)
					)

					--get normal from source faces barycenters
					select sObjSnap
					--for each hitFace...
					for faceBary in hitFaceBary_Arr do (
						f = faceBary[1]
						bary = faceBary[2]

						--get its vertex normals
						n1 = _getNormal (_getNormalID f 1)
						n2 = _getNormal (_getNormalID f 2)
						n3 = _getNormal (_getNormalID f 3)

						--get barycenter normal
						n= (bary.x*n1)+(bary.y*n2)+(bary.z*n3)
						append nVal_Arr n
					)

					--set normals on target object
					select tObj
					subobjectLevel = 1
					disableRefMsgs()
					for i=1 to nID_Arr.count do (
						nID = nID_Arr[i]
						n = nVal_Arr[i]
						_setSelection #{nID}
						_MakeExplicit()
						oldNrm = (_getNormal nID)
						finalangle = normalize ( n * Strength + oldNrm * (1 - Strength))
						_setNormal nID finalangle
						PFProgressBar.value = 100.*i/nID_Arr.count
					)
					enableRefMsgs()

					--clean
					mpi.Free()
					delete sObjSnap
					gc light:true
					select tObj
					PFProgressBar.value = 0

					--restore to new object old object custom properties
					tObj.pos = oldObjPos
					tObj.wireColor = oldObjWireC
					setUserPropBuffer tObj oldObjProp

					isoSurface.pos = oldObjPos
					if (PSurface_delete.checked == true ) then delete isoSurface
					clearNodeSelection redraw:true

					--reset maxscript values
					tObj = undefined
					subobjectLevel = 0
				)
				else
				(messageBox "One selected Object are not valid geometry objects." title:"Oops !" )
			)
		)
		else
		(messageBox "Please select any EditPoly or EditMesh Objects." title:"Oops !" )
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_NPuff open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_NPuff ((getINISetting INIfile "NPuff" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_NPuff moved new_pos do
	(
		try(setINISetting INIfile "NPuff" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_NPuff close do
	(
		try(setINISetting INIfile "NPuff" "DialogPos" ((GetDialogPos GJ_NPuff) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
CreateDialog GJ_NPuff style:#(#style_titlebar, #style_sysmenu, #style_toolwindow)
)

--main rollout for Degenerated Triangles Checker
macroScript GJ_DegenerateTrisCheck
buttontext:"Degenerated Triangles Checker..."
category:"GJ_Tools"
IconName:"MainUI\Compass"

(
try(destroyDialog GJ_DegenerateTrisCheck)catch()
mainPosDG = rollout GJ_DegenerateTrisCheck "Dagor Degenerate Faces" width:200 height:318
(
	group "Settings"(
		edittext min_val_deg "Minimum face area size:" align:#center width:180 fieldWidth:60 multiLine:false text:"0.000001"
		edittext min_val_angle "Minimum face angle:" align:#center width:180 fieldWidth:60 multiLine:false text:"0.01"

		checkbox chk_showlog "Show Log Window" width:180 height:15 checked:true
		checkbox chk_showSelection "Show Degenerated Face(s)" width:180 height:15 checked:true

		progressBar pbProgressBar_Total "Progress" width:180 height:10 color:(color 0 132 255) align:#center value:50
		progressBar pbProgressBar "Progress" width:180 height:10 color:(color 100 180 0) align:#center value:50

		button deg_go "Check Degenerated Triangles!" align:#center width:180 height:25 toolTip:"Check Degenerated Triangles"
		button SaveSet "Save Settings" align:#center width:180 height:25 toolTip:"Save Settings"
		button DefSet "Default Settings" align:#center width:180 height:25 toolTip:"Save Settings"
	)

		--Write settings
		fn WriteIni = (
			local IniFileName = (GetDir #userStartupScripts) + "/gj_degenerate_face_checker.ini"
			local IniFile = Createfile IniFileName encoding: #utf8

			local pos_main = GetDialogPos GJ_DegenerateTrisCheck

			format "%\n" (substituteString min_val_deg.text "\n" "")  to:IniFile
			format "%\n" (substituteString min_val_angle.text "\n" "")  to:IniFile
			format "%\n" chk_showlog.checked  to:IniFile
			format "%\n" chk_showSelection.checked  to:IniFile
			format "%\n" (GetDialogPos GJ_DegenerateTrisCheck)  to:IniFile

			close IniFile
			return IniFile
		)

		--Save current settings
		on SaveSet pressed do
		(
			WriteIni()
		)

		--Load default settings
		on DefSet pressed do
		(
			min_val_deg.text = "0.000001"
			min_val_angle.text = "0.01"
			chk_showlog.checked = true
			chk_showSelection.checked = true
			local pos_main = [200,200]
			SetDialogPos GJ_DegenerateTrisCheck pos_main
		)

		--First start save setings procedure after firs open GUI
		on GJ_DegenerateTrisCheck open do
		(
			local IniFileName = (GetDir #userStartupScripts) + "/gj_degenerate_face_checker.ini"
			if ((getfiles IniFilename).count == 0) then
			(
				--ini file not exist create default setting
				WriteIni()
			)
			else
			(

				-- ini file exist and read settings from it
				IniFile = openfile IniFileName

				min_val_deg.text = ReadLine IniFile
				min_val_angle.text = ReadLine IniFile
				chk_showlog.checked = ReadValue IniFile
				chk_showSelection.checked = ReadValue IniFile

				local pos_main = ReadValue IniFile as Point2

				SetDialogPos GJ_DegenerateTrisCheck pos_main

				close IniFile
			)
		)

	--measure angles between tree point3d
	fn measureAngle a b c =
	(
		--compute angle betwen tree point3d
		v1 = b[1] - a[1]
		v2 = c[1] - a[1]
		n1 = normalize v1
		n2 = normalize v2
		angleF = acos (dot n1 n2)
		return angleF
	)

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
		finalPath1 = finalPath1 + "documentation\check-degenerated-faces-tool.html "
		finalPath2 = finalPath2 + "documentation\check-degenerated-faces-tool.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/check_degenerated_faces_tool.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	on min_val_deg changed val do (
		local a = min_val_deg.text as double
		if a == undefined then (
			messagebox "Please enter only valid numbers! Skiped to default value..." title: "Warning!"
			min_val_deg.text = "0.000001"
		)
	)

	on min_val_angle changed val do (
		local a = min_val_angle.text as double
		if a == undefined then (
			messagebox "Please enter only valid numbers! Skiped to default value..." title: "Warning!"
			min_val_angle.text = "1.0"
		)
	)

	--main loop
	on deg_go pressed do
	(
		pbProgressBar.value = 0
		pbProgressBar_Total.value = 0

		ChrLog = "Dagor degenerative triangles searching tool v0.1\n"
		ChrLog = ChrLog + "Minimum face area size: "+ min_val_deg.text + "\n"
		ChrLog = ChrLog + "\n"
		ChrLog = ChrLog + "Checking started...\n"

		local ObjSel = selection as array
		if ObjSel.count > 0 then (
			local totalStepsTot = ObjSel.count
			local pBar_upd = 0
			for obj in ObjSel do (
				achei = 0
				quais = #()
				rb = Edit_mesh()

				--see if object is editable poly
				if classof obj == Editable_poly or classof obj == Editable_Mesh then (
					convertToMesh obj

					--for Edit Mesh or TriMesh
					if classof obj == Editable_mesh then (

						--make some magic
						obj_copy = snapshot obj
						dBox = box length:20 width:20 height:20
						convertToMesh dBox
						for a = 1 to (meshop.getNumVerts dBox)-1 do (
							meshop.deleteVerts dBox #{1}
						)
						meshop.deleteIsoVerts dBox
						convertToMesh dBox
						meshop.attach dBox obj_copy
						obj_copy = dBox
						resetXForm obj_copy
						convertToMesh obj_copy

						-- select all faces
						setFaceSelection obj_copy #{1..obj.numfaces}
						local totalSteps = obj_copy.numfaces
						for x = 1 to obj_copy.numfaces do (
							num = obj_copy.selectedfaces[x].index
							if meshop.getFaceArea obj_copy #{x} < (min_val_deg.text as double) then (
								achei = 1
								-- get index of face and put in a array
								append quais x
							)

							--get vertex ID by selected face
							vertsFaceIDs = meshop.getface obj_copy x

							--get vertexes position by index from face
							vrt1 = (meshop.getVerts obj_copy #{(vertsFaceIDs[1])})
							vrt2 = (meshop.getVerts obj_copy #{(vertsFaceIDs[2])})
							vrt3 = (meshop.getVerts obj_copy #{(vertsFaceIDs[3])})

							res1 = measureAngle vrt1 vrt2 vrt3
							res2 = measureAngle vrt2 vrt3 vrt1
							res3 = measureAngle vrt3 vrt1 vrt2

							if res1 < (min_val_angle.text as double) or res2 < (min_val_angle.text as double) or res3 < (min_val_angle.text as double) then (
								achei = 1
								-- get index of face and put in a array
								append quais x
							)
							pbProgressBar.value = 100.000 / totalSteps * x
						)

						delete obj_copy
						pbProgressBar.value = 100
						pBar_upd = pBar_upd + 1

						if achei == 0 then (
							setFaceSelection obj #{}
						)
						else
						(
							setFaceSelection obj #{}

							-- select faces using array of indexes
							setFaceSelection obj quais
							ChrLog = ChrLog + "| WARNING! | Object | " + obj.name + " | contains"+ quais.count as string + " degenerated face(s)!\n"
						)
					)

				)
				else
				(
					messagebox "The object selected isn't a Editable Poly or Editable Mesh. Skiped..." title: "Warning!"
				)
				pbProgressBar_Total.value = 100.000 / totalStepsTot * pBar_upd
			)

			if chk_showSelection.checked == true then (
				setCommandPanelTaskMode mode: #modify
				modPanel.addModToSelection (Edit_Mesh ()) ui:on
				subobjectlevel = 4
			)
			CompleteRedraw()
			pbProgressBar_Total.value = 100
		)
		else
		(
			messagebox "Please select Editable Poly or Editable Mesh objects!" title: "Warning!"
		)


		ChrLog = ChrLog + "Checking ended...\n"

		if chk_showlog.checked == true and ObjSel.count > 0 then (

			--we have errors and need open window with log
			try(form.close()) catch()
			form = dotnetobject "MaxCustomControls.Maxform"
			form.Text = "Errors Log"
			form.StartPosition = form.StartPosition.Manual
			form.Location = dotnetobject "System.Drawing.Point" 0 0
			form.Size = dotnetobject "System.Drawing.Size" 800 500
			tb = dotnetobject "RichTextBox"
			tb.Height = 800
			tb.Dock = tb.Dock.Top
			form.Controls.add tb

			form.showmodeless() (
				c = dotnetclass "System.Drawing.Color"
				fn printColored text tb colors:#(c.Red, c.Green) tokens:"	 " = (
					tb.SuspendLayout()
					txt = filterstring text tokens splitEmptyTokens:on
					tb.ForeColor = tb.ForeColor.Black
					tb.Text = text
					tb.SelectionStart = 0
					for k=0 to txt.count-1 do (
						s = txt[k+1].count
						tb.SelectionLength = s
						tb.selectionColor = colors[(mod k colors.count) + 1]
						tb.SelectionStart += s+1
					)
					tb.SelectionLength = 0
					tb.ResumeLayout()
				)

				--take text from ChrLog stored early
				printColored ChrLog tb colors:#(c.Gray, c.Red) tokens:"*|"
			)
		)

	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_DegenerateTrisCheck open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_DegenerateTrisCheck ((getINISetting INIfile "DfaceChk" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_DegenerateTrisCheck moved new_pos do
	(
		try(setINISetting INIfile "DfaceChk" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_DegenerateTrisCheck close do
	(
		try(setINISetting INIfile "DfaceChk" "DialogPos" ((GetDialogPos GJ_DegenerateTrisCheck) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
CreateDialog GJ_DegenerateTrisCheck

)


-- Create Main Menu Bar Dialogs and Menu
-- For complete earase old menu and Script Settings you need delete *.mrc MAX generated files from bottom path
-- Path where placed "temp" Max Meny *.mrc macroscrips
-- C:\Users\%UserName%\AppData\Local\Autodesk\3dsMax\20xx - 64bit\enu\UI\usermacros
(
	--check max version and create drop down menu if version below 2025
	local max_version = ((maxVersion())[1] / 1000)
	if max_version < 27 then (
		local mainMenu = menuMan.getMainMenuBar()
		local subMenu = menuMan.createMenu "GaijinTools"

		-- add Damage Fixer Menu
		local myItem1 = menuMan.createActionItem "GJDamagesSetup" "GJ_Tools"
		myItem1.setTitle "Damage Objects Setup..."
		myItem1.setUseCustomTitle true
		subMenu.addItem myItem1 -1

		-- add Remove Unused Bones
		local myItem2 = menuMan.createActionItem "GJRemoveUnusedBones" "GJ_Tools"
		myItem2.setTitle "Remove Unused Bones..."
		myItem2.setUseCustomTitle true
		subMenu.addItem myItem2 -2

		-- add Fix Normals Menu
		local myItem3 = menuMan.createActionItem "GJFixNormal" "GJ_Tools"
		myItem3.setTitle "Fix Normal Orientation..."
		myItem3.setUseCustomTitle true
		subMenu.addItem myItem3 -3

		-- add Fix Illegal Characters Menu
		local myItem4 = menuMan.createActionItem "GJFiGliff" "GJ_Tools"
		myItem4.setTitle "Fix Illegal Characters..."
		myItem4.setUseCustomTitle true
		subMenu.addItem myItem4 -4

		-- add LOD Generator
		local myItem5 = menuMan.createActionItem "GJ_LODGenerator" "GJ_Tools"
		myItem5.setTitle "LOD Generator..."
		myItem5.setUseCustomTitle true
		subMenu.addItem myItem5 -5

		-- add Icons Floater
		local myItem6 = menuMan.createActionItem "GJ_Floater" "GJ_Tools"
		myItem6.setTitle "Icons Floater..."
		myItem6.setUseCustomTitle true
		subMenu.addItem myItem6 -6

		-- add Batch DAG Import\Export
		local myItem7 = menuMan.createActionItem "GJ_IO" "GJ_Tools"
		myItem7.setTitle "Batch DAG Import\Export..."
		myItem7.setUseCustomTitle true
		subMenu.addItem myItem7 -7

		-- add Fast User Defined Properties
		local myItem8 = menuMan.createActionItem "GJ_Floater_UD" "GJ_Tools"
		myItem8.setTitle "Dagor Fast Properties..."
		myItem8.setUseCustomTitle true
		subMenu.addItem myItem8 -8

		-- add AO Baker Tool
		local myItem9 = menuMan.createActionItem "GJ_AO_Baker" "GJ_Tools"
		myItem9.setTitle "Ambient Occlusion Baker..."
		myItem9.setUseCustomTitle true
		subMenu.addItem myItem9 -9

		-- add PPainter Vegetation Tool
		local myItem10 = menuMan.createActionItem "GJ_Dagorppainter" "GJ_Tools"
		myItem10.setTitle "Pivot Painter Vegetation Tool..."
		myItem10.setUseCustomTitle true
		subMenu.addItem myItem10 -10

		-- add Normal Transfer Puffines Tool
		local myItem11 = menuMan.createActionItem "GJ_NPuff" "GJ_Tools"
		myItem11.setTitle "Dagor Editing Normals Tool..."
		myItem11.setUseCustomTitle true
		subMenu.addItem myItem11 -11

		try(destroyDialog GJ_NPuff)catch()

		-- add Vertex Position Transfer to Vertex Color
		local myItem12 = menuMan.createActionItem "GJ_VColorPosTransfer" "GJ_Tools"
		myItem12.setTitle "Dagor Vetex Pos to VColor.."
		myItem12.setUseCustomTitle true
		subMenu.addItem myItem12 -12

		-- add Degenerated Triangles Checker
		local myItem13 = menuMan.createActionItem "GJ_DegenerateTrisCheck" "GJ_Tools"
		myItem13.setTitle "Degenerated Triangles Checker..."
		myItem13.setUseCustomTitle true
		subMenu.addItem myItem13 -13

		-- add CMP Generator
		local myItem15 = menuMan.createActionItem "GJ_CMP_Gen" "GJ_Tools"
		myItem15.setTitle "CMP Export..."
		myItem15.setUseCustomTitle true
		subMenu.addItem myItem15 -15

		-- add About Menu
		local myItem14 = menuMan.createActionItem "About_GJ" "GJ_Tools"
		myItem14.setTitle "About..."
		myItem14.setUseCustomTitle true
		subMenu.addItem myItem14 -14

		local subMenuItem = menuMan.createSubMenuItem "GaijinTools" subMenu
		local exportSelectedIndex
		local addItem = true
		for	i = 1 to (mainMenu.numItems()) do
		(
			local item = mainMenu.getItem i
			local itemTitle = item.getTitle()
			if (itemTitle == "GaijinTools") then
			(
				addItem = false; exit
			)
			else if (itemTitle == "Help" or itemTitle == "&Help") then ( exportSelectedIndex = i )
		)
		if (addItem AND (exportSelectedIndex != undefined)) then
		(
			mainMenu.addItem subMenuItem (exportSelectedIndex+1)
		)
		menuMan.updateMenuBar()
	)
	else
	(
		--macros.run "GJ_Tools" "GJ_AO_Baker"
		--macros.run "GJ_Tools" "About_GJ"
		--macros.run "GJ_Tools" "GJDamagesSetup"
		--macros.run "GJ_Tools" "GJFiGliff"
		--macros.run "GJ_Tools" "GJFixNormal"
		--macros.run "GJ_Tools" "GJRemoveUnusedBones"
		--macros.run "GJ_Tools" "GJ_AO_Baker"
		--macros.run "GJ_Tools" "GJ_CMP_Gen"
		--macros.run "GJ_Tools" "GJ_Dagorppainter"
		--macros.run "GJ_Tools" "GJ_DegenerateTrisCheck"
		macros.run "GJ_Tools" "GJ_Floater"
		--macros.run "GJ_Tools" "GJ_Floater_UD"
		--macros.run "GJ_Tools" "GJ_IO" "GJ_Tools"
		--macros.run "GJ_Tools" "GJ_LODGenerator"
		--macros.run "GJ_Tools" "GJ_NPuff"
		--macros.run "GJ_Tools" "GJ_VColorPosTransfer"
	)
)
