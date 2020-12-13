#ifndef FLUXARC_HH
#define FLUXARC_HH

#include <cstdint>
#include <cstring>
#include <iostream>
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

    /** A little helper class for creating binary files */
    class BinaryFile
    {
    public:
        BinaryFile()
        {
            index = 0;
            size = 0;
        };

        BinaryFile(char* data, uint32_t size)
        {
            index = 0;
            this->data = data;
            this->size = size;
        };

        ~BinaryFile()
        {
            delete[] data;
        }

        /** Puts the given variable into the binary file */
        template <typename T> 
        void set(const T& object)
        {
            uint32_t new_size = size + sizeof(T);
            char* new_data = new char[new_size];

            if (data != nullptr)
            {
                memcpy(new_data, data, size);
            }
            memcpy(new_data + index, (char *)&object, sizeof(T));

            delete[] data;
            data = new_data;
            size = new_size;
            index = new_size;
        }

        void set(const std::string& object)
        {
            uint32_t new_size = size + sizeof(uint32_t) + object.size();
            char* new_data = new char[new_size];

            if (data != nullptr)
            {
                memcpy(new_data, data, size);
            }
            uint32_t string_size = object.size();
            memcpy(new_data + index, &string_size, sizeof(uint32_t));
            memcpy(new_data + index + sizeof(uint32_t), object.c_str(), string_size);

            delete[] data;
            data = new_data;
            size = new_size;
            index = new_size;
        }

        /** Gets a variable from the binary file. Returns true if it could be read, otherwise false */
        template <typename T>
        bool get(T* object)
        {
            if (index + sizeof(T) > size)
            {
                std::cout << index + sizeof(T) << std::endl;
                return false;
            }

            memcpy(object, data + index, sizeof(T));
            index += sizeof(T);

            return true;
        }

        /** Gets a string */
        std::string get()
        {
            if (index + sizeof(uint32_t) > size)
            {
                return std::string("BinaryFile.get() failed :(");
            }

            uint32_t string_size;
            memcpy(data + index, &string_size, sizeof(uint32_t));

            char* str = new char[string_size];
            memcpy(str, data + index + sizeof(uint32_t), string_size);
            index += sizeof(uint32_t) + string_size;

            std::string output(str, string_size);
            delete[] str;

            return output;
        }

        char* getDataPtr() const
        {
            return data;
        }

        uint32_t getSize() const
        {
            return size;
        }

    private:
        uint32_t index;
        char* data = nullptr;

        uint32_t size;

    };

    class Archive
    {
    public:
        Archive(const std::string& filename);

        /**
        Checks if a file exists within the archive
        */
        bool hasFile(const std::string& fname)
        {
            return database.find(fname) != database.end();
        }

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
        Gets a file as a BinaryFile
        */
        BinaryFile getBinaryFile(const std::string& fname)
        {
            uint32_t size = getFileSize(fname);
            char* buffer = new char[size];
            getFile(fname, buffer);

            return BinaryFile(buffer, size);
        }

        /** 
        Adds a file to the archive. This function is not smart; it re-builds the entire archive every time
        */
        void setFile(const std::string& fname, char* data, int size, bool compressed = false, bool compress_release = false);

        /**
        Adds a text file to the archive. This function is not smart; it re-builds the entire archive every time
        */
        void setFile(const std::string& fname, const std::string& data, bool compressed = false, bool compress_release = false);

        /**
        Puts a BinaryFile into the FluxArc
        */
        void setFile(const std::string& fname, const BinaryFile& file, bool compressed = false, bool compress_release = false)
        {
            setFile(fname, file.getDataPtr(), file.getSize(), compressed, compress_release);
        }

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