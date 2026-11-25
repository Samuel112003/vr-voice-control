using System;
using UnityEngine;
using VoiceControl;

/// <summary>
/// Checks that voicecontrol_c.dll loads in Unity: logs the API version,
/// creates a controller and destroys it with the component.
/// </summary>
public class NativeCoreTest : MonoBehaviour {
    private IntPtr controller = IntPtr.Zero;

    private void Start() {
        int version = VoiceControlNative.vc_api_version();
        if (version != VoiceControlNative.ExpectedApiVersion) {
            Debug.LogError($"voicecontrol_c API version {version}, expected {VoiceControlNative.ExpectedApiVersion}.");
            return;
        }
        Debug.Log($"voicecontrol_c loaded, API version {version}.");

        controller = VoiceControlNative.vc_create();
        if (controller == IntPtr.Zero) {
            Debug.LogError("vc_create failed.");
        } else {
            Debug.Log("Native controller created.");
        }
    }

    private void OnDestroy() {
        if (controller != IntPtr.Zero) {
            VoiceControlNative.vc_destroy(controller);
            controller = IntPtr.Zero;
        }
    }
}
