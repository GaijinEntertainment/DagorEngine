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
		finalPath1 = finalPath1 + "documentation\dagor2_max_tools\pivot-painter-vegetation-tool.html "
		finalPath2 = finalPath2 + "documentation\dagor2_max_tools\pivot-painter-vegetation-tool.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/fast_editor_user_properties.html" ""
	)

	on btnContactF pressed do (
		ShellLaunch "https://www.scriptattack.com/Contact.html" "" --scriptattack.com@gmail.com
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
