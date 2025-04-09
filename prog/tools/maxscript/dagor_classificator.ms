--GJClassificator

global ObjNameBase
global ObjCount
global MainObj

if GJClassificator !=undefined do destroyDialog GJClassificator
rollout GJClassificator "Dagor Boxes Finding and Classification Tool" width:200
	(
		group "1. Pick Object for Box Classification"
		(
			spinner MeshOpAngle "Angle Error Threshold" range:[0,90,3.5] fieldwidth:40 type:#float tooltip:"Coplanar Angle Threshold" align:#center
			spinner ErrorTrs "Obj Error Threshold %" range:[0 , 100 , 1.5] fieldwidth:40 type:#float tooltip:"Global Object Error Threshold" align:#center
			button pick_source "CLASSIFICATE FROM SELECTION!" width:180 height:25 align:#center
		)
		group "Progress Bar"
		(
			progressBar pbProgressBar "TotalProgress" width:180 height:10 color:(color 190 0 0) align:#center
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
			finalPath1 = finalPath1 + "documentation\index.html "
			finalPath2 = finalPath2 + "documentation\index.html "

			ShellLaunch finalPath1 ""
			ShellLaunch finalPath2 ""
			ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/index.html" ""
		)

		on btnAut pressed do (
			ShellLaunch "https://github.com/VlasovAlexey " ""
		)


		-- Main loop
		on pick_source pressed do (
			--disableSceneRedraw()

			local ObjSel = selection as array
			local i = 1
			ObjCount = 0
			for MainObj in ObjSel do (

				--copy object and remove coplanar faces,edges,vertexs
				TriObj = snapshot MainObj
				addmodifier TriObj (optimize facethreshold1:MeshOpAngle.value)
				convertToMesh TriObj

				TriObj.allEdges = true
				select TriObj
				max modify mode
				subObjectLevel = 2
				local edgeSelSet=#()
				local faceSelSet=#()
				for face = 1 to TriObj.numfaces do
				for edge = 1 to 3 do
					 --append faceSelSet (face)
				if (getedgevis TriObj face edge == false) do append edgeSelSet (((face-1)*3)+edge)

				-- Select all invisible edges and make visibly
				setedgeselection TriObj edgeSelSet
				meshOps.visibleEdge TriObj
				convertToPoly TriObj

				--select identical lengths groups of edges

				pbProgressBar.value = 100.000 / ObjSel.count * i
				i = i + 1
			)
			i = 1

			EnableSceneRedraw()
			CompleteRedraw()
		)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJClassificator open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJClassificator ((getINISetting INIfile "Classificator" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJClassificator moved new_pos do
	(
		try(setINISetting INIfile "Classificator" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJClassificator close do
	(
		try(setINISetting INIfile "Classificator" "DialogPos" ((GetDialogPos GJClassificator) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
	)
createDialog GJClassificator style:#(#style_toolwindow, #style_sysmenu)
