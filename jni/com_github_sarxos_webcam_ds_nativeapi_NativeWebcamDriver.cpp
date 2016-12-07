#include <com_github_sarxos_webcam_ds_nativeapi_NativeWebcamDriver.h>
#include <videocapture/Capture.h>

using namespace ca;

JNIEXPORT jint JNICALL Java_com_github_sarxos_webcam_ds_nativeapi_NativeWebcamDriver_nativeGetDevices(JNIEnv *env, jobject obj) {
	// Update if support for new formats is added! (keep in sync with fcallback in RoxluWebcamDevice!)
	// Order represents preferred order of pixel formats (if camera supports multiple formats for given setting, then the one with lowest index in this list is chosen)
	std::vector<int> supportedPixelFormats = { CA_RGB24, CA_YUV420P, CA_YUYV422, CA_YUV420BP };

	Capture cap(NULL, NULL);

	std::vector<Device> devices = cap.getDevices();

	size_t numDevicesFound = devices.size();
	if (numDevicesFound == 0) { // no devices found
		return -1;
	}

	/* 1. Retrieve Devices with Information and save in temporary variables (number of devices with valid pixel formats yet unknown) */

	std::vector<jint> tmp_devicesIndex;
	std::vector<std::string> tmp_devicesName;
	std::vector<std::vector<jint> > tmp_devicesCapabilityIndex;
	std::vector<std::vector<jint> > tmp_devicesResolutionX;
	std::vector<std::vector<jint> > tmp_devicesResolutionY;
	
	for (size_t i = 0; i < numDevicesFound; ++i) {
		/* Capabilities */
		// A. Find best Caps: One Cap per Resolution: Highest FPS and if multiple at that FPS exist then with preferred pixel_format (lowest index in supportedPixelFormats)
		std::vector<Capability> caps = cap.getCapabilities(devices[i].index);
		std::vector<Capability> bestCaps;
		for (size_t j = 0; j < caps.size(); ++j) {
			Capability cb = caps[j];
			if (std::find(supportedPixelFormats.begin(), supportedPixelFormats.end(), cb.pixel_format) != supportedPixelFormats.end()) { // is supported
				jboolean insertCb = true;
				for (size_t k = 0; k < bestCaps.size(); ++k) {
					Capability curBest = bestCaps[k];
					if (curBest.height == cb.height && curBest.width == cb.width) {
						insertCb = false; // cap with resolution already exísts
						if (cb.fps > curBest.fps || (cb.fps == curBest.fps && std::find(supportedPixelFormats.begin(), supportedPixelFormats.end(), cb.pixel_format) < std::find(supportedPixelFormats.begin(), supportedPixelFormats.end(), curBest.pixel_format))) {
							bestCaps.erase(bestCaps.begin() + k);
							insertCb = true; // new cap is better
							break;
						}
					}
				}
				if (insertCb) {
					bestCaps.push_back(cb);
				}
			}
		}

		// B. Save device info and best caps in temp vars
		if (bestCaps.size() > 0) { // Current Device features at least one supported Pixel Format
			std::vector<jint> tmp_curDevice_devicesCapabilityIndex;
			std::vector<jint> tmp_curDevice_devicesResolutionX;
			std::vector<jint> tmp_curDevice_devicesResolutionY;

			for (size_t j = 0; j < bestCaps.size(); ++j) {
				Capability cb = bestCaps[j];
				tmp_curDevice_devicesCapabilityIndex.push_back(cb.capability_index);
				tmp_curDevice_devicesResolutionX.push_back(cb.width);
				tmp_curDevice_devicesResolutionY.push_back(cb.height);
			}

			tmp_devicesIndex.push_back(devices[i].index);
			tmp_devicesName.push_back(devices[i].name);
			tmp_devicesCapabilityIndex.push_back(tmp_curDevice_devicesCapabilityIndex);
			tmp_devicesResolutionX.push_back(tmp_curDevice_devicesResolutionX);
			tmp_devicesResolutionY.push_back(tmp_curDevice_devicesResolutionY);
		}
	}

	/* 2. Copy values to Java layer */

	numDevicesFound = tmp_devicesIndex.size();
	if (numDevicesFound == 0) { // no devices (*with supported pixel formats*) found
		return -1;
	}

	jfieldID fieldID_devicesIndex = env->GetFieldID(env->GetObjectClass(obj), "devicesIndex", "[I");
	jfieldID fieldID_devicesName = env->GetFieldID(env->GetObjectClass(obj), "devicesName", "[Ljava/lang/String;");
	jfieldID fieldID_devicesCapabilityIndex = env->GetFieldID(env->GetObjectClass(obj), "devicesCapabilityIndex", "[[I");
	jfieldID fieldID_devicesResolutionX = env->GetFieldID(env->GetObjectClass(obj), "devicesResolutionX", "[[I");
	jfieldID fieldID_devicesResolutionY = env->GetFieldID(env->GetObjectClass(obj), "devicesResolutionY", "[[I");

	jintArray devicesIndex = env->NewIntArray(numDevicesFound);
	jobjectArray devicesName = env->NewObjectArray(numDevicesFound, env->FindClass("java/lang/String"), NULL);
	jobjectArray devicesCapabilityIndex = env->NewObjectArray(numDevicesFound, env->FindClass("[I"), NULL);
	jobjectArray devicesResolutionX = env->NewObjectArray(numDevicesFound, env->FindClass("[I"), NULL);
	jobjectArray devicesResolutionY = env->NewObjectArray(numDevicesFound, env->FindClass("[I"), NULL);

	env->SetIntArrayRegion(devicesIndex, 0, numDevicesFound, tmp_devicesIndex.data());
	for (size_t i = 0; i < numDevicesFound; ++i) {
		env->SetObjectArrayElement(devicesName, i, env->NewStringUTF(tmp_devicesName[i].c_str()));

		size_t numCaps = tmp_devicesCapabilityIndex[i].size();
		// devicesCapabilityIndex
		jintArray tmpIntArray = env->NewIntArray(numCaps);
		env->SetIntArrayRegion(tmpIntArray, 0, numCaps, tmp_devicesCapabilityIndex[i].data());
		env->SetObjectArrayElement(devicesCapabilityIndex, i, tmpIntArray);
		// devicesResolutionX
		tmpIntArray = env->NewIntArray(numCaps);
		env->SetIntArrayRegion(tmpIntArray, 0, numCaps, tmp_devicesResolutionX[i].data());
		env->SetObjectArrayElement(devicesResolutionX, i, tmpIntArray);
		// devicesResolutionY
		tmpIntArray = env->NewIntArray(numCaps);
		env->SetIntArrayRegion(tmpIntArray, 0, numCaps, tmp_devicesResolutionY[i].data());
		env->SetObjectArrayElement(devicesResolutionY, i, tmpIntArray);
	}

	env->SetObjectField(obj, fieldID_devicesIndex, devicesIndex);
	env->SetObjectField(obj, fieldID_devicesName, devicesName);
	env->SetObjectField(obj, fieldID_devicesCapabilityIndex, devicesCapabilityIndex);
	env->SetObjectField(obj, fieldID_devicesResolutionX, devicesResolutionX);
	env->SetObjectField(obj, fieldID_devicesResolutionY, devicesResolutionY);
	

	return 0;
}
