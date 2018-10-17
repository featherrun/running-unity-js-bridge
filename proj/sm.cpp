// sm.cpp

#include "stdafx.h"
#include "sm.h"
#include "js/Initialization.h"
#include <list>

/*
#include <iostream>
#include <fstream>
void LOG(const char* s) {
	std::ofstream outfile;
	outfile.open("log.txt", std::ios::app);
	outfile << s << std::endl;
	outfile.close();
}
*/

struct class_SM {
	OnImportScripts onImportScripts;
	OnAlert onAlert;
	OnInvoke onInvoke;
	OnError onError;

	JSContext *cx;
	JSObject *global;
	JSCompartment *compartment;

	JS::CallArgs invokeArgs; //javascript::invoke()
	std::list<void *> rubbish; //垃圾桶
} self;

inline void freeAll() {
	if (self.rubbish.empty()) return;
	for (auto iter = self.rubbish.begin(); iter != self.rubbish.end(); iter++) {
		JS_free(self.cx, *iter);
	}
	self.rubbish.clear();
}

//-----------------------------------------------------

static const JSClassOps global_ops = {
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr,
	nullptr,
	nullptr, nullptr, nullptr, JS_GlobalObjectTraceHook
};

/* The class of the global object. */
static const JSClass global_class = {
	"global",
	JSCLASS_GLOBAL_FLAGS,
	&global_ops
};

//-----------------------------------------------------

// Call C functions from JavaScript
static bool importScripts(JSContext *cx, unsigned argc, JS::Value *vp) { //导入脚本
	if (self.onImportScripts) {
		JS::CallArgs args = CallArgsFromVp(argc, vp);
		JS::HandleValue hv = args.get(0);
		if (hv.isString()) {
			char *s = JS_EncodeString(cx, hv.toString());
			bool ok = self.onImportScripts(s);
			JS_free(self.cx, s);
			return ok;
		}
		JS_ReportWarningUTF8(cx, "[importScripts] Parameter error");
	} else {
		JS_ReportWarningUTF8(cx, "[importScripts] Undefined");
	}
	return false;
}

static bool alert(JSContext *cx, unsigned argc, JS::Value *vp) { //打印调试信息
	if (self.onAlert) {
		JS::CallArgs args = CallArgsFromVp(argc, vp);
		JS::HandleValue hv = args.get(0);
		if (hv.isString()) {
			//size_t length;
			//JS::AutoCheckCannotGC nogc;
			//const char16_t *s = JS_GetTwoByteStringCharsAndLength(self.cx, nogc, hv.toString(), &length);
			//return self.onAlert(s, length);
			JS::RootedString rs(cx, hv.toString());
			char *s = JS_EncodeStringToUTF8(self.cx, rs);
			bool ok = self.onAlert(s, strlen(s));
			JS_free(self.cx, s);
			return ok;
		}
		JS_ReportWarningUTF8(cx, "[alert] Parameter error");
	} else {
		JS_ReportWarningUTF8(cx, "[alert] Undefined");
	}
	return false;
}

static bool invoke(JSContext *cx, unsigned argc, JS::Value *vp) { //反射
	if (self.onInvoke) {
		JS::CallArgs args = CallArgsFromVp(argc, vp);
		JS::HandleValue hv1 = args.get(0);
		JS::HandleValue hv2 = args.get(1);

		int clazz, func;

		if (hv1.isInt32()) {
			clazz = hv1.toInt32();
		} else if (hv1.isNumber()) {
			clazz = (int)hv1.toNumber();
		} else {
			JS_ReportWarningUTF8(cx, "[invoke] Parameter 1 type error");
			return false;
		}
		if (hv2.isInt32()) {
			func = hv2.toInt32();
		} else if (hv2.isNumber()) {
			func = (int)hv2.toNumber();
		} else {
			JS_ReportWarningUTF8(cx, "[invoke] Parameter 2 type error");
			return false;
		}

		self.invokeArgs = args;
		bool ok = self.onInvoke(clazz, func, argc - 2);
		freeAll();
		return ok;

	} else {
		JS_ReportWarningUTF8(cx, "[invoke] Undefined");
	}
	return false;
}

/*
static const JSFunctionSpec globalFunctions[] = {
	JS_FN("importScripts", importScripts, 1, JSPROP_PERMANENT),
	JS_FN("alert", alert, 1, JSPROP_PERMANENT),
	JS_FN("invoke", invoke, 2, JSPROP_PERMANENT),
	JS_FS_END
};
*/

//-----------------------------------------------------

void reportError(JSContext *cx, JSErrorReport *report) {
	const char* filename = report->filename ? report->filename : "[no filename]";
	unsigned int lineno = report->lineno;
	const char* msg = report->message().c_str();
	if (self.onError) {
		self.onError(filename, lineno, msg);
	}
	if (JS_IsExceptionPending(cx)) {
		JS_ClearPendingException(cx);
	}
}

void handlePendingException(JSContext *cx) {
	JS::RootedValue err(cx);
	if (JS_GetPendingException(cx, &err) && err.isObject()) {
		JS_ClearPendingException(cx);
		JS::RootedObject errObj(cx, err.toObjectOrNull());
		JSErrorReport *report = JS_ErrorFromException(cx, errObj);
		reportError(cx, report);
	}
}

//-----------------------------------------------------

bool _Inited;
bool startupEngine(OnImportScripts onImportScripts, OnAlert onAlert, OnInvoke onInvoke, OnError onError) {
	shutdownEngine();
	_Inited = _Inited || JS_Init();

	// Create a context.
	JSContext *cx = JS_NewContext(JS::DefaultHeapMaxBytes);
	if (!cx)
		return false;

	self.onImportScripts = onImportScripts;
	self.onAlert = onAlert;
	self.onInvoke = onInvoke;
	self.onError = onError;
	self.cx = cx;
	
	JS_SetGCParameter(cx, JSGC_MAX_BYTES, 0xffffffff);
	JS_SetGCParameter(cx, JSGC_MODE, JSGC_MODE_INCREMENTAL); 
	JS_SetNativeStackQuota(cx, 500000);
	JS_SetFutexCanWait(cx);
	JS_SetDefaultLocale(cx, "UTF-8");
	JS::SetWarningReporter(cx, reportError);
	
	if (!JS::InitSelfHostedCode(cx))
		return false;

	//JSAutoRequest ar(cx);
	JS_BeginRequest(cx);

	JS::CompartmentOptions options;
	options.behaviors().setVersion(JSVERSION_LATEST);
	options.creationOptions().setSharedMemoryAndAtomicsEnabled(true);

	JS::ContextOptionsRef(cx)
		.setIon(true)
		.setBaseline(true)
		.setAsmJS(true)
		.setNativeRegExp(true);

	JS::RootedObject g(cx, JS_NewGlobalObject(cx, &global_class, nullptr, JS::DontFireOnNewGlobalHook, options));
	JS::PersistentRootedObject global(cx, g);

	//JSAutoCompartment ac(cx, global);
	self.compartment = JS_EnterCompartment(cx, global);
	self.global = global.get();

	JS_InitStandardClasses(cx, global);
	JS_DefineDebuggerObject(cx, global);
	JS_DefineFunction(cx, global, "importScripts", importScripts, 1, JSPROP_PERMANENT);
	JS_DefineFunction(cx, global, "alert", alert, 1, JSPROP_PERMANENT);
	JS_DefineFunction(cx, global, "invoke", invoke, 1, JSPROP_PERMANENT);

	JS_FireOnNewGlobalObject(cx, global);

	return true;
}

bool shutdownEngine() {
	if (!_Inited || !self.cx)
		return false;
	freeAll();
	JS_GC(self.cx);
	JS_LeaveCompartment(self.cx, self.compartment);
	JS_EndRequest(self.cx);
	JS_DestroyContext(self.cx);
	JS_ShutDown();
	self.cx = nullptr;
	self.global = nullptr;
	self.compartment = nullptr;
	_Inited = false;
	return true;
}

bool evaluate(const char *script, const char *filename) { //加载JS脚本
	JSContext *cx = self.cx;
	//JS::RootedObject global(cx, self.global);
	//JSAutoCompartment ac(cx, global);
	JS::CompileOptions options(cx);
	options.setVersion(JSVERSION_LATEST);
	options.setUTF8(true);
	options.setFileAndLine(filename, 1);
	JS::PersistentRootedScript rscript(cx);
	bool ok = JS::Compile(cx, options, script, strlen(script), &rscript);
	if (ok) {
		JS::RootedValue rval(cx);
		ok = JS_ExecuteScript(cx, rscript, &rval);
		if (!ok) {
			handlePendingException(cx);
		}
	}
	return ok;
}

//-----------------------------------------------------

bool readBoolean(int i) { //读取invoke参数
	const JS::CallArgs args = self.invokeArgs;
	JS::HandleValue v = args.get(i + 2);
	return v.isBoolean() ? v.toBoolean() : false;
}

int readInt(int i) {
	const JS::CallArgs args = self.invokeArgs;
	JS::HandleValue v = args.get(i + 2);
	return v.isInt32() ? v.toInt32() : 0;
}

double readDouble(int i) {
	const JS::CallArgs args = self.invokeArgs;
	JS::HandleValue v = args.get(i + 2);
	return v.isNumber() ? v.toNumber() : 0.0;
}

Words readString(int i) {
	const JS::CallArgs args = self.invokeArgs;
	JS::HandleValue v = args.get(i + 2);
	Words w;
	if (v.isString()) {
		//JS::AutoCheckCannotGC nogc;
		//w.s = JS_GetTwoByteStringCharsAndLength(self.cx, nogc, v.toString(), &w.length);
		JS::RootedString rs(self.cx, v.toString());
		w.s = JS_EncodeStringToUTF8(self.cx, rs);
		w.length = strlen(w.s);
		self.rubbish.push_back(w.s);
	} else {
		w.s = nullptr;
		w.length = 0;
	}
	return w;
}

//-----------------------------------------------------

void returnBoolean(bool b) { //invoke返回值
	const JS::CallArgs args = self.invokeArgs;
	args.rval().setBoolean(b);
}

void returnInt(int i) {
	const JS::CallArgs args = self.invokeArgs;
	args.rval().setInt32(i);
}

void returnIntArray(const int *iArr, size_t length) {
	const JS::CallArgs args = self.invokeArgs;
	if (length <= 0) {
		args.rval().setUndefined();
	} else {
		JSObject *o = JS_NewArrayObject(self.cx, length);
		JS::RootedObject ro(self.cx, o);
		for (size_t aN = 0; aN < length; aN++) {
			JS_SetElement(self.cx, ro, aN, iArr[aN]);
		}
		args.rval().setObject(*o);
	}
}

void returnDouble(double d) {
	const JS::CallArgs args = self.invokeArgs;
	args.rval().setDouble(d);
}

void returnDoubleArray(const double *dArr, size_t length) {
	const JS::CallArgs args = self.invokeArgs;
	if (length <= 0) {
		args.rval().setUndefined();
	} else {
		JSObject *o = JS_NewArrayObject(self.cx, length);
		JS::RootedObject ro(self.cx, o);
		for (size_t aN = 0; aN < length; aN++) {
			JS_SetElement(self.cx, ro, aN, dArr[aN]);
		}
		args.rval().setObject(*o);
	}
}

void returnString(const char16_t* s) {
	const JS::CallArgs args = self.invokeArgs;
	if (s == nullptr) {
		args.rval().setUndefined();
	} else {
		args.rval().setString(JS_NewUCStringCopyZ(self.cx, s));
	}
}

//-----------------------------------------------------

bool callByName(const char* func) {
	JS::RootedObject global(self.cx, self.global);
	//JSAutoCompartment ac(self.cx, global);
	JS::RootedValue rval(self.cx);
	JS::RootedValue funcVal(self.cx);
	bool ok = false;
	if (JS_GetProperty(self.cx, global, func, &funcVal)) {
		if (!funcVal.isNullOrUndefined()) {
			ok = JS_CallFunctionValue(self.cx, global, funcVal, JS::HandleValueArray::empty(), &rval);
			if (JS_IsExceptionPending(self.cx)) {
				handlePendingException(self.cx);
			}
		}
	}
	return ok;
}

bool onload(JS::HandleValueArray args) {
	JS::RootedObject global(self.cx, self.global);
	//JSAutoCompartment ac(self.cx, global);
	JS::RootedValue rval(self.cx);
	JS::RootedValue funcVal(self.cx);
	bool ok = false;
	if (JS_GetProperty(self.cx, global, "onload", &funcVal)) {
		if (!funcVal.isNullOrUndefined()) {
			ok = JS_CallFunctionValue(self.cx, global, funcVal, args, &rval);
			if (JS_IsExceptionPending(self.cx)) {
				handlePendingException(self.cx);
			}
		}
	}
	return ok;
}

bool load(int obj, int func) {
	JS::AutoValueArray<2> argv(self.cx);
	argv[0].setInt32(obj);
	argv[1].setInt32(func);
	return onload(argv);
}

bool loadBoolean(int obj, int func, bool b) {
	JS::AutoValueArray<3> argv(self.cx);
	argv[0].setInt32(obj);
	argv[1].setInt32(func);
	argv[2].setBoolean(b);
	return onload(argv);
}

bool loadInt(int obj, int func, int i) {
	JS::AutoValueArray<3> argv(self.cx);
	argv[0].setInt32(obj);
	argv[1].setInt32(func);
	argv[2].setInt32(i);
	return onload(argv);
}

bool loadDouble(int obj, int func, double d) {
	JS::AutoValueArray<3> argv(self.cx);
	argv[0].setInt32(obj);
	argv[1].setInt32(func);
	argv[2].setDouble(d);
	return onload(argv);
}

bool loadString(int obj, int func, const char16_t* s) {
	JS::RootedString rs(self.cx, JS_NewUCStringCopyZ(self.cx, s));
	JS::AutoValueArray<3> argv(self.cx);
	argv[0].setInt32(obj);
	argv[1].setInt32(func);
	argv[2].setString(rs);
	return onload(argv);
}



#ifdef _DEBUG
bool test__OnImportScripts(char *s) {
	printf("import:%s\n", s);
	return true;
}
bool test__OnAlert(char *s, size_t length) {
	printf("alert:%s\n", s);
	return true;
}
bool test__OnInvoke(int obj, int func, int argc) {
	printf("invoke:%d-%d\n", obj, func);
	readString(0);
	//returnInt(12345);
	//returnString(u"invoke-result");
	const double d[] = { 1.1, 1.2, 1.3 };
	returnDoubleArray(d, 3);
	return true;
}
bool test__OnError(const char *filename, unsigned int lineno, const char *message) {
	printf("error(%s-%d):%s\n", filename, lineno, message);
	return true;
}
int main() {
	startupEngine(&test__OnImportScripts, &test__OnAlert, &test__OnInvoke, &test__OnError);
	evaluate("function onload(obj,func,a){ for(var i=0;i<10;i++) alert('x'+invoke(0,0,a+'b')); importScripts('some'); foo.error(); } function boot(){alert('booted!');}", "test");
	callByName("boot");
	loadInt(0, 0, 100);
	loadInt(0, 0, 100);
	system("pause");
	startupEngine(&test__OnImportScripts, &test__OnAlert, &test__OnInvoke, &test__OnError);
	evaluate("function boot(){alert('2-booted!');}", "test");
	callByName("boot");
	shutdownEngine();
	system("pause");
	return 0;
}
#endif