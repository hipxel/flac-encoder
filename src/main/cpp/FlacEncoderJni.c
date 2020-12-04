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

#include "FlacEncoder.h"
#include "JavaDataWriter.h"

#include <jni.h>
#include <stdlib.h>

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
	return JNI_VERSION_1_6;
}

JNIEXPORT jobject JNICALL
Java_com_hipxel_flac_FlacEncoder_create(JNIEnv *env, jobject thiz, jobject dataWriter,
                                        jint sampleRate, jint channelsCount) {
	hipxel_DataWriter jdw = hipxel_JavaDataWriter_create(env, dataWriter);
	hipxel_FlacEncoder *ptr = hipxel_FlacEncoder_new(jdw, (uint32_t) sampleRate,
	                                                 (uint32_t) channelsCount);

	if (ptr->invalid) {
		hipxel_FlacEncoder_delete(ptr);
		return NULL;
	}

	return (*env)->NewDirectByteBuffer(env, ptr, sizeof(ptr));
}

JNIEXPORT void JNICALL
Java_com_hipxel_flac_FlacEncoder_release(JNIEnv *env, jobject thiz, jobject pointer) {
	hipxel_FlacEncoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	hipxel_FlacEncoder_delete(ptr);
}

JNIEXPORT jlong JNICALL
Java_com_hipxel_flac_FlacEncoder_write(JNIEnv *env, jobject thiz, jobject pointer,
                                       jbyteArray buffer, jlong offset, jlong length) {
	hipxel_FlacEncoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	return (hipxel_FlacEncoder_writeJni(ptr, env, buffer, offset, length));
}

JNIEXPORT jboolean JNICALL
Java_com_hipxel_flac_FlacEncoder_finish(JNIEnv *env, jobject thiz,
                                        jobject pointer) {
	hipxel_FlacEncoder *ptr = (*env)->GetDirectBufferAddress(env, pointer);
	hipxel_FlacEncoder_finish(ptr);
	return (jboolean) (ptr->finished && !ptr->invalid);
}
