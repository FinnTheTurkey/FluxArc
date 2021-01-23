#ifndef ASYNC_FILES_HH
#define ASYNC_FILES_HH
#include <filesystem>

#ifndef __EMSCRIPTEN__
#include <future>
#endif

namespace AsyncFiles
{
    enum Operation
    {
        Read, Write
    };

    class FilePromise
    {
    public:
        FilePromise(std::filesystem::path file, Operation op, char* data = nullptr, uint32_t isize = 0) {};
        virtual bool isDone() const {return false;}
        virtual char* get(uint32_t &size) {return nullptr;}
        virtual void wait() {};

    private:

    };

#ifndef __EMSCRIPTEN__

    class desktop_FilePromise : public FilePromise
    {
    public:
        desktop_FilePromise(std::filesystem::path file, Operation op, char* data = nullptr, uint32_t isize = 0);
        virtual bool isDone() const override;
        virtual char* get(uint32_t &size) override;
        virtual void wait() override;

    private:
        std::future<char*> promise;
        uint32_t size;
    };

#else

    class web_FilePromise : public FilePromise
    {
    public:
        web_FilePromise(std::filesystem::path file, Operation op, char* data = nullptr, uint32_t isize = 0);
        virtual bool isDone() const override;
        virtual char* get(uint32_t &size) override;
        virtual void wait() override;

        void setData(char* data, uint32_t size);
    private:
        uint32_t size;
        bool done;
        char* data;
    };

#endif

    /** Asyncrinously reads a binary file */
    FilePromise* read(std::filesystem::path filename);

    /** Asyncrinously writes a binary file. Currently doesn't work in WASM */
    FilePromise* write(std::filesystem::path filename, char* data, uint32_t size);

    void close(FilePromise* file);

}

#endif