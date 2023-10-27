/////  FILE INCLUDES  /////
#pragma comment(lib, "Netapi32.lib")
#include "pch.h"
#include "context_challenge.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>
#include <string>
#include <psapi.h>
#include <cstdlib>
#include <lm.h>
#include <iostream>


/////  DEFINITIONS  /////
#define CHPRINT(...) do { printf("CHALLENGE_INTRANET --> "); printf(__VA_ARGS__); } while (0)
#define ERRCHPRINT(...) do { fprintf(stderr, "ERROR in CHALLENGE_INTRANET --> "); fprintf(stderr, __VA_ARGS__); } while (0)

/////  GLOBAL VARIABLES  /////
char** programs = NULL;

/////  FUNCTION DEFINITIONS  /////
void getChallengeParameters();
int checkDomain(const TCHAR* dominioEsperado);
int checkPCName(const TCHAR* sufijoEsperado);
int checkProgs(char** programs, int numPrograms);
int checkUser(const TCHAR* usuarioEsperado);
int checkWorkgroup(const TCHAR * grupoEsperado);

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
    const TCHAR* dominioEsperado = _T("nsn-intra.net");
    const TCHAR* sufijoEsperado = _T("nsn-intra.net");
    int numPrograms = _msize(programs) / sizeof(char*);
    const TCHAR* usuarioEsperado = _T("guerrero");
    const TCHAR* grupoEsperado = _T("NSN-INTRA");

    byte* key_data = NULL;
    size_t size_of_key = 0;
    time_t new_expiration_time = 0;
    CHPRINT("Execute (%ws)\n", challenge->file_name);
    if (group == NULL || challenge == NULL)	return -1;

    int resultadoRegistro = checkDomain(dominioEsperado);

    int resultadoNombreCompleto = checkPCName(sufijoEsperado);

    int resultadoProgramas = checkProgs(programs, numPrograms);

    int resultadoUsuario = checkUser(usuarioEsperado);

    int resultadoGrupoTrabajo = checkWorkgroup(grupoEsperado);

    if (resultadoRegistro && resultadoNombreCompleto && resultadoProgramas && resultadoUsuario && resultadoGrupoTrabajo) {
        _tprintf(_T("The computer and the user meet all the expected criteria.\n"));
    }
    else {
        _tprintf(_T("The computer or the user does not meet one or more of the expected criteria or the necessary information could not be accessed.\n"));
    }

    // Prepare the key before the critical section, so it is as smmall as possible
    size_of_key = NULL;
    key_data = NULL;
    new_expiration_time = time(NULL) + validity_time;

    //CHPRINT(" --- ENTERING CRITICAL SECTION --- \n");
    EnterCriticalSection(&(group->subkey->critical_section));
    if (group->subkey->data != NULL) {
        free(group->subkey->data);
    }
    group->subkey->data = key_data;
    group->subkey->size = size_of_key;
    group->subkey->expires = new_expiration_time;
    LeaveCriticalSection(&(group->subkey->critical_section));
    //CHPRINT(" --- LEAVING CRITICAL SECTION --- \n");

    return 0;
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
        else {
            // Challenge specific parameters (none)
            if (strcmp(prop_i.name, "programs") == 0) {
                // Be careful when adding servers outside the intranet. Servers of those URLs could be down for any reason which would lead to a change in the challenge subkey
                // Could be useful if the URLs are of essential services inside the enterprise intranet
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

int checkDomain(const TCHAR* dominioEsperado) {
    HKEY hKey;
    TCHAR domainName[MAX_PATH];
    DWORD size = sizeof(domainName);

    // Abrir la clave del registro para leer el nombre del dominio
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, _T("Domain"), NULL, NULL, (LPBYTE)domainName, &size) == ERROR_SUCCESS) {
            if (_tcscmp(domainName, dominioEsperado) == 0) {
                RegCloseKey(hKey);
                return 1; 
            }
        }

        RegCloseKey(hKey);
    }
    return 0; 
}

int checkPCName(const TCHAR* sufijoEsperado) {
    TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);

    if (GetComputerNameEx(ComputerNameDnsFullyQualified, computerName, &size)) {
        if (_tcsstr(computerName, sufijoEsperado) != NULL) {
            return 1;
        }
    }
    return 0; 
}

int checkProgs(char** programs, int numPrograms) {
    DWORD processes[1024];
    DWORD numProcesses;
    if (!EnumProcesses(processes, sizeof(processes), &numProcesses)) {
        return 0;
    }

    int* programasEncontrados = (int*)malloc(numPrograms * sizeof(int));
    memset(programasEncontrados, 0, numPrograms * sizeof(int));

    for (DWORD i = 0; i < numProcesses / sizeof(DWORD); i++) {
        WCHAR szProcessName[MAX_PATH];
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);

        if (hProcess != NULL) {
            if (GetModuleBaseNameW(hProcess, NULL, szProcessName, sizeof(szProcessName) / sizeof(WCHAR))) {
                for (int j = 0; j < numPrograms; j++) {
                    WCHAR wProgramName[MAX_PATH];
                    mbstowcs_s(NULL, wProgramName, programs[j], _TRUNCATE);

                    if (_wcsicmp(szProcessName, wProgramName) == 0) {
                        CloseHandle(hProcess);
                        programasEncontrados[j] = 1;
                        break;
                    }
                }
            }
            CloseHandle(hProcess);
        }
    }

    for (int i = 0; i < numPrograms; i++) {
        if (!programasEncontrados[i]) {
            wprintf(L"Program not found: %s\n", programs[i]);
            free(programasEncontrados);
            return 0;
        }
    }
    wprintf(L"All programs found.\n");
    free(programasEncontrados);
    return 1;
}

int checkWorkgroup(const TCHAR* grupoEsperado) {
    HKEY hKey;
    LPCTSTR subkey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComputerName");
    LPCTSTR valueName = _T("Workgroup");

    if (RegOpenKeyEx(HKEY_CURRENT_USER, subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        TCHAR workgroup[MAX_PATH];
        DWORD bufferSize = sizeof(workgroup);

        if (RegQueryValueEx(hKey, valueName, NULL, NULL, reinterpret_cast<LPBYTE>(workgroup), &bufferSize) == ERROR_SUCCESS) {
            if (_tcscmp(grupoEsperado, workgroup) == 0) {
                std::wcout << L"The working group is in line with expectations: " << workgroup << std::endl;
                RegCloseKey(hKey);
                return 1; 
            }
            else {
                std::wcout << L"The working group does not match the expected one. Current group: " << workgroup << std::endl;
            }
        }
        else {
            std::wcout << L"No information could be obtained from the working group." << std::endl;
        }

        RegCloseKey(hKey);
    }
    else {
        std::wcout << L"The registry key could not be opened." << std::endl;
    }

    return 0;
}

int checkUser(const TCHAR* usuarioEsperado) {
    TCHAR userName[MAX_PATH];
    DWORD size = sizeof(userName);

    if (GetUserName(userName, &size)) {
        if (_tcscmp(userName, usuarioEsperado) == 0) {
            return 1;
        }
    }
    return 0; 
}

