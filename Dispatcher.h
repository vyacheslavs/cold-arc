//
// Created by developer on 8/10/23.
//

#ifndef COLD_ARC_GTK_DISPATCHER_H
#define COLD_ARC_GTK_DISPATCHER_H

#include <gtkmm-3.0/gtkmm.h>

enum class DispatcherEmitPolicy {
    Force,
    Throttled,
    NoEmit,
};

struct Dispatcher {
    sigc::connection connect(sigc::slot<void>&& slot);
    void emit(DispatcherEmitPolicy policy = DispatcherEmitPolicy::Force);
    bool timeToEmit() const;
    Glib::Dispatcher dispatcher;
    std::chrono::time_point<std::chrono::steady_clock> lastEmit;
};

#endif //COLD_ARC_GTK_DISPATCHER_H
