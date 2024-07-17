/*
 *  Modern Native AddIn
 *  Copyright (C) 2018  Infactum
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

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

    void testfile();

    void searchAndReplace(const variant_t &keysAndValues);
    void searchAndReplaceOneCMD(const variant_t &pathToTemplate, const variant_t &keysAndValues, const variant_t &altPathToSave);


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
        


    wchar_t* stringToWchar(const std::string& str);
    std::vector<std::string>  splitString(std::string source, std::string delimiter);

    unsigned int getExtension(const variant_t &path);
    bool fileExists(const variant_t &path);

    bool pathExists(const variant_t &path);

    enum class ExtensionUINT {
        DOCX = OFFICESTUDIO_FILE_DOCUMENT_DOCX,
        DOC = OFFICESTUDIO_FILE_DOCUMENT_DOC,
        ODT = OFFICESTUDIO_FILE_DOCUMENT_ODT,
        RTF = OFFICESTUDIO_FILE_DOCUMENT_RTF,
        PDF = OFFICESTUDIO_FILE_CROSSPLATFORM_PDF
    };
};

#endif //DocBuilderAddIn_H
