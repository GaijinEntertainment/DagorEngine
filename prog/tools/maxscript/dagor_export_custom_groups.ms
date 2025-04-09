
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