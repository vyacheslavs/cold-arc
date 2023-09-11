//
// Created by developer on 9/7/23.
//

#include "ISOBuilder.h"
#include "Exceptions.h"
#include <iostream>
#include "FileStream.h"
#include <memory>

ISOBuilder::ISOBuilder() {
    if (iso_init() < 0)
        throw ISOBuilderConstructionException();
}

ISOBuilder::~ISOBuilder() {
    if (m_image) {
        iso_image_unref(m_image);
        m_image = nullptr;
    }
    iso_finish();
}

void ISOBuilder::prepareImage(const char* imageId) {
    int res = iso_image_new(imageId, &m_image);
    if (res != ISO_SUCCESS)
        throw ISOBuilderPrepareImageException();

}

IsoDir* ISOBuilder::new_folder(const char* folder_name, IsoDir* parent) {
    if (!parent)
        parent = iso_image_get_root(m_image);
    IsoDir* new_dir;
    auto res = iso_image_add_new_dir(m_image, parent, folder_name, &new_dir);
    if (res<0)
        throw ISOBuilderNewDirException();
    return new_dir;
}

void ISOBuilder::burn(const char* outfile, bool rockridge, bool joliet) {
    burn_source* bsrc;
    IsoWriteOpts* opts;

    auto res = iso_write_opts_new(&opts, 0);
    if (res<0)
        throw ISOBuilderNewOptsException();

    iso_write_opts_set_iso_level(opts, 1);
    iso_write_opts_set_rockridge(opts, rockridge ? 1 : 0);
    iso_write_opts_set_joliet(opts, joliet ? 1 : 0);
    iso_write_opts_set_iso1999(opts, 0);
    res = iso_image_create_burn_source(m_image,opts,&bsrc);
    if (res<0)
        throw ISOBuilderNewBurnSourceException();
    if (m_stream_callback) {
        m_stream_callback->offset = 0;
        m_stream_callback->total = bsrc->get_size(bsrc);
    }
    FILE* fp = fopen(outfile, "w");
    {
        auto file_closer = [](FILE* fp) {fclose(fp); };
        std::unique_ptr<FILE, decltype(file_closer)> auto_closer(fp, file_closer);
        const int bsize = 2048;
        unsigned char buf[bsize];
        while (bsrc->read_xt(bsrc, buf, bsize) == bsize) {
            if (fwrite(buf, 1, bsize, fp) != bsize)
                throw ISOBuilderException("write iso failed");
            if (m_stream_callback) {
                m_stream_callback->offset += bsize;
            }
        }
    }
    iso_write_opts_free(opts);
    bsrc->free_data(bsrc);
    free(bsrc);
}

IsoStream* ISOBuilder::new_file(const char* file_name, const char* file_path, IsoDir* parent) {
    IsoFile* file;
    IsoStream* stream = file_stream(file_path, m_stream_callback);
    if (!stream)
        throw ISOBuilderStreamException();

    auto res = iso_image_add_new_file(m_image, parent, file_name, stream, &file);
    if (res<0)
        throw ISOBuilderStreamException();

    return stream;
}

IsoDir* ISOBuilder::root_folder() const {
    return iso_image_get_root(m_image);
}
void ISOBuilder::set_stream_callback(stream_callback* cb) {
    m_stream_callback = cb;
}
