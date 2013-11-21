#pragma once
#include <stdint.h>
#include <jni.h>
#if 0 // ANDROID
#include <android/log.h>
#define JNI_TRACE(...) __android_log_print(ANDROID_LOG_VERBOSE, "JNIBridge", __VA_ARGS__)
#else
#define JNI_TRACE(...) do {} while (false)
#endif

typedef void* jvoid; // make it possible to return void

namespace jni
{

enum Errno
{
	kJNI_NO_ERROR = 0,
	kJNI_ATTACH_FAILED,
	kJNI_INVALID_PARAMETERS,
	kJNI_EXCEPTION_THROWN
};

// --------------------------------------------------------------------------------------
// Initialization and error functions
// --------------------------------------------------------------------------------------
void        Initialize(JavaVM& vm);

Errno       CheckError();
Errno       PeekError();
const char* GetErrorMessage();

jthrowable  ExceptionThrown(jclass clazz = 0);

// Internalish
bool        CheckForParameterError(bool valid);
bool        CheckForExceptionError(JNIEnv* env);

// --------------------------------------------------------------------------------------
// Oracle JNI functions (a selection of)
// http://docs.oracle.com/javase/6/docs/technotes/guides/jni/spec/functions.html#wp9502
// --------------------------------------------------------------------------------------

JavaVM*      GetJavaVM();
JNIEnv*      GetEnv();
JNIEnv*      AttachCurrentThread();
void         DetachCurrentThread();

jclass       FindClass(const char* name);

jint         Throw(jthrowable object);
jint         ThrowNew(jclass exception, const char* message);
void         FatalError(const char* str);

jobject      NewGlobalRef(jobject obj);
void         DeleteGlobalRef(jobject obj);

jclass       GetObjectClass(jobject object);
jboolean     IsInstanceOf(jobject object, jclass clazz);

jmethodID    GetMethodID(jclass clazz, const char* name, const char* signature);
jfieldID     GetFieldID(jclass clazz, const char* name, const char* signature);
jmethodID    GetStaticMethodID(jclass clazz, const char* name, const char* signature);
jfieldID     GetStaticFieldID(jclass clazz, const char* name, const char* signature);

jobject		 NewObject(jclass clazz, jmethodID methodID, ...);

jstring      NewStringUTF(const char* str);
jsize        GetStringUTFLength(jstring string);
const char*  GetStringUTFChars(jstring str, jboolean* isCopy = 0);
void         ReleaseStringUTFChars(jstring str, const char* utfchars);

size_t       GetArrayLength(jarray obj);
jobject      GetObjectArrayElement(jobjectArray obj, size_t index);
void         SetObjectArrayElement(jobjectArray obj, size_t index, jobject val);
jobjectArray NewObjectArray(jsize length, jclass elementClass, jobject initialElement = 0);

class ThreadScope
{
public:
	ThreadScope();
	~ThreadScope();

private:
	bool m_NeedDetach;
};

class LocalFrame
{
public:
	LocalFrame(jint capacity = 64);
	~LocalFrame();

private:
	LocalFrame(const LocalFrame& frame);
	LocalFrame& operator=(const LocalFrame& rhs);
	bool m_FramePushed;
};

// Some logic explanation
// Only invoke function if thread can be attached
// Only invoke function if 'parameters' == true
// If function is 'exception safe' only check for exceptions after function has been invoked
// Only adjust return value on exception if function is not exception safe
#define JNI_CALL_NO_RET(parameters, check_exception, function)                                          \
	JNI_TRACE("%d:%d:%s", static_cast<bool>(parameters), check_exception, #function);                   \
	JNIEnv* env(AttachCurrentThread());                                                                 \
	if (env && !CheckForParameterError(parameters) && !(check_exception && CheckForExceptionError(env)))\
	{                                                                                                   \
		function;                                                                                       \
	    CheckForExceptionError(env);                                                                    \
	}

#define JNI_CALL(rt, parameters, check_exception, function)                                             \
	JNI_TRACE("%d:%d:%s %s", static_cast<bool>(parameters), check_exception, #rt, #function);           \
	JNIEnv* env(AttachCurrentThread());                                                                 \
	if (env && !CheckForParameterError(parameters) && !(check_exception && CheckForExceptionError(env)))\
	{                                                                                                   \
		rt JNI_CALL_result = function;                                                                  \
		if (!(CheckForExceptionError(env) && check_exception))                                          \
			return JNI_CALL_result;                                                                     \
	}                                                                                                   \
	return 0

//----------------------------------------------------------------------------
// JNI Operations
// Heavily inspired by https://github.com/kohsuke/jnitl
//----------------------------------------------------------------------------
template <typename JT, typename RT,
	RT   (JNIEnv::* CallMethodOP)(jobject, jmethodID, va_list),
	RT   (JNIEnv::* CallNonvirtualMethodOP)(jobject, jclass, jmethodID, va_list),
	RT   (JNIEnv::* CallStaticMethodOP)(jclass, jmethodID, va_list),
	RT   (JNIEnv::* GetFieldOP)(jobject, jfieldID),
	void (JNIEnv::* SetFieldOP)(jobject, jfieldID, RT),
	RT   (JNIEnv::* GetStaticFieldOP)(jclass, jfieldID),
	void (JNIEnv::* SetStaticFieldOP)(jclass, jfieldID, RT)
>
class Basic_Op
{
public:
	static JT CallMethod(jobject object, jmethodID id, ...)
	{
		va_list args;
		va_start(args, id);
		JNI_CALL(JT, object && id, true, static_cast<JT>((env->*CallMethodOP)(object, id, args)));
		va_end(args);
	}
	static JT CallNonVirtualMethod(jobject object, jclass clazz, jmethodID id, ...)
	{
		va_list args;
		va_start(args, id);
		JNI_CALL(JT, object && clazz && id, true, static_cast<JT>((env->*CallNonvirtualMethodOP)(object, clazz, id, args)));
		va_end(args);
	}
	static JT CallStaticMethod(jclass clazz, jmethodID id, ...)
	{
		va_list args;
		va_start(args, id);
		JNI_CALL(JT, clazz && id, true, static_cast<JT>((env->*CallStaticMethodOP)(clazz, id, args)));
		va_end(args);
	}
	static JT GetField(jobject object, jfieldID id)
	{
		JNI_CALL(JT, object && id, true, static_cast<JT>((env->*GetFieldOP)(object, id)));
	}
	static void SetField(jobject object, jfieldID id, const RT& value)
	{
		JNI_CALL_NO_RET(object && id, true, (env->*SetFieldOP)(object, id, value));
	}
	static JT GetStaticField(jclass clazz, jfieldID id)
	{
		JNI_CALL(JT, clazz && id, true, static_cast<JT>((env->*GetStaticFieldOP)(clazz, id)));
	}
	static void SetStaticField(jclass clazz, jfieldID id, const RT& value)
	{
		JNI_CALL_NO_RET(clazz && id, true, (env->*SetStaticFieldOP)(clazz, id, value));
	}
};

template <typename RT, typename RAT,
	RT   (JNIEnv::* CallMethodOP)(jobject, jmethodID, va_list),
	RT   (JNIEnv::* CallNonVirualMethodOP)(jobject, jclass, jmethodID, va_list),
	RT   (JNIEnv::* CallStaticMethodOP)(jclass, jmethodID, va_list),
	RT   (JNIEnv::* GetFieldOP)(jobject, jfieldID),
	void (JNIEnv::* SetFieldOP)(jobject, jfieldID, RT),
	RT   (JNIEnv::* GetStaticFieldOP)(jclass, jfieldID),
	void (JNIEnv::* SetStaticFieldOP)(jclass, jfieldID, RT),
	RAT  (JNIEnv::* NewArrayOP)(jsize),
	RT*  (JNIEnv::* GetArrayElementsOP)(RAT, jboolean*),
	void (JNIEnv::* ReleaseArrayElementsOP)(RAT, RT*, jint),
	void (JNIEnv::* GetArrayRegionOP)(RAT, jsize, jsize, RT*),
	void (JNIEnv::* SetArrayRegionOP)(RAT, jsize, jsize, const RT*)
>
class Primitive_Op : public Basic_Op<RT, RT, CallMethodOP, CallNonVirualMethodOP, CallStaticMethodOP, GetFieldOP, SetFieldOP, GetStaticFieldOP, SetStaticFieldOP>
{
public:
	static RAT NewArray(jsize size)
	{
		JNI_CALL(RAT, true, true, static_cast<RAT>((env->*NewArrayOP)(size)));
	}
	static RT* GetArrayElements(RAT array, jboolean* isCopy = NULL)
	{
		JNI_CALL(RT*, array, true, static_cast<RT*>((env->*GetArrayElementsOP)(array, isCopy)));
	}
	static void ReleaseArrayElements(RAT array, RT* elements, jint mode = NULL)
	{
		JNI_CALL_NO_RET(array && elements, true, (env->*ReleaseArrayElementsOP)(array, elements, mode));
	}
	static void GetArrayRegion(RAT array, jsize start, jsize len, RT* buffer)
	{
		JNI_CALL_NO_RET(array && buffer, true, (env->*GetArrayRegionOP)(array, start, len, buffer));
	}
	static void SetArrayRegion(RAT array, jsize start, jsize len, RT* buffer)
	{
		JNI_CALL_NO_RET(array && buffer, true, (env->*SetArrayRegionOP)(array, start, len, buffer));
	}
};

#define JNITL_DEF_OP_LIST(t) \
	&JNIEnv::Call##t##MethodV, \
	&JNIEnv::CallNonvirtual##t##MethodV, \
	&JNIEnv::CallStatic##t##MethodV, \
	&JNIEnv::Get##t##Field, \
	&JNIEnv::Set##t##Field, \
	&JNIEnv::GetStatic##t##Field, \
	&JNIEnv::SetStatic##t##Field

#define JNITL_DEF_PRIMITIVE_OP_LIST(t) \
	JNITL_DEF_OP_LIST(t), \
	&JNIEnv::New##t##Array, \
	&JNIEnv::Get##t##ArrayElements, \
	&JNIEnv::Release##t##ArrayElements, \
	&JNIEnv::Get##t##ArrayRegion, \
	&JNIEnv::Set##t##ArrayRegion

#define JNITL_DEF_PRIMITIVE_OP(jt,t) \
	template <> \
	class Op<jt> : public Primitive_Op<jt,jt##Array, \
		JNITL_DEF_PRIMITIVE_OP_LIST(t) \
	> {};

// it defaults to jobject
template<typename T>
class Op : public Basic_Op<T, jobject, JNITL_DEF_OP_LIST(Object)> {};

// specialization for primitives
JNITL_DEF_PRIMITIVE_OP(jboolean,Boolean)
JNITL_DEF_PRIMITIVE_OP(jint,Int)
JNITL_DEF_PRIMITIVE_OP(jshort,Short)
JNITL_DEF_PRIMITIVE_OP(jbyte,Byte)
JNITL_DEF_PRIMITIVE_OP(jlong,Long)
JNITL_DEF_PRIMITIVE_OP(jfloat,Float)
JNITL_DEF_PRIMITIVE_OP(jdouble,Double)
JNITL_DEF_PRIMITIVE_OP(jchar,Char)

#undef JNITL_DEF_PRIMITIVE_OP
#undef JNITL_DEF_PRIMITIVE_OP_LIST
#undef JNITL_DEF_OP_LIST

// void requires a specialization.
template <>
class Op<jvoid>
{
public:
	static jvoid CallMethod(jobject object, jmethodID id, ...)
	{
		va_list args;
		va_start(args, id);
		JNI_CALL_NO_RET(object && id, true, env->CallVoidMethodV(object, id, args));
		va_end(args);
		return 0;
	}
	static jvoid CallStaticMethod(jclass clazz, jmethodID id, ...)
	{
		va_list args;
		va_start(args, id);
		JNI_CALL_NO_RET(clazz && id, true, env->CallStaticVoidMethodV(clazz, id, args));
		va_end(args);
		return 0;
	}
};

}
