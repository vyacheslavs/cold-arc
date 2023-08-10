//
// Created by developer on 8/10/23.
//

#include "Dispatcher.h"

sigc::connection Dispatcher::connect(sigc::slot<void>&& slot) {
    return dispatcher.connect(std::move(slot));
}

void Dispatcher::emit(DispatcherEmitPolicy policy) {
    if (policy == DispatcherEmitPolicy::Force)
        dispatcher.emit();
    else if (policy == DispatcherEmitPolicy::Throttled) {
        if (timeToEmit()) {
            lastEmit = std::chrono::steady_clock::now();
            dispatcher.emit();
        }
    }
}

bool Dispatcher::timeToEmit() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(now - lastEmit).count() > 0;
}

