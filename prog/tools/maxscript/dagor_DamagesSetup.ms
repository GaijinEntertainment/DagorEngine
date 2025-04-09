--GJDamagesSetup

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
