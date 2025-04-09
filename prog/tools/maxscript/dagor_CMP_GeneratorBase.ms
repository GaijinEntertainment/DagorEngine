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
