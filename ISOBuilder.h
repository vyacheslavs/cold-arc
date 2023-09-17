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
#include "Utils.h"

class ISOBuilder {
    public:
        ISOBuilder() = default;
        ~ISOBuilder();

        [[nodiscard]] cold_arc::Result<> construct();
        [[nodiscard]] cold_arc::Result<> prepareImage(const std::string& imageId);
        [[nodiscard]] cold_arc::Result<IsoDir*> new_folder(const std::string& folder_name, IsoDir* parent = nullptr);
        [[nodiscard]] IsoDir* root_folder() const;
        [[nodiscard]] cold_arc::Result<IsoStream*> new_file(const std::string& file_name, const std::string& file_path, IsoDir* parent);
        [[nodiscard]] cold_arc::Result<> burn(const std::string& outfile, bool rockridge, bool joliet);
        void set_stream_callback(stream_callback* cb);
    private:
        IsoImage* m_image {nullptr};
        stream_callback* m_stream_callback {nullptr};
};


#endif //COLD_ARC_GTK_ISOBUILDER_H
