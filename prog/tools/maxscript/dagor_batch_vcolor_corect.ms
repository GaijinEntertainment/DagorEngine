--Batch Import change Color and Export same name
--main rollout

rollout GJ_BatchVcolorCorect "Dagor VColor corection" width:200
(
	local listFiles = #()

	--interface
	button btn_directory "..." align:#center width:180 height:25
	edittext etx_LastPath "" readOnly:true
	multilistbox mlb_files ""
	button btn_all "Select All Files" align:#center width:180 height:25
	button btn_none "Deselect All Files" align:#center width:180 height:25
	progressBar pgb_load
	button btn_go "GO!" align:#center width:180 height:25

	--get directory with *.dag files
	on btn_directory pressed do (
		--local selDir = etx_LastPath.text
		local intDir = (GetDir #import)
		if (dir = getSavePath caption:"Select the directory" initialDir:intDir) != undefined do (
			etx_LastPath.text = dir
			listFiles = getFiles (dir+"/*.dag")
			mlb_files.items = for f in listFiles collect (filenameFromPath f)

			--select all files
			mlb_files.selection = #{1..(mlb_files.items.count)}
		)

	)

	--select all files
	on btn_all pressed do (
		mlb_files.selection = #{1..(mlb_files.items.count)}
	)

	--deselect all files
	on btn_none pressed do ( mlb_files.selection = #{} )

	--import\export
	on btn_go pressed do (
		local seqFiles = #()
		local sel = (mlb_files.selection as array)
		for i = 1 to sel.count do append seqFiles listFiles[sel[i]]

		--max create mode -- speed up process
		local failToLoad = #() -- for debug log

		if heapSize < 80000000L do heapSize = 80000000L -- set 80MB increase buffer for large files reading
		local oldAB = autoBackup.enabled
		autoBackup.enabled = false
		clearUndoBuffer()
		--set undo off

		--main import\export loop
		for i=1 to seqFiles.count do (
			local ObjsCount = objects.count -- need for FBX importer
			local done = false -- import process

			--import
			--select objects
			--delete selection

			if not (done = importFile seqFiles[i] #noPrompt) do
			append failToLoad seqFiles[i]

			--working with vcolor

			--export
			deselect

			pgb_load.value = 100.*i/seqFiles.count -- Update ProgressBar
		)

		--set undo on
		pgb_load.value = 0
		DialogMonitorOPS.Enabled = false
		DialogMonitorOPS.unRegisterNotification id:#autoRaname
		autoBackup.enabled = oldAB

		if failToLoad.count > 0 do ( -- log failed files to Listener
			clearListener()
			actionMan.executeAction 0 "40472"  -- open Listener
			format "The following files were not loaded:\n"
			for i in failToLoad do format "%\n" i
		)
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_BatchVcolorCorect open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_BatchVcolorCorect ((getINISetting INIfile "BatchVcolorCorect" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_BatchVcolorCorect moved new_pos do
	(
		try(setINISetting INIfile "BatchVcolorCorect" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_BatchVcolorCorect close do
	(
		try(setINISetting INIfile "BatchVcolorCorect" "DialogPos" ((GetDialogPos GJ_BatchVcolorCorect) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_BatchVcolorCorect --style:#(#style_border)