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

package com.hipxel.flac

import java.nio.ByteBuffer

class FlacEncoder(dataWriter: DataWriter, sampleRate: Int, channelsCount: Int) {
	private var pointer: ByteBuffer? = null

	init {
		if (!Loader.loadNative())
			throw IllegalStateException("native library is not loaded")

		pointer = create(dataWriter, sampleRate, channelsCount)
		if (pointer == null)
			throw IllegalStateException("native create failed")
	}

	fun release() {
		pointer?.let {
			pointer = null
			release(it)
		}
	}

	fun finish(): Boolean {
		return pointer?.let { finish(it) } ?: false
	}

	fun write(buffer: ByteArray, offset: Long, length: Long): Long {
		return pointer?.let { write(it, buffer, offset, length) } ?: -1L
	}

	private external fun create(dataWriter: DataWriter, sampleRate: Int, channelsCount: Int): ByteBuffer?

	private external fun release(pointer: ByteBuffer)

	private external fun finish(pointer: ByteBuffer): Boolean

	private external fun write(pointer: ByteBuffer, buffer: ByteArray, offset: Long, length: Long): Long

	private object Loader {
		private val loaded by lazy {
			try {
				System.loadLibrary("HipxelFlacEncoder")
				true
			} catch (t: Throwable) {
				t.printStackTrace()
				false
			}
		}

		fun loadNative(): Boolean {
			return loaded
		}
	}
}