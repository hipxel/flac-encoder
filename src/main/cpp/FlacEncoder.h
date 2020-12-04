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

#ifndef HIPXEL_FLACENCODER
#define HIPXEL_FLACENCODER

#include "DataWriter.h"

#include <stdbool.h>

#include <jni.h>

struct hipxel_GrowingBuffer;

typedef struct hipxel_FlacEncoder {
	hipxel_DataWriter writer;
	void *internalEncoder;

	int16_t *buffer16;
	int32_t *buffer32;
	size_t buffersLength;

	int64_t currentOffset;

	bool finished;
	bool invalid;

	struct {
		uint32_t sampleRate;
		uint32_t channelsCount;
		uint32_t bitsPerSample;
	} info;
} hipxel_FlacEncoder;

hipxel_FlacEncoder *hipxel_FlacEncoder_new(hipxel_DataWriter writer,
                                           uint32_t sampleRate, uint32_t channelsCount);

void hipxel_FlacEncoder_finish(hipxel_FlacEncoder *fd);

void hipxel_FlacEncoder_delete(hipxel_FlacEncoder *fd);

jlong hipxel_FlacEncoder_writeJni(hipxel_FlacEncoder *fd,
                                  JNIEnv *env, jbyteArray buffer, jlong offset, jlong length);

#endif // HIPXEL_FLACENCODER
