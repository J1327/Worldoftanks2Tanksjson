#include "pch.h"        // Header
#include"json\json.h"    // parse json
#include <Windows.h>      // Windows API
#include "tinyxml2.h"      // parse xml
#include <fstream>          // I/O
#include <filesystem>        // R/W
#include <iostream>           // R/W

namespace fs = std::filesystem;

std::wstring GetApplicationDir()
{
    wchar_t exe_name[MAX_PATH + 1] = { '\0' };
    ::GetModuleFileNameW(nullptr, exe_name, MAX_PATH);
    return std::wstring(exe_name);
}
std::wstring ExecuteCommand(const std::wstring& command) {
    std::wstring output;
    SECURITY_ATTRIBUTES saAttr;
    HANDLE hReadPipe, hWritePipe;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
        return output;
    }
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;
    if (CreateProcess(NULL, const_cast<LPWSTR>(command.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hWritePipe);
        DWORD dwRead;
        CHAR chBuf[4096];
        std::string outputStr;
        while (true) {
            if (!ReadFile(hReadPipe, chBuf, sizeof(chBuf) - 1, &dwRead, NULL) || dwRead == 0) {
                break;
            }
            chBuf[dwRead] = '\0';
            outputStr += chBuf;
        }
        CloseHandle(hReadPipe);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        output = std::wstring(outputStr.begin(), outputStr.end());
    }
    return output;
}

// Convert from wstring to string
std::string wstringToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// function to search in .po files
bool FindNext(const std::wstring& searchString, const std::wstring& filepath, std::wstring& foundLine)
{
    std::wifstream file(filepath);
    if (!file) {return false;}
    std::wstring line;
    bool found = false;
    bool captureNextLine = false;

    while (std::getline(file, line))
    {
        if (captureNextLine) {foundLine = line;found = true;break;}
        if (line.find(searchString) != std::wstring::npos) {captureNextLine = true;}
    }

    return found;
}

int main()
{

    std::wstring _space = L"\n";
    std::wstring _gpath = L"G:\\World_of_Tanks_EU\\";

    std::wcout << L"Enter a path for World Of Tanks (press Enter to keep default G:\\World_of_Tanks_EU\\): ";
        std::wstring newPath;
        std::getline(std::wcin, newPath);
        if (!newPath.empty()){_gpath = newPath;}
        std::wcout << L"Game Path directory: " << _gpath << std::endl;

    std::wstring _lpath = L"res\\text\\LC_MESSAGES\\";
        _gpath.append(_lpath);
        std::wcout << L"Path to decompiling (primary directory) : " << _gpath << _lpath << std::endl;

    std::wstring _vpath = L"res\\scripts\\item_defs\\vehicles\\";
        _gpath.erase(_gpath.size() - _lpath.size());
        _gpath.append(_vpath);
        std::wcout << L"Path to decompiling (secondary directory) : " << _gpath << _vpath << std::endl;
    
     _gpath.erase(_gpath.size() - _vpath.size());
    
    std::wstring _moextension = L"*.mo";
    std::wstring path = _gpath + _lpath;
    std::wstring secondarypath = _gpath + _vpath;     

    try
    {
        fs::path appDirPath = GetApplicationDir();
        fs::path outputDirPath = appDirPath.parent_path();
        fs::path outputFolderPath = outputDirPath / L"output";
        fs::path XMLoutputFolderPath = outputDirPath / L"XMLoutput";
        fs::path JSONoutputFolderPath = outputDirPath / L"JSONoutput";

        fs::create_directory(outputFolderPath);
        fs::create_directory(XMLoutputFolderPath);
        fs::create_directory(JSONoutputFolderPath);

        for (const auto& entry : fs::directory_iterator(path))
        {
            if (entry.is_regular_file())
            {
                std::wstring filename = entry.path().filename();
                if (filename.find(L"_vehicles.mo") != std::wstring::npos)
                {
                    fs::path outputFilePath = outputFolderPath / filename;
                    outputFilePath.replace_extension(L".po");
                    std::wstring command = L"msgunfmt.exe \"" + entry.path().wstring() + L"\" -o " + outputFilePath.wstring();
                        std::string narrowCommand;
                        narrowCommand.resize(command.size());
                        size_t bufferSize = command.length() + 1;
                        size_t convertedChars = 0;
                        wcstombs_s(&convertedChars, narrowCommand.data(), bufferSize, command.c_str(), _TRUNCATE);
                        int motopoconversioncheck = system(narrowCommand.c_str());
                        std::size_t underscorePos = filename.find(L"_vehicles.mo");
                    std::wstring name = filename.substr(0, underscorePos);
                    std::wstring _wpath = secondarypath + name;

                    size_t gbPos = name.find(L"gb");
                    if (gbPos != std::wstring::npos) {
                      name.replace(gbPos, 2, L"uk");
                      _wpath = secondarypath + name;
                    }

                    if (fs::exists(_wpath))
                    {
                        fs::path listXmlPath = _wpath + L"\\list.xml";
                        if (fs::exists(listXmlPath))
                        {
                            std::wstring sc = L"wottoolslib.exe " + listXmlPath.wstring(); 
                            std::wstring output = ExecuteCommand(sc);
                            std::wstring XMLOutput = name + L".xml";
                            fs::path outputFile = XMLoutputFolderPath / XMLOutput;

                            std::ofstream createFile(outputFile);
                            createFile.close();

                            std::wofstream ofs(outputFile);
                            if (!ofs) {std::wcerr << L"Error: Unable to open the output file." << outputFile << std::endl;return 1;}

                            ofs << output << std::endl;

                            ofs.close();

                            if (!output.empty())
                            {
                               std::string narrowOutput;
                               narrowOutput.resize(output.size());
                               std::wcstombs(&narrowOutput[0], output.c_str(), output.size());
                                    tinyxml2::XMLDocument doc;
                                    if (doc.Parse(narrowOutput.c_str()) != tinyxml2::XML_SUCCESS)
                                    {std::cout << "XML parsing failed." << std::endl;return -1;}
                                    else {
                                        //Debug
                                        //tinyxml2::XMLElement* root = doc.RootElement();
                                        //PrintXMLElement(root);
                                    }

                                std::wstring JSONOutput = name + L".json";
                                fs::path JSONoutputFile = JSONoutputFolderPath / JSONOutput;
                                std::ofstream createFile(JSONoutputFile);
                                if (!createFile) {
                                    std::wcerr << "Error with-in new created .json file" << std::endl;
                                    return 1;
                                }
                                createFile.close();

                                Json::Value jsonData;
                                tinyxml2::XMLElement* root = doc.RootElement();

                                if (root) {
                                            Json::Value rootJson(Json::objectValue);
                                            rootJson[root->Name()] = Json::Value(Json::arrayValue);
                                            tinyxml2::XMLElement* element = root->FirstChildElement();

                                                while (element) {
                                                     Json::Value itemJson(Json::objectValue);
                                                     for (const tinyxml2::XMLAttribute* attribute = element->FirstAttribute(); attribute; attribute = attribute->Next()) {
                                                       std::string attributeName = attribute->Name();
                                            std::string attributeValue = attribute->Value();
                                            attributeValue.erase(std::remove(attributeValue.begin(), attributeValue.end(), '\t'), attributeValue.end());
                                            itemJson[attributeName] = attributeValue;
                                        }
                                        for (const tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
                                            std::string elementName = child->Name();
                                            std::string elementValue = child->GetText();
                                            elementValue.erase(std::remove(elementValue.begin(), elementValue.end(), '\t'), elementValue.end());

                                            std::string prefix = "#";                                    
                                            std::string nationName = wstringToString(name);                            
                                            prefix.append(nationName).append("_vehicles:");                              
                                            std::string::size_type pos = elementValue.find(prefix);                                
                                            if (pos != std::string::npos) {elementValue = elementValue.substr(pos + prefix.length());} 
                                            const std::string Premiumprefix = "#igr_vehicles:";if (elementValue.find(Premiumprefix) == 0) {elementValue = elementValue.substr(Premiumprefix.length());}

             
                                            if (elementName == "id") {
                                                elementName = "tankid";
                                            }
                                            else if (elementName == "notInShop") {
                                                elementName = "Hidden";
                                            }
                                            else if (elementName == "level") {
                                                elementName = "tier";
                                            }
                                            else if (elementName == "userString") {
                                                elementName = "codename";
                                                std::wstring wElementValue(elementValue.begin(), elementValue.end());
                                                std::wstring searchString = L"msgid \"" + wElementValue + L"\"";
                                                std::wstring foundLine;
                                                if (FindNext(searchString, outputFilePath, foundLine)) {
                                                    if (foundLine.compare(0, 8, L"msgstr \"") == 0 && foundLine.back() == L'"') {
                                                        std::wstring extractedText = foundLine.substr(8, foundLine.size() - 9);
                                                        std::string extractedString(extractedText.begin(), extractedText.end());
                                                        itemJson[Json::StaticString("vehicle")] = extractedString;
                                                    }
                                                }
                                                else {
                                                    std::wstring popremiumfilename = L"igr_vehicles.po";
                                                    std::wstring popremiumFilePath = outputFolderPath;
                                                    popremiumFilePath.append(L"\\");
                                                    popremiumFilePath.append(popremiumfilename);
                                                    if (fs::exists(popremiumFilePath)) {
                                                        if (FindNext(searchString, popremiumFilePath, foundLine)) {
                                                            if (foundLine.compare(0, 8, L"msgstr \"") == 0 && foundLine.back() == L'"') {
                                                                std::wstring extractedText = foundLine.substr(8, foundLine.size() - 9);
                                                                std::string extractedString(extractedText.begin(), extractedText.end());
                                                                itemJson[Json::StaticString("vehicle")] = extractedString;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            
                                            if (elementName == "price") {
                                                const tinyxml2::XMLElement* goldElement = child->FirstChildElement("gold");
                                                if (goldElement) { itemJson["premium"] = "true"; }
                                                else {itemJson["premium"] = "false";}
                                            }

                                            if (elementName == "tags") {
                                                if (elementValue.find("lightTank") != std::string::npos) {itemJson["Type"] = "LT";}
                                                else if (elementValue.find("mediumTank") != std::string::npos) {itemJson["Type"] = "MT";}
                                                else if (elementValue.find("heavyTank") != std::string::npos) {itemJson["Type"] = "HT";}
                                                else if (elementValue.find("AT-SPG") != std::string::npos) {itemJson["Type"] = "TD";}
                                                else if (elementValue.find("SPG") != std::string::npos) {itemJson["Type"] = "SPG";}
                                            };

                                            const tinyxml2::XMLElement* notInShopElement = element->FirstChildElement("notInShop");
                                            if (notInShopElement) { itemJson["Hidden"] = "true";}
                                            else {itemJson["Hidden"] = "false";}

                                            itemJson[elementName] = elementValue;
                                        }

                                        itemJson["nationId"] = std::string(name.begin(), name.end());
                                        itemJson.removeMember("tags");

                                        rootJson[root->Name()].append(itemJson);

                                        element = element->NextSiblingElement();
                                    }
                                    jsonData.append(rootJson);
                                }

                                std::ofstream outputFile(JSONoutputFile.string());
                                if (outputFile.is_open()) {
                                    Json::StreamWriterBuilder writer;
                                    std::string jsonString = Json::writeString(writer, jsonData);
                                    outputFile << jsonString;
                                    outputFile.close();
                                }
                                else {std::cout << "Unable to open the output file." << std::endl;}
                                std::wcout << L"JSON FILE created successfully." << std::endl;
                                std::wcout << JSONoutputFile << std::endl;
                            }
                            else{std::wcout << L"ERR-OR. Failed to parase .mo file, maybe you missing wottoolslib?" << std::endl;}
                        }
                        else{std::wcout << L"list.xml file not found in the directory." << std::endl;}
                    }
                    else{
                        // suppress known unused folder names (not nation)
                        size_t aPos = name.find(L"multinational");
                        if (aPos != std::wstring::npos) { continue; }
                        size_t bPos = name.find(L"igr");
                        if (bPos != std::wstring::npos) { continue; }
                        std::wcout << L"Directory does not exist: " << _wpath << std::endl;}
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {std::wcout << L"File system error: " << e.what() << std::endl;}
    
    std::wcout << L"Execution completed." << std::endl;
    system("pause");
return 0;
}