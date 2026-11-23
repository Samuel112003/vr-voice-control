#include "voicecontrol/vosk_recognizer.hpp"
#include "voicecontrol/vocabulary.hpp"

#include <vosk_api.h>

#include <algorithm>
#include <climits>
#include <stdexcept>

namespace vc {

namespace {

std::string jsonQuote(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out + '"';
}

void appendUtf8(std::string& out, unsigned cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

} // namespace

std::string extractVoskText(const std::string& json) {
    auto key = json.find("\"text\"");
    if (key == std::string::npos) return {};
    auto colon = json.find(':', key + 6);
    if (colon == std::string::npos) return {};
    auto quote = json.find('"', colon + 1);
    if (quote == std::string::npos) return {};

    std::string text;
    for (std::size_t i = quote + 1; i < json.size(); ++i) {
        char c = json[i];
        if (c == '"') break;
        if (c != '\\' || i + 1 >= json.size()) { text += c; continue; }
        char e = json[++i];
        switch (e) {
            case 'n': text += '\n'; break;
            case 't': text += '\t'; break;
            case 'r': text += '\r'; break;
            case 'b': text += '\b'; break;
            case 'f': text += '\f'; break;
            case 'u':
                if (i + 4 < json.size()) {
                    appendUtf8(text, static_cast<unsigned>(std::stoul(json.substr(i + 1, 4), nullptr, 16)));
                    i += 4;
                }
                break;
            default: text += e; break;   // \" \\ \/
        }
    }
    return text;
}

VoskRecognizer::VoskRecognizer(const std::string& modelPath, float sampleRate)
    : sampleRate_(sampleRate) {
    model_ = vosk_model_new(modelPath.c_str());
    if (!model_) throw std::runtime_error("Vosk : impossible de charger le modele '" + modelPath + "'");
    rebuildRecognizer();
}

VoskRecognizer::~VoskRecognizer() {
    if (recognizer_) vosk_recognizer_free(recognizer_);
    if (model_) vosk_model_free(model_);
}

void VoskRecognizer::setLogLevel(int level) { vosk_set_log_level(level); }

void VoskRecognizer::setGrammar(const std::vector<std::string>& phrases) {
    std::string json;
    ignored_.clear();
    for (const auto& phrase : phrases) {
        // Une expression n'est gardée que si le modèle connaît tous ses mots.
        auto words = tokenize(phrase);
        bool known = !words.empty();
        for (const auto& w : words) known = known && vosk_model_find_word(model_, w.c_str()) >= 0;
        if (!known) {
            ignored_.push_back(phrase);
            continue;
        }
        json += (json.empty() ? "[" : ", ") + jsonQuote(phrase);
    }
    // "[unk]" : un bruit ou un mot inconnu n'est pas forcé sur un mot de la grammaire.
    grammarJson_ = json.empty() ? std::string() : json + ", \"[unk]\"]";
    rebuildRecognizer();
}

void VoskRecognizer::rebuildRecognizer() {
    if (recognizer_) vosk_recognizer_free(recognizer_);
    recognizer_ = grammarJson_.empty()
        ? vosk_recognizer_new(model_, sampleRate_)
        : vosk_recognizer_new_grm(model_, sampleRate_, grammarJson_.c_str());
    if (!recognizer_) throw std::runtime_error("Vosk : impossible de creer le recognizer");
    result_.clear();
}

bool VoskRecognizer::acceptAudio(const std::int16_t* samples, std::size_t count) {
    bool complete = false;
    while (count > 0) {
        int n = static_cast<int>(std::min<std::size_t>(count, INT_MAX / 2));
        int rc = vosk_recognizer_accept_waveform_s(recognizer_, reinterpret_cast<const short*>(samples), n);
        if (rc < 0) throw std::runtime_error("Vosk : erreur de traitement audio");
        if (rc == 1 && storeResult(vosk_recognizer_result(recognizer_))) complete = true;
        samples += n;
        count -= static_cast<std::size_t>(n);
    }
    return complete;
}

bool VoskRecognizer::finish() {
    return storeResult(vosk_recognizer_final_result(recognizer_));
}

void VoskRecognizer::reset() {
    vosk_recognizer_reset(recognizer_);
    result_.clear();
}

bool VoskRecognizer::storeResult(const char* json) {
    std::string text = json ? extractVoskText(json) : std::string();
    // Ne garde pas les phrases composées uniquement de "[unk]".
    std::string cleaned;
    std::size_t pos = 0;
    while (pos < text.size()) {
        auto end = text.find(' ', pos);
        if (end == std::string::npos) end = text.size();
        std::string word = text.substr(pos, end - pos);
        if (!word.empty() && word != "[unk]") cleaned += (cleaned.empty() ? "" : " ") + word;
        pos = end + 1;
    }
    if (cleaned.empty()) return false;
    result_ = std::move(cleaned);
    return true;
}

std::string VoskRecognizer::takeResult() {
    std::string r = std::move(result_);
    result_.clear();
    return r;
}

} // namespace vc
