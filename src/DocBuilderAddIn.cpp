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

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <iostream>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
#endif
#ifdef __linux__
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif

#include "DocBuilderAddIn.h"


std::string DocBuilderAddIn::extensionName() {
    return "DocBuilder";
}

DocBuilderAddIn::DocBuilderAddIn() {
    // Universal property. Could store any supported by native api type.
    sample_property = std::make_shared<variant_t>();
    AddProperty(L"SampleProperty", L"ОбразецСвойства", sample_property);
    
    workDir = std::make_shared<variant_t>();
    AddProperty(L"WorkDir", L"РабочаяДиректория", [&]() { return workDir; }, [&](variant_t &&v) {
        *workDir = std::move(v);
        WorkDirIsSet = (
            std::holds_alternative<std::string>(*workDir)
            && !std::get<std::string>(*workDir).empty()
            && pathExists(std::get<std::string>(*workDir))
        );
        if (!WorkDirIsSet) {
            AddError(
                ADDIN_E_FAIL, extensionName(),
                u8"Рабочая директория не существует.",
                false
            );
        } else {
            wchar_t *BuildPath = stringToWchar(std::get<std::string>(*workDir));    
            NSDoctRenderer::CDocBuilder::Initialize(BuildPath);
            delete[] BuildPath;
        };
    });
    
    pathToFile = std::make_shared<variant_t>();
    AddProperty(L"PathToFile", L"ПутьКФайлу", [&]() { return pathToFile; }, [&](variant_t &&v){
        *pathToFile = std::move(v);
        if (WorkDirIsSet) {
            FileIsSet = (
                std::holds_alternative<std::string>(*pathToFile)
                && !std::get<std::string>(*pathToFile).empty()
                && getExtension(std::get<std::string>(*pathToFile)) != 0
            );
            if (!FileIsSet) {
                AddError(
                    ADDIN_E_FAIL, extensionName(),
                    u8"Файл не задан.",
                    false
                );
            } else {
                wchar_t *BuildPath = stringToWchar(std::get<std::string>(*workDir)); 
                Cbuild.SetProperty("--work-directory", BuildPath);
                wchar_t *wPTT = stringToWchar(std::get<std::string>(*pathToFile));
                if (fileExists(std::get<std::string>(*pathToFile))) {
                    Cbuild.OpenFile(wPTT, L"");
                    *pathToSave = std::get<std::string>(*pathToFile);
                } else {
                    Cbuild.CreateFile(getExtension(std::get<std::string>(*pathToFile))); 
                    NewFileIsCreated = true;   
                }  
                delete[] wPTT;
                delete[] BuildPath;
            }
        };
    });

    pathToSave = std::make_shared<variant_t>();
    AddProperty(L"PathToSave", L"ПутьКФайлуДляСохранения", [&]() { return pathToSave; }, [&](variant_t &&v) {
        *pathToSave = std::move(v);
        AltPathToSaveIsSet = true;
        AltSavePathIsCorrect = (
            std::holds_alternative<std::string>(*pathToSave)
            && !std::get<std::string>(*pathToSave).empty()
            && getExtension(std::get<std::string>(*pathToSave)) != 0
        );
        if (!AltSavePathIsCorrect) {
            AddError(
                ADDIN_E_FAIL, extensionName(),
                u8"Файл для сохранения задан некорректно.",
                false
            );
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
    AddMethod(L"SearchAndReplace", L"НайтиИЗаменить", this, &DocBuilderAddIn::searchAndReplace);
    AddMethod(L"SearchAndReplaceOneCMD", L"НайтиИЗаменитьОднойКМНД", this, &DocBuilderAddIn::searchAndReplace);
    AddMethod(L"TestFile", L"ТестФайл", this, &DocBuilderAddIn::testfile);
    AddMethod(L"CurrentDate", L"ТекущаяДата", this, &DocBuilderAddIn::currentDate);
    AddMethod(L"Assign", L"Присвоить", this, &DocBuilderAddIn::assign);
    AddMethod(L"SamplePropertyValue", L"ЗначениеСвойстваОбразца", this, &DocBuilderAddIn::samplePropertyValue);

    AddMethod(L"SaveAndCloseFile", L"СохранитьИЗакрытьФайл", this, &DocBuilderAddIn::saveAndCloseFile);

    // Method registration with default arguments
    //
    // Notice that if variant_t would be non-copy you can't use initializer list.
    // Proper way to register def args would be then:
    //        std::map<long, variant_t> def_args;
    //        def_args.insert({0, 5});
    //        AddMethod(u"Sleep", u"Ожидать", this, &DocBuilderAddIn::sleep, std::move(def_args));
    //
    AddMethod(L"Sleep", L"Ожидать", this, &DocBuilderAddIn::sleep, {{0, 5}});

}

// Sample of addition method. Support both integer and string params.
// Every exceptions derived from std::exceptions are handled by components API
variant_t DocBuilderAddIn::add(const variant_t &a, const variant_t &b) {
    if (std::holds_alternative<int32_t>(a) && std::holds_alternative<int32_t>(b)) {
        return std::get<int32_t>(a) + std::get<int32_t>(b);
    } else if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b)) {
        return std::string{std::get<std::string>(a) + std::get<std::string>(b)};
    } else {
        throw std::runtime_error(u8"Неподдерживаемые типы данных");
    }
}

void DocBuilderAddIn::message(const variant_t &msg) {
    std::visit(overloaded{
            [&](const std::string &v) { AddError(ADDIN_E_INFO, extensionName(), v, false); },
            [&](const int32_t &v) {
                AddError(ADDIN_E_INFO, extensionName(), std::to_string(static_cast<int>(v)), false);
            },
            [&](const double &v) { AddError(ADDIN_E_INFO, extensionName(), std::to_string(v), false); },
            [&](const bool &v) {
                AddError(ADDIN_E_INFO, extensionName(), std::string(v ? u8"Истина" : u8"Ложь"), false);
            },
            [&](const std::tm &v) {
                std::ostringstream oss;
                oss.imbue(std::locale("ru_RU.utf8"));
                oss << std::put_time(&v, "%c");
                AddError(ADDIN_E_INFO, extensionName(), oss.str(), false);
            },
            [&](const std::vector<char> &v) {},
            [&](const std::monostate &) {}
    }, msg);
}

 void DocBuilderAddIn::searchAndReplace(const variant_t &keysAndValues) {
    if (!std::holds_alternative<std::string>(keysAndValues)) {
        AddError(
            ADDIN_E_FAIL, extensionName(),
            u8"Не поддерживаемые типы данных. НайтиИЗаменить(СтрокаСКлючамиИЗначениями(напр.: 'ключ=значение;')",
            false
        );
        return;
    }
    unsigned int extension = getExtension(std::get<std::string>(*pathToFile));
    if (extension == 0 || extension < static_cast<unsigned int>(ExtensionUINT::DOCX) || extension > static_cast<unsigned int>(ExtensionUINT::RTF)) {
        AddError(
            ADDIN_E_FAIL, extensionName(),
            u8"Не поддерживаемое расширение исходного документа.",
            false
        );
        return;
    }
    
    std::string kAV = std::get<std::string>(keysAndValues);
    bool useAltPath = (AltPathToSaveIsSet && AltSavePathIsCorrect);
    if (useAltPath && getExtension(std::get<std::string>(*pathToSave)) == 0) {
        AddError(
            ADDIN_E_FAIL, extensionName(),
            u8"Не поддерживаемое расширение для пути сохранения.",
            false
        );
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

void DocBuilderAddIn::searchAndReplaceOneCMD(const variant_t &pathToTemplate, const variant_t &keysAndValues, const variant_t &altPathToSave) {
    if (!fileExists(std::get<std::string>(pathToTemplate))) {
        AddError(
            ADDIN_E_FAIL, extensionName(),
            u8"Файл не существует. НайтиИЗаменить(СторокаСПутемКФайлу, СтрокаСКлючамиИЗначениями)",
            false
        );
        return;
    }
    if (!std::holds_alternative<std::string>(keysAndValues) || !std::holds_alternative<std::string>(pathToTemplate)) {
        AddError(
            ADDIN_E_FAIL, extensionName(),
            u8"Не поддерживаемые типы данных. НайтиИЗаменить(СторокаСПутемКФайлу, СтрокаСКлючамиИЗначениями,"
            u8" СторокаСПутемКФайлуДляСохранения(Опционально))",
            false
        );
        return;
    }
    unsigned int extension = getExtension(std::get<std::string>(pathToTemplate));
    if (extension == 0 || extension < static_cast<unsigned int>(ExtensionUINT::DOCX) || extension > static_cast<unsigned int>(ExtensionUINT::RTF)) {
        AddError(
            ADDIN_E_FAIL, extensionName(),
            u8"Не поддерживаемое расширение исходного документа.",
            false
        );
        return;
    }
    
    std::string kAV = std::get<std::string>(keysAndValues);
    std::string pTT = std::get<std::string>(pathToTemplate);
    wchar_t *wPTT = stringToWchar(pTT);
    bool useAltPath = (std::holds_alternative<std::string>(altPathToSave) && !std::get<std::string>(altPathToSave).empty());
    int saveExtension = 0;
    std::string APS;
    wchar_t *wAPS;
    if (useAltPath) {
        APS = std::get<std::string>(altPathToSave);
        wAPS = stringToWchar(APS);
        saveExtension = getExtension(std::get<std::string>(altPathToSave));
    }
    if (useAltPath && saveExtension == 0) {
        AddError(
            ADDIN_E_FAIL, extensionName(),
            u8"Не поддерживаемое расширение для пути сохранения.",
            false
        );
        return;
    }
    const wchar_t *BuildPath;
    bool checkWorkDir = (std::holds_alternative<std::string>(*workDir) && !std::get<std::string>(*workDir).empty());
    if (!checkWorkDir) {
        AddError(
            ADDIN_E_FAIL, extensionName(),
            u8"Не задана рабочая директория.",
            false
        );
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
        
        Cbuild.SaveFile((useAltPath ? saveExtension : extension), (useAltPath ? wAPS : wPTT));
        Cbuild.CloseFile();
        NSDoctRenderer::CDocBuilder::Dispose(); 
        delete[] wPTT; 
        delete[] BuildPath;
        if (useAltPath) {
            delete[] wAPS;
        } 
        char tmpPath[1024] = u8"Документ сформирован. Путь: ";
        strncat(tmpPath, useAltPath ? APS.c_str() : pTT.c_str(), useAltPath ? strlen(APS.c_str()) : strlen(pTT.c_str()));
        AddError(
            ADDIN_E_INFO, extensionName(),
            tmpPath,
            false
        );
    }
}

std::vector<std::string>  DocBuilderAddIn::splitString(std::string source, std::string delimiter) {
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
wchar_t* DocBuilderAddIn::stringToWchar(const std::string& str) {
    size_t len = str.length();
    wchar_t* res = new wchar_t[len + 1];
    const char* cstr = str.c_str();
    std::mbstowcs(res, cstr, len);
    res[len] = L'\0';
    return res;
}

unsigned int DocBuilderAddIn::getExtension(const variant_t &path) {
    if (!std::holds_alternative<std::string>(path)) {
        AddError(ADDIN_E_FAIL, extensionName(), u8"Не поддерживаемые типы данных. Тип пути должен быть строкой и включать имя файла с расширением.", false);
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
    if (!std::holds_alternative<std::string>(path)  || !fileExists(path)) {
        return false;
    }
    std::string pathToFile = std::get<std::string>(path);
    struct stat bf;
    stat(pathToFile.c_str(), &bf) == 0 ? res = true : res = false;
    return res;
}

void DocBuilderAddIn::testfile() {
    const wchar_t *BuildPath = L"/opt/onlyoffice/documentbuilder/";
    NSDoctRenderer::CDocBuilder::Initialize(BuildPath);
    NSDoctRenderer::CDocBuilder Cbuild;
    Cbuild.SetProperty("--work-directory", BuildPath);
    Cbuild.CreateFile(OFFICESTUDIO_FILE_DOCUMENT_DOCX);
    
    NSDoctRenderer::CContext oContext = Cbuild.GetContext();
    NSDoctRenderer::CContextScope oScope = oContext.CreateScope();
    NSDoctRenderer::CValue oGlobal = oContext.GetGlobal();
    NSDoctRenderer::CValue oApi = oGlobal["Api"];

    // Create basic form
    NSDoctRenderer::CValue oDocument = oApi.Call("GetDocument");
    NSDoctRenderer::CValue oParagraph = oDocument.Call("GetElement", 0);
    NSDoctRenderer::CValue oHeadingStyle = oDocument.Call("GetStyle", "Heading 3");

    oParagraph.Call("AddText", "Employee pass card");
    oParagraph.Call("SetStyle", oHeadingStyle);
    oDocument.Call("Push", oParagraph);

    Cbuild.SaveFile(OFFICESTUDIO_FILE_DOCUMENT_DOCX, L"/home/klaxonvs/Desktop/xxx.docx");
    Cbuild.CloseFile();
    NSDoctRenderer::CDocBuilder::Dispose();
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
void DocBuilderAddIn::assign(variant_t &out) {
    out = true;
}

// Despite that you can return property value through method this is not recommended
// due to unwanted data copying
variant_t DocBuilderAddIn::samplePropertyValue() {
    return *sample_property;
}

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
