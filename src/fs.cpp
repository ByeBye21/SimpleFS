#include "fs.h"
#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <vector>
#include <algorithm>

const char* DISK_FILENAME = "disk.sim";
const char* LOG_FILENAME  = "fs.log";

//-------------------------
// Yardımcı Fonksiyonlar
//-------------------------

// Disk üzerindeki metadata bilgisini yükler (ilk int file_count ardından MAX_FILES adet metadata)
static int load_metadata(FileMetadata files[], int *file_count) {
    int fd = open(DISK_FILENAME, O_RDWR);
    if (fd < 0) {
        perror("load_metadata: disk.sim acilamadi");
        return -1;
    }
    if (read(fd, file_count, sizeof(int)) != sizeof(int)) {
        perror("load_metadata: file_count okunurken hata");
        close(fd);
        return -1;
    }
    if (read(fd, files, sizeof(FileMetadata) * MAX_FILES) != sizeof(FileMetadata) * MAX_FILES) {
        perror("load_metadata: metadata okunurken hata");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

// Değiştirilmiş metadata bilgisini diske yazar
static int write_metadata(FileMetadata files[], int file_count) {
    int fd = open(DISK_FILENAME, O_RDWR);
    if (fd < 0) {
        perror("write_metadata: disk.sim acilamadi");
        return -1;
    }
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("write_metadata: lseek hatasi");
        close(fd);
        return -1;
    }
    if (write(fd, &file_count, sizeof(int)) != sizeof(int)) {
        perror("write_metadata: file_count yazilirken hata");
        close(fd);
        return -1;
    }
    if (write(fd, files, sizeof(FileMetadata) * MAX_FILES) != sizeof(FileMetadata) * MAX_FILES) {
        perror("write_metadata: metadata yazilirken hata");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

// Yeni dosya için veri bloğunun başlangıç offsetini hesaplar
static uint32_t get_new_file_start(FileMetadata files[], int file_count, int new_size) {
    uint32_t start = METADATA_SIZE; // Veri alanı metadata sonrasında başlar.
    for (int i = 0; i < MAX_FILES; i++) {
       if (files[i].valid) {
           uint32_t end = files[i].start + files[i].size;
           if (end > start)
                start = end;
       }
    }
    if (start + new_size > DISK_SIZE)
         return 0;
    return start;
}

// Belirtilen isimde dosyanın indeksini döner (bulamazsa -1)
static int find_file_index(const char* filename, FileMetadata files[], int file_count) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].valid && strcmp(files[i].name, filename) == 0)
            return i;
    }
    return -1;
}

//-------------------------
// Fonksiyonlar
//-------------------------

// fs_format: Disk içeriğini sıfırların ve boş metadata ile doldurarak formatlar.
int fs_format() {
    int fd = open(DISK_FILENAME, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
         perror("fs_format: disk.sim acilamadi");
         return -1;
    }
    if (ftruncate(fd, DISK_SIZE) < 0) {
         perror("fs_format: diskin boyutu ayarlanamadi");
         close(fd);
         return -1;
    }
    int file_count = 0;
    FileMetadata files[MAX_FILES];
    memset(files, 0, sizeof(files));
    if (lseek(fd, 0, SEEK_SET) < 0) {
         perror("fs_format: lseek hatasi");
         close(fd);
         return -1;
    }
    if (write(fd, &file_count, sizeof(int)) != sizeof(int)) {
         perror("fs_format: file_count yazilamadi");
         close(fd);
         return -1;
    }
    if (write(fd, files, sizeof(files)) != sizeof(files)) {
         perror("fs_format: metadata yazilamadi");
         close(fd);
         return -1;
    }
    close(fd);
    fs_log("Disk formatlandi");
    return 0;
}

// fs_create: Yeni bir dosya oluşturur ve metadata’ya kayıt ekler.
int fs_create(const char* filename) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    if (find_file_index(filename, files, file_count) != -1) {
         std::cerr << "fs_create: Dosya zaten mevcut\n";
         return -1;
    }
    int index = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].valid) {
            index = i;
            break;
        }
    }
    if (index == -1) {
         std::cerr << "fs_create: Bos metadata slotu yok\n";
         return -1;
    }
    uint32_t new_start = get_new_file_start(files, file_count, 0);  // Boyut 0, henüz veri yok
    if (new_start == 0) {
         std::cerr << "fs_create: Yeni dosya icin yer ayrilmadi\n";
         return -1;
    }
    files[index].valid = 1;
    strncpy(files[index].name, filename, sizeof(files[index].name) - 1);
    files[index].name[sizeof(files[index].name) - 1] = '\0';
    files[index].size = 0;
    files[index].start = new_start;
    files[index].creationTime = time(NULL);
    file_count++;
    if (write_metadata(files, file_count) < 0)
         return -1;
    fs_log((std::string("Dosya olusturuldu: ") + filename).c_str());
    return 0;
}

// fs_delete: Belirtilen dosyayı siler, metadata’da geçersiz kılar.
int fs_delete(const char* filename) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    int index = find_file_index(filename, files, file_count);
    if (index == -1) {
         std::cerr << "fs_delete: Dosya bulunamadi\n";
         return -1;
    }
    files[index].valid = 0;
    file_count--;
    if (write_metadata(files, file_count) < 0)
         return -1;
    fs_log((std::string("Dosya silindi: ") + filename).c_str());
    return 0;
}

// fs_write: Dosyanın içeriğini, verilen veri ile (eski içeriğin üzerine) yazar.
int fs_write(const char* filename, const char* data, int size) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    int index = find_file_index(filename, files, file_count);
    if (index == -1) {
         std::cerr << "fs_write: Dosya bulunamadi\n";
         return -1;
    }
    uint32_t new_start = get_new_file_start(files, file_count, size);
    if (new_start == 0) {
         std::cerr << "fs_write: Yeterli alan yok\n";
         return -1;
    }
    int fd = open(DISK_FILENAME, O_RDWR);
    if (fd < 0) {
         perror("fs_write: disk.sim acilamadi");
         return -1;
    }
    if (lseek(fd, new_start, SEEK_SET) < 0) {
         perror("fs_write: lseek hatasi");
         close(fd);
         return -1;
    }
    if (write(fd, data, size) != size) {
         perror("fs_write: yazma hatasi");
         close(fd);
         return -1;
    }
    close(fd);
    files[index].start = new_start;
    files[index].size = size;
    if (write_metadata(files, file_count) < 0)
         return -1;
    fs_log((std::string("Veri yazldi: ") + filename).c_str());
    return 0;
}

// fs_read: Dosyadan, belirtilen offset'ten başlayarak, istenen boyutta veri okur.
int fs_read(const char* filename, int offset, int size, char* buffer) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    int index = find_file_index(filename, files, file_count);
    if (index == -1) {
         std::cerr << "fs_read: Dosya bulunamadi\n";
         return -1;
    }
    if (offset + size > (int)files[index].size) {
         std::cerr << "fs_read: Okuma, dosya boyutunu asiyor\n";
         return -1;
    }
    int fd = open(DISK_FILENAME, O_RDONLY);
    if (fd < 0) {
         perror("fs_read: disk.sim acilamadi");
         return -1;
    }
    if (lseek(fd, files[index].start + offset, SEEK_SET) < 0) {
         perror("fs_read: lseek hatasi");
         close(fd);
         return -1;
    }
    if (read(fd, buffer, size) != size) {
         perror("fs_read: okuma hatasi");
         close(fd);
         return -1;
    }
    close(fd);
    return size;
}

// fs_ls: Diskteki tüm dosyaların isimlerini ve boyutlarını listeler.
int fs_ls() {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    std::cout << "Dosya Listesi:\n";
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].valid) {
            std::cout << "Dosya: " << files[i].name << ", Boyut: " << files[i].size << " bytes\n";
        }
    }
    fs_log("Dosyalar listelendi");
    return 0;
}

// fs_rename: Dosyanın ismini değiştirir, metadata’da güncelleme yapar.
int fs_rename(const char* old_name, const char* new_name) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    int index = find_file_index(old_name, files, file_count);
    if (index == -1) {
         std::cerr << "fs_rename: Eski dosya bulunamadi\n";
         return -1;
    }
    if (find_file_index(new_name, files, file_count) != -1) {
         std::cerr << "fs_rename: Yeni isimde dosya zaten mevcut\n";
         return -1;
    }
    strncpy(files[index].name, new_name, sizeof(files[index].name)-1);
    files[index].name[sizeof(files[index].name)-1] = '\0';
    if (write_metadata(files, file_count) < 0)
         return -1;
    fs_log((std::string("Dosya yeniden adlandirildi: ") + old_name + " -> " + new_name).c_str());
    return 0;
}

// fs_exists: Dosyanın var olup olmadığını kontrol eder (1/0 olarak döner).
int fs_exists(const char* filename) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return 0;
    int index = find_file_index(filename, files, file_count);
    return (index != -1) ? 1 : 0;
}

// fs_size: Dosyanın boyutunu metadata'dan döner.
int fs_size(const char* filename) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    int index = find_file_index(filename, files, file_count);
    if (index == -1) {
         std::cerr << "fs_size: Dosya bulunamadi\n";
         return -1;
    }
    return files[index].size;
}

// fs_append: Dosyanın mevcut içeriğinin sonuna, veriyi ekler.
// Uygulamada, dosyanın eski içeriğini okuyup, ekleyeceğimiz veriyi yeni bir alana topluca yazıyoruz.
int fs_append(const char* filename, const char* data, int size) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    int index = find_file_index(filename, files, file_count);
    if (index == -1) {
         std::cerr << "fs_append: Dosya bulunamadi\n";
         return -1;
    }
    int old_size = files[index].size;
    int new_total_size = old_size + size;
    char* temp = new char[new_total_size];
    if (old_size > 0) {
         if (fs_read(filename, 0, old_size, temp) < 0) {
             delete[] temp;
             return -1;
         }
    }
    memcpy(temp + old_size, data, size);
    uint32_t new_start = get_new_file_start(files, file_count, new_total_size);
    if (new_start == 0) {
         std::cerr << "fs_append: Yeterli alan yok\n";
         delete[] temp;
         return -1;
    }
    int fd = open(DISK_FILENAME, O_RDWR);
    if (fd < 0) {
         perror("fs_append: disk.sim acilamadi");
         delete[] temp;
         return -1;
    }
    if (lseek(fd, new_start, SEEK_SET) < 0) {
         perror("fs_append: lseek hatasi");
         close(fd);
         delete[] temp;
         return -1;
    }
    if (write(fd, temp, new_total_size) != new_total_size) {
         perror("fs_append: yazma hatasi");
         close(fd);
         delete[] temp;
         return -1;
    }
    close(fd);
    files[index].start = new_start;
    files[index].size = new_total_size;
    if (write_metadata(files, file_count) < 0) {
         delete[] temp;
         return -1;
    }
    delete[] temp;
    fs_log((std::string("Veri eklendi: ") + filename).c_str());
    return 0;
}

// fs_truncate: Dosyanın mevcut içeriğinin, belirtilen yeni boyuta kadar olan kısmını kalır.
int fs_truncate(const char* filename, int new_size) {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    int index = find_file_index(filename, files, file_count);
    if (index == -1) {
         std::cerr << "fs_truncate: Dosya bulunamadi\n";
         return -1;
    }
    if (new_size > (int)files[index].size) {
         std::cerr << "fs_truncate: Yeni boyut, mevcut boyuttan buyuk olamaz\n";
         return -1;
    }
    char* temp = new char[new_size];
    if (new_size > 0) {
         if (fs_read(filename, 0, new_size, temp) < 0) {
             delete[] temp;
             return -1;
         }
    }
    uint32_t new_start = get_new_file_start(files, file_count, new_size);
    if (new_start == 0) {
         std::cerr << "fs_truncate: Yeterli alan yok\n";
         delete[] temp;
         return -1;
    }
    int fd = open(DISK_FILENAME, O_RDWR);
    if (fd < 0) {
         perror("fs_truncate: disk.sim acilamadi");
         delete[] temp;
         return -1;
    }
    if (lseek(fd, new_start, SEEK_SET) < 0) {
         perror("fs_truncate: lseek hatasi");
         close(fd);
         delete[] temp;
         return -1;
    }
    if (write(fd, temp, new_size) != new_size) {
         perror("fs_truncate: yazma hatasi");
         close(fd);
         delete[] temp;
         return -1;
    }
    close(fd);
    files[index].start = new_start;
    files[index].size = new_size;
    if (write_metadata(files, file_count) < 0) {
         delete[] temp;
         return -1;
    }
    delete[] temp;
    fs_log((std::string("Dosya kirpildi: ") + filename).c_str());
    return 0;
}

// fs_copy: Kaynak dosyanın içeriğini, hedef dosyaya kopyalar.
int fs_copy(const char* src_filename, const char* dest_filename) {
    int src_size = fs_size(src_filename);
    if (src_size < 0) {
         std::cerr << "fs_copy: Kaynak dosya bulunamadi\n";
         return -1;
    }
    char* buffer = new char[src_size];
    if (fs_read(src_filename, 0, src_size, buffer) < 0) {
         delete[] buffer;
         return -1;
    }
    if (fs_create(dest_filename) < 0) {
         std::cerr << "fs_copy: Hedef dosya olusturulamadi\n";
         delete[] buffer;
         return -1;
    }
    if (fs_write(dest_filename, buffer, src_size) < 0) {
         std::cerr << "fs_copy: Yazma hatasi\n";
         delete[] buffer;
         return -1;
    }
    delete[] buffer;
    fs_log((std::string("Dosya kopyalandi: ") + src_filename + " -> " + dest_filename).c_str());
    return 0;
}

// fs_mv: Dosyayı başka bir isimle taşır; burada basitçe yeniden adlandırma yapılır.
int fs_mv(const char* old_path, const char* new_path) {
    return fs_rename(old_path, new_path);
}

// fs_defragment: Disk üzerindeki parçalı veri bloklarını düzenler; tüm valid dosyaların verisini sıralı olarak yeni alana yazar.
int fs_defragment() {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    std::vector<FileMetadata> valid_files;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].valid)
            valid_files.push_back(files[i]);
    }
    std::sort(valid_files.begin(), valid_files.end(), [](const FileMetadata &a, const FileMetadata &b) {
         return a.start < b.start;
    });
    int fd = open(DISK_FILENAME, O_RDWR);
    if (fd < 0) {
         perror("fs_defragment: disk.sim acilamadi");
         return -1;
    }
    uint32_t current_offset = METADATA_SIZE;
    for (size_t i = 0; i < valid_files.size(); i++) {
         if (valid_files[i].start != current_offset) {
             char* buffer = new char[valid_files[i].size];
             if (lseek(fd, valid_files[i].start, SEEK_SET) < 0) {
                 perror("fs_defragment: lseek hatasi");
                 delete[] buffer;
                 close(fd);
                 return -1;
             }
             if (read(fd, buffer, valid_files[i].size) != (ssize_t)valid_files[i].size) {
                 perror("fs_defragment: okuma hatasi");
                 delete[] buffer;
                 close(fd);
                 return -1;
             }
             if (lseek(fd, current_offset, SEEK_SET) < 0) {
                 perror("fs_defragment: lseek hatasi");
                 delete[] buffer;
                 close(fd);
                 return -1;
             }
             if (write(fd, buffer, valid_files[i].size) != (ssize_t)valid_files[i].size) {
                 perror("fs_defragment: yazma hatasi");
                 delete[] buffer;
                 close(fd);
                 return -1;
             }
             // Metadata güncellemesi: Aynı isme sahip dosyayı bulup yeni start değerini ata.
             for (int j = 0; j < MAX_FILES; j++) {
                 if (files[j].valid && strcmp(files[j].name, valid_files[i].name) == 0) {
                      files[j].start = current_offset;
                      break;
                 }
             }
             delete[] buffer;
         }
         current_offset += valid_files[i].size;
    }
    close(fd);
    if (write_metadata(files, file_count) < 0)
         return -1;
    fs_log("Disk defragmente edildi");
    return 0;
}

// fs_check_integrity: Metadata ve veri bloklarının tutarlılığını kontrol eder.
int fs_check_integrity() {
    FileMetadata files[MAX_FILES];
    int file_count;
    if (load_metadata(files, &file_count) < 0)
         return -1;
    bool integrityOk = true;
    for (int i = 0; i < MAX_FILES; i++) {
         if (files[i].valid) {
             if (files[i].start < METADATA_SIZE || files[i].start + files[i].size > DISK_SIZE) {
                 std::cerr << "fs_check_integrity: " << files[i].name << " dosyasinda tutarsizlik bulundu\n";
                 integrityOk = false;
             }
         }
    }
    fs_log("Integrity kontrolu yapildi");
    return integrityOk ? 0 : -1;
}

// fs_backup: Tüm disk dosyasının yedeğini alır.
int fs_backup(const char* backup_filename) {
    int src_fd = open(DISK_FILENAME, O_RDONLY);
    if (src_fd < 0) {
         perror("fs_backup: disk.sim acilamadi");
         return -1;
    }
    int dest_fd = open(backup_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dest_fd < 0) {
         perror("fs_backup: backup dosyasi acilamadi");
         close(src_fd);
         return -1;
    }
    char buffer[1024];
    ssize_t bytes;
    while ((bytes = read(src_fd, buffer, sizeof(buffer))) > 0) {
         if (write(dest_fd, buffer, bytes) != bytes) {
             perror("fs_backup: yazma hatasi");
             close(src_fd);
             close(dest_fd);
             return -1;
         }
    }
    close(src_fd);
    close(dest_fd);
    fs_log((std::string("Disk yedegi alindi: ") + backup_filename).c_str());
    return 0;
}

// fs_restore: Yedek dosyasını, mevcut disk.sim dosyasının yerine geri yükler.
int fs_restore(const char* backup_filename) {
    int src_fd = open(backup_filename, O_RDONLY);
    if (src_fd < 0) {
         perror("fs_restore: backup dosyasi acilamadı");
         return -1;
    }
    int dest_fd = open(DISK_FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dest_fd < 0) {
         perror("fs_restore: disk.sim acilamadi");
         close(src_fd);
         return -1;
    }
    char buffer[1024];
    ssize_t bytes;
    while ((bytes = read(src_fd, buffer, sizeof(buffer))) > 0) {
         if (write(dest_fd, buffer, bytes) != bytes) {
             perror("fs_restore: yazma hatasi");
             close(src_fd);
             close(dest_fd);
             return -1;
         }
    }
    close(src_fd);
    close(dest_fd);
    fs_log((std::string("Disk yedegi geri yuklendi: ") + backup_filename).c_str());
    return 0;
}

// fs_cat: Dosyanın içeriğini ekrana yazdırır.
int fs_cat(const char* filename) {
    int size = fs_size(filename);
    if (size < 0)
         return -1;
    char* buffer = new char[size+1];
    if (fs_read(filename, 0, size, buffer) < 0) {
         delete[] buffer;
         return -1;
    }
    buffer[size] = '\0';
    std::cout << buffer << "\n";
    delete[] buffer;
    fs_log((std::string("Dosya goruntulendi (cat): ") + filename).c_str());
    return 0;
}

// fs_diff: İki dosyanın içeriğini karşılaştırır.
int fs_diff(const char* file1, const char* file2) {
    int size1 = fs_size(file1);
    int size2 = fs_size(file2);
    if (size1 < 0 || size2 < 0)
         return -1;
    if (size1 != size2) {
         std::cout << "Dosyalar farkli boyutta.\n";
         fs_log("Dosyalar farkli (diff): boyutlar uyumsuz");
         return 0;
    }
    char* buffer1 = new char[size1];
    char* buffer2 = new char[size2];
    if (fs_read(file1, 0, size1, buffer1) < 0 || fs_read(file2, 0, size2, buffer2) < 0) {
         delete[] buffer1;
         delete[] buffer2;
         return -1;
    }
    bool identical = (memcmp(buffer1, buffer2, size1) == 0);
    if (identical)
         std::cout << "Dosyalar ayni.\n";
    else
         std::cout << "Dosyalar farkli.\n";
    delete[] buffer1;
    delete[] buffer2;
    fs_log((std::string("Dosya karsilastirmasi (diff) yapildi: ") + file1 + " ve " + file2).c_str());
    return 0;
}

// fs_log: Yapılan işlemleri, zaman damgalı olarak log dosyasına ekler.
int fs_log(const char* operation) {
    int fd = open(LOG_FILENAME, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
         perror("fs_log: Log dosyasi acilamadi");
         return -1;
    }
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    std::string log_entry = std::string(time_str) + " - " + operation + "\n";
    if (write(fd, log_entry.c_str(), log_entry.length()) != (ssize_t)log_entry.length()) {
         perror("fs_log: Yazma hatasi");
         close(fd);
         return -1;
    }
    close(fd);
    return 0;
}
