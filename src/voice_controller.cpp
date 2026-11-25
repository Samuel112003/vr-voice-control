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
        applyReset();   // le délai de sélection repart avant d'analyser la suite
        toParse.clear();
        for (std::size_t i = resumeAt; i < words.size(); ++i) toParse += (i > resumeAt ? " " : "") + words[i];
        break;
    }
    case ListeningState::Active:
        break;
    }

    // Chaque ordre est exécuté dès qu'il est reconnu : une action qui désélectionne
    // ou met en pause s'applique à la suite de la phrase.
    std::vector<Command> executed;
    parser_.parse(
        toParse,
        [this, &executed](const Command& c) {
            if (state_.load() != ListeningState::Active) return false;
            dispatch(c);
            executed.push_back(c);
            // "pause drone un avance" : la suite n'est pas exécutée.
            return state_.load() == ListeningState::Active;
        },
        [this](const Rejection& r) {
            if (unhandled_ && state_.load() == ListeningState::Active) unhandled_(r);
        });
    notifyTargets();
    return executed;
}

void VoiceController::clearTargets() {
    parser_.clearTargets();
    notifyTargets();
}

void VoiceController::update() {
    applyReset();
    if (state_.load() != ListeningState::Active) return;   // le délai ne court pas en pause / veille
    if (parser_.expireTargets()) notifyTargets();
}

void VoiceController::notifyTargets() {
    if (parser_.targets() == notifiedTargets_) return;
    notifiedTargets_ = parser_.targets();
    if (targetsChanged_) targetsChanged_(notifiedTargets_);
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
    const ListeningState previous = state_.exchange(state);
    if (previous == state) return;
    // Sortie de pause / veille : le temps passé ne compte pas dans le délai de
    // sélection. Appliqué par le thread de l'audio (applyReset), car setState
    // peut venir du thread d'un bouton.
    if (state == ListeningState::Active) touchRequested_ = true;
    if (listeningChanged_) listeningChanged_(state);
}

void VoiceController::applyReset() {
    if (touchRequested_.exchange(false)) parser_.touchTargets();
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
    update();   // oubli demandé par pause(), délai de sélection
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
        else update();   // pas d'audio : le délai de sélection court quand même
    }
    finishAudio();
}

std::vector<Command> VoiceController::handleRecognized() {
    std::string text = recognizer_->takeResult();
    if (transcript_) transcript_(text);
    return handleText(text);
}

} // namespace vc
