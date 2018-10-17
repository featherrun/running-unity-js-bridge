#pragma once

#ifdef _EXPORT
#define API_DECLSPEC extern "C" //_declspec(dllexport)
#else
#define API_DECLSPEC extern "C" //_declspec(dllimport)
#endif

#include "jsapi.h"
#include "jsfriendapi.h"

struct Words
{
	char* s;
	size_t length;
};

// Delegate
typedef bool(*OnImportScripts)(char *s);
typedef bool(*OnAlert)(char *s, size_t length);
typedef bool(*OnInvoke)(int obj, int func, int argc);
typedef bool(*OnError)(const char *filename, unsigned int lineno, const char *message);

// Function
API_DECLSPEC bool startupEngine(OnImportScripts onImportScripts, OnAlert onAlert, OnInvoke onInvoke, OnError onError);
API_DECLSPEC bool shutdownEngine();
API_DECLSPEC bool evaluate(const char *script, const char *filename);

// Get Invoke Args
API_DECLSPEC bool readBoolean(int i);
API_DECLSPEC int readInt(int i);
API_DECLSPEC double readDouble(int i);
API_DECLSPEC Words readString(int i);

// Set Invoke Rval
API_DECLSPEC void returnBoolean(bool b);
API_DECLSPEC void returnInt(int i);
API_DECLSPEC void returnIntArray(const int *iArr, size_t length);
API_DECLSPEC void returnDouble(double d);
API_DECLSPEC void returnDoubleArray(const double *dArr, size_t length);
API_DECLSPEC void returnString(const char16_t* s);

// Call JS
API_DECLSPEC bool callByName(const char* func);
API_DECLSPEC bool load(int obj, int func);
API_DECLSPEC bool loadBoolean(int obj, int func, bool b);
API_DECLSPEC bool loadInt(int obj, int func, int i);
API_DECLSPEC bool loadDouble(int obj, int func, double d);
API_DECLSPEC bool loadString(int obj, int func, const char16_t* s);

