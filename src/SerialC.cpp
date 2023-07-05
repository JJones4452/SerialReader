#include "SerialC.h"

#include "SerialReader.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// Inside this "extern C" block, I can implement functions in C++, which will externally 
//   appear as C functions (which means that the function IDs will be their names, unlike
//   the regular C++ behavior, which allows defining multiple functions with the same name
//   (overloading) and hence uses function signature hashing to enforce unique IDs),


static SerialReader *SerialReader_instance = NULL;

void InitialiseReader(const char* id, unsigned baud, unsigned buffer_size) 
{
    if (SerialReader_instance == NULL) 
    {
        SerialReader_instance = new SerialReader(id, baud, buffer_size);
        printf("Size of object: %zu bytes\n", sizeof(*SerialReader_instance));
    }
}

int Connect()
{
    if(SerialReader_instance == NULL) return false;
    return SerialReader_instance->Connect();
}

void StartRead()
{
    if(SerialReader_instance == NULL) return;

    std::atomic<bool> start_signal(false);
    if(SerialReader_instance->IsConnected())
    {
        SerialReader_instance->StartReading(start_signal);
    }
}

void StopRead(void)
{
    if(SerialReader_instance == NULL) return;
    if(SerialReader_instance->IsConnected())
    {
        std::atomic<bool> stop_signal(true);

        SerialReader_instance->StopReading(stop_signal);
    }
}


void Delay(unsigned seconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}


struct ResultStruct GetCTypeArray()
{
    ResultStruct result;
    if (SerialReader_instance == nullptr)
    {
        result.results = nullptr;
        result.size = 0;
        return result;
    }

    auto result_vec = SerialReader_instance->GetImmutableResults();
    size_t totalLength = 0;
    for (const std::string& str : result_vec)
    {
        totalLength += str.length() + 1; // Include space for the null-terminator
    }

    // Create a shared_ptr to manage the memory
    std::shared_ptr<char[]> buffer(new char[totalLength]);
    std::vector<const char*> results;

    // Copy each string to the C-friendly array
    char* current = buffer.get();
    for (const std::string& str : result_vec)
    {
#ifdef _MSC_VER
        strcpy_s(current, str.size() + 1, str.c_str());
#else
        strcpy(current, str.c_str());
#endif
        results.push_back(current);
        current += str.length() + 1; // Move the pointer to the next position
    }

    // Assign the results to the ResultStruct
    result.results = results.data();
    result.size = results.size();

    return result;
}

void DeleteReaderPointer()
  {
    if (SerialReader_instance != NULL)
    {
      delete (static_cast<SerialReader*>(SerialReader_instance));
    }
  }
#ifdef __cplusplus
}
#endif