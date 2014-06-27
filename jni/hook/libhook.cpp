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
#include "AndroidRuntime.h"
#include "../inject/utils.h"
#include "Dalvik.h"
#include "Proxy.h"

extern "C" {

static void Hook_generateProxyClass(JNIEnv* env, jobject thiz,
		jstring className, jstring srcClass) {
	const char* clazz = (env)->GetStringUTFChars(className, 0);
	const char* name = (env)->GetStringUTFChars(srcClass, 0);

	dvmGenerateProxyClass(clazz, name);

	env->ReleaseStringUTFChars(className, clazz);
	env->ReleaseStringUTFChars(srcClass, name);
	return;
}

const JNINativeMethod gMethods[] = { { "generateProxyClass",
		"(Ljava/lang/String;Ljava/lang/String;)V",
		(void *) Hook_generateProxyClass } };

static int register_android_jni(JNIEnv *env, jclass clazz) {
	return env->RegisterNatives(clazz, gMethods, sizeof(gMethods));
}

int invoke_dex_method(const char* dexPath, const char* className,
		const char* proxyName, const char* methodName, int argc, char *argv[]) {
	LOGD("Invoke dex E");
	JNIEnv * env = android::AndroidRuntime::getJNIEnv();

	jclass stringClass, classLoaderClass, dexClassLoaderClass, targetClass,
			proxyClass;
	jmethodID getSystemClassLoaderMethod, dexClassLoaderContructor,
			loadClassMethod, targetMethod, closeDexFile;
	jobject systemClassLoaderObject, dexClassLoaderObject;
	jstring dexPathString, dexOptDirString, classNameString, tmpString;
	jobjectArray stringArray;

	/* Get SystemClasLoader */
	stringClass = (jclass) env->NewGlobalRef(
			env->FindClass("java/lang/String"));
	classLoaderClass = (jclass) env->NewGlobalRef(
			env->FindClass("java/lang/ClassLoader"));
	dexClassLoaderClass = (jclass) env->NewGlobalRef(
			env->FindClass("dalvik/system/PathClassLoader"));
	getSystemClassLoaderMethod = env->GetStaticMethodID(classLoaderClass,
			"getSystemClassLoader", "()Ljava/lang/ClassLoader;");
	systemClassLoaderObject = env->CallStaticObjectMethod(classLoaderClass,
			getSystemClassLoaderMethod);

	/* Create DexClassLoader */
	dexClassLoaderContructor = env->GetMethodID(dexClassLoaderClass, "<init>",
			"(Ljava/lang/String;Ljava/lang/ClassLoader;)V");
	dexPathString = (jstring) env->NewGlobalRef(env->NewStringUTF(dexPath));

	dexClassLoaderObject = env->NewObject(dexClassLoaderClass,
			dexClassLoaderContructor, dexPathString, systemClassLoaderObject);

	/* Use DexClassLoader to load target class */
	loadClassMethod = env->GetMethodID(dexClassLoaderClass, "loadClass",
			"(Ljava/lang/String;)Ljava/lang/Class;");
	classNameString = (jstring) env->NewGlobalRef(env->NewStringUTF(className));
	targetClass = (jclass) env->CallObjectMethod(dexClassLoaderObject,
			loadClassMethod, classNameString);

	classNameString = (jstring) env->NewGlobalRef(env->NewStringUTF(proxyName));
	proxyClass = (jclass) env->CallObjectMethod(dexClassLoaderObject,
			loadClassMethod, classNameString);

	if (!targetClass) {
		LOGE("Failed to load target class %s", className);
		return -1;
	}

	if (!proxyClass) {
		LOGE("Failed to load proxyClass class %s", proxyName);
		return -1;
	}
	register_android_jni(env, proxyClass);

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

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = NULL;
	jint result = -1;

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		return -1;
	}
	assert(env != NULL);

	jclass proxyClass = env->FindClass("com.assquad.inject.Proxy");

	register_android_jni(env, proxyClass);

	result = JNI_VERSION_1_4;

	return result;
}

int hook(char *argv) {
	LOGD("loading dex begin");
	invoke_dex_method(argv, "com.assquad.inject.Hook",
			"com.assquad.inject.Proxy", "main", 0, NULL);
	LOGD("loading dex end");
	return -1;
}
}
