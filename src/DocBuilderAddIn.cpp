#include <sys/stat.h>

#include <chrono>
#include <cwchar>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <codecvt>

#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#endif
#ifdef __linux__
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#include "DocBuilderAddIn.h"

std::string DocBuilderAddIn::extensionName() { return "DocBuilder"; }

DocBuilderAddIn::DocBuilderAddIn() {
  try {
    // Universal property. Could store any supported by native api type.
    sample_property = std::make_shared<variant_t>();
    AddProperty(L"SampleProperty", L"ОбразецСвойства", sample_property);

    workDir = std::make_shared<variant_t>();
    AddProperty(
        L"WorkDir", L"РабочаяДиректория", [&]() { return workDir; },
        [&](variant_t &&v) {
          *workDir = std::move(v);
          WorkDirIsSet = (std::holds_alternative<std::string>(*workDir) &&
                          !std::get<std::string>(*workDir).empty() &&
                          pathExists(std::get<std::string>(*workDir)));
          if (!WorkDirIsSet) {
            AddError(ADDIN_E_FAIL, extensionName(),
                     u8"Рабочая директория не существует.", false);
          } else {
            wchar_t *BuildPath = stringToWchar(std::get<std::string>(*workDir));
            NSDoctRenderer::CDocBuilder::Initialize(BuildPath);
            delete[] BuildPath;
          };
        });

    pathToFile = std::make_shared<variant_t>();
    AddProperty(
        L"PathToFile", L"ПутьКФайлу", [&]() { return pathToFile; },
        [&](variant_t &&v) {
          *pathToFile = std::move(v);
          if (WorkDirIsSet) {
            FileIsSet = (std::holds_alternative<std::string>(*pathToFile) &&
                         !std::get<std::string>(*pathToFile).empty() &&
                         getExtension(std::get<std::string>(*pathToFile)) != 0);
            if (!FileIsSet) {
              AddError(ADDIN_E_FAIL, extensionName(), u8"Файл не задан.",
                       false);
            } else {
              wchar_t *BuildPath =
                  stringToWchar(std::get<std::string>(*workDir));
              Cbuild.SetProperty("--work-directory", BuildPath);
              wchar_t *wPTT = stringToWchar(std::get<std::string>(*pathToFile));
              if (fileExists(std::get<std::string>(*pathToFile))) {
                Cbuild.OpenFile(wPTT, L"");
              } else {
                Cbuild.CreateFile(
                    getExtension(std::get<std::string>(*pathToFile)));
                NewFileIsCreated = true;
              }
              *pathToSave = std::get<std::string>(*pathToFile);
              delete[] wPTT;
              delete[] BuildPath;
            }
          };
        });

    pathToSave = std::make_shared<variant_t>();
    AddProperty(
        L"PathToSave", L"ПутьКФайлуДляСохранения", [&]() { return pathToSave; },
        [&](variant_t &&v) {
          *pathToSave = std::move(v);
          AltPathToSaveIsSet = true;
          AltSavePathIsCorrect =
              (std::holds_alternative<std::string>(*pathToSave) &&
               !std::get<std::string>(*pathToSave).empty() &&
               getExtension(std::get<std::string>(*pathToSave)) != 0);
          if (!AltSavePathIsCorrect) {
            AddError(ADDIN_E_FAIL, extensionName(),
                     u8"Файл для сохранения задан некорректно.", false);
          }
        });

    // Full featured property registration example
    AddProperty(L"Version", L"ВерсияКомпоненты", [&]() {
      auto s = std::string(Version);
      return std::make_shared<variant_t>(std::move(s));
    });

    // Method registration.
    // Lambdas as method handlers are not supported.
    AddMethod(L"Add", L"Сложить", this, &DocBuilderAddIn::add);
    AddMethod(L"Message", L"Сообщить", this, &DocBuilderAddIn::message);
    AddMethod(L"SearchAndReplace", L"НайтиИЗаменить", this,
              &DocBuilderAddIn::searchAndReplace);
    AddMethod(L"SearchAndReplaceOneCMD", L"НайтиИЗаменитьОднойКМНД", this,
              &DocBuilderAddIn::searchAndReplace);
    AddMethod(L"FillRow", L"ЗаполнитьСтроку", this, &DocBuilderAddIn::fillRow);
    AddMethod(L"CurrentDate", L"ТекущаяДата", this,
              &DocBuilderAddIn::currentDate);
    AddMethod(L"Assign", L"Присвоить", this, &DocBuilderAddIn::assign);
    AddMethod(L"SamplePropertyValue", L"ЗначениеСвойстваОбразца", this,
              &DocBuilderAddIn::samplePropertyValue);
    AddMethod(L"SaveAndCloseFile", L"СохранитьИЗакрытьФайл", this,
              &DocBuilderAddIn::saveAndCloseFile);
    AddMethod(L"GetDataFromRange", L"ПолучитьДанныеИзДиапазона", this,
              &DocBuilderAddIn::getDataFromRange);
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
    //
    AddMethod(L"Sleep", L"Ожидать", this, &DocBuilderAddIn::sleep, {{0, 5}});

  } catch (std::exception &e) {
    AddError(ADDIN_E_FAIL, extensionName(), e.what(), false);
  }
}

// Sample of addition method. Support both integer and string params.
// Every exceptions derived from std::exceptions are handled by components API
variant_t DocBuilderAddIn::add(const variant_t &a, const variant_t &b) {
  if (std::holds_alternative<int32_t>(a) &&
      std::holds_alternative<int32_t>(b)) {
    return std::get<int32_t>(a) + std::get<int32_t>(b);
  } else if (std::holds_alternative<std::string>(a) &&
             std::holds_alternative<std::string>(b)) {
    return std::string{std::get<std::string>(a) + std::get<std::string>(b)};
  } else {
    throw std::runtime_error(u8"Неподдерживаемые типы данных");
  }
}

void DocBuilderAddIn::message(const variant_t &msg) {
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
  if (!std::holds_alternative<std::string>(keysAndValues)) {
    AddError(
        ADDIN_E_FAIL, extensionName(),
        u8"Не поддерживаемые типы данных. "
        u8"НайтиИЗаменить(СтрокаСКлючамиИЗначениями(напр.: 'ключ=значение;')",
        false);
    return;
  }
  unsigned int extension = getExtension(std::get<std::string>(*pathToFile));
  if (extension == 0 ||
      extension < static_cast<unsigned int>(ExtensionUINT::DOCX) ||
      extension > static_cast<unsigned int>(ExtensionUINT::RTF)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемое расширение исходного документа.", false);
    return;
  }

  std::string kAV = std::get<std::string>(keysAndValues);
  bool useAltPath = (AltPathToSaveIsSet && AltSavePathIsCorrect);
  if (useAltPath && getExtension(std::get<std::string>(*pathToSave)) == 0) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемое расширение для пути сохранения.", false);
    return;
  }
  if (!WorkDirIsSet) {
    return;
  }
  NSDoctRenderer::CContext oContext = Cbuild.GetContext();
  NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
  NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
  NSDoctRenderer::CValue oApi = oGlobal["Api"];

  NSDoctRenderer::CValue oDocument = oApi.Call("GetDocument");

  std::vector<std::string> keysAndValuesList = splitString(kAV, ";");
  for (std::string kAV : keysAndValuesList) {
    std::vector<std::string> keyAndValue = splitString(kAV, "=");
    if (keyAndValue.size() == 2) {
      NSDoctRenderer::CValue oObject = oContext.CreateObject();
      oObject.SetProperty(L"searchString", keyAndValue[0].c_str());
      oObject.SetProperty(L"replaceString", keyAndValue[1].c_str());
      oDocument.Call("SearchAndReplace", oObject);
    }
  }
}

void DocBuilderAddIn::searchAndReplaceOneCMD(const variant_t &pathToTemplate,
                                             const variant_t &keysAndValues,
                                             const variant_t &altPathToSave) {
  if (!fileExists(std::get<std::string>(pathToTemplate))) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Файл не существует. НайтиИЗаменить(СторокаСПутемКФайлу, "
             u8"СтрокаСКлючамиИЗначениями)",
             false);
    return;
  }
  if (!std::holds_alternative<std::string>(keysAndValues) ||
      !std::holds_alternative<std::string>(pathToTemplate)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемые типы данных. "
             u8"НайтиИЗаменить(СторокаСПутемКФайлу, СтрокаСКлючамиИЗначениями,"
             u8" СторокаСПутемКФайлуДляСохранения(Опционально))",
             false);
    return;
  }
  unsigned int extension = getExtension(std::get<std::string>(pathToTemplate));
  if (extension == 0 ||
      extension < static_cast<unsigned int>(ExtensionUINT::DOCX) ||
      extension > static_cast<unsigned int>(ExtensionUINT::RTF)) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемое расширение исходного документа.", false);
    return;
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
    saveExtension = getExtension(std::get<std::string>(altPathToSave));
  }
  if (useAltPath && saveExtension == 0) {
    AddError(ADDIN_E_FAIL, extensionName(),
             u8"Не поддерживаемое расширение для пути сохранения.", false);
    return;
  }
  const wchar_t *BuildPath;
  bool checkWorkDir = (std::holds_alternative<std::string>(*workDir) &&
                       !std::get<std::string>(*workDir).empty());
  if (!checkWorkDir) {
    AddError(ADDIN_E_FAIL, extensionName(), u8"Не задана рабочая директория.",
             false);
  } else {
    BuildPath = stringToWchar(std::get<std::string>(*workDir));
  }
  if (checkWorkDir && pathExists(std::get<std::string>(*workDir))) {
    NSDoctRenderer::CDocBuilder::Initialize(BuildPath);
    NSDoctRenderer::CDocBuilder Cbuild;
    Cbuild.SetProperty("--work-directory", BuildPath);
    Cbuild.OpenFile(wPTT, L"");

    NSDoctRenderer::CContext oContext = Cbuild.GetContext();
    NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
    NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
    NSDoctRenderer::CValue oApi = oGlobal["Api"];

    NSDoctRenderer::CValue oDocument = oApi.Call("GetDocument");

    std::vector<std::string> keysAndValuesList = splitString(kAV, ";");
    for (std::string kAV : keysAndValuesList) {
      std::vector<std::string> keyAndValue = splitString(kAV, "=");
      if (keyAndValue.size() == 2) {
        NSDoctRenderer::CValue oObject = oContext.CreateObject();
        oObject.SetProperty(L"searchString", keyAndValue[0].c_str());
        oObject.SetProperty(L"replaceString", keyAndValue[1].c_str());
        oDocument.Call("SearchAndReplace", oObject);
      }
    }

    Cbuild.SaveFile((useAltPath ? saveExtension : extension),
                    (useAltPath ? wAPS : wPTT));
    Cbuild.CloseFile();
    NSDoctRenderer::CDocBuilder::Dispose();
    delete[] wPTT;
    delete[] BuildPath;
    if (useAltPath) {
      delete[] wAPS;
    }
    char tmpPath[1024] = u8"Документ сформирован. Путь: ";
    strncat(tmpPath, useAltPath ? APS.c_str() : pTT.c_str(),
            useAltPath ? strlen(APS.c_str()) : strlen(pTT.c_str()));
    AddError(ADDIN_E_INFO, extensionName(), tmpPath, false);
  }
}

void DocBuilderAddIn::fillRow(const variant_t &range,
                              const variant_t &rowData) {
  if (!WorkDirIsSet || !FileIsSet) {
    return;
  }
  unsigned int extension = getExtension(std::get<std::string>(*pathToFile));
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
  NSDoctRenderer::CContext oContext = Cbuild.GetContext();
  NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
  NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
  NSDoctRenderer::CValue oApi = oGlobal["Api"];

  NSDoctRenderer::CValue oWorksheet = oApi.Call("GetActiveSheet");
  std::vector<std::string> colDataArr =
      splitString(std::get<std::string>(rowData), ";");
  NSDoctRenderer::CValue rArray = oContext.CreateArray(1);
  NSDoctRenderer::CValue cArray = oContext.CreateArray(colDataArr.size());
  for (int i = 0; i < colDataArr.size(); i++) {
    cArray[i] = colDataArr[i].c_str();
  }
  rArray[0] = cArray;
  std::string sRange = std::get<std::string>(range);
  oWorksheet.Call("GetRange", NSDoctRenderer::CDocBuilderValue(sRange.c_str()))
      .Call("SetValue", rArray);
}

bool DocBuilderAddIn::checkRangeInOneRow(const std::string range) {
  bool res = false;
  std::string rangeCpy = range;
  int rows[3];
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
  std::vector<std::string> result;
  size_t pos = 0;
  std::string token;
  while ((pos = source.find(delimiter)) != std::string::npos) {
    token = source.substr(0, pos);
    result.push_back(token);
    source.erase(0, pos + delimiter.length());
  }
  result.push_back(source);
  return result;
}
wchar_t *DocBuilderAddIn::stringToWchar(const std::string &str) {
  size_t len = str.length();
  wchar_t *res = new wchar_t[len + 1];
  const char *cstr = str.c_str();
  std::mbstowcs(res, cstr, len);
  res[len] = L'\0';
  return res;
}

unsigned int DocBuilderAddIn::getExtension(const variant_t &path) {
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
  } else if (ext == "xltx") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::XLTX);
  } else if (ext == "ots") {
    extUInt = static_cast<unsigned int>(ExtensionUINT::OTS);
  }
  return extUInt;
}

bool DocBuilderAddIn::fileExists(const variant_t &path) {
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
  bool res = false;
  if (!std::holds_alternative<std::string>(path) || !fileExists(path)) {
    return false;
  }
  std::string pathToFile = std::get<std::string>(path);
  struct stat bf;
  stat(pathToFile.c_str(), &bf) == 0 ? res = true : res = false;
  return res;
}

void DocBuilderAddIn::saveAndCloseFile() {
  if (WorkDirIsSet && FileIsSet) {
    std::string path = std::get<std::string>(*pathToSave);
    wchar_t *wpath = stringToWchar(path);
    Cbuild.SaveFile(getExtension(path), wpath);
    Cbuild.CloseFile();
    delete[] wpath;
  }
}

void DocBuilderAddIn::sleep(const variant_t &delay) {
  using namespace std;
  // It safe to get any type from variant.
  // Exceptions are handled by component API.
  this_thread::sleep_for(chrono::seconds(get<int32_t>(delay)));
}

// Out params support option must be enabled for this to work
void DocBuilderAddIn::assign(variant_t &out) { out = true; }

// Despite that you can return property value through method this is not
// recommended due to unwanted data copying
variant_t DocBuilderAddIn::samplePropertyValue() { return *sample_property; }

variant_t DocBuilderAddIn::currentDate() {
  using namespace std;
  tm current{};
  time_t t = time(nullptr);
#ifdef _WINDOWS
  localtime_s(&current, &t);
#else
  localtime_r(&t, &current);
#endif
  return current;
}

void DocBuilderAddIn::getColIndx(const std::string cell, int *indx) {
  *indx = 0;
  char *cellCpy = (char *)calloc(cell.size() + 1, sizeof(char));
  strncpy(cellCpy, cell.c_str(), cell.size());
  cellCpy[cell.size()] = '\0';
  int len = cell.size();
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
  if (indx > 25) {
    getColLetters(indx / 26 - 1, res_string);
  }
  *res_string += indx % 26 + 'A';
}

void DocBuilderAddIn::getRowIndx(const std::string cell, int *indx) {
  *indx = 0;
  int len = (int)cell.size();
  char num[len + 1];
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
}

std::string DocBuilderAddIn::getNextCellInRange(const std::string range,
                                                const std::string prevCell) {
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

variant_t DocBuilderAddIn::getDataFromRange(const variant_t &range) {
  if (!WorkDirIsSet || !FileIsSet) {
    return "";
  }
  std::string lRange = std::get<std::string>(range);
  std::string res = "";
  NSDoctRenderer::CContext oContext = Cbuild.GetContext();
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
    AddError(ADDIN_E_INFO, extensionName(), cell, false);
    NSDoctRenderer::CValue oCell = oWorksheet.Call(
        "GetRange", NSDoctRenderer::CDocBuilderValue(cell.c_str())
    );
    NSDoctRenderer::CValue oValue = oCell.Call("GetValue");
    NSDoctRenderer::CString val = oValue.ToString();
    wchar_t *w_val = val.c_str();
    if (w_val != NULL) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        res += converter.to_bytes(w_val);
        res += ";";
    }
    
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