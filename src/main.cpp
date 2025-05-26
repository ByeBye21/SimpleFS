#include <iostream>
#include <cstring>
#include "fs.h"

int main() {
    int choice;
    char filename[100], filename2[100];
    char data[1024];
    int size, offset, new_size;
    char backup_name[100];

    while (true) {
        std::cout << "\n--- SimpleFS Menu ---\n";
        std::cout << "1. Disk formatla (fs_format)\n";
        std::cout << "2. Dosya olustur (fs_create)\n";
        std::cout << "3. Dosya sil (fs_delete)\n";
        std::cout << "4. Dosyaya veri yaz (fs_write)\n";
        std::cout << "5. Dosyadan veri oku (fs_read)\n";
        std::cout << "6. Dosyalari listele (fs_ls)\n";
        std::cout << "7. Dosya yeniden adlandir (fs_rename)\n";
        std::cout << "8. Dosyanin varligini kontrol et (fs_exists)\n";
        std::cout << "9. Dosya boyutunu ogren (fs_size)\n";
        std::cout << "10. Dosyaya veri ekle (fs_append)\n";
        std::cout << "11. Dosya icerigini kisalt (fs_truncate)\n";
        std::cout << "12. Dosya kopyala (fs_copy)\n";
        std::cout << "13. Dosya tasi (fs_mv)\n";
        std::cout << "14. Disk defragmente et (fs_defragment)\n";
        std::cout << "15. Integrity kontrolu (fs_check_integrity)\n";
        std::cout << "16. Disk yedegi al (fs_backup)\n";
        std::cout << "17. Disk yedegini geri yukle (fs_restore)\n";
        std::cout << "18. Dosyayi goruntule (fs_cat)\n";
        std::cout << "19. Dosyalari karsilastir (fs_diff)\n";
        std::cout << "20. Cikis\n";
        std::cout << "Seciminiz: ";
        std::cin >> choice;
        
        switch(choice) {
            case 1:
                if (fs_format() == 0)
                    std::cout << "Disk formatlandi.\n";
                break;
            case 2:
                std::cout << "Dosya adi: ";
                std::cin >> filename;
                if (fs_create(filename) == 0)
                    std::cout << "Dosya olusturuldu.\n";
                break;
            case 3:
                std::cout << "Silinecek dosya adi: ";
                std::cin >> filename;
                if (fs_delete(filename) == 0)
                    std::cout << "Dosya silindi.\n";
                break;
            case 4:
                std::cout << "Dosya adi: ";
                std::cin >> filename;
                std::cout << "Yazilacak veri: ";
                std::cin.ignore(); // Gerekirse kalan newline karakterini temizle
                std::cin.getline(data, 1024);
                size = strlen(data);  // Gelen string uzunluğunu hesapla
                if (fs_write(filename, data, size) == 0)
                    std::cout << "Veri yazildi.\n";
                else
                    std::cout << "Veri yazma hatasi.\n";
                break;
            case 5:
                std::cout << "Dosya adi: ";
                std::cin >> filename;
                std::cout << "Baslangic offset: ";
                std::cin >> offset;
                std::cout << "Okunacak boyut: ";
                std::cin >> size;
                {
                    char buffer[1024];
                    int ret = fs_read(filename, offset, size, buffer);
                    if (ret > 0) {
                        buffer[ret] = '\0';
                        std::cout << "Okunan veri: " << buffer << "\n";
                    }
                }
                break;
            case 6:
                fs_ls();
                break;
            case 7:
                std::cout << "Eski dosya adi: ";
                std::cin >> filename;
                std::cout << "Yeni dosya adi: ";
                std::cin >> filename2;
                if (fs_rename(filename, filename2) == 0)
                    std::cout << "Dosya yeniden adlandirildi.\n";
                break;
            case 8:
                std::cout << "Dosya adi: ";
                std::cin >> filename;
                if (fs_exists(filename))
                    std::cout << "Dosya mevcut.\n";
                else
                    std::cout << "Dosya mevcut degil.\n";
                break;
            case 9:
                std::cout << "Dosya adi: ";
                std::cin >> filename;
                size = fs_size(filename);
                if (size >= 0)
                    std::cout << "Dosya boyutu: " << size << " bytes\n";
                break;
            case 10:
                std::cout << "Dosya adi: ";
                std::cin >> filename;
                std::cout << "Eklenecek veri: ";
                std::cin.ignore();
                std::cin.getline(data, 1024);
                size = strlen(data);
                if (fs_append(filename, data, size) == 0)
                    std::cout << "Veri eklendi.\n";
                else
                    std::cout << "Veri ekleme hatasi.\n";
                break;
            case 11:
                std::cout << "Dosya adi: ";
                std::cin >> filename;
                std::cout << "Yeni dosya boyutu: ";
                std::cin >> new_size;
                if (fs_truncate(filename, new_size) == 0)
                    std::cout << "Dosya kisaltildi.\n";
                break;
            case 12:
                std::cout << "Kaynak dosya adi: ";
                std::cin >> filename;
                std::cout << "Hedef dosya adi: ";
                std::cin >> filename2;
                if (fs_copy(filename, filename2) == 0)
                    std::cout << "Dosya kopyalandi.\n";
                break;
            case 13:
                std::cout << "Eski dosya adi: ";
                std::cin >> filename;
                std::cout << "Yeni dosya adi: ";
                std::cin >> filename2;
                if (fs_mv(filename, filename2) == 0)
                    std::cout << "Dosya tasindi.\n";
                break;
            case 14:
                if (fs_defragment() == 0)
                    std::cout << "Disk defragmente edildi.\n";
                break;
            case 15:
                if (fs_check_integrity() == 0)
                    std::cout << "Integrity kontrolu basarili.\n";
                else
                    std::cout << "Integrity kontrolu basarisiz.\n";
                break;
            case 16:
                std::cout << "Yedek dosya adi: ";
                std::cin >> backup_name;
                if (fs_backup(backup_name) == 0)
                    std::cout << "Disk yedegi alindi.\n";
                break;
            case 17:
                std::cout << "Yedek dosya adi: ";
                std::cin >> backup_name;
                if (fs_restore(backup_name) == 0)
                    std::cout << "Disk yedegi geri yuklendi.\n";
                break;
            case 18:
                std::cout << "Dosya adi: ";
                std::cin >> filename;
                fs_cat(filename);
                break;
            case 19:
                std::cout << "Birinci dosya adi: ";
                std::cin >> filename;
                std::cout << "Ikinci dosya adi: ";
                std::cin >> filename2;
                fs_diff(filename, filename2);
                break;
            case 20:
                std::cout << "Cikis yapiliyor...\n";
                return 0;
            default:
                std::cout << "Gecersiz secim, lutfen tekrar deneyin.\n";
                break;
        }
    }
    return 0;
}
