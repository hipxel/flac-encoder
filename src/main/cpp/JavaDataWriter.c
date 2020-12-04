/*
 * Copyright (C) 2020 Janusz Jankowski
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JavaDataWriter.h"

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
	JNIEnv *env;
	bool wasDetached;
} hipxel_jni;

static JavaVM *getVM(JNIEnv *env) {
	JavaVM *vm = NULL;
	(*env)->GetJavaVM(env, &vm);
	return vm;
}

static JNIEnv *getAttachedJNIEnv(JavaVM *jvm, bool *outWasDetached) {
	JNIEnv *env;

	if ((*jvm)->GetEnv(jvm, (void **) &env, JNI_VERSION_1_6) == JNI_EDETACHED) {
		*outWasDetached = true;
		(*jvm)->AttachCurrentThread(jvm, &env, NULL);
	} else {
		*outWasDetached = false;
	}
	return env;
}

static hipxel_jni prepareJni(JavaVM *jvm) {
	hipxel_jni jni;
	jni.wasDetached = false;
	if (NULL == jvm) {
		jni.env = NULL;
	} else {
		jni.env = getAttachedJNIEnv(jvm, &(jni.wasDetached));
	}
	return jni;
}

static void releaseJni(hipxel_jni jni, JavaVM *jvm) {
	if (jni.wasDetached)
		(*jvm)->DetachCurrentThread(jvm);
}

typedef struct {
	JavaVM *jvm;
	jobject javaObject;

	jmethodID mid_write;
	jmethodID mid_release;

	jbyteArray tmpBuffer;
	jint tmpLength;
} hipxel_JavaDataWriter;

static jbyteArray newGlobalByteArray(JNIEnv *env, jint length) {
	jbyteArray arr = (*env)->NewByteArray(env, length);
	jbyteArray globalRef = (jbyteArray) (*env)->NewGlobalRef(env, arr);
	(*env)->DeleteLocalRef(env, arr);
	return globalRef;
}

static hipxel_JavaDataWriter *hipxel_JavaDataWriter_new(JNIEnv *env, jobject dataReader) {
	hipxel_JavaDataWriter *jdw = malloc(sizeof(hipxel_JavaDataWriter));

	jdw->jvm = getVM(env);
	if (NULL == jdw->jvm)
		return jdw;

	jdw->javaObject = (*env)->NewGlobalRef(env, dataReader);
	jdw->tmpLength = 1;
	jdw->tmpBuffer = newGlobalByteArray(env, jdw->tmpLength);

	jclass cls = (*env)->FindClass(env, "com/hipxel/flac/DataWriter");
	jdw->mid_write = (*env)->GetMethodID(env, cls, "write", "(JJ[B)J");
	jdw->mid_release = (*env)->GetMethodID(env, cls, "release", "()V");
	(*env)->DeleteLocalRef(env, cls);

	return jdw;
}

static void cleanup_(hipxel_JavaDataWriter *jdw, JNIEnv *env) {
	if (!(*env)->ExceptionCheck(env))
		(*env)->CallVoidMethod(env, jdw->javaObject, jdw->mid_release);

	(*env)->DeleteGlobalRef(env, jdw->javaObject);
	(*env)->DeleteGlobalRef(env, jdw->tmpBuffer);
}

static void hipxel_JavaDataWriter_delete(hipxel_JavaDataWriter *jdw) {
	hipxel_jni jni = prepareJni(jdw->jvm);
	JNIEnv *env = jni.env;

	if (NULL != env)
		cleanup_(jdw, env);

	releaseJni(jni, jdw->jvm);

	free(jdw);
}

static jbyteArray prepareBuffer(hipxel_JavaDataWriter *jdw, JNIEnv *env, jint length) {
	if (length <= jdw->tmpLength)
		return jdw->tmpBuffer;

	(*env)->DeleteGlobalRef(env, jdw->tmpBuffer);

	jdw->tmpBuffer = newGlobalByteArray(env, length);
	return jdw->tmpBuffer;
}

static int64_t write_(hipxel_JavaDataWriter *jdw, JNIEnv *env,
                      int64_t position, int64_t length, const void *buffer) {
	jbyteArray jb = prepareBuffer(jdw, env, (jint) length);

	(*env)->SetByteArrayRegion(env, jb, 0, (jsize) length, (const jbyte *) buffer);

	if ((*env)->ExceptionCheck(env))
		return (jlong) -1;

	jlong ret = (*env)->CallLongMethod(env, jdw->javaObject, jdw->mid_write, position, length, jb);

	if ((*env)->ExceptionCheck(env))
		return (jlong) -1;

	return ret;
}

static int64_t hipxel_JavaDataWriter_write(void *p, int64_t position,
                                           int64_t length, const void *buffer) {
	hipxel_JavaDataWriter *jdw = (hipxel_JavaDataWriter *) p;

	hipxel_jni jni = prepareJni(jdw->jvm);
	JNIEnv *env = jni.env;

	int64_t ret = -1;
	if (NULL != env)
		ret = write_(jdw, env, position, length, buffer);

	releaseJni(jni, jdw->jvm);

	return ret;
}

static void jdw_release(void *p) {
	hipxel_JavaDataWriter_delete((hipxel_JavaDataWriter *) p);
}

hipxel_DataWriter hipxel_JavaDataWriter_create(JNIEnv *env, jobject obj) {
	hipxel_DataWriter v;
	v.write = hipxel_JavaDataWriter_write;
	v.release = jdw_release;
	v.p = hipxel_JavaDataWriter_new(env, obj);
	return v;
}
