#include "AsyncFiles/AsyncFiles.hh"
#include <cstring>
#include <filesystem>
#include <iostream>

using namespace AsyncFiles;

// #define __EMSCRIPTEN__

#ifndef __EMSCRIPTEN__
#include <fstream>
#include <future>

char* doFileSystem(std::filesystem::path file, Operation op, char* data, uint32_t osize)
{
    if (op == Operation::Read) {
        std::ifstream infile;
        infile.open(file, std::ios::binary | std::ios::in);
        infile.seekg(std::ios::end);
        uint32_t size = infile.tellg();
        infile.seekg(std::ios::beg);

        auto output = new char[size];
        infile.read(output, size);
        osize = size;

        infile.close();

        return output;
    }
    else
    {
        std::ofstream infile;
        infile.open(file, std::ios::binary | std::ios::out);
        infile.write(data, osize);
        infile.flush();
        infile.close();
        delete[] data;
        return nullptr;
    }
}

desktop_FilePromise::desktop_FilePromise(std::filesystem::path file, Operation op, char* data, uint32_t isize): FilePromise(file, op, data, isize)
{
    if (op == Operation::Read)
    {
        promise = std::async(std::launch::async, doFileSystem, file, op, data, size);
    }
    else
    {
        promise = std::async(std::launch::async, doFileSystem, file, op, data, isize);
    }
}

bool desktop_FilePromise::isDone() const
{
    return promise.valid();
}

char* desktop_FilePromise::get(uint32_t &size)
{
    size = this->size;
    return promise.get();
}

void desktop_FilePromise::wait()
{
    promise.wait();
}

#else

#include "emscripten.h"

void em_dataLoaded(void* userdata, void* data, int size)
{
    auto fp = (web_FilePromise*)userdata;
    auto buffer = new char[size];
    memcpy(buffer, data, size);
    fp->setData(buffer, size);
}

void em_error(void* userdata)
{
    std::cerr << "File failed to load\n";
}

web_FilePromise::web_FilePromise(std::filesystem::path file, Operation op, char* data, uint32_t isize):
FilePromise(file, op, data, isize)
{
    if (op == Operation::Write)
    {
        // Um.....
        // TODO: Implement this
    }
    else
    {
        emscripten_async_wget_data(file.string().c_str(), this, em_dataLoaded, em_error);
        done = false;
    }
}

void web_FilePromise::setData(char* data, uint32_t size)
{
    this->data = data;
    this->size = size;
    done = true;
}

bool web_FilePromise::isDone() const
{
    return done;
}

char* web_FilePromise::get(uint32_t &size)
{
    size = this->size;
    return data;
}

void web_FilePromise::wait()
{
    while (!done)
    {
        // Wait for async download to finish
    }
}

#endif

FilePromise* AsyncFiles::read(std::filesystem::path filename)
{
#ifndef __EMSCRIPTEN__
    return new desktop_FilePromise(filename, Operation::Read);
#else
    return new web_FilePromise(filename, Operation::Read);
#endif
}

FilePromise* AsyncFiles::write(std::filesystem::path filename, char* data, uint32_t size)
{
#ifndef __EMSCRIPTEN__
    return new desktop_FilePromise(filename, Operation::Write, data, size);
#else
    return new web_FilePromise(filename, Operation::Write, data, size);
#endif
}

void AsyncFiles::close(FilePromise *file)
{
    delete file;
}