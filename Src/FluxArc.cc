
#include "FluxArc/FluxArc.hh"
#include "lz4.h"
#include "lz4hc.h"
#include <fstream>
#include <ios>
#include <cstring>
#include <exception>
#include <stdexcept>

using namespace FluxArc;

Archive::Archive(const std::string& filename)
{
    std::ifstream wf(filename, std::ifstream::ate | std::ios::in | std::ios::binary);
    archive_filename = filename;

    if (!wf || !wf.good())
    {
        // File doesn't exist - new archive
        header = Header();

        // No idea what this does, or if this works
        header.magic_number = 5639;

        header.version = FLUX_ARC_VERSION;
        header.file_size = sizeof(Header);
        header.file_quantity = 0;

        database = std::map<std::string, FileHeader>();
        return;
    }

    // Load files
    Header memblock;

    // Check file size
    std::streampos size;
    // wf.seekg(std::ios::end);
    size = wf.tellg();
    wf.seekg(0, wf.beg);
    

    if (size < sizeof(Header))
    {
        throw std::invalid_argument("Error: File is to small to be a valid FluxArc");
    }

    // Get header
    wf.read((char *)&memblock, sizeof(Header));

    if (memblock.file_size != size)
    {
        throw std::invalid_argument("Error: Invalid FluxArc");
    }

    if (memblock.version != FLUX_ARC_VERSION)
    {
        throw std::invalid_argument("Error: Unsupported Flux Arc Version");
    }

    if (memblock.magic_number != 5639)
    {
        throw "Error: Invalid FluxArc";
    }

    // Load file database
    database = std::map<std::string, FileHeader>();
    for (int i = 0; i < memblock.file_quantity; i++)
    {
        FileHeader file;
        wf.read((char *)&file, sizeof(FileHeader));

        // Read name
        char* name = new char[file.name_size];
        wf.read(name, file.name_size);

        database[std::string(name, file.name_size)] = file;
    }

    header = memblock;

    // And done!
    wf.close();
}

int Archive::getFileSize(const std::string& fname)
{
    if (database.find(fname) == database.end())
    {
        throw std::invalid_argument("Error: File not in archive");
    }

    return database[fname].file_size_uc;
}

char* decompress(char* data, size_t size_c, size_t size, int* out_size)
{
    char* output = new char [size];
    int out = LZ4_decompress_safe(data, output, size_c, size);

    if (out < 0)
    {
        throw std::invalid_argument("Error: LZ4 decompression failed");
    }
    
    std::memcpy(out_size, &out, sizeof(out));

    delete[] data;
    
    return output;
}

char* compress(char* data, size_t size, int* out_size, bool release)
{
    
    auto dst_size = LZ4_compressBound(size);

    // One char = one byte. Remember that
    char* output = new char [dst_size];

    int out = 0;
    if (release)
    {
        // MAXIMUM COMPRESSION!!!!
        // Also maximum time, but that's not important
        out = LZ4_compress_HC(data, output, size, dst_size, LZ4HC_CLEVEL_MAX);
    }
    else
    {
        out = LZ4_compress_default(data, output, size, dst_size);
    }

    if (out == 0)
    {
        throw std::invalid_argument("Error: LZ4 compression failed");
    }

    int size_var = out;
    std::memcpy(out_size, &size_var, sizeof(out));

    // delete[] data;

    return output;
}

int Archive::getFile(const std::string& fname, char* data, bool res_compressed)
{
    if (database.find(fname) == database.end())
    {
        throw std::invalid_argument("Error: File not in archive");
    }

    std::ifstream wf(archive_filename, std::ios::out | std::ios::binary);

    if (!wf)
    {
        throw std::invalid_argument("Archive has been deleted since it was opened");
    }

    // Go to location of file
    wf.seekg(database[fname].position, std::ios::beg);

    // Load data into buffer
    char* buffer = new char[database[fname].file_size_c];
    wf.read(buffer, database[fname].file_size_c);

    int fs = database[fname].file_size_c;

    // Uncompress if needed
    if (database[fname].compressed && !res_compressed)
    {
        int out_size;
        buffer = decompress(buffer, database[fname].file_size_c, database[fname].file_size_uc, &out_size);

        if (out_size != database[fname].file_size_uc)
        {
            throw std::invalid_argument("Error: Invalid Archive");
        }

        fs = database[fname].file_size_uc;
    }

    // Return
    std::memcpy(data, buffer, fs);

    delete[] buffer;

    return fs;
}

std::string Archive::getFile(const std::string& fname)
{
    std::uint32_t size;
    char* data = new char[getFileSize(fname)];
    int fsize = getFile(fname, data);
    
    // Get size
    std::memcpy(&size, data, sizeof(std::uint32_t));

    // Get data
    char* out_str = new char[getFileSize(fname) - sizeof(std::uint32_t)];
    std::memcpy(out_str, data + sizeof(std::uint32_t), getFileSize(fname) - sizeof(std::uint32_t));

    std::string result(out_str, size);

    delete[] data;
    delete[] out_str;

    return result;
}

void Archive::setFile(const std::string& fname, char* data, int size, bool compressed, bool compress_release)
{
    if (database.find(fname) != database.end())
    {
        // Remove the old one
        database.erase(fname);
    }
    rebuild(fname, data, size, compressed, compress_release, true);
}

void Archive::setFile(const std::string& fname, const std::string& data, bool compressed, bool compress_release)
{
    char* buffer = new char[data.size() + sizeof(std::uint32_t)];
    std::uint32_t size = data.size();
    std::memcpy(buffer, &size, sizeof(std::uint32_t));

    // Copy in string data
    std::memcpy(buffer + sizeof(std::uint32_t), data.c_str(), size);

    // Actually set the file
    setFile(fname, buffer, size + sizeof(std::uint32_t), compressed, compress_release);
}

void Archive::rebuild()
{
    char x = 'o';
    rebuild("", &x, 0, false, false, false);
}

void Archive::rebuild(const std::string& fname, char* data, int size, bool compressed, bool compress_release, bool new_file)
{
    // Calculate file size
    int total_size = sizeof(Header);
    int header_size = sizeof(Header);
    for (auto it : database)
    {
        total_size += it.first.size();
        total_size += sizeof(FileHeader);
        header_size += it.first.size();
        header_size += sizeof(FileHeader);

        total_size += it.second.file_size_c;
    }

    FileHeader fh;
    if (new_file)
    {
        fh.file_size_uc = size;
        fh.name_size = fname.size();
        fh.compressed = compressed;
        total_size += fname.size();
        total_size += sizeof(FileHeader);
        header_size += fname.size();
        header_size += sizeof(FileHeader);

        if (compressed)
        {
            int out_size;
            data = compress(data, size, &out_size, compress_release);
            size = out_size;
        }

        fh.file_size_c = size;
        total_size += size;
    }

    // Build file
    char* buffer = new char[total_size];

    // Build old content first
    long long position = header_size;
    auto new_headers = database;
    for (auto it : database)
    {
        char* file_buffer = new char[it.second.file_size_c];
        int size = getFile(it.first, file_buffer, true);

        if (size != it.second.file_size_c)
        {
            throw std::invalid_argument("bad :(");
        }

        new_headers[it.first].position = position;

        memcpy(buffer + position, file_buffer, it.second.file_size_c);

        position += it.second.file_size_c;

        delete[] file_buffer;
    }

    database = new_headers;

    if (new_file)
    {
        // Build new content
        fh.position = position;
        memcpy(buffer + position, data, fh.file_size_c);

        // Add to database for header creation
        database[fname] = fh;
    }

    // Build headers
    position = 0;
    Header h;
    h.file_quantity = database.size();
    h.file_size = total_size;
    h.magic_number = 5639;
    h.version = FLUX_ARC_VERSION;

    header = h;

    memcpy(buffer, &h, sizeof(Header));
    position += sizeof(Header);

    // File headers
    for (auto it: database)
    {
        // Write header
        memcpy(buffer + position, &it.second, sizeof(FileHeader));
        position += sizeof(FileHeader);

        // Write name
        memcpy(buffer + position, it.first.c_str(), it.second.name_size);
        position += it.second.name_size;
    }

    // Now actually write it to a file
    std::ofstream wf(archive_filename, std::ios::binary | std::ios::out);
    wf.write(buffer, total_size);
    wf.close();

    delete[] buffer;

    if (compressed)
    {
        // We only need to do it for the compressed one because
        // compress() allocates memory
        delete[] data;
    }
}

void Archive::removeFile(const std::string& fname)
{
    if (database.find(fname) == database.end())
    {
        throw std::invalid_argument("Error: File not in archive");
    }

    database.erase(fname);

    rebuild();
}