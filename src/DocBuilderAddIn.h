#ifndef DocBuilderAddIn_H
#define DocBuilderAddIn_H

#include "Component.h"
#include "common.h"
#include "docbuilder.h"

class DocBuilderAddIn final : public Component {
 public:
  const char *Version = u8"1.0.0";

  DocBuilderAddIn();

 private:
  std::string extensionName() override;

  void message(const variant_t &msg);
  // Doc API
  void searchAndReplace(const variant_t &keysAndValues);
  variant_t searchAndReplaceOneCMD(const variant_t &pathToTemplate,
                                   const variant_t &keysAndValues,
                                   const variant_t &altPathToSave);
  // Doc API End

  // Spreadsheet API
  void fillRow(const variant_t &range, const variant_t &rowData);
  variant_t getDataFromRange(const variant_t &range);
  void initGetDataFromRangeByCell(const variant_t &range);
  variant_t isGettingDataFromRange();
  variant_t getNextCell();
  // Spreadsheet API End

  variant_t saveAndCloseFile();
  variant_t closeFile();

  std::shared_ptr<variant_t> workDir;
  std::shared_ptr<variant_t> pathToFile;
  std::shared_ptr<variant_t> pathToSave;
  NSDoctRenderer::CDocBuilder Cbuild;
  bool FileIsSet = false;
  bool NewFileIsCreated = false;
  bool WorkDirIsSet = false;
  bool PathToSaveIsSet = false;
  bool initGetDataFromRangeByCellIsSet = false;
  std::string spreadsheetRange = "";
  std::string spreadsheetCell = "";

  wchar_t *stringToWchar(const std::string &str);
  std::vector<std::string> splitString(std::string source,
                                       std::string delimiter);

  std::string wcharToString(const wchar_t *wstr);

  unsigned int getExtension(const variant_t &path);
  bool fileExists(const variant_t &path);
  bool pathExists(const variant_t &path);

  bool checkRangeInOneRow(const std::string range);
  std::string getNextCellInRange(const std::string range,
                                 const std::string prevCell);
  void getRowIndx(const std::string cell, int *indx);
  void getColLetters(int indx, std::string *res_string);
  void getColIndx(const std::string cell, int *indx);
  bool checkRange(const std::string range);

  enum class ExtensionUINT {
    DOCX = OFFICESTUDIO_FILE_DOCUMENT_DOCX,
    DOC = OFFICESTUDIO_FILE_DOCUMENT_DOC,
    ODT = OFFICESTUDIO_FILE_DOCUMENT_ODT,
    RTF = OFFICESTUDIO_FILE_DOCUMENT_RTF,
    PDF = OFFICESTUDIO_FILE_CROSSPLATFORM_PDF,
    XLSX = OFFICESTUDIO_FILE_SPREADSHEET_XLSX,
    XLS = OFFICESTUDIO_FILE_SPREADSHEET_XLS,
    ODS = OFFICESTUDIO_FILE_SPREADSHEET_ODS,
    CSV = OFFICESTUDIO_FILE_SPREADSHEET_CSV,  // TODO: Add CSV support
    XLTX = OFFICESTUDIO_FILE_SPREADSHEET_XLTX,
    OTS = OFFICESTUDIO_FILE_SPREADSHEET_OTS
  };

  // void setBorders(const variant_t &range, const variant_t &borders,
  //                 const variant_t &type, const variant_t &color);

  // std::string getCellInRange(const std::string range, const std::string
  // prevCell);
};

#endif  // DocBuilderAddIn_H
