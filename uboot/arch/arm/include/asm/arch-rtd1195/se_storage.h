#ifndef __SE_STORAGE_LIB_H__
#define __SE_STORAGE_LIB_H__

#define SE_STORAGE_HEADER_FILE_NAME		"tmp/se_storage/"
#define SE_STORAGE_DATA_ADDR    (0x01500000)

int se_storage_dump_info(void);
int se_storage_reset(void);
int se_storage_delete(char *path);
int se_storage_read(char *path, char**buffer, int *length);
int se_storage_write(char *path, char *buffer, int length);
int se_storage_save(void);
int se_storage_init(void);

#endif /* __SE_STORAGE_LIB_H__ */

