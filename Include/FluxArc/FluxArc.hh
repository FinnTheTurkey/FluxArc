#ifndef FLUXARC_HH
#define FLUXARC_HH

#include <cstdint>
#include <sstream>
#include <string>
#include <map>

#define FLUX_ARC_VERSION 1

namespace FluxArc
{

    /** Storage Structs */
    struct Header
    {
        std::uint16_t magic_number;
        std::uint16_t version;
        std::uint64_t file_size;

        std::uint32_t file_quantity;
    };

    struct FileHeader
    {
        std::uint32_t name_size;
        bool compressed;
        std::uint64_t position;
        std::uint32_t file_size_uc;
        std::uint32_t file_size_c;
    };

    class Archive
    {
    public:
        Archive(const std::string& filename);

        /**
        Gets the file size of a file in the archive
        */
        int getFileSize(const std::string& fname);

        /**
        Loads a file from the archive. Loads directly from disk.
        Returns the size of the loaded file
        */
        int getFile(const std::string& fname, char* data, bool res_compressed=false);

        /**
        Gets a file from the archive as a string
        */
        std::string getFile(const std::string& fname);

        /** 
        Adds a file to the archive. This function is not smart; it re-builds the entire archive every time
        */
        void setFile(const std::string& fname, char* data, int size, bool compressed = false, bool compress_release = false);

        /**
        Adds a text file to the archive. This function is not smart; it re-builds the entire archive every time
        */
        void setFile(const std::string& fname, const std::string& data, bool compressed = false, bool compress_release = false);

        /**
        Removes a file from the archive. This function is not smart; it re-builds the entire archive
        */
        void removeFile(const std::string& fname);

        /**
        Rebuild the archive. Optionally do so with a new file
        */
        void rebuild();
        void rebuild(const std::string& fname, char* data, int size, bool compressed = false, bool compress_release = false, bool new_file = true);

    private:
        Header header;
        std::map<std::string, FileHeader> database;
        std::string archive_filename;
    };
}

#endif