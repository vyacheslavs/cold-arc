//
// Created by developer on 8/7/23.
//

#ifndef COLD_ARC_GTK_EXCEPTIONS_H
#define COLD_ARC_GTK_EXCEPTIONS_H

#include <stdexcept>
#include <glibmm/ustring.h>

    class WrongDatabaseVersion : public std::runtime_error {
        public:
            WrongDatabaseVersion(uint64_t vnow, uint64_t vexpected) : std::runtime_error(Glib::ustring::compose("Wrong databse version, supplied %1, expected %2", vnow, vexpected)) {}
    };

    class DBNotThreadSafe : public std::runtime_error {
        public:
            DBNotThreadSafe() : std::runtime_error("Sqlite compiled without thread safety support") {}

    };

    class ExtractResourceException : public std::runtime_error {
        public:
            explicit ExtractResourceException(const char* w) : std::runtime_error(w) {}
    };

    class ISOBuilderException : public std::runtime_error {
        public:
            explicit ISOBuilderException(const char* what) : std::runtime_error(what) {}
    };

    class ISOBuilderConstructionException : public ISOBuilderException {
        public:
            explicit ISOBuilderConstructionException() : ISOBuilderException("iso initialization failure") {}
    };

    class ISOBuilderPrepareImageException : public ISOBuilderException {
        public:
            explicit ISOBuilderPrepareImageException() : ISOBuilderException("iso prepare image failure") {}
    };

    class ISOBuilderNewDirException : public ISOBuilderException {
        public:
            explicit ISOBuilderNewDirException() : ISOBuilderException("iso new dir failed") {}
    };

    class ISOBuilderNewOptsException : public ISOBuilderException {
        public:
            explicit ISOBuilderNewOptsException() : ISOBuilderException("iso new opts failed") {}
    };

    class ISOBuilderNewBurnSourceException : public ISOBuilderException {
        public:
            explicit ISOBuilderNewBurnSourceException() : ISOBuilderException("iso new opts failed") {}
    };

    class ISOBuilderStreamException : public ISOBuilderException {
        public:
            explicit ISOBuilderStreamException() : ISOBuilderException("iso new stream failed") {}
    };

    class ISOBuilderInvalidArgument : public ISOBuilderException {
        public:
            explicit ISOBuilderInvalidArgument() : ISOBuilderException("wrong argument exception") {}
    };

    class ExceptionCargoBase {
        public:
            virtual void rethrow() = 0;
    };

    template <typename ET>
    class ExceptionCargo : public  ExceptionCargoBase {
        public:
            explicit ExceptionCargo(const ET& e) : m_e_cargo(e) {}

            void rethrow() override {
                throw ET(m_e_cargo);
            }
        private:
            ET m_e_cargo;
    };

#endif //COLD_ARC_GTK_EXCEPTIONS_H
