#include "DocBuilderAddIn.h"
#include <fstream>


std::string DocBuilderAddIn::extensionName() { return "DocBuilder"; }

DocBuilderAddIn::DocBuilderAddIn() {
  Cbuild = std::make_shared<NSDoctRenderer::CDocBuilder>();
  setlocale(LC_ALL, "Russian.UTF8");
  workDir = std::make_shared<variant_t>();
  AddProperty(
      L"WorkDir", L"РабочаяДиректория", [&]() { return workDir; },
      [&](variant_t &&v) {
        bool isSetNotEmpty = std::holds_alternative<std::string>(v) &&
                             !std::get<std::string>(v).empty();
        bool pExists = pathExists(v);
        WorkDirIsSet = (isSetNotEmpty && pExists);
        if (!WorkDirIsSet) {
          std::string msg =
              "Рабочая директория не существует или некорректна: " +
              std::get<std::string>(v) +
              "\nПуть задан и не пустой: " + std::to_string(isSetNotEmpty) +
              "\nПуть существует: " + std::to_string(pExists);
          AddError(ADDIN_E_FAIL, extensionName(), msg, true);
        } else {
          *workDir = std::move(v);
          wchar_t* BuildPath = stringToWchar(std::get<std::string>(*workDir));
          NSDoctRenderer::CDocBuilder::Initialize(BuildPath);
          Cbuild->SetProperty("--work-directory", BuildPath);
          delete[] BuildPath;
        };
      });

  pathToFile = std::make_shared<variant_t>();
  AddProperty(
      L"PathToFile", L"ПутьКФайлу", [&]() { return pathToFile; },
      [&](variant_t &&v) {
        FileIsSet = (std::holds_alternative<std::string>(v) &&
                     !std::get<std::string>(v).empty() && getExtension(v) != 0);
        if (!FileIsSet) {
          AddError(ADDIN_E_FAIL, extensionName(),
                   u8"Файл не задан. (Полный путь к файлу включая расширение)",
                   true);
        } else {
          *pathToFile = std::move(v);
          wchar_t *wPTT = stringToWchar(std::get<std::string>(*pathToFile), true);
          if (fileExists(*pathToFile)) {
            int x2t = Cbuild->OpenFile(wPTT, L"");
          } else {
              AddError(ADDIN_E_FAIL, extensionName(),
                  u8"Файл не существует.",
                  true);
          }
          delete[] wPTT;
        }
        return "OK";
      });

  pathToSave = std::make_shared<variant_t>();
  AddProperty(
      L"PathToSave", L"ПутьКФайлуДляСохранения", [&]() { return pathToSave; },
      [&](variant_t &&v) {
        bool SavePathIsCorrect =
            (std::holds_alternative<std::string>(v) &&
             !std::get<std::string>(v).empty() && getExtension(v) != 0);
        if (!SavePathIsCorrect) {
          AddError(ADDIN_E_FAIL, extensionName(),
                   u8"Файл не задан. (Полный путь к файлу включая расширение)",
                   true);
        }
        *pathToSave = std::move(v);
        PathToSaveIsSet = true;
      });

  // Full featured property registration example
  AddProperty(L"Version", L"ВерсияКомпоненты", [&]() {
    auto s = std::string(Version);
    return std::make_shared<variant_t>(std::move(s));
  });

  // Method registration.
  // Lambdas as method handlers are not supported.
  AddMethod(L"Message", L"Сообщить", this, &DocBuilderAddIn::message);
  AddMethod(L"SearchAndReplace", L"НайтиИЗаменить", this,
            &DocBuilderAddIn::searchAndReplace);
  AddMethod(L"SearchAndReplaceOneCMD", L"НайтиИЗаменитьОднойКомандой", this,
            &DocBuilderAddIn::searchAndReplaceOneCMD);
  AddMethod(L"FillRow", L"ЗаполнитьСтроку", this, &DocBuilderAddIn::fillRow);
  AddMethod(L"SaveAndCloseFile", L"СохранитьИЗакрытьФайл", this,
            &DocBuilderAddIn::saveAndCloseFile);
  AddMethod(L"CloseFile", L"ЗакрытьФайл", this, &DocBuilderAddIn::closeFile);
  AddMethod(L"GetDataFromRange", L"ПолучитьДанныеИзДиапазона", this,
            &DocBuilderAddIn:: getDataFromRange);
  AddMethod(L"InitGetDataFromRangeByCell",
            L"ИнициализироватьПолучениеДанныхИзДиапазонаПоКлетке", this,
            &DocBuilderAddIn::initGetDataFromRangeByCell);
  AddMethod(L"IsGettingDataFromRange", L"ИдетПолучениеДанныхИзДиапазона", this,
            &DocBuilderAddIn::isGettingDataFromRange);
  AddMethod(L"GetNextCell", L"ПолучитьСледующуюЯчейку", this,
            &DocBuilderAddIn::getNextCell);
  // AddMethod(L"SetBorders", L"УстановитьГраницы", this,
  // &DocBuilderAddIn::setBorders);

  // Method registration with default arguments
  //
  // Notice that if variant_t would be non-copy you can't use initializer
  // list. Proper way to register def args would be then:
  //        std::map<long, variant_t> def_args;
  //        def_args.insert({0, 5});
  //        AddMethod(u"Sleep", u"Ожидать", this, &DocBuilderAddIn::sleep,
  //        std::move(def_args));
}

void DocBuilderAddIn::message(const variant_t &msg) {
  setlocale(LC_ALL, "Russian.UTF8");
  std::visit(
      overloaded{
          [&](const std::string &v) {
            AddError(ADDIN_E_INFO, extensionName(), v, false);
          },
          [&](const int32_t &v) {
            AddError(ADDIN_E_INFO, extensionName(),
                     std::to_string(static_cast<int>(v)), false);
          },
          [&](const double &v) {
            AddError(ADDIN_E_INFO, extensionName(), std::to_string(v), false);
          },
          [&](const bool &v) {
            AddError(ADDIN_E_INFO, extensionName(),
                     std::string(v ? u8"Истина" : u8"Ложь"), false);
          },
          [&](const std::tm &v) {
            std::ostringstream oss;
            oss.imbue(std::locale("ru_RU.utf8"));
            oss << std::put_time(&v, "%c");
            AddError(ADDIN_E_INFO, extensionName(), oss.str(), false);
          },
          [&](const std::vector<char> &v) {}, [&](const std::monostate &) {}},
      msg);
}

void DocBuilderAddIn::searchAndReplace(const variant_t &keysAndValues) {
  if (!WorkDirIsSet || !FileIsSet) {
    return;
  }
  setlocale(LC_ALL, "Russian.UTF8");
  if (!std::holds_alternative<std::string>(keysAndValues)) {
    AddError(
        ADDIN_E_FAIL, extensionName(),
        u8"Не поддерживаемые типы данных. "
        u8"НайтиИЗаменить(СтрокаСКлючамиИЗначениями(напр.: 'ключ=значение;')",
        false);
    return;
  }
  unsigned int extension = getExtension(*pathToFile);
  if (extension == 0 ||
      extension < static_cast<unsigned int>(ExtensionUINT::DOCX) ||
      extension > static_cast<unsigned int>(ExtensionUINT::RTF)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемое расширение исходного документа.", false);
    return;
  }

  std::string kAV = std::get<std::string>(keysAndValues);

  NSDoctRenderer::CContext oContext = Cbuild->GetContext();
  NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
  NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
  NSDoctRenderer::CValue oApi = oGlobal["Api"];

  NSDoctRenderer::CValue oDocument = oApi.Call("GetDocument");

  std::vector<std::string> keysAndValuesList = splitString(kAV, ";");
  for (std::string kv : keysAndValuesList) {
    if (kv.empty()) break;
    std::vector<std::string> keyAndValue = splitString(kv, "=");
    if (keyAndValue.size() == 2) {
      NSDoctRenderer::CValue oObject = oContext.CreateObject();
      oObject.SetProperty(L"searchString", keyAndValue[0].c_str());
      oObject.SetProperty(L"replaceString", keyAndValue[1].c_str());
      oDocument.Call("SearchAndReplace", oObject);
    }
  }
}

variant_t DocBuilderAddIn::searchAndReplaceOneCMD(
    const variant_t &pathToTemplate, const variant_t &keysAndValues,
    const variant_t &altPathToSave) {
  if (!WorkDirIsSet) {
      AddError(ADDIN_E_FAIL, extensionName(),
          u8"Рабочая директория не задана!",
          false);
    return "Ошибка!";
  }
  setlocale(LC_ALL, "Russian.UTF8");
  if (!fileExists(pathToTemplate)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Файл не существует. НайтиИЗаменить(СторокаСПутемКФайлу, "
             u8"СтрокаСКлючамиИЗначениями(ключ=значение;';' - делимитер), "
             u8"ПутьКФайлуДляСохранения)",
             false);
    return "Ошибка!";
  }
  if (!std::holds_alternative<std::string>(keysAndValues) ||
      !std::holds_alternative<std::string>(pathToTemplate)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемые типы данных. "
             u8"НайтиИЗаменить(СторокаСПутемКФайлу, СтрокаСКлючамиИЗначениями,"
             u8" СторокаСПутемКФайлуДляСохранения)",
             false);
    return "Ошибка!";
  }
  unsigned int extension = getExtension(pathToTemplate);
  if (extension == 0 ||
      extension < static_cast<unsigned int>(ExtensionUINT::DOCX) ||
      extension > static_cast<unsigned int>(ExtensionUINT::RTF)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемое расширение исходного документа.", false);
    return "Ошибка!";
  }

  std::string kAV = std::get<std::string>(keysAndValues);
  std::string pTT = std::get<std::string>(pathToTemplate);
  wchar_t *wPTT = stringToWchar(pTT);
  bool useAltPath = (std::holds_alternative<std::string>(altPathToSave) &&
                     !std::get<std::string>(altPathToSave).empty());
  int saveExtension = 0;
  std::string APS;
  wchar_t *wAPS;
  if (useAltPath) {
    APS = std::get<std::string>(altPathToSave);
    wAPS = stringToWchar(APS);
    saveExtension = getExtension(altPathToSave);
  }
  if (useAltPath && saveExtension == 0) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемое расширение для пути сохранения.", false);
    return "Ошибка!";
  }
  std::string tmp = std::get<std::string>(altPathToSave);

  int x2t = Cbuild->OpenFile(wPTT, L"");

  NSDoctRenderer::CContext oContext = Cbuild->GetContext();
  NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
  NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
  NSDoctRenderer::CValue oApi = oGlobal["Api"];

  NSDoctRenderer::CValue oDocument = oApi.Call("GetDocument");

  std::vector<std::string> keysAndValuesList = splitString(kAV, ";");
  for (std::string kv : keysAndValuesList) {
    if (kv.empty()) break;
   std::vector<std::string> keyAndValue = splitString(kv, "=");
   if (keyAndValue.size() == 2) {
      NSDoctRenderer::CValue oObject = oContext.CreateObject();
      oObject.SetProperty(L"searchString", keyAndValue[0].c_str());
      oObject.SetProperty(L"replaceString", keyAndValue[1].c_str());
      oDocument.Call("SearchAndReplace", oObject);
    }
  }
  int check2 = Cbuild->SaveFile((useAltPath ? saveExtension : extension),
                  (useAltPath ? wAPS : wPTT));
  Cbuild->CloseFile();

  delete[] wPTT;
  if (useAltPath) {
    delete[] wAPS;
  }
  char tmpPath[1024] = u8"Документ сформирован. Путь: ";
  strncat(tmpPath, useAltPath ? APS.c_str() : pTT.c_str(),
          useAltPath ? strlen(APS.c_str()) : strlen(pTT.c_str()));
  AddError(ADDIN_E_INFO, extensionName(), tmpPath, false);
  std::string res;
  useAltPath == true ? res = APS : res = pTT;
  return res;
}

void DocBuilderAddIn::fillRow(const variant_t &range,
                              const variant_t &rowData) {
  if (!WorkDirIsSet || !FileIsSet) {
    return;
  }
  setlocale(LC_ALL, "Russian.UTF8");
  unsigned int extension = getExtension(*pathToFile);
  if (extension == 0 ||
      extension < static_cast<unsigned int>(ExtensionUINT::XLSX) ||
      extension > static_cast<unsigned int>(ExtensionUINT::OTS)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемое расширение исходного документа.", false);
    return;
  }
  if (!std::holds_alternative<std::string>(range) ||
      !checkRangeInOneRow(std::get<std::string>(range))) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Диапазон ячеек должен быть в одной строке.", false);
    return;
  }
  NSDoctRenderer::CContext oContext = Cbuild->GetContext();
  NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
  NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
  NSDoctRenderer::CValue oApi = oGlobal["Api"];

  NSDoctRenderer::CValue oWorksheet = oApi.Call("GetActiveSheet");
  std::vector<std::string> colDataArr =
      splitString(std::get<std::string>(rowData), ";");
  NSDoctRenderer::CValue rArray = oContext.CreateArray(1);
  int arr_size = (int)colDataArr.size();
  NSDoctRenderer::CValue cArray = oContext.CreateArray(arr_size);
  for (int i = 0; i < colDataArr.size(); i++) {
    cArray[i] = colDataArr[i].c_str();
  }
  rArray[0] = cArray;
  std::string sRange = std::get<std::string>(range);
  oWorksheet.Call("GetRange", NSDoctRenderer::CDocBuilderValue(sRange.c_str()))
      .Call("SetValue", rArray);
}

bool DocBuilderAddIn::checkRangeInOneRow(const std::string range) {
  setlocale(LC_ALL, "Russian.UTF8");
  bool res = false;
  std::string rangeCpy = range;
  int rows[3] = {0, 0};
  std::regex pattern("\\d+");

  std::smatch matches;
  std::regex_search(rangeCpy, matches, pattern);
  int count = 0;
  while (std::regex_search(rangeCpy, matches, pattern)) {
    std::string match = matches.str();
    rows[count] = std::stoi(match);
    rangeCpy = matches.suffix();
    count++;
    if (count == 2) {
      break;
    }
  }
  if (count == 2) {
    res = (rows[0] == rows[1]);
  }
  return res;
}

std::vector<std::string> DocBuilderAddIn::splitString(std::string source,
                                                      std::string delimiter) {
  setlocale(LC_ALL, "Russian.UTF8");
  std::vector<std::string> result;
  std::string tmpsource = source;
  size_t pos = 0;
  std::string token;
  while ((pos = tmpsource.find(delimiter)) != std::string::npos) {
    token = tmpsource.substr(0, pos);
    result.push_back(token);
    tmpsource.erase(0, pos + delimiter.length());
  }
  result.push_back(tmpsource);
  return result;
}
wchar_t *DocBuilderAddIn::stringToWchar(const std::string &str) {
  setlocale(LC_ALL, "Russian.UTF8");
  size_t len = str.length();
  wchar_t *res = new wchar_t[len + 1];
  const char *cstr = str.c_str();
  std::mbstowcs(res, cstr, len);
  res[len] = L'\0'; 
  return res;
}

wchar_t *DocBuilderAddIn::stringToWchar(const std::string &str, bool alt) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  std::wstring wstr = converter.from_bytes(str);
  wchar_t *res = new wchar_t[wstr.size() + 1];
  std::copy(wstr.begin(), wstr.end(), res);
  res[wstr.size()] = L'\0';
  return res;
}

std::string DocBuilderAddIn::wcharToString(const wchar_t *wstr) {
  setlocale(LC_ALL, "Russian.UTF8");
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return (wstr != NULL ? converter.to_bytes(wstr) : "");
}

unsigned int DocBuilderAddIn::getExtension(const variant_t &path) {
  setlocale(LC_ALL, "Russian.UTF8");
  if (!std::holds_alternative<std::string>(path)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемые типы данных. Тип пути должен быть строкой и "
             u8"включать имя файла с расширением.",
             false);
    return 0;
  }
  std::string pathToFile = std::get<std::string>(path);
  size_t dotindx = pathToFile.find_last_of(".");
  if (dotindx == std::string::npos) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемые типы данных. Не найдено расширения.", false);
    return 0;
  }
  std::string ext = pathToFile.substr(dotindx + 1);
  unsigned int extUInt = 0;
  if (ext == "docx") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::DOCX);
  } else if (ext == "doc") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::DOC);
  } else if (ext == "odt") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::ODT);
  } else if (ext == "rtf") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::RTF);
  } else if (ext == "pdf") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::PDF);
  } else if (ext == "xlsx") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::XLSX);
  } else if (ext == "xls") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::XLS);
  } else if (ext == "ods") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::ODS);
  } else if (ext == "csv") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::CSV);
    AddError(ADDIN_E_FAIL, extensionName(), "Формат CSV пока не поддерживается",
             false);
    return 0;
  } else if (ext == "xltx") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::XLTX);
  } else if (ext == "ots") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::OTS);
  } else {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемые типы данных. Неизвестное расширение.", false);
    return 0;
  }
  return extUInt;
}

bool DocBuilderAddIn::fileExists(const variant_t &path) {
  setlocale(LC_ALL, "Russian.UTF8");
  bool res = false;
  if (!std::holds_alternative<std::string>(path)) {
    return false;
  }
  std::string pathToFile = std::get<std::string>(path);
  if (FILE *file = fopen(pathToFile.c_str(), "r")) {
    fclose(file);
    res = true;
  }
  return res;
}

bool DocBuilderAddIn::pathExists(const variant_t &path) {
  setlocale(LC_ALL, "Russian.UTF8");
  bool res = false;
  if (!std::holds_alternative<std::string>(path)) {
    return false;
  }
  std::string pathToFile = std::get<std::string>(path);
  struct stat bf;
#ifdef __linux__
  res = stat(pathToFile.c_str(), &bf) == 0 && S_ISDIR(bf.st_mode);
#elif _WIN32
  res = stat(pathToFile.c_str(), &bf) == 0 && !fileExists(path);
#endif
  return res;
}

variant_t DocBuilderAddIn::saveAndCloseFile() {
  setlocale(LC_ALL, "Russian.UTF8");
  if (WorkDirIsSet && FileIsSet && PathToSaveIsSet) {
    std::string path = std::get<std::string>(*pathToSave);
    wchar_t *wpath = stringToWchar(path);
    bool res = Cbuild->SaveFile(getExtension(path), wpath);
    Cbuild->CloseFile();
    delete[] wpath;
    return path;
  }
  return "Что-то пошло не так!";
}

variant_t DocBuilderAddIn::closeFile() {
  setlocale(LC_ALL, "Russian.UTF8");
  if (WorkDirIsSet && FileIsSet) {
    Cbuild->CloseFile();
    return std::get<std::string>(*pathToFile);
  }
  return "";
}

variant_t DocBuilderAddIn::getDataFromRange(const variant_t &range) {
  setlocale(LC_ALL, "Russian.UTF8");
  if (!WorkDirIsSet || !FileIsSet) {
    return "";
  }
  std::string lRange = std::get<std::string>(range);
  std::string res = "";
  NSDoctRenderer::CContext oContext = Cbuild->GetContext();
  NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
  NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
  NSDoctRenderer::CValue oApi = oGlobal["Api"];
  NSDoctRenderer::CValue oWorksheet = oApi.Call("GetActiveSheet");
  std::string cell = "start";
  while (cell != "") {
    cell = (cell == "start" ? "" : cell);
    cell = getNextCellInRange(lRange, cell);
    if (cell == "") {
      break;
    }
    NSDoctRenderer::CValue oCell = oWorksheet.Call(
        "GetRange", NSDoctRenderer::CDocBuilderValue(cell.c_str()));
    NSDoctRenderer::CValue oValue = oCell.Call("GetValue");
    NSDoctRenderer::CString val = oValue.ToString();
    wchar_t *w_val = val.c_str();
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    res += (w_val != NULL) ? converter.to_bytes(w_val) : " ";
    res += ";";
  }

  return res;
}

void DocBuilderAddIn::initGetDataFromRangeByCell(const variant_t &range) {
  setlocale(LC_ALL, "Russian.UTF8");
  if (!WorkDirIsSet || !FileIsSet) {
    return;
  }
  initGetDataFromRangeByCellIsSet = true;
  spreadsheetRange = std::get<std::string>(range);
}

variant_t DocBuilderAddIn::isGettingDataFromRange() {
  return initGetDataFromRangeByCellIsSet;
}

variant_t DocBuilderAddIn::getNextCell() {
  if (!WorkDirIsSet || !FileIsSet || !initGetDataFromRangeByCellIsSet) {
    return "";
  }
  
  std::string res = "";
  spreadsheetCell = getNextCellInRange(spreadsheetRange, spreadsheetCell);
  if (spreadsheetCell != "") {
    NSDoctRenderer::CContext oContext = Cbuild->GetContext();
    NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
    NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
    NSDoctRenderer::CValue oApi = oGlobal["Api"];
    NSDoctRenderer::CValue oWorksheet = oApi.Call("GetActiveSheet");
    NSDoctRenderer::CValue oCell = oWorksheet.Call(
        "GetRange", NSDoctRenderer::CDocBuilderValue(spreadsheetCell.c_str()));
    NSDoctRenderer::CValue oValue = oCell.Call("GetValue");
    NSDoctRenderer::CString val = oValue.ToString();
    wchar_t *w_val = val.c_str();
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    res += (w_val != NULL) ? converter.to_bytes(w_val) : " ";
  } else {
    initGetDataFromRangeByCellIsSet = false;
    return "";
  }
  return res;
}

std::string DocBuilderAddIn::getNextCellInRange(const std::string range,
                                                const std::string prevCell) {
  setlocale(LC_ALL, "Russian.UTF8");
  std::string rangeCpy = range;
  std::vector<std::string> rangeArr = splitString(range, ":");
  std::string from = rangeArr[0];
  if (prevCell == "") {
    return from;
  }
  std::string to = rangeArr[1];
  int fromRow, toRow, fromCol, toCol;
  getRowIndx(from, &fromRow);
  getRowIndx(to, &toRow);
  getColIndx(from, &fromCol);
  getColIndx(to, &toCol);
  int prevRow, prevCol;
  getRowIndx(prevCell, &prevRow);
  getColIndx(prevCell, &prevCol);
  int nextRow = prevRow;
  int nextCol = prevCol;
  if (nextCol < toCol) {
    nextCol++;
  } else if (nextCol == toCol) {
    nextRow++;
    nextCol = fromCol;
  }
  if (nextRow > toRow || nextCol > toCol) {
    return "";
  }
  std::string res = "";
  getColLetters(nextCol, &res);
  res += std::to_string(nextRow);

  return res;
}

void DocBuilderAddIn::getColIndx(const std::string cell, int *indx) {
  setlocale(LC_ALL, "Russian.UTF8");
  *indx = 0;
  char *cellCpy = (char *)calloc(cell.size() + 1, sizeof(char));
  strncpy(cellCpy, cell.c_str(), cell.size());
  cellCpy[cell.size()] = '\0';
  int len = (int)cell.size();
  unsigned colCorrection = 1;
  for (int i = len - 1; i >= 0; i--) {
    char c = cellCpy[i];
    if (c >= 'A' && c <= 'Z') {
      *indx += c - 'A' + colCorrection;
      colCorrection *= 26;
    }
  }
  *indx -= 1;
  free(cellCpy);
}

void DocBuilderAddIn::getColLetters(int indx, std::string *res_string) {
  setlocale(LC_ALL, "Russian.UTF8");
  if (indx > 25) {
    getColLetters(indx / 26 - 1, res_string);
  }
  *res_string += indx % 26 + 'A';
}

void DocBuilderAddIn::getRowIndx(const std::string cell, int *indx) {
  setlocale(LC_ALL, "Russian.UTF8");
  *indx = 0;
  int len = (int)cell.size();
  char *num = new char[len + 1];
  for (int i = 0; i < len; i++) {
    num[i] = '0';
  }
  num[len] = '\0';
  for (int i = len - 1; i >= 0; i--) {
    char c = cell[i];
    if (c >= '0' && c <= '9') {
      num[i] = c;
    }
  }
  *indx = atoi(num);
  delete[] num;
}

bool DocBuilderAddIn::checkRange(const std::string range) {
  setlocale(LC_ALL, "Russian.UTF8");
  bool res = false;
  if (range.size() > 0) {
    std::regex rgx("[A-Z]+[0-9]+:[A-Z]+[0-9]+");
    res = std::regex_match(range, rgx) && splitString(range, ":").size() == 2;
  }
  return res;
}

// void DocBuilderAddIn::setBorders(const variant_t &range,
//                                  const variant_t &borders,
//                                  const variant_t &type,
//                                  const variant_t &color) {
//   if (!WorkDirIsSet || !FileIsSet) {
//     return;
//   }
//   std::string lRange = std::get<std::string>(range);
//   std::string lBorders = std::get<std::string>(borders);
//   std::vector<std::string> bordersArr = splitString(lBorders, ";");
//   std::string lType = std::get<std::string>(type);
//   std::string lColor = std::get<std::string>(color);

//   NSDoctRenderer::CContext oContext = Cbuild.GetContext();
//   NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
//   NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
//   NSDoctRenderer::CValue oApi = oGlobal["Api"];
//   NSDoctRenderer::CValue RGBParams = oContext.CreateObject();
//   std::vector<std::string> colorArr = splitString(lBorders, ";");
//   for (int i = 0; i < colorArr.size() && i < 3; i++) {
//     RGBParams.SetProperty(i == 0 ? L"r" : i == 1 ? L"g" : L"b",
//     colorArr[i].c_str());
//   }
//   NSDoctRenderer::CValue ApiColor = oApi.Call("CreateColorFromRGB",
//   RGBParams); NSDoctRenderer::CValue oWorksheet =
//   oApi.Call("GetActiveSheet"); std::vector<std::string> colBorderArr =
//   splitString(lBorders, ";"); for (int i = 0; i < colBorderArr.size(); i++) {
//     oWorksheet.Call("GetRange",
//     NSDoctRenderer::CDocBuilderValue(lRange.c_str()))
//       .Call("SetBorders", colBorderArr[i].c_str(), lType.c_str(), ApiColor);
//   }
// }

// std::string DocBuilderAddIn::getCellInRange(const std::string range, const
// std::string prevCell) {
//   std::vector<std::string> rangeArr = splitString(range, ":");

// }