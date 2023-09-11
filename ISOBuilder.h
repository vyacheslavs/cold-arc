//
// Created by developer on 9/7/23.
//

#ifndef COLD_ARC_GTK_ISOBUILDER_H
#define COLD_ARC_GTK_ISOBUILDER_H

#include <cstdint>
#include <string>

#define LIBISOFS_WITHOUT_LIBBURN yes
#include "libisofs.h"
#include "FileStream.h"

class ISOBuilder {
    public:
        /**
         * Constructs ISO builder
         *
         * @throws ISOBuilderConstructionException
         */
        ISOBuilder();
        ~ISOBuilder();

        void prepareImage(const char* imageId);
        [[nodiscard]] IsoDir* new_folder(const char* folder_name, IsoDir* parent = nullptr);
        [[nodiscard]] IsoDir* root_folder() const;
        [[nodiscard]] IsoStream* new_file(const char* file_name, const char* file_path, IsoDir* parent);
        void burn(const char* outfile, bool rockridge, bool joliet);
        void set_stream_callback(stream_callback* cb);
    private:
        IsoImage* m_image {nullptr};
        stream_callback* m_stream_callback {nullptr};
};


#endif //COLD_ARC_GTK_ISOBUILDER_H
