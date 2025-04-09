
try(DestroyDialog ::GJ_AO_Baker)catch()
rollout GJ_AO_Baker "Dagor AO Baker" width:200 height:398
(
	group "Setup AO"
	(
		colorpicker theAOColor "AO Tint Color:       " color:[255,255,255] modal:false enabled:true
		colorpicker AmbColor "AO Ambient Color:" color:[45,45,45] modal:false
		checkbox DisplayAO "Display AO on Viewport" align:#left checked:true enable:true
		checkbox beautifierAO "Use Beautifier(experemental)" align:#left checked:true enable:true
		dropdownlist MapSh "Transfer AO to Map Channel" items:#("1", "2", "3", "4", "5", "6", "7", "8", "9", "10") selection: 8

		checkbox GradEnable "Enable Gradient" align:#left checked:true enable:true
		spinner GradPow "Gradient Blend Percentages" fieldwidth:30 range:[0,100,80] type:#integer tooltip:"Gradient Blend %" align:#left
		dropdownlist GradBlend "Gradiend Blend Type" items:#("Normal", "Multiply", "Add", "Overlay") selection: 1

		progressBar pbProgressBar "Progress" width:180 height:10 color:(color 0 132 255) align:#center value:50
		button btnBakeAO "Bake Selected Object" align:#center width:180 height:25
	)


	on theAOColor changed nColor do (
		theAOColor.color = nColor
	)

	on AmbColor changed nnColor do (
		AmbColor.color = nnColor
	)

	on GradEnable changed GradEnable_st do (
		if GradEnable_st == true then (
			GradPow.enabled = true
			GradBlend.enabled = true
		)
		if GradEnable_st == false then (
			GradPow.enabled = false
			GradBlend.enabled = false
		)
	)

	--start AO baking
	on btnBakeAO pressed do (

		local obj = selection as array

		--fixing problem with inverted/broken normals by clear properties by attach to box
		local objNewList = #()
		for f in obj do (
			--first make active layer from selected objects
			local aLayer = f.layer.name
			for i = 0 to (LayerManager.count - 1) do
			(
				CLayerProp = layermanager.getlayer i
				if CLayerProp.name == aLayer then CLayerProp.current = true
			)
			--get properties for save transfer to new object
			local oldObjPos = f.pos
			local oldObjName = f.name
			local oldObjWireC = f.wireColor
			local oldObjProp = getUserPropBuffer f
			local oldObjMat = f.material
			f.pos = [0,0,0]

			--reset any transforms to default. It is important for many strange imported objects transforms
			resetBox = box length:2 width:2 height:2
			ConvertTo resetBox Editable_Poly
			polyop.attach resetBox f

			faceBoxList = #(1,2,3,4,5,6)
			polyop.deleteFaces resetBox faceBoxList

			--restore all properties from old object to new created
			resetBox.name = oldObjName
			resetBox.pos = oldObjPos
			resetBox.wireColor = oldObjWireC
			resetBox.material = oldObjMat
			setUserPropBuffer resetBox oldObjProp

			join objNewList #(resetBox)
		)
		obj = objNewList

		pbProgressBar.value = 0
		local totalSteps = Obj.count
		local currentSteps = 0
		for d in obj do (
			if d.isHidden == false then (
				local env_ambient =  ambientColor
				max select all
				--Hide Selection for GI computation
				for o in selection do
				(
					o.isGIExcluded = true
				)
				obj.isGIExcluded = false
				select d

				--setup AO material
				global AOmat = StandardMaterial()
				AOmat.name = ("AO_mat")
				AOmat.diffuse = theAOColor.color
				AOmat.ambient = theAOColor.color

				--back up material from selection
				global backMat = copy d.material
				d.material = AOmat

				--Create a shadow casting white sky light
				theLight = Skylight pos:[0,0,0] rgb:white
				theLight.multiplier = 1.0
				theLight.color = (color 255 255 255)
				theLight.on = true
				theLight.sky_mode = 1
				rays_per_sample = 20
				ray_bias = 0.005
				castShadows = true

				--setup default radiosity
				SceneRadiosity.radiosity = Radiosity();

				sceneRadiosity.radiosity.Reset true true
				sceneRadiosity.radiosity.lightUnitsUsed = 0
				sceneRadiosity.radiosity.radProcessInRenderOnly = false
				sceneRadiosity.radiosity.radInitialQuality = 85.0
				sceneRadiosity.radiosity.radGlobalRefineSteps = 0
				sceneRadiosity.radiosity.radSelectionRefineSteps = 0
				sceneRadiosity.radiosity.radFiltering = 0
				sceneRadiosity.radiosity.radDirectFiltering = 0

				sceneRadiosity.radiosity.radDisplayInViewport = true

				sceneRadiosity.radiosity.meshingEnabled = false
				sceneRadiosity.radiosity.shootDirectLights = true
				sceneRadiosity.radiosity.includePointLights = false
				sceneRadiosity.radiosity.includeLinearLights = false
				sceneRadiosity.radiosity.includeAreaLights = false
				sceneRadiosity.radiosity.includeSkylight = true
				sceneRadiosity.radiosity.includeSelfEmittingLights = true
				sceneRadiosity.radiosity.minimumSelfEmittingSize = 0.01

				sceneRadiosity.radiosity.rmReuseDirectIllumination = true
				sceneRadiosity.radiosity.rmRegather = false

				--setup radiosity render properties

				RadiosityPreferences.useAdvancedLighting = true
				RadiosityPreferences.computeAdvancedLighting = true

				--setup fastest render
				renderWidth = 1
				renderHeight = 1
				rendTimeType = 1
				rendShowVFB = false
				rendHidden = false
				ambientColor = AmbColor.color

				renderers.target = "local"
				renderers.current = RendererClass.classes[1]()

				setRenderType #selection

				--star compute radiosity
				renderSceneDialog.close()
				max quick render

				--asign lighting to map shannel 8
				utilitypanel.openutility Assign_Vertex_Colors
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Map Channel" do exit with i[1])
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Lighting &Only" do exit with i[1])
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Radiosity, Reuse Direct" do exit with i[1])
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Color by Vertex" do exit with i[1])
				UIAccessor.PressButton (for i in windows.getChildrenHWND #max where i[5]=="Assign to Selected" do exit with i[1])

				--transfer color info from ch3 to ch8
				macros.run "Tools" "Channel_Info"
				if (isValidNode d) then (
					channelInfo.CopyChannel d 3 3
					for r = 1 to 5 do channelInfo.addChannel d
					channelInfo.PasteChannel d 3 (MapSh.selected as integer)
					maxOps.CollapseNode d true
					setUserPropBuffer d ""
					setUserProp d "renderable:b" "yes"
					setUserProp d "collidable:b" "no"
				)
				global hwnd = windows.getChildHWND 0 "Map Channel Info"
				windows.sendMessage hwnd[1] 0x0010 0 0
				max modify mode

				--display AO
				if DisplayAO.checked == true then (
					setCVertMode d true
					d.vertexColorMapChannel = MapSh.selected as integer
					d.vertexColorType = #map_channel
				)

				--remove special created objects and lights
				delete theLight
				d.material = backMat
				ambientColor = env_ambient
				currentSteps = currentSteps + 1
				pbProgressBar.value = 100.000 / totalSteps * currentSteps

				--unhide all objects in scene one command
				--objects.isHidden = false
				max modify mode


				local ao_mult_color = (filterString (theAOColor.color as string) "() ")


				--add AO tint color
				VPaintMod = PaintLayerMod()
				addModifier d VPaintMod
				d.modifiers[1].mapChannel = (MapSh.selected as integer)
				d.modifiers[1].layerMode = "Multiply"
				d.modifiers[1].layerOpacity = 100

				VPaintState = VPaintMod.AcquirePaintState d
				for y = 1 to getNumVerts d do (
					VPaintState.SetVertColor y [(ao_mult_color[2] as float)/255, (ao_mult_color[3] as float)/255, (ao_mult_color[4] as float)/255, 1]
				)
				VPaintMod.ApplyPaintState d VPaintState
				convertToPoly d

				-- beautifierAO
				if beautifierAO.checked == true then (
					addModifier d VPaintMod
					d.modifiers[1].mapChannel = (MapSh.selected as integer)
					d.modifiers[1].layerMode = "Soft Light"
					d.modifiers[1].layerOpacity = 100

					VPaintState = VPaintMod.AcquirePaintState d
					for y = 1 to getNumVerts d do (
						VPaintState.SetVertColor y [(ao_mult_color[2] as float)/255, (ao_mult_color[3] as float)/255, (ao_mult_color[4] as float)/255, 1]
					)
					VPaintMod.ApplyPaintState d VPaintState
					convertToPoly d

					--second pass
					addModifier d VPaintMod
					d.modifiers[1].mapChannel = (MapSh.selected as integer)
					d.modifiers[1].layerMode = "Soft Light"
					d.modifiers[1].layerOpacity = 50

					VPaintState = VPaintMod.AcquirePaintState d
					for y = 1 to getNumVerts d do (
						VPaintState.SetVertColor y [(ao_mult_color[2] as float)/255, (ao_mult_color[3] as float)/255, (ao_mult_color[4] as float)/255, 1]
					)
					VPaintMod.ApplyPaintState d VPaintState
					convertToPoly d
				)

				--Ambient coloring
				ao_amb_color = (filterString (AmbColor.color as string) "() ")

				addModifier d VPaintMod
				d.modifiers[1].mapChannel = (MapSh.selected as integer)
				d.modifiers[1].layerMode = "Screen"
				d.modifiers[1].layerOpacity = 100

				VPaintState = VPaintMod.AcquirePaintState d
				for y = 1 to getNumVerts d do (
					VPaintState.SetVertColor y [(ao_amb_color[2] as float)/255, (ao_amb_color[3] as float)/255, (ao_amb_color[4] as float)/255, 1]
				)
				VPaintMod.ApplyPaintState d VPaintState
				convertToPoly d

				--adding gradient if enabled
				if GradEnable.checked == true then (
					d = $
					convertToPoly d
					obj_copy = copy d

					--make gradient on copied object
					nHeight = for i=1 to obj_copy.numVerts collect (polyop.getVert obj_copy i)[3]
					maxH = amax nHeight
					minH = amin nHeight

					for i=1 to obj_copy.numVerts do (
						colorVal = ( ( ( (polyop.getVert obj_copy i)[3] - 0 ) / (maxH - 0) )*255)
						if colorVal < 0.0 do (colorVal = 0.0)
						polyop.setVertSelection obj_copy i
						polyop.setvertcolor obj_copy 0 obj_copy.selectedVerts (color colorVal colorVal colorVal)
					)

					--second copy from vcolor to mapPaintState data
					VPaintMod = PaintLayerMod()
					addModifier obj_copy VPaintMod

					--create copy of paint state
					VPaintMod.mapChannel = 0
					VPaintMod.MergeVertexColors obj_copy
					VPaintState = VPaintMod.AcquirePaintState obj_copy

					--we have copied gradient state and don`t need any more copied object
					delete obj_copy

					--add same modifier to final object
					VPaintModF = PaintLayerMod()
					addModifier d VPaintModF
					VPaintModF.mapChannel = (MapSh.selected as integer)
					VPaintModF.layerMode = GradBlend.selected
					VPaintModF.layerOpacity = (GradPow.value as integer)

					--apply copied gradient data to final object
					VPaintModF.ApplyPaintState d VPaintState
				)
			)
		)
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
		finalPath1 = finalPath1 + "documentation\ambient-occlusion-baker.html "
		finalPath2 = finalPath2 + "documentation\ambient-occlusion-baker.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/ambient_occlusion_baker.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_AO_Baker open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_AO_Baker ((getINISetting INIfile "AO_Baker" "DialogPos")as Point2)
		) catch (
			--our data not found or inin file not exist and we nothing change in dialog pos
		)
    )

	--write to ini file if dialog moved or closed
	on GJ_AO_Baker moved new_pos do
	(
		try(setINISetting INIfile "AO_Baker" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_AO_Baker close do
	(
		try(setINISetting INIfile "AO_Baker" "DialogPos" ((GetDialogPos GJ_AO_Baker) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_AO_Baker
