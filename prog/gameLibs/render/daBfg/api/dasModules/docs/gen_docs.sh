CUR_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
DAGOR_DIR=$CUR_DIR/../../../../../../..
DAS_COMP=$DAGOR_DIR/enlisted/tools/das-aot/enlisted-aot-win64-c-dev.exe
DAS_ROOT=$DAGOR_DIR/prog/1stPartyLibs/daScript
SCRIPT_FILE=$CUR_DIR/gen_docs.das

$DAS_COMP $SCRIPT_FILE -dasroot $DAS_ROOT