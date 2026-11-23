#include "voicecontrol/voice_controller.hpp"

#include <algorithm>
#include <iterator>

namespace vc {

std::vector<Command> VoiceController::handleText(const std::string& text) {
    applyReset();
    std::string toParse = text;

    switch (state_.load()) {
    case ListeningState::Paused:
        return {};
    case ListeningState::SoftPaused: {
        // Veille : on cherche le terme de réveil ; la suite de la phrase est exécutée.
        const Term* wake = wakeTerm_.load();
        const auto words = tokenize(text);
        std::size_t resumeAt = 0;   // position qui suit le terme de réveil (0 : pas trouvé)
        for (std::size_t pos = 0; wake && pos < words.size() && !resumeAt; ++pos)
            for (const auto& form : wake->tokens())   // la forme la plus longue l'emporte
                if (pos + form.size() <= words.size() &&
                    std::equal(form.begin(), form.end(), words.begin() + static_cast<std::ptrdiff_t>(pos)))
                    resumeAt = std::max(resumeAt, pos + form.size());
        if (!resumeAt) return {};
        setState(ListeningState::Active);
        toParse.clear();
        for (std::size_t i = resumeAt; i < words.size(); ++i) toParse += (i > resumeAt ? " " : "") + words[i];
        break;
    }
    case ListeningState::Active:
        break;
    }

    std::vector<Rejection> rejected;
    auto commands = parser_.parse(toParse, &rejected);
    std::vector<Command> executed;
    for (auto& c : commands) {
        // Une action peut mettre en pause ("pause drone un avance") : la suite n'est pas exécutée.
        if (state_.load() != ListeningState::Active) break;
        dispatch(c);
        executed.push_back(std::move(c));
    }
    if (unhandled_ && state_.load() == ListeningState::Active)
        for (const auto& r : rejected) unhandled_(r);
    return executed;
}

void VoiceController::pause() {
    resetRequested_ = true;
    setState(ListeningState::Paused);
}

void VoiceController::softPause(const Term& wakeTerm) {
    wakeTerm_ = &wakeTerm;
    resetRequested_ = true;
    setState(ListeningState::SoftPaused);
}

void VoiceController::resume() { setState(ListeningState::Active); }

void VoiceController::setState(ListeningState state) {
    if (state_.exchange(state) != state && listeningChanged_) listeningChanged_(state);
}

void VoiceController::applyReset() {
    if (!resetRequested_.exchange(false)) return;
    if (recognizer_) recognizer_->reset();
    parser_.cancelPending();
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
    applyReset();
    if (state_.load() == ListeningState::Paused) return {};   // audio jeté
    if (!recognizer_ || !recognizer_->acceptAudio(samples, count)) return {};
    return handleRecognized();
}

std::vector<Command> VoiceController::finishAudio() {
    applyReset();
    if (state_.load() == ListeningState::Paused) return {};
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
