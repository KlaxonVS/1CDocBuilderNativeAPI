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

  variant_t add(const variant_t &a, const variant_t &b);

  void message(const variant_t &msg);
  // Doc API
  void searchAndReplace(const variant_t &keysAndValues);
  void searchAndReplaceOneCMD(const variant_t &pathToTemplate,
                              const variant_t &keysAndValues,
                              const variant_t &altPathToSave);
  // Doc API End

  // Spreadsheet API
  void fillRow(const variant_t &range, const variant_t &rowData);
  variant_t getDataFromRange(const variant_t &range);
  // Spreadsheet API End

  void sleep(const variant_t &delay);

  void assign(variant_t &out);

  void saveAndCloseFile();

  variant_t samplePropertyValue();

  variant_t currentDate();

  std::shared_ptr<variant_t> sample_property;
  std::shared_ptr<variant_t> workDir;
  std::shared_ptr<variant_t> pathToFile;
  std::shared_ptr<variant_t> pathToSave;
  NSDoctRenderer::CDocBuilder Cbuild;
  bool FileIsSet = false;
  bool NewFileIsCreated = false;
  bool WorkDirIsSet = false;
  bool AltPathToSaveIsSet = false;
  bool AltSavePathIsCorrect = false;

  wchar_t *stringToWchar(const std::string &str);
  std::vector<std::string> splitString(std::string source,
                                       std::string delimiter);

  unsigned int getExtension(const variant_t &path);
  bool fileExists(const variant_t &path);

  bool pathExists(const variant_t &path);

  bool checkRangeInOneRow(const std::string range);
  // void setBorders(const variant_t &range, const variant_t &borders,
  //                 const variant_t &type, const variant_t &color);

  // std::string getCellInRange(const std::string range, const std::string prevCell);
  std::string getNextCellInRange(const std::string range, const std::string prevCell);
  void getRowIndx(const std::string cell, int *indx);
  void getColLetters(int indx, std::string *res_string);
  void getColIndx(const std::string cell, int *indx);


  enum class ExtensionUINT {
    DOCX = OFFICESTUDIO_FILE_DOCUMENT_DOCX,
    DOC = OFFICESTUDIO_FILE_DOCUMENT_DOC,
    ODT = OFFICESTUDIO_FILE_DOCUMENT_ODT,
    RTF = OFFICESTUDIO_FILE_DOCUMENT_RTF,
    PDF = OFFICESTUDIO_FILE_CROSSPLATFORM_PDF,
    XLSX = OFFICESTUDIO_FILE_SPREADSHEET_XLSX,
    XLS = OFFICESTUDIO_FILE_SPREADSHEET_XLS,
    ODS = OFFICESTUDIO_FILE_SPREADSHEET_ODS,
    CSV = OFFICESTUDIO_FILE_SPREADSHEET_CSV,
    XLTX = OFFICESTUDIO_FILE_SPREADSHEET_XLTX,
    OTS = OFFICESTUDIO_FILE_SPREADSHEET_OTS
  };
};

#endif  // DocBuilderAddIn_H
