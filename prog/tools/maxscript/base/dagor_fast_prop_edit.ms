
global rolloutArray = #() -- this array holds rollouts for created/existing rollouts and "undefined" for removed rollouts
global rolloutNumber = 1 -- global variable to act as index for the rolloutArray
global selectedPropVar = ""

--Create callback for object info overdraw
fn showObjectNames=
(
	--gw.setTransform(Matrix3 1)
	local ObjSelMaxF = 0
	for o in selection do
		(
		if ObjSelMaxF <= 20 then (
		--compute draw backgroud block size
		local ParamArrY = FilterString (getUserPropBuffer o) "\n"
		local ParamArrX = 0
		for i = 1 to ParamArrY.count do (
			local ParamArrLocalY = ParamArrY[i]
			if(ParamArrLocalY.count > ParamArrX) then ParamArrX = ParamArrLocalY.count
		)

		ParamArrLocalY = o.name
		if(ParamArrLocalY.count > ParamArrX) then ParamArrX = ParamArrLocalY.count

		--draw Rectangle
		local DrawXY = gw.TransPoint o.pos
		local DrawX = DrawXY[1]-10
		local DrawY = DrawXY[2]-15
		local rect1 = (box2 (DrawX-1) (DrawY-1) (((ParamArrX+1)*6) + 12) ((ParamArrY.count+1)*13 + 32))
		local rect2 = (box2 DrawX DrawY (((ParamArrX+1)*6) + 10) ((ParamArrY.count+1)*13 + 30))
		local rect3 = (box2 DrawX (DrawY + 26) (((ParamArrX+1)*6) + 10) ((ParamArrY.count+1)*13 + 30 - 26))

		local aColor = color 50 50 50 1
		local bColor = color 0 0 0 1
		local dColor = color 100 100 100 1

		--gw.clearScreen rect1 useBkg:true
		gw.wrect rect1 bColor
		gw.wrect rect2 aColor
		gw.wrect rect3 dColor

		--draw text
		gw.text o.pos (o.name + "\n" + "\n" + (getUserPropBuffer o)) color:white

		ObjSelMaxF = ObjSelMaxF + 1
		--print (getUserPropBuffer o)
		)
	)
	gw.updateScreen()
)
--registerRedrawViewsCallback showObjectNames
--showObjectNames()
unRegisterRedrawViewsCallback showObjectNames

--main rollout for Fast Properties
try(DestroyDialog ::GJ_Floater_UD)catch()
rollout GJ_Floater_UD "Dagor Fast Properties" width:200 height:600
(

	timer MainLoopClock "MainLoopClock" interval:1 -- 10 times a second

	local isKeyDown = (dotNetClass "managedservices.keyboard").isKeyDown
	local keys = dotNetClass "system.windows.forms.keys"

	group "Selected Objects Parameters"
	(
		button btnCopyF "Copy Obj User Defined" align:#center width:180 height:23
		button btnPasteF "Paste Obj User Defined" align:#center width:180 height:23
		button btnClearF "Clear Obj User Defined" align:#center width:180 height:23
		button btnVColorOn "Vertex Color ON" align:#center width:180 height:23
		button btnVColorOff "Vertex Color OFF" align:#center width:180 height:23
		editText objF_param "" fieldWidth:180 height:0 align:#center text:"Please select any objects" labelOnTop:true
		checkbox ParamFloatingShow "Display Properties on Viewport" align:#left checked:true enable:true
		--checkbox ParamFloatingShowAlways "Display Properties Always" align:#left checked:false enable:true
		--checkbox VColorShow "Viewport Vertex Color" align:#left checked:false enable:true
		--checkbox BBoxShow "Display as Box" align:#left checked:false enable:true
	)

	-- adds a rollout to the rolloutArray
	fn AddRolloutOpen = (
	local ObjSelMax = 0
		for i in selection do
		(	if ObjSelMax <= 9 then(
			global customDefM = getUserPropBuffer i

			--compute block height
			global ParamArrY = FilterString (getUserPropBuffer i) "\n"
			ParamArrY = (ParamArrY.count*13) + 20

			roCreator = rolloutCreator "dynamicRollout" (i.name as string)
			roCreator.begin()
			roCreator.addControl #editText ("CustomDef" + rolloutNumber as string) ("") paramStr:"fieldWidth:180 height:ParamArrY align:#center labelOnTop:true text:customDefM"
			--roCreator.addControl #button #btnRemoveRollout "Copy This Properties"

			roCreator.addLocal "thisRolloutNumber" init:"rolloutNumber"
			roCreator.addHandler #btnRemoveRollout #pressed codeStr:"
			RemoveSubRollout GJ_Floater_UD.theSubRollout rolloutArray[thisRolloutNumber]
			print (@Closed rollout @ + thisRolloutNumber as string)
			rolloutArray[thisRolloutNumber] = undefined
			--print (rolloutArray.count)
			"
			roCreator.end()

			AddSubRollout GJ_Floater_UD.theSubRollout roCreator.def
			rolloutArray[rolloutNumber] = roCreator.def -- adds the created rollout into the array at the last position
			rolloutNumber += 1
			ObjSelMax +=1
			)
		)
	)

	-- shows/hides the created rollouts
	fn HideRollouts =
	(
		-- hide rollouts
		for rolloutIndex = 1 to rolloutArray.count do
		(
			if (rolloutArray[rolloutIndex] != undefined) do
			(
				RemoveSubRollout GJ_Floater_UD.theSubRollout rolloutArray[rolloutIndex]
			)
		)
		rolloutNumber = 1
	)

		fn deleteUserProp node = (
		setUserPropBuffer node ""
	)

	--copy first selected object custopm properties to global variable
	on btnCopyF pressed do (
		if selection.count > 0 then
		(
			local ObjSelF = selection as array
			selectedPropVar = getUserPropBuffer ObjSelF[1]
			setclipboardText selectedPropVar
			--print selectedPropVar
		)
		else
		(
			messageBox "Please Select Any Object"
		)

	)
	--paste custopm properties of first selected object from global var
	on btnPasteF pressed do (

		if selection.count > 0 then
		(
			local ObjSelF = selection as array
			for i in ObjSelF do (
				deleteUserProp i
			)

			selectedPropVar = getclipboardText()
			if selectedPropVar == undefined then selectedPropVar = ""

			for i = 1 to ObjSelF.count do (
				setUserPropBuffer ObjSelF[i] selectedPropVar
			)
			HideRollouts()
			AddRolloutOpen()
		)
		else
		(
			messageBox "Please Select Any Object"
		)
	)

	--clear custopm properties on selected objects
	on btnClearF pressed do (

		if selection.count > 0 then
		(
			local ObjSelF = selection as array
			for i in ObjSelF do (
				deleteUserProp i
			)
			HideRollouts()
			AddRolloutOpen()
		)
		else
		(
			messageBox "Please Select Any Object"
		)
	)

	--show\hide on viewport selected objects user defined properties and objects names
	on ParamFloatingShow changed theState do (
		if ParamFloatingShow.checked == false then (
			unregisterRedrawViewsCallback showObjectNames
			showObjectNames()
		)
		else
		(
			registerRedrawViewsCallback showObjectNames
			showObjectNames()
		)
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
		finalPath1 = finalPath1 + "documentation\fast-editor-user-properties.html "
		finalPath2 = finalPath2 + "documentation\fast-editor-user-properties.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/fast_editor_user_properties.html" ""
	)

	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	-- show\hide vertex color on selected objects
	on btnVColorOn pressed do (
		local ObjSelF = selection as array
		for MainObjF in ObjSelF do (
			setCVertMode MainObjF true
			setShadeCVerts MainObjF false
		)
		redrawViews()
	)
	on btnVColorOff pressed do (
		local ObjSelF = selection as array
		for MainObjF in ObjSelF do (
			setCVertMode MainObjF false
			setShadeCVerts MainObjF false
		)
		redrawViews()
	)

	-- show\hide bounding boxes on selected objects
	/*on BBoxShow changed theState do (

		local ObjSelF = selection as array

		if BBoxShow.checked == false then (
			for MainObjF in ObjSelF do (
				MainObjF.boxMode = false
			)
		)
		else
		(
			for MainObjF in ObjSelF do (
				MainObjF.boxMode = true
			)
		)
		redrawViews()
	)
	*/

	global FDialogOpened = 0
	global ShiftTimer = -11
	fn MnLoop =
	(
		-- main loop
		if (keyboard.controlPressed == true	and keyboard.shiftPressed == true and FDialogOpened == 0 and ShiftTimer < -10) then
			(
				FDialogOpened = 1

				if ParamFloatingShow.checked == false then (
					unregisterRedrawViewsCallback showObjectNames
					showObjectNames()
				)
				else
				(
					registerRedrawViewsCallback showObjectNames
					showObjectNames()
				)

				HideRollouts()
				AddRolloutOpen()

				local FDPos = mouse.screenpos
				SetDialogPos GJ_Floater_UD [(FDPos[1]-100),(FDPos[2]-35)]

				ShiftTimer = 0
			)
			if (FDialogOpened == 1) then (
				ShiftTimer = ShiftTimer + 1
			)
			if (FDialogOpened == 0) then (
				ShiftTimer = ShiftTimer - 1
			)
		if (keyboard.controlPressed == true	and keyboard.shiftPressed == true and FDialogOpened == 1 and ShiftTimer > 10) then
			(
				FDialogOpened = 0
				unRegisterRedrawViewsCallback showObjectNames
				ShiftTimer = 0
				SetDialogPos GJ_Floater_UD [-1000,-1000]
			)

		-- for any other keys
		--local space_down = isKeyDown keys.Space
		--if space.checked != space_down do space.checked = space_down
	)

	--update main loop only selected times
	local ObjSelA = selection as array
	local SelectionOld = undefined
	local textTick = 0

	--main loop update every tick
	on MainLoopClock tick do
	(
		MnLoop()
		--Update text of user difined properties if selection changed
		if selection.count > 0 then (
			ObjSelA = selection as array
			if (SelectionOld as string) != (ObjSelA as string) then (
				textTick = 1
			)
			if textTick == 1 then (
					SelectionOld = deepcopy ObjSelA
					objF_param.text = (getUserPropBuffer ObjSelA[1])
				textTick = 0

				HideRollouts()
				AddRolloutOpen()
			)
		)
		else
		(
			objF_param.text = ("Please select any Objects" as string)
			HideRollouts()
		)
	)

	--Add\Remove rollouts with objects custom properties
	subRollout theSubRollout width:194 height:315 align:#center

	group "Learning Path"
	(
		button btnLearnF "Open Local Documentation" align:#center width:180 height:25
		button btnContactF "Contact with Developer" align:#center width:180 height:25
	)
	on GJ_Floater_UD close do (
		FDialogOpened = 0
		unRegisterRedrawViewsCallback showObjectNames
	)
)
createDialog GJ_Floater_UD pos:[-1000, -1000] --style:#(#style_border)