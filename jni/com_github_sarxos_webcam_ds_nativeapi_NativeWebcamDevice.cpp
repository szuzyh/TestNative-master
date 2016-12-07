#include <com_github_sarxos_webcam_ds_nativeapi_NativeWebcamDevice.h>
#include <videocapture/Capture.h>
#include <libyuv.h>

using namespace ca;

void fcallback(PixelBuffer& buffer);

JNIEXPORT jint JNICALL Java_com_github_sarxos_webcam_ds_nativeapi_NativeWebcamDevice_nativeOpen(JNIEnv *env, jobject obj, jint indexDevice, jint indexCapability, jobject byteBufferARGB) {
	if (sizeof(jlong) < sizeof(Capture *)) { // Abort because pointer to Capture object does not fit into jlong (this conversion is required for the current implementation and should work on most systems without problems)
		return -1;
	}

	void* pByteBufferARGB = env->GetDirectBufferAddress(byteBufferARGB);
	if (!pByteBufferARGB) { // Buffer not direct or direct buffers not supported by JVM
		return -1;
	}

	int retVal = 0;

	Capture* cap = new Capture(fcallback, pByteBufferARGB);
	jfieldID fieldID_ptrNativeCapture = env->GetFieldID(env->GetObjectClass(obj), "ptrNativeCapture", "J");
	env->SetLongField(obj, fieldID_ptrNativeCapture, reinterpret_cast<jlong>(cap));

	Settings cfg;
	cfg.device = indexDevice;
	cfg.capability = indexCapability;

	if ((*cap).open(cfg) < 0) {
		retVal = -1;
	}

	if ((*cap).start() < 0) {
		retVal = -1;
	}

	if (retVal != 0) {
		delete cap;
		cap = NULL;
		env->SetLongField(obj, fieldID_ptrNativeCapture, reinterpret_cast<jlong>(cap)); // reset pointer to NULL in Java layer
	}

	return retVal;
}

JNIEXPORT jint JNICALL Java_com_github_sarxos_webcam_ds_nativeapi_NativeWebcamDevice_nativeClose(JNIEnv *env, jobject obj) {
	if (sizeof(jlong) < sizeof(Capture *)) { // Abort because pointer to Capture object does not fit into jlong (this conversion is required for the current implementation and should work on most systems without problems)
		return -1;
	}

	jfieldID fieldID_ptrNativeCapture = env->GetFieldID(env->GetObjectClass(obj), "ptrNativeCapture", "J");
	Capture* cap = reinterpret_cast<Capture*>(env->GetLongField(obj, fieldID_ptrNativeCapture));

	int retVal = 0;
	if (cap) {
		if ((*cap).stop() < 0) {
			retVal = -1;
		}
		if ((*cap).close() < 0) {
			retVal = -1;
		}
		delete cap;
		cap = NULL;
		env->SetLongField(obj, fieldID_ptrNativeCapture, reinterpret_cast<jlong>(cap)); // reset pointer to NULL in Java layer
	}
	else {
		retVal = -2;
	}

	return retVal;

	return 0;
}

void fcallback(PixelBuffer& buffer) {
	size_t width = buffer.width[0];
	size_t height = buffer.height[0];

	// buffer.user (= byteBufferARGB): holds direct byte buffer passed from Java to native layer (read in Java to get webcam picture)

	// keep in sync with NativeWebcamDriver supportedPixelFormats
	if (buffer.pixel_format == CA_YUV420P) {
		libyuv::I420ToARGB(buffer.plane[0], buffer.stride[0], buffer.plane[1], buffer.stride[1], buffer.plane[2], buffer.stride[2], (uint8_t*)buffer.user, width * 4, width, height);
	}
	else if (buffer.pixel_format == CA_RGB24) {
		libyuv::RGB24ToARGB(buffer.plane[0], buffer.stride[0], (uint8_t*)buffer.user, width * 4, width, -1 * height); // -1 * flip vertically
	}
	else if (buffer.pixel_format == CA_YUYV422) {
		libyuv::YUY2ToARGB(buffer.plane[0], buffer.stride[0], (uint8_t*)buffer.user, width * 4, width, height);
	}
	else if (buffer.pixel_format == CA_YUV420BP) {
		libyuv::NV12ToARGB(buffer.plane[0], buffer.stride[0], buffer.plane[1], buffer.stride[1], (uint8_t*)buffer.user, width * 4, width, height);
	}
}