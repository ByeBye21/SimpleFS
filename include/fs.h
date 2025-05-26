#ifndef FS_H
#define FS_H

#include <cstdint>
#include <ctime>

const int METADATA_SIZE = 65536;               // Metadata alanı için ayrılmış alan
const int DISK_SIZE = 10 * 1024 * 1024;        // 10 MB disk
const int BLOCK_SIZE = 512;                    // Sabit blok boyutu
const int MAX_FILES = 100;                     // Maksimum dosya sayısı

#pragma pack(push, 1)
struct FileMetadata {
    uint8_t valid;         // 0: boş, 1: dolu
    char name[100];        // Dosya ismi
    uint32_t size;         // Dosya boyutu (byte)
    uint32_t start;        // Veri alanındaki başlangıç offseti
    time_t creationTime;   // Dosya oluşturulma zamanı
};
#pragma pack(pop)

/// Fonksiyon prototipleri ///
int fs_create(const char* filename);
int fs_delete(const char* filename);
int fs_write(const char* filename, const char* data, int size);
int fs_read(const char* filename, int offset, int size, char* buffer);
int fs_ls();
int fs_format();
int fs_rename(const char* old_name, const char* new_name);
int fs_exists(const char* filename);
int fs_size(const char* filename);
int fs_append(const char* filename, const char* data, int size);
int fs_truncate(const char* filename, int new_size);
int fs_copy(const char* src_filename, const char* dest_filename);
int fs_mv(const char* old_path, const char* new_path);
int fs_defragment();
int fs_check_integrity();
int fs_backup(const char* backup_filename);
int fs_restore(const char* backup_filename);
int fs_cat(const char* filename);
int fs_diff(const char* file1, const char* file2);
int fs_log(const char* operation);

#endif // FS_H
