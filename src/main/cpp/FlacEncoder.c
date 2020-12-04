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

#include <FLAC/stream_encoder.h>

#include <android/log.h>

#include <stdlib.h>
#include <string.h>

#define HIPXEL_LOG_ERROR(...) \
    ((void)__android_log_print(ANDROID_LOG_ERROR, "FlacEncoder", __VA_ARGS__))

static FLAC__StreamEncoderSeekStatus seekCallback(
		const FLAC__StreamEncoder *encoder,
		FLAC__uint64 absolute_byte_offset,
		void *client_data) {
	hipxel_FlacEncoder *fd = (hipxel_FlacEncoder *) client_data;

	fd->currentOffset = absolute_byte_offset;
	return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
}

static FLAC__StreamEncoderTellStatus tellCallback(
		const FLAC__StreamEncoder *encoder,
		FLAC__uint64 *absolute_byte_offset,
		void *client_data) {
	hipxel_FlacEncoder *fd = (hipxel_FlacEncoder *) client_data;

	int64_t p = fd->currentOffset;
	*absolute_byte_offset = (uint64_t) (0 > p ? 0 : p);
	return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
}

static FLAC__StreamEncoderWriteStatus writeCallback(
		const FLAC__StreamEncoder *encoder,
		const FLAC__byte buffer[],
		size_t bytes, unsigned samples, unsigned current_frame,
		void *client_data) {
	hipxel_FlacEncoder *fd = (hipxel_FlacEncoder *) client_data;

	int64_t r = fd->writer.write(fd->writer.p, fd->currentOffset, bytes, buffer);
	if (r != bytes) {
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	}

	fd->currentOffset += r;

	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

static void init(hipxel_FlacEncoder *fd) {
	fd->invalid = true;

	FLAC__StreamEncoder *encoder = FLAC__stream_encoder_new();
	if (NULL == encoder) {
		HIPXEL_LOG_ERROR("couldn't create encoder");
		return;
	}
	fd->internalEncoder = encoder;

	bool ok = true;

	ok &= FLAC__stream_encoder_set_verify(encoder, true);
	ok &= FLAC__stream_encoder_set_compression_level(encoder, 5);
	ok &= FLAC__stream_encoder_set_channels(encoder, fd->info.channelsCount);
	ok &= FLAC__stream_encoder_set_bits_per_sample(encoder, fd->info.bitsPerSample);
	ok &= FLAC__stream_encoder_set_sample_rate(encoder, fd->info.sampleRate);

	if (!ok) {
		HIPXEL_LOG_ERROR("FLAC encoder prepare failed");
		return;
	}

	FLAC__StreamEncoderInitStatus initStatus = FLAC__stream_encoder_init_stream(
			encoder,
			writeCallback,
			seekCallback,
			tellCallback,
			NULL,
			fd);

	if (initStatus != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
		HIPXEL_LOG_ERROR("FLAC encoder init failed %d", (int) initStatus);
		return;
	}

	fd->invalid = false;
}

static void prepareBuffers(hipxel_FlacEncoder *fd, size_t length) {
	if (length <= fd->buffersLength)
		return;

	fd->buffersLength = length;
	fd->buffer16 = realloc(fd->buffer16, length * sizeof(int16_t));
	fd->buffer32 = realloc(fd->buffer32, length * sizeof(int32_t));

	if (fd->buffer16 == NULL || fd->buffer32 == NULL) {
		HIPXEL_LOG_ERROR("FLAC encoder failed buffers realloc");
		fd->invalid = true;
	}
}

jlong hipxel_FlacEncoder_writeJni(hipxel_FlacEncoder *fd,
                                  JNIEnv *env, jbyteArray buffer, jlong offset, jlong length) {
	FLAC__StreamEncoder *encoder = (FLAC__StreamEncoder *) fd->internalEncoder;

	size_t samples = (size_t) (length / 2);
	size_t frames = samples / fd->info.channelsCount;

	prepareBuffers(fd, samples);
	if (fd->invalid)
		return -1;

	int16_t *b16 = fd->buffer16;
	int32_t *b32 = fd->buffer32;

	(*env)->GetByteArrayRegion(env, buffer, (jsize) offset, samples * 2, (jbyte *) b16);

	for (size_t i = 0; i < samples; ++i) {
		b32[i] = b16[i];
	}

	FLAC__bool ret = FLAC__stream_encoder_process_interleaved(encoder, b32, frames);

	return ret ? length : -1;
}

void hipxel_FlacEncoder_finish(hipxel_FlacEncoder *fd) {
	FLAC__StreamEncoder *encoder = (FLAC__StreamEncoder *) fd->internalEncoder;

	if (fd->finished || fd->invalid || encoder == NULL)
		return;

	fd->finished = true;
	fd->invalid = !FLAC__stream_encoder_finish(encoder);
}

hipxel_FlacEncoder *hipxel_FlacEncoder_new(hipxel_DataWriter writer,
                                           uint32_t sampleRate, uint32_t channelsCount) {
	hipxel_FlacEncoder *fd = malloc(sizeof(hipxel_FlacEncoder));

	fd->writer = writer;
	fd->internalEncoder = NULL;

	fd->buffersLength = 0;
	fd->buffer16 = NULL;
	fd->buffer32 = NULL;

	fd->currentOffset = 0;

	fd->finished = false;
	fd->invalid = false;

	fd->info.sampleRate = sampleRate;
	fd->info.channelsCount = channelsCount;
	fd->info.bitsPerSample = 16;

	if (!fd->invalid)
		init(fd);

	return fd;
}

void hipxel_FlacEncoder_delete(hipxel_FlacEncoder *fd) {
	if (NULL != fd->internalEncoder)
		FLAC__stream_encoder_delete((FLAC__StreamEncoder *) fd->internalEncoder);

	free(fd->buffer16);
	free(fd->buffer32);

	fd->writer.release(fd->writer.p);

	free(fd);
}
