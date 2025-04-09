--Script for simulation Normals Puffiness like Speed Tree v7.0 and above
--WARNING! Minimum 3ds Max 2023!

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
		finalPath1 = finalPath1 + "documentation\dagor2_max_tools\editing-normals-tool.html "
		finalPath2 = finalPath2 + "documentation\dagor2_max_tools\editing-normals-tool.html "

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
