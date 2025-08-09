#ifndef ZONE_H
#define ZONE_H

typedef enum {
    GREEN_ZONE       = 0x00,  //  Dentro del cerco virtual (sin estímulo)
    
    LIGHT_BLUE_ZONE  = 0x01,  //  Buzzer leve (frecuencia baja, duty bajo)
    BLUE_ZONE        = 0x02,  //  Buzzer medio
    DARK_BLUE_ZONE   = 0x03,  //  Buzzer intenso
    
    YELLOW_ZONE      = 0x04,  //  Buzzer + vibración
    
    RED_ZONE         = 0x05,  //  Vibración intensa (sin shock)
    BLACK_ZONE       = 0x06   //  Se escapó → alerta crítica
} zone_t;

#endif