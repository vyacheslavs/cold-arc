//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_SIGNALS_H
#define COLD_ARC_GTK_SIGNALS_H

#include <sigc++/sigc++.h>

class Signals {
    public:
        static Signals& instance();

        sigc::signal<void> update_main_window;
};


#endif //COLD_ARC_GTK_SIGNALS_H
