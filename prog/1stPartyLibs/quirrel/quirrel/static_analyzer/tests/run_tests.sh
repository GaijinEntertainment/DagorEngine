error=""

find . -name "*.nut" > ~nuts.tmp
find . -name "*.nut.txt" >> ~nuts.tmp
find ./expected_compiler_error -name "*.txt" >> ~nuts.tmp

../../../../../../tools/dagor3_cdk/util-linux64/sq3_static_analyzer-dev \
  --csq-exe:../../../../../../tools/dagor3_cdk/util-linux64/csq-dev \
  --files:~nuts.tmp --output:analysis_log.txt

if [ $? -ne 0 ]
then
  error="BASE TEST"
else

  find . -name "*.nut.2pass" > ~nuts.tmp
  ../../../../../../tools/dagor3_cdk/util-linux64/sq3_static_analyzer-dev \
    --csq-exe:../../../../../../tools/dagor3_cdk/util-linux64/csq-dev \
    --predefinition-files:~nuts.tmp --files:~nuts.tmp --output:analysis_log.txt

  if [ $? -ne 0 ]
  then
    error="2 PASS CSQ"
  else
    ../../../../../tools/dagor3_cdk/util-linux64/sq3_static_analyzer-dev \
      "--csq-exe:../../../../../../tools/util/sq-dev" \
      --predefinition-files:~nuts.tmp --files:~nuts.tmp --output:analysis_log.txt

    if [ $? -ne 0 ]
    then
      error="2 PASS SQ"
    fi
  fi
fi


rm ~nuts.tmp

if [[ $error == "" ]]
then
  echo "OK"
  exit 0
else
  echo "FAIL:" $error
  exit 1
fi
