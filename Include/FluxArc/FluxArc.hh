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
        uint16_t magic_number;
        uint16_t version;
        uint64_t file_size;

        uint32_t file_quantity;
    };

    struct FileHeader
    {
        uint32_t name_size;
        bool compressed;
        uint64_t position;
        uint32_t file_size_uc;
        uint32_t file_size_c;
    };

    /** A little helper class for creating binary files */
    class BinaryFile
    {
    public:
        BinaryFile()
        {
            index = 0;
            size = 0;
            // std::cout << "Created binary file" << std::endl;
        }

        BinaryFile(char* data, uint32_t size)
        {
            index = 0;
            this->data = data;
            this->size = size;

            // std::cout << "Created binary file from data" << std::endl;
        }

        BinaryFile(const BinaryFile &f)
        {
            index = f.index;
            size = f.size;
            data = new char[size];
            memcpy(data, f.data, size);
        }

        BinaryFile(BinaryFile&& f)
        {
            index = f.index;
            size = f.size;
            data = f.data;
            f.data = nullptr;
        }

        ~BinaryFile()
        {
            if (data != nullptr)
            {
                delete[] data;
                data = nullptr;
            }
            // std::cout << "Destroyed BinaryFile" << std::endl;
        }

        /** Puts the given variable into the binary file */
        template <typename T> 
        void set(const T& object)
        {
            set((char*)&object, sizeof(T));
        }

        void set(const char* to_add_data, uint32_t to_add_size)
        {
            if (index + to_add_size > size)
            {
                char* new_data = new char[index + to_add_size];

                if (data != nullptr)
                {
                    memcpy(new_data, data, size);
                }
                memcpy(new_data + index, to_add_data, to_add_size);

                if (data != nullptr)
                {
                    delete[] data;
                }
                data = new_data;
                size = index + to_add_size;
                index += to_add_size;
            }
            else
            {
                memcpy(data + index, to_add_data, to_add_size);
                index += to_add_size;
            }
        }

        void set(const std::string& object)
        {
            set((uint32_t)object.size());
            set(object.c_str(), object.size());
        }

        /** Adds the given amount of bytes as headroom. The cursor stays at the same place */
        void allocate(uint32_t add_size)
        {
            char* new_data = new char[size + add_size];
            if (data != nullptr)
            {
                memcpy(new_data, data, size);
                delete[] data;
            }

            data = new_data;

            size += add_size;
        }

        /** Gets an unspecified amount of data. You must free the result */
        bool get(char* new_data, int new_size)
        {
            if (index + new_size > size)
            {
                // std::cout << index + sizeof(T) << std::endl;
                return false;
            }

            memcpy(new_data, data + index, new_size);
            index += new_size;

            return true;
        }

        /** Gets a variable from the binary file. Returns true if it could be read, otherwise false */
        template <typename T>
        bool get(T* object)
        {
            get((char *)object, sizeof(T));

            return true;
        }

        /** Gets a string */
        std::string get()
        {
            uint32_t string_size;
            get(&string_size);

            char* str = new char[string_size];
            get(str, string_size);

            std::string output(str, string_size);
            delete[] str;

            return output;
        }

        /** Move the writing/reading cursor in the file to the given position. If it fails, it will return false */
        bool setCursor(uint32_t position)
        {
            if (position > size)
            {
                return false;
            }
            index = position;
            return true;
        }

        /** Returns where the writing/reading cursor is currently located */
        int getCursor() const
        {
            return index;
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
        Archive(const std::string& filename, bool dynamic = false);
        Archive() {dynamic = true;};
        ~Archive();

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

        bool dynamic;
        std::map<std::string, char* > file_data;
    };
}

#endif