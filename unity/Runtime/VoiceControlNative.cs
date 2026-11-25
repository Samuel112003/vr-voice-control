using System;
using System.Runtime.InteropServices;

namespace VoiceControl {
    /// <summary>
    /// P/Invoke declarations of the native library voicecontrol_c
    /// (Assets/Plugins/x86_64/voicecontrol_c.dll). Mirrors bindings/c/include/voicecontrol_c.h.
    /// </summary>
    public static class VoiceControlNative {
        private const string Lib = "voicecontrol_c";

        /// <summary>Version of the C API; must match ExpectedApiVersion.</summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int vc_api_version();

        /// <summary>Creates a controller. Returns IntPtr.Zero on failure.</summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr vc_create();

        /// <summary>Destroys a controller created by vc_create. IntPtr.Zero is accepted.</summary>
        [DllImport(Lib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void vc_destroy(IntPtr controller);

        /// <summary>API version these declarations were written for.</summary>
        public const int ExpectedApiVersion = 1;
    }
}
