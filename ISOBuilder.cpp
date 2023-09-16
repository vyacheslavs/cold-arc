//
// Created by developer on 9/7/23.
//

#include "ISOBuilder.h"
#include <iostream>
#include "FileStream.h"
#include <memory>

ISOBuilder::~ISOBuilder() {
    if (m_image) {
        iso_image_unref(m_image);
        m_image = nullptr;
    }
    iso_finish();
}

std::string explain_iso_builder_error(const cold_arc::Error& e) {
    std::stringstream ss;

    ss << cold_arc::explain_generic(e);
    ss << "libisofs error (code " << std::any_cast<int>(e.aux)<<")\n";

    return ss.str();
}

cold_arc::Result<> ISOBuilder::prepareImage(const std::string& imageId) {
    if (imageId.empty())
        return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ISOPrepareImageError, "imageId");

    if (int res = iso_image_new(imageId.c_str(), &m_image); res != ISO_SUCCESS)
        return unexpected_explained(cold_arc::ErrorCode::ISOPrepareImageError, explain_iso_builder_error, res);

    return {};
}

cold_arc::Result<IsoDir*> ISOBuilder::new_folder(const std::string& folder_name, IsoDir* parent) {
    if (folder_name.empty())
        return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ISONewFolderError, "folder_name");

    if (!parent)
        parent = iso_image_get_root(m_image);
    IsoDir* new_dir;
    if (auto res = iso_image_add_new_dir(m_image, parent, folder_name.c_str(), &new_dir); res < 0)
        return unexpected_explained(cold_arc::ErrorCode::ISONewFolderError, explain_iso_builder_error, res);
    return new_dir;
}

static std::string file_not_opened_for_writing(const cold_arc::Error& e) {
    std::stringstream ss;
    ss << cold_arc::explain_generic(e) << "couldn't open file "<<std::any_cast<std::string>(e.aux)<<" for writing\n";
    return ss.str();
}

[[nodiscard]] cold_arc::Result<> ISOBuilder::burn(const std::string& outfile, bool rockridge, bool joliet) {
    burn_source* bsrc;
    IsoWriteOpts* opts;

    if (outfile.empty())
        return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ISOBurnError, "outfile");

    if (auto res = iso_write_opts_new(&opts, 0); res < 0)
        return unexpected_explained(cold_arc::ErrorCode::ISOBurnError, explain_iso_builder_error, res);

    auto opt_clean_up = [](IsoWriteOpts* opt) { if (opt) iso_write_opts_free(opt); };
    std::unique_ptr<IsoWriteOpts, decltype(opt_clean_up)> opt_auto_clean_up {opts, opt_clean_up};

    iso_write_opts_set_iso_level(opts, 1);
    iso_write_opts_set_rockridge(opts, rockridge ? 1 : 0);
    iso_write_opts_set_joliet(opts, joliet ? 1 : 0);
    iso_write_opts_set_iso1999(opts, 0);
    if (auto res = iso_image_create_burn_source(m_image,opts,&bsrc); res < 0)
        return unexpected_explained(cold_arc::ErrorCode::ISOBurnError, explain_iso_builder_error, res);

    auto burn_source_clean_up = [](burn_source* b) {if (b) b->free_data(b); free(b);};
    std::unique_ptr<burn_source, decltype(burn_source_clean_up)> bsrc_auto_clean_up {bsrc, burn_source_clean_up};

    if (m_stream_callback) {
        m_stream_callback->offset = 0;
        m_stream_callback->total = bsrc->get_size(bsrc);
    }
    FILE* fp = fopen(outfile.c_str(), "w");
    if (!fp)
        return unexpected_explained(cold_arc::ErrorCode::ISOOutputFileCreateError, file_not_opened_for_writing, outfile);

    auto file_closer = [](FILE* fp) {fclose(fp); };
    std::unique_ptr<FILE, decltype(file_closer)> auto_closer(fp, file_closer);
    const int bsize = 2048;
    unsigned char buf[bsize];
    while (bsrc->read_xt(bsrc, buf, bsize) == bsize) {
        if (fwrite(buf, 1, bsize, fp) != bsize)
            return unexpected_error(cold_arc::ErrorCode::ISOBurnError);
        if (m_stream_callback) {
            m_stream_callback->offset += bsize;
        }
    }
    return {};
}

cold_arc::Result<IsoStream*> ISOBuilder::new_file(const std::string& file_name, const std::string& file_path, IsoDir* parent) {

    if (file_name.empty())
        return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ISONewFileError, "file_name");

    if (file_path.empty())
        return unexpected_invalid_input_parameter(cold_arc::ErrorCode::ISONewFileError, "file_path");

    IsoFile* file;
    IsoStream* stream = file_stream(file_path.c_str(), m_stream_callback);
    if (!stream)
        return unexpected_explained(cold_arc::ErrorCode::ISONewFileError, explain_iso_builder_error, ISO_OUT_OF_MEM);

    if (auto res = iso_image_add_new_file(m_image, parent, file_name.c_str(), stream, &file); res < 0) {
        file_stream_free(stream);
        return unexpected_explained(cold_arc::ErrorCode::ISONewFileError, explain_iso_builder_error, res);
    }
    return stream;
}

IsoDir* ISOBuilder::root_folder() const {
    return iso_image_get_root(m_image);
}
void ISOBuilder::set_stream_callback(stream_callback* cb) {
    m_stream_callback = cb;
}

cold_arc::Result<> ISOBuilder::construct() {
    if (auto ret = iso_init(); ret < 0)
        return unexpected_explained(cold_arc::ErrorCode::ISOInitError, explain_iso_builder_error, ret);
    return {};
}
