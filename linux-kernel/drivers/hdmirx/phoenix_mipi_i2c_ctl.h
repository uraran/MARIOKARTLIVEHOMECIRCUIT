#ifndef PHOENIX_MIPI_I2C_CTL_H
#define PHOENIX_MIPI_I2C_CTL_H

typedef enum {
    MIPI_CAMERA_RLE0551C = 1,
    MIPI_CAMERA_RTS5845 = 2,
    MIPI_CAMERA_OV5645 = 3,
    MIPI_CAMERA_OV5647 = 4,
    MIPI_CAMERA_OV9782 = 5,
} MIPI_CAMERA_TYPE;

enum CAMERA_FPS {
	CAMERA_FPS15 = 15,
	CAMERA_FPS25 = 25,
	CAMERA_FPS30 = 30,
	CAMERA_FPS60 = 60,
	CAMERA_FPS100 = 100,
	CAMERA_FPS120 = 120
};

unsigned int OV9782_get_frame_rate(void);
void OV9782_set_frame_rate(unsigned int fps, int stream_on);

struct i2c_adapter * mipi_i2c_init(void);
int mipi_i2c_read(struct i2c_adapter *p_adap, int len, unsigned char *buf);
int mipi_i2c_write(struct i2c_adapter *p_adap, int len,unsigned char *buf);

#endif
