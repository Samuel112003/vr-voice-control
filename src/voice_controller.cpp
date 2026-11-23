#include "voicecontrol/voice_controller.hpp"

#include <algorithm>
#include <iterator>

namespace vc {

std::vector<Command> VoiceController::handleText(const std::string& text) {
    std::vector<Rejection> rejected;
    auto commands = parser_.parse(text, &rejected);
    for (const auto& c : commands) dispatch(c);
    if (unhandled_)
        for (const auto& r : rejected) unhandled_(r);
    return commands;
}

void VoiceController::dispatch(const Command& command) const {
    if (!command.order) return;
    if (!command.order->owner()) {
        command.order->execute(command.args);
        return;
    }
    for (const Actor* actor : command.actors) command.order->execute(*actor, command.args);
}

void VoiceController::setRecognizer(std::unique_ptr<ISpeechRecognizer> recognizer) {
    recognizer_ = std::move(recognizer);
    refreshGrammar();
}

void VoiceController::refreshGrammar() {
    if (recognizer_) recognizer_->setGrammar(vocabulary_.phrases());
}

std::vector<Command> VoiceController::feedAudio(const std::int16_t* samples, std::size_t count) {
    if (!recognizer_ || !recognizer_->acceptAudio(samples, count)) return {};
    return handleRecognized();
}

std::vector<Command> VoiceController::finishAudio() {
    if (!recognizer_ || !recognizer_->finish()) return {};
    return handleRecognized();
}

std::vector<Command> VoiceController::processAudio(const AudioClip& clip) {
    // Blocs de 100 ms : le moteur détecte la fin de chaque phrase au fil de l'eau.
    const std::size_t block = std::max<std::size_t>(1, clip.sampleRate / 10);
    std::vector<Command> all;
    auto append = [&all](std::vector<Command> cmds) {
        all.insert(all.end(), std::make_move_iterator(cmds.begin()), std::make_move_iterator(cmds.end()));
    };
    for (std::size_t pos = 0; pos < clip.samples.size(); pos += block)
        append(feedAudio(clip.samples.data() + pos, std::min(block, clip.samples.size() - pos)));
    append(finishAudio());
    return all;
}

void VoiceController::listen(Microphone& microphone, const std::function<bool()>& keepListening) {
    std::vector<std::int16_t> block;
    while (keepListening()) {
        if (microphone.read(block)) feedAudio(block.data(), block.size());
    }
    finishAudio();
}

std::vector<Command> VoiceController::handleRecognized() {
    std::string text = recognizer_->takeResult();
    if (transcript_) transcript_(text);
    return handleText(text);
}

} // namespace vc
