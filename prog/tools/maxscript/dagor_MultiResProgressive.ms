--GJ LOD Generator

try(DestroyDialog ::GJ_MultiRes)catch()
rollout GJ_MultiRes "Dagor LOD Generator" width:200
(
	group "Selected Objects Parameters"
	(
		spinner LodPercent "Vertex %" range:[1,100,50] fieldwidth:50 scale: 5
		spinner MResIt "Iterations" range:[1,20,10] fieldwidth:50 type:#integer
		spinner LODNum "Number of LODs" range:[0,99,1] fieldwidth:50 type:#integer
		editText objNamePostfix "LOD Postfix" fieldWidth:180 height:21 align:#center text:".lod" labelOnTop:true
		progressBar pbProgressBar "TotalExportProgress" width:180 height:16 color:(color 190 0 0) align:#center
		button btnGo "Build LODs!" align:#center width:180 height:23

	)
	group "Advanced Options"
	(	checkbox AdvOpt "Use Advanced Options" checked:false enabled:true
		checkbox ColOpt "Generate Collision" checked:true enabled:true
		checkbox ColCutOpt "Collision Cut From Top" checked:false enabled:true
		checkbox ColdMat "Collision Asign Dagor Mat" checked:true enabled:true
		checkbox AddCustomProp "Replace Custom Properties" checked:true enabled:true
		checkbox ColKeepMask "Collision Use Name Mask" checked:true enabled:true
		checkbox ColSmParts "Collision Remove Small Parts" checked:true enabled:false
	)
	group "Advanced Parameters"
	(
		spinner ColPercent "Collision Vertex %" range:[1,100,30] fieldwidth:50
		spinner ColSizePercent "Collision Part Size %" range:[1,100,10] fieldwidth:50 enabled:false
		spinner ColCutPercent "Collision Cut Height" range:[1,10000,3000] fieldwidth:50
		--spinner ColPartPercent "Keep Collision Parts %" range:[1,100,30] fieldwidth:50
		editText ColPostfix "Collision Postfix" fieldWidth:180 height:21 align:#center text:".collision" labelOnTop:true
		editText ColMaskProp "Keep Material Name" fieldWidth:180 height:21 align:#center text:"*bark*,*stone,brick*" labelOnTop:true
		editText LODCstProp "LODs Custom Properties" fieldWidth:180 height:40 align:#center text:"renderable:b=yes\ncollidable:b=no\n" labelOnTop:true
		editText ColCstProp "Collision Custom Properties" fieldWidth:180 height:40 align:#center text:"renderable:b=no\ncollidable:b=yes\ncollision:t=\"mesh\"\nphmat:t=\"wood_solid\"\nisPhysCollidable:b=yes\nisTraceable:b=yes" labelOnTop:true
	)
	group "Learning Path"
	(
		button btnLearnF "Open Local Documentation" align:#center width:180 height:25
		button btnContactF "Contact with Developer" align:#center width:180 height:25
	)

	--main loop
	on btnGo pressed do
	(
		disableSceneRedraw()
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
		totalSteps = ObjSel.count * nrm
		if ObjSel.count > 0 then ( -- check selection
			for baseObj in ObjSel do (
				for i = 1 to (LODNum.value as integer) do (
					lodObj = instance baseObj -- important for GrowFX because "copy" broken some elements

					-- important because MultiRes working proper only with EditPoly from max 2018
					convertToPoly lodObj
					-- important because work only with selected object in modify pannel
					select lodObj
					local objNFaces = lodObj.mesh.numfaces
					for r = 1 to i do (
						local currentVal  = 100
						local oneStep = 100.0*(100.0 - LodPercent.value)/(100.0*MResIt.value)
						for a = 1 to MResIt.value do (
							modPanel.addModToSelection (MultiRes ())
							local currentPercent = 100.0*(1 - oneStep / currentVal)
							currentVal = currentPercent * currentVal / 100.0
							lodObj.modifiers[1].vertexPercent = currentPercent
							lodObj.modifiers[1].reqGenerate = true
							convertToPoly lodObj
							pbProgressBar.value = 100.000 / totalSteps * p
							p = p + 1
						)
					)

					local addNrm = i as string
					if i < 10 then addNrm = "0" + (i as string)
					lodObj.name = baseObj.name + objNamePostfix.text + addNrm

					-- delete objects with zero faces count
					if lodObj.mesh.numFaces == 0 then delete lodObj

					--add custom properties	to current LOD
					if AddCustomProp.checked == true and AdvOpt.checked == true then(
						setUserPropBuffer lodObj ""
						local ParamArr = FilterString LODCstProp.text "=,\n"
						for i = 1 to ParamArr.count by 2 do (
							setUserProp lodObj ParamArr[i] ParamArr[i+1]
						)
					)
				)

				--advanced options
				totalSteps = MResIt.value*ObjSel.count
				p = 1
				if AdvOpt.checked == true and ColOpt.checked then (

					--create Collision
					collObj = instance baseObj
					convertToPoly collObj
					select collObj
					if ColKeepMask.checked == true then (
						matFr = $.material
						--get class of object material top level material
						matFrClass = classof matFr
						--showProperties matFr
						local Ptext = ColMaskProp.text as string
						if matFrClass == Multimaterial then ( -- if Multimaterial we go...
							local paternArr = (FilterString Ptext " ,*,/" splitEmptyTokens:false)
							print paternArr
							local matNameArr = matFr.materialList
							convertToPoly $
							local keepArr = #()
							--make array with preserved materials IDs by material patern name
							for a = 1 to paternArr.count do (
									for i = 1 to matNameArr.count do (
									MSubMatName = findString (matNameArr[i] as string) (paternArr[a] as string)
									if MSubMatName != undefined then (
										append keepArr (i as integer)
									)
								)
							)
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
							messageBox "No Multimaterial on Based Object. Skiped..."
						)
					)
					local currentVal  = 100
					local oneStep = 100.0*(100.0 - ColPercent.value)/(100.0*MResIt.value)
					for a = 1 to MResIt.value do (
						--optimize collision
						modPanel.addModToSelection (MultiRes ())
						local currentPercent = 100.0*(1 - oneStep / currentVal)
						currentVal = currentPercent * currentVal / 100.0
						collObj.modifiers[1].vertexPercent = currentPercent
						collObj.modifiers[1].reqGenerate = true
						convertToPoly collObj
						pbProgressBar.value = 100.000 / totalSteps * p
						p = p + 1
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

					--rename
					collObj.name = baseObj.name + ColPostfix.text

					--add custom properties	to collision
					if AddCustomProp.checked == true then(
						setUserPropBuffer collObj ""
						local ParamArr = FilterString ColCstProp.text "=,\n"
						for i = 1 to ParamArr.count by 2 do (
							setUserProp collObj ParamArr[i] ParamArr[i+1]
						)
					)

					--assign material
					if ColdMat.checked == true then(
						local pFile = (GetDir #userStartupScripts) + "/DagorLibrary.mat"
						local pFileError = doesFileExist pFile
						if pFileError == false then (
							messageBox "DagorLibrary.mat not found! PLease put DagorLibrary.mat to C:\Users\$YOUR_USER_NAME$\AppData\Local\Autodesk\3dsMax_$YOUR_3DSMAX_VERSION$\ENU\scripts\startup"
						)
						else
						(
							matLib = loadTempMaterialLibrary pFile
							$.material = matLib[1]--asign first material from lib
						)

					)
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

	--learn
	on btnLearnF pressed do (
		local pathArr = filterString (pathConfig.removePathLeaf(getSourceFileName()) as string) "\\"
		local finalPath1 = ""
		local finalPath2 = ""
		for t = 1 to pathArr.count do (
			finalPath2 = finalPath2 + pathArr[t] + "\\"
			if t <= pathArr.count-1 then finalPath1 = finalPath1 + pathArr[t] + "\\"
		)
		finalPath1 = finalPath1 + "documentation\index.html "
		finalPath2 = finalPath2 + "documentation\index.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
	)

	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_MultiRes open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_MultiRes ((getINISetting INIfile "MultiRes" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_MultiRes moved new_pos do
	(
		try(setINISetting INIfile "MultiRes" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_MultiRes close do
	(
		try(setINISetting INIfile "MultiRes" "DialogPos" ((GetDialogPos GJ_MultiRes) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_MultiRes
