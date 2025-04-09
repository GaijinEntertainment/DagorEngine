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