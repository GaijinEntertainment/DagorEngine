#include <libTools/staticGeom/staticGeometryContainer.h>

#include <EditorCore/ec_interface.h>

#include <libTools/util/strUtil.h>
#include <libTools/dtx/dtx.h>
#include <libTools/dagFileRW/dagUtil.h>
#include <winGuiWrapper/wgw_dialogs.h>

#include <osApiWrappers/dag_direct.h>

#include <coolConsole/coolConsole.h>
#include <debug/dag_debug.h>


bool StaticGeometryContainer::importDag(const char *source_dag_name, const char *dest_dag_name)
{
  CoolConsole &con = IEditorCoreEngine::get()->getConsole();
  con.startProgress();

  const bool success = replace_dag(source_dag_name, dest_dag_name, &con);

  if (success)
  {
    con.setActionDesc("Finishing import...");
    String fileName(dest_dag_name);
    simplify_fname(fileName);
    loadFromDag(fileName, &con);
  }
  else
  {
    ::dd_erase(dest_dag_name);
    wingw::message_box(wingw::MBS_EXCL, "DAG import failed", "Failed to import '%s':\n%s", dest_dag_name, get_dagutil_last_error());
  }

  con.endProgress();
  return success;
}
