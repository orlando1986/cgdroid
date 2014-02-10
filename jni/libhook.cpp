/*
 * libmynet.c
 *
 *  Created on: 2013-1-17
 *      Author: d
 */

#include <jni.h>
#include <dlfcn.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <android/log.h>
#include <AndroidRuntime.h>

#define HOOK_DEX   "hook.jar"
#define ODEX_PATH  "/data/system/"
#define HOOK_PATH  "/data/system/inject/"
#define ENABLE_DEBUG 1

#define LOG_TAG "inject"
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##args)

extern "C" {

char * str_contact(const char *str1,const char *str2) {
     char * result;
     result = (char*)malloc(strlen(str1) + strlen(str2) + 1); //str1的长度 + str2的长度 + \0;
     if(!result){ //如果内存动态分配失败
        LOGD("Error: malloc failed in concat! \n");
        exit(EXIT_FAILURE);
     }
     strcpy(result,str1);
     strcat(result,str2); //字符串拼接
    return result;
}

int invoke_dex_method(const char* dexPath, const char* dexOptDir,
		const char* className, const char* methodName, int argc, char *argv[]) {
	LOGD("Invoke dex E");
	JNIEnv * env = android::AndroidRuntime::getJNIEnv();
	jclass stringClass, classLoaderClass, dexClassLoaderClass, targetClass;
	jmethodID getSystemClassLoaderMethod, dexClassLoaderContructor,
			loadClassMethod, targetMethod;
	jobject systemClassLoaderObject, dexClassLoaderObject;
	jstring dexPathString, dexOptDirString, classNameString, tmpString;
	jobjectArray stringArray;

	/* Get SystemClasLoader */
	stringClass = (jclass) env->NewGlobalRef(env->FindClass("java/lang/String"));
	classLoaderClass = (jclass) env->NewGlobalRef(env->FindClass("java/lang/ClassLoader"));
	dexClassLoaderClass = (jclass) env->NewGlobalRef(env->FindClass("dalvik/system/DexClassLoader"));
	getSystemClassLoaderMethod = env->GetStaticMethodID(classLoaderClass,
			"getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	systemClassLoaderObject = env->CallStaticObjectMethod(classLoaderClass,
			getSystemClassLoaderMethod);

	/* Create DexClassLoader */
	dexClassLoaderContructor =
			env->GetMethodID(dexClassLoaderClass, "<init>",
					"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
	dexPathString = env->NewStringUTF(dexPath);
	dexOptDirString = env->NewStringUTF(dexOptDir);
	dexClassLoaderObject = env->NewObject(dexClassLoaderClass,
			dexClassLoaderContructor, dexPathString, dexOptDirString, NULL,
			systemClassLoaderObject);

	/* Use DexClassLoader to load target class */
	loadClassMethod = env->GetMethodID(dexClassLoaderClass, "loadClass",
			"(Ljava/lang/String;)Ljava/lang/Class;");
	classNameString = env->NewStringUTF(className);
	LOGD("step 1");
	targetClass = (jclass) env->CallObjectMethod(dexClassLoaderObject,
			loadClassMethod, classNameString);
	LOGD("step 2");
	if (!targetClass) {
		LOGE("Failed to load target class %s", className);
		return -1;
	}
	LOGD("step 3");
	/* Invoke target method */
	targetMethod = env->GetStaticMethodID(targetClass, methodName,
			"([Ljava/lang/String;)V");
	if (!targetMethod) {
		LOGE("Failed to load target method %s", methodName);
		return -1;
	}
	LOGD("step 4");
	stringArray = env->NewObjectArray(argc, stringClass, NULL);
	for (int i = 0; i < argc; i++) {
		tmpString = env->NewStringUTF(argv[i]);
		env->SetObjectArrayElement(stringArray, i, tmpString);
	}
	LOGD("step 5");
	env->CallStaticVoidMethod(targetClass, targetMethod, stringArray);
	env->DeleteGlobalRef(stringClass);
	env->DeleteGlobalRef(classLoaderClass);
	env->DeleteGlobalRef(dexClassLoaderClass);
	LOGD("Invoke dex X");
	return 0;
}

int main(int argc, char *argv[]) {
	LOGD("loading dex begin");
	invoke_dex_method(str_contact(HOOK_PATH, HOOK_DEX), ODEX_PATH, "com.android.inject.Hooker", "main", 0, NULL);
	LOGD("loading dex end");
    return -1;
}
}
