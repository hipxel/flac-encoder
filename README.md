# FLAC encoder for android
Android's [MediaCodec](https://developer.android.com/reference/android/media/MediaCodec) has issues
with making proper FLAC files (with valid headers) on most of pre-Android 10 devices.

This library adds minimalistic FLAC decoder written in C with Kotlin bindings (just takes ~30kB per arch).
In order to use it, add it f.e. from Android Studio as Gradle module.
