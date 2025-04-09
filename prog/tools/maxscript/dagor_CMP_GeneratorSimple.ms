-- Simple CMP file generator from one selected Layer
-- Only for 3dsMax 2021 and above
global r_height = 940

rollout GJ_CMP_Gen "Dagor CMP Exporter" width:232 height:r_height
(
	global placeItemsList = #("No Collision Type", "Place Pivot", "Place Pivot and use Nrm", "Place 3-point (bbox)", "Place Foundation (bbox)", "Place on Water (flootable)", "Place Pivot with Rendinst Cls")
	global nodeControlParams = #("Transform Matrix3", "place_type", "rot_x","rot_y","rot_z", "offset_x", "offset_y", "offset_z", "scale", "yScale", "aboveHt")

	timer CMP_Clock "CMP_Clock" interval:1 -- 1 times a second

	group "CMP Saving Properties"
	(
		label lbl2 "Output Path" width:180 height:16
		editText edtOutput "" pos:[7,42] width:164 height:20 text: "Select Path..." readonly: true
		button btnOutput "..." pos:[178,40] width:42 height:25
		editText edtNodeType "Default Node Type" width:188 height:20 text: "rendInst" readonly: false multiLine: false
		checkbox CMP_remSuffix "Remove numeric suffixes if exist" checked:true enable:true
		button btnCMP_Gen "Export CMP" align:#center width:180 height:25
	)
	group "Transform Matrix3"
	(
		checkbox CMP_Matrix3 "Transform Matrix3 Enabled" align:#left checked:false enable:true
	)
	group "place_type"
	(
		checkbox CMP_place_type "place_type Enabled" align:#left checked:false enable:true
		spinner CMP_place_type_val "Index" fieldwidth:16 range:[0,6,1] type:#integer tooltip:"rot_x minimum value" align:#left pos:[16,234]
		label lbl3 "test" width:122 height:16 pos:[76,234]
	)
	group "rot_x"
	(
		checkbox CMP_rot_x "rot_x Enabled" align:#left checked:false enable:true
		spinner CMP_rot_x_min "rot_x" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"rot_x minimum value" align:#left
		spinner CMP_rot_x_max "rot_x_var" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"rot_x maximum value" pos:[106,300]
	)
	group "rot_y"
	(
		checkbox CMP_rot_y "rot_y Enabled" align:#left checked:false enable:true
		spinner CMP_rot_y_min "rot_y" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"rot_y minimum value" align:#left
		spinner CMP_rot_y_max "rot_y_var" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"rot_y maximum value" pos:[106,366]

	)
	group "rot_z"
	(
		checkbox CMP_rot_z "rot_z Enabled" align:#left checked:false enable:true
		spinner CMP_rot_z_min "rot_z" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"rot_z minimum value" align:#left
		spinner CMP_rot_z_max "rot_z_var" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"rot_z maximum value" pos:[106,432]

	)
	group "offset_x"
	(
		checkbox CMP_offset_x "offset_x Enabled" align:#left checked:false enable:true
		spinner CMP_offset_x_min "offset_x" fieldwidth:40 type:#float range:[-1e10,1e10,0.00001] tooltip:"offset_x minimum value" align:#left
		spinner CMP_offset_x_max "offset_x_var" fieldwidth:40 type:#float range:[-1e10,1e10,0.00001] tooltip:"offset_x maximum value" pos:[104,498]
	)
	group "offset_y"
	(
		checkbox CMP_offset_y "offset_y Enabled" align:#left checked:false enable:true
		spinner CMP_offset_y_min "offset_y" fieldwidth:40 type:#float range:[-1e10,1e10,0.00001] tooltip:"offset_y minimum value" align:#left
		spinner CMP_offset_y_max "offset_y_var" fieldwidth:40 type:#float range:[-1e10,1e10,0.00001] tooltip:"offset_y maximum value" pos:[104,564]
	)
	group "offset_z"
	(
		checkbox CMP_offset_z "offset_z Enabled" align:#left checked:false enable:true
		spinner CMP_offset_z_min "offset_z" fieldwidth:40 type:#float range:[-1e10,1e10,0.00001] tooltip:"offset_z minimum value" align:#left
		spinner CMP_offset_z_max "offset_z_var" fieldwidth:40 type:#float range:[-1e10,1e10,0.00001] tooltip:"offset_z maximum value" pos:[104,630]
	)
	group "scale"
	(
		checkbox CMP_scale "Scale Enabled" align:#left checked:false enable:true
		spinner CMP_scale_min "scale" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"scale minimum value" align:#left
		spinner CMP_scale_max "scale_var" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"scale maximum value" pos:[106,696]
	)
	group "yScale"
	(
		checkbox CMP_yScale "yScale Enabled" align:#left checked:false enable:true
		spinner CMP_yScale_min "yScale" fieldwidth:45 type:#float range:[-1e10,1e10,0.00001] tooltip:"yScale minimum value" align:#left
		spinner CMP_yScale_max "yScale_var" fieldwidth:45 type:#float range:[-1e10,1e10,0.00001] tooltip:"yScale maximum value" pos:[106,762]
	)
	group "aboveHt"
	(
		checkbox CMP_aboveHt "aboveHt Enabled" align:#left checked:false enable:true
		spinner CMP_aboveHt_val "aboveHt" fieldwidth:50 type:#float range:[-1e10,1e10,0.00001] tooltip:"aboveHt value" align:#left
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
		finalPath1 = finalPath1 + "documentation\simple-cmp-editor.html "
		finalPath2 = finalPath2 + "documentation\simple-cmp-editor.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/simple_cmp_editor.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	on btnOutput pressed do (
		DefaultDirS = (getINISetting ((GetDir #userStartupScripts) + "/GJ_WindowsPos.ini") "CMP_Generator" "CMP_Save_Path") as string

		SavePath = getSavePath caption:"Select Output Directory:" initialDir:(DefaultDirS)
		if (SavePath == undefined) then (
			--SavePath = (getINISetting ((GetDir #userStartupScripts) + "/GJ_WindowsPos.ini") "CMP_Generator" "CMP_Save_Path") as string
			SavePath = (GetDir #export) as string
			edtOutput.text = "Default 3DSMax Export Directory..."

		)
		else (
			edtOutput.text = SavePath
			--SavePath = (GetDir #export) as string
		)
	)

	--return rounded value by 0.00001 as string
	fn valRoundStr val = (
		val = ((floor ((val as float) * 10000)/10000))
		return (val as string)
	)

	on edtNodeType changed ret do (
		if edtNodeType.text == "" then (
			edtNodeType.text = "rendInst"
			messageBox "Defaut node type name is empty! Changed to default rendInst"
		)
		local tRep =  (FilterString (edtNodeType.text as string) ":;,. +-!@#$%^&*()|~`\/")
		edtNodeType.text = tRep[1] as string
	)

	--set states to GUI from user custom properties
	fn readGUIStateFromCustomProp = (

		local obj = selection[1]
		local CMP_list_params = ""
		local tmpArr = #()

		if (getUserPropBuffer obj) != undefined then (
		if (getUserPropBuffer obj) != "" then (
			CMP_list_params = FilterString (getUserPropBuffer obj) "\n" splitEmptyTokens:false

			--all existing elements in CMP_list_params
			for t = 1 to CMP_list_params.count do (
				if (findstring CMP_list_params[t] "tm:m=[") != undefined then (
					CMP_Matrix3.enabled = true
					CMP_Matrix3.checked = true
				)

				if (findstring CMP_list_params[t] "place_type:i=") != undefined then (
					CMP_place_type.enabled = true
					CMP_place_type.checked = true
					CMP_place_type_val.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_place_type_val.value = tmpArr[2] as integer
					lbl3.text = placeItemsList[(CMP_place_type_val.value + 1 as integer)]
				)

				if (findstring CMP_list_params[t] "rot_x:p2=") != undefined then (
					CMP_rot_x.enabled = true
					CMP_rot_x.checked = true
					CMP_rot_x_min.enabled = false
					CMP_rot_x_max.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_rot_x_min.value = tmpArr[2] as float
					CMP_rot_x_max.value = tmpArr[3] as float
				)

				if (findstring CMP_list_params[t] "rot_y:p2=") != undefined then (
					CMP_rot_y.enabled = true
					CMP_rot_y.checked = true
					CMP_rot_y_min.enabled = false
					CMP_rot_y_max.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_rot_y_min.value = tmpArr[2] as float
					CMP_rot_y_max.value = tmpArr[3] as float
				)

				if (findstring CMP_list_params[t] "rot_z:p2=") != undefined then (
					CMP_rot_z.enabled = true
					CMP_rot_z.checked = true
					CMP_rot_z_min.enabled = false
					CMP_rot_z_max.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_rot_z_min.value = tmpArr[2] as float
					CMP_rot_z_max.value = tmpArr[3] as float
				)

				if (findstring CMP_list_params[t] "offset_x:p2=") != undefined then (
					CMP_offset_x.enabled = true
					CMP_offset_x.checked = true
					CMP_offset_x_min.enabled = false
					CMP_offset_x_max.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_offset_x_min.value = tmpArr[2] as float
					CMP_offset_x_max.value = tmpArr[3] as float
				)

				if (findstring CMP_list_params[t] "offset_y:p2=") != undefined then (
					CMP_offset_y.enabled = true
					CMP_offset_y.checked = true
					CMP_offset_y_min.enabled = false
					CMP_offset_y_max.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_offset_y_min.value = tmpArr[2] as float
					CMP_offset_y_max.value = tmpArr[3] as float
				)

				if (findstring CMP_list_params[t] "offset_z:p2=") != undefined then (
					CMP_offset_z.enabled = true
					CMP_offset_z.checked = true
					CMP_offset_z_min.enabled = false
					CMP_offset_z_max.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_offset_z_min.value = tmpArr[2] as float
					CMP_offset_z_max.value = tmpArr[3] as float
				)

				if (findstring CMP_list_params[t] "scale:p2=") != undefined then (
					if (findstring CMP_list_params[t] "scale:p2=") == 1 then (
						CMP_scale.enabled = true
						CMP_scale.checked = true
						CMP_scale_min.enabled = false
						CMP_scale_max.enabled = true
						tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
						CMP_scale_min.value = tmpArr[2] as float
						CMP_scale_max.value = tmpArr[3] as float
					)
				)

				if (findstring CMP_list_params[t] "yScale:p2=") != undefined then (
					CMP_yScale.enabled = true
					CMP_yScale.checked = true
					CMP_yScale_min.enabled = false
					CMP_yScale_max.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_yScale_min.value = tmpArr[2] as float
					CMP_yScale_max.value = tmpArr[3] as float
				)

				if (findstring CMP_list_params[t] "aboveHt:r=") != undefined then (
					CMP_aboveHt.enabled = true
					CMP_aboveHt.checked = true
					CMP_aboveHt_val.enabled = true
					tmpArr = (FilterString CMP_list_params[t] "= , ;" splitEmptyTokens:false)
					CMP_aboveHt_val.value = tmpArr[2] as float
				)
			)
		)
		)
	)

	fn doCMP_ExistMatrix objPropC = (
		local tmp = false
		if (findString objPropC "rot_x:p2=") != undefined then tmp = true
		if (findString objPropC "rot_y:p2=") != undefined then tmp = true
		if (findString objPropC "rot_z:p2=") != undefined then tmp = true
		if (findString objPropC "offset_x:p2=") != undefined then tmp = true
		if (findString objPropC "offset_y:p2=") != undefined then tmp = true
		if (findString objPropC "offset_z:p2=") != undefined then tmp = true
		if (findString objPropC "scale:p2=") != undefined then tmp = true
		if (findString objPropC "yScale:p2=") != undefined then tmp = true
		return tmp
	)

	fn doCMP_Exist objPropC = (
		local tmp = false
		if (findString objPropC "place_type:i=") != undefined then tmp = true
		if (findString objPropC "rot_x:p2=") != undefined then tmp = true
		if (findString objPropC "rot_y:p2=") != undefined then tmp = true
		if (findString objPropC "rot_z:p2=") != undefined then tmp = true
		if (findString objPropC "offset_x:p2=") != undefined then tmp = true
		if (findString objPropC "offset_y:p2=") != undefined then tmp = true
		if (findString objPropC "offset_z:p2=") != undefined then tmp = true
		if (findString objPropC "scale:p2=") != undefined then tmp = true
		if (findString objPropC "yScale:p2=") != undefined then tmp = true
		if (findString objPropC "aboveHt:r=") != undefined then tmp = true
		if (findString objPropC "tm:m=[") != undefined then tmp = true
		return tmp
	)

	fn Matrix3ToDagor3 obj = (
		local R_x_90 = matrix3 [1,0,0] [0,0,-1] [0,1,0] [0,0,0]
		local reflect_z = matrix3 [1,0,0] [0,1,0] [0,0,-1] [0,0,0]

		local transform_matrix = R_x_90 * reflect_z
		local ObjFMatrix = transform_matrix * obj.transform

		ObjFMatrix = replace (ObjFMatrix as string) 1 9 ""
		ObjFMatrix = replace ObjFMatrix ObjFMatrix.count 1 ""

		--make array from matrix
		ObjFMatrix = filterString ObjFMatrix "[ ] , " splitEmptyTokens: false
		local x1 = ObjFMatrix[1] as float
		local y1 = ObjFMatrix[2] as float
		local z1 = ObjFMatrix[3] as float

		local x2 = ObjFMatrix[4] as float
		local y2 = ObjFMatrix[5] as float
		local z2 = ObjFMatrix[6] as float

		local x3 = ObjFMatrix[7] as float
		local y3 = ObjFMatrix[8] as float
		local z3 = ObjFMatrix[9] as float

		local x4 = ObjFMatrix[10] as float
		local y4 = ObjFMatrix[11] as float
		local z4 = ObjFMatrix[12] as float

		--swap YZ
		ObjFMatrix = ("["+x1 as string+", "+z1 as string+", "+y1 as string+"] ["+x2 as string+", "+z2 as string+", "+y2 as string+"] ["+x3 as string+", "+z3 as string+", "+y3 as string+"] ["+x4 as string+", "+z4 as string+", "+y4 as string+"]")
		return ObjFMatrix
	)

	--return rotation transform as XYZ array
	fn rotationXYZArr trns = (
		local tmp = (FilterString ((quatToEuler (trns) order:1) as string) "() ")
		return #(tmp[2] as float, tmp[3] as float, tmp[4] as float)
	)

	--change GUI parameters to actual position\rotation\scale from ONE selected object
	fn updGUIXYZParam = (
		local rotationXYZ = rotationXYZArr (selection[1].transform)
		CMP_rot_x_min.value = rotationXYZ[1]
		CMP_rot_z_min.value = rotationXYZ[2]
		CMP_rot_y_min.value = rotationXYZ[3]

		CMP_offset_x_min.value = selection[1].pos.x
		CMP_offset_y_min.value = selection[1].pos.z
		CMP_offset_z_min.value = selection[1].pos.y

		CMP_scale_min.value = selection[1].scale[1]
		CMP_yScale_min.value = selection[1].scale[3]
	)

	--get states from GUI elements and write CMP to selected object user custom properties
	fn writeGUIStateToCustomProp = (

		updGUIXYZParam()

		iState = #()
		Matrix3enable = 1
		if CMP_place_type.checked == true then (
			join iState #( "place_type:i=" + (CMP_place_type_val.value as string) + "\n")
		)

		if CMP_rot_x.enabled == true then (
			if CMP_rot_x.checked == true then (
				join iState #( "rot_x:p2=" + (valRoundStr CMP_rot_x_min.value) + ", " + (valRoundStr CMP_rot_x_max.value) + "\n")
				Matrix3enable = 0
			)
		)

		if CMP_rot_y.enabled == true then (
			if CMP_rot_y.checked == true then (
				join iState #( "rot_y:p2=" + (valRoundStr CMP_rot_y_min.value) + ", " + (valRoundStr CMP_rot_y_max.value) + "\n")
				Matrix3enable = 0
			)
		)

		if CMP_rot_z.enabled == true then (
			if CMP_rot_z.checked == true then (
				join iState #( "rot_z:p2=" + (valRoundStr CMP_rot_z_min.value) + ", " + (valRoundStr CMP_rot_z_max.value) + "\n")
				Matrix3enable = 0
			)
		)

		if CMP_offset_x.enabled == true then (
			if CMP_offset_x.checked == true then (
				join iState #( "offset_x:p2=" + (valRoundStr CMP_offset_x_min.value) + ", " + (valRoundStr CMP_offset_x_max.value) + "\n")
				Matrix3enable = 0
			)
		)

		if CMP_offset_y.enabled == true then (
			if CMP_offset_y.checked == true then (
				join iState #( "offset_y:p2=" + (valRoundStr CMP_offset_y_min.value) + ", " + (valRoundStr CMP_offset_y_max.value) + "\n")
				Matrix3enable = 0
			)
		)

		if CMP_offset_z.enabled == true then (
			if CMP_offset_z.checked == true then (
				join iState #( "offset_z:p2=" + (valRoundStr CMP_offset_z_min.value) + ", " + (valRoundStr CMP_offset_z_max.value) + "\n")
				Matrix3enable = 0
			)
		)

		if CMP_scale.enabled == true then (
			if CMP_scale.checked == true then (
				join iState #( "scale:p2=" + (valRoundStr CMP_scale_min.value) + ", " + (valRoundStr CMP_scale_max.value) + "\n")
				Matrix3enable = 0
			)
		)

		if CMP_yScale.enabled == true then (
			if CMP_yScale.checked == true then (
				join iState #( "yScale:p2=" + (valRoundStr CMP_yScale_min.value) + ", " + (valRoundStr CMP_yScale_max.value) + "\n")
				Matrix3enable = 0
			)
		)

		if CMP_aboveHt.enabled == true then (
			if CMP_aboveHt.checked == true then (
				join iState #( "aboveHt:r=" + (valRoundStr CMP_aboveHt_val.value) + "\n")
			)
		)

		--must be last
		local selObj = selection
		local toDagorProp = Matrix3ToDagor3 selObj[1]
		local ObjTranform = "tm:m=[" + toDagorProp + "]"  + "\n"

		if Matrix3enable == 1 then (
			join iState #(ObjTranform)
		)



		local CMP_Line = ""
		--all parameters readed and we need build CMP Properties lines
		if iState.count > 0 then (
			--build updated CMP_Property line

			for t = 1 to iState.count do (
				CMP_Line = CMP_Line + (iState[t] as string)
			)
		)
		else
		--custom properties not exist and we make default CMP Properties line
		(
			CMP_Line = (ObjTranform + "place_type:i=1" + "\n")
		)

		--get current another settings inside user properties and delete old CMP Properties
		local obj = selection[1]
		local filteredObjProp = ""
		local objProp = FilterString (getUserPropBuffer obj) "\n" splitEmptyTokens:false
		local cToDell = 0
		for t = 1 to objProp.count do (
			cToDell = 0

			--remove all old know properties
			if (findString objProp[t] "place_type:i=") != undefined then cToDell = 1
			if (findString objProp[t] "rot_x:p2=") != undefined then cToDell = 1
			if (findString objProp[t] "rot_y:p2=") != undefined then cToDell = 1
			if (findString objProp[t] "rot_z:p2=") != undefined then cToDell = 1
			if (findString objProp[t] "offset_x:p2=") != undefined then cToDell = 1
			if (findString objProp[t] "offset_y:p2=") != undefined then cToDell = 1
			if (findString objProp[t] "offset_z:p2=") != undefined then cToDell = 1
			if (findString objProp[t] "scale:p2=") != undefined then cToDell = 1
			if (findString objProp[t] "yScale:p2=") != undefined then cToDell = 1
			if (findString objProp[t] "aboveHt:r=") != undefined then cToDell = 1
			if (findString objProp[t] "tm:m=[") != undefined then cToDell = 1

			--and create filtered
			if cToDell == 0 then (
				filteredObjProp = filteredObjProp + objProp[t] + "\n"
			)
		)
		if filteredObjProp.count > 2 then filteredObjProp = (replace filteredObjProp (filteredObjProp.count - 2) 2 "")

		--replace with new changed parameters
		setUserPropBuffer obj (filteredObjProp + CMP_Line)
	)

	--change selected object user custom property if any GUI elements changed
	--matrix3
	on CMP_Matrix3 changed Matrix3_st do (
		if Matrix3_st == true then (
			CMP_rot_x.checked = false
			CMP_rot_x_min.enabled = false
			CMP_rot_x_max.enabled = false

			CMP_rot_y.checked = false
			CMP_rot_y_min.enabled = false
			CMP_rot_y_max.enabled = false

			CMP_rot_z.checked = false
			CMP_rot_z_min.enabled = false
			CMP_rot_z_max.enabled = false

			CMP_offset_x.checked = false
			CMP_offset_x_min.enabled = false
			CMP_offset_x_max.enabled = false

			CMP_offset_y.checked = false
			CMP_offset_y_min.enabled = false
			CMP_offset_y_max.enabled = false

			CMP_offset_z.checked = false
			CMP_offset_z_min.enabled = false
			CMP_offset_z_max.enabled = false

			CMP_scale.checked = false
			CMP_scale_min.enabled = false
			CMP_scale_max.enabled = false

			CMP_yScale.checked = false
			CMP_yScale_min.enabled = false
			CMP_yScale_max.enabled = false
		)
		writeGUIStateToCustomProp()
	)

	--place_type
	on CMP_place_type changed place_type_st do (
		if place_type_st == true then (
			CMP_place_type_val.enabled = true
		)
		if place_type_st == false then (
			CMP_place_type_val.enabled = false
		)
		writeGUIStateToCustomProp()
	)

	on CMP_place_type_val changed place_type_val_st do (
		---
		lbl3.text = placeItemsList[place_type_val_st + 1]
		writeGUIStateToCustomProp()
	)

	--rot_x
	on CMP_rot_x changed rot_x_st do (
		if rot_x_st == true then (
			CMP_Matrix3.checked = false
			CMP_rot_x_min.enabled = false
			CMP_rot_x_max.enabled = true

		)
		if rot_x_st == false then (
			CMP_rot_x_min.enabled = false
			CMP_rot_x_max.enabled = false

		)
		writeGUIStateToCustomProp()
	)
	on CMP_rot_x_min changed theState do (
		writeGUIStateToCustomProp()
	)
	on CMP_rot_x_max changed theState do (
		writeGUIStateToCustomProp()
	)

	--rot_y
	on CMP_rot_y changed rot_y_st do (
		if rot_y_st == true then (
			CMP_Matrix3.checked = false
			CMP_rot_y_min.enabled = false
			CMP_rot_y_max.enabled = true

		)
		if rot_y_st == false then (
			CMP_rot_y_min.enabled = false
			CMP_rot_y_max.enabled = false

		)
		writeGUIStateToCustomProp()
	)
	on CMP_rot_y_min changed theState do (
		writeGUIStateToCustomProp()
	)
	on CMP_rot_y_max changed theState do (
		writeGUIStateToCustomProp()
	)

	--rot_z
	on CMP_rot_z changed rot_z_st do (
		if rot_z_st == true then (
			CMP_Matrix3.checked = false
			CMP_rot_z_min.enabled = false
			CMP_rot_z_max.enabled = true

		)
		if rot_z_st == false then (
			CMP_rot_z_min.enabled = false
			CMP_rot_z_max.enabled = false

		)
		writeGUIStateToCustomProp()
	)
	on CMP_rot_z_min changed theState do (
		writeGUIStateToCustomProp()
	)
	on CMP_rot_z_max changed theState do (
		writeGUIStateToCustomProp()
	)

	--offset_x
	on CMP_offset_x changed offset_x_st do (
		if offset_x_st == true then (
			CMP_Matrix3.checked = false
			CMP_offset_x_min.enabled = false
			CMP_offset_x_max.enabled = true

		)
		if offset_x_st == false then (
			CMP_offset_x_min.enabled = false
			CMP_offset_x_max.enabled = false

		)
		writeGUIStateToCustomProp()
	)
	on CMP_offset_x_min changed theState do (
		writeGUIStateToCustomProp()
	)
	on CMP_offset_x_max changed theState do (
		writeGUIStateToCustomProp()
	)

	--offset_y
	on CMP_offset_y changed offset_y_st do (
		if offset_y_st == true then (
			CMP_Matrix3.checked = false
			CMP_offset_y_min.enabled = false
			CMP_offset_y_max.enabled = true

		)
		if offset_y_st == false then (
			CMP_offset_y_min.enabled = false
			CMP_offset_y_max.enabled = false

		)
		writeGUIStateToCustomProp()
	)
	on CMP_offset_y_min changed theState do (
		writeGUIStateToCustomProp()
	)
	on CMP_offset_y_max changed theState do (
		writeGUIStateToCustomProp()
	)

	--offset_z
	on CMP_offset_z changed offset_z_st do (
		if offset_z_st == true then (
			CMP_Matrix3.checked = false
			CMP_offset_z_min.enabled = false
			CMP_offset_z_max.enabled = true

		)
		if offset_z_st == false then (
			CMP_offset_z_min.enabled = false
			CMP_offset_z_max.enabled = false

		)
		writeGUIStateToCustomProp()
	)
	on CMP_offset_z_min changed theState do (
		writeGUIStateToCustomProp()
	)
	on CMP_offset_z_max changed theState do (
		writeGUIStateToCustomProp()
	)

	--scale
	on CMP_scale changed scale_st do (
		if scale_st == true then (
			CMP_Matrix3.checked = false
			CMP_scale_min.enabled = false
			CMP_scale_max.enabled = true

		)
		if scale_st == false then (
			CMP_scale_min.enabled = false
			CMP_scale_max.enabled = false

		)
		writeGUIStateToCustomProp()
	)
	on CMP_scale_min changed theState do (
		writeGUIStateToCustomProp()
	)
	on CMP_scale_max changed theState do (
		writeGUIStateToCustomProp()
	)

	--yScale
	on CMP_yScale changed yScale_st do (
		if yScale_st == true then (
			CMP_Matrix3.checked = false
			CMP_yScale_min.enabled = false
			CMP_yScale_max.enabled = true

		)
		if yScale_st == false then (
			CMP_yScale_min.enabled = false
			CMP_yScale_max.enabled = false

		)
		writeGUIStateToCustomProp()
	)
	on CMP_yScale_min changed theState do (
		writeGUIStateToCustomProp()
	)
	on CMP_yScale_max changed theState do (
		writeGUIStateToCustomProp()
	)

	--aboveHt
	on CMP_aboveHt changed aboveHt_st do (
		if aboveHt_st == true then (
			CMP_aboveHt_val.enabled = true

		)
		if aboveHt_st == false then (
			CMP_aboveHt_val.enabled = false
		)
		writeGUIStateToCustomProp()
	)
	on CMP_aboveHt_val changed theState do (
		writeGUIStateToCustomProp()
	)

	--disable all params if objects not selected
	fn DisableAllParams = (
		CMP_Matrix3.checked = false
		CMP_Matrix3.enabled = false

		CMP_place_type.checked = false
		CMP_place_type.enabled = false
		CMP_place_type_val.enabled = false

		CMP_rot_x.checked = false
		CMP_rot_x.enabled = false
		CMP_rot_x_min.enabled = false
		CMP_rot_x_max.enabled = false

		CMP_rot_y.checked = false
		CMP_rot_y.enabled = false
		CMP_rot_y_min.enabled = false
		CMP_rot_y_max.enabled = false

		CMP_rot_z.checked = false
		CMP_rot_z.enabled = false
		CMP_rot_z_min.enabled = false
		CMP_rot_z_max.enabled = false

		CMP_offset_x.checked = false
		CMP_offset_x.enabled = false
		CMP_offset_x_min.enabled = false
		CMP_offset_x_max.enabled = false

		CMP_offset_y.checked = false
		CMP_offset_y.enabled = false
		CMP_offset_y_min.enabled = false
		CMP_offset_y_max.enabled = false

		CMP_offset_z.checked = false
		CMP_offset_z.enabled = false
		CMP_offset_z_min.enabled = false
		CMP_offset_z_max.enabled = false

		CMP_scale.checked = false
		CMP_scale.enabled = false
		CMP_scale_min.enabled = false
		CMP_scale_max.enabled = false

		CMP_yScale.checked = false
		CMP_yScale.enabled = false
		CMP_yScale_min.enabled = false
		CMP_yScale_max.enabled = false

		CMP_aboveHt.checked = false
		CMP_aboveHt.enabled = false
		CMP_aboveHt_val.enabled = false
	)

	fn enableAllChekers = (
		CMP_Matrix3.enabled = true
		CMP_place_type.enabled = true
		CMP_rot_x.enabled = true
		CMP_rot_y.enabled = true
		CMP_rot_z.enabled = true
		CMP_offset_x.enabled = true
		CMP_offset_y.enabled = true
		CMP_offset_z.enabled = true
		CMP_scale.enabled = true
		CMP_yScale.enabled = true
		CMP_aboveHt.enabled = true
	)

	--generate from existig CMP user properties only Dagor properties
	fn WriteUserProp sObj = (
		RetArr = #()
		local CMP_list_params = ""
		CMP_list_params = FilterString (getUserPropBuffer sObj) "\n" splitEmptyTokens:false

		local ArrEnu = #()
		for s = 1 to CMP_list_params.count do (
			if (findString CMP_list_params[s] "tm:m=[") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "place_type:i=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "rot_x:p2=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "rot_y:p2=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "rot_z:p2=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "offset_x:p2=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "offset_y:p2=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "offset_z:p2=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "scale:p2=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "yScale:p2=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
			if (findString CMP_list_params[s] "aboveHt:r=") != undefined then (
				join RetArr #(CMP_list_params[s])
			)
		)
		return RetArr
	)

	--update user object properties to actual position\rotation\scale
	fn updXYZonChange = (

		--update GUI params only for first selected object
		local currentObject = 1
		for o in selection do (
			local objProp = FilterString (getUserPropBuffer o) "\n" splitEmptyTokens:false
			local tmpArr = #()

			--filter for empty cariage return lines
			/*for i = 1 to objProp.count do (
				if objProp[i] != "\r" then join tmpArr #(objProp[i])
			)
			objProp = tmpArr
			*/

			--update user prop param if exist
			local rotationXYZ = undefined
			for i = 1 to objProp.count do (

				--matrix3
				if (findstring objProp[i] "tm:m=[") != undefined then (
					local selObj = selection
					local toDagorProp = Matrix3ToDagor3 selObj[currentObject]

					objProp[i] = "tm:m=[" + toDagorProp + "]"
					currentObject = currentObject + 1
				)

				--rotate
				if (findstring objProp[i] "rot_x:p2=") != undefined then (
					tmpArr =  FilterString (objProp[i]) "=, " splitEmptyTokens:false
					rotationXYZ = rotationXYZArr (o.transform)
					objProp[i] = "rot_x:p2=" + (valRoundStr rotationXYZ[1]) + ", " + tmpArr[3]
				)
				if (findstring objProp[i] "rot_y:p2=") != undefined then (
					tmpArr =  FilterString (objProp[i]) "=, " splitEmptyTokens:false
					rotationXYZ = rotationXYZArr (o.transform)
					objProp[i] = "rot_y:p2=" + (valRoundStr rotationXYZ[3]) + ", " + tmpArr[3]
				)
				if (findstring objProp[i] "rot_z:p2=") != undefined then (
					tmpArr =  FilterString (objProp[i]) "=, " splitEmptyTokens:false
					rotationXYZ = rotationXYZArr (o.transform)
					objProp[i] = "rot_z:p2=" + (valRoundStr rotationXYZ[2]) + ", " + tmpArr[3]
				)

				--offset
				if (findstring objProp[i] "offset_x:p2=") != undefined then (
					tmpArr =  FilterString (objProp[i]) "=, " splitEmptyTokens:false
					objProp[i] = "offset_x:p2=" + (valRoundStr o.pos.x) + ", " + tmpArr[3]
				)
				if (findstring objProp[i] "offset_y:p2=") != undefined then (
					tmpArr =  FilterString (objProp[i]) "=, " splitEmptyTokens:false
					objProp[i] = "offset_y:p2=" + (valRoundStr o.pos.z) + ", " + tmpArr[3]
				)
				if (findstring objProp[i] "offset_z:p2=") != undefined then (
					tmpArr =  FilterString (objProp[i]) "=, " splitEmptyTokens:false
					objProp[i] = "offset_z:p2=" + (valRoundStr o.pos.y) + ", " + tmpArr[3]
				)

				--scale
				if (findstring objProp[i] "scale:p2=") != undefined then (
					if (findstring objProp[i] "scale:p2=") == 1 then (
						tmpArr =  FilterString (objProp[i]) "=, " splitEmptyTokens:false
						objProp[i] = "scale:p2=" + (valRoundStr o.scale[1]) + ", " + tmpArr[3]
					)
				)
				if (findstring objProp[i] "yScale:p2=") != undefined then (
					tmpArr =  FilterString (objProp[i]) "=, " splitEmptyTokens:false
					objProp[i] = "yScale:p2=" + (valRoundStr o.scale[3]) + ", " + tmpArr[3]
				)
			)

			local tmpBuff = ""
			for i = 1 to objProp.count do (
				tmpBuff = tmpBuff + objProp[i] + "\n"
			)
			--print tmpBuff
			setUserPropBuffer o tmpBuff
		)
	)

	--main CMP start here
	on btnCMP_Gen pressed do (

		local error_select = 0
		local SExplorerOpened = 0
		if SceneExplorerManager.ExplorerIsOpen "Layer Explorer" == true then
		(
			SExplorerOpened = 1
		)
		else
		(
			SceneExplorerManager.ClearAllExplorers()
			SceneExplorerManager.CreateExplorerFromDefault "Layer Explorer"
			SExplorerOpened = 1
		)

		layerExplorer = SceneExplorerManager.GetExplorer "Layer Explorer"
		items = layerExplorer.SelectedItems()

		--make selection error codes
		local SLayer = 0
		for i = 0 to (LayerManager.count - 1) do
		(
			CLayerProp = layermanager.getlayer i
			if (CLayerProp.current) == true then SLayer = i
		)


		--show warn if layer name is bad
		myLayer = LayerManager.getLayer SLayer
		myLayer.select true

		local findItems = #("_cmp" ,  "_rand" , "_gameobj")
		local errItems = 1
		for t = 1 to findItems.count do (
			local a = findstring myLayer.name findItems[t]
			if a != undefined then (
				if (a + (findItems[t].count - 1)) == (myLayer.name).count then (
					errItems = 0
				)


			)
		)
		if errItems == 1 then messageBox ("You forgot to specify the composite type in the layer postfix! It should be from the list: " + (findItems as string))

		--check for duplicated names in right selected objects
		local ObjInSelection = for a in selection collect a.name
		local duplicates = #()

		for i = 1 to ObjInSelection.count-1 while i < ObjInSelection.count do
		(
			for j = (i + 1) to ObjInSelection.count where stricmp ObjInSelection[i] ObjInSelection[j] == 0 do
			(
				append duplicates (getNodeByName ObjInSelection[i])
			)
		)
		if duplicates.count > 0 do
		(
			--duplicates exist
			--if CMP_Dup_Name.checked == true then error_select = 4
		)


		--select all objects inside current one layer
		clearSelection()
		myLayer = LayerManager.getLayer SLayer
		myLayer.select true

		--abort export if layer doesn`t have objects
		if selection.count == 0 then error_select = 3

		if error_select == 0 then
		(
			--selection ok
			--try
			--(
				--update all user properties with CMP settings to actual position\rotation\scale
				updXYZonChange()

				--fix illegal character inside current layer name if present
				local tmp =  (FilterString (myLayer.name as string) ":;,. +-!@#$%^&*()|~`\/")

				CMP_File = createfile (SavePath + "\/" + (tmp[1] as string) + ".composit.blk")
				format "%\n" "className:t=\"composit\"" to:CMP_File
				format "%\n" "" to:CMP_File
				format "%\n" "node{" to:CMP_File
				format "%\n" "  tm:m=[[1, 0, 0] [0, 1, 0] [0, 0, 1] [0, 0, 0]]" to:CMP_File

				local selObj = selection
				local selCounter = 1
				for i in selection do
				(
					--object visible
					if i.isHidden == false then (
						format "%\n" "" to:CMP_File
						format "%\n" "  node{" to:CMP_File

						if CMP_remSuffix.checked == true then (
							--check name for last tree number if existing and remove if existing
							local lastChr = i.name

							--for tree number
							local tmp = ((replace lastChr 1 (lastChr.count - 3) "") as float)
							if tmp != undefined then (
								lastChr = (replace lastChr (lastChr.count - 2) 3 "")
							)
							--two number
							local tmp = ((replace lastChr 1 (lastChr.count - 2) "") as float)
							if tmp != undefined then (
								lastChr = (replace lastChr (lastChr.count - 1) 2 "")
							)
							--one number
							local tmp = ((replace lastChr 1 (lastChr.count - 1) "") as float)
							if tmp != undefined then (
								lastChr = (replace lastChr (lastChr.count) 1 "")
							)

							--remove "_" symbol from last if exist
							for i = 1 to 2048 do (
								if lastChr[lastChr.count] == "_" then lastChr = (replace lastChr (lastChr.count) 1 "")
							)
						)

						--fix illegal character inside current object name if present
						lastChr =  (FilterString lastChr ":;,. +-!@#$%^&*()|~`\/")

						local ObjName = "    name:t=\""+ (lastChr[1] as string) + ":" + edtNodeType.text +"\""
						format "%\n" ObjName to:CMP_File

						local toDagorProp = Matrix3ToDagor3 selObj[selCounter]
						if doCMP_Exist (getUserPropBuffer i) == true then  (
							--CMP parameters exist inside user properties
							--write all finded parameters
							local toDagorProp = WriteUserProp selObj[selCounter]
							for f = 1 to toDagorProp.count do (
								format "%\n" ("    " + toDagorProp[f]) to:CMP_File
							)
						)

						if doCMP_Exist (getUserPropBuffer i) == false then  (
							--CMP Properties doesn`t exist and write default properties
							ObjTranform = "    tm:m=[" + toDagorProp +"]"
							format "%\n" ObjTranform to:CMP_File
						)

						selCounter = selCounter + 1
						format "%\n" "  }" to:CMP_File
					)
				)
				format "%\n" "}"  to:CMP_File
				close CMP_File
			--)
			--catch
			--(
				--messageBox "Can`t create CMP file! Please check you rights, Antivirus and other Magic..."
			--)
				clearSelection()
		)
		else
		(
			--selection have error
			if error_select == 3 then messageBox "Selected Layer is empty! Need any objects inside Layer..."
			if error_select == 4 then messageBox "Duplicate Objects Names! Please fix naming first..."
		)

		if SExplorerOpened == 0 then SceneExplorerManager.CloseExplorer "Layer Explorer"
	)

	--main loop update every tick
	local UpdGUI = 1
	local oldObjProp = undefined
	local objSelTick = 1
	local sObjPosOld = ""
	local sObjRotateOld = ""
	local sObjScaleOld = ""

	on CMP_Clock tick do
	(
		local sObj = selection[1]

		--update selected object position\rotation\scale inside user properties if object moved, rotated or scaled
		if sObj != undefined then (
			if classof sObj == Editable_Mesh then (

					if objSelTick == 1 then (
						sObjPosOld = sObj.pos
						sObjRotateOld = sObj.rotation
						sObjScaleOld = sObj.scale
						updXYZonChange()
						objSelTick = 0
					)
					--position check
					if sObjPosOld != sObj.pos then (
						sObjPosOld = sObj.pos
						updXYZonChange()
						objSelTick = 1
					)
					--rotation
					if sObjRotateOld != sObj.rotation then (
						sObjRotateOld = sObj.rotation
						updXYZonChange()
						objSelTick = 1
					)
					--scale
					if sObjScaleOld != sObj.scale then (
						sObjScaleOld = sObj.scale
						updXYZonChange()
						objSelTick = 1
					)

					--updXYZonChange()

					if selection.count == 1 then (
						readGUIStateFromCustomProp()
					)
					else (
						DisableAllParams()
					)

			)
		)

		if selection.count == 1 then (

			if classof sObj  == EditablePolyMesh then convertToMesh sObj
			if classof sObj == Editable_Mesh then (
				if UpdGUI == 1 then (

					--read custom prop and setup GUI
					--firs reset do disable all GUI elements
					DisableAllParams()
					enableAllChekers()

					updGUIXYZParam()
					updXYZonChange()
					--read current params for selected object
					readGUIStateFromCustomProp()

					oldObjProp = (getUserPropBuffer sObj)
					UpdGUI = 0
					--global CMP_list_params = undefined

					--warn if scale nonuniform
					local scaleErr = 0
					if ((selection[1].scale[1]*100) as integer) != ((selection[1].scale[2]*100) as integer) then scaleErr = 1
					if ((selection[1].scale[1]*100) as integer) != ((selection[1].scale[3]*100) as integer) then scaleErr = 1
					if ((selection[1].scale[2]*100) as integer) != ((selection[1].scale[3]*100) as integer) then scaleErr = 1
					if scaleErr == 1 then messageBox ("Non uniform scale in: \n" + (selection[1].name as string) + "\n! Please fix it...")
				)
				if oldObjProp != (getUserPropBuffer sObj) then (
					UpdGUI = 1
					DisableAllParams()
					updGUIXYZParam()
					readGUIStateFromCustomProp()
				)
			)
			else
			(	--object is not Editable Mesh
				if oldObjProp != (getUserPropBuffer sObj) then (
					UpdGUI = 0
					DisableAllParams()
					oldObjProp = (getUserPropBuffer sObj)
				)
			)

			if (doCMP_Exist (getUserPropBuffer sObj)) == false then (
					if classof sObj == Editable_Mesh then (
					--create default user prop for selected object
					local oldUPB = getUserPropBuffer sObj
					local toDagorProp = Matrix3ToDagor3 sObj
					local ObjTranform = "tm:m=[" + toDagorProp + "]"  + "\n"
					setUserPropBuffer sObj (oldUPB + "\n" + "place_type:i=1" + "\n" + ObjTranform)

					if UpdGUI == 0 then (
						UpdGUI = 1
						DisableAllParams()
						enableAllChekers()
						readGUIStateFromCustomProp()
					)
				)
			)
		)
		else
		(
			if UpdGUI == 0 then (

				UpdGUI = 1
				DisableAllParams()
			)
		)

	)

	--SubRollouts with objects custom properties
	subRollout theSubRollout width:194 height:315 align:#center

	-----------------------------save script window_position on background-----------------------------------------------
  global INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"

	on GJ_CMP_Gen open do
	(
		DisableAllParams()

		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_CMP_Gen ((getINISetting INIfile "CMP_Generator" "DialogPos")as Point2)
			SavePath = (getINISetting INIfile "CMP_Generator" "CMP_Save_Path") as string
			edtOutput.text = SavePath
		) catch (
			--our data not found or inin file not exist
			SavePath = (GetDir #export) as string
			edtOutput.text = "Default 3DSMax Export Directory..."
			setINISetting INIfile "CMP_Generator" "CMP_Save_Path" (SavePath as string) forceUTF16:false
			setINISetting INIfile "CMP_Generator" "DialogPos" ((GetDialogPos GJ_CMP_Gen) as string) forceUTF16:false

		)
    )

	--write to ini file if dialog moved or closed
	on GJ_CMP_Gen moved new_pos do
	(
		setINISetting INIfile "CMP_Generator" "DialogPos" (new_pos as string) forceUTF16:false
	)
    on GJ_CMP_Gen close do
	(
		setINISetting INIfile "CMP_Generator" "DialogPos" ((GetDialogPos GJ_CMP_Gen) as string) forceUTF16:false
		setINISetting INIfile "CMP_Generator" "CMP_Save_Path" (SavePath as string) forceUTF16:false
    )
	-------------------------------------------------------------------------------------------------------------------
)
createDialog GJ_CMP_Gen
