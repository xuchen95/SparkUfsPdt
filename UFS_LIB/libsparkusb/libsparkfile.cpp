#include "pch.h"
#include "libsparkusb.h"
#include <fstream>

using namespace std;

LIBSPARK_API int spark::file::fnReadFile(const char* file_path, char* dest, int off /*= 0*/, int size /*= 0*/, int* len /*= nullptr*/)
{
    ifstream file(file_path, ios::binary | ios::in);
    if (!file)
    {
        return ERROR_FILE_NOT_FOUND;
    }

    //2 get file size
    int tcsize;
    file.seekg(0, file.end);
    tcsize = file.tellg();

    if (size)
    {
        if (tcsize < (off + size))
        {
#if TOOLSET_VER > 141
            return ERROR_FILE_NOT_SUPPORTED;
#else
            return 425L;
#endif // TOOLSET_VER
        }
    }
    else
    {
        size = tcsize - off;
    }
    file.seekg(off, file.beg);
    file.read(dest, size);

    if (len != nullptr)
    {
        if (file)
        {
            *len = size;
        }
        else
        {
            *len = file.gcount();
        }
    }
    file.close();

    return ERROR_SUCCESS;
}

LIBSPARK_API int spark::file::fnWriteFile(const char* file_path, char* src, int size)
{
    ofstream file(file_path, ios::binary | ios::out);
    if (!file)
    {
        return ERROR_FILE_INVALID;
    }

    file.write(src, size);
    file.close();

    return ERROR_SUCCESS;
}

LIBSPARK_API int spark::file::fnFileSize(const char* file_path, int* size)
{
    ifstream file(file_path, ios::binary | ios::in);
    if (!file)
    {
        return ERROR_FILE_NOT_FOUND;
    }

    //2 get file size
    int tcsize;
    file.seekg(0, file.end);
    tcsize = file.tellg();
    *size = tcsize;
    file.close();

    return ERROR_SUCCESS;
}
