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
#include <AndroidRuntime.h>
#include "utils.h"

extern "C" {

int invoke_dex_method(const char* dexPath,
		const char* className, const char* methodName, int argc, char *argv[]) {
	LOGD("Invoke dex E");
	JNIEnv * env = android::AndroidRuntime::getJNIEnv();
	jclass stringClass, classLoaderClass, dexClassLoaderClass, targetClass;
	jmethodID getSystemClassLoaderMethod, dexClassLoaderContructor,
			loadClassMethod, targetMethod, closeDexFile;
	jobject systemClassLoaderObject, dexClassLoaderObject;
	jstring dexPathString, dexOptDirString, classNameString, tmpString;
	jobjectArray stringArray;

	/* Get SystemClasLoader */
	stringClass = (jclass) env->NewGlobalRef(env->FindClass("java/lang/String"));
	classLoaderClass = (jclass) env->NewGlobalRef(env->FindClass("java/lang/ClassLoader"));
	dexClassLoaderClass = (jclass) env->NewGlobalRef(env->FindClass("dalvik/system/DexFile"));
	getSystemClassLoaderMethod = env->GetStaticMethodID(classLoaderClass,
			"getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	systemClassLoaderObject = env->CallStaticObjectMethod(classLoaderClass,
			getSystemClassLoaderMethod);

	/* Create DexClassLoader */
	dexClassLoaderContructor =
			env->GetMethodID(dexClassLoaderClass, "<init>",
					"(Ljava/lang/String;)V");
	dexPathString = (jstring) env->NewGlobalRef(env->NewStringUTF(dexPath));
	//dexOptDirString = (jstring) env->NewGlobalRef(env->NewStringUTF(dexOptDir));

	dexClassLoaderObject = env->NewObject(dexClassLoaderClass, dexClassLoaderContructor,
			dexPathString);

	/* Use DexClassLoader to load target class */
	loadClassMethod = env->GetMethodID(dexClassLoaderClass, "loadClass",
			"(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/Class;");
	classNameString = (jstring) env->NewGlobalRef(env->NewStringUTF(className));
	targetClass = (jclass) env->CallObjectMethod(dexClassLoaderObject,
			loadClassMethod, classNameString, systemClassLoaderObject);

	/* close dexfile */
	closeDexFile = env->GetMethodID(dexClassLoaderClass, "close", "()V");
	env->CallObjectMethod(dexClassLoaderObject, closeDexFile);

	if (!targetClass) {
		LOGE("Failed to load target class %s", className);
		return -1;
	}

	/* Invoke target method*/
	targetMethod = env->GetStaticMethodID(targetClass, methodName,
			"([Ljava/lang/String;)V");
	if (!targetMethod) {
		LOGE("Failed to load target method %s", methodName);
		return -1;
	}

	stringArray = env->NewObjectArray(argc, stringClass, NULL);
	for (int i = 0; i < argc; i++) {
		tmpString = env->NewStringUTF(argv[i]);
		env->SetObjectArrayElement(stringArray, i, tmpString);
	}

	env->CallStaticVoidMethod(targetClass, targetMethod, stringArray);
	env->DeleteGlobalRef(stringClass);
	env->DeleteGlobalRef(classLoaderClass);
	env->DeleteGlobalRef(dexClassLoaderClass);
	env->DeleteGlobalRef(dexPathString);
	env->DeleteGlobalRef(dexOptDirString);
	LOGD("Invoke dex X");
	return 0;
}

int hook(char *argv) {
	LOGD("loading dex begin: %s", argv);
	invoke_dex_method(argv, "com.assquad.inject.Hooker", "main", 1, &argv);
	LOGD("loading dex end");
    return -1;
}
}
