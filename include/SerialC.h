#ifndef SERIALC_H 
#define SERIALC_H 

#ifdef __cplusplus
extern "C" {
#endif
 
struct ResultStruct {
    unsigned long long size;
    const char** results;
};

void InitialiseReader(const char* id, unsigned baud, unsigned buffer_size);

int Connect(void);

void StartRead(void);

void StopRead(void);

void Delay(unsigned seconds);

struct ResultStruct GetCTypeArray(void);

void DeleteReaderPointer(void);

#ifdef __cplusplus
}
#endif


#endif //SERIALC_H