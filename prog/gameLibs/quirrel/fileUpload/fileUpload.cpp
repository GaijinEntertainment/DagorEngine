// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <asyncHTTPClient/asyncHTTPClient.h>
#include <sqmodules/sqmodules.h>
#include <sqrat.h>
#include <squirrel.h>
#include <quirrel/quirrel_json/jsoncpp.h>
#include <quirrel/sqEventBus/sqEventBus.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_basePath.h>
#include <osApiWrappers/dag_direct.h>
#include <util/dag_delayedAction.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>


namespace bindquirrel
{

struct UploadData
{
  // for send part request
  eastl::string sendUrl;
  httprequests::AsyncRequestParams sendReqParams;
  file_ptr_t fileHandle = nullptr;
  eastl::string fileLengthStr;
  eastl::string partsTotalStr;
  eastl::string partSizeStr;
  int currentPart = 0;
  int partsTotal = 0;
  int partSize = 0;
  size_t partHeader = 0;
  size_t partSizeHeader = 0;
  eastl::unique_ptr<char[]> readBuffer;
  eastl::string partSentEvent;
  // for save file request
  eastl::string saveUrl;
  eastl::string fileType;
  eastl::string savedFileEvent;
  // common data
  eastl::string tokenRaw;
  eastl::string tokenStr;
  eastl::string filePath;
  eastl::string fileName;
  eastl::string uuidStr;
};

typedef eastl::shared_ptr<UploadData> UploadDataPtr;


static void sendPart(UploadDataPtr &&data);
static void saveFile(UploadDataPtr &&data);


// ================== SEND PART ==================


class SendPartCallback : public httprequests::IAsyncHTTPCallback
{
  UploadDataPtr data;

  void handleResponse(const dag::ConstSpan<char> &response, Json::Value &resp)
  {
    rapidjson::Document doc;
    doc.Parse(response.data(), response.size());

    if (doc.HasParseError())
    {
      logerr("Error parsing response: %s", response.data());
      resp["msg"] = "Error parsing response";

      if (data->fileHandle)
      {
        df_close(data->fileHandle);
        data->fileHandle = nullptr;
      }

      return;
    }

    // TODO: don't write every time?
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);

    if (doc.HasMember("status"))
      resp["status"] = doc["status"].GetString();

    if (resp["status"] != "OK" || !doc.HasMember("data") || !doc["data"].HasMember("state"))
    {
      resp["msg"] = doc.HasMember("msg") ? doc["msg"].GetString() : "Unknown error";

      if (data->fileHandle)
      {
        df_close(data->fileHandle);
        data->fileHandle = nullptr;
      }

      return;
    }

    auto *state = doc["data"]["state"].GetString();
    resp["state"] = state;

    if (strcmp(state, "CONTINUE") == 0)
    {
      resp["received"] = doc["data"]["received"].GetInt();
      sendPart(eastl::move(data));
    }
    else if (strcmp(state, "DONE") == 0)
    {
      saveFile(eastl::move(data));
    }
    else
    {
      // should never happen
      logerr("Unknown state: %s", state);

      if (data->fileHandle)
      {
        df_close(data->fileHandle);
        data->fileHandle = nullptr;
      }
    }
  }

public:
  SendPartCallback(UploadDataPtr &&data) : data(eastl::move(data)) {}

  void onRequestDone(httprequests::RequestStatus status, int http_code, dag::ConstSpan<char> response,
    httprequests::StringMap const & /*resp_headers*/) override
  {
    Json::Value resp;
    resp["http_status"] = (int)status;
    resp["http_code"] = http_code;
    resp["uuid"] = data->uuidStr;
    resp["totalParts"] = data->partsTotal;
    resp["currentPart"] = data->currentPart;

    // cody data if event will be sent
    bool needToSendEvent = !data->partSentEvent.empty();
    eastl::string partSentEvent = data->partSentEvent;

    if (status == httprequests::RequestStatus::SUCCESS)
      handleResponse(response, resp);

    if (needToSendEvent)
      // Note: no need to delayed call since `sendResponseInMainThreadOnly=true`
      sqeventbus::write_event_main_thread(partSentEvent.c_str(),
        [&](auto &evdata) { evdata.obj = jsoncpp_to_quirrel(evdata.getVM(), resp); });
  }

  void release() override { delete this; }
};


static void sendPart(UploadDataPtr &&data)
{
  data->currentPart++;
  if (data->currentPart > data->partsTotal)
  {
    logerr("All parts sent, but apparently DONE state wasn't received. Something went wrong");
    return;
  }
  int bytesRead = df_read(data->fileHandle, data->readBuffer.get(), data->partSize);
  data->sendReqParams.postData = make_span(data->readBuffer.get(), bytesRead);

  auto &headers = data->sendReqParams.headers;
  eastl::string partStr = eastl::to_string(data->currentPart - 1); // starting from 0
  headers[data->partHeader].second = partStr.c_str();
  eastl::string partSizeStr;
  if (bytesRead < data->partSize)
  {
    partSizeStr = eastl::to_string(bytesRead);
    headers[data->partSizeHeader].second = partSizeStr.c_str();
  }
  else
  {
    headers[data->partSizeHeader].second = data->partSizeStr.c_str();
  }

  auto *dataPtr = data.get();
  dataPtr->sendReqParams.callback = new SendPartCallback(eastl::move(data));
  httprequests::async_request(dataPtr->sendReqParams);
}

static void prepareSendPart(UploadDataPtr &data)
{
  data->fileHandle = df_open(data->filePath.c_str(), DF_READ);
  int fileLength = df_length(data->fileHandle);
  data->partSizeStr = eastl::to_string(data->partSize);
  data->partsTotal = (fileLength + data->partSize - 1) / data->partSize;
  data->readBuffer = eastl::make_unique<char[]>(data->partSize);

  data->sendReqParams.sendResponseInMainThreadOnly = true;
  data->sendReqParams.url = data->sendUrl.c_str();
  data->fileLengthStr = eastl::to_string(fileLength);
  data->partsTotalStr = eastl::to_string(data->partsTotal);

  auto &headers = data->sendReqParams.headers;
  // will not change
  headers.push_back({"Content-Type", "application/octet-stream"});
  headers.push_back({"Mpu-Fname", data->fileName.c_str()});
  headers.push_back({"Mpu-Fsize", data->fileLengthStr.c_str()});
  headers.push_back({"Mpu-Multipart", "true"});
  headers.push_back({"Mpu-Partstotal", data->partsTotalStr.c_str()});
  headers.push_back({"Mpu-Uuid", data->uuidStr.c_str()});
  headers.push_back({"Authorization", data->tokenStr.c_str()});
  // change each part
  data->partSizeHeader = headers.size();
  headers.push_back({"Mpu-Partsize", nullptr});
  data->partHeader = headers.size();
  headers.push_back({"Mpu-Part", nullptr});

  logdbg("httpUpload: file %s, size %d KB, parts %d, part size %d KB", data->fileName.c_str(), fileLength / 1024, data->partsTotal,
    data->partSize / 1024);
}


// ================== SAVE FILE ==================


class SaveFileCallback : public httprequests::IAsyncHTTPCallback
{
  UploadDataPtr data;

  void handleResponse(const dag::ConstSpan<char> &response, Json::Value &resp)
  {
    debug("httpUpload: file %s was saved", data->fileName.c_str());
    rapidjson::Document doc;
    doc.Parse(response.data(), response.size());
    if (doc.HasParseError())
    {
      // logerr here because it should never happen
      logerr("Error parsing response: %s", response.data());
      return;
    }
    rapidjson::StringBuffer buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
    doc.Accept(writer);

    if (doc.HasMember("status"))
      resp["status"] = doc["status"].GetString();
    if (!doc.HasMember("data"))
    {
      // no need to raise logerr here, pass it to quirrel
      resp["msg"] = doc.HasMember("msg") ? doc["msg"].GetString() : "Unknown error";
      return;
    }
    if (doc["data"].HasMember("hash"))
      resp["hash"] = doc["data"]["hash"].GetString();
    if (doc["data"].HasMember("uri"))
      resp["uri"] = doc["data"]["uri"].GetString();
    if (doc["data"].HasMember("new"))
      resp["new"] = doc["data"]["new"].GetBool();
  }

public:
  SaveFileCallback(UploadDataPtr &&data) : data(eastl::move(data)) {}

  void onRequestDone(httprequests::RequestStatus status, int http_code, dag::ConstSpan<char> response,
    httprequests::StringMap const & /*resp_headers*/) override
  {
    Json::Value resp;
    resp["http_status"] = (int)status;
    resp["http_code"] = http_code;
    resp["uuid"] = data->uuidStr;

    if (status == httprequests::RequestStatus::SUCCESS)
      handleResponse(response, resp);

    bool needToSendEvent = !data->savedFileEvent.empty();
    if (needToSendEvent)
      // Note: no need to delayed call since `sendResponseInMainThreadOnly=true`
      sqeventbus::write_event_main_thread(data->savedFileEvent.c_str(),
        [&](auto &evdata) { evdata.obj = jsoncpp_to_quirrel(evdata.getVM(), resp); });
  }

  void release() override { delete this; }
};


static void saveFile(UploadDataPtr &&data)
{
  df_close(data->fileHandle);

  httprequests::AsyncRequestParams saveReqParams;
  saveReqParams.sendResponseInMainThreadOnly = true;
  saveReqParams.url = data->saveUrl.c_str();
  saveReqParams.headers.push_back({"Authorization", data->tokenStr.c_str()});
  saveReqParams.headers.push_back({"Content-Type", "application/x-www-form-urlencoded; charset=UTF-8"});

  eastl::string postData = eastl::string(eastl::string::CtorSprintf{}, "resourceName=%s&fileName=%s&fileType=%s",
    data->uuidStr.c_str(), data->fileName.c_str(), data->fileType.c_str());
  saveReqParams.postData = make_span(postData.data(), postData.size());

  saveReqParams.callback = new SaveFileCallback(eastl::move(data));
  httprequests::async_request(saveReqParams);
}


// ================== COMMON ==================


static SQRESULT extractArgumentsFromSq(HSQUIRRELVM vm, UploadDataPtr &data)
{
  Sqrat::Var<Sqrat::Table> varSqParams(vm, 2);
  Sqrat::Table &sqParams = varSqParams.value;
  if (sqParams.GetType() != OT_TABLE)
    return sq_throwerror(vm, "First (and only) argument must be a table");

  Sqrat::Object tokenObj = sqParams.RawGetSlot("token");
  if (tokenObj.GetType() != OT_STRING)
    return sq_throwerror(vm, "`token` is required and must be a string");
  Sqrat::Object filePathObj = sqParams.RawGetSlot("filepath");
  if (filePathObj.GetType() != OT_STRING)
    return sq_throwerror(vm, "`filepath` is required and must be a string");
  Sqrat::Object fileTypeObj = sqParams.RawGetSlot("filetype");
  if (fileTypeObj.GetType() != OT_STRING)
    return sq_throwerror(vm, "`filetype` is required and must be a string");
  Sqrat::Object partSizeObj = sqParams.RawGetSlot("partSize");
  if (partSizeObj.GetType() != OT_INTEGER && partSizeObj.GetType() != OT_NULL)
    return sq_throwerror(vm, "`partSize` must be an integer");
  Sqrat::Object sentPartEventObj = sqParams.RawGetSlot("sentPartEvent");
  if (sentPartEventObj.GetType() != OT_STRING && sentPartEventObj.GetType() != OT_NULL)
    return sq_throwerror(vm, "`sentPartEvent` must be a string");
  Sqrat::Object savedFileEventObj = sqParams.RawGetSlot("savedFileEvent");
  if (savedFileEventObj.GetType() != OT_STRING && savedFileEventObj.GetType() != OT_NULL)
    return sq_throwerror(vm, "`savedFileEvent` must be a string");
  Sqrat::Object sendUrlObj = sqParams.RawGetSlot("sendUrl");
  if (sendUrlObj.GetType() != OT_STRING)
    return sq_throwerror(vm, "`sendUrl` is required must be a string");
  Sqrat::Object saveUrlObj = sqParams.RawGetSlot("saveUrl");
  if (saveUrlObj.GetType() != OT_STRING)
    return sq_throwerror(vm, "`saveUrl` must be a string");

  data->sendUrl = sq_objtostring(&sendUrlObj.GetObject());
  data->saveUrl = sq_objtostring(&saveUrlObj.GetObject());

  data->tokenRaw = sq_objtostring(&tokenObj.GetObject());
  data->filePath = sq_objtostring(&filePathObj.GetObject());
  data->fileType = sq_objtostring(&fileTypeObj.GetObject());
  constexpr int DEFAULT_PART_SIZE = 1 << 20; // 1 MB
  if (partSizeObj.GetType() == OT_INTEGER)
    data->partSize = sq_objtointeger(&partSizeObj.GetObject());
  else
    data->partSize = DEFAULT_PART_SIZE;
  if (sentPartEventObj.GetType() == OT_STRING)
    data->partSentEvent = sq_objtostring(&sentPartEventObj.GetObject());
  if (savedFileEventObj.GetType() == OT_STRING)
    data->savedFileEvent = sq_objtostring(&savedFileEventObj.GetObject());

  return SQ_OK;
}

/* qdox @module dagor.uploadFile
@function uploadFile

@kwarged

@param token s : jwt token to use for authentication
@param filepath s : path to file to upload
@param filetype t : meta info about file, will be passed in save file request
@param partSize i : (optional), size of each part in bytes, 1 MB by default
@param sendUrl s : url to send parts to
@param saveUrl s : url to call to save file after all parts are sent
@param sentPartEvent s : (optional) : event id to send into eventbus after each part is sent
@param savedFileEvent s : (optional) : event id to send into eventbus after file is saved

response for sentPartEvent in eventbus data or in callback will be table
@code response
  {
    "http_status" : integer (SUCCESS, FAILED, ABORTED)
    "http_code" : integer
    "uuid" : string
    // if http_status is SUCCESS
    "status" : string, optional
    "msg" : string, optional
    "state" : string ("CONTINUE" or "DONE")
    "received" : integer, optional, how many parts was received. Present if state is CONTINUE
  }
code@
response for savedFileEvent in eventbus data or in callback will be table
@code response
  {
    "http_status" : integer (SUCCESS, FAILED, ABORTED)
    "http_code" : integer
    "uuid" : string
    // if http_status is SUCCESS
    "status" : string, optional
    "msg" : string, optional
    "hash" : string, optional, hash of the saved file
    "uri" : string, optional, uri of the saved file
    "new" : bool, optional, true if file was newly saved, false if file with such hash already exists
  }
*/
///@resetscope
static SQInteger upload(HSQUIRRELVM vm)
{
  UploadDataPtr data = eastl::make_shared<UploadData>();
  SQRESULT errorOnExtraction = extractArgumentsFromSq(vm, data);
  if (errorOnExtraction != SQ_OK)
    return errorOnExtraction;

  if (!dd_file_exists(data->filePath.c_str()))
    return sq_throwerror(vm, "File does not exist");

  data->fileName = dd_get_fname(data->filePath.c_str());
  // TODO: find proper uuid generation
  data->uuidStr = eastl::to_string(ref_time_ticks());
  data->tokenStr = eastl::string("Bearer ") + data->tokenRaw;

  prepareSendPart(data);
  sendPart(eastl::move(data));

  return 0;
}


void bind_file_upload(SqModules *module_mgr)
{
  Sqrat::Table nsTbl(module_mgr->getVM());
  ///@module dagor.http
  nsTbl //
    .SquirrelFuncDeclString(upload, "uploadFile(params: table): null")
    /**/;
  module_mgr->addNativeModule("dagor.uploadFile", nsTbl);
}

} // namespace bindquirrel
