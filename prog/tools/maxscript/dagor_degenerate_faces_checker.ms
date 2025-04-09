try(destroyDialog GJ_DegenerateTrisCheck)catch()
mainPosDG = rollout GJ_DegenerateTrisCheck "Dagor Degenerate Faces" width:200 height:318
(
	group "Settings"(
		edittext min_val_deg "Minimum face area size:" align:#center width:180 fieldWidth:60 multiLine:false text:"0.000001"
		edittext min_val_angle "Minimum face angle:" align:#center width:180 fieldWidth:60 multiLine:false text:"0.01"

		checkbox chk_showlog "Show Log Window" width:180 height:15 checked:true
		checkbox chk_showSelection "Show Degenerated Face(s)" width:180 height:15 checked:true

		progressBar pbProgressBar_Total "Progress" width:180 height:10 color:(color 0 132 255) align:#center value:50
		progressBar pbProgressBar "Progress" width:180 height:10 color:(color 100 180 0) align:#center value:50

		button deg_go "Check Degenerated Triangles!" align:#center width:180 height:25 toolTip:"Check Degenerated Triangles"
		button SaveSet "Save Settings" align:#center width:180 height:25 toolTip:"Save Settings"
		button DefSet "Default Settings" align:#center width:180 height:25 toolTip:"Save Settings"
	)

		--Write settings
		fn WriteIni = (
			local IniFileName = (GetDir #userStartupScripts) + "/gj_degenerate_face_checker.ini"
			local IniFile = Createfile IniFileName encoding: #utf8

			local pos_main = GetDialogPos GJ_DegenerateTrisCheck

			format "%\n" (substituteString min_val_deg.text "\n" "")  to:IniFile
			format "%\n" (substituteString min_val_angle.text "\n" "")  to:IniFile
			format "%\n" chk_showlog.checked  to:IniFile
			format "%\n" chk_showSelection.checked  to:IniFile
			format "%\n" (GetDialogPos GJ_DegenerateTrisCheck)  to:IniFile

			close IniFile
			return IniFile
		)

		--Save current settings
		on SaveSet pressed do
		(
			WriteIni()
		)

		--Load default settings
		on DefSet pressed do
		(
			min_val_deg.text = "0.000001"
			min_val_angle.text = "0.01"
			chk_showlog.checked = true
			chk_showSelection.checked = true
			local pos_main = [200,200]
			SetDialogPos GJ_DegenerateTrisCheck pos_main
		)

		--First start save setings procedure after firs open GUI
		on GJ_DegenerateTrisCheck open do
		(
			local IniFileName = (GetDir #userStartupScripts) + "/gj_degenerate_face_checker.ini"
			if ((getfiles IniFilename).count == 0) then
			(
				--ini file not exist create default setting
				WriteIni()
			)
			else
			(

				-- ini file exist and read settings from it
				IniFile = openfile IniFileName

				min_val_deg.text = ReadLine IniFile
				min_val_angle.text = ReadLine IniFile
				chk_showlog.checked = ReadValue IniFile
				chk_showSelection.checked = ReadValue IniFile

				local pos_main = ReadValue IniFile as Point2

				SetDialogPos GJ_DegenerateTrisCheck pos_main

				close IniFile
			)
		)

	--measure angles between tree point3d
	fn measureAngle a b c =
	(
		--compute angle betwen tree point3d
		v1 = b[1] - a[1]
		v2 = c[1] - a[1]
		n1 = normalize v1
		n2 = normalize v2
		angleF = acos (dot n1 n2)
		return angleF
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
		finalPath1 = finalPath1 + "documentation\check-degenerated-faces-tool.html "
		finalPath2 = finalPath2 + "documentation\check-degenerated-faces-tool.html "

		ShellLaunch finalPath1 ""
		ShellLaunch finalPath2 ""
		ShellLaunch "https://gaijinentertainment.github.io/DagorEngine/dagor-tools/addons/3ds-max/dagor-maxscript-toolbox/check_degenerated_faces_tool.html" ""
	)
	on btnContactF pressed do (
		ShellLaunch "https://github.com/VlasovAlexey " ""
	)

	on min_val_deg changed val do (
		local a = min_val_deg.text as double
		if a == undefined then (
			messagebox "Please enter only valid numbers! Skiped to default value..." title: "Warning!"
			min_val_deg.text = "0.000001"
		)
	)

	on min_val_angle changed val do (
		local a = min_val_angle.text as double
		if a == undefined then (
			messagebox "Please enter only valid numbers! Skiped to default value..." title: "Warning!"
			min_val_angle.text = "1.0"
		)
	)

	--main loop
	on deg_go pressed do
	(
		pbProgressBar.value = 0
		pbProgressBar_Total.value = 0

		ChrLog = "Dagor degenerative triangles searching tool\n"
		ChrLog = ChrLog + "Minimum face area size: "+ min_val_deg.text + "\n"
		ChrLog = ChrLog + "\n"
		ChrLog = ChrLog + "Checking started...\n"

		local ObjSel = selection as array
		if ObjSel.count > 0 then (
			local totalStepsTot = ObjSel.count
			local pBar_upd = 0
			for obj in ObjSel do (
				achei = 0
				quais = #()
				rb = Edit_mesh()

				--see if object is editable poly
				if classof obj == Editable_poly or classof obj == Editable_Mesh then (
					convertToMesh obj

					--for Edit Mesh or TriMesh
					if classof obj == Editable_mesh then (

						--make some magic
						obj_copy = snapshot obj
						dBox = box length:20 width:20 height:20
						convertToMesh dBox
						for a = 1 to (meshop.getNumVerts dBox)-1 do (
							meshop.deleteVerts dBox #{1}
						)
						meshop.deleteIsoVerts dBox
						convertToMesh dBox
						meshop.attach dBox obj_copy
						obj_copy = dBox
						resetXForm obj_copy
						convertToMesh obj_copy

						-- select all faces
						setFaceSelection obj_copy #{1..obj.numfaces}
						local totalSteps = obj_copy.numfaces
						for x = 1 to obj_copy.numfaces do (
							num = obj_copy.selectedfaces[x].index
							if meshop.getFaceArea obj_copy #{x} < (min_val_deg.text as double) then (
								achei = 1
								-- get index of face and put in a array
								append quais x
							)

							--get vertex ID by selected face
							vertsFaceIDs = meshop.getface obj_copy x

							--get vertexes position by index from face
							vrt1 = (meshop.getVerts obj_copy #{(vertsFaceIDs[1])})
							vrt2 = (meshop.getVerts obj_copy #{(vertsFaceIDs[2])})
							vrt3 = (meshop.getVerts obj_copy #{(vertsFaceIDs[3])})

							res1 = measureAngle vrt1 vrt2 vrt3
							res2 = measureAngle vrt2 vrt3 vrt1
							res3 = measureAngle vrt3 vrt1 vrt2

							if res1 < (min_val_angle.text as double) or res2 < (min_val_angle.text as double) or res3 < (min_val_angle.text as double) then (
								achei = 1
								-- get index of face and put in a array
								append quais x
							)
							pbProgressBar.value = 100.000 / totalSteps * x
						)

						delete obj_copy
						pbProgressBar.value = 100
						pBar_upd = pBar_upd + 1

						if achei == 0 then (
							setFaceSelection obj #{}
						)
						else
						(
							setFaceSelection obj #{}

							-- select faces using array of indexes
							setFaceSelection obj quais
							ChrLog = ChrLog + "| WARNING! | Object | " + obj.name + " | contains"+ quais.count as string + " degenerated face(s)!\n"
						)
					)

				)
				else
				(
					messagebox "The object selected isn't a Editable Poly or Editable Mesh. Skiped..." title: "Warning!"
				)
				pbProgressBar_Total.value = 100.000 / totalStepsTot * pBar_upd
			)

			if chk_showSelection.checked == true then (
				setCommandPanelTaskMode mode: #modify
				modPanel.addModToSelection (Edit_Mesh ()) ui:on
				subobjectlevel = 4
			)
			CompleteRedraw()
			pbProgressBar_Total.value = 100
		)
		else
		(
			messagebox "Please select Editable Poly or Editable Mesh objects!" title: "Warning!"
		)


		ChrLog = ChrLog + "Checking ended...\n"

		if chk_showlog.checked == true and ObjSel.count > 0 then (

			--we have errors and need open window with log
			try(form.close()) catch()
			form = dotnetobject "MaxCustomControls.Maxform"
			form.Text = "Errors Log"
			form.StartPosition = form.StartPosition.Manual
			form.Location = dotnetobject "System.Drawing.Point" 0 0
			form.Size = dotnetobject "System.Drawing.Size" 800 500
			tb = dotnetobject "RichTextBox"
			tb.Height = 800
			tb.Dock = tb.Dock.Top
			form.Controls.add tb

			form.showmodeless() (
				c = dotnetclass "System.Drawing.Color"
				fn printColored text tb colors:#(c.Red, c.Green) tokens:"	 " = (
					tb.SuspendLayout()
					txt = filterstring text tokens splitEmptyTokens:on
					tb.ForeColor = tb.ForeColor.Black
					tb.Text = text
					tb.SelectionStart = 0
					for k=0 to txt.count-1 do (
						s = txt[k+1].count
						tb.SelectionLength = s
						tb.selectionColor = colors[(mod k colors.count) + 1]
						tb.SelectionStart += s+1
					)
					tb.SelectionLength = 0
					tb.ResumeLayout()
				)

				--take text from ChrLog stored early
				printColored ChrLog tb colors:#(c.Gray, c.Red) tokens:"*|"
			)
		)

	)

	-----------------------------save script window_position on background-----------------------------------------------
	local INIfile = (GetDir #userStartupScripts) + "/GJ_WindowsPos.ini"
    on GJ_DegenerateTrisCheck open do
	(
		try (
			--ini file alredy exist and have our data
			SetDialogPos GJ_DegenerateTrisCheck ((getINISetting INIfile "DfaceChk" "DialogPos")as Point2)
		) catch (
			--our data not found or ini file not exist and we nothing change in dialog pos
		)
    )

	--write to inin file if dialog moved or closed
	on GJ_DegenerateTrisCheck moved new_pos do
	(
		try(setINISetting INIfile "DfaceChk" "DialogPos" (new_pos as string) forceUTF16:false)catch()
	)
    on GJ_DegenerateTrisCheck close do
	(
		try(setINISetting INIfile "DfaceChk" "DialogPos" ((GetDialogPos GJ_DegenerateTrisCheck) as string) forceUTF16:false)catch()
    )
	-------------------------------------------------------------------------------------------------------------------
)
CreateDialog GJ_DegenerateTrisCheck
