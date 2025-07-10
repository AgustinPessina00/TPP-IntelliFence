#ifndef SENSORDATATYPE_H
#define SENSORDATATYPE_H

struct Acceleration { 
    float ax; 
    float ay; 
    float az; 
};

struct Gyroscope { 
    float gx; 
    float gy; 
    float gz; 
};

#endif /* SENSORDATATYPE_H */