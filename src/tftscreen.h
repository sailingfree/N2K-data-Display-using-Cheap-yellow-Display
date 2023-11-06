
typedef struct {
    double value;
    uint32_t x;
    uint32_t y;
    char type;
    String label;
    bool doneLabel;
    uint32_t oldx;
    uint32_t oldw;
    double oldvalue;
} Digital;

//void metersTask(void* param);
void metersSetup();
void metersWork();
void setMeter(uint16_t, double);
void setilabel(String&);


typedef enum {
    HOUSEV,
    HOUSEI,
    ENGINEV,
    RPM
} MeterIdx;
