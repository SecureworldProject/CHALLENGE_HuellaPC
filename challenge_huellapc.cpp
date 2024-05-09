/////  FILE INCLUDES  /////
#include "pch.h"
#include "context_challenge.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <string>
#include <psapi.h>
#include <iostream>
#include <sstream>
#include <locale>
#include <codecvt>
#include <TlHelp32.h>



/////  DEFINITIONS  /////
#define CHPRINT(...) do { printf("CHALLENGE_HUELLAPC --> "); printf(__VA_ARGS__); } while (0)
#define ERRCHPRINT(...) do { fprintf(stderr, "ERROR in CHALLENGE_HUELLAPC --> "); fprintf(stderr, __VA_ARGS__); } while (0)

/////  GLOBAL VARIABLES  /////
char** programs = NULL;
const char* expectedDomain = NULL;
const char* expectedSufix = NULL;
const char* expectedWebex = NULL;
const char* expectedFont = NULL;
const int COMPUTERNAME_LENGTH = 30;

/////  FUNCTION DEFINITIONS  /////
void getChallengeParameters();
int checkDomain(const char * expectedValue);
int checkPCName(const char * expectedValue);
int checkWebex(const char * expectedValue);
int checkFont(const char * expectedValue);
int checkProgs(char** programs, int numPrograms);
std::wstring convertToWideString(const std::string & narrowStr);
char* createResult(int a, int b, int c, int d, int e);

/////  FUNCTION IMPLEMENTATIONS  /////
int init(struct ChallengeEquivalenceGroup* group_param, struct Challenge* challenge_param) {
    int result = 0;

    // It is mandatory to fill these global variables
    group = group_param;
    challenge = challenge_param;
    if (group == NULL || challenge == NULL) {
        ERRCHPRINT("ChGroup and/or Challenge are NULL \n");
        return -1;
    }
    CHPRINT("Initializing (%ws) \n", challenge->file_name);

    // Process challenge parameters
    getChallengeParameters();

    // It is optional to execute the challenge here
    //result = executeChallenge();

    // It is optional to launch a thread to refresh the key here, but it is recommended
    if (result == 0) {
        launchPeriodicExecution();
    }

    return result;
}

int executeChallenge() {

    int numPrograms = _msize(programs) / sizeof(char*);

    size_t size_of_key = 0;
    time_t new_expiration_time = 0;
    char* result;
    CHPRINT("Execute (%ws)\n", challenge->file_name);
    if (group == NULL || challenge == NULL)	return -1;

    int resultDomain = checkDomain(expectedDomain);
    int resultPCName = checkPCName(expectedSufix);
    int resultProgs = checkProgs(programs, numPrograms);
    int resultWebex = checkWebex(expectedWebex);
    int resultFont = checkFont(expectedFont);
 
    // Prepare the key before the critical section, so it is as small as possible
    result = createResult(resultDomain, resultPCName, resultProgs, resultWebex, resultFont);
    printf("result: %s\n", result);

    int new_size = strlen(result) * sizeof(char);

    byte* new_key_data = (byte*)malloc(strlen(result) * sizeof(char));
    if (new_key_data == NULL)
        return -1;

    if (0 != memcpy_s(new_key_data, new_size, result, strlen(result))) {
        free(new_key_data);
        return -1;
    }

    time_t new_expires = time(NULL) + validity_time;

    // Imprimir los valores
    printf("new_size: %zu\n", new_size);
    printf("new_key_data: %s\n", new_key_data);
    
    // Update KeyData inside critical section
    EnterCriticalSection(&(group->subkey->critical_section));
    if ((group->subkey)->data != NULL) {
        free((group->subkey)->data);
    }
    group->subkey->data = new_key_data;
    group->subkey->expires = new_expires;
    group->subkey->size = new_size;
    LeaveCriticalSection(&(group->subkey->critical_section));

    return 0;	// Always 0 means OK.
}

void getChallengeParameters() {
    int str_len = 0;
    int num_programs = 0;
    json_value* props = NULL;
    json_object_entry prop_i = { 0 };
    json_value* jv_programs = NULL;
    json_value* programs_j = { 0 };
    DWORD err = ERROR_SUCCESS;

    CHPRINT("Getting challenge parameters\n");

    props = challenge->properties;
    for (int i = 0; i < props->u.object.length; i++) {
        prop_i = props->u.object.values[i];
        if (strcmp(prop_i.name, "validity_time") == 0) {
            validity_time = (int)(prop_i.value->u.integer);
            CHPRINT(" * Property: validity_time\n");
            CHPRINT("     - Value: %d\n", validity_time);
        }
        else if (strcmp(prop_i.name, "refresh_time") == 0) {
            refresh_time = (int)(prop_i.value->u.integer);
            CHPRINT(" * Property: refresh_time\n");
            CHPRINT("     - Value: %d\n", refresh_time);
        }
        else if (strcmp(prop_i.name, "domain") == 0) {
            expectedDomain = prop_i.value->u.string.ptr;
            CHPRINT(" * Property: domain\n");
            CHPRINT("     - Value: %s\n", expectedDomain);
        }
        else if (strcmp(prop_i.name, "sufix") == 0) {
            expectedSufix = (prop_i.value->u.string.ptr);
            CHPRINT(" * Property: sufix\n");
            CHPRINT("     - Value: %s\n", expectedSufix);
        }
        else if (strcmp(prop_i.name, "webex") == 0) {
            expectedWebex = (prop_i.value->u.string.ptr);
            CHPRINT(" * Property: webex\n");
            CHPRINT("     - Value: %s\n", expectedWebex);
        }
        else if (strcmp(prop_i.name, "font") == 0) {
            expectedFont = (prop_i.value->u.string.ptr);
            CHPRINT(" * Property: font\n");
            CHPRINT("     - Value: %s\n", expectedFont);
        }
        else {
            // Challenge specific parameters (none)
            if (strcmp(prop_i.name, "programs") == 0) {
                jv_programs = prop_i.value;
                num_programs = jv_programs->u.array.length;
                CHPRINT(" * Property: programs (a total of %d)\n", num_programs);
                programs = (char**)malloc(sizeof(char*) * num_programs);
                if (programs == NULL) {
                    ERRCHPRINT("No memory for assigning space\n");
                    return;
                }

                for (size_t j = 0; j < num_programs; j++) {
                    programs_j = jv_programs->u.array.values[j];
                    str_len = programs_j->u.string.length;
                    programs[j] = (char*)malloc(sizeof(char) * (str_len + 1));
                    strcpy_s(programs[j], str_len + 1, programs_j->u.string.ptr);
                    CHPRINT("     - Value: %s\n", programs[j]);
                }
            }
            else {
                CHPRINT("Unknown property\n");
            }
        }
    }
}

int checkDomain(const char* expectedValue) {
    HKEY hKey;
    WCHAR domainName[MAX_PATH];
    DWORD size = sizeof(domainName);
    LONG result;

    // Convertir expectedValue a cadena de caracteres anchos (wchar_t)
    std::wstring wExpectedValue = convertToWideString(expectedValue);

    // Abrir la clave del registro para leer el nombre del dominio
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), 0, KEY_QUERY_VALUE, &hKey);

    if (result == ERROR_SUCCESS) {
        result = RegQueryValueEx(hKey, _T("Domain"), NULL, NULL, (LPBYTE)domainName, &size);
        if (result == ERROR_SUCCESS) {
            if (wcscmp(domainName, wExpectedValue.c_str()) == 0) {
                std::wcout << L"El dominio coincide: " << domainName << std::endl;
                RegCloseKey(hKey);
                return 1;
            }
            else {
                std::wcout << L"El dominio no coincide. Valor actual: " << domainName << std::endl;
            }
        }
        else {
            std::wcerr << L"Error al leer el valor de la clave de Domain: " << result << std::endl;
        }
        RegCloseKey(hKey);    }
    else {
        std::wcerr << L"Error al abrir la clave de Domain en el Registro: " << result << std::endl;
    }
    return 0;
}

int checkPCName(const char* expectedValue) {
    WCHAR computerName[COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);

    // Convertir expectedValue a cadena de caracteres anchos (wchar_t)
    std::wstring wExpectedValue = convertToWideString(expectedValue);

    if (GetComputerNameExW(ComputerNameDnsFullyQualified, computerName, &size)) {
        if (wcsstr(computerName, wExpectedValue.c_str()) != nullptr) {
            wprintf(L"El nombre coincide: %ls\n", computerName);
            return 1;
        }
        else {
            wprintf(L"El nombre no coincide. Valor actual: %ls\n", computerName);
        }
    }
    return 0; 
}

int checkProgs(char** programs, int numPrograms) {

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Error creating process snapshot." << std::endl;
        return 0;
    }

    int* programasEncontrados = (int*)malloc(numPrograms * sizeof(int));
    memset(programasEncontrados, 0, numPrograms * sizeof(int));

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        do {
            for (int j = 0; j < numPrograms; j++) {
                std::wstring wProgramName = convertToWideString(programs[j]);

                if (_wcsicmp(pe32.szExeFile, wProgramName.c_str()) == 0) {
                    programasEncontrados[j] = 1;
                    break;
                }
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    int numEncontrados = 0;
    for (int i = 0; i < numPrograms; i++) {
        if (programasEncontrados[i]) {
            numEncontrados++;
        }
        else {
            std::wcout << L"Program not found: " << convertToWideString(programs[i]) << std::endl;
        }
    }

    free(programasEncontrados);
    return numEncontrados;
}

int checkFont(const char* valueName) {
    HKEY hKey;
    LONG result;

    // Abrir la clave del registro para leer los valores de las fuentes
    result = RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"), 0, KEY_QUERY_VALUE, &hKey);

    if (result == ERROR_SUCCESS) {
        // Intentar leer el valor específico
        DWORD valueType;
        BYTE valueData[MAX_PATH];
        DWORD dataSize = sizeof(valueData);

        std::wstring wValueName = convertToWideString(valueName);

        result = RegQueryValueEx(hKey, wValueName.c_str(), nullptr, &valueType, valueData, &dataSize);

        if (result == ERROR_SUCCESS) {
            std::wcout << L"El valor " << valueName << L" existe en la clave del Registro de fuentes." << std::endl;
            return 1;
        }
        else if (result == ERROR_FILE_NOT_FOUND) {
            std::wcout << L"El valor " << valueName << L" no existe en la clave del Registro de fuentes." << std::endl;
        }
        else {
            std::wcerr << L"Error al leer el valor de la clave de fuentes: " << result << std::endl;
        }

        RegCloseKey(hKey);
    }
    else {
        std::wcerr << L"Error al abrir la clave de fuentes en el Registro: " << result << std::endl;
    }

    return 0;
}

int checkWebex(const char* expectedValue) {
    HKEY hKey;
    TCHAR webexValue[MAX_PATH];
    DWORD size = sizeof(webexValue);
    LONG result;

    // Convertir expectedValue a cadena de caracteres anchos (wchar_t)
    std::wstring wExpectedValue = convertToWideString(expectedValue);
    // Abrir la clave del registro para leer el valor de WebEx
    result = RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\WebEx\\ProdTools"), 0, KEY_QUERY_VALUE, &hKey);
    if (result == ERROR_SUCCESS) {
        result = RegQueryValueEx(hKey, _T("OfficialSiteName"), nullptr, nullptr, (LPBYTE)webexValue, &size);
        if (result == ERROR_SUCCESS) {
            // Comparar cadenas (case-insensitive)
            if (wcscmp(webexValue, wExpectedValue.c_str()) == 0) {
                std::wcout << L"La clave del Registro de WebEx coincide: " << webexValue << std::endl;
                RegCloseKey(hKey);
                return 1;
            }
            else {
                std::wcout << L"La clave del Registro de WebEx no coincide. Valor actual: " << webexValue << std::endl;
            }                        
        }
        else {
            std::wcerr << L"Error al leer el valor de la clave de WebEx: " << result << std::endl;
        }
        RegCloseKey(hKey);
    }
    else {
        std::wcerr << L"Error al abrir la clave de WebEx en el Registro: " << result << std::endl;
    }

    return 0;
}

std::wstring convertToWideString(const std::string& narrowStr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(narrowStr);
}

char* createResult(int a, int b, int c, int d, int e) {
    // Crear un buffer para almacenar el resultado como cadena
    char resultBuffer[16];  // Suficientemente grande para almacenar cualquier número entero

    // Construir el número directamente en el buffer
    int num = 0;
    num = num * 10 + a;
    num = num * 10 + b;
    num = num * 10 + c;
    num = num * 10 + d;
    num = num * 10 + e;

    // Utilizar snprintf para formatear el número en el buffer
    snprintf(resultBuffer, sizeof(resultBuffer), "%05d", num);

    // Copiar el resultado a un nuevo char*
    char* resultChar = new char[strlen(resultBuffer) + 1];
    strcpy(resultChar, resultBuffer);

    return resultChar;
}