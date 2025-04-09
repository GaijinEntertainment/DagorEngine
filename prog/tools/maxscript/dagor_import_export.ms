--GJ_LayerProperties script
global rolloutArray = #() -- this array holds rollouts for created/existing rollouts and "undefined" for removed rollouts
global rolloutNumber = 1 -- global variable to act as index for the rolloutArray for properties

--assign global variables to the parameters that will be stored.
--The variables are global because the contents of the rollouts do not see each other.
global IniFileName = (GetDir #userStartupScripts) + "/gj_import_export_settings.ini"
global GJ_GlobalPathTxt = "Select Path..."
global GJ_isLods = true
global GJ_isDp = true
global GJ_isDmg = false
global GJ_MemoPath = 1
global GJ_UseFileImport = 1
global GJ_ImportType = 1

--if ini file exist read data fron him
if ((getfiles ((GetDir #userStartupScripts) + "/gj_import_export_settings.ini")).count != 0) then (
	global IniFile = openfile ((GetDir #userStartupScripts) + "/gj_import_export_settings.ini")
	global GJ_GlobalPathTxt = ReadLine IniFile
	global GJ_isLods = ReadValue IniFile
	global GJ_isDp = ReadValue IniFile
	global GJ_isDmg = ReadValue IniFile
	global GJ_MemoPath = ReadValue IniFile
	global GJ_UseFileImport = ReadValue IniFile
	global GJ_ImportType = ReadValue IniFile
	close IniFile
)

--main global rollout
try(DestroyDialog MainRollOut)catch()
rollout MainRollOut "Dagor Import/Export" width:238 height:360
(
	local opened = if opened != undefined then opened else off
	subRollout SubrolloutMain height:340 align:#center offset:[0,0]
	--button btnMainrollout1 "Button 1" align:#center width:100 offset:[0, 0] across:2
	--button btnMainrollout2 "Button 2" align:#center width:100 offset:[0, 0]
	local ui = #()
	--local ui = #(btnMainrollout1, btnMainrollout2)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on MainRollOut open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos MainRollOut ((getINISetting INIfile "Export_Import" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on MainRollOut moved new_pos do
	(
		try(setINISetting INIfile "Export_Import" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on MainRollOut close do
	(
		try(setINISetting INIfile "Export_Import" "DialogPos" ((GetDialogPos MainRollOut) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)

--Paths override code here
rollout subrollout1 "Dagor Paths Override" height:300
(
	local main_ui = MainRollOut.ui

	--create UI elements
	--Add sub rollout with objects custom properties
	subRollout theSubRollout width:190 height:250 align:#center
	button btnAddLayer "+ Add New Property" align:#center width:180 height:23

	group "Global"
	(
		edittext GlobalPathTxt "" align:#center width:180 height:20 text:GJ_GlobalPathTxt readOnly: true
		button btnSetPath "Select Global Path..." align:#center width:180 height:25
	)

	--decrease or increase global rollout size if subrollout closed or expand
	on subrollout1 rolledup state do if MainRollOut.opened do
	(
		subrol = subrollout1
		vshift = subrol.height + 1
		if state then
		(
			MainRollOut.height += vshift
			MainRollOut.SubrolloutMain.height += vshift
			main_ui.pos.y += vshift
		)
		else
		(
			MainRollOut.height -= vshift
			MainRollOut.SubrolloutMain.height -= vshift
			main_ui.pos.y -= vshift
		)
	)

	-- adds a rollout to the rolloutArray
	fn AddRolloutOpen = (
		global customLayerName = "Layer "
		global customPathName = "Your Custom Path Here... "
		local LayerNum = 0
		--global customPathName = customLayerName + (LayerNum as string)
		roCreator = rolloutCreator "dynamicRollout" (customLayerName + (LayerNum as string))
		roCreator.begin()
		global customLayerNameEdit = customLayerName + (LayerNum as string)
		roCreator.addControl #button #btnRemoveRollout "-" paramStr:"width:20 height:20 align:#left across:2"
		roCreator.addControl #editText ("Lr" + rolloutNumber as string) ("") paramStr:"fieldWidth:126 height:20 align:#right text:customLayerNameEdit"
		roCreator.addControl #editText ("LayerProperty" + rolloutNumber as string) ("") paramStr:"fieldWidth:172 height:20 align:#center labelOnTop:true text:customPathName"

		--roCreator.addControl #button #btnSelectPath "Select Custom Path..." paramStr:"width:172 height:20 align:#center"
		--roCreator.addControl #button #btnRemoveRollout "Remove Property" paramStr:"width:172 height:20 align:#center"

		roCreator.addLocal "thisRolloutNumber" init:"rolloutNumber"
		roCreator.addHandler #btnRemoveRollout #pressed codeStr:"
		RemoveSubRollout subrollout1.theSubRollout rolloutArray[thisRolloutNumber]
		--print (@Closed rollout @ + thisRolloutNumber as string)
		rolloutArray[thisRolloutNumber] = undefined
		"
		roCreator.end()
		AddSubRollout subrollout1.theSubRollout roCreator.def
		rolloutArray[rolloutNumber] = roCreator.def -- adds the created rollout into the array at the last position
		rolloutNumber = rolloutNumber + 1
		LayerNum = LayerNum + 1
	)

	--add one more Layer parameters
	on btnAddLayer pressed do (
		AddRolloutOpen()
	)

	--select global path
	on btnSetPath pressed do (
		OpenPath = getSavePath caption:"Select Input Directory:" initialDir:(getDir #export)
		if (OpenPath == undefined) then
		(
			OpenPath = #export
			GlobalPathTxt.text = "Default 3DSMax Export Directory..."
		)
		else
		(
			--change label text to selected path
			GlobalPathTxt.text = OpenPath
		)
		--update global variables for wrine ini
		GJ_GlobalPathTxt = GlobalPathTxt.text
	)

	-- shows/hides the created rollouts
	fn HideRollouts =
	(
		-- hide rollouts
		for rolloutIndex = 1 to rolloutArray.count do
		(
			if (rolloutArray[rolloutIndex] != undefined) do
			(
				RemoveSubRollout GJ_LayerProperties.theSubRollout rolloutArray[rolloutIndex]
			)
		)
		rolloutNumber = 1
	)
)

--import lod+ rollout
rollout importLod00 "Import Type"
(
	progressBar PBImport "TotalExportProgress" value:100 width:150 height:10 color:(color 50 80 190)
	button btnSaveSet "Import Linked DAG`s" align:#center width:150 height:25
	button btnLoadSet "IMPORT" align:#center width:150 height:25
)

--importAssetName
 rollout importAName "Import Type"
(
	checkbox MemoPath "Remember Paths" width:150 height:20 checked:true align:#left triState:GJ_MemoPath
	label all02 "Also Import:" align:#center width:170 height:20
	checkbutton isLods "LODs" align:#left width:55 offset:[0, 0] across:3 checked:GJ_isLods
	checkbutton isDp "_dp" align:#center width:55 offset:[0, 0] checked:GJ_isDp
	checkbutton isDmg "_dmg" align:#right width:55 offset:[0, 0] checked:GJ_isDmg
	progressBar PBImport "TotalExportProgress" value:100 width:150 height:10 color:(color 50 80 190)
	button btnLoadSet "IMPORT" align:#center width:150 height:25

	--update global variables on change
	on isLods changed state do (
		GJ_isLods = isLods.checked
	)
	on isDp changed state do (
		GJ_isDp = isDp.checked
	)
	on isDmg changed state do (
		GJ_isDmg = isDmg.checked
	)
	on MemoPath changed state do (
		GJ_MemoPath = MemoPath.triState
	)
)

--Importer code hire
rollout subrollout2 "Dagor Scene Importer" height:200
(
	local main_ui = MainRollOut.ui
	checkbox UseFileImport "Use For File\Import" width:170 height:20 checked:true align:#left triState:GJ_UseFileImport
	group "Layer Naming"
	(
		radiobuttons ImportType labels:#("LOD00+", "Asset Name") align:#left columns:1 default:GJ_ImportType
	)
	subRollout SubrolloutImport height:150 width:190 align:#center

	--update global parameters variable
	on UseFileImport changed state do (
		GJ_UseFileImport = UseFileImport.triState
	)

	--create dynamic rollouts for different import types
	on ImportType changed state do (
		if ImportType.state == 2 then (
			removeSubRollOut subrollout2.SubrolloutImport importLod00
			AddSubRollout subrollout2.SubrolloutImport importAName
			--SubrolloutImport.height = 150
		)
		if ImportType.state == 1 then (
			removeSubRollOut subrollout2.SubrolloutImport importAName
			AddSubRollout subrollout2.SubrolloutImport importLod00
			--SubrolloutImport.height = 110
		)
		--update global parameters variable
		GJ_ImportType = ImportType.state
	)

	--decrease or increase global rollout size if subrollout closed or expand
	on subrollout2 rolledup state do if MainRollOut.opened do
	(
		subrol = subrollout2
		vshift = subrol.height + 1

		if state then
		(
			MainRollOut.height += vshift
			MainRollOut.SubrolloutMain.height += vshift
			main_ui.pos.y += vshift
		)
		else
		(
			MainRollOut.height -= vshift
			MainRollOut.SubrolloutMain.height -= vshift
			main_ui.pos.y -= vshift
		)
	)
)

--Export code here
rollout subrollout3 "Dagor Scene Exporter" height:200
(
	local main_ui = MainRollOut.ui
	group "Export"
	(
		progressBar PBImport "TotalExportProgress" value:100 width:172 height:10 color:(color 50 80 190)
		button btnExportMain "EXPORT" align:#center width:180 height:25
	)

	--decrease or increase global rollout size if subrollout closed or expand
	on subrollout3 rolledup state do if MainRollOut.opened do
	(
		subrol = subrollout3
		vshift = subrol.height + 1
		if state then
		(
			MainRollOut.height += vshift
			MainRollOut.SubrolloutMain.height += vshift
			main_ui.pos.y += vshift
		)
		else
		(
			MainRollOut.height -= vshift
			MainRollOut.SubrolloutMain.height -= vshift
			main_ui.pos.y -= vshift
		)
	)

	--learn
	on btnLearnF pressed do (
		--web url to documentation
		ShellLaunch "https://info.gaijin.lan/display/DAG/Dagor+2+Fast+Editor+User+Properties" ""
	)
)

--save\load preferences code here
rollout subrollout4 "Dagor I/O Preferences" height:200
(
	local main_ui = MainRollOut.ui
	group "Settings"
	(
		button btnSaveSet "Save Current Settings" align:#center width:180 height:25
		button btnLoadSet "Load Saved Settings" align:#center width:180 height:25
		--button btnResetSet "Reset Settings to Default" align:#center width:180 height:25
	)
	group "Learning Path"
	(
		button btnLearnF "Visit to Learning Web Site" align:#center width:180 height:25
	)

	--decrease or increase global rollout size if subrollout closed or expand
	on subrollout4 rolledup state do if MainRollOut.opened do
	(
		subrol = subrollout4
		vshift = subrol.height + 1
		if state then
		(
			MainRollOut.height += vshift
			MainRollOut.SubrolloutMain.height += vshift

			main_ui.pos.y += vshift
		)
		else
		(
			MainRollOut.height -= vshift
			MainRollOut.SubrolloutMain.height -= vshift

			main_ui.pos.y -= vshift
		)
	)

	--learn
	on btnLearnF pressed do (
		--web url to documentation
		ShellLaunch "https://info.gaijin.lan/display/DAG/Dagor+2+Fast+Editor+User+Properties" ""
	)

	--Save\Load
	--Read ini file
	fn ReadIni = (
		IniFile = openfile IniFileName
		GJ_GlobalPathTxt = ReadLine IniFile
		GJ_isLods = ReadValue IniFile
		GJ_isDp = ReadValue IniFile
		GJ_isDmg = ReadValue IniFile
		GJ_MemoPath = ReadValue IniFile
		GJ_UseFileImport = ReadValue IniFile
		GJ_ImportType = ReadValue IniFile

		--recreate all sub rollouts with new loaded parameters
		removeSubRollOut MainRollOut.SubrolloutMain subrollout1
		removeSubRollOut MainRollOut.SubrolloutMain subrollout2
		removeSubRollOut MainRollOut.SubrolloutMain subrollout3
		removeSubRollOut MainRollOut.SubrolloutMain subrollout4
		addSubRollOut MainRollOut.SubrolloutMain subrollout1 rolledUp:true
		addSubRollOut MainRollOut.SubrolloutMain subrollout2
		addSubRollOut MainRollOut.SubrolloutMain subrollout3 rolledUp:true
		addSubRollOut MainRollOut.SubrolloutMain subrollout4 rolledUp:true
		if (GJ_ImportType == 2) then addSubRollOut subrollout2.SubrolloutImport importAName
		if (GJ_ImportType == 1) then addSubRollOut subrollout2.SubrolloutImport importLod00
		MainRollOut.height = 360
		MainRollOut.SubrolloutMain.height = 340
		close IniFile
	)

	--Write Ini file
	fn WriteIni = (
		local IniFileName = (GetDir #userStartupScripts) + "/gj_import_export_settings.ini"
		if ((getfiles IniFilename).count > 0) then
		(
			deleteFile IniFileName
		)

		--ini file not exist create default settings
		local IniFile = Createfile IniFileName encoding: #utf8
		format "%\n" GJ_GlobalPathTxt to:IniFile
		format "%\n" GJ_isLods to:IniFile
		format "%\n" GJ_isDp to:IniFile
		format "%\n" GJ_isDmg to:IniFile
		format "%\n" GJ_MemoPath to:IniFile
		format "%\n" GJ_UseFileImport to:IniFile
		format "%\n" GJ_ImportType to:IniFile
		close IniFile
		return IniFile
	)

	--save settings
	on btnSaveSet pressed do
	(
		WriteIni()
	)

	--load settings
	on btnLoadSet pressed do
	(
		-- ini file exist and read settings from it
		ReadIni()
	)
)

--first time creation all rollouts
createDialog MainRollOut
addSubRollOut MainRollOut.SubrolloutMain subrollout1 rolledUp:true
addSubRollOut MainRollOut.SubrolloutMain subrollout2
addSubRollOut MainRollOut.SubrolloutMain subrollout3 rolledUp:true
addSubRollOut MainRollOut.SubrolloutMain subrollout4 rolledUp:true

--create rollout on previos read global ini settings
if (GJ_ImportType == 2) then addSubRollOut subrollout2.SubrolloutImport importAName
if (GJ_ImportType == 1) then addSubRollOut subrollout2.SubrolloutImport importLod00

MainRollOut.opened = on
