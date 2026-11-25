using System.Collections.Generic;
using UnityEngine;

namespace VoiceControl {
    /// <summary>
    /// Marks a GameObject as controllable by voice.
    /// <see cref="id"/> is the actor name in the vocabulary; the registry maps it to this object.
    /// </summary>
    public class VoiceControllable : MonoBehaviour {
        [Tooltip("Actor name in the vocabulary (e.g. \"cube\").")]
        public string id;

        private static readonly Dictionary<string, VoiceControllable> registry =
            new Dictionary<string, VoiceControllable>();

        /// <summary>Finds the controllable object registered under this actor id.</summary>
        public static bool TryGet(string actorId, out VoiceControllable controllable) {
            return registry.TryGetValue(actorId, out controllable);
        }

        private void OnEnable() {
            if (string.IsNullOrEmpty(id)) {
                Debug.LogWarning($"VoiceControllable on '{name}' has no id: it will not receive orders.", this);
                return;
            }
            if (registry.TryGetValue(id, out var other) && other != this) {
                Debug.LogWarning($"VoiceControllable id '{id}' is used by '{other.name}' and '{name}'.", this);
            }
            registry[id] = this;
        }

        private void OnDisable() {
            if (!string.IsNullOrEmpty(id) && registry.TryGetValue(id, out var current) && current == this) {
                registry.Remove(id);
            }
        }
    }
}
