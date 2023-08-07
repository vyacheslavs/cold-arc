//
// Created by developer on 8/7/23.
//

#include "Signals.h"

Signals &Signals::instance() {
    static Signals _s;
    return _s;
}

